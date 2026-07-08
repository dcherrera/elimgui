# Menus (Phase 16)

Menu bars, dropdown menus, submenus, and menu items. A menu dropdown **is a popup**
(Phase 17): this module reuses the popup stack for open/close state and only overrides
where the dropdown appears (below a bar button, or to the right of a submenu row).

Header: `include/eli/widgets/eli_menu.h` — usable via
`#include <eli/widgets/eli_menu.h>`.

## Overview

- **Window menu bar** — a horizontal strip at the top of a window's work area. The
  window must be created with the `ELI_WINDOW_MENU_BAR` flag. Because the core window
  system does not reserve a menu-bar strip, this module lays the bar out at the top of
  the work area and drops the body cursor below it at `eli_end_menu_bar`.
- **Main menu bar** — a borderless, full-display-width bar pinned to the top of the
  screen, hosted in its own window.
- **Menus** — `eli_begin_menu` is a bar button (in a menu bar) or a labelled row with a
  right-pointing arrow (inside another menu). Clicking a bar menu, or clicking/hovering a
  submenu row, opens its dropdown.
- **Menu items** — selectable rows with an optional right-aligned shortcut label and an
  optional check mark. Choosing an item closes the whole open menu chain.

## Public API

### Menu bars

```c
bool eli_begin_menu_bar(void);
void eli_end_menu_bar(void);
bool eli_begin_main_menu_bar(void);
void eli_end_main_menu_bar(void);
```

- `eli_begin_menu_bar` — begin the current window's menu bar. Returns `true` only when
  the window carries `ELI_WINDOW_MENU_BAR`; call `eli_end_menu_bar` only when it returned
  `true`.
- `eli_begin_main_menu_bar` — begin the top-of-screen main menu bar (hosted in a
  full-width `##MainMenuBar` window). Balance with `eli_end_main_menu_bar` only when it
  returned `true`.

### Menus

```c
bool eli_begin_menu(const char *label, bool enabled);
void eli_end_menu(void);
```

- `eli_begin_menu` — a menu entry. In a menu bar it is a horizontal button whose click
  opens a dropdown below it; inside a menu it is a row with a submenu arrow whose hover or
  click opens a dropdown to the right. Returns `true` when the dropdown is open and its
  items should be emitted — call `eli_end_menu` only then.
- `enabled` — pass `false` to render the entry dimmed and non-interactive.

### Menu items

```c
bool eli_menu_item(const char *label, const char *shortcut, bool selected, bool enabled);
bool eli_menu_item_bool(const char *label, const char *shortcut, bool *p_selected, bool enabled);
```

- `eli_menu_item` — a selectable row. `shortcut` (may be `NULL`) is drawn right-aligned
  and dimmed as display-only text (no key handling). `selected` draws a check mark in the
  left icon column. Returns `true` on the frame it is activated (mouse released inside),
  and closes the whole open menu chain.
- `eli_menu_item_bool` — the same, but bound to a `bool`: choosing it toggles
  `*p_selected` and the check mark reflects the current value. `p_selected` must be
  non-`NULL`.

## Usage

```c
if (eli_begin("Editor", NULL, ELI_WINDOW_MENU_BAR)) {
    if (eli_begin_menu_bar()) {
        if (eli_begin_menu("File", true)) {
            if (eli_menu_item("New",  "Ctrl+N", false, true)) { /* ... */ }
            if (eli_menu_item("Open", "Ctrl+O", false, true)) { /* ... */ }
            eli_menu_item_bool("Word Wrap", NULL, &g_word_wrap, true);
            if (eli_begin_menu("Recent", true)) {          /* submenu */
                eli_menu_item("a.txt", NULL, false, true);
                eli_end_menu();
            }
            eli_end_menu();
        }
        if (eli_begin_menu("Edit", true)) {
            eli_menu_item("Undo", "Ctrl+Z", false, g_can_undo);
            eli_end_menu();
        }
        eli_end_menu_bar();
    }
    /* window body flows below the menu bar */
    eli_end();
}

/* Top-of-screen bar */
if (eli_begin_main_menu_bar()) {
    if (eli_begin_menu("App", true)) {
        eli_menu_item("Quit", "Ctrl+Q", false, true);
        eli_end_menu();
    }
    eli_end_main_menu_bar();
}
```

## Behavior notes

- **Dropdowns are popups.** They open/close through the Phase 17 popup stack, so the
  standard popup dismissal applies: clicking outside the open menu, or pressing Escape,
  closes it (handled in `eli_popup_new_frame`). Clicking a bar button again toggles its
  dropdown closed.
- **Choosing an item closes the whole chain.** `eli_menu_item` closes every open menu
  down to the outermost one, matching Dear ImGui, so a pick in a submenu also dismisses
  its parents.
- **Shortcut text is display-only.** It is measured (so the dropdown is wide enough) and
  drawn right-aligned in the row; it does not register or handle a keyboard shortcut.
- **Appear lag.** A freshly opened dropdown is auto-sized from its contents, so its final
  size is available on the frame after it first appears (the same one-frame settle as any
  auto-resize popup).

## Gotchas

- Only call `eli_end_menu_bar` / `eli_end_menu` / `eli_end_main_menu_bar` when the
  matching `begin` returned `true`.
- `eli_begin_menu_bar` returns `false` on a window without `ELI_WINDOW_MENU_BAR`.
- Menu bars do not nest; one bar is active at a time.
- The main menu bar draws at the top of the display but does not yet subtract its height
  from the space available to other windows.
