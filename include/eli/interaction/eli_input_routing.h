/**
 * @file eli_input_routing.h
 * @brief Shortcut routing, per-key ownership, and the capture-intent outputs
 *        (io.want_capture_mouse / want_capture_keyboard / want_text_input).
 *
 * Routes for eli_shortcut_ex (spec §9.3, D15):
 *   - ELI_INPUT_ROUTE_FOCUSED (default, eli_shortcut): fires when the calling
 *     window is the focused window or in its tree. Called outside any window it
 *     behaves like GLOBAL (app-level code).
 *   - ELI_INPUT_ROUTE_GLOBAL: fires anywhere unless an active item owns the key.
 *   - ELI_INPUT_ROUTE_ACTIVE: fires only for the active item (call it right after
 *     the item, so the last item is the active one).
 * A widget that is using keys claims them with eli_set_key_owner each frame (the
 * active input_text owns its edit keys and the edit chords' keys), so GLOBAL and
 * FOCUSED shortcuts on those keys do not fire while typing. Apps with their own
 * keymap use GLOBAL and gate on io.want_capture_keyboard.
 *
 * @status In use.
 * @issues None
 * @todo Keyboard navigation would also set want_capture_keyboard (no nav yet).
 */
#ifndef ELI_INTERACTION_ELI_INPUT_ROUTING_H
#define ELI_INTERACTION_ELI_INPUT_ROUTING_H

#include "../core/eli_core.h"
#include "../input/eli_input.h"
#include "../window/eli_window_types.h"

/** Routing / behavior flags for eli_shortcut_ex. */
typedef int eli_input_flags;
enum eli_input_flags_ {
    ELI_INPUT_ROUTE_FOCUSED = 0,       /* default */
    ELI_INPUT_ROUTE_GLOBAL  = 1 << 0,
    ELI_INPUT_ROUTE_ACTIVE  = 1 << 1,
    ELI_INPUT_REPEAT        = 1 << 2   /* also fire on typematic repeats */
};

/**
 * Claim `key` for item `owner_id` (0 releases) for the next frame. Call every
 * frame while the item uses the key; claims lapse when no longer renewed.
 *
 * @param key       Key index (modifier bits are ignored).
 * @param owner_id  Owning item id.
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline void eli_set_key_owner(eli_key key, eli_id owner_id)
{
    eli_context *ctx = eli_get_current_context();
    key = (eli_key)(key & ~ELI_MOD_MASK);
    if (!ctx || !eli_key_is_valid(key))
        return;
    ctx->key_owner_next[key] = owner_id;
}

/** @return the item that owns `key` this frame, or 0. */
static inline eli_id eli_get_key_owner(eli_key key)
{
    eli_context *ctx = eli_get_current_context();
    key = (eli_key)(key & ~ELI_MOD_MASK);
    if (!ctx || !eli_key_is_valid(key))
        return 0u;
    return ctx->key_owner[key];
}

/** @return true if window `w` is the focused window or shares its root. */
static inline bool eli_input__window_in_focus_tree(const eli_context *ctx, const eli_window *w)
{
    const eli_window *nav = ctx->nav_window;
    if (!nav)
        return false;
    const eli_window *nav_root = nav->root_window ? nav->root_window : nav;
    const eli_window *w_root = w->root_window ? w->root_window : w;
    return w == nav || w_root == nav_root;
}

