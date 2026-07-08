/**
 * @file main.c
 * @brief elimgui browser demo: a self-contained immediate-mode UI driven from
 *        JavaScript. Exposes a WASM entrypoint (js_start), a per-frame builder
 *        (frame), font-atlas + draw-data getters for the Canvas2D renderer, and
 *        input event entrypoints wired from DOM events by web/elimgui.js.
 *
 * The UI here does NOT depend on eli_show_demo_window — it hand-builds a window
 * exercising text, buttons, checkbox, radio, slider, input text/number, a color
 * editor, a tab bar, a table, and a progress bar, so the demo stands alone.
 *
 * @status Web-integration demo (Phase 31) in use.
 * @issues None
 * @todo None
 */

/* Drop asserts so the freestanding build needs no stdio/stderr. Must precede
 * any header that pulls <assert.h> (i.e. before <eli/elimgui.h>). */
#define NDEBUG

#include <eli/elimgui.h>

/* Freestanding libc glue (allocator, qsort, strtod, vsnprintf). Include once,
 * after the library, so the module links standalone under clang+wasm-ld. */
#include "eli_wasm_runtime.h"

/* ---------------------------------------------------------------------------
 * Persistent state (context, font atlas, and per-widget demo values)
 * ------------------------------------------------------------------------- */

static eli_context *g_ctx;
static eli_font_atlas *g_atlas;
static unsigned char *g_atlas_pixels; /* RGBA32, owned by the atlas */
static int g_atlas_w;
static int g_atlas_h;

/* Demo widget state — retained across frames (immediate-mode reads/writes it). */
static int g_click_count;
static bool g_check_enabled = true;
static bool g_check_wireframe;
static int g_radio_choice = 1;
static float g_slider_value = 0.42f;
static float g_drag_speed = 12.0f;
static double g_input_number = 3.14159;
static char g_input_buf[128] = "edit me";
static float g_color[4] = {0.26f, 0.59f, 0.98f, 1.0f};
static float g_progress;

/* ---------------------------------------------------------------------------
 * UI builders (kept small; called from frame())
 * ------------------------------------------------------------------------- */

/** Basic widgets tab: text, buttons, toggles, and value editors. */
static void demo_tab_widgets(void)
{
    eli_text("elimgui is a pure C11 reimplementation of Dear ImGui.");
    eli_text_disabled("Rendered by a Canvas2D backend over WebAssembly.");
    eli_bullet_text("Immediate mode: %d clicks so far", g_click_count);
    eli_separator();

    if (eli_button("Click me"))
        g_click_count++;
    eli_same_line(0.0f, -1.0f);
    if (eli_button("Reset"))
        g_click_count = 0;

    eli_checkbox("Enable feature", &g_check_enabled);
    eli_same_line(0.0f, -1.0f);
    eli_checkbox("Wireframe", &g_check_wireframe);

    eli_text("Mode:");
    eli_same_line(0.0f, -1.0f);
    eli_radio_button_int("Fast", &g_radio_choice, 0);
    eli_same_line(0.0f, -1.0f);
    eli_radio_button_int("Nice", &g_radio_choice, 1);
    eli_same_line(0.0f, -1.0f);
    eli_radio_button_int("Ultra", &g_radio_choice, 2);

    eli_separator();
    eli_slider_float("Value", &g_slider_value, 0.0f, 1.0f, "%.3f", 0);
    eli_drag_float("Speed", &g_drag_speed, 0.5f, 0.0f, 100.0f, "%.1f", 0);
    eli_input_double("Number", &g_input_number, 0.01, 1.0, "%.5f", 0);
    eli_input_text("Text", g_input_buf, sizeof(g_input_buf), 0, NULL, NULL);
    eli_color_edit4("Tint", g_color, 0);

    eli_separator();
    eli_progress_bar(g_progress, eli_make_vec2(0.0f, 0.0f), NULL);
}

/** Table tab: a small bordered table with a header row and typed cells. */
static void demo_tab_table(void)
{
    eli_text("A %d-column table with borders and striped rows:", 3);
    eli_table_flags flags = ELI_TABLE_BORDERS | ELI_TABLE_ROW_BG | ELI_TABLE_RESIZABLE;
    if (eli_begin_table("demo_table", 3, flags)) {
        eli_table_setup_column("Name", 0, 0.0f, 0);
        eli_table_setup_column("Kind", 0, 0.0f, 0);
        eli_table_setup_column("Value", 0, 0.0f, 0);
        eli_table_headers_row();

        static const char *names[] = {"alpha", "beta", "gamma", "delta"};
        static const char *kinds[] = {"float", "int", "bool", "string"};
        for (int row = 0; row < 4; row++) {
            eli_table_next_row();
            eli_table_next_column();
            eli_text("%s", names[row]);
            eli_table_next_column();
            eli_text("%s", kinds[row]);
            eli_table_next_column();
            eli_text("%.2f", (float)row * 1.5f + 0.25f);
        }
        eli_end_table();
    }
}

