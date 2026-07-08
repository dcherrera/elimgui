/**
 * @file eli_popup.h
 * @brief Phase 17 popups and modals: the open-popup stack and begin-popup stack,
 *        the open/close/query API (eli_open_popup / eli_close_current_popup /
 *        eli_is_popup_open), popup windows (eli_begin_popup / eli_end_popup and the
 *        context-menu openers), and modal dialogs with a dimmed backdrop.
 *
 * A popup IS a window: each popup composes ELI_WINDOW_POPUP with the no-title /
 * no-resize / auto-resize window flags and reuses eli_begin_ex / eli_end for
 * layout, focus, and z-order. This module owns two file-static stacks — the open
 * popups (id + owner window + open frame + open mouse pos) and the begin-popup
 * nesting stack — and drives per-frame maintenance through eli_popup_new_frame /
 * eli_popup_end_frame, which the frame orchestrator composes right after
 * eli_window_new_frame (so the hovered window is known) and before widgets run.
 *
 * @status Phase 17 popups and modals in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WIDGETS_ELI_POPUP_H
#define ELI_WIDGETS_ELI_POPUP_H

#include "eli_item_status.h"

#include "../core/eli_platform.h"
#include "../window/eli_window.h"
#include "../input/eli_input.h"

#include <stdio.h>

/* ---------------------------------------------------------------------------
 * Flags
 * ------------------------------------------------------------------------- */

/* Popup interaction flags (mirrors Dear ImGui's ImGuiPopupFlags). Defined here
 * because popups are the first consumer; the combo/menu phases reuse them. The
 * low bits carry the mouse button used by the context-menu openers. */
#ifndef ELI_POPUP_NONE
typedef int eli_popup_flags;
enum eli_popup_flags_ {
    ELI_POPUP_NONE                        = 0,
    ELI_POPUP_MOUSE_BUTTON_LEFT           = 0,
    ELI_POPUP_MOUSE_BUTTON_RIGHT          = 1,
    ELI_POPUP_MOUSE_BUTTON_MIDDLE         = 2,
    ELI_POPUP_MOUSE_BUTTON_MASK_          = 0x1f,
    ELI_POPUP_NO_REOPEN                   = 1 << 5,
    ELI_POPUP_NO_OPEN_OVER_EXISTING_POPUP = 1 << 7,
    ELI_POPUP_NO_OPEN_OVER_ITEMS          = 1 << 8,
    ELI_POPUP_ANY_POPUP_ID                = 1 << 10,
    ELI_POPUP_ANY_POPUP_LEVEL             = 1 << 11,
    ELI_POPUP_ANY_POPUP                   = ELI_POPUP_ANY_POPUP_ID | ELI_POPUP_ANY_POPUP_LEVEL
};
#endif /* ELI_POPUP_NONE */

/* Internal window-flag markers for popup/modal windows. High bits keep them clear
 * of the public eli_window_flags (which only reach bit 18). Guarded so a later
 * phase may promote them into eli_enums.h without a redefinition clash. */
#ifndef ELI_WINDOW_POPUP
#define ELI_WINDOW_POPUP (1 << 24)
#endif
#ifndef ELI_WINDOW_MODAL
#define ELI_WINDOW_MODAL (1 << 25)
#endif

/* Maximum simultaneously-open (or nested) popups. */
#define ELI_POPUP_STACK_MAX 32

/* ---------------------------------------------------------------------------
 * Popup state
 * ------------------------------------------------------------------------- */

/**
 * One popup's bookkeeping. Lives in both the open-popup stack (a popup that has
 * been opened and not yet closed) and the begin-popup stack (a popup currently
 * being emitted this frame). `window` is NULL until the popup is first begun.
 */
typedef struct eli_popup_data {
    eli_id      popup_id;         /* hashed id used to match open/begin/query */
    eli_window *window;           /* the popup window (NULL until first begun) */
    eli_window *parent_window;    /* window that owned the cursor when opened */
    int         open_frame_count; /* frame index the popup was opened */
    eli_vec2    open_popup_pos;   /* preferred top-left position on appear */
    eli_vec2    open_mouse_pos;   /* mouse position captured when opened */
    bool        is_modal;         /* begun via eli_begin_popup_modal */
} eli_popup_data;

