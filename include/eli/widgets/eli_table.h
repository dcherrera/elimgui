/**
 * @file eli_table.h
 * @brief The table widget lifecycle and features: begin/end, row and cell
 *        advancement, headers with click-to-sort, sort-spec queries, row/cell
 *        background colors, and border/background rendering.
 *
 * A table lays its columns out across the host window's work area (see
 * eli_table_columns.h for the width solver), then advances the window's layout
 * cursor cell-by-cell so ordinary widgets draw inside each cell. Backgrounds and
 * borders render behind cell content via a two-channel split of the window draw
 * list. Table state (widths, sort order, hidden columns) persists across frames in
 * a module-private pool; call eli_table_shutdown() before destroying the context.
 *
 * Include this header to pull in the whole table module.
 *
 * @status Phase 19 table lifecycle in use.
 * @issues None
 * @todo Independent table scrolling (scroll child + frozen rows/cols) and angled
 *       header rendering are simplified; see the notes on the relevant functions.
 */
#ifndef ELI_WIDGETS_ELI_TABLE_H
#define ELI_WIDGETS_ELI_TABLE_H

#include "eli_table_types.h"
#include "eli_table_columns.h"
#include "eli_widget_behavior.h"

#include "../draw/eli_draw.h"
#include "../layout/eli_layout.h"

#include "../core/eli_platform.h"
#include "../core/eli_context.h"

/* ---------------------------------------------------------------------------
 * Cell / row internals
 * ------------------------------------------------------------------------- */

/** @return the y coordinate at which cell clip rects are cut off (window clip bottom). */
static inline float eli_table__clip_bottom(const eli_table *table)
{
    const eli_window *win = table->inner_window;
    return win->clip_rect.y + win->clip_rect.h;
}

/** Move the window layout cursor into column `column_n` and push its clip rect. */
static inline void eli_table__begin_cell(eli_table *table, int column_n)
{
    eli_window *win = table->inner_window;
    eli_table_column *c = &table->columns[column_n];
    table->current_column = column_n;

    float cell_top = table->row_pos_y1 + table->cell_padding_y;
    table->cell_top_y = cell_top;

    win->cursor_pos = eli_make_vec2(c->work_min_x, cell_top);
    win->cursor_pos_prev_line = win->cursor_pos;
    win->curr_line_size = eli_make_vec2(0.0f, 0.0f);
    win->curr_line_text_baseline_offset = 0.0f;
    win->is_same_line = false;
    win->is_set_pos = false;
    win->indent = c->work_min_x - win->pos.x;

    eli_draw_list *dl = &win->draw_list;
    float clip_bottom = eli_table__clip_bottom(table);
    eli_draw_list_push_clip_rect(dl, eli_make_vec2(c->clip_min_x, table->row_pos_y1),
                                 eli_make_vec2(c->clip_max_x, clip_bottom), false);
    table->is_cell_open = true;
}

/** Close the current cell: record its content bottom and pop its clip rect. */
static inline void eli_table__end_cell(eli_table *table)
{
    if (!table->is_cell_open)
        return;
    eli_window *win = table->inner_window;
    const eli_context *ctx = eli_get_current_context();
    float spacing_y = ctx->style.item_spacing.y;

    float content_bottom = (win->cursor_pos.y > table->cell_top_y)
                               ? win->cursor_pos.y - spacing_y
                               : table->cell_top_y;
    if (content_bottom > table->row_pos_y2)
        table->row_pos_y2 = content_bottom;

    int col = table->current_column;
    if (col >= 0 && col < table->columns_count && win->cursor_max_pos.x >
                                                      table->columns[col].content_max_x)
        table->columns[col].content_max_x = win->cursor_max_pos.x;

    eli_draw_list_pop_clip_rect(&win->draw_list);
    table->is_cell_open = false;
}