/** About tab: static informational text. */
static void demo_tab_about(void)
{
    eli_text_wrapped("This window is built every frame from plain C calls: "
                     "eli_begin/eli_end, widgets, then eli_frame_end. The draw "
                     "lists it produces are handed to JavaScript, which rasterizes "
                     "solid triangles and blits font-atlas glyphs onto a 2D canvas.");
    eli_separator();
    eli_bullet_text("Language: C11, header-only");
    eli_bullet_text("Target:   wasm32 via clang (no Emscripten)");
    eli_bullet_text("Renderer: Canvas2D");
    eli_text("Frame time: %.2f ms (%.0f FPS)", g_ctx ? g_ctx->io.delta_time * 1000.0f : 0.0f,
             g_ctx ? g_ctx->io.framerate : 0.0f);
}

/** Compose the whole demo window for this frame. */
static void demo_build_ui(void)
{
    eli_io *io = eli_get_io();
    float dw = io ? io->display_size.x : 1280.0f;

    eli_set_next_window_pos(eli_make_vec2(40.0f, 40.0f), ELI_COND_FIRST_USE_EVER,
                            eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(dw > 520.0f ? 480.0f : dw - 40.0f, 560.0f),
                            ELI_COND_FIRST_USE_EVER);

    if (eli_begin("elimgui - C11 / WASM Demo", NULL, ELI_WINDOW_NONE)) {
        if (eli_begin_tab_bar("demo_tabs", 0)) {
            if (eli_begin_tab_item("Widgets", NULL, 0)) {
                demo_tab_widgets();
                eli_end_tab_item();
            }
            if (eli_begin_tab_item("Table", NULL, 0)) {
                demo_tab_table();
                eli_end_tab_item();
            }
            if (eli_begin_tab_item("About", NULL, 0)) {
                demo_tab_about();
                eli_end_tab_item();
            }
            eli_end_tab_bar();
        }
    }
    eli_end();
}

/* ---------------------------------------------------------------------------
 * WASM lifecycle entrypoints (called by web/elimgui.js)
 * ------------------------------------------------------------------------- */

/**
 * One-time startup: create the context, apply the dark theme, bake the default
 * font atlas, hand its pixels to the renderer via the getters below, and make
 * the font current.
 */
JS_EXPORT(js_start)
void js_start(void)
{
    g_ctx = eli_create_context();
    eli_style_colors_dark(&g_ctx->style);

    g_atlas = eli_font_atlas_create();
    eli_font_atlas_add_font_default(g_atlas, NULL);
    eli_font_atlas_build(g_atlas);
    eli_font_atlas_set_tex_id(g_atlas, 1);

    int bytes_per_pixel;
    eli_font_atlas_get_tex_data_as_rgba32(g_atlas, &g_atlas_pixels, &g_atlas_w, &g_atlas_h,
                                          &bytes_per_pixel);

    eli_push_font(g_atlas->fonts[0]);
    g_ctx->io.fonts = g_atlas;
    g_ctx->io.font_default = g_atlas->fonts[0];
    g_ctx->io.display_size = eli_make_vec2(1280.0f, 720.0f);
}

/**
 * Build one UI frame. `dt` is the elapsed seconds since the previous frame,
 * supplied by the requestAnimationFrame loop.
 */
JS_EXPORT(frame)
void frame(double dt)
{
    eli_io *io = eli_get_io();
    if (io)
        io->delta_time = (dt > 0.0 && dt < 1.0) ? (float)dt : (1.0f / 60.0f);

    /* Animate the progress bar so motion is visible even when idle. */
    g_progress += (io ? io->delta_time : 1.0f / 60.0f) * 0.4f;
    if (g_progress > 1.0f)
        g_progress -= 1.0f;

    eli_frame_begin();
    demo_build_ui();
    eli_frame_end();
}

/** Set the display (canvas) size in logical pixels; call on load and resize. */
JS_EXPORT(eli_set_display_size)
void eli_set_display_size(float width, float height)
{
    eli_io *io = eli_get_io();
    if (io)
        io->display_size = eli_make_vec2(width, height);
}

/* ---------------------------------------------------------------------------
 * Font-atlas getters (renderer uploads the texture once after js_start)
 * ------------------------------------------------------------------------- */

/** @return Pointer (wasm memory offset) to the RGBA32 atlas pixels. */
JS_EXPORT(eli_atlas_pixels)
uintptr_t eli_atlas_pixels(void)
{
    return (uintptr_t)g_atlas_pixels;
}

