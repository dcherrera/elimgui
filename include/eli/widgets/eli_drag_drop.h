/**
 * @file eli_drag_drop.h
 * @brief Phase 21 drag & drop: payload storage plus the source/target API pair
 *        (begin/set/end source, begin/accept/end target, get payload) that mirrors
 *        Dear ImGui's immediate-mode drag-drop contract.
 *
 * A drag begins when the last-submitted item is the active (held) item and the
 * mouse is dragging: eli_begin_drag_drop_source() opens the source, and
 * eli_set_drag_drop_payload() copies a type-tagged blob into the module-private
 * payload buffer (allocated through the eli_mem seam). Any later item can become a
 * drop target via eli_begin_drag_drop_target(); eli_accept_drag_drop_payload()
 * returns the payload once it is delivered (mouse released over the target) or,
 * with peek flags, while it is merely hovered.
 *
 * All drag state (active flag, payload, target/accept bookkeeping, heap buffer) is
 * file-static and lives entirely in this header. Because the phase must not touch
 * the core frame path, the two lifecycle hooks eli_drag_drop_new_frame() and
 * eli_drag_drop_end_frame() are exposed for the host/test to call around
 * eli_new_frame()/eli_end_frame(): the first rolls the accept-id snapshot, the
 * second clears the payload once the drop is delivered or the drag expires.
 *
 * @status Phase 21 drag & drop in use.
 * @issues Drag preview renders onto the current window's draw list (no dedicated
 *         tooltip/overlay layer exists yet); it follows the mouse but is clipped
 *         to the source window.
 * @todo None
 */
#ifndef ELI_WIDGETS_ELI_DRAG_DROP_H
#define ELI_WIDGETS_ELI_DRAG_DROP_H

#include "eli_item_status.h"

#include "../core/eli_platform.h"
#include "../core/eli_context.h"
#include "../core/eli_types.h"
#include "../id/eli_id_active.h"
#include "../id/eli_id_hash.h"
#include "../input/eli_input_mouse.h"
#include "../layout/eli_layout_item.h"
#include "../draw/eli_draw_prim.h"
#include "../font/eli_font_text.h"
#include "../style/eli_style_color_utils.h"
#include "../util/eli_mem.h"
#include "../window/eli_window_query.h"

/* ---------------------------------------------------------------------------
 * Flags
 * ------------------------------------------------------------------------- */

/** Bit flags controlling drag-drop source and target behaviour. */
typedef int eli_drag_drop_flags;
enum eli_drag_drop_flags_ {
    ELI_DRAG_DROP_NONE                          = 0,

    /* Source flags. */
    ELI_DRAG_DROP_SOURCE_NO_PREVIEW_TOOLTIP     = 1 << 0,
    ELI_DRAG_DROP_SOURCE_NO_DISABLE_HOVER       = 1 << 1,
    ELI_DRAG_DROP_SOURCE_NO_HOLD_TO_OPEN_OTHERS = 1 << 2,
    ELI_DRAG_DROP_SOURCE_ALLOW_NULL_ID          = 1 << 3,
    ELI_DRAG_DROP_SOURCE_EXTERN                 = 1 << 4,
    ELI_DRAG_DROP_SOURCE_AUTO_EXPIRE_PAYLOAD    = 1 << 5,

    /* Target (accept) flags. */
    ELI_DRAG_DROP_ACCEPT_BEFORE_DELIVERY        = 1 << 10,
    ELI_DRAG_DROP_ACCEPT_NO_DRAW_DEFAULT_RECT   = 1 << 11,
    ELI_DRAG_DROP_ACCEPT_NO_PREVIEW_TOOLTIP     = 1 << 12,
    ELI_DRAG_DROP_ACCEPT_PEEK_ONLY =
        ELI_DRAG_DROP_ACCEPT_BEFORE_DELIVERY | ELI_DRAG_DROP_ACCEPT_NO_DRAW_DEFAULT_RECT
};

/** Maximum length (excluding the terminator) of a payload type tag. */
#define ELI_DRAG_DROP_TYPE_MAX 32

/* ---------------------------------------------------------------------------
 * Payload
 * ------------------------------------------------------------------------- */