/** Draw one row's backgrounds (striping, header, overrides, cell) into the bg channel. */
static inline void eli_table__draw_row_bg(eli_table *table, float row_top, float row_bottom)
{
    eli_draw_list *dl = &table->inner_window->draw_list;
    eli_vec2 rmin = eli_make_vec2(table->work_min_x, row_top);
    eli_vec2 rmax = eli_make_vec2(table->columns_right_x, row_bottom);

    if (table->row_flags & ELI_TABLE_ROW_HEADERS) {
        eli_col32 hdr = eli_get_color_u32(ELI_COL_TABLE_HEADER_BG, 1.0f);
        eli_draw_list_add_rect_filled(dl, rmin, rmax, hdr, 0.0f, ELI_DRAW_ROUND_CORNERS_NONE);
    } else if (table->flags & ELI_TABLE_ROW_BG) {
        eli_col idx = (table->current_row & 1) ? ELI_COL_TABLE_ROW_BG_ALT : ELI_COL_TABLE_ROW_BG;
        eli_col32 base = eli_get_color_u32(idx, 1.0f);
        if (base & ELI_COL32_A_MASK)
            eli_draw_list_add_rect_filled(dl, rmin, rmax, base, 0.0f, ELI_DRAW_ROUND_CORNERS_NONE);
    }

    for (int k = 0; k < 2; k++)
        if (table->row_bg_color[k] & ELI_COL32_A_MASK)
            eli_draw_list_add_rect_filled(dl, rmin, rmax, table->row_bg_color[k], 0.0f,
                                          ELI_DRAW_ROUND_CORNERS_NONE);

    for (int i = 0; i < table->columns_count; i++) {
        if (!(table->cell_bg_color[i] & ELI_COL32_A_MASK))
            continue;
        const eli_table_column *c = &table->columns[i];
        if (!c->is_enabled)
            continue;
        eli_draw_list_add_rect_filled(dl, eli_make_vec2(c->min_x, row_top),
                                      eli_make_vec2(c->max_x, row_bottom),
                                      table->cell_bg_color[i], 0.0f, ELI_DRAW_ROUND_CORNERS_NONE);
    }
}

/** Finalize the current row: compute its height, draw backgrounds/borders, advance. */
static inline void eli_table__end_row(eli_table *table)
{
    if (!table->is_inside_row)
        return;
    eli_table__end_cell(table);

    float row_top = table->row_pos_y1;
    float row_bottom = table->row_pos_y2 + table->cell_padding_y;
    float min_bottom = row_top + table->row_min_height;
    if (row_bottom < min_bottom)
        row_bottom = min_bottom;

    eli_draw_list *dl = &table->inner_window->draw_list;
    int prev_channel = dl->channels_current;
    if (table->channels_split)
        eli_draw_list_channels_set_current(dl, 0);

    eli_table__draw_row_bg(table, row_top, row_bottom);

    if (table->flags & ELI_TABLE_BORDERS_INNER_H) {
        eli_col32 light = eli_get_color_u32(ELI_COL_TABLE_BORDER_LIGHT, 1.0f);
        eli_draw_list_add_line(dl, eli_make_vec2(table->work_min_x, row_bottom),
                               eli_make_vec2(table->columns_right_x, row_bottom), light,
                               ELI_TABLE_BORDER_SIZE);
    }

    if (table->channels_split)
        eli_draw_list_channels_set_current(dl, prev_channel);

    table->row_pos_y1 = row_bottom;
    table->row_pos_y2 = row_bottom;
    table->is_inside_row = false;
    if (row_bottom - table->outer_rect.y > table->outer_rect.h)
        table->outer_rect.h = row_bottom - table->outer_rect.y;
}

/* ---------------------------------------------------------------------------
 * Row + column advancement
 * ------------------------------------------------------------------------- */