/* The currently-open popups, outer-to-inner. */
static eli_popup_data g_eli_open_popup_stack[ELI_POPUP_STACK_MAX];
static int            g_eli_open_popup_count = 0;

/* The popups being emitted this frame (pushed by begin, popped by end). */
static eli_popup_data g_eli_begin_popup_stack[ELI_POPUP_STACK_MAX];
static int            g_eli_begin_popup_count = 0;

/* ---------------------------------------------------------------------------
 * Queries
 * ------------------------------------------------------------------------- */

/**
 * Test whether a popup id is open, honoring the AnyPopup gating flags.
 *
 * @param id     Popup id (ignored when ELI_POPUP_ANY_POPUP_ID is set).
 * @param flags  eli_popup_flags controlling level/any gating.
 * @return       true if the requested popup is open.
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline bool eli_is_popup_open_id(eli_id id, eli_popup_flags flags)
{
    if (flags & ELI_POPUP_ANY_POPUP_ID) {
        if (flags & ELI_POPUP_ANY_POPUP_LEVEL)
            return g_eli_open_popup_count > 0;
        return g_eli_open_popup_count > g_eli_begin_popup_count;
    }
    if (flags & ELI_POPUP_ANY_POPUP_LEVEL) {
        for (int n = 0; n < g_eli_open_popup_count; n++)
            if (g_eli_open_popup_stack[n].popup_id == id)
                return true;
        return false;
    }
    return g_eli_open_popup_count > g_eli_begin_popup_count &&
           g_eli_open_popup_stack[g_eli_begin_popup_count].popup_id == id;
}

/**
 * Test whether the popup identified by a string id is open at the current
 * begin-nesting level (or any level/any id per flags).
 *
 * @param str_id  Popup string id (hashed against the current id seed).
 * @param flags   eli_popup_flags gating.
 * @return        true if open.
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline bool eli_is_popup_open(const char *str_id, eli_popup_flags flags)
{
    eli_id id = (flags & ELI_POPUP_ANY_POPUP_ID) ? 0u : eli_get_id(str_id);
    return eli_is_popup_open_id(id, flags);
}

/* ---------------------------------------------------------------------------
 * Opening
 * ------------------------------------------------------------------------- */

/**
 * Open a popup by id at the current begin-nesting level. Replaces any existing
 * popup (and closes deeper ones) at that level unless it is a same-id reopen of a
 * popup opened last frame, or ELI_POPUP_NO_REOPEN forbids re-opening.
 *
 * @param id     Popup id to open.
 * @param flags  eli_popup_flags (NoReopen / NoOpenOverExistingPopup honored).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_open_popup_id(eli_id id, eli_popup_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;

    int level = g_eli_begin_popup_count;
    if (level + 1 > ELI_POPUP_STACK_MAX)
        return;
    if ((flags & ELI_POPUP_NO_OPEN_OVER_EXISTING_POPUP) &&
        eli_is_popup_open_id(0u, ELI_POPUP_ANY_POPUP_ID))
        return;

    eli_vec2 mouse = eli_get_mouse_pos();
    eli_popup_data ref;
    ref.popup_id = id;
    ref.window = NULL;
    ref.parent_window = ctx->current_window;
    ref.open_frame_count = ctx->frame_count;
    ref.open_popup_pos = eli_mouse_pos_is_valid(mouse) ? mouse : eli_make_vec2(0.0f, 0.0f);
    ref.open_mouse_pos = ref.open_popup_pos;
    ref.is_modal = false;

    if (g_eli_open_popup_count < level + 1) {
        g_eli_open_popup_stack[g_eli_open_popup_count++] = ref;
        return;
    }
    /* A popup already occupies this level. */
    if (g_eli_open_popup_stack[level].popup_id == id &&
        g_eli_open_popup_stack[level].open_frame_count == ctx->frame_count - 1) {
        /* Same popup re-asserted a frame later: keep its window, refresh frame. */
        g_eli_open_popup_stack[level].open_frame_count = ref.open_frame_count;
        return;
    }
    if ((flags & ELI_POPUP_NO_REOPEN) && g_eli_open_popup_stack[level].popup_id == id)
        return;
    g_eli_open_popup_count = level + 1;
    g_eli_open_popup_stack[level] = ref;
}