/**
 * A drag-drop payload: the type-tagged data blob carried from source to target,
 * plus the source identity and the preview/delivery state a target inspects.
 *
 * `data` points into module-private storage owned by this header; it is valid
 * only while a drag is active. Copy it out on delivery if it must outlive the drop.
 */
typedef struct {
    void   *data;                              /* pointer to the copied blob */
    int     data_size;                         /* size of the blob in bytes */
    eli_id  source_id;                         /* id of the source item */
    eli_id  source_parent_id;                  /* id of the source's parent, if any */
    int     data_frame_count;                  /* frame the payload was last set */
    char    data_type[ELI_DRAG_DROP_TYPE_MAX + 1]; /* NUL-terminated type tag */
    bool    preview;                           /* target is hovering (peek) this frame */
    bool    delivery;                          /* payload delivered this frame */
} eli_payload;

/* ---------------------------------------------------------------------------
 * Module-private state
 * ------------------------------------------------------------------------- */

static bool                s_dd_active                    = false;
static eli_drag_drop_flags s_dd_source_flags              = ELI_DRAG_DROP_NONE;
static eli_mouse_button    s_dd_mouse_button              = ELI_MOUSE_BUTTON_LEFT;
static int                 s_dd_source_frame_count        = -1;
static eli_payload         s_dd_payload                   = {0};

static eli_rect            s_dd_target_rect               = {0.0f, 0.0f, 0.0f, 0.0f};
static eli_id              s_dd_target_id                 = 0;
static bool                s_dd_within_source             = false;
static bool                s_dd_within_target             = false;

static eli_drag_drop_flags s_dd_accept_flags              = ELI_DRAG_DROP_NONE;
static eli_id              s_dd_accept_id_curr            = 0;
static eli_id              s_dd_accept_id_prev            = 0;
static float              s_dd_accept_id_curr_rect_surface = 0.0f;
static int                 s_dd_accept_frame_count        = -1;

static void               *s_dd_payload_buf               = NULL;
static size_t              s_dd_payload_buf_capacity       = 0;

/* ---------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

/** Ensure the payload heap buffer holds at least `size` bytes. */
static inline bool eli_drag_drop__reserve(size_t size)
{
    if (s_dd_payload_buf != NULL && size <= s_dd_payload_buf_capacity)
        return true;
    void *fresh = eli_mem_alloc(size);
    if (fresh == NULL)
        return false;
    eli_mem_free(s_dd_payload_buf);
    s_dd_payload_buf = fresh;
    s_dd_payload_buf_capacity = size;
    return true;
}

/** @return true if the active payload carries the given type tag. */
static inline bool eli_drag_drop__is_data_type(const char *type)
{
    if (type == NULL)
        return false;
    return s_dd_payload.data_frame_count != -1 &&
           strncmp(s_dd_payload.data_type, type, ELI_DRAG_DROP_TYPE_MAX + 1) == 0;
}

/** Reset the payload record to its empty state (does not touch the heap buffer). */
static inline void eli_drag_drop__reset_payload(void)
{
    s_dd_payload.data = NULL;
    s_dd_payload.data_size = 0;
    s_dd_payload.source_id = 0;
    s_dd_payload.source_parent_id = 0;
    s_dd_payload.data_frame_count = -1;
    s_dd_payload.data_type[0] = '\0';
    s_dd_payload.preview = false;
    s_dd_payload.delivery = false;
}

