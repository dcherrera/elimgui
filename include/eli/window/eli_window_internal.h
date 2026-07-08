/**
 * @file eli_window_internal.h
 * @brief Shared window-engine internals: the window registry (lookup/create/GC),
 *        current-window accessors, focus-order (z-order) management, decoration
 *        and work-rect computation, scrollbar sizing, scroll-range clamping, and
 *        the next-window-data accessor plus the context shutdown hook.
 *
 * These are the low-level pieces the public window API (begin/end, settings,
 * scroll, query, child) is built on. They depend on the context but not on the
 * higher-level window headers, so they can be included freely without cycles.
 *
 * @status Phase 7 window engine internals in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WINDOW_ELI_WINDOW_INTERNAL_H
#define ELI_WINDOW_ELI_WINDOW_INTERNAL_H

#include "eli_window_types.h"

#include "../core/eli_platform.h"
#include "../core/eli_core.h"
#include "../draw/eli_draw.h"
#include "../font/eli_font.h"
#include "../id/eli_id.h"
#include "../input/eli_input.h"
#include "../style/eli_style.h"

/* ---------------------------------------------------------------------------
 * Small helpers
 * ------------------------------------------------------------------------- */

/** Grow a pointer array (of eli_window* or eli_draw_list*) to hold `need` slots. */
static inline void **eli_window_grow_ptr_array(void **data, int *capacity, int need)
{
    if (*capacity >= need)
        return data;
    int cap = (*capacity > 0) ? *capacity : 8;
    while (cap < need)
        cap += cap / 2 + 1;
    void **grown = (void **)realloc(data, (size_t)cap * sizeof(void *));
    if (grown != NULL)
        *capacity = cap;
    return (grown != NULL) ? grown : data;
}

/** @return the effective font size to use for decoration sizing (never 0). */
static inline float eli_window_effective_font_size(const eli_context *ctx)
{
    float fs = ctx->font_size;
    return (fs > 0.0f) ? fs : ELI_WINDOW_DEFAULT_FONT_SIZE;
}

/* ---------------------------------------------------------------------------
 * Context shutdown hook
 * ------------------------------------------------------------------------- */

/**
 * Release all window heap state owned by a context: every pooled window's draw
 * list, name, and the window itself, plus the pool/focus/render arrays and the
 * next-window-data block. Registered on ctx->window_shutdown_fn so
 * eli_destroy_context can call it.
 *
 * @param ctx  Context whose window state is freed (non-NULL).
 */
static inline void eli_window_shutdown_context(eli_context *ctx)
{
    for (int i = 0; i < ctx->windows_count; i++) {
        eli_window *win = ctx->windows[i];
        if (win == NULL)
            continue;
        eli_draw_list_clear(&win->draw_list);
        free(win->name);
        free(win);
    }
    free(ctx->windows);
    ctx->windows = NULL;
    ctx->windows_count = 0;
    ctx->windows_capacity = 0;

    free(ctx->windows_focus_order);
    ctx->windows_focus_order = NULL;
    ctx->windows_focus_order_count = 0;
    ctx->windows_focus_order_capacity = 0;

    free(ctx->render_draw_lists);
    ctx->render_draw_lists = NULL;
    ctx->render_draw_lists_count = 0;
    ctx->render_draw_lists_capacity = 0;

    free(ctx->next_window_data);
    ctx->next_window_data = NULL;

    free(ctx->draw_data);
    ctx->draw_data = NULL;
}

/* ---------------------------------------------------------------------------
 * Next-window data
 * ------------------------------------------------------------------------- */

/**
 * Return the context's next-window-data block, allocating and zeroing it on
 * first use. Also registers the window shutdown hook so the block (and the rest
 * of the window state) is released when the context is destroyed.
 *
 * @param ctx  Context (non-NULL).
 * @return     The next-window-data block, or NULL on allocation failure.
 */
static inline eli_next_window_data *eli_get_next_window_data(eli_context *ctx)
{
    if (ctx->window_shutdown_fn == NULL)
        ctx->window_shutdown_fn = eli_window_shutdown_context;
    if (ctx->next_window_data == NULL)
        ctx->next_window_data = (eli_next_window_data *)calloc(1, sizeof(*ctx->next_window_data));
    return ctx->next_window_data;
}

/** Clear all pending next-window settings (called after a window consumes them). */
static inline void eli_clear_next_window_data(eli_context *ctx)
{
    if (ctx->next_window_data != NULL)
        memset(ctx->next_window_data, 0, sizeof(*ctx->next_window_data));
}

