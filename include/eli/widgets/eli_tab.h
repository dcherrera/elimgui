/**
 * @file eli_tab.h
 * @brief Phase 20 tab-bar widget: eli_begin_tab_bar / eli_end_tab_bar,
 *        eli_begin_tab_item / eli_end_tab_item, eli_tab_item_button, and
 *        eli_set_tab_item_closed, plus the module-private tab-bar pool + stack.
 *
 * A tab bar renders a horizontal strip of tabs above its contents. Tabs persist
 * in a pooled, per-bar list keyed by id so the bar can lay them out (widths and
 * offsets), remember which tab is selected across frames, scroll when the tabs
 * overflow the strip, and reorder them by drag. eli_begin_tab_item returns true
 * for exactly the selected tab, so the caller draws that tab's contents below the
 * strip. Mirrors Dear ImGui's tab-bar behavior, trimmed to a single tab section.
 *
 * Usage (inside a window scope opened by eli_begin/eli_end):
 *     if (eli_begin_tab_bar("tabs", ELI_TAB_BAR_REORDERABLE)) {
 *         if (eli_begin_tab_item("One", NULL, 0))   { ...; eli_end_tab_item(); }
 *         if (eli_begin_tab_item("Two", &open, 0))  { ...; eli_end_tab_item(); }
 *         eli_end_tab_bar();
 *     }
 *
 * The pool is heap-allocated through the libc seam and must be released once at
 * teardown via eli_tab_shutdown(). eli_tab_new_frame() defensively resets the
 * current-bar stack at the start of a frame.
 *
 * @status Phase 20 tab bar in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WIDGETS_ELI_TAB_H
#define ELI_WIDGETS_ELI_TAB_H

#include "eli_tab_types.h"
#include "eli_widget_behavior.h"

#include "../core/eli_platform.h"

/* ---------------------------------------------------------------------------
 * Module-private pool + current-bar stack
 *
 * Tab bars persist across frames in a heap pool keyed by id (grown via the libc
 * seam, freed by eli_tab_shutdown). The active-bar stack holds pool indices, not
 * pointers, so a nested eli_begin_tab_bar that reallocates the pool cannot leave a
 * dangling parent reference.
 * ------------------------------------------------------------------------- */

#define ELI_TAB_BAR_STACK_MAX 8

static eli_tab_bar *g_eli_tab_bar_pool = NULL;
static int          g_eli_tab_bar_pool_count = 0;
static int          g_eli_tab_bar_pool_capacity = 0;

static int          g_eli_tab_bar_stack[ELI_TAB_BAR_STACK_MAX];
static int          g_eli_tab_bar_stack_size = 0;

/** Find the pooled tab bar for `id`, creating a zeroed one if absent. */
static inline eli_tab_bar *eli_tab__pool_find_or_create(eli_id id)
{
    for (int i = 0; i < g_eli_tab_bar_pool_count; i++)
        if (g_eli_tab_bar_pool[i].id == id)
            return &g_eli_tab_bar_pool[i];

    if (g_eli_tab_bar_pool_count >= g_eli_tab_bar_pool_capacity) {
        int cap = g_eli_tab_bar_pool_capacity ? g_eli_tab_bar_pool_capacity * 2 : 4;
        eli_tab_bar *grown = (eli_tab_bar *)realloc(g_eli_tab_bar_pool, (size_t)cap * sizeof(*grown));
        if (grown == NULL)
            return NULL;
        g_eli_tab_bar_pool = grown;
        g_eli_tab_bar_pool_capacity = cap;
    }
    eli_tab_bar *bar = &g_eli_tab_bar_pool[g_eli_tab_bar_pool_count++];
    memset(bar, 0, sizeof(*bar));
    bar->id = id;
    bar->curr_frame_visible = -1;
    bar->prev_frame_visible = -1;
    bar->last_tab_item_idx = -1;
    return bar;
}

/** @return the current (innermost) tab bar, or NULL when none is active. */
static inline eli_tab_bar *eli_tab__current(void)
{
    if (g_eli_tab_bar_stack_size <= 0)
        return NULL;
    return &g_eli_tab_bar_pool[g_eli_tab_bar_stack[g_eli_tab_bar_stack_size - 1]];
}

/**
 * Release the whole tab-bar pool (and each bar's tab array) and reset the stack.
 * Call once at teardown; safe to call when nothing was allocated.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_tab_shutdown(void)
{
    for (int i = 0; i < g_eli_tab_bar_pool_count; i++) {
        free(g_eli_tab_bar_pool[i].tabs);
        g_eli_tab_bar_pool[i].tabs = NULL;
    }
    free(g_eli_tab_bar_pool);
    g_eli_tab_bar_pool = NULL;
    g_eli_tab_bar_pool_count = 0;
    g_eli_tab_bar_pool_capacity = 0;
    g_eli_tab_bar_stack_size = 0;
}

/**
 * Reset the current-bar stack at the start of a frame. Defensive: a well-formed
 * frame always balances begin/end, but this recovers from a mismatched teardown.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_tab_new_frame(void)
{
    g_eli_tab_bar_stack_size = 0;
}

/* ---------------------------------------------------------------------------
 * Tab list helpers
 * ------------------------------------------------------------------------- */