/**
 * Start a new table row.
 *
 * @param row_flags       ELI_TABLE_ROW_NONE or ELI_TABLE_ROW_HEADERS.
 * @param min_row_height  Minimum row height in pixels (0 = fit content).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_table_next_row_ex(eli_table_row_flags row_flags, float min_row_height)
{
    eli_table *table = eli_table_get_current();
    if (table == NULL)
        return;
    eli_table__update_layout(table);
    if (table->is_inside_row)
        eli_table__end_row(table);

    table->current_row++;
    table->current_column = -1;
    table->row_flags = row_flags;
    table->row_min_height = (min_row_height > 0.0f) ? min_row_height : 0.0f;
    table->row_pos_y1 = table->row_pos_y2;
    table->is_inside_row = true;
    table->row_bg_color[0] = 0u;
    table->row_bg_color[1] = 0u;
    for (int i = 0; i < table->columns_count; i++)
        table->cell_bg_color[i] = 0u;
}

/** Start a new default table row (fit content, no special flags). */
static inline void eli_table_next_row(void)
{
    eli_table_next_row_ex(ELI_TABLE_ROW_NONE, 0.0f);
}

/**
 * Move to a specific column in the current row, opening a new row first if none is
 * active.
 *
 * @param column_n  Target column index.
 * @return          true if the column is visible this frame.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_table_set_column_index(int column_n)
{
    eli_table *table = eli_table_get_current();
    if (table == NULL)
        return false;
    if (!table->is_inside_row)
        eli_table_next_row();
    if (column_n < 0 || column_n >= table->columns_count)
        return false;
    eli_table__end_cell(table);
    eli_table__begin_cell(table, column_n);
    return table->columns[column_n].is_visible;
}

/**
 * Advance to the next column, wrapping to a fresh row after the last column.
 *
 * @return  true if the newly entered column is visible this frame.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_table_next_column(void)
{
    eli_table *table = eli_table_get_current();
    if (table == NULL)
        return false;
    if (!table->is_inside_row || table->current_column + 1 >= table->columns_count) {
        eli_table_next_row();
        return eli_table_set_column_index(0);
    }
    return eli_table_set_column_index(table->current_column + 1);
}

/* ---------------------------------------------------------------------------
 * Sorting
 * ------------------------------------------------------------------------- */

/** Apply a header click to a sortable column: set it as the sole sort key / toggle. */
static inline void eli_table__on_header_click(eli_table *table, int column_n)
{
    eli_table_column *c = &table->columns[column_n];
    bool no_asc = (c->flags & ELI_TABLE_COLUMN_NO_SORT_ASCENDING) != 0;
    bool no_desc = (c->flags & ELI_TABLE_COLUMN_NO_SORT_DESCENDING) != 0;

    eli_sort_direction next;
    if (c->sort_order < 0) {
        next = (c->flags & ELI_TABLE_COLUMN_PREFER_SORT_DESCENDING) ? ELI_SORT_DESCENDING
                                                                    : ELI_SORT_ASCENDING;
        if (next == ELI_SORT_ASCENDING && no_asc)
            next = ELI_SORT_DESCENDING;
        else if (next == ELI_SORT_DESCENDING && no_desc)
            next = ELI_SORT_ASCENDING;
    } else if (c->sort_direction == ELI_SORT_ASCENDING) {
        next = no_desc ? ELI_SORT_ASCENDING : ELI_SORT_DESCENDING;
    } else {
        next = (table->flags & ELI_TABLE_SORT_TRISTATE) ? ELI_SORT_NONE
             : no_asc                                   ? ELI_SORT_DESCENDING
                                                        : ELI_SORT_ASCENDING;
    }

    for (int i = 0; i < table->columns_count; i++) {
        table->columns[i].sort_order = -1;
        table->columns[i].sort_direction = ELI_SORT_NONE;
    }
    if (next != ELI_SORT_NONE) {
        c->sort_order = 0;
        c->sort_direction = next;
    }
    table->sort_dirty = true;
}

