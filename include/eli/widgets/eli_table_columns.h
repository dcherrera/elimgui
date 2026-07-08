/**
 * @file eli_table_columns.h
 * @brief Table column setup, the per-frame width/position layout solver, and the
 *        column-oriented query API (count/index/name/flags/enabled/hovered) plus
 *        scroll-freeze setup.
 *
 * The layout solver mirrors Dear ImGui's TableUpdateLayout: it resolves each
 * column's effective width policy (fixed vs stretch), computes fixed slot widths
 * from the requested/auto content width, distributes the remaining space across
 * stretch columns by weight, and lays out the screen-space column edges in display
 * order (skipping hidden columns). It runs once per table, locked at the first row.
 *
 * @status Phase 19 column layout + queries in use.
 * @issues None
 * @todo Column reordering permutes display_order only; no drag-reorder UI yet.
 */
#ifndef ELI_WIDGETS_ELI_TABLE_COLUMNS_H
#define ELI_WIDGETS_ELI_TABLE_COLUMNS_H

#include "eli_table_types.h"

#include "../window/eli_window.h"
#include "../layout/eli_layout.h"
#include "../font/eli_font.h"
#include "../style/eli_style.h"

#include "../core/eli_platform.h"
#include "../core/eli_context.h"

/* ---------------------------------------------------------------------------
 * Setup
 * ------------------------------------------------------------------------- */

/**
 * Declare the next column of the current table. Must be called after
 * eli_begin_table and before the first row. Column state (width, hidden, sort)
 * persists across frames keyed by the table id; the one-time seed values here
 * apply only on the column's first appearance.
 *
 * @param label                 Header label (copied; may be NULL for an unnamed column).
 * @param flags                 eli_table_column_flags (width policy, sort/hide rules).
 * @param init_width_or_weight  Initial fixed content width (fixed columns) or stretch
 *                              weight (stretch columns). <= 0 uses defaults.
 * @param user_id               Optional stable user id surfaced in the sort specs.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_table_setup_column(const char *label, eli_table_column_flags flags,
                                          float init_width_or_weight, eli_id user_id)
{
    eli_table *table = eli_table_get_current();
    if (table == NULL || table->is_layout_locked)
        return;
    int i = table->setup_column_next;
    if (i < 0 || i >= table->columns_count)
        return;

    eli_table_column *c = &table->columns[i];
    c->flags_in = flags;
    c->is_setup = true;
    c->user_id = user_id;

    if (label != NULL && label[0] != '\0') {
        eli_table__copy_name(c->name, label);
        c->has_name = true;
    } else {
        c->name[0] = '\0';
        c->has_name = false;
    }

    if (!c->init_done) {
        c->init_done = true;
        c->is_enabled = (flags & (ELI_TABLE_COLUMN_DEFAULT_HIDE | ELI_TABLE_COLUMN_DISABLED)) == 0;
        if (flags & ELI_TABLE_COLUMN_WIDTH_STRETCH)
            c->stretch_weight = (init_width_or_weight > 0.0f) ? init_width_or_weight : 1.0f;
        else
            c->width_requested = (init_width_or_weight > 0.0f) ? init_width_or_weight : 0.0f;

        if (flags & ELI_TABLE_COLUMN_DEFAULT_SORT) {
            c->sort_order = 0;
            c->sort_direction = (flags & ELI_TABLE_COLUMN_PREFER_SORT_DESCENDING)
                                    ? ELI_SORT_DESCENDING : ELI_SORT_ASCENDING;
            table->sort_dirty = true;
        } else {
            c->sort_order = -1;
            c->sort_direction = ELI_SORT_NONE;
        }
    }

    if (flags & ELI_TABLE_COLUMN_DISABLED)
        c->is_enabled = false;

    table->setup_column_next++;
}

/**
 * Freeze a number of leading columns and/or rows so they stay pinned while the
 * table scrolls. Recorded for the current table; must be called before the first
 * row. (Freeze rendering is a scrolling refinement — see eli_table.h.)
 *
 * @param cols  Number of leading columns to freeze.
 * @param rows  Number of leading rows to freeze.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_table_setup_scroll_freeze(int cols, int rows)
{
    eli_table *table = eli_table_get_current();
    if (table == NULL || table->is_layout_locked)
        return;
    table->freeze_columns = (cols < 0) ? 0 : cols;
    table->freeze_rows = (rows < 0) ? 0 : rows;
}

/* ---------------------------------------------------------------------------
 * Layout solver
 * ------------------------------------------------------------------------- */

