# Testing (Phase 32)

This document describes the elimgui unit-test framework, explains the host-libc
`ELI_TEST_HOSTED` seam that lets the library run as native binaries, covers the
`./build.sh test` invocation and filter option, and maps every Phase 32 coverage area
to the test suites that satisfy it.

---

## Test Framework (`tests/eli_test.h`)

The framework is a single header at `tests/eli_test.h`. It uses only the host system
libc (`<stdio.h>`, `<stdlib.h>`, `<string.h>`, `<math.h>`), has no dependencies on
JAClibc or the browser, and is intentionally minimal.

### Defining tests

```c
#include "eli_test.h"

ELI_TEST(my_test_name) {
    /* assertions here */
}

ELI_TEST_MAIN()   /* must appear once, at the end of the file */
```

`ELI_TEST(name)` expands to a static function definition preceded by an
`__attribute__((constructor))` trampoline that registers `name` in a global table
before `main()` runs. `ELI_TEST_MAIN()` expands to a `main()` function that runs every
registered test and returns 1 if any assertion failed (so the runner can gate on the
exit code).

### Assertion macros

| Macro | Passes when |
|-------|-------------|
| `ELI_ASSERT_TRUE(x)` | `x` is truthy |
| `ELI_ASSERT_FALSE(x)` | `x` is falsy |
| `ELI_ASSERT_EQ(a, b)` | `a == b` |
| `ELI_ASSERT_NE(a, b)` | `a != b` |
| `ELI_ASSERT_GT(a, b)` | `a > b` |
| `ELI_ASSERT_LT(a, b)` | `a < b` |
| `ELI_ASSERT_GE(a, b)` | `a >= b` |
| `ELI_ASSERT_LE(a, b)` | `a <= b` |
| `ELI_ASSERT_NULL(p)` | `p == NULL` |
| `ELI_ASSERT_NOT_NULL(p)` | `p != NULL` |
| `ELI_ASSERT_STR_EQ(a, b)` | `strcmp(a, b) == 0` |
| `ELI_ASSERT_FLT_NEAR(a, b, tol)` | `fabs(a - b) <= tol` |

On failure each macro prints `FAIL: ASSERT_XXX(...)  (file:line)` to stderr, marks the
current test as failed, and continues to the next assertion.

### Failure reporting

`ELI_FAIL(msg)` can be called directly to record a failure with a custom message.
After all tests run the framework prints a summary line and exits with status 1 if any
test failed.

---

## The `ELI_TEST_HOSTED` Seam

The library never includes `<jaclibc.h>` directly. Instead, every header routes its
libc dependency through `include/eli/core/eli_platform.h`:

```c
/* include/eli/core/eli_platform.h */
#ifdef ELI_TEST_HOSTED
#  include <stddef.h>
#  include <stdint.h>
#  include <stdbool.h>
#  include <string.h>
#  include <math.h>
#  include <stdlib.h>
#else
#  include <jaclibc.h>
#endif
```

When the test runner compiles with `-DELI_TEST_HOSTED`, the seam substitutes the host
system libc so the library builds and runs as a normal native binary — no WASM runtime,
no Docker, no browser. The WASM-only `JS_EXPORT`/`JS_CODE` blocks are guarded with
`#ifdef ELI_JSIO` and compile out automatically.

---

## Running Tests

### Full suite

```bash
./build.sh test
```

Compiles every `tests/unit/*.c` with:

```
cc -std=c11 -Wall -Wextra -Werror -g -DELI_TEST_HOSTED \
   -Iinclude -Ivendor -Itests
```

Warnings are errors (`-Werror`). Each test binary is run and its exit code is checked.
The runner prints a per-suite summary, then a totals line:

```
Suites: 62/62 passed
ALL GREEN
```

### Filtered run

```bash
./build.sh test <substring>
```

Only suite files whose filename contains `<substring>` are compiled and run. Examples:

