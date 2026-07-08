/**
 * @file eli_list_clipper.h
 * @brief List clipper for efficient rendering of very large, evenly-spaced item
 *        lists: computes which item indices intersect the window's visible region
 *        and advances the layout cursor past skipped items so the scroll range stays
 *        correct (mirrors Dear ImGui's ImGuiListClipper).
 *
 * The eli_list_clipper struct is caller-owned (declared on the stack); all state
 * lives inside it, so no context storage is required. Typical usage:
 *
 *     eli_list_clipper clipper;
 *     eli_list_clipper_begin(&clipper, item_count, line_height);
 *     while (eli_list_clipper_step(&clipper)) {
 *         for (int i = clipper.display_start; i < clipper.display_end; i++)
 *             render_item(i);
 *     }
 *     eli_list_clipper_end(&clipper);
 *
 * Only items within [display_start, display_end) are emitted each step; the cursor
 * is seeked past clipped ranges so the window's measured content height reflects the
 * whole list and the scrollbar is sized correctly.
 *
 * @status Phase 26 list clipper in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_UTIL_ELI_LIST_CLIPPER_H
#define ELI_UTIL_ELI_LIST_CLIPPER_H

#include "../window/eli_window.h"

#include "../core/eli_platform.h"
#include "../core/eli_context.h"
#include "../core/eli_types.h"

/* Maximum number of clip/include ranges tracked per clipper. One slot is reserved
 * for the computed visible range; the rest hold caller force-include ranges (plus a
 * possible measurement range on the first step when the item height is unknown). */
#define ELI_LIST_CLIPPER_MAX_RANGES 8

/* ---------------------------------------------------------------------------
 * Types
 * ------------------------------------------------------------------------- */

/** A half-open item index range [min, max) tracked by the clipper. */
typedef struct eli_list_clipper_range {
    int min;
    int max;
} eli_list_clipper_range;

/**
 * Caller-owned list clipper. Public fields display_start/display_end give the item
 * index range to render after each eli_list_clipper_step. The remaining fields are
 * internal step state; declare the struct zero-initialized and drive it through the
 * begin/step/end API rather than touching them directly.
 */
typedef struct eli_list_clipper {
    eli_context *ctx;              /* context captured at begin */
    int    display_start;          /* first item index to render this step */
    int    display_end;            /* one past the last item index this step */
    int    items_count;            /* total item count passed to begin */
    float  items_height;           /* per-item height (measured if <= 0 at begin) */
    float  start_pos_y;            /* screen-space cursor y at begin (item 0 top) */
    double start_seek_offset_y;    /* seek bias so cursor math survives large ranges */

    int    step_no;                /* index of the next range to emit */
    int    ranges_count;           /* number of active ranges */
    eli_list_clipper_range ranges[ELI_LIST_CLIPPER_MAX_RANGES];
} eli_list_clipper;

/* ---------------------------------------------------------------------------
 * Small integer helpers (local, prefixed to avoid collisions)
 * ------------------------------------------------------------------------- */

static inline int eli_lc_mini(int a, int b)
{
    return a < b ? a : b;
}

static inline int eli_lc_maxi(int a, int b)
{
    return a > b ? a : b;
}

static inline int eli_lc_clampi(int v, int lo, int hi)
{
    if (v < lo)
        return lo;
    if (v > hi)
        return hi;
    return v;
}

/* ---------------------------------------------------------------------------
 * Internal range + cursor helpers
 * ------------------------------------------------------------------------- */

/**
 * Sort ranges[offset..count) by ascending min, then fuse any that touch/overlap.
 * Ranges below `offset` (already emitted this frame) are left untouched.
 *
 * @param ranges  Range array.
 * @param count   In/out active range count; shrinks as ranges fuse.
 * @param offset  First range index eligible for sorting/fusing.
 */
static inline void eli_list_clipper_sort_and_fuse(eli_list_clipper_range *ranges, int *count,
                                                  int offset)
{
    if (*count - offset <= 1)
        return;

    for (int sort_end = *count - offset - 1; sort_end > 0; --sort_end)
        for (int i = offset; i < sort_end + offset; ++i)
            if (ranges[i].min > ranges[i + 1].min) {
                eli_list_clipper_range tmp = ranges[i];
                ranges[i] = ranges[i + 1];
                ranges[i + 1] = tmp;
            }

    for (int i = 1 + offset; i < *count; i++) {
        if (ranges[i - 1].max < ranges[i].min)
            continue;
        ranges[i - 1].min = eli_lc_mini(ranges[i - 1].min, ranges[i].min);
        ranges[i - 1].max = eli_lc_maxi(ranges[i - 1].max, ranges[i].max);
        for (int j = i; j < *count - 1; j++)
            ranges[j] = ranges[j + 1];
        (*count)--;
        i--;
    }
}

