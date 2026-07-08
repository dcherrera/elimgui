# Tab Bars (Phase 20)

Tab bars render a horizontal strip of tabs above their contents. Clicking a tab
selects it; `eli_begin_tab_item` returns `true` for exactly the selected tab, so
the caller draws that tab's contents beneath the strip. Tabs persist across frames
in a module-private pool keyed by id, which lets the bar remember the selection,
lay tabs out, scroll when they overflow, and reorder them by drag.

Header: `#include <eli/widgets/eli_tab.h>` (pulls in the widget/layout/id/input/
style/font/draw stack it builds on). Drive it inside a window scope opened by
`eli_begin` / `eli_end`.

## Quick start

```c
if (eli_begin_tab_bar("MyTabs", ELI_TAB_BAR_REORDERABLE)) {
    if (eli_begin_tab_item("One", NULL, 0)) {
        eli_text("contents of tab one");
        eli_end_tab_item();
    }

    static bool doc_open = true;
    if (doc_open && eli_begin_tab_item("Doc", &doc_open, 0)) {
        eli_text("this tab has a close button");
        eli_end_tab_item();
    }

    if (eli_tab_item_button("+", 0)) {
        /* add a new tab / document */
    }
    eli_end_tab_bar();
}
```

Only call `eli_end_tab_item` when the matching `eli_begin_tab_item` returned
`true` (the standard immediate-mode `if (...) { ...; end; }` pattern). Always call
`eli_end_tab_bar` if `eli_begin_tab_bar` returned `true`.

## Lifetime

The tab-bar pool is heap-allocated through the libc seam and persists for the life
of the program. Release it once at teardown:

```c
eli_tab_shutdown();   /* frees the pool and every bar's tab array */
```

`eli_tab_new_frame()` resets the current-bar stack; call it at the start of a
frame if you want a defensive recovery from an unbalanced begin/end (a well-formed
frame does not need it).

## API

### `bool eli_begin_tab_bar(const char *str_id, eli_tab_bar_flags flags)`
Open a tab bar at the current cursor position, spanning the remaining content
width. Returns `true` when the bar is open (still balance it with
`eli_end_tab_bar`). No-op outside a window / when the window is clipped.

### `void eli_end_tab_bar(void)`
Close the current tab bar and advance the window cursor below the tab contents.

### `bool eli_begin_tab_item(const char *label, bool *p_open, eli_tab_item_flags flags)`
Submit a tab. Returns `true` when the tab is selected — draw its contents and call
`eli_end_tab_item`. If `p_open` is non-NULL the tab shows a close button; clicking
it (or middle-clicking the tab) sets `*p_open = false`. When `*p_open` is already
`false` the tab is skipped entirely. Pushes the tab id on the id stack while open
(unless `ELI_TAB_ITEM_NO_PUSH_ID`).

### `void eli_end_tab_item(void)`
Close a tab item. Call only when `eli_begin_tab_item` returned `true`.

### `bool eli_tab_item_button(const char *label, eli_tab_item_flags flags)`
Submit a tab that behaves like a button: it never becomes the selected tab and
returns `true` on the frame it is pressed. Useful for a "+" add-tab affordance.

### `void eli_set_tab_item_closed(const char *label)`
Flag a tab (by label) for removal on the next layout, avoiding a one-frame glitch.
Call it between `eli_begin_tab_bar`/`eli_end_tab_bar` on the frame you decide to
close the tab, and stop submitting that tab. Tabs closed via the close button are
flagged automatically.

### `void eli_tab_shutdown(void)`
Free the whole tab-bar pool. Call once at teardown.

### `void eli_tab_new_frame(void)`
Reset the current-bar stack (defensive).

## Flags

### `eli_tab_bar_flags`

| Flag | Effect |
|------|--------|
| `ELI_TAB_BAR_REORDERABLE` | Drag tabs to reorder them. |
| `ELI_TAB_BAR_AUTO_SELECT_NEW_TABS` | Select tabs as they first appear. |
| `ELI_TAB_BAR_NO_CLOSE_WITH_MIDDLE_MOUSE` | Disable middle-click close. |
| `ELI_TAB_BAR_DRAW_SELECTED_OVERLINE` | Draw an accent line over the selected tab. |
| `ELI_TAB_BAR_FITTING_POLICY_SHRINK` | Shrink tab widths to fit (default). |
| `ELI_TAB_BAR_FITTING_POLICY_SCROLL` | Scroll the strip when tabs overflow. |

### `eli_tab_item_flags`

| Flag | Effect |
|------|--------|
| `ELI_TAB_ITEM_UNSAVED_DOCUMENT` | Show an unsaved-dot marker. |
| `ELI_TAB_ITEM_SET_SELECTED` | Programmatically select the tab this frame. |
| `ELI_TAB_ITEM_NO_CLOSE_WITH_MIDDLE_MOUSE` | Disable middle-click close for this tab. |
| `ELI_TAB_ITEM_NO_PUSH_ID` | Don't push/pop the tab id on begin/end. |
| `ELI_TAB_ITEM_NO_REORDER` | Pin the tab against reordering. |

## Behavior notes

- **Default selection.** With no prior selection the first submitted tab is
  selected. On the first appearing frame the first tab's contents are shown to
  avoid a blank frame; from the second frame the bar's persistent
  `selected_tab_id` drives selection.
- **Selection latency.** Clicking a tab queues the selection; the newly selected
  tab returns `true` on the **following** frame (immediate-mode two-phase layout).
- **Overflow.** With the shrink policy, tabs are shrunk proportionally to fit.
  With the scroll policy, the strip scrolls so the selected tab stays visible and
  tabs are clipped to the bar rect.
- **Colors / style.** Uses `ELI_COL_TAB`, `ELI_COL_TAB_HOVERED`,
  `ELI_COL_TAB_SELECTED`, `ELI_COL_TAB_SELECTED_OVERLINE`, and the
  `tab_rounding`, `tab_border_size`, `tab_bar_border_size`,
  `tab_bar_overline_size` style values.

## Deferred sub-features

- **Tab-list popup button** and **on-screen scroll arrow buttons**
  (`ELI_TAB_BAR_NO_TAB_LIST_SCROLLING_BUTTONS` is accepted but a no-op) are not
  implemented; scrolling is handled by clamping the strip to keep the selected tab
  visible.
- **Leading / trailing tab sections** — all tabs live in a single central section.
- **Scroll animation and hover tooltips** are not implemented (scroll snaps to
  target; `ELI_TAB_BAR_NO_TOOLTIP` / `ELI_TAB_ITEM_NO_TOOLTIP` are no-ops).
- **Reordering** is best-effort using cumulative drag direction rather than
  per-frame mouse delta.