/** Derive the auto (content-fit) width of a column from its header label. */
static inline float eli_table__calc_auto_width(const eli_table_column *c)
{
    if (c->has_name && (c->flags & ELI_TABLE_COLUMN_NO_HEADER_WIDTH) == 0) {
        eli_vec2 sz = eli_calc_text_size(c->name, NULL);
        if (sz.x >= 1.0f)
            return sz.x;
    }
    return ELI_TABLE_DEFAULT_COLUMN_WIDTH;
}

/** Resolve the default column width policy (fixed vs stretch) from table flags. */
static inline bool eli_table__default_is_fixed(eli_table_flags flags)
{
    eli_table_flags sizing = flags & ELI_TABLE_SIZING_MASK_;
    if (sizing == 0)
        sizing = (flags & ELI_TABLE_SCROLL_X) ? ELI_TABLE_SIZING_FIXED_FIT
                                              : ELI_TABLE_SIZING_STRETCH_SAME;
    return (sizing == ELI_TABLE_SIZING_FIXED_FIT || sizing == ELI_TABLE_SIZING_FIXED_SAME);
}

/**
 * Resolve every column's effective width policy, compute fixed slot widths, split
 * the remaining space across stretch columns by weight, and lay out screen-space
 * column edges in display order. Runs once and locks the layout for the frame.
 *
 * @param table  Table being built (must be non-NULL).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_table__update_layout(eli_table *table)
{
    if (table->is_layout_locked)
        return;
    table->is_layout_locked = true;

    int n = table->columns_count;
    bool default_fixed = eli_table__default_is_fixed(table->flags);
    float inner_pad = table->cell_padding_x;

    /* Pass 1: resolve effective flags + fixed slot widths; sum weights. */
    float sum_fixed = 0.0f;
    float sum_weights = 0.0f;
    bool any_sortable = false;
    for (int i = 0; i < n; i++) {
        eli_table_column *c = &table->columns[i];

        /* Columns never passed to setup_column still exist and default to enabled. */
        if (!c->init_done) {
            c->init_done = true;
            c->is_enabled = true;
            c->sort_order = -1;
            c->sort_direction = ELI_SORT_NONE;
        }

        eli_table_column_flags f = c->flags_in;

        if ((f & ELI_TABLE_COLUMN_WIDTH_MASK_) == 0)
            f |= default_fixed ? ELI_TABLE_COLUMN_WIDTH_FIXED : ELI_TABLE_COLUMN_WIDTH_STRETCH;
        if ((table->flags & ELI_TABLE_SCROLL_X) && (f & ELI_TABLE_COLUMN_WIDTH_STRETCH)) {
            f &= ~ELI_TABLE_COLUMN_WIDTH_STRETCH;
            f |= ELI_TABLE_COLUMN_WIDTH_FIXED;
        }
        c->flags = f;
        if (f & ELI_TABLE_COLUMN_DISABLED)
            c->is_enabled = false;
        c->width_auto = eli_table__calc_auto_width(c);

        if ((table->flags & ELI_TABLE_SORTABLE) && (f & ELI_TABLE_COLUMN_NO_SORT) == 0)
            any_sortable = true;

        if (!c->is_enabled)
            continue;
        if (f & ELI_TABLE_COLUMN_WIDTH_FIXED) {
            float w = (c->width_requested > 0.0f) ? c->width_requested : c->width_auto;
            c->width_given = w + inner_pad * 2.0f;
            sum_fixed += c->width_given;
        } else {
            sum_weights += (c->stretch_weight > 0.0f) ? c->stretch_weight : 1.0f;
        }
    }
    table->is_sortable = (table->flags & ELI_TABLE_SORTABLE) != 0 && any_sortable;

    /* Pass 2: distribute the remaining width to stretch columns by weight. */
    float work_width = table->work_max_x - table->work_min_x;
    float remaining = work_width - sum_fixed;
    if (remaining < 0.0f)
        remaining = 0.0f;
    float min_slot = inner_pad * 2.0f + 1.0f;
    for (int i = 0; i < n; i++) {
        eli_table_column *c = &table->columns[i];
        if (!c->is_enabled || (c->flags & ELI_TABLE_COLUMN_WIDTH_FIXED))
            continue;
        float wt = (c->stretch_weight > 0.0f) ? c->stretch_weight : 1.0f;
        float w = (sum_weights > 0.0f) ? remaining * (wt / sum_weights) : 0.0f;
        c->width_given = (w < min_slot) ? min_slot : w;
    }

    /* Pass 3: lay out screen-space edges in display order (skip hidden columns). */
    float x = table->work_min_x;
    table->left_most_enabled = -1;
    table->right_most_enabled = -1;
    for (int order = 0; order < n; order++) {
        int i = table->display_order[order];
        if (i < 0 || i >= n)
            continue;
        eli_table_column *c = &table->columns[i];
        if (!c->is_enabled) {
            c->min_x = c->max_x = x;
            c->work_min_x = c->work_max_x = x;
            c->clip_min_x = c->clip_max_x = x;
            c->width_given = 0.0f;
            c->is_visible = false;
            continue;
        }
        c->min_x = x;
        c->max_x = x + c->width_given;
        c->work_min_x = c->min_x + inner_pad;
        c->work_max_x = c->max_x - inner_pad;
        c->clip_min_x = c->min_x;
        c->clip_max_x = c->max_x;
        c->is_visible = true;
        if (table->left_most_enabled < 0)
            table->left_most_enabled = i;
        table->right_most_enabled = i;
        x = c->max_x;
    }
    table->columns_right_x = x;
}