/** Draw the drag preview near the mouse using the source window's draw list. */
static inline void eli_drag_drop__render_preview(eli_context *ctx)
{
    eli_draw_list *dl = eli_get_window_draw_list();
    if (dl == NULL)
        return;

    const char *label = s_dd_payload.data_type[0] != '\0' ? s_dd_payload.data_type : "...";
    float font_size = ctx->font_size > 0.0f ? ctx->font_size : 13.0f;
    eli_vec2 mouse = eli_get_mouse_pos();
    eli_vec2 pad = eli_make_vec2(4.0f, 2.0f);
    eli_vec2 text_pos = eli_make_vec2(mouse.x + 16.0f, mouse.y + 8.0f);
    eli_vec2 box_min = eli_make_vec2(text_pos.x - pad.x, text_pos.y - pad.y);
    eli_vec2 box_max = eli_make_vec2(text_pos.x + font_size * 6.0f + pad.x,
                                     text_pos.y + font_size + pad.y);

    eli_draw_list_add_rect_filled(dl, box_min, box_max,
                                  eli_get_color_u32(ELI_COL_POPUP_BG, 1.0f), 2.0f,
                                  ELI_DRAW_ROUND_CORNERS_ALL);
    eli_draw_list_add_text(dl, text_pos, eli_get_color_u32(ELI_COL_TEXT, 1.0f),
                           label, NULL);
}

/** Draw the default drop-target highlight around the accepting item's rect. */
static inline void eli_drag_drop__render_target_rect(void)
{
    eli_draw_list *dl = eli_get_window_draw_list();
    if (dl == NULL)
        return;
    eli_vec2 mn = eli_make_vec2(s_dd_target_rect.x - 3.5f, s_dd_target_rect.y - 3.5f);
    eli_vec2 mx = eli_make_vec2(s_dd_target_rect.x + s_dd_target_rect.w + 3.5f,
                                s_dd_target_rect.y + s_dd_target_rect.h + 3.5f);
    eli_draw_list_add_rect(dl, mn, mx, eli_get_color_u32(ELI_COL_DRAG_DROP_TARGET, 1.0f),
                           0.0f, ELI_DRAW_ROUND_CORNERS_ALL, 2.0f);
}

/* ---------------------------------------------------------------------------
 * Lifecycle hooks
 * ------------------------------------------------------------------------- */

/**
 * Reset every field of the active drag and release the payload heap buffer.
 * Called automatically when a drop is delivered or the drag expires.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_clear_drag_drop(void)
{
    s_dd_active = false;
    s_dd_source_flags = ELI_DRAG_DROP_NONE;
    s_dd_source_frame_count = -1;
    s_dd_within_source = false;
    s_dd_within_target = false;
    s_dd_target_id = 0;
    s_dd_target_rect = eli_make_rect(0.0f, 0.0f, 0.0f, 0.0f);
    s_dd_accept_flags = ELI_DRAG_DROP_NONE;
    s_dd_accept_id_curr = 0;
    s_dd_accept_id_prev = 0;
    s_dd_accept_id_curr_rect_surface = 0.0f;
    s_dd_accept_frame_count = -1;
    eli_drag_drop__reset_payload();
    eli_mem_free(s_dd_payload_buf);
    s_dd_payload_buf = NULL;
    s_dd_payload_buf_capacity = 0;
}

/**
 * Begin-of-frame hook: roll the accept-id snapshot forward and clear the
 * per-frame within-source/within-target markers. Call once per frame right after
 * eli_new_frame().
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_drag_drop_new_frame(void)
{
    s_dd_accept_id_prev = s_dd_accept_id_curr;
    s_dd_accept_id_curr = 0;
    s_dd_accept_id_curr_rect_surface = FLT_MAX;
    s_dd_within_source = false;
    s_dd_within_target = false;
}

/**
 * End-of-frame hook: clear the payload once it has been delivered, or once the
 * drag has expired (the source stopped submitting a payload and either the button
 * was released or the source opted into auto-expiry). Call once per frame just
 * before eli_end_frame().
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_drag_drop_end_frame(void)
{
    eli_context *ctx = eli_get_current_context();
    if (!s_dd_active || ctx == NULL)
        return;

    bool is_delivered = s_dd_payload.delivery;
    bool is_elapsed = (s_dd_payload.data_frame_count + 1 < ctx->frame_count) &&
                      ((s_dd_source_flags & ELI_DRAG_DROP_SOURCE_AUTO_EXPIRE_PAYLOAD) ||
                       !eli_is_mouse_down(s_dd_mouse_button));
    if (is_delivered || is_elapsed)
        eli_clear_drag_drop();
}

/* ---------------------------------------------------------------------------
 * Source
 * ------------------------------------------------------------------------- */