```bash
./build.sh test draw        # runs test_draw_list, test_draw_prim, test_draw_channels
./build.sh test p19         # runs test_p19_tables
./build.sh test p32         # runs test_p32_integration
./build.sh test window      # runs test_window_lifecycle, test_window_interaction, test_window_scroll
```

---

## File Naming Conventions

| Pattern | Meaning |
|---------|---------|
| `test_<subsystem>.c` | Focused suite for one subsystem (e.g. `test_draw_list.c`) |
| `test_<subsystem>_<aspect>.c` | Aspect slice of a large subsystem (e.g. `test_layout_item.c`) |
| `test_integration_<topic>.c` | Cross-subsystem integration suite |
| `test_p<NN>_<widget>.c` | Phase-tagged suite for a widget or feature added in phase NN |
| `test_p32_*.c` | Phase 32 testing-phase suites |

Phase-tagged names (`test_p<NN>_`) are used for widget/feature suites that were added
during the corresponding build phase. Core subsystem suites (draw, layout, id, input,
window) use the plain `test_<subsystem>` pattern instead.

---

## Coverage Map

The table below maps each Phase 32 coverage area to its `tests/unit/` suites, with the
current suite/test counts (62 suites, 337 tests total as of Phase 32).

### Core types and math

| Suite | Tests | What is covered |
|-------|-------|-----------------|
| `test_core_types` | 8 | `eli_vec2` arithmetic, scalar helpers (`eli_min_f`, `eli_max_f`, `eli_clamp_f`), rect dimensions/corners/contains, `ELI_COL32` packing, color `u32↔vec4` round-trip, out-of-range saturation |
| `test_core_context` | 8 | Context create/destroy, IO zeroing, style defaults, frame scope flag, within-frame assertions |
| `test_core_enums` | 7 | Enum value correctness (key codes, mouse buttons, color indices, style-var indices, flag bits) |
| `test_platform` | 3 | `eli_platform.h` seam: `ELI_TEST_HOSTED` pulls host libc, sizes match expectations |

### ID hashing

| Suite | Tests | What is covered |
|-------|-------|-----------------|
| `test_id_hash` | 6 | CRC32 snapshot values, determinism, seed dependence, `##` (hidden label) and `###` (stable ID) semantics, empty-string edge case |
| `test_id_stack` | 6 | Push/pop string/ptr/int IDs, nesting depth, hash chaining across stack levels |
| `test_id_active` | 4 | `active_id` and `hot_id` set/clear, active ID survives same-frame resets |
| `test_id_storage` | 7 | `eli_storage` get/set int/float/ptr, missing-key defaults, overwrite |

### Draw list generation

| Suite | Tests | What is covered |
|-------|-------|-----------------|
| `test_draw_list` | 7 | Init opens single command, clip-rect stack push/pop/intersect, clip change splits command, texture stack, prim reserve + write, buffer growth across 100 primitives, `clone_output` deep copies |
| `test_draw_prim` | 8 | Exact vertex/index counts and positions: filled rect, line, open/closed polyline, convex-fill, filled circle, stroked circle, multi-color rect; transparent-color no-op |
| `test_draw_channels` | 2 | Channel split/merge preserves draw order; set-current directs geometry to the correct channel |

### Layout calculations

| Suite | Tests | What is covered |
|-------|-------|-----------------|
| `test_layout_helpers` | 8 | `eli_same_line` (follow + explicit offset), `eli_spacing`, `eli_dummy`, `eli_indent`/`eli_unindent`, `eli_separator`, `eli_begin_group`/`eli_end_group` bounding-box sizing and cursor restoration |
| `test_layout_item` | 6 | `eli_item_size` cursor advancement, item rect recording, `eli_item_add` hit-region, `eli_align_text_to_frame_padding` |
| `test_layout_sizing` | 2 | `eli_get_text_line_height`, `eli_get_frame_height` match style values |
| `test_layout_stack` | 5 | Item-width stack, text-wrap-pos stack, `eli_calc_item_width` |

### Input state