/* ---------------------------------------------------------------------------
 * Column queries
 * ------------------------------------------------------------------------- */

/** @return the number of columns declared for the current table (0 if none). */
static inline int eli_table_get_column_count(void)
{
    eli_table *table = eli_table_get_current();
    return table ? table->columns_count : 0;
}

/** @return the index of the column currently being filled (-1 before the first cell). */
static inline int eli_table_get_column_index(void)
{
    eli_table *table = eli_table_get_current();
    return table ? table->current_column : -1;
}

/** @return the index of the current row (-1 before the first row). */
static inline int eli_table_get_row_index(void)
{
    eli_table *table = eli_table_get_current();
    return table ? table->current_row : -1;
}

/**
 * @param column_n  Column index, or -1 for the current column.
 * @return          The column's header label, or "" if unnamed / out of range.
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline const char *eli_table_get_column_name(int column_n)
{
    eli_table *table = eli_table_get_current();
    if (table == NULL)
        return "";
    if (column_n < 0)
        column_n = table->current_column;
    if (column_n < 0 || column_n >= table->columns_count)
        return "";
    return table->columns[column_n].name;
}

/**
 * @param column_n  Column index, or -1 for the current column.
 * @return          The column's effective flags OR'd with the live output bits
 *                  (IS_ENABLED / IS_VISIBLE / IS_SORTED / IS_HOVERED).
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline eli_table_column_flags eli_table_get_column_flags(int column_n)
{
    eli_table *table = eli_table_get_current();
    if (table == NULL)
        return ELI_TABLE_COLUMN_NONE;
    if (column_n < 0)
        column_n = table->current_column;
    if (column_n < 0 || column_n >= table->columns_count)
        return ELI_TABLE_COLUMN_NONE;

    const eli_table_column *c = &table->columns[column_n];
    eli_table_column_flags f = c->flags & ~ELI_TABLE_COLUMN_STATUS_MASK_;
    if (c->is_enabled)
        f |= ELI_TABLE_COLUMN_IS_ENABLED;
    if (c->is_visible)
        f |= ELI_TABLE_COLUMN_IS_VISIBLE;
    if (c->sort_order >= 0)
        f |= ELI_TABLE_COLUMN_IS_SORTED;
    if (table->hovered_column_body == column_n)
        f |= ELI_TABLE_COLUMN_IS_HOVERED;
    return f;
}

/**
 * Show or hide a column. Takes effect on the next frame's layout (columns are
 * locked once the current frame's first row begins).
 *
 * @param column_n  Column index.
 * @param enabled   true to show, false to hide.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_table_set_column_enabled(int column_n, bool enabled)
{
    eli_table *table = eli_table_get_current();
    if (table == NULL || column_n < 0 || column_n >= table->columns_count)
        return;
    table->columns[column_n].is_enabled = enabled;
}

/** @return the column index under the mouse this frame, or -1 if none. */
static inline int eli_table_get_hovered_column(void)
{
    eli_table *table = eli_table_get_current();
    return table ? table->hovered_column_body : -1;
}

#endif /* ELI_WIDGETS_ELI_TABLE_COLUMNS_H */