/** @return the tab with the given id in this bar, or NULL. */
static inline eli_tab_item *eli_tab__find_tab(eli_tab_bar *bar, eli_id id)
{
    if (id != 0u)
        for (int i = 0; i < bar->tab_count; i++)
            if (bar->tabs[i].id == id)
                return &bar->tabs[i];
    return NULL;
}

/** @return the visible order (index) of a tab within its bar. */
static inline int eli_tab__tab_order(const eli_tab_bar *bar, const eli_tab_item *tab)
{
    return (int)(tab - bar->tabs);
}

/** Append a fresh (zeroed) tab with the given id, growing the array. NULL on OOM. */
static inline eli_tab_item *eli_tab__add_tab(eli_tab_bar *bar, eli_id id)
{
    if (bar->tab_count >= bar->tab_capacity) {
        int cap = bar->tab_capacity ? bar->tab_capacity * 2 : 4;
        eli_tab_item *grown = (eli_tab_item *)realloc(bar->tabs, (size_t)cap * sizeof(*grown));
        if (grown == NULL)
            return NULL;
        bar->tabs = grown;
        bar->tab_capacity = cap;
    }
    eli_tab_item *tab = &bar->tabs[bar->tab_count++];
    memset(tab, 0, sizeof(*tab));
    tab->id = id;
    tab->last_frame_visible = -1;
    tab->last_frame_selected = -1;
    return tab;
}

/**
 * @param label            Tab label (visible part stops at "##").
 * @param has_close_button Whether the tab reserves room for a close/unsaved marker.
 * @return                 The tab's ideal (width, height), capped to a max width.
 */
static inline eli_vec2 eli_tab__calc_size(const char *label, bool has_close_button)
{
    const eli_style *style = eli_get_style();
    eli_vec2 label_size = eli_calc_text_size(label, eli_find_rendered_text_end(label, NULL));
    float x = label_size.x + style->frame_padding.x;
    float y = label_size.y + style->frame_padding.y * 2.0f;
    if (has_close_button)
        x += style->frame_padding.x + (style->item_inner_spacing.x + eli_get_font_size());
    else
        x += style->frame_padding.x + 1.0f;
    float max_w = eli_get_font_size() * ELI_TAB_MAX_WIDTH_FONT_MULT;
    return eli_make_vec2(eli_min_f(x, max_w), y);
}

/* ---------------------------------------------------------------------------
 * Scrolling + reordering
 * ------------------------------------------------------------------------- */

/** Clamp a horizontal scroll value to the valid [0, overflow] range. */
static inline float eli_tab__scroll_clamp(const eli_tab_bar *bar, float scrolling)
{
    float max_scroll = eli_max_f(0.0f, bar->width_all_tabs - bar->bar_rect.w);
    return eli_clamp_f(scrolling, 0.0f, max_scroll);
}

/** Nudge the scroll target so the given tab is fully visible in the strip. */
static inline void eli_tab__scroll_to_tab(eli_tab_bar *bar, eli_id id)
{
    eli_tab_item *tab = eli_tab__find_tab(bar, id);
    if (tab == NULL)
        return;
    float margin = eli_get_font_size();
    float x1 = tab->offset - margin;
    float x2 = tab->offset + tab->width + margin;
    if (bar->scrolling_target > x1)
        bar->scrolling_target = x1;
    if (bar->scrolling_target + bar->bar_rect.w < x2)
        bar->scrolling_target = x2 - bar->bar_rect.w;
}

/**
 * Apply a queued reorder request: move the requested tab by its offset within the
 * list. Mirrors Dear ImGui's TabBarProcessReorder block-move.
 *
 * @return true if the tabs were actually moved.
 */
static inline bool eli_tab__process_reorder(eli_tab_bar *bar)
{
    eli_tab_item *tab1 = eli_tab__find_tab(bar, bar->reorder_request_tab_id);
    if (tab1 == NULL || (tab1->flags & ELI_TAB_ITEM_NO_REORDER))
        return false;
    int order1 = eli_tab__tab_order(bar, tab1);
    int order2 = order1 + bar->reorder_request_offset;
    if (order2 < 0 || order2 >= bar->tab_count)
        return false;
    eli_tab_item *tab2 = &bar->tabs[order2];
    if (tab2->flags & ELI_TAB_ITEM_NO_REORDER)
        return false;

    eli_tab_item tmp = *tab1;
    if (bar->reorder_request_offset > 0)
        memmove(tab1, tab1 + 1, (size_t)(order2 - order1) * sizeof(*tab1));
    else
        memmove(&bar->tabs[order2 + 1], &bar->tabs[order2],
                (size_t)(order1 - order2) * sizeof(*tab1));
    bar->tabs[order2] = tmp;
    return true;
}