/** Rebuild the caller-facing sort-spec array from the columns' sort_order/direction. */
static inline void eli_table__build_sort_specs(eli_table *table)
{
    int count = 0;
    for (int i = 0; i < table->columns_count; i++)
        if (table->columns[i].sort_order >= 0)
            count++;

    if (count == 0) {
        table->sort_specs.specs_count = 0;
        return;
    }

    eli_table_column_sort_specs *arr = (eli_table_column_sort_specs *)realloc(
        table->sort_specs.specs, (size_t)count * sizeof(*arr));
    if (arr == NULL) {
        table->sort_specs.specs_count = 0;
        return;
    }
    table->sort_specs.specs = arr;

    int w = 0;
    for (int order = 0; order < count; order++) {
        for (int i = 0; i < table->columns_count; i++) {
            eli_table_column *c = &table->columns[i];
            if (c->sort_order != order)
                continue;
            arr[w].column_user_id = c->user_id;
            arr[w].column_index = i;
            arr[w].sort_order = order;
            arr[w].sort_direction = c->sort_direction;
            w++;
            break;
        }
    }
    table->sort_specs.specs_count = w;
}

/**
 * @return  The current table's sort specifications, or NULL if the table is not
 *          sortable. `specs_dirty` is true on the first query after the sort
 *          changed; re-sort your data while it is true.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline eli_table_sort_specs *eli_table_get_sort_specs(void)
{
    eli_table *table = eli_table_get_current();
    if (table == NULL || !(table->flags & ELI_TABLE_SORTABLE))
        return NULL;
    if (table->sort_dirty) {
        eli_table__build_sort_specs(table);
        table->sort_specs.specs_dirty = true;
        table->sort_dirty = false;
    } else {
        table->sort_specs.specs_dirty = false;
    }
    return &table->sort_specs;
}

/* ---------------------------------------------------------------------------
 * Backgrounds
 * ------------------------------------------------------------------------- */

/**
 * Override a background color for the current row or a cell.
 *
 * @param target    Which background layer to set (ROW_BG0 / ROW_BG1 / CELL_BG).
 * @param color     Packed color (alpha 0 clears the override).
 * @param column_n  Column index for CELL_BG (-1 = current column); ignored otherwise.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_table_set_bg_color(eli_table_bg_target target, eli_col32 color,
                                          int column_n)
{
    eli_table *table = eli_table_get_current();
    if (table == NULL)
        return;
    switch (target) {
    case ELI_TABLE_BG_TARGET_ROW_BG0:
        table->row_bg_color[0] = color;
        break;
    case ELI_TABLE_BG_TARGET_ROW_BG1:
        table->row_bg_color[1] = color;
        break;
    case ELI_TABLE_BG_TARGET_CELL_BG:
        if (column_n < 0)
            column_n = table->current_column;
        if (column_n >= 0 && column_n < table->columns_count)
            table->cell_bg_color[column_n] = color;
        break;
    default:
        break;
    }
}

/* ---------------------------------------------------------------------------
 * Headers
 * ------------------------------------------------------------------------- */

