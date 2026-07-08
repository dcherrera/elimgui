# Tables

Phase 19 adds elimgui's table widget: a multi-column layout system with resizable,
reorderable, hideable, and sortable columns, row/cell backgrounds, and borders.
Tables lay their columns out across the host window's work area and advance the
window layout cursor cell-by-cell so ordinary widgets (`eli_text`, `eli_button`, …)
draw inside each cell.

Header: `#include <eli/widgets/eli_table.h>` (pulls in the whole table module —
types, the column layout solver, and the lifecycle/feature API).

## Contents

- [Frame flow and lifetime](#frame-flow-and-lifetime)
- [Lifecycle](#lifecycle)
- [Column setup](#column-setup)
- [Rows and cells](#rows-and-cells)
- [Headers and sorting](#headers-and-sorting)
- [Backgrounds and borders](#backgrounds-and-borders)
- [Queries](#queries)
- [Table flags](#table-flags)
- [Column flags](#column-flags)
- [Compiling example](#compiling-example)
- [Limitations](#limitations)

## Frame flow and lifetime

Table state (column widths, sort order, hidden columns) persists across frames in a
module-private pool keyed by the table id. The pool is heap-allocated on demand.
Because the table module is self-contained (the core context knows nothing about
tables), you release it explicitly:

```c
eli_table_shutdown();   /* free the table pool; call once before eli_destroy_context */
```

Two optional per-frame hooks mirror the other subsystems. `eli_table_new_frame()`
resets the current-table stack defensively at the top of a frame;
`eli_table_end_frame()` is a symmetric no-op today. Neither is required for correct
operation — a well-formed frame ends every table it begins.

```c
eli_new_frame();
eli_input_update_begin_frame();
eli_window_new_frame();
eli_table_new_frame();           /* optional */
/* ... eli_begin(...) / tables / eli_end() ... */
eli_window_render();
eli_render();
eli_input_update_end_frame();
```

## Lifecycle

```c
bool eli_begin_table(const char *str_id, int columns_count, eli_table_flags flags);
bool eli_begin_table_ex(const char *str_id, int columns_count, eli_table_flags flags,
                        eli_vec2 outer_size, float inner_width);
void eli_end_table(void);
```

`eli_begin_table` starts a table filling the available work-area width; the height
grows to fit the rows. `eli_begin_table_ex` takes an explicit `outer_size` (an x
component `<= 0` fills/trims the work area) and an `inner_width` (reserved for
horizontal scrolling). Both return `true` when the table body should be emitted —
**call `eli_end_table` only when the begin returned `true`**, exactly like windows.

`columns_count` must be `>= 1` and is clamped to `ELI_TABLE_MAX_COLUMNS` (64).
`eli_end_table` closes any open row, draws borders, restores the host window's
layout cursor, and advances it past the table.

## Column setup

Declare columns immediately after `eli_begin_table`, before the first row:

```c
void eli_table_setup_column(const char *label, eli_table_column_flags flags,
                            float init_width_or_weight, eli_id user_id);
void eli_table_setup_scroll_freeze(int cols, int rows);
```

`init_width_or_weight` is the initial fixed content width for fixed columns, or the
stretch weight for stretch columns (`<= 0` uses defaults). `user_id` is a stable id
surfaced back in the sort specs. Seed values (initial width/weight, `DEFAULT_HIDE`,
`DEFAULT_SORT`) apply only on the column's first appearance — user resizing/hiding/
sorting persists across frames. Columns you never set up still exist and default to
enabled with the table's default width policy.

Column widths are resolved once, on the first row of the frame:

- **Fixed** columns take `init_width` (or an auto width derived from the header
  label) plus cell padding.
- **Stretch** columns split the remaining width in proportion to their weights.
- The default policy for columns with no explicit `WIDTH_*` flag comes from the
  table's `SIZING_*` flag (or `STRETCH_SAME` by default, `FIXED_FIT` under
  `SCROLL_X`).

## Rows and cells

```c
void eli_table_next_row(void);
void eli_table_next_row_ex(eli_table_row_flags row_flags, float min_row_height);
bool eli_table_next_column(void);           /* advance; wraps to a new row after the last */
bool eli_table_set_column_index(int column_n);
```

Two idioms, both valid:

```c
/* Explicit rows */
eli_table_next_row();
eli_table_set_column_index(0); eli_text("a");
eli_table_set_column_index(1); eli_text("b");

/* Column-driven (auto-wraps rows) */
eli_table_next_column(); eli_text("a");
eli_table_next_column(); eli_text("b");
```

`eli_table_next_column` / `eli_table_set_column_index` return `true` when the target
column is visible this frame (hidden columns return `false`). Entering a cell moves
the window cursor to the column's content origin and clips subsequent drawing to the
column; the row grows to the tallest cell.

## Headers and sorting

```c
void eli_table_header(const char *label);        /* one header cell for the current column */
void eli_table_headers_row(void);                /* full header row using setup names */
void eli_table_angled_headers_row(void);         /* horizontal fallback (see Limitations) */
eli_table_sort_specs *eli_table_get_sort_specs(void);
```

`eli_table_header` draws the label, a hover highlight, and — for a sortable column —
a sort-direction arrow, and handles click-to-sort. Clicking a sortable header makes
that column the sort key (ascending first, honoring `PREFER_SORT_*` and `NO_SORT_*`);
clicking again toggles ascending/descending (or cycles to unsorted with
`ELI_TABLE_SORT_TRISTATE`).

`eli_table_get_sort_specs` returns the current sort description (or `NULL` if the
table is not `ELI_TABLE_SORTABLE`). Re-sort your data while `specs_dirty` is `true`:

```c
if (eli_begin_table("t", 3, ELI_TABLE_SORTABLE)) {
    eli_table_setup_column("Name", 0, 0, 0);
    eli_table_setup_column("Size", 0, 0, 0);
    eli_table_setup_column("Date", 0, 0, 0);
    eli_table_headers_row();

    eli_table_sort_specs *sort = eli_table_get_sort_specs();
    if (sort && sort->specs_dirty && sort->specs_count > 0) {
        int col = sort->specs[0].column_index;
        bool ascending = (sort->specs[0].sort_direction == ELI_SORT_ASCENDING);
        /* re-sort your rows by `col` / `ascending` here */
    }
    /* ... rows ... */
    eli_end_table();
}
```

Each `eli_table_column_sort_specs` carries `column_index`, `column_user_id`,
`sort_order` (0 = primary), and `sort_direction`.

## Backgrounds and borders

```c
void eli_table_set_bg_color(eli_table_bg_target target, eli_col32 color, int column_n);
```

`target` is `ELI_TABLE_BG_TARGET_ROW_BG0`, `..._ROW_BG1` (two stacked row layers), or
`..._CELL_BG` (per-column; `column_n = -1` targets the current column). Alpha 0
clears an override. With `ELI_TABLE_ROW_BG` the table also draws alternating row
striping (`ELI_COL_TABLE_ROW_BG` / `_ALT`). Backgrounds render behind cell content
(the table splits the window draw list into a background and a content channel and
merges them at `eli_end_table`). Borders are controlled by the `ELI_TABLE_BORDERS_*`
flags and use `ELI_COL_TABLE_BORDER_STRONG` (outer) and `_LIGHT` (inner).

## Queries

All operate on the current table (between begin/end):

```c
int          eli_table_get_column_count(void);
int          eli_table_get_column_index(void);          /* current column, -1 before first cell */
int          eli_table_get_row_index(void);             /* current row, -1 before first row */
const char  *eli_table_get_column_name(int column_n);   /* -1 = current column */
int          eli_table_get_column_flags(int column_n);  /* effective flags + output bits */
void         eli_table_set_column_enabled(int column_n, bool enabled);
int          eli_table_get_hovered_column(void);        /* column under the mouse, or -1 */
```

`eli_table_get_column_flags` OR's the live output bits `ELI_TABLE_COLUMN_IS_ENABLED`,
`_IS_VISIBLE`, `_IS_SORTED`, and `_IS_HOVERED` onto the column's flags.
`eli_table_set_column_enabled` takes effect on the next frame (columns are locked
once the current frame's first row begins).

## Table flags

`eli_table_flags` — features: `RESIZABLE`, `REORDERABLE`, `HIDEABLE`, `SORTABLE`;
decorations: `ROW_BG`, `BORDERS_*` (`INNER_H/V`, `OUTER_H/V`, and the `H`/`V`/`INNER`/
`OUTER`/`BORDERS` combinations); sizing: `SIZING_FIXED_FIT`, `SIZING_FIXED_SAME`,
`SIZING_STRETCH_PROP`, `SIZING_STRETCH_SAME`; scrolling: `SCROLL_X`, `SCROLL_Y`;
sorting: `SORT_MULTI`, `SORT_TRISTATE`; padding: `PAD_OUTER_X`, `NO_PAD_OUTER_X`,
`NO_PAD_INNER_X`, plus `NO_CLIP`, `HIGHLIGHT_HOVERED_COLUMN`, and others.

## Column flags

`eli_table_column_flags` — `WIDTH_STRETCH` / `WIDTH_FIXED`, `DEFAULT_HIDE`,
`DEFAULT_SORT`, `NO_RESIZE`, `NO_REORDER`, `NO_HIDE`, `NO_CLIP`, `NO_SORT`,
`NO_SORT_ASCENDING` / `NO_SORT_DESCENDING`, `PREFER_SORT_ASCENDING` /
`PREFER_SORT_DESCENDING`, `NO_HEADER_LABEL`, `NO_HEADER_WIDTH`, `ANGLED_HEADER`, and
`DISABLED`. Output bits (`IS_ENABLED`, `IS_VISIBLE`, `IS_SORTED`, `IS_HOVERED`) are
returned by `eli_table_get_column_flags`.

## Compiling example

```c
#include <eli/widgets/eli_table.h>
#include <eli/widgets/eli_widgets.h>

void draw_table(void)
{
    if (eli_begin_table("files", 3,
                        ELI_TABLE_ROW_BG | ELI_TABLE_BORDERS | ELI_TABLE_SORTABLE |
                        ELI_TABLE_RESIZABLE)) {
        eli_table_setup_column("Name", ELI_TABLE_COLUMN_WIDTH_STRETCH, 0.0f, 0);
        eli_table_setup_column("Size", ELI_TABLE_COLUMN_WIDTH_FIXED, 80.0f, 0);
        eli_table_setup_column("Date", ELI_TABLE_COLUMN_WIDTH_FIXED, 120.0f, 0);
        eli_table_headers_row();

        for (int row = 0; row < 100; row++) {
            eli_table_next_row();
            eli_table_next_column(); eli_text("file.txt");
            eli_table_next_column(); eli_text("4 KB");
            eli_table_next_column(); eli_text("2026-01-01");
        }
        eli_end_table();
    }
}

/* At shutdown: */
/* eli_table_shutdown(); eli_destroy_context(ctx); */
```

## Limitations

- **Independent table scrolling** (a dedicated scroll child with `SCROLL_X`/
  `SCROLL_Y` and frozen rows/columns) is simplified: a tall table scrolls with its
  host window, and `eli_table_setup_scroll_freeze` records the freeze counts but
  frozen-region pinning is not yet rendered.
- **Multi-column sort** (`ELI_TABLE_SORT_MULTI`) resolves to a single primary sort
  key; `eli_table_get_sort_specs` always reports one sorted column.
- **Angled headers** (`eli_table_angled_headers_row` / `ELI_TABLE_COLUMN_ANGLED_HEADER`)
  render as a standard horizontal header row.
- **Column drag-reorder UI** is not implemented; the display order is identity
  (the `REORDERABLE` flag and `display_order` plumbing are in place for it).
- `eli_table_get_hovered_column` reflects the previous frame's geometry (the full
  table height is known only at `eli_end_table`).
```
