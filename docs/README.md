# elimgui Documentation

elimgui is a pure C11, header-only reimplementation of [Dear ImGui](https://github.com/ocornut/imgui)
targeting WebAssembly. This directory is the documentation index; each phase of the
build adds one document describing the APIs it introduces.

## Getting Started

Include the umbrella header and everything is available:

```c
#include <eli/elimgui.h>
```

Build and serve it in the browser with `./build.sh serve`, or build and run the
native unit tests with `./build.sh test` (see the top-level README for details).

## Frame Lifecycle

A UI frame is a sequence of per-subsystem hooks. `include/eli/eli_frame.h` (the umbrella's
final include) composes them into two convenience entry points so you never hand-order the
hooks:

```c
#include <eli/elimgui.h>

eli_frame_begin();            /* roll every subsystem's begin-of-frame state */
    if (eli_begin("Main", NULL, 0)) {
        eli_button("Click");
        /* ...tables, tabs, popups, etc... */
    }
    eli_end();
eli_frame_end();              /* flush end-of-frame state + assemble draw data */

eli_draw_data *dd = eli_get_draw_data();   /* non-NULL, populated after frame_end */
```

- **`eli_frame_begin()`** runs, in order:
  `eli_new_frame` -> `eli_input_update_begin_frame` -> `eli_window_new_frame` ->
  `eli_popup_new_frame` -> `eli_drag_drop_new_frame` -> `eli_table_new_frame` ->
  `eli_tab_new_frame`. On first call it also registers a widget-shutdown hook on the
  context so `eli_destroy_context` releases the table/tab pools and any pending
  drag-drop payload.
- **`eli_frame_end()`** runs, in order:
  `eli_popup_end_frame` -> `eli_drag_drop_end_frame` -> `eli_window_render` ->
  `eli_render` -> `eli_input_update_end_frame`. Afterwards `eli_get_draw_data()`
  returns this frame's assembled, valid draw data.

Each `eli_begin`/`eli_begin_child` seeds the id stack with the window id (mirroring Dear
ImGui's `PushOverrideID`) and `eli_end`/`eli_end_child` pops it, so identical widget labels
in different windows derive distinct ids. The hooks may still be called individually if you
need finer control; `eli_frame_begin`/`eli_frame_end` are the recommended composition.

## Table of Contents

| Phase | Document | Contents |
|-------|----------|----------|
| 1 | [Core Types](core-types.md) | Vectors, rects, colors, ids, enums, IO, style, context, and the frame lifecycle |
| 2 | [Draw System](draw-system.md) | Draw lists, primitives, path API, clip/texture stacks, primitive reservation, and channels |
| 3 | [Font System](font-system.md) | Font atlas (stb_truetype), embedded default font, glyph ranges, text rendering, font stack |
| 4 | [Input System](input-system.md) | Mouse, keyboard, text input, shortcuts, clipboard, and backend event integration |
| 5 | [ID System](id-system.md) | ID hashing (CRC32, `##`/`###`), ID stack, active/hot id, key/value storage |
| 6 | [Style System](style-system.md) | Themes, style color/var stacks, item flags, color conversion utilities |
| 7 | [Windows](windows.md) | Window lifecycle, interaction (move/resize/collapse), child windows, scrolling, state queries |
| 8 | [Layout System](layout-system.md) | Cursor, item layout (`item_size`/`item_add`), same-line/groups, content region, item width, sizing |
| 9 | [Basic Widgets](basic-widgets.md) | Text family, buttons, checkbox/radio, progress bar, links, `button_behavior` |
| 10 | [Item Status](item-status.md) | Per-item queries: hovered/active/clicked/edited/activated, item rect accessors |
| 11 | [Sliders & Drags](sliders-drags.md) | Slider/drag widgets (float/int/scalar/angle/vertical/range), shared value behavior |
| 12 | [Input Widgets](input-widgets.md) | Text input (edit/select/clipboard), multiline, hint, numeric input |
| 13 | [Color Widgets](color-widgets.md) | Color edit/picker/button, hue bar, saturation-value square, alpha |
| 14 | [Combo & Selectable](combo-selectable.md) | Selectable rows, combo dropdown, list box |
| 15 | [Trees & Collapsing](trees-collapsing.md) | Tree nodes, tree push/pop, collapsing headers |
| 16 | [Menus](menus.md) | Menu bar, main menu bar, menus, menu items, submenus, shortcuts |
| 17 | [Popups & Modals](popups-modals.md) | Popup stack, context menus, modal dialogs + backdrop |
| 18 | [Tooltips](tooltips.md) | Tooltip windows, item tooltips, hover delay |
| 19 | [Tables](tables.md) | Columns (fixed/stretch/hide), rows/cells, headers, sorting, row backgrounds |
| 20 | [Tab Bars](tab-bars.md) | Tab bar/items, selection, reorder, scroll, close button |
| 21 | [Drag & Drop](drag-drop.md) | Payload, source/target, accept, drag preview |
| 22 | [Images](images.md) | Image + image button widgets, textured draw-list primitives |
| 23 | [Plotting](plotting.md) | Line and histogram plots (array + getter callback) |
| 24 | [Value Display](value-display.md) | `Value()` helpers for bool/int/uint/float |
| 25 | [Disabling & Clipping](disabling-clipping.md) | begin/end disabled, clip-rect push/pop, focus requests |
| 26 | [List Clipper](list-clipper.md) | Efficient rendering of very large lists |
| 27 | [Utilities](utilities.md) | Rect visibility, time/frame count, viewport, background/foreground draw lists |
| 28 | [Settings & Logging](settings-logging.md) | INI window settings load/save, logging to tty/file/clipboard |
| 29 | [Memory](memory.md) | User-overridable allocator functions |
| 30 | [Demo & Debug](demo-debug.md) | `show_demo_window`, metrics, debug log, style editor, about, version |
| 31 | [Web Integration](web-integration.md) | WASM loader, Canvas2D renderer, JS event wiring, browser demo |
| 32 | [Testing](testing.md) | Test framework, hosted-libc seam, per-subsystem coverage map |
| 33 | [Optimization](optimization.md) | Draw-gen profiling, buffer reuse, batching, per-frame allocations, wasm size |
| 34 | [Docking](docking.md) | Dock-node tree, drag-to-dock, edge splitting, tabbed docking, dock space |

_Additional phase documents are added here as each phase lands._
