# Demo & Debug Windows

Phase 30 adds the showcase demo window and the debug/inspection tooling, mirroring
Dear ImGui's `ShowDemoWindow` / metrics / style editor. Everything here is composed
purely from the public widget/layout/container API — there are no new core types.

The category lives under `include/eli/demo/` and is reached through the aggregator:

```c
#include <eli/demo/eli_demo_all.h>   /* or the umbrella <eli/elimgui.h> */
```

| Header | Contents |
|--------|----------|
| `demo/eli_demo.h` | `eli_show_demo_window` and its per-section helpers |
| `demo/eli_debug.h` | Metrics, debug log, id-stack tool, about, style/font selectors, style editor, user guide, `eli_get_version`, `eli_debug_log` |
| `demo/eli_demo_all.h` | Aggregator including both |

## Demo Window

### `void eli_show_demo_window(bool *p_open)`

One large window whose collapsing-header sections exercise the real widgets. It is
both a feature showcase and a smoke test of the whole library.

- **`p_open`** — optional visibility flag. When non-NULL, a close button in the title
  bar clears it, and the window is skipped entirely while `*p_open` is false. Pass
  `NULL` for an always-open window.

Sections (each an `eli_collapsing_header`): Basic widgets, Layout, Input widgets,
Sliders & Drags, Color widgets, Trees & Collapsing, Tables, Tabs, Popups & Modals,
Drag & Drop, and an embedded Style Editor. Per-widget values persist in a single
file-static `eli_demo_state` seeded with first-use defaults, so the demo is fully
self-contained.

```c
bool demo_open = true;
eli_frame_begin();
    eli_show_demo_window(&demo_open);
eli_frame_end();
```

## Debug & Inspection Windows

### `const char *eli_get_version(void)`

Returns the library version string (`ELI_VERSION`, currently `"1.0.0"`). Never NULL,
never empty; the returned pointer is a static string literal.

### `void eli_show_metrics_window(bool *p_open)`

Frame count and framerate, the assembled draw-data vertex/index/command-list totals
(`eli_get_draw_data()`), the active and hovered ids, and a collapsible list of every
live window with its position, size, and active/collapsed/hidden state.

### `void eli_show_debug_log_window(bool *p_open)`

Displays the accumulated debug log with **Clear** and a byte count. Application code
records lines with:

```c
void eli_debug_log(const char *fmt, ...);   /* printf-style; newline appended */
```

The log is a fixed 8 KB append buffer (no allocation); it resets when full.

### `void eli_show_id_stack_tool_window(bool *p_open)`

Shows the current ID-stack seed values (top to bottom) plus the last-item id — useful
for diagnosing id collisions between identically-labelled widgets.

### `void eli_show_about_window(bool *p_open)`

Version, a one-line description, and credits.

### `void eli_show_style_editor(eli_style *ref)`

Theme/font selectors, common sizing/rounding sliders (Alpha, WindowRounding,
FrameRounding, GrabRounding), and an editable `eli_color_edit4` row for every
themeable color (`ELI_COL_COUNT` entries). When `ref` is non-NULL a **Revert** button
copies it back over the live style. Pass `NULL` for an editor with no revert.

### `bool eli_show_style_selector(const char *label)`

A combo that applies **Dark / Light / Classic** to the current style. Returns `true`
on the frame the theme changes.

### `void eli_show_font_selector(const char *label)`

A font combo. elimgui builds a single embedded default font, so this reports the
active font and size rather than switching typefaces.

### `void eli_show_user_guide(void)`

Emits a short bulleted list of the mouse/keyboard interaction conventions. Call it
inside your own window (it does not open one).

## Usage Example

```c
#include <eli/elimgui.h>
#include <eli/demo/eli_demo_all.h>

static bool g_demo_open = true;
static bool g_metrics_open = true;

void render_frame(void)
{
    eli_frame_begin();
        eli_show_demo_window(&g_demo_open);
        eli_show_metrics_window(&g_metrics_open);
        eli_debug_log("frame %d", eli_get_frame_count());
    eli_frame_end();

    eli_draw_data *dd = eli_get_draw_data();   /* feed to your renderer */
    (void)dd;
}
```

## Gotchas

- All of these must be called **between** `eli_frame_begin()` and `eli_frame_end()`
  (or the equivalent hand-ordered hooks) — they each open their own window.
- `eli_show_style_editor` / `eli_show_style_selector` mutate the **current context's**
  live style (`eli_get_style()`), not a copy. Save a reference style and pass it as
  `ref` if you want a revert path.
- The demo and debug state (widget values, debug log, selected theme index) is
  process-wide file-static, matching the immediate-mode "no hidden per-frame
  allocation" model.