/**
 * Submit a header cell for the current column: draws the label, an optional sort
 * arrow, and handles click-to-sort. Call once per column inside a header row.
 *
 * @param label  Header text (falls back to the column's setup name when NULL/empty).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_table_header(const char *label)
{
    eli_table *table = eli_table_get_current();
    if (table == NULL)
        return;
    int column_n = table->current_column;
    if (column_n < 0 || column_n >= table->columns_count)
        return;
    eli_table_column *c = &table->columns[column_n];
    const char *lbl = (label != NULL && label[0] != '\0') ? label : c->name;

    float pad_y = table->cell_padding_y;
    float label_h = eli_get_font_size();
    float y0 = table->row_pos_y1;
    float y1 = y0 + label_h + pad_y * 2.0f;
    eli_rect bb = eli_make_rect(c->min_x, y0, c->max_x - c->min_x, y1 - y0);

    float content_bottom = y0 + pad_y + label_h;
    if (content_bottom > table->row_pos_y2)
        table->row_pos_y2 = content_bottom;

    eli_push_id_int(column_n);
    eli_id id = eli_get_id((lbl != NULL && lbl[0] != '\0') ? lbl : "##hdr");
    eli_pop_id();
    eli_item_add(id, bb, 0);

    bool can_sort = table->is_sortable && (c->flags & ELI_TABLE_COLUMN_NO_SORT) == 0;
    bool hovered = false, held = false;
    bool pressed = eli_button_behavior(bb, id, &hovered, &held, ELI_BUTTON_NONE);

    eli_draw_list *dl = &table->inner_window->draw_list;
    if (held || hovered) {
        eli_col32 hc = eli_get_color_u32(held ? ELI_COL_HEADER_ACTIVE : ELI_COL_HEADER_HOVERED, 1.0f);
        eli_draw_list_add_rect_filled(dl, eli_make_vec2(bb.x, bb.y),
                                      eli_make_vec2(bb.x + bb.w, bb.y + bb.h), hc, 0.0f,
                                      ELI_DRAW_ROUND_CORNERS_NONE);
    }

    if (pressed && can_sort)
        eli_table__on_header_click(table, column_n);

    if ((c->flags & ELI_TABLE_COLUMN_NO_HEADER_LABEL) == 0) {
        eli_vec2 tp = eli_make_vec2(c->work_min_x, y0 + pad_y);
        eli_render_text(tp, eli_get_color_u32(ELI_COL_TEXT, 1.0f), lbl, NULL, true);
    }

    if (c->sort_order >= 0 && can_sort) {
        eli_dir dir = (c->sort_direction == ELI_SORT_ASCENDING) ? ELI_DIR_UP : ELI_DIR_DOWN;
        float ax = c->work_max_x - label_h;
        eli_render_arrow(dl, eli_make_vec2(ax, y0 + pad_y), eli_get_color_u32(ELI_COL_TEXT, 1.0f),
                         dir, 0.7f);
    }
}

/**
 * Submit a full header row: begins a header row and emits a header cell for each
 * visible column using its setup name. Convenience over manual per-column calls.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_table_headers_row(void)
{
    eli_table *table = eli_table_get_current();
    if (table == NULL)
        return;
    float header_h = eli_get_font_size() + table->cell_padding_y * 2.0f;
    eli_table_next_row_ex(ELI_TABLE_ROW_HEADERS, header_h);
    int n = table->columns_count;
    for (int i = 0; i < n; i++) {
        if (!eli_table_set_column_index(i))
            continue;
        eli_table_header(table->columns[i].name);
    }
}

/**
 * Submit an angled header row. Angled glyph rendering is a refinement not yet
 * implemented; this currently renders a standard horizontal header row so tables
 * declaring ANGLED_HEADER columns still show their labels.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_table_angled_headers_row(void)
{
    eli_table_headers_row();
}

/* ---------------------------------------------------------------------------
 * Borders + hover
 * ------------------------------------------------------------------------- */