/** Queue a reorder derived from the mouse position while dragging `src`. */
static inline void eli_tab__queue_reorder_from_mouse(eli_tab_bar *bar, eli_tab_item *src,
                                                     eli_vec2 mouse)
{
    if ((bar->flags & ELI_TAB_BAR_REORDERABLE) == 0 || bar->reorder_request_tab_id != 0)
        return;
    float spacing = eli_get_style()->item_inner_spacing.x;
    float bar_offset = bar->bar_rect.x - bar->scrolling_target;
    int dir = (bar_offset + src->offset) > mouse.x ? -1 : +1;
    int src_idx = eli_tab__tab_order(bar, src);
    int dst_idx = src_idx;
    for (int i = src_idx; i >= 0 && i < bar->tab_count; i += dir) {
        eli_tab_item *dst = &bar->tabs[i];
        if (dst->flags & ELI_TAB_ITEM_NO_REORDER)
            break;
        dst_idx = i;
        float x1 = bar_offset + dst->offset - spacing;
        float x2 = bar_offset + dst->offset + dst->width + spacing;
        if ((dir < 0 && mouse.x > x1) || (dir > 0 && mouse.x < x2))
            break;
    }
    if (dst_idx != src_idx) {
        bar->reorder_request_tab_id = src->id;
        bar->reorder_request_offset = dst_idx - src_idx;
    }
}

/* ---------------------------------------------------------------------------
 * Layout
 * ------------------------------------------------------------------------- */

/** Remove tabs that stopped being submitted or were flagged closed. */
static inline void eli_tab__layout_gc(eli_tab_bar *bar)
{
    int dst = 0;
    for (int src = 0; src < bar->tab_count; src++) {
        eli_tab_item *tab = &bar->tabs[src];
        if (tab->last_frame_visible < bar->prev_frame_visible || tab->want_close) {
            if (bar->visible_tab_id == tab->id)
                bar->visible_tab_id = 0;
            if (bar->selected_tab_id == tab->id)
                bar->selected_tab_id = 0;
            if (bar->next_selected_tab_id == tab->id)
                bar->next_selected_tab_id = 0;
            continue;
        }
        if (dst != src)
            bar->tabs[dst] = bar->tabs[src];
        dst++;
    }
    bar->tab_count = dst;
}

/** Proportionally shrink tab widths so the whole strip fits the bar width. */
static inline void eli_tab__shrink_to_fit(eli_tab_bar *bar, float spacing)
{
    float spacing_total = (bar->tab_count > 1) ? spacing * (float)(bar->tab_count - 1) : 0.0f;
    float avail = eli_max_f(1.0f, bar->bar_rect.w - spacing_total);
    float sum = 0.0f;
    for (int i = 0; i < bar->tab_count; i++)
        sum += bar->tabs[i].width;
    if (sum <= avail)
        return;
    float scale = avail / sum;
    for (int i = 0; i < bar->tab_count; i++)
        bar->tabs[i].width = eli_max_f(1.0f, eli_layout_trunc(bar->tabs[i].width * scale));
}