/**
 * Open a popup by string id (hashed against the current id seed).
 *
 * @param str_id  Popup string id.
 * @param flags   eli_popup_flags.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_open_popup(const char *str_id, eli_popup_flags flags)
{
    if (str_id == NULL)
        return;
    eli_open_popup_id(eli_get_id(str_id), flags);
}

/**
 * Open a popup when the last-submitted item is clicked with the popup's mouse
 * button (default right). Uses the release edge to match Dear ImGui. When str_id
 * is NULL/empty the last item's id is used as the popup id.
 *
 * @param str_id  Popup string id, or NULL to use the last item's id.
 * @param flags   eli_popup_flags; the low bits select the mouse button.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_open_popup_on_item_click(const char *str_id, eli_popup_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    int button = flags & ELI_POPUP_MOUSE_BUTTON_MASK_;
    if (eli_is_mouse_released((eli_mouse_button)button) &&
        eli_is_item_hovered(ELI_HOVERED_ALLOW_WHEN_BLOCKED_BY_POPUP)) {
        eli_id id = (str_id != NULL && str_id[0] != '\0') ? eli_get_id(str_id) : ctx->last_item_id;
        if (id != 0u)
            eli_open_popup_id(id, flags);
    }
}

/* ---------------------------------------------------------------------------
 * Closing
 * ------------------------------------------------------------------------- */

/**
 * Close the popup currently being emitted (the innermost begin-popup) and any
 * popups nested inside it. Call from within a eli_begin_popup / eli_end_popup
 * scope (e.g. after a menu item is chosen).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_close_current_popup(void)
{
    int popup_idx = g_eli_begin_popup_count - 1;
    if (popup_idx < 0 || popup_idx >= g_eli_open_popup_count)
        return;
    g_eli_open_popup_count = popup_idx;
}

/**
 * Close every popup down to (and excluding) the given open-stack level.
 *
 * @param level  Number of popups to keep (0 closes all).
 */
static inline void eli_close_popup_to_level(int level)
{
    if (level < 0)
        level = 0;
    if (level < g_eli_open_popup_count)
        g_eli_open_popup_count = level;
}

/**
 * End the innermost popup window opened by a eli_begin_popup* that returned true.
 * Pops the begin-popup stack and the window scope.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_end_popup(void)
{
    if (eli_get_current_context() == NULL)
        return;
    if (g_eli_begin_popup_count > 0)
        g_eli_begin_popup_count--;
    eli_end();
}

/* ---------------------------------------------------------------------------
 * Modal backdrop
 * ------------------------------------------------------------------------- */

/**
 * Render the dimmed backdrop behind a modal by emitting a full-display window
 * (drawn just under the modal in z-order) that fills the viewport with the modal
 * dim color. Because it captures the mouse over the whole display, it also traps
 * hover/focus away from the windows behind the modal.
 *
 * @param ctx  Context (non-NULL).
 */