/** @return Atlas texture width in texels. */
JS_EXPORT(eli_atlas_width)
int eli_atlas_width(void)
{
    return g_atlas_w;
}

/** @return Atlas texture height in texels. */
JS_EXPORT(eli_atlas_height)
int eli_atlas_height(void)
{
    return g_atlas_h;
}

/* ---------------------------------------------------------------------------
 * Draw-data getters (walked by the renderer each frame after frame())
 *
 * Vertex layout  (stride 20): float x,y,u,v; uint32 col   (RGBA byte order).
 * Command layout (stride 40): float clip.x,y,w,h; uint32 texture_id,
 *                             vtx_offset, idx_offset, elem_count; two pointers.
 * Index type: uint16.
 * ------------------------------------------------------------------------- */

/** Resolve draw list `li`, or NULL if out of range / no draw data. */
static eli_draw_list *demo_list(int li)
{
    eli_draw_data *dd = eli_get_draw_data();
    if (!dd || li < 0 || li >= dd->cmd_lists_count)
        return NULL;
    return dd->cmd_lists[li];
}

/** @return Number of draw lists in the current frame's draw data. */
JS_EXPORT(eli_dd_list_count)
int eli_dd_list_count(void)
{
    eli_draw_data *dd = eli_get_draw_data();
    return dd ? dd->cmd_lists_count : 0;
}

/** @return Pointer to draw list `li`'s vertex buffer. */
JS_EXPORT(eli_dd_vtx_ptr)
uintptr_t eli_dd_vtx_ptr(int li)
{
    eli_draw_list *l = demo_list(li);
    return l ? (uintptr_t)l->vtx : 0;
}

/** @return Vertex count of draw list `li`. */
JS_EXPORT(eli_dd_vtx_count)
int eli_dd_vtx_count(int li)
{
    eli_draw_list *l = demo_list(li);
    return l ? (int)l->vtx_count : 0;
}

/** @return Pointer to draw list `li`'s index buffer (uint16). */
JS_EXPORT(eli_dd_idx_ptr)
uintptr_t eli_dd_idx_ptr(int li)
{
    eli_draw_list *l = demo_list(li);
    return l ? (uintptr_t)l->idx : 0;
}

/** @return Index count of draw list `li`. */
JS_EXPORT(eli_dd_idx_count)
int eli_dd_idx_count(int li)
{
    eli_draw_list *l = demo_list(li);
    return l ? (int)l->idx_count : 0;
}

/** @return Pointer to draw list `li`'s command array. */
JS_EXPORT(eli_dd_cmd_ptr)
uintptr_t eli_dd_cmd_ptr(int li)
{
    eli_draw_list *l = demo_list(li);
    return l ? (uintptr_t)l->cmds : 0;
}

/** @return Command count of draw list `li`. */
JS_EXPORT(eli_dd_cmd_count)
int eli_dd_cmd_count(int li)
{
    eli_draw_list *l = demo_list(li);
    return l ? (int)l->cmd_count : 0;
}

/* ---------------------------------------------------------------------------
 * Input entrypoints (DOM event -> elimgui event queue)
 * ------------------------------------------------------------------------- */

/** Feed a mouse position (canvas-space pixels). */
JS_EXPORT(eli_on_mouse_pos)
void eli_on_mouse_pos(float x, float y)
{
    eli_io_add_mouse_pos_event(x, y);
}

/** Feed a mouse-button transition (0=left,1=right,2=middle; down!=0 = press). */
JS_EXPORT(eli_on_mouse_button)
void eli_on_mouse_button(int button, int down)
{
    eli_io_add_mouse_button_event(button, down != 0);
}

/** Feed a mouse-wheel delta. */
JS_EXPORT(eli_on_mouse_wheel)
void eli_on_mouse_wheel(float wheel_x, float wheel_y)
{
    eli_io_add_mouse_wheel_event(wheel_x, wheel_y);
}

/** Feed a keyboard transition (key is an eli_key code; down!=0 = press). */
JS_EXPORT(eli_on_key)
void eli_on_key(int key, int down)
{
    eli_io_add_key_event(key, down != 0);
}

/** Feed a typed Unicode codepoint for text-input widgets. */
JS_EXPORT(eli_on_char)
void eli_on_char(int codepoint)
{
    eli_io_add_input_character((unsigned int)codepoint);
}

/* ---------------------------------------------------------------------------
 * Clipboard bridge — satisfies the library's JS_IMPORT(eli_host_set_clipboard).
 * The host page installs the matching env import; this file only declares it via
 * the library header. No definition needed here.
 * ------------------------------------------------------------------------- */