/**
 * Lay out the tab bar: garbage-collect dead tabs, resolve the selected tab from
 * queued selection/reorder requests, compute each tab's width and offset, apply
 * the fitting policy, and update scrolling. Runs once per frame just before the
 * first tab item (or at end if none). Mirrors Dear ImGui's TabBarLayout.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_tab__layout(eli_tab_bar *bar)
{
    eli_context *ctx = eli_get_current_context();
    const eli_style *style = &ctx->style;
    bar->want_layout = false;

    bool scroll_to_selected = (bar->bar_rect_prev_width > bar->bar_rect.w);
    bar->bar_rect_prev_width = bar->bar_rect.w;

    eli_tab__layout_gc(bar);

    eli_id scroll_to_id = 0;
    if (bar->next_selected_tab_id != 0) {
        bar->selected_tab_id = bar->next_selected_tab_id;
        bar->next_selected_tab_id = 0;
        scroll_to_id = bar->selected_tab_id;
    }
    if (bar->reorder_request_tab_id != 0) {
        if (eli_tab__process_reorder(bar) && bar->reorder_request_tab_id == bar->selected_tab_id)
            scroll_to_id = bar->reorder_request_tab_id;
        bar->reorder_request_tab_id = 0;
    }

    float spacing = style->item_inner_spacing.x;
    eli_tab_item *most_recent = NULL;
    bool found_selected = false;
    float total = 0.0f;
    for (int i = 0; i < bar->tab_count; i++) {
        eli_tab_item *tab = &bar->tabs[i];
        if ((most_recent == NULL || most_recent->last_frame_selected < tab->last_frame_selected) &&
            !(tab->flags & ELI_TAB_ITEM_BUTTON))
            most_recent = tab;
        if (tab->id == bar->selected_tab_id)
            found_selected = true;
        tab->width = eli_max_f(tab->content_width, 1.0f);
        total += tab->width + (i > 0 ? spacing : 0.0f);
    }
    bar->width_all_tabs_ideal = total;

    if (total > bar->bar_rect.w && (bar->flags & ELI_TAB_BAR_FITTING_POLICY_SHRINK) &&
        !(bar->flags & ELI_TAB_BAR_FITTING_POLICY_SCROLL))
        eli_tab__shrink_to_fit(bar, spacing);

    float offset = 0.0f;
    for (int i = 0; i < bar->tab_count; i++) {
        bar->tabs[i].offset = offset;
        offset += bar->tabs[i].width + (i < bar->tab_count - 1 ? spacing : 0.0f);
    }
    bar->width_all_tabs = offset;

    bool appearing = (bar->prev_frame_visible + 1 < ctx->frame_count);
    if (!found_selected && !appearing)
        bar->selected_tab_id = 0;
    if (bar->selected_tab_id == 0 && bar->next_selected_tab_id == 0 && most_recent != NULL) {
        bar->selected_tab_id = most_recent->id;
        scroll_to_id = most_recent->id;
    }
    bar->visible_tab_id = bar->selected_tab_id;
    bar->visible_tab_was_submitted = false;

    if (scroll_to_id != 0)
        eli_tab__scroll_to_tab(bar, scroll_to_id);
    else if (scroll_to_selected)
        eli_tab__scroll_to_tab(bar, bar->selected_tab_id);
    bar->scrolling_target = eli_tab__scroll_clamp(bar, bar->scrolling_target);
    bar->scrolling_anim = bar->scrolling_target;

    eli_window *win = ctx->current_window;
    win->cursor_pos = eli_make_vec2(bar->bar_rect.x, bar->bar_rect.y);
    eli_item_size(eli_make_vec2(bar->width_all_tabs, bar->bar_rect.h), bar->frame_padding.y);
}

/* ---------------------------------------------------------------------------
 * Tab item rendering
 * ------------------------------------------------------------------------- */

/** Draw the tab's rounded-top background (and optional border). */
static inline void eli_tab__render_background(eli_draw_list *dl, eli_rect bb,
                                              eli_tab_item_flags flags, eli_col32 col)
{
    const eli_style *style = eli_get_style();
    if (bb.w <= 0.0f)
        return;
    float rounding = eli_max_f(0.0f,
        eli_min_f((flags & ELI_TAB_ITEM_BUTTON) ? style->frame_rounding : style->tab_rounding,
                  bb.w * 0.5f - 1.0f));
    eli_vec2 p_min = eli_make_vec2(bb.x, bb.y + 1.0f);
    eli_vec2 p_max = eli_make_vec2(bb.x + bb.w, bb.y + bb.h - style->tab_bar_border_size);
    eli_draw_list_add_rect_filled(dl, p_min, p_max, col, rounding, ELI_DRAW_ROUND_CORNERS_TOP);
    if (style->tab_border_size > 0.0f)
        eli_draw_list_add_rect(dl, p_min, p_max, eli_get_color_u32(ELI_COL_BORDER, 1.0f), rounding,
                               ELI_DRAW_ROUND_CORNERS_TOP, style->tab_border_size);
}

/** Draw an "x" close glyph (with a hover/held background) at `pos`. */
static inline void eli_tab__render_close_glyph(eli_draw_list *dl, eli_vec2 pos, float sz,
                                               bool hovered, bool held)
{
    if (hovered)
        eli_draw_list_add_rect_filled(dl, pos, eli_make_vec2(pos.x + sz, pos.y + sz),
                                      eli_get_color_u32(held ? ELI_COL_BUTTON_ACTIVE
                                                             : ELI_COL_BUTTON_HOVERED, 1.0f),
                                      0.0f, ELI_DRAW_ROUND_CORNERS_NONE);
    eli_col32 col = eli_get_color_u32(ELI_COL_TEXT, 1.0f);
    eli_vec2 c = eli_make_vec2(pos.x + sz * 0.5f - 0.5f, pos.y + sz * 0.5f - 0.5f);
    float e = sz * 0.5f * 0.7071f - 1.0f;
    eli_draw_list_add_line(dl, eli_make_vec2(c.x + e, c.y + e), eli_make_vec2(c.x - e, c.y - e),
                           col, 1.0f);
    eli_draw_list_add_line(dl, eli_make_vec2(c.x + e, c.y - e), eli_make_vec2(c.x - e, c.y + e),
                           col, 1.0f);
}