static inline void eli_popup__render_dim_background(eli_context *ctx)
{
    eli_vec2 disp = ctx->io.display_size;
    if (disp.x <= 0.0f || disp.y <= 0.0f)
        return;

    eli_window_flags wf = ELI_WINDOW_NO_TITLEBAR | ELI_WINDOW_NO_RESIZE | ELI_WINDOW_NO_MOVE |
                          ELI_WINDOW_NO_SCROLLBAR | ELI_WINDOW_NO_COLLAPSE |
                          ELI_WINDOW_NO_SAVED_SETTINGS | ELI_WINDOW_NO_BACKGROUND |
                          ELI_WINDOW_NO_NAV;
    eli_set_next_window_pos(eli_make_vec2(0.0f, 0.0f), ELI_COND_ALWAYS, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(disp, ELI_COND_ALWAYS);
    if (eli_begin("##ModalDimBg", NULL, wf)) {
        eli_draw_list *dl = eli_get_window_draw_list();
        if (dl != NULL) {
            eli_col32 dim = eli_get_color_u32(ELI_COL_MODAL_WINDOW_DIM_BG, 1.0f);
            eli_draw_list_push_clip_rect(dl, eli_make_vec2(0.0f, 0.0f), disp, false);
            eli_draw_list_add_rect_filled(dl, eli_make_vec2(0.0f, 0.0f), disp, dim, 0.0f, 0);
            eli_draw_list_pop_clip_rect(dl);
        }
    }
    eli_end();
}

/* ---------------------------------------------------------------------------
 * Popup windows
 * ------------------------------------------------------------------------- */

/**
 * Shared popup entry. Verifies the popup id is open at the current nesting level,
 * positions it (non-modals appear at the open position), begins the popup window,
 * and pushes the begin-popup stack. On failure the next-window data is cleared.
 *
 * @param id           Popup id (also used as the popup window's id).
 * @param name         Window name (synthetic for popups, visible for modals).
 * @param p_open       Optional open flag (modal close button); may be NULL.
 * @param extra_flags  Caller-supplied + composed window flags.
 * @param is_modal     True to render the dim backdrop and mark the window modal.
 * @return             true if the popup body should be emitted.
 */
static inline bool eli_popup__begin(eli_id id, const char *name, bool *p_open,
                                    eli_window_flags extra_flags, bool is_modal)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return false;

    int level = g_eli_begin_popup_count;
    if (!(g_eli_open_popup_count > level && g_eli_open_popup_stack[level].popup_id == id)) {
        eli_clear_next_window_data(ctx);
        return false;
    }
    if (level >= ELI_POPUP_STACK_MAX) {
        eli_clear_next_window_data(ctx);
        return false;
    }

    eli_popup_data *src = &g_eli_open_popup_stack[level];
    if (is_modal)
        eli_popup__render_dim_background(ctx);
    else
        eli_set_next_window_pos(src->open_popup_pos, ELI_COND_APPEARING,
                                eli_make_vec2(0.0f, 0.0f));

    eli_window_flags wflags = extra_flags | ELI_WINDOW_POPUP | ELI_WINDOW_NO_SAVED_SETTINGS;
    if (is_modal)
        wflags |= ELI_WINDOW_MODAL;

    bool is_open = eli_begin_ex(name, id, p_open, wflags, NULL);

    g_eli_begin_popup_stack[g_eli_begin_popup_count] = *src;
    g_eli_begin_popup_stack[g_eli_begin_popup_count].window = ctx->current_window;
    g_eli_begin_popup_stack[g_eli_begin_popup_count].is_modal = is_modal;
    g_eli_begin_popup_count++;
    src->window = ctx->current_window;
    src->is_modal = is_modal;

    if (!is_open) {
        eli_end_popup();
        return false;
    }
    if (is_modal && p_open != NULL && !*p_open) {
        eli_close_current_popup();
        eli_end_popup();
        return false;
    }
    return true;
}

/* Standard flag composition for auto-sized, chrome-less popup windows. */
#define ELI_POPUP_WINDOW_FLAGS \
    (ELI_WINDOW_NO_TITLEBAR | ELI_WINDOW_NO_RESIZE | ELI_WINDOW_NO_MOVE | \
     ELI_WINDOW_NO_COLLAPSE | ELI_WINDOW_AUTO_RESIZE)