/**
 * Open a drag-drop source scope on the last-submitted item. The source becomes
 * active when that item is the held (active) item and the mouse is dragging.
 * Call eli_set_drag_drop_payload() inside the scope, then eli_end_drag_drop_source().
 *
 * @param flags  eli_drag_drop_flags source options (SOURCE_* bits).
 * @return       true if a drag is active and the scope is open; false otherwise.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline bool eli_begin_drag_drop_source(eli_drag_drop_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return false;

    eli_window *window = ctx->current_window;
    eli_mouse_button mouse_button = ELI_MOUSE_BUTTON_LEFT;
    eli_id source_id = 0;
    eli_id source_parent_id = 0;
    bool source_drag_active = false;

    if (flags & ELI_DRAG_DROP_SOURCE_EXTERN) {
        source_drag_active = true;
    } else {
        source_id = ctx->last_item_id;
        if (source_id != 0) {
            if (ctx->active_id != source_id)
                return false;
            if (!eli_is_mouse_down(mouse_button) || (window != NULL && window->skip_items))
                return false;
            source_drag_active = eli_is_mouse_dragging(mouse_button, -1.0f);
        } else {
            if (!(flags & ELI_DRAG_DROP_SOURCE_ALLOW_NULL_ID))
                return false;
            if (!eli_is_mouse_down(mouse_button))
                return false;
            source_drag_active = eli_is_mouse_dragging(mouse_button, -1.0f);
        }
    }

    if (!source_drag_active)
        return false;

    if (!s_dd_active) {
        eli_drag_drop__reset_payload();
        s_dd_payload.source_id = source_id;
        s_dd_payload.source_parent_id = source_parent_id;
        s_dd_active = true;
        s_dd_source_flags = flags;
        s_dd_mouse_button = mouse_button;
    }
    s_dd_source_frame_count = ctx->frame_count;
    s_dd_within_source = true;

    if (!(flags & ELI_DRAG_DROP_SOURCE_NO_PREVIEW_TOOLTIP))
        eli_drag_drop__render_preview(ctx);

    return true;
}

/**
 * Set (or refresh) the payload carried by the active drag. The data is copied
 * into module-private storage and tagged with `type` (truncated to
 * ELI_DRAG_DROP_TYPE_MAX bytes). Call inside a drag-drop source scope.
 *
 * @param type  NUL-terminated type tag (must match a target's requested type).
 * @param data  Pointer to the blob to copy (may be NULL only if sz == 0).
 * @param sz    Size of the blob in bytes.
 * @param cond  eli_cond controlling refresh: ELI_COND_ALWAYS (default) copies
 *              every frame; otherwise the copy happens only when the type changes.
 * @return      true if a target accepted this payload on the current/previous
 *              frame (useful for source-side feedback).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline bool eli_set_drag_drop_payload(const char *type, const void *data,
                                             size_t sz, eli_cond cond)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || type == NULL)
        return false;
    if (cond == ELI_COND_NONE)
        cond = ELI_COND_ALWAYS;

    bool type_changed = strncmp(s_dd_payload.data_type, type, ELI_DRAG_DROP_TYPE_MAX + 1) != 0;
    strncpy(s_dd_payload.data_type, type, ELI_DRAG_DROP_TYPE_MAX);
    s_dd_payload.data_type[ELI_DRAG_DROP_TYPE_MAX] = '\0';

    bool should_copy = (cond & ELI_COND_ALWAYS) || s_dd_payload.data_frame_count == -1 ||
                       type_changed;
    if (should_copy) {
        if (sz > 0) {
            if (!eli_drag_drop__reserve(sz))
                return false;
            memcpy(s_dd_payload_buf, data, sz);
            s_dd_payload.data = s_dd_payload_buf;
        } else {
            s_dd_payload.data = NULL;
        }
        s_dd_payload.data_size = (int)sz;
    }
    s_dd_payload.data_frame_count = ctx->frame_count;

    return s_dd_accept_frame_count == ctx->frame_count ||
           s_dd_accept_frame_count == ctx->frame_count - 1;
}

/**
 * Close the drag-drop source scope opened by eli_begin_drag_drop_source().
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_end_drag_drop_source(void)
{
    s_dd_within_source = false;
}

/* ---------------------------------------------------------------------------
 * Target
 * ------------------------------------------------------------------------- */