/** Draw the tab label (clipped to its inner rect) and the unsaved marker. */
static inline void eli_tab__render_label(eli_draw_list *dl, eli_rect bb, eli_tab_item_flags flags,
                                         const char *label, eli_vec2 frame_padding,
                                         bool close_visible, float button_sz)
{
    float clip_r = close_visible ? (bb.x + bb.w - frame_padding.x - button_sz) : (bb.x + bb.w);
    eli_draw_list_push_clip_rect(dl, eli_make_vec2(bb.x + frame_padding.x, bb.y),
                                 eli_make_vec2(clip_r, bb.y + bb.h), true);
    eli_render_text(eli_make_vec2(bb.x + frame_padding.x, bb.y + frame_padding.y),
                    eli_get_color_u32(ELI_COL_TEXT, 1.0f), label, NULL, true);
    eli_draw_list_pop_clip_rect(dl);

    if ((flags & ELI_TAB_ITEM_UNSAVED_DOCUMENT) && !close_visible) {
        eli_vec2 marker = eli_make_vec2(bb.x + bb.w - frame_padding.x - button_sz,
                                        bb.y + frame_padding.y);
        eli_render_bullet(dl, marker, eli_get_color_u32(ELI_COL_TEXT, 1.0f));
    }
}

/** Flag a tab for closure; layout selects a neighbor if it was the visible tab. */
static inline void eli_tab__close_tab(eli_tab_bar *bar, eli_tab_item *tab)
{
    if (tab->flags & ELI_TAB_ITEM_BUTTON)
        return;
    tab->want_close = true;
    if (bar->next_selected_tab_id == tab->id)
        bar->next_selected_tab_id = 0;
}

/* ---------------------------------------------------------------------------
 * Tab item core
 * ------------------------------------------------------------------------- */

/** Resolve the close button geometry for a tab; returns whether it is shown. */
static inline bool eli_tab__close_geometry(eli_rect bb, eli_vec2 frame_padding, float button_sz,
                                           bool has_p_open, eli_vec2 *out_pos)
{
    if (!has_p_open || bb.w < button_sz)
        return false;
    out_pos->x = eli_max_f(bb.x, bb.x + bb.w - frame_padding.x - button_sz);
    out_pos->y = bb.y + frame_padding.y;
    return true;
}

