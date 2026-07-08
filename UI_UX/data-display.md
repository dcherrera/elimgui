# Data Display — Tables, Lists, Trees, Tabs

> Alignment, density, and scannability decide whether a table is a tool or a wall of noise.

## Overview

Pro-tool UIs are mostly data surfaces: tables, lists, trees, tab strips. The recurring principles: align by data type, separate rows with the lightest sufficient means, keep headers visible, make sorting/selection obvious, and virtualize anything big.

---

## 1. Tables

### Density & row heights

| Density | Row height | Cell padding |
|---------|-----------|--------------|
| Compact | **24–32px** | 4–8px |
| Default | **40–48px** (Material: 52dp rows / 56dp header) | 8–16px |
| Relaxed | **56px+** | 16px |

Offer a density toggle for data-heavy views ([Pencil & Paper tables guide](https://www.pencilandpaper.io/articles/ux-pattern-analysis-enterprise-data-tables), [Material data tables](https://m2.material.io/components/data-tables/web)).

### Alignment (non-negotiable)

- **Text → left.** **Numbers → right, with tabular figures** (so magnitudes align).
- Dates, codes, phone-like identifiers → left (they're read, not compared numerically).
- **Header alignment matches its column.**
- Column gaps ≥ 16px padding per side; don't center-align data columns.

### Row separation — lightest that works

1. Whitespace alone (short, sparse tables)
2. **1px hairline row dividers** (default for dense tables — [Carbon](https://carbondesignsystem.com/components/data-table/style/) requires dividers if striping is off)
3. Zebra striping (very wide tables where the eye must track across; keep the stripe subtle, ~3–5% delta)

Plus **row hover highlight** (tracking aid) and a distinct **selected-row fill** (primary @ ~25%) — hover ≠ selected.

### Sorting

- Click header to sort; arrow (▲/▼) on the sorted column; click again to reverse; unsorted columns show a faint affordance on hover ([NN/g data tables](https://www.nngroup.com/articles/data-tables/)).
- Ship a meaningful default sort (recency or name).

### Headers, freezing, structure

- **Sticky header** whenever the table scrolls; freeze key identifier columns when scrolling horizontally.
- Column ops for pro tables: resize (drag boundary, ≥8px hit), reorder (drag header), hide/show (context menu on header) — all with menu equivalents.
- **Empty cells**: an em dash "—", never blank (blank is ambiguous: missing? loading? zero?).
- Numbers: consistent decimals per column; units in the header, not every cell.

### Big data

- **Pagination** (25–50 rows default) for deliberate browsing; **virtualization** (render only the visible window + buffer) for continuous scroll past ~1,000 rows ([LogRocket](https://blog.logrocket.com/ux-design/data-table-design-best-practices/)). Infinite scroll without virtualization is the worst of both.

### Selection & inline editing

- Checkbox column for explicit multi-select (header checkbox = all, with indeterminate state); or click + **Shift-click range + Ctrl/Cmd-click toggle** for desktop-native feel.
- Inline editing for simple, low-risk fields: visible affordance on hover, Enter commits, Esc reverts, Tab moves to next cell; complex records edit in a panel/dialog instead ([UXDW inline editing](https://uxdworld.com/inline-editing-in-tables-design/)).

## 2. Lists

- Heights: single-line **40–48px** (32 dense), two-line 64–72px.
- Anatomy: leading icon/avatar → primary text → secondary text (secondary color) → trailing meta/actions.
- Reserve consistent leading width so text aligns whether or not an icon is present.
- Hover, selected, and focus states as in tables.

## 3. Trees ([W3C tree pattern](https://www.w3.org/WAI/ARIA/apg/patterns/treeview/))

- **Indent 16–24px per level**; chevron (▸/▾) as the expand affordance, rotating ~90° when open.
- Hit rules: chevron click toggles; **row click selects; double-click opens/toggles**; the whole row is hover/hit territory.
- Keyboard: ↑/↓ move; **→ expands (then enters first child); ← collapses (then jumps to parent)**; Home/End extremes; type-ahead jumps by name. This exact arrow-key model is what makes a tree feel native.
- Connecting indent guide lines: optional; faint (border color) vertical guides help in deep hierarchies (IDE convention).
- Lazy-load children on first expand with an inline spinner row; keep focus vs selection visually distinct.
- Drag-and-drop in trees: see drag-and-drop.md (into-folder highlight vs between-siblings insertion line).

## 4. Tabs (content tabs & document tabs)

- **Fixed-purpose tabs** (settings sections): ≤6, generous widths, selected = fill or 2px underline + weight change.
- **Document tabs** (editor-style): fit-content width with **min ~100px / max ~200–240px**, middle-truncate long titles with tooltip, close × on hover/active (with a ≥16px hit), dirty-dot indicator replacing × when unsaved.
- Overflow: horizontal scroll (with mouse-wheel support) plus a **dropdown list-all-tabs button** — hidden tabs must stay reachable.
- Keyboard: Ctrl+Tab / Ctrl+PgUp/PgDn cycle; middle-click closes (editor convention).
- Never mix navigation tabs and document tabs in one strip ([NN/g tabs](https://www.nngroup.com/articles/tabs-used-right/)).

## 5. Virtualization

Render cost must track *visible* items, not total items:
- Fixed row height ⇒ visible range = `scroll_offset / row_height … + viewport/row_height + buffer`.
- Threshold: anything that can exceed a few hundred rows should be virtualized; thousands, always.
- Keep scrollbar geometry based on total count so the thumb is honest.

## 6. Data-Viz Basics

- **Bars compare categories; lines show trends over time; pies only for simple part-of-whole with ≤5 slices** ([chart choice](https://chartgen.ai/resources/blog/bar-line-pie-chart-decision-framework)).
- **Bar axes start at zero** (truncated bars lie); line charts may zoom the range.
- **Direct-label** series where possible instead of legends; label values on hover.
- Tufte: maximize data-ink — drop heavy gridlines, borders, 3D, backgrounds ([Tufte principles](https://thedoublethink.com/tuftes-principles-for-visualizing-quantitative-information/)). Faint gridlines (border color), no chart junk.
- Use colorblind-safe series palettes; ≤6–8 series before grouping.

---

## Do's & Don'ts

**Do**
- Right-align numbers with tabular figures; left-align text; match headers.
- Sticky headers; hover highlight; distinct selection fill.
- Em dash for empty cells; units in headers.
- Chevrons + full arrow-key model in trees.
- Min/max tab widths, truncation with tooltips, overflow menu.
- Virtualize big lists; paginate deliberate browsing.

**Don't**
- Center-align data columns or mix decimal precision within a column.
- Combine zebra stripes with heavy borders and hover fills (pick the lightest sufficient set).
- Make the chevron the only clickable part of a tree row.
- Let tabs shrink to unreadable slivers or vanish without an overflow menu.
- Render 10,000 rows of widgets.
- Start bar charts above zero.

## Common Pitfalls

1. **Ragged number columns** — proportional digits, left alignment, inconsistent decimals.
2. **Header scrolls away** — context lost instantly in tall tables.
3. **Hover = selected confusion** — identical fills for transient and persistent states.
4. **Tree rows with dead zones** — only the label is clickable.
5. **Tab strip collapse** — 30 open tabs, each 20px wide, all unidentifiable.
6. **Blank cells** — indistinguishable from bugs.

---

## Applying It in elimgui (immediate-mode / desktop GUI)

- **`eli_tables.h` API** (keep the proven ImGui shape): `eli_begin_table(id, cols, flags)` / `eli_table_setup_column(name, flags, width)` / `eli_table_headers_row()` / `eli_table_next_row()` / `eli_table_next_column()`. Column flags: `SORTABLE`, `RESIZABLE`, `FIXED/STRETCH`, `ALIGN_RIGHT`, `FROZEN`.
- **Table flags map to this doc**: `ELI_TABLE_ROW_DIVIDERS` (default on), `ELI_TABLE_STRIPED` (subtle 4% delta), `ELI_TABLE_HOVER_HIGHLIGHT` (default on), `ELI_TABLE_STICKY_HEADER` (header drawn after/atop the clipped body scroll), `ELI_TABLE_BORDERS_OUTER` (default off — modern look).
- **Sorting built-in**: header click cycles asc/desc, draws the arrow, and exposes `eli_table_get_sort_spec()` — the app sorts its own data (library stays data-agnostic).
- **Clipper for virtualization**: `eli_list_clipper(count, row_height)` returning the visible [begin,end) range — use it in tables, lists, and flat trees; document it as mandatory beyond ~500 rows.
- **Numeric cells**: pair `ALIGN_RIGHT` with the tabular-digit font variant (typography.md) — make `eli_table_cell_number(fmt, value)` do both automatically.
- **Tree API**: `eli_tree_node(label, flags)` with `ELI_TREE_OPEN_ON_ARROW | OPEN_ON_DOUBLE_CLICK | SELECTED | LEAF | SPAN_FULL_WIDTH(default)`; indent from `style.indent_spacing` (20px); rotating chevron; full arrow-key navigation wired into the nav layer; optional `ELI_TREE_DRAW_LINES` for indent guides.
- **Tab bar widget**: `eli_begin_tab_bar(id)` / `eli_tab_item(label, &open, flags)` with min/max widths (100/220px), middle-truncation + tooltip, hover-revealed close ×, dirty indicator flag, wheel scrolling, and a built-in "▾ list all" overflow popup. Reorderable via flag (feeds the docking system later).
- **Selection helpers**: `eli_selectable(label, selected, flags)` supporting `SPAN_ALL_COLUMNS` in tables plus Shift/Ctrl modifier reporting, so range/toggle multi-select is a few app-side lines.
- **`eli_plot_lines` / `eli_plot_histogram`**: keep minimal but honest — zero-based bars, faint gridlines, hover value readout, direct min/max labels.