/**
 * Move the window layout cursor to an absolute screen y and fix up the previous-line
 * bookkeeping so scroll-here / same-line queries still work after clipping. Mirrors
 * Dear ImGui's ImGuiListClipper_SeekCursorAndSetupPrevLine.
 *
 * @param clipper      Active clipper (its ctx must have a current window).
 * @param pos_y        Target absolute cursor y in screen space.
 * @param line_height  Per-item pitch used to derive prev-line geometry.
 */
static inline void eli_list_clipper_seek_and_setup_prev_line(eli_list_clipper *clipper,
                                                             float pos_y, float line_height)
{
    eli_context *ctx = clipper->ctx;
    if (ctx == NULL || ctx->current_window == NULL)
        return;
    eli_window *win = ctx->current_window;
    win->cursor_pos.y = pos_y;
    win->cursor_max_pos.y = eli_max_f(win->cursor_max_pos.y, pos_y - ctx->style.item_spacing.y);
    win->cursor_pos_prev_line.y = pos_y - line_height;
    win->prev_line_size.y = line_height - ctx->style.item_spacing.y;
    win->is_same_line = false;
}

/* ---------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

/**
 * Position the layout cursor at the top of a specific item, computed from the
 * clipper's start position and item height. Used internally to skip clipped items,
 * and available to callers that passed an indeterminate count to begin.
 *
 * @param clipper     Active clipper.
 * @param item_index  Item index whose top the cursor should be placed at.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_list_clipper_seek_cursor_for_item(eli_list_clipper *clipper, int item_index)
{
    float pos_y = (float)((double)clipper->start_pos_y + clipper->start_seek_offset_y +
                          (double)item_index * (double)clipper->items_height);
    eli_list_clipper_seek_and_setup_prev_line(clipper, pos_y, clipper->items_height);
}

/**
 * Begin clipping a list of `items_count` items. Captures the current window's cursor
 * as the origin of item 0. If `items_height` is <= 0, the first step submits one item
 * so its height can be measured, then clipping is computed from that.
 *
 * @param clipper       Caller-owned clipper to initialize.
 * @param items_count   Number of items in the list (0 renders nothing).
 * @param items_height  Fixed per-item height in pixels, or <= 0 to measure it.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_list_clipper_begin(eli_list_clipper *clipper, int items_count,
                                          float items_height)
{
    eli_context *ctx = eli_get_current_context();
    eli_window *win = ctx ? ctx->current_window : NULL;

    clipper->ctx = ctx;
    clipper->start_pos_y = win ? win->cursor_pos.y : 0.0f;
    clipper->start_seek_offset_y = 0.0;
    clipper->items_height = items_height;
    clipper->items_count = items_count;
    clipper->display_start = -1;
    clipper->display_end = 0;
    clipper->step_no = 0;
    clipper->ranges_count = 0;
}

/**
 * Force a range of item indices to be rendered even if it falls outside the visible
 * region (e.g. selected or keyboard-focused rows). Must be called after begin and
 * before the first step.
 *
 * @param clipper     Active clipper.
 * @param item_begin  First index to include.
 * @param item_end    One past the last index to include.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_list_clipper_include_items_by_index(eli_list_clipper *clipper,
                                                           int item_begin, int item_end)
{
    /* Only valid after Begin() and before the visible range has been resolved. */
    if (clipper->display_start >= 0)
        return;
    if (item_begin < item_end && clipper->ranges_count < ELI_LIST_CLIPPER_MAX_RANGES) {
        clipper->ranges[clipper->ranges_count].min = item_begin;
        clipper->ranges[clipper->ranges_count].max = item_end;
        clipper->ranges_count++;
    }
}