/* ---------------------------------------------------------------------------
 * Registry (lookup / create)
 * ------------------------------------------------------------------------- */

/**
 * Find a window in the pool by its id.
 *
 * @param ctx  Context (non-NULL).
 * @param id   Window id to find.
 * @return     The window, or NULL if none matches.
 */
static inline eli_window *eli_find_window_by_id(const eli_context *ctx, eli_id id)
{
    for (int i = 0; i < ctx->windows_count; i++)
        if (ctx->windows[i]->id == id)
            return ctx->windows[i];
    return NULL;
}

/**
 * Find a window by its (unmodified) begin() name.
 *
 * @param ctx   Context (non-NULL).
 * @param name  Name to match (non-NULL).
 * @return      The window, or NULL if none matches.
 */
static inline eli_window *eli_find_window_by_name(const eli_context *ctx, const char *name)
{
    eli_id id = eli_hash_str(name, 0);
    return eli_find_window_by_id(ctx, id);
}

/**
 * Get an existing window by name or create and register a new one. New windows
 * start collapsed=false, hidden, with default position and a fresh draw list.
 *
 * @param ctx   Context (non-NULL).
 * @param name  Window name (non-NULL).
 * @param id    Precomputed window id (hash of name against root seed 0).
 * @return      The window, or NULL on allocation failure.
 */
static inline eli_window *eli_window_get_or_create(eli_context *ctx, const char *name, eli_id id)
{
    eli_window *win = eli_find_window_by_id(ctx, id);
    if (win != NULL)
        return win;

    win = (eli_window *)calloc(1, sizeof(*win));
    if (win == NULL)
        return NULL;

    size_t name_len = strlen(name);
    win->name = (char *)malloc(name_len + 1);
    if (win->name == NULL) {
        free(win);
        return NULL;
    }
    memcpy(win->name, name, name_len + 1);

    win->id = id;
    win->pos = eli_make_vec2(ELI_WINDOW_DEFAULT_POS_X, ELI_WINDOW_DEFAULT_POS_Y);
    win->size = eli_make_vec2(0.0f, 0.0f);
    win->size_full = eli_make_vec2(0.0f, 0.0f);
    win->font_window_scale = 1.0f;
    win->last_frame_active = -1;
    win->focus_order = -1;
    eli_draw_list_init(&win->draw_list, ELI_DRAW_LIST_NONE);

    if (ctx->window_shutdown_fn == NULL)
        ctx->window_shutdown_fn = eli_window_shutdown_context;

    ctx->windows = (eli_window **)eli_window_grow_ptr_array(
        (void **)ctx->windows, &ctx->windows_capacity, ctx->windows_count + 1);
    ctx->windows[ctx->windows_count++] = win;
    return win;
}

/* ---------------------------------------------------------------------------
 * Focus order (z-order)
 * ------------------------------------------------------------------------- */

/** @return the index of `win` in the focus-order list, or -1 if absent. */
static inline int eli_window_focus_index(const eli_context *ctx, const eli_window *win)
{
    for (int i = 0; i < ctx->windows_focus_order_count; i++)
        if (ctx->windows_focus_order[i] == win)
            return i;
    return -1;
}

/** Refresh the cached focus_order index on every window in the focus list. */
static inline void eli_window_reindex_focus_order(eli_context *ctx)
{
    for (int i = 0; i < ctx->windows_focus_order_count; i++)
        ctx->windows_focus_order[i]->focus_order = i;
}

/** Append a window to the front (top) of the focus order if not already present. */
static inline void eli_window_add_to_focus_order(eli_context *ctx, eli_window *win)
{
    if (eli_window_focus_index(ctx, win) >= 0)
        return;
    ctx->windows_focus_order = (eli_window **)eli_window_grow_ptr_array(
        (void **)ctx->windows_focus_order, &ctx->windows_focus_order_capacity,
        ctx->windows_focus_order_count + 1);
    ctx->windows_focus_order[ctx->windows_focus_order_count++] = win;
    eli_window_reindex_focus_order(ctx);
}

/**
 * Bring a window to the front of the z-order and make it the focused (nav)
 * window. Child windows focus their root window.
 *
 * @param ctx  Context (non-NULL).
 * @param win  Window to focus (NULL clears focus).
 */