| Suite | Tests | What is covered |
|-------|-------|-----------------|
| `test_input_mouse` | 8 | Click/release, double-click, double-click distance reset, drag threshold + delta, hover-rect boundaries, position validity, cursor get/set, mouse wheel accumulate + clear |
| `test_input_keyboard` | 7 | Key down/press/release, held-key repeat, modifier flags (ctrl/shift/alt), `eli_get_key_name`, next-frame capture keyboard |
| `test_input_shortcut` | 3 | `eli_shortcut` with modifier chord, predefined shortcuts (`ELI_SHORTCUT_COPY`), shortcut not firing without modifier |
| `test_input_text` | 5 | `eli_io_add_input_character`, UTF-8 multi-character add, queue overflow, queue cleared each frame |

### Window management

| Suite | Tests | What is covered |
|-------|-------|-----------------|
| `test_window_lifecycle` | 7 | Create/retrieve same instance, `set_next_window_pos`/size, pivot center, work-region insets, collapse (body skip + size clamp), `ELI_WINDOW_NO_TITLEBAR`, draw data gathers active windows |
| `test_window_interaction` | 4 | Hover/focus z-order, title-bar drag moves window, child-window create/end, child ID stability |
| `test_window_scroll` | 5 | Scroll range from content, set + clamp, `set_scroll_from_pos_y`, mouse-wheel scrolls hovered window, horizontal scrollbar flag |

### Widget behavior

| Suite | Tests | What is covered |
|-------|-------|-----------------|
| `test_widget_buttons` | 8 | `eli_button` click (press + release inside), release-outside no-press, hover sets item-hovered, `eli_checkbox` toggle + edited flag, `eli_radio_button_int`, `eli_progress_bar` clamping, `eli_text` cursor advance, `eli_checkbox_flags_int` bit manipulation |
| `test_widget_status` | 4 | `eli_is_item_deactivated_after_edit`, item-id/rect/visibility accessors, non-interactive item status |
| `test_p11_slider` | 3 | Slider value clamping, step quantization, keyboard adjustment |
| `test_p11_drag` | 2 | Drag float threshold, drag release |
| `test_p11_scalar` | 8 | Scalar behavior shared by drag/slider: format string, data-type promotion, range reversal |
| `test_p12_input_text` | 6 | Text input edit/select/delete, cursor movement, callback invocation |
| `test_p12_input_number` | 5 | Numeric input float/int parsing, step buttons |
| `test_p13_color` | 5 | Color edit4, picker4, HSV conversion round-trip, alpha bar |
| `test_p14_combo` | 3 | Combo open/close/select |
| `test_p14_selectable` | 3 | Selectable click, selected highlight, `ELI_SELECTABLE_DONT_CLOSE_POPUPS` |
| `test_p14_listbox` | 2 | List box item iteration, selection |
| `test_p15_trees` | 7 | Tree open/close, indentation, collapsing header |
| `test_p16_menus` | 6 | Menu bar open, menu item click, shortcut display, submenu |
| `test_p17_popups` | 6 | Popup open/close, context-item popup, modal backdrop |
| `test_p18_tooltip` | 5 | Tooltip appear on hover, item tooltip, hover delay |
| `test_p20_tabs` | 5 | Tab bar open/close, tab item selection, tab close button |
| `test_p21_drag_drop` | 4 | Payload set/accept, drop-target highlight, payload cleared on cancel |
| `test_p22_images` | 10 | `eli_image`, `eli_image_button`, textured draw-list primitives |
| `test_p23_plot` | 11 | `eli_plot_lines` (array + fn), `eli_plot_histogram` (array + fn), scale/bounds |
| `test_p24_value_display` | 8 | `eli_value_bool/int/uint/float` output format |
| `test_p25_disabled` | 3 | `eli_begin_disabled` blocks interaction |
| `test_p25_clip` | 2 | `eli_push_clip_rect`, `eli_pop_clip_rect` |
| `test_p25_focus` | 4 | `eli_set_item_default_focus`, `eli_set_keyboard_focus_here` |
| `test_p26_list_clipper` | 8 | Clipper step range, `include_item_by_index`, `seek_cursor_for_item` |
| `test_p27_util` | 6 | `eli_is_rect_visible`, `eli_get_time`, `eli_get_frame_count`, `eli_get_main_viewport`, bg/fg draw lists |
| `test_p28_settings` | 4 | INI save/load round-trip, per-window state persistence |
| `test_p28_logging` | 4 | Log to tty/file/clipboard, `eli_log_finish`, `eli_log_text` |
| `test_p30_demo` | 4 | `eli_show_demo_window` runs without crash, returns |
| `test_p30_debug` | 5 | `eli_show_metrics_window`, `eli_show_about_window`, `eli_get_version` |