/**
 * Begin a popup window if its id is open at the current nesting level. Must be
 * balanced with eli_end_popup only when this returns true.
 *
 * @param str_id  Popup string id (hashed against the current id seed).
 * @param flags   Extra window flags to merge into the popup window.
 * @return        true if the popup is open and its body should be emitted.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_begin_popup(const char *str_id, eli_window_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL)
        return false;
    if (g_eli_open_popup_count <= g_eli_begin_popup_count) {
        eli_clear_next_window_data(ctx);
        return false;
    }
    eli_id id = eli_get_id(str_id);
    char name[32];
    snprintf(name, sizeof name, "##Popup_%08x", (unsigned)id);
    return eli_popup__begin(id, name, NULL, flags | ELI_POPUP_WINDOW_FLAGS, false);
}

/**
 * Begin a modal dialog if its id is open. A modal keeps a title bar, dims and
 * blocks the windows behind it, and stays open until eli_close_current_popup (or
 * the close button clears *p_open). Balance with eli_end_popup only when true.
 *
 * @param name    Dialog name (identity + visible title).
 * @param p_open  Optional open flag; a title-bar close button clears it.
 * @param flags   Extra window flags to merge into the modal window.
 * @return        true if the modal is open and its body should be emitted.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_begin_popup_modal(const char *name, bool *p_open, eli_window_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || name == NULL)
        return false;
    eli_id id = eli_get_id(name);
    if (!(g_eli_open_popup_count > g_eli_begin_popup_count &&
          g_eli_open_popup_stack[g_eli_begin_popup_count].popup_id == id)) {
        eli_clear_next_window_data(ctx);
        return false;
    }
    return eli_popup__begin(id, name, p_open, flags | ELI_WINDOW_NO_COLLAPSE, true);
}

/* ---------------------------------------------------------------------------
 * Context-menu openers
 * ------------------------------------------------------------------------- */

/**
 * Open (on right/middle/left click of the last item) and begin a context-menu
 * popup for that item. When str_id is NULL/empty the last item's id is used.
 *
 * @param str_id  Popup string id, or NULL to key off the last item's id.
 * @param flags   eli_popup_flags; the low bits select the mouse button.
 * @return        true if the context popup is open and its body should be emitted.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_begin_popup_context_item(const char *str_id, eli_popup_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return false;
    eli_id id = (str_id != NULL && str_id[0] != '\0') ? eli_get_id(str_id) : ctx->last_item_id;
    if (id == 0u)
        return false;
    int button = flags & ELI_POPUP_MOUSE_BUTTON_MASK_;
    if (eli_is_mouse_released((eli_mouse_button)button) &&
        eli_is_item_hovered(ELI_HOVERED_ALLOW_WHEN_BLOCKED_BY_POPUP))
        eli_open_popup_id(id, flags);
    char name[32];
    snprintf(name, sizeof name, "##Popup_%08x", (unsigned)id);
    return eli_popup__begin(id, name, NULL, ELI_POPUP_WINDOW_FLAGS, false);
}

/**
 * Open (on click over the current window, away from items when NoOpenOverItems)
 * and begin a context-menu popup for the whole window.
 *
 * @param str_id  Popup string id, or NULL for a default per-window id.
 * @param flags   eli_popup_flags; the low bits select the mouse button.
 * @return        true if the context popup is open and its body should be emitted.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_begin_popup_context_window(const char *str_id, eli_popup_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL)
        return false;
    if (str_id == NULL || str_id[0] == '\0')
        str_id = "window_context";
    eli_id id = eli_get_id(str_id);
    int button = flags & ELI_POPUP_MOUSE_BUTTON_MASK_;
    if (eli_is_mouse_released((eli_mouse_button)button) && eli_is_window_hovered())
        if (!(flags & ELI_POPUP_NO_OPEN_OVER_ITEMS) || !eli_is_any_item_hovered())
            eli_open_popup_id(id, flags);
    char name[32];
    snprintf(name, sizeof name, "##Popup_%08x", (unsigned)id);
    return eli_popup__begin(id, name, NULL, ELI_POPUP_WINDOW_FLAGS, false);
}

/**
 * Open (on click over empty space, no window hovered) and begin a context-menu
 * popup anchored to the void.
 *
 * @param str_id  Popup string id, or NULL for a default id.
 * @param flags   eli_popup_flags; the low bits select the mouse button.
 * @return        true if the context popup is open and its body should be emitted.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_begin_popup_context_void(const char *str_id, eli_popup_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL)
        return false;
    if (str_id == NULL || str_id[0] == '\0')
        str_id = "void_context";
    eli_id id = eli_get_id(str_id);
    int button = flags & ELI_POPUP_MOUSE_BUTTON_MASK_;
    if (eli_is_mouse_released((eli_mouse_button)button) && ctx->hovered_window == NULL &&
        g_eli_open_popup_count == 0)
        eli_open_popup_id(id, flags);
    char name[32];
    snprintf(name, sizeof name, "##Popup_%08x", (unsigned)id);
    return eli_popup__begin(id, name, NULL, ELI_POPUP_WINDOW_FLAGS, false);
}

/* ---------------------------------------------------------------------------
 * Per-frame maintenance
 * ------------------------------------------------------------------------- */

