/**
 * @file eli_util_misc.h
 * @brief Miscellaneous per-frame utilities: rectangle-visibility queries, time
 *        and frame-count accessors, and the lazily-allocated background /
 *        foreground draw lists that render behind and in front of all windows.
 *
 * The background/foreground draw lists are persistent (allocated once on first
 * use, reused every frame). eli_util_new_frame clears them at the start of each
 * frame; the window module prepends the background list and appends the
 * foreground list when it assembles the frame's draw data. Both are released by
 * util_shutdown_fn on eli_destroy_context.
 *
 * @status Phase 27 misc utilities in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_UTIL_ELI_UTIL_MISC_H
#define ELI_UTIL_ELI_UTIL_MISC_H

#include "../core/eli_platform.h"

#include "../core/eli_types.h"
#include "../core/eli_context.h"
#include "../draw/eli_draw.h"
#include "../window/eli_window.h"

/* ---------------------------------------------------------------------------
 * Time & frame
 * ------------------------------------------------------------------------- */

/**
 * @return  Accumulated application time in seconds (sum of io.delta_time across
 *          frames), or 0.0 if no context is current.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline double eli_get_time(void)
{
    eli_context *ctx = eli_get_current_context();
    return ctx ? ctx->time : 0.0;
}

/**
 * @return  Number of frames begun so far (incremented by eli_new_frame), or 0 if
 *          no context is current.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline int eli_get_frame_count(void)
{
    eli_context *ctx = eli_get_current_context();
    return ctx ? ctx->frame_count : 0;
}

/* ---------------------------------------------------------------------------
 * Visibility
 * ------------------------------------------------------------------------- */

/** True when clip rect `clip` overlaps the axis-aligned rect [rmin, rmax]. */
static inline bool eli_util__clip_overlaps_rect(eli_rect clip, eli_vec2 rmin, eli_vec2 rmax)
{
    eli_vec2 cmin = eli_rect_min(clip);
    eli_vec2 cmax = eli_rect_max(clip);
    return rmin.x < cmax.x && rmin.y < cmax.y && rmax.x > cmin.x && rmax.y > cmin.y;
}

/**
 * Test whether a rectangle of `size`, positioned at the current window's layout
 * cursor, is at least partially inside the current window's clip rectangle.
 * Mirrors Dear ImGui's culling helper for skipping off-screen content.
 *
 * @param size  Size of the rectangle to test (its top-left is the cursor pos).
 * @return      true if the rectangle overlaps the clip rect; false otherwise or
 *              when there is no current window.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline bool eli_is_rect_visible(eli_vec2 size)
{
    eli_window *win = eli_get_current_window();
    if (win == NULL)
        return false;
    eli_vec2 rmin = win->cursor_pos;
    eli_vec2 rmax = eli_make_vec2(rmin.x + size.x, rmin.y + size.y);
    return eli_util__clip_overlaps_rect(win->clip_rect, rmin, rmax);
}

/**
 * Test whether the rectangle [rect_min, rect_max] (in screen space) is at least
 * partially inside the current window's clip rectangle.
 *
 * @param rect_min  Top-left corner of the rectangle, in screen space.
 * @param rect_max  Bottom-right corner of the rectangle, in screen space.
 * @return          true if the rectangle overlaps the clip rect; false otherwise
 *                  or when there is no current window.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline bool eli_is_rect_visible_vec2(eli_vec2 rect_min, eli_vec2 rect_max)
{
    eli_window *win = eli_get_current_window();
    if (win == NULL)
        return false;
    return eli_util__clip_overlaps_rect(win->clip_rect, rect_min, rect_max);
}

/* ---------------------------------------------------------------------------
 * Background / foreground draw lists
 * ------------------------------------------------------------------------- */

/**
 * Release the background/foreground draw lists. Registered as the context's
 * util_shutdown_fn on first use so eli_destroy_context can free them.
 *
 * @param ctx  Owning context (non-NULL).
 */
static inline void eli_util__shutdown_draw_lists(struct eli_context *ctx)
{
    if (ctx->background_draw_list != NULL) {
        eli_draw_list_clear(ctx->background_draw_list);
        free(ctx->background_draw_list);
        ctx->background_draw_list = NULL;
    }
    if (ctx->foreground_draw_list != NULL) {
        eli_draw_list_clear(ctx->foreground_draw_list);
        free(ctx->foreground_draw_list);
        ctx->foreground_draw_list = NULL;
    }
}

/** Lazily allocate and initialize one of the persistent util draw lists. */
static inline eli_draw_list *eli_util__get_or_create_draw_list(eli_context *ctx,
                                                               eli_draw_list **slot)
{
    if (*slot != NULL)
        return *slot;

    eli_draw_list *dl = (eli_draw_list *)malloc(sizeof(*dl));
    if (dl == NULL)
        return NULL;
    eli_draw_list_init(dl, ELI_DRAW_LIST_NONE);
    eli_draw_list_push_clip_rect_full_screen(dl);

    if (ctx->util_shutdown_fn == NULL)
        ctx->util_shutdown_fn = eli_util__shutdown_draw_lists;

    *slot = dl;
    return dl;
}

/**
 * Get the shared background draw list, which renders behind every window. The
 * list persists across frames and is cleared at the start of each frame; draw
 * into it after eli_frame_begin.
 *
 * @return  The background draw list, or NULL if no context or on allocation
 *          failure. Owned by the context; do not free.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline eli_draw_list *eli_get_background_draw_list(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return NULL;
    return eli_util__get_or_create_draw_list(ctx, &ctx->background_draw_list);
}

/**
 * Get the shared foreground draw list, which renders in front of every window.
 * The list persists across frames and is cleared at the start of each frame;
 * draw into it after eli_frame_begin.
 *
 * @return  The foreground draw list, or NULL if no context or on allocation
 *          failure. Owned by the context; do not free.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline eli_draw_list *eli_get_foreground_draw_list(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return NULL;
    return eli_util__get_or_create_draw_list(ctx, &ctx->foreground_draw_list);
}

/** Reset one persistent util draw list for a new frame, re-seeding its state. */
static inline void eli_util__reset_frame_draw_list(eli_context *ctx, eli_draw_list *dl)
{
    eli_draw_list_reset(dl);
    if (ctx->font != NULL && ctx->font->container_atlas != NULL) {
        dl->tex_uv_white_pixel = ctx->font->container_atlas->tex_uv_white_pixel;
        eli_draw_list_push_texture_id(dl, ctx->font->container_atlas->tex_id);
    }
    eli_draw_list_push_clip_rect_full_screen(dl);
}

/**
 * Clear the background and foreground draw lists for a new frame. A no-op for
 * lists that have not been created yet. Call once per frame, after eli_new_frame
 * and before any drawing into the util lists (wired into eli_frame_begin).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_util_new_frame(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    if (ctx->background_draw_list != NULL)
        eli_util__reset_frame_draw_list(ctx, ctx->background_draw_list);
    if (ctx->foreground_draw_list != NULL)
        eli_util__reset_frame_draw_list(ctx, ctx->foreground_draw_list);
}

#endif /* ELI_UTIL_ELI_UTIL_MISC_H */