### Table functionality

| Suite | Tests | What is covered |
|-------|-------|-----------------|
| `test_p19_tables` | 8 | Column count, cursor-to-column alignment, stretch-width split, `set_column_index` jump, column names, hidden column exclusion, row/cell background color, header click toggles sort asc/desc |

### Style and memory

| Suite | Tests | What is covered |
|-------|-------|-----------------|
| `test_style_colors` | 5 | Dark/light/classic themes set non-zero colors for all `ELI_COL_*` indices |
| `test_style_stack` | 6 | `eli_push/pop_style_color`, `eli_push/pop_style_var`, `eli_push/pop_item_flag` |
| `test_style_utils` | 7 | `eli_get_color_u32`, `eli_get_style_color_name`, HSV↔RGB conversions |
| `test_mem` | 8 | Default allocator, custom allocator override, `eli_mem_alloc`/`eli_mem_free` |

### Font system

| Suite | Tests | What is covered |
|-------|-------|-----------------|
| `test_font_atlas` | 9 | Default font load, `eli_font_atlas_build`, `get_tex_data_as_rgba32`, glyph lookup |
| `test_font_text` | 7 | `eli_calc_text_size`, multi-line text metrics, `eli_draw_list_add_text` vertex output |

### Cross-subsystem integration

| Suite | Tests | What is covered |
|-------|-------|-----------------|
| `test_integration_frame` | 2 | Full 5-frame cycle with button + table + tabs + popup: draw data valid, id stack balanced, widget shutdown hook registered; empty frame produces valid draw data |
| `test_integration_winid` | 3 | Same label yields different IDs across windows, window-seed ID stable across frames, id stack depth balanced across begin/end including child windows |
| `test_integration_channels` | 2 | Draw-channel split/merge preserves ordering across a multi-layer window frame |
| `test_p32_integration` | 3 | **Phase 32 cross-subsystem:** draw data vertex/index totals identical across 5 repeated frames (stability); widget item rects ordered vertically with positive dimensions (layout ordering); widget rects contained within window content region (window↔layout↔widget bridge) |

---

## Phase 32 Gap Analysis

Before Phase 32 landed, all eight coverage areas were already addressed by the suites
listed above (the per-phase tests established during Phases 1–31). No critical gap was
found.

The one addition made in Phase 32 is `tests/unit/test_p32_integration.c` — a
cross-cutting integration suite that verifies three invariants no existing suite covered
as a unit:

1. **Draw data stability**: identical frame content must produce identical `total_vtx_count`
   and `total_idx_count` across consecutive frames. This exercises the ID-hashing
   stability invariant (stable IDs → same layout decisions → same draw output).

2. **Vertical layout ordering**: widgets submitted in sequence must produce item rects
   where each rect's top edge is at or below the previous rect's bottom edge. This
   checks that the layout cursor advances correctly and that no two widgets collide.

3. **Window containment**: widget item rects must lie within the window's content region.
   This bridges the window-management subsystem (content region computed in `eli_begin`)
   through the layout system (cursor starts at content region origin) into the widget
   system (item rect recorded after the widget).

---

## Current Totals (Phase 32)

**62 suites, 337 tests, ALL GREEN.**