/**
 * Test a key chord through a route (see the file header).
 *
 * @param key_chord  ELI_MOD_* (incl. ELI_MOD_SHORTCUT) OR'd with an eli_key.
 * @param flags      ELI_INPUT_ROUTE_* and optionally ELI_INPUT_REPEAT.
 * @return           true if the chord fired this frame for this route.
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline bool eli_shortcut_ex(eli_key_chord key_chord, eli_input_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (!ctx)
        return false;
    eli_key key = (eli_key)(key_chord & ~ELI_MOD_MASK);
    bool fired = (flags & ELI_INPUT_REPEAT)
                     ? (eli_is_key_chord_pressed(key_chord & ELI_MOD_MASK) &&
                        eli_is_key_pressed_ex(key, true))
                     : eli_is_key_chord_pressed(key_chord);
    if (!fired)
        return false;
    eli_id owner = eli_key_is_valid(key) ? ctx->key_owner[key] : 0u;
    if (flags & ELI_INPUT_ROUTE_ACTIVE)
        return ctx->active_id != 0u && ctx->active_id == ctx->last_item_id &&
               (owner == 0u || owner == ctx->active_id);
    if (owner != 0u)
        return false;                  /* an active item is using this key */
    if (flags & ELI_INPUT_ROUTE_GLOBAL)
        return true;
    if (!ctx->current_window)
        return true;                   /* app-level code outside any window */
    return eli_input__window_in_focus_tree(ctx, ctx->current_window);
}

/**
 * Test a shortcut chord on the FOCUSED route (see eli_shortcut_ex).
 *
 * @param key_chord  ELI_MOD_* flags OR'd with an eli_key (e.g. ELI_SHORTCUT_COPY).
 * @return           true if the chord fired this frame for the calling window.
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline bool eli_shortcut(eli_key_chord key_chord)
{
    return eli_shortcut_ex(key_chord, ELI_INPUT_ROUTE_FOCUSED);
}

/** Mark that a text-input widget is active this frame (sets io.want_text_input). */
static inline void eli_input_mark_text_input_active(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx)
        ctx->want_text_input_next = true;
}

/**
 * Begin-frame roll: last frame's key claims become this frame's owners. Called
 * by eli_frame_begin after the input update.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_input_routing_new_frame(void)
{
    eli_context *ctx = eli_get_current_context();
    if (!ctx)
        return;
    memcpy(ctx->key_owner, ctx->key_owner_next, sizeof(ctx->key_owner));
    memset(ctx->key_owner_next, 0, sizeof(ctx->key_owner_next));
}

/**
 * End-of-frame capture outputs, for the app to decide whether an input event is
 * elimgui's (computed from this frame; next-frame overrides still win):
 *   want_capture_mouse    = no button held and the mouse is over an elimgui window,
 *                           or a button is held that was pressed over one (a drag
 *                           that started in the app stays the app's);
 *   want_capture_keyboard = an item active;
 *   want_text_input       = a text widget active.
 * Called by eli_frame_end.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_input_update_capture_flags(void)
{
    eli_context *ctx = eli_get_current_context();
    if (!ctx)
        return;
    eli_io *io = &ctx->io;
    bool any_down = false, held_owned = false;
    for (int b = 0; b < ELI_MOUSE_BUTTON_COUNT; b++) {
        if (io->mouse_clicked[b])
            ctx->mouse_down_owned[b] = ctx->hovered_window != NULL;
        if (!io->mouse_down[b])
            ctx->mouse_down_owned[b] = false;
        any_down = any_down || io->mouse_down[b];
        held_owned = held_owned || ctx->mouse_down_owned[b];
    }
    io->want_capture_mouse = any_down ? held_owned : ctx->hovered_window != NULL;
    io->want_capture_keyboard = ctx->active_id != 0u;
    io->want_text_input = ctx->want_text_input_next;
    ctx->want_text_input_next = false;
    if (io->want_capture_mouse_next_frame != ELI_CAPTURE_OVERRIDE_NONE)
        io->want_capture_mouse = io->want_capture_mouse_next_frame == ELI_CAPTURE_OVERRIDE_TRUE;
    if (io->want_capture_keyboard_next_frame != ELI_CAPTURE_OVERRIDE_NONE)
        io->want_capture_keyboard =
            io->want_capture_keyboard_next_frame == ELI_CAPTURE_OVERRIDE_TRUE;
}

#endif /* ELI_INTERACTION_ELI_INPUT_ROUTING_H */