/**
 * Force a single item index to be rendered even if outside the visible region.
 *
 * @param clipper     Active clipper.
 * @param item_index  Index to include.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_list_clipper_include_item_by_index(eli_list_clipper *clipper, int item_index)
{
    eli_list_clipper_include_items_by_index(clipper, item_index, item_index + 1);
}

/**
 * Advance the clipper to the next visible range. On success sets display_start and
 * display_end to the item index range to render and positions the cursor at the first
 * item of that range.
 *
 * @param clipper  Active clipper.
 * @return         true while there is a range to render; false when the list is done
 *                 (at which point the cursor has been advanced past all items).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_list_clipper_step(eli_list_clipper *clipper)
{
    eli_context *ctx = clipper->ctx;
    eli_window *win = ctx ? ctx->current_window : NULL;
    if (win == NULL || win->skip_items || clipper->items_count == 0)
        return false;

    bool calc_clipping = false;
    if (clipper->step_no == 0) {
        clipper->start_pos_y = win->cursor_pos.y;
        if (clipper->items_height <= 0.0f) {
            /* Submit item 0 (front range) so the next step can measure its height. */
            for (int i = eli_lc_mini(clipper->ranges_count, ELI_LIST_CLIPPER_MAX_RANGES - 1);
                 i > 0; --i)
                clipper->ranges[i] = clipper->ranges[i - 1];
            clipper->ranges[0].min = 0;
            clipper->ranges[0].max = 1;
            if (clipper->ranges_count < ELI_LIST_CLIPPER_MAX_RANGES)
                clipper->ranges_count++;
            clipper->display_start = 0;
            clipper->display_end = eli_lc_mini(1, clipper->items_count);
            clipper->step_no = 1;
            return true;
        }
        calc_clipping = true;
    }

    if (clipper->items_height <= 0.0f) {
        /* Step 1: infer the item height from the just-measured first item. */
        int measured_n = clipper->display_end - clipper->display_start;
        float measured_h = win->cursor_pos.y - clipper->start_pos_y;
        clipper->items_height = (measured_n > 0) ? measured_h / (float)measured_n : 0.0f;
        if (clipper->items_height <= 0.0f)
            return false;
        calc_clipping = true;
    }

    const int already_submitted = clipper->display_end;
    if (calc_clipping) {
        clipper->start_seek_offset_y = 0.0;

        float min_y = win->clip_rect.y;
        float max_y = win->clip_rect.y + win->clip_rect.h;
        int m1 = (int)(((double)min_y - (double)win->cursor_pos.y) / (double)clipper->items_height);
        int m2 = (int)((((double)max_y - (double)win->cursor_pos.y) / (double)clipper->items_height)
                       + 0.999999);
        int vmin = eli_lc_clampi(already_submitted + m1, already_submitted,
                                 clipper->items_count - 1);
        int vmax = eli_lc_clampi(already_submitted + m2, vmin + 1, clipper->items_count);
        if (clipper->ranges_count < ELI_LIST_CLIPPER_MAX_RANGES) {
            clipper->ranges[clipper->ranges_count].min = vmin;
            clipper->ranges[clipper->ranges_count].max = vmax;
            clipper->ranges_count++;
        }
        eli_list_clipper_sort_and_fuse(clipper->ranges, &clipper->ranges_count, clipper->step_no);
    }

    while (clipper->step_no < clipper->ranges_count) {
        clipper->display_start = eli_lc_maxi(clipper->ranges[clipper->step_no].min,
                                             already_submitted);
        clipper->display_end = eli_lc_mini(clipper->ranges[clipper->step_no].max,
                                           clipper->items_count);
        clipper->step_no++;
        if (clipper->display_start >= clipper->display_end)
            continue;
        if (clipper->display_start > already_submitted)
            eli_list_clipper_seek_cursor_for_item(clipper, clipper->display_start);
        return true;
    }

    /* Past the last range: advance the cursor to the end so content height is right. */
    eli_list_clipper_seek_cursor_for_item(clipper, clipper->items_count);
    return false;
}

/**
 * Finish clipping. Ensures the cursor has been advanced past every item so the
 * window's measured content height (and thus the scrollbar) is correct. Safe to call
 * whether or not the step loop ran to completion; safe to call more than once.
 *
 * @param clipper  Clipper to finalize.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_list_clipper_end(eli_list_clipper *clipper)
{
    if (clipper->items_count >= 0 && clipper->display_start >= 0 && clipper->items_height > 0.0f)
        eli_list_clipper_seek_cursor_for_item(clipper, clipper->items_count);
    clipper->items_count = -1;
    clipper->display_start = -1;
}

#endif /* ELI_UTIL_ELI_LIST_CLIPPER_H */