static inline void eli_window_focus(eli_context *ctx, eli_window *win)
{
    ctx->nav_window = win;
    if (win == NULL)
        return;

    eli_window *root = win->root_window ? win->root_window : win;
    int idx = eli_window_focus_index(ctx, root);
    if (idx < 0) {
        eli_window_add_to_focus_order(ctx, root);
        return;
    }
    /* Move to the end (front). */
    for (int i = idx; i < ctx->windows_focus_order_count - 1; i++)
        ctx->windows_focus_order[i] = ctx->windows_focus_order[i + 1];
    ctx->windows_focus_order[ctx->windows_focus_order_count - 1] = root;
    eli_window_reindex_focus_order(ctx);
}

/* ---------------------------------------------------------------------------
 * Current window accessors
 * ------------------------------------------------------------------------- */

/** @return the current (innermost begun) window, or NULL. */
static inline eli_window *eli_get_current_window(void)
{
    eli_context *ctx = eli_get_current_context();
    return ctx ? ctx->current_window : NULL;
}

/* ---------------------------------------------------------------------------
 * Geometry + scroll computation
 * ------------------------------------------------------------------------- */

/**
 * Recompute a window's decoration rects (outer/title/inner) and its work area,
 * scrollbar presence, scroll range, and clip rect from its current pos/size,
 * content size, and the active style. Called by eli_begin after pos/size are
 * finalized. Also clamps the current scroll to the new range.
 *
 * @param ctx    Context (non-NULL).
 * @param win    Window to lay out (non-NULL).
 * @param style  Active style (non-NULL).
 */
static inline void eli_window_update_layout(eli_context *ctx, eli_window *win,
                                            const eli_style *style)
{
    float scrollbar_size = style->scrollbar_size;
    eli_vec2 pad = win->window_padding;

    win->outer_rect = eli_make_rect(win->pos.x, win->pos.y, win->size.x, win->size.y);
    float tb = win->title_bar_height;
    win->title_bar_rect = eli_make_rect(win->pos.x, win->pos.y, win->size.x, tb);

    float inner_h = win->size.y - tb;
    if (inner_h < 0.0f)
        inner_h = 0.0f;
    win->inner_rect = eli_make_rect(win->pos.x, win->pos.y + tb, win->size.x, inner_h);

    /* Available work area before scrollbars. */
    float avail_w = win->inner_rect.w - pad.x * 2.0f;
    float avail_h = win->inner_rect.h - pad.y * 2.0f;
    if (avail_w < 0.0f) avail_w = 0.0f;
    if (avail_h < 0.0f) avail_h = 0.0f;

    /* Decide scrollbars from content size vs. available area. */
    bool want_x = false, want_y = false;
    if (eli_window_allows_scrollbar(win)) {
        want_y = (win->flags & ELI_WINDOW_ALWAYS_VERTICAL_SCROLLBAR) != 0 ||
                 win->content_size.y > avail_h + 0.01f;
        if (want_y)
            avail_w -= scrollbar_size;
        bool horiz_enabled = (win->flags & ELI_WINDOW_HORIZONTAL_SCROLLBAR) != 0 ||
                             (win->flags & ELI_WINDOW_ALWAYS_HORIZONTAL_SCROLLBAR) != 0;
        if (horiz_enabled) {
            want_x = (win->flags & ELI_WINDOW_ALWAYS_HORIZONTAL_SCROLLBAR) != 0 ||
                     win->content_size.x > avail_w + 0.01f;
            if (want_x)
                avail_h -= scrollbar_size;
        }
        if (avail_w < 0.0f) avail_w = 0.0f;
        if (avail_h < 0.0f) avail_h = 0.0f;
    }
    win->has_scrollbar_x = want_x;
    win->has_scrollbar_y = want_y;

    win->content_region_rect = eli_make_rect(win->inner_rect.x + pad.x, win->inner_rect.y + pad.y,
                                             avail_w, avail_h);

    win->scroll_max.x = eli_max_f(0.0f, win->content_size.x - avail_w);
    win->scroll_max.y = eli_max_f(0.0f, win->content_size.y - avail_h);
    win->scroll.x = eli_clamp_f(win->scroll.x, 0.0f, win->scroll_max.x);
    win->scroll.y = eli_clamp_f(win->scroll.y, 0.0f, win->scroll_max.y);

    win->clip_rect = win->inner_rect;

    (void)ctx;
}

/** Clamp a window's current scroll to its scroll range. */
static inline void eli_window_clamp_scroll(eli_window *win)
{
    win->scroll.x = eli_clamp_f(win->scroll.x, 0.0f, win->scroll_max.x);
    win->scroll.y = eli_clamp_f(win->scroll.y, 0.0f, win->scroll_max.y);
}

#endif /* ELI_WINDOW_ELI_WINDOW_INTERNAL_H */