/**
 * The shared tab submission core behind eli_begin_tab_item / eli_tab_item_button.
 * Registers/updates the tab, positions it in the strip, handles selection, close,
 * and drag-reorder interaction, and renders the tab. Mirrors Dear ImGui's TabItemEx.
 *
 * @param bar     The active tab bar.
 * @param label   Tab label / identity.
 * @param p_open  Optional close flag; a non-NULL, true target shows a close button
 *                that clears the target when clicked (may be NULL).
 * @param flags   eli_tab_item_flags.
 * @return        For a normal tab: true when its contents are visible (selected).
 *                For a tab button: true on the frame it is pressed.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_tab__item_ex(eli_tab_bar *bar, const char *label, bool *p_open,
                                    eli_tab_item_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (bar->want_layout)
        eli_tab__layout(bar);
    eli_window *win = ctx->current_window;
    if (win == NULL || win->skip_items)
        return false;
    const eli_style *style = &ctx->style;

    eli_id id = eli_get_id(label);
    if (p_open != NULL && !*p_open) {
        eli_item_add(id, eli_make_rect(0.0f, 0.0f, 0.0f, 0.0f), 0);
        return false;
    }
    if (flags & ELI_TAB_ITEM_NO_CLOSE_BUTTON)
        p_open = NULL;
    else if (p_open == NULL)
        flags |= ELI_TAB_ITEM_NO_CLOSE_BUTTON;
    bool is_tab_button = (flags & ELI_TAB_ITEM_BUTTON) != 0;

    eli_tab_item *tab = eli_tab__find_tab(bar, id);
    bool tab_is_new = false;
    if (tab == NULL) {
        tab = eli_tab__add_tab(bar, id);
        if (tab == NULL)
            return false;
        bar->tabs_added_new = tab_is_new = true;
    }
    bar->last_tab_item_idx = eli_tab__tab_order(bar, tab);

    bool has_close = (p_open != NULL) || (flags & ELI_TAB_ITEM_UNSAVED_DOCUMENT);
    eli_vec2 size = eli_tab__calc_size(label, has_close);
    if (tab_is_new)
        tab->width = eli_max_f(1.0f, size.x);
    tab->content_width = size.x;
    tab->begin_order = bar->tabs_active_count++;

    bool tab_bar_appearing = (bar->prev_frame_visible + 1 < ctx->frame_count);
    bool tab_appearing = (tab->last_frame_visible + 1 < ctx->frame_count);
    tab->last_frame_visible = ctx->frame_count;
    tab->flags = flags;

    if (!is_tab_button) {
        if (tab_appearing && (bar->flags & ELI_TAB_BAR_AUTO_SELECT_NEW_TABS) &&
            bar->next_selected_tab_id == 0 && (!tab_bar_appearing || bar->selected_tab_id == 0))
            bar->next_selected_tab_id = id;
        if ((flags & ELI_TAB_ITEM_SET_SELECTED) && bar->selected_tab_id != id)
            bar->next_selected_tab_id = id;
    }

    bool tab_contents_visible = (bar->visible_tab_id == id);
    if (tab_contents_visible)
        bar->visible_tab_was_submitted = true;
    if (!tab_contents_visible && bar->selected_tab_id == 0 && tab_bar_appearing &&
        bar->tab_count == 1 && !(bar->flags & ELI_TAB_BAR_AUTO_SELECT_NEW_TABS))
        tab_contents_visible = true;

    if (tab_appearing && (!tab_bar_appearing || tab_is_new)) {
        eli_item_add(id, eli_make_rect(0.0f, 0.0f, 0.0f, 0.0f), 0);
        return is_tab_button ? false : tab_contents_visible;
    }
    if (bar->selected_tab_id == id)
        tab->last_frame_selected = ctx->frame_count;

    /* Position the tab within the strip (scrolled), reserving no layout cursor. */
    eli_vec2 backup_cursor = win->cursor_pos;
    size.x = tab->width;
    eli_vec2 pos = eli_make_vec2(bar->bar_rect.x + eli_layout_trunc(tab->offset - bar->scrolling_anim),
                                 bar->bar_rect.y);
    win->cursor_pos = pos;
    eli_rect bb = eli_make_rect(pos.x, pos.y, size.x, size.y);

    eli_draw_list *dl = eli_get_window_draw_list();
    bool want_clip = (bb.x < bar->bar_rect.x || bb.x + bb.w > bar->bar_rect.x + bar->bar_rect.w);
    if (want_clip)
        eli_draw_list_push_clip_rect(dl, eli_make_vec2(eli_max_f(bb.x, bar->bar_rect.x), bb.y - 1.0f),
                                     eli_make_vec2(bar->bar_rect.x + bar->bar_rect.w, bb.y + bb.h),
                                     true);

    eli_vec2 backup_cursor_max = win->cursor_max_pos;
    eli_item_size(eli_rect_size(bb), style->frame_padding.y);
    win->cursor_max_pos = backup_cursor_max;
    if (!eli_item_add(id, bb, 0)) {
        if (want_clip)
            eli_draw_list_pop_clip_rect(dl);
        win->cursor_pos = backup_cursor;
        return is_tab_button ? false : tab_contents_visible;
    }
    bool is_visible = (eli_get_item_status_flags() & ELI_ITEM_STATUS_VISIBLE) != 0;

    /* Interaction: close button first so it captures the click over the tab. */
    float button_sz = eli_get_font_size();
    eli_vec2 close_pos = eli_make_vec2(0.0f, 0.0f);
    bool close_visible = eli_tab__close_geometry(bb, style->frame_padding, button_sz,
                                                 p_open != NULL, &close_pos);
    eli_id close_id = close_visible ? eli_hash_str("#CLOSE", id) : 0u;
    bool close_hovered = false, close_held = false, just_closed = false;
    if (close_visible) {
        eli_rect cbb = eli_make_rect(close_pos.x, close_pos.y, button_sz, button_sz);
        just_closed = eli_button_behavior(cbb, close_id, &close_hovered, &close_held,
                                          ELI_BUTTON_NONE);
    }

    int tab_button_flags = is_tab_button ? ELI_BUTTON_PRESSED_ON_CLICK_RELEASE
                                         : ELI_BUTTON_PRESSED_ON_CLICK;
    bool hovered = false, held = false;
    bool pressed = eli_button_behavior(bb, id, &hovered, &held, tab_button_flags);
    if (pressed && !is_tab_button)
        bar->next_selected_tab_id = id;

    bool mid_ok = !(flags & ELI_TAB_ITEM_NO_CLOSE_WITH_MIDDLE_MOUSE) &&
                  !(bar->flags & ELI_TAB_BAR_NO_CLOSE_WITH_MIDDLE_MOUSE);
    if (p_open != NULL && mid_ok && (hovered || close_hovered) &&
        eli_is_mouse_clicked(ELI_MOUSE_BUTTON_MIDDLE))
        just_closed = true;

    if (held && !tab_appearing && (bar->flags & ELI_TAB_BAR_REORDERABLE) &&
        eli_is_mouse_dragging(ELI_MOUSE_BUTTON_LEFT, -1.0f)) {
        eli_vec2 mouse = eli_get_mouse_pos();
        eli_vec2 delta = eli_get_mouse_drag_delta(ELI_MOUSE_BUTTON_LEFT, -1.0f);
        if ((delta.x < 0.0f && mouse.x < bb.x) || (delta.x > 0.0f && mouse.x > bb.x + bb.w))
            eli_tab__queue_reorder_from_mouse(bar, tab, mouse);
    }

    if (is_visible) {
        eli_col32 tab_col = eli_get_color_u32(
            (held || hovered) ? ELI_COL_TAB_HOVERED
                              : tab_contents_visible ? ELI_COL_TAB_SELECTED : ELI_COL_TAB, 1.0f);
        eli_tab__render_background(dl, bb, flags, tab_col);
        if (tab_contents_visible && (bar->flags & ELI_TAB_BAR_DRAW_SELECTED_OVERLINE) &&
            style->tab_bar_overline_size > 0.0f)
            eli_draw_list_add_line(dl, eli_make_vec2(bb.x, bb.y + 1.0f),
                                   eli_make_vec2(bb.x + bb.w, bb.y + 1.0f),
                                   eli_get_color_u32(ELI_COL_TAB_SELECTED_OVERLINE, 1.0f),
                                   style->tab_bar_overline_size);
        eli_render_nav_highlight(bb, id);
        eli_tab__render_label(dl, bb, flags, label, style->frame_padding, close_visible, button_sz);
        if (close_visible)
            eli_tab__render_close_glyph(dl, close_pos, button_sz, close_hovered, close_held);
    }
    if (just_closed && p_open != NULL) {
        *p_open = false;
        eli_tab__close_tab(bar, tab);
    }

    if (want_clip)
        eli_draw_list_pop_clip_rect(dl);
    win->cursor_pos = backup_cursor;
    return is_tab_button ? pressed : tab_contents_visible;
}