/** Draw the table's outer frame and inner vertical column borders (bg channel). */
static inline void eli_table__draw_borders(eli_table *table)
{
    eli_draw_list *dl = &table->inner_window->draw_list;
    float x0 = table->work_min_x;
    float x1 = table->columns_right_x;
    float y0 = table->outer_rect.y;
    float y1 = table->row_pos_y2;
    eli_col32 strong = eli_get_color_u32(ELI_COL_TABLE_BORDER_STRONG, 1.0f);

    if (table->flags & ELI_TABLE_BORDERS_OUTER_H) {
        eli_draw_list_add_line(dl, eli_make_vec2(x0, y0), eli_make_vec2(x1, y0), strong,
                               ELI_TABLE_BORDER_SIZE);
        eli_draw_list_add_line(dl, eli_make_vec2(x0, y1), eli_make_vec2(x1, y1), strong,
                               ELI_TABLE_BORDER_SIZE);
    }
    if (table->flags & ELI_TABLE_BORDERS_OUTER_V) {
        eli_draw_list_add_line(dl, eli_make_vec2(x0, y0), eli_make_vec2(x0, y1), strong,
                               ELI_TABLE_BORDER_SIZE);
        eli_draw_list_add_line(dl, eli_make_vec2(x1, y0), eli_make_vec2(x1, y1), strong,
                               ELI_TABLE_BORDER_SIZE);
    }
    if (table->flags & ELI_TABLE_BORDERS_INNER_V) {
        eli_col32 light = eli_get_color_u32(ELI_COL_TABLE_BORDER_LIGHT, 1.0f);
        for (int order = 0; order < table->columns_count; order++) {
            int i = table->display_order[order];
            if (i < 0 || i >= table->columns_count)
                continue;
            const eli_table_column *c = &table->columns[i];
            if (!c->is_enabled || i == table->left_most_enabled)
                continue;
            eli_draw_list_add_line(dl, eli_make_vec2(c->min_x, y0), eli_make_vec2(c->min_x, y1),
                                   light, ELI_TABLE_BORDER_SIZE);
        }
    }
}

/** Recompute which column is under the mouse (stored for next-frame queries). */
static inline void eli_table__update_hovered(eli_table *table)
{
    const eli_io *io = eli_get_io();
    table->hovered_column_body = -1;
    if (io == NULL)
        return;
    float mx = io->mouse_pos.x;
    float my = io->mouse_pos.y;
    if (my < table->outer_rect.y || my > table->outer_rect.y + table->outer_rect.h)
        return;
    for (int i = 0; i < table->columns_count; i++) {
        const eli_table_column *c = &table->columns[i];
        if (!c->is_enabled)
            continue;
        if (mx >= c->min_x && mx < c->max_x) {
            table->hovered_column_body = i;
            return;
        }
    }
}

/* ---------------------------------------------------------------------------
 * Lifecycle
 * ------------------------------------------------------------------------- */

