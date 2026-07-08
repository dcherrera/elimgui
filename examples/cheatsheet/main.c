/**
 * @file main.c
 * @brief elimgui visual cheatsheet: an interactive browser app that showcases
 *        every elimgui widget alongside the exact eli_*() call that produces it,
 *        with search, category filtering, and copy-to-clipboard.
 *
 * This translation unit is the WASM app shell. It mirrors examples/demo/main.c's
 * export contract exactly (js_start, frame, input handlers, font-atlas and
 * draw-data getters) so the same web/elimgui.js Canvas2D renderer drives it. The
 * per-frame UI is delegated to the reusable cheatsheet engine, which renders from
 * the aggregated entry registry in cheat_entries.h.
 *
 * @status Cheatsheet app shell (stage 1). Not part of the elimgui library API.
 * @issues None
 * @todo None
 */

/* Drop asserts so the freestanding build needs no stdio/stderr. Must precede
 * any header that pulls <assert.h> (i.e. before <eli/elimgui.h>). */
#define NDEBUG

#include <eli/elimgui.h>

/* Freestanding libc glue (allocator, qsort, strtod, vsnprintf). Include once,
 * after the library, so the module links standalone under clang+wasm-ld. The
 * demo's runtime header is reused verbatim. */
#include "../demo/eli_wasm_runtime.h"

/* Cheatsheet engine + aggregated content registry. */
#include "cheat_engine.h"
#include "cheat_entries.h"

/* ---------------------------------------------------------------------------
 * Persistent state (context + font atlas)
 * ------------------------------------------------------------------------- */

static eli_context *g_ctx;
static eli_font_atlas *g_atlas;
static unsigned char *g_atlas_pixels; /* RGBA32, owned by the atlas */
static int g_atlas_w;
static int g_atlas_h;

/* ---------------------------------------------------------------------------
 * UI composition — a full-window host that hosts the cheatsheet engine
 * ------------------------------------------------------------------------- */

/** Open a borderless window covering the whole canvas and render the cheatsheet. */
static void cheat_build_ui(void)
{
    eli_io *io = eli_get_io();
    eli_vec2 display = io ? io->display_size : eli_make_vec2(1280.0f, 720.0f);

    eli_set_next_window_pos(eli_make_vec2(0.0f, 0.0f), ELI_COND_ALWAYS,
                            eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(display, ELI_COND_ALWAYS);

    eli_window_flags flags = ELI_WINDOW_NO_TITLEBAR | ELI_WINDOW_NO_RESIZE |
                             ELI_WINDOW_NO_MOVE | ELI_WINDOW_NO_COLLAPSE |
                             ELI_WINDOW_NO_BRING_TO_FRONT_ON_FOCUS;

    if (eli_begin("elimgui Cheatsheet", NULL, flags)) {
        eli_text("elimgui Visual Cheatsheet");
        eli_same_line(0.0f, -1.0f);
        eli_text_disabled("- live widgets with copyable eli_*() calls");
        eli_separator();
        cheat_show(cheat_all_entries, cheat_all_entries_count);
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

    eli_frame_begin();
    cheat_build_ui();
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
static eli_draw_list *cheat_list(int li)
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
    eli_draw_list *l = cheat_list(li);
    return l ? (uintptr_t)l->vtx : 0;
}

/** @return Vertex count of draw list `li`. */
JS_EXPORT(eli_dd_vtx_count)
int eli_dd_vtx_count(int li)
{
    eli_draw_list *l = cheat_list(li);
    return l ? (int)l->vtx_count : 0;
}

/** @return Pointer to draw list `li`'s index buffer (uint16). */
JS_EXPORT(eli_dd_idx_ptr)
uintptr_t eli_dd_idx_ptr(int li)
{
    eli_draw_list *l = cheat_list(li);
    return l ? (uintptr_t)l->idx : 0;
}

/** @return Index count of draw list `li`. */
JS_EXPORT(eli_dd_idx_count)
int eli_dd_idx_count(int li)
{
    eli_draw_list *l = cheat_list(li);
    return l ? (int)l->idx_count : 0;
}

/** @return Pointer to draw list `li`'s command array. */
JS_EXPORT(eli_dd_cmd_ptr)
uintptr_t eli_dd_cmd_ptr(int li)
{
    eli_draw_list *l = cheat_list(li);
    return l ? (uintptr_t)l->cmds : 0;
}

/** @return Command count of draw list `li`. */
JS_EXPORT(eli_dd_cmd_count)
int eli_dd_cmd_count(int li)
{
    eli_draw_list *l = cheat_list(li);
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