/** @return true if `win` is `root` or one of its child windows. */
static inline bool eli_popup__window_in_tree(const eli_window *win, const eli_window *root)
{
    if (win == NULL || root == NULL)
        return false;
    const eli_window *win_root = win->root_window ? win->root_window : win;
    const eli_window *ref_root = root->root_window ? root->root_window : root;
    return win_root == ref_root;
}

/**
 * Close popups that no longer contain the reference (hovered) window, mirroring
 * Dear ImGui's ClosePopupsOverWindow. Popups not yet displayed this session
 * (window == NULL) and modal popups are always kept; a deeper non-modal popup is
 * trimmed when the reference window is not part of it.
 *
 * @param ref_window  Window under the mouse (NULL closes all non-modal popups).
 */
static inline void eli_popup__close_over_window(eli_window *ref_window)
{
    if (g_eli_open_popup_count == 0)
        return;

    int keep = 0;
    for (; keep < g_eli_open_popup_count; keep++) {
        eli_popup_data *p = &g_eli_open_popup_stack[keep];
        if (p->window == NULL || p->is_modal)
            continue; /* just-opened or modal popups are not click-closed */
        bool ref_in_popup = false;
        if (ref_window != NULL) {
            for (int n = keep; n < g_eli_open_popup_count; n++) {
                if (eli_popup__window_in_tree(ref_window, g_eli_open_popup_stack[n].window)) {
                    ref_in_popup = true;
                    break;
                }
            }
        }
        if (!ref_in_popup)
            break;
    }
    eli_close_popup_to_level(keep);
}

/**
 * Begin-of-frame popup maintenance: reset the begin-popup stack, close popups
 * whose owner window vanished, close the top popup on Escape, and close popups
 * the user clicked away from. Call after eli_window_new_frame (so the hovered
 * window is resolved) and before emitting widgets.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_popup_new_frame(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;

    g_eli_begin_popup_count = 0;

    /* Reap popups whose owning window is no longer being submitted. */
    for (int n = 0; n < g_eli_open_popup_count; n++) {
        eli_window *owner = g_eli_open_popup_stack[n].parent_window;
        if (owner != NULL && owner->last_frame_active < ctx->frame_count - 1) {
            eli_close_popup_to_level(n);
            break;
        }
    }

    if (g_eli_open_popup_count > 0 && eli_is_key_pressed(ELI_KEY_ESCAPE))
        eli_close_popup_to_level(g_eli_open_popup_count - 1);

    if (eli_is_mouse_clicked(ELI_MOUSE_BUTTON_LEFT) ||
        eli_is_mouse_clicked(ELI_MOUSE_BUTTON_RIGHT) ||
        eli_is_mouse_clicked(ELI_MOUSE_BUTTON_MIDDLE))
        eli_popup__close_over_window(ctx->hovered_window);
}

/**
 * End-of-frame popup maintenance: drop any open popups deeper than what was
 * actually begun this frame (a popup opened but never begun is discarded), so the
 * open-popup stack cannot outgrow the emitted nesting. Call after all widgets and
 * before eli_window_render.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_popup_end_frame(void)
{
    if (eli_get_current_context() == NULL)
        return;
    /* Any popup deeper than the begun nesting whose window was never created this
     * frame is stale; trim to the last displayed level. */
    while (g_eli_open_popup_count > 0 &&
           g_eli_open_popup_stack[g_eli_open_popup_count - 1].window == NULL &&
           g_eli_open_popup_stack[g_eli_open_popup_count - 1].open_frame_count <
               eli_get_current_context()->frame_count)
        g_eli_open_popup_count--;
}

#endif /* ELI_WIDGETS_ELI_POPUP_H */