/**
 * Begin a table with an explicit outer size.
 *
 * @param str_id         Table identifier (hashed for the table id + cell id scope).
 * @param columns_count  Number of columns (>= 1, clamped to ELI_TABLE_MAX_COLUMNS).
 * @param flags          eli_table_flags.
 * @param outer_size     Requested outer size; x <= 0 fills / trims the work area,
 *                       height grows to fit content.
 * @param inner_width    Reserved for horizontal-scroll inner width (unused today).
 * @return               true if the table body should be emitted (call eli_end_table
 *                       only when this returns true).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_begin_table_ex(const char *str_id, int columns_count, eli_table_flags flags,
                                      eli_vec2 outer_size, float inner_width)
{
    (void)inner_width;
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || str_id == NULL)
        return false;
    eli_window *outer = ctx->current_window;
    if (outer == NULL || outer->skip_items)
        return false;
    if (columns_count < 1 || g_eli_table_stack_size >= ELI_TABLE_STACK_MAX)
        return false;
    if (columns_count > ELI_TABLE_MAX_COLUMNS)
        columns_count = ELI_TABLE_MAX_COLUMNS;

    eli_id id = eli_get_id(str_id);
    eli_table *table = eli_table__find_or_create(id);
    if (table == NULL)
        return false;

    const eli_style *style = &ctx->style;
    eli_push_id(str_id);

    table->flags = flags;
    table->columns_count = columns_count;
    table->current_row = -1;
    table->current_column = -1;
    table->setup_column_next = 0;
    table->is_layout_locked = false;
    table->is_inside_row = false;
    table->is_cell_open = false;
    table->row_flags = ELI_TABLE_ROW_NONE;
    table->row_min_height = 0.0f;
    table->outer_window = outer;
    table->inner_window = outer;
    table->cell_padding_x = style->cell_padding.x;
    table->cell_padding_y = style->cell_padding.y;
    table->resized_column = -1;

    for (int i = 0; i < columns_count; i++) {
        table->columns[i].is_setup = false;
        table->columns[i].content_max_x = 0.0f;
        table->display_order[i] = i;
    }

    eli_vec2 avail = eli_get_content_region_avail();
    eli_vec2 pos = outer->cursor_pos;
    float width = (outer_size.x > 0.0f)   ? outer_size.x
                  : (outer_size.x < 0.0f) ? eli_max_f(4.0f, avail.x + outer_size.x)
                                          : avail.x;
    table->outer_rect = eli_make_rect(pos.x, pos.y, width, 0.0f);
    /* last_outer_height is preserved from the previous frame for geometry queries. */

    bool pad_outer_x = (flags & ELI_TABLE_PAD_OUTER_X)      ? true
                       : (flags & ELI_TABLE_NO_PAD_OUTER_X) ? false
                                                            : (flags & ELI_TABLE_BORDERS_OUTER_V) != 0;
    float outer_pad = pad_outer_x ? table->cell_padding_x : 0.0f;
    table->work_min_x = pos.x + outer_pad;
    table->work_max_x = pos.x + width - outer_pad;
    table->work_min_y = pos.y;
    table->row_pos_y1 = pos.y;
    table->row_pos_y2 = pos.y;

    table->host_cursor_backup = outer->cursor_pos;
    table->host_cursor_max_backup = outer->cursor_max_pos;
    table->host_indent_backup = outer->indent;

    eli_draw_list *dl = &outer->draw_list;
    eli_draw_list_channels_split(dl, 2);
    eli_draw_list_channels_set_current(dl, 1);
    table->channels_split = true;

    g_eli_table_stack[g_eli_table_stack_size++] = table;
    return true;
}

/**
 * Begin a table filling the available work-area width.
 *
 * @param str_id         Table identifier.
 * @param columns_count  Number of columns (>= 1).
 * @param flags          eli_table_flags.
 * @return               true if the table body should be emitted.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_begin_table(const char *str_id, int columns_count, eli_table_flags flags)
{
    return eli_begin_table_ex(str_id, columns_count, flags, eli_make_vec2(0.0f, 0.0f), 0.0f);
}

/**
 * End the current table: close any open row, draw borders, merge draw channels,
 * restore the host window's layout cursor, and advance it past the table.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_end_table(void)
{
    eli_table *table = eli_table_get_current();
    if (table == NULL)
        return;

    eli_table__update_layout(table);
    if (table->is_inside_row)
        eli_table__end_row(table);

    eli_draw_list *dl = &table->inner_window->draw_list;
    if (table->channels_split)
        eli_draw_list_channels_set_current(dl, 0);
    eli_table__draw_borders(table);
    if (table->channels_split) {
        eli_draw_list_channels_merge(dl);
        table->channels_split = false;
    }

    float height = table->row_pos_y2 - table->outer_rect.y;
    if (height < 0.0f)
        height = 0.0f;
    table->outer_rect.h = height;
    table->last_outer_height = height;

    eli_table__update_hovered(table);

    eli_window *win = table->inner_window;
    win->indent = table->host_indent_backup;
    win->cursor_pos = table->host_cursor_backup;
    win->cursor_pos_prev_line = table->host_cursor_backup;
    win->curr_line_size = eli_make_vec2(0.0f, 0.0f);
    win->cursor_max_pos = table->host_cursor_max_backup;
    win->is_same_line = false;

    eli_item_size(eli_make_vec2(table->outer_rect.w, table->outer_rect.h), -1.0f);
    eli_item_add(table->id, table->outer_rect, 0);

    eli_pop_id();
    g_eli_table_stack_size--;
}

#endif /* ELI_WIDGETS_ELI_TABLE_H */