/* ---------------------------------------------------------------------------
 * Public tab-bar API
 * ------------------------------------------------------------------------- */

/**
 * Open a tab bar. Submit tabs with eli_begin_tab_item between this and the
 * matching eli_end_tab_bar. Must be called inside a window scope.
 *
 * @param str_id  Identity string for the bar.
 * @param flags   eli_tab_bar_flags.
 * @return        true if the bar is open (always call eli_end_tab_bar to match).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_begin_tab_bar(const char *str_id, eli_tab_bar_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return false;
    eli_window *win = ctx->current_window;

    eli_id id = eli_get_id(str_id);
    eli_tab_bar *bar = eli_tab__pool_find_or_create(id);
    if (bar == NULL)
        return false;
    bar->id = id;

    float x1 = win->cursor_pos.x;
    float y1 = win->cursor_pos.y;
    float x2 = win->content_region_rect.x + win->content_region_rect.w;
    float bar_h = eli_get_font_size() + ctx->style.frame_padding.y * 2.0f;
    eli_rect bar_bb = eli_make_rect(x1, y1, eli_max_f(1.0f, x2 - x1), bar_h);

    eli_id_stack_push_raw(ctx, id);
    int pool_idx = (int)(bar - g_eli_tab_bar_pool);
    if (g_eli_tab_bar_stack_size >= ELI_TAB_BAR_STACK_MAX) {
        eli_pop_id();
        return false;
    }
    g_eli_tab_bar_stack[g_eli_tab_bar_stack_size++] = pool_idx;

    bar->backup_cursor_pos = win->cursor_pos;
    if (bar->curr_frame_visible == ctx->frame_count) {
        win->cursor_pos = eli_make_vec2(bar->bar_rect.x,
                                        bar->bar_rect.y + bar->bar_rect.h + bar->item_spacing_y);
        bar->begin_count++;
        return true;
    }

    if ((flags & ELI_TAB_BAR_FITTING_POLICY_MASK) == 0)
        flags |= ELI_TAB_BAR_FITTING_POLICY_DEFAULT;
    bar->flags = flags;
    bar->bar_rect = bar_bb;
    bar->want_layout = true;
    bar->prev_frame_visible = bar->curr_frame_visible;
    bar->curr_frame_visible = ctx->frame_count;
    bar->prev_tabs_contents_height = bar->curr_tabs_contents_height;
    bar->curr_tabs_contents_height = 0.0f;
    bar->item_spacing_y = ctx->style.item_spacing.y;
    bar->frame_padding = ctx->style.frame_padding;
    bar->tabs_active_count = 0;
    bar->last_tab_item_idx = -1;
    bar->begin_count = 1;
    bar->tabs_added_new = false;

    win->cursor_pos = eli_make_vec2(bar->bar_rect.x,
                                    bar->bar_rect.y + bar->bar_rect.h + bar->item_spacing_y);

    if (ctx->style.tab_bar_border_size > 0.0f) {
        eli_draw_list *dl = eli_get_window_draw_list();
        float y = bar->bar_rect.y + bar->bar_rect.h;
        eli_draw_list_add_rect_filled(
            dl, eli_make_vec2(bar->bar_rect.x, y - ctx->style.tab_bar_border_size),
            eli_make_vec2(bar->bar_rect.x + bar->bar_rect.w, y),
            eli_get_color_u32(ELI_COL_TAB_SELECTED, 1.0f), 0.0f, ELI_DRAW_ROUND_CORNERS_NONE);
    }
    return true;
}

/**
 * Close the current tab bar opened by eli_begin_tab_bar. Advances the window
 * cursor below the tab-bar contents.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_end_tab_bar(void)
{
    eli_context *ctx = eli_get_current_context();
    eli_tab_bar *bar = eli_tab__current();
    if (ctx == NULL || bar == NULL)
        return;
    eli_window *win = ctx->current_window;

    if (win != NULL && !win->skip_items) {
        if (bar->want_layout)
            eli_tab__layout(bar);
        float bar_bottom = bar->bar_rect.y + bar->bar_rect.h;
        bool appearing = (bar->prev_frame_visible + 1 < ctx->frame_count);
        if (bar->visible_tab_was_submitted || bar->visible_tab_id == 0 || appearing) {
            bar->curr_tabs_contents_height =
                eli_max_f(win->cursor_pos.y - bar_bottom, bar->curr_tabs_contents_height);
            win->cursor_pos.y = bar_bottom + bar->curr_tabs_contents_height;
        } else {
            win->cursor_pos.y = bar_bottom + bar->prev_tabs_contents_height;
        }
        if (bar->begin_count > 1)
            win->cursor_pos = bar->backup_cursor_pos;
    }
    bar->last_tab_item_idx = -1;

    if (g_eli_tab_bar_stack_size > 0)
        g_eli_tab_bar_stack_size--;
    eli_pop_id();
}

/**
 * Submit a tab. When it returns true the tab is selected: draw its contents and
 * call eli_end_tab_item. Must be called between begin/end tab bar.
 *
 * @param label   Tab label (visible part stops at "##").
 * @param p_open  Optional pointer to a bool; when non-NULL the tab shows a close
 *                button that sets *p_open to false when clicked (may be NULL).
 * @param flags   eli_tab_item_flags.
 * @return        true when the tab is selected (its contents should be drawn).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_begin_tab_item(const char *label, bool *p_open, eli_tab_item_flags flags)
{
    eli_tab_bar *bar = eli_tab__current();
    eli_context *ctx = eli_get_current_context();
    if (bar == NULL || ctx == NULL || ctx->current_window == NULL ||
        ctx->current_window->skip_items)
        return false;

    bool ret = eli_tab__item_ex(bar, label, p_open, flags & ~ELI_TAB_ITEM_BUTTON);
    if (ret && !(flags & ELI_TAB_ITEM_NO_PUSH_ID)) {
        eli_tab_item *tab = &bar->tabs[bar->last_tab_item_idx];
        eli_id_stack_push_raw(ctx, tab->id);
    }
    return ret;
}

/**
 * Close a tab item opened by eli_begin_tab_item. Only call it when the matching
 * eli_begin_tab_item returned true.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_end_tab_item(void)
{
    eli_tab_bar *bar = eli_tab__current();
    if (bar == NULL || bar->last_tab_item_idx < 0 || bar->last_tab_item_idx >= bar->tab_count)
        return;
    eli_tab_item *tab = &bar->tabs[bar->last_tab_item_idx];
    if (!(tab->flags & ELI_TAB_ITEM_NO_PUSH_ID))
        eli_pop_id();
}

/**
 * Submit a tab that behaves like a button (never becomes the selected tab).
 *
 * @param label  Button label / identity.
 * @param flags  eli_tab_item_flags.
 * @return       true on the frame the tab button is pressed.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_tab_item_button(const char *label, eli_tab_item_flags flags)
{
    eli_tab_bar *bar = eli_tab__current();
    if (bar == NULL)
        return false;
    return eli_tab__item_ex(bar, label, NULL,
                            flags | ELI_TAB_ITEM_BUTTON | ELI_TAB_ITEM_NO_REORDER);
}

/**
 * Flag a tab (by label) as closed so the bar removes it on the next layout and
 * avoids a one-frame visual glitch. Call between begin/end tab bar.
 *
 * @param label  Label of the tab to close.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_set_tab_item_closed(const char *label)
{
    eli_tab_bar *bar = eli_tab__current();
    if (bar == NULL)
        return;
    eli_id id = eli_get_id(label);
    eli_tab_item *tab = eli_tab__find_tab(bar, id);
    if (tab != NULL)
        tab->want_close = true;
}

#endif /* ELI_WIDGETS_ELI_TAB_H */