/**
 * Turn the last-submitted item into a drop target for the active drag. Only
 * succeeds while a payload is active and the mouse is hovering the item's rect
 * inside the hovered window. Follow with eli_accept_drag_drop_payload().
 *
 * @return  true if the item is a live drop target this frame; false otherwise.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline bool eli_begin_drag_drop_target(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || !s_dd_active)
        return false;

    eli_window *window = ctx->current_window;
    if (window == NULL)
        return false;
    if ((ctx->last_item_status_flags & ELI_ITEM_STATUS_HOVERED_RECT) == 0)
        return false;
    if (ctx->hovered_window == NULL)
        return false;

    eli_window *root = window->root_window ? window->root_window : window;
    eli_window *hov_root = ctx->hovered_window->root_window ? ctx->hovered_window->root_window
                                                            : ctx->hovered_window;
    if (root != hov_root)
        return false;

    eli_id id = ctx->last_item_id;
    if (id == 0) {
        eli_rect r = ctx->last_item_rect;
        float coords[4] = { r.x, r.y, r.w, r.h };
        id = eli_hash_data(coords, sizeof(coords), window->id);
    }
    if (s_dd_payload.source_id == id)
        return false;

    s_dd_target_rect = ctx->last_item_rect;
    s_dd_target_id = id;
    s_dd_within_target = true;
    return true;
}

/**
 * Accept the active payload at the current drop target if it matches `type`.
 * Returns the payload when it is delivered (mouse released over the target after
 * a hover), or immediately while hovered if ELI_DRAG_DROP_ACCEPT_BEFORE_DELIVERY
 * is set. When multiple nested targets overlap, the smallest rect wins.
 *
 * @param type   Required type tag, or NULL to accept any type.
 * @param flags  eli_drag_drop_flags accept options (ACCEPT_* bits).
 * @return       The payload (its `data`/`data_size`/`data_type` valid) on accept,
 *               or NULL if the type mismatches or delivery has not happened.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline const eli_payload *eli_accept_drag_drop_payload(const char *type,
                                                              eli_drag_drop_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || !s_dd_active || s_dd_target_id == 0)
        return NULL;
    if (type != NULL && !eli_drag_drop__is_data_type(type))
        return NULL;

    bool was_accepted_previously = (s_dd_accept_id_prev == s_dd_target_id);
    float r_surface = s_dd_target_rect.w * s_dd_target_rect.h;
    if (r_surface > s_dd_accept_id_curr_rect_surface)
        return NULL;

    s_dd_accept_flags = flags;
    s_dd_accept_id_curr = s_dd_target_id;
    s_dd_accept_id_curr_rect_surface = r_surface;

    s_dd_payload.preview = was_accepted_previously;
    flags |= (s_dd_source_flags & ELI_DRAG_DROP_ACCEPT_NO_DRAW_DEFAULT_RECT);
    if (!(flags & ELI_DRAG_DROP_ACCEPT_NO_DRAW_DEFAULT_RECT) && s_dd_payload.preview)
        eli_drag_drop__render_target_rect();

    s_dd_accept_frame_count = ctx->frame_count;
    s_dd_payload.delivery = was_accepted_previously && !eli_is_mouse_down(s_dd_mouse_button);
    if (!s_dd_payload.delivery && !(flags & ELI_DRAG_DROP_ACCEPT_BEFORE_DELIVERY))
        return NULL;

    return &s_dd_payload;
}

/**
 * Close the drop-target scope opened by eli_begin_drag_drop_target().
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_end_drag_drop_target(void)
{
    s_dd_within_target = false;
}

/**
 * @return  The active payload while a drag is in progress, or NULL when no drag
 *          is active. Lets callers peek at the payload/type outside a target scope.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline const eli_payload *eli_get_drag_drop_payload(void)
{
    return s_dd_active ? &s_dd_payload : NULL;
}

#endif /* ELI_WIDGETS_ELI_DRAG_DROP_H */
