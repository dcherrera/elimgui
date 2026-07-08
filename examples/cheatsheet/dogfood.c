/**
 * @file dogfood.c
 * @brief Headless dogfooding harness for the visual cheatsheet.
 *
 * Runs the real cheatsheet UI natively (via the ELI_TEST_HOSTED libc seam),
 * simulates input (mouse move, wheel, clicks), and dumps internal window state
 * so behavior can be diagnosed without a browser — e.g. verifying that a pane
 * actually scrolls, that hover resolves to the right (possibly child) window, or
 * that docked windows dock/undock. Build + run with `./build.sh dogfood`.
 *
 * This is a developer tool, not part of the library or the shipped app.
 *
 * @status In use for cheatsheet diagnosis.
 * @issues None
 * @todo None
 */
#include <eli/elimgui.h>

#include "cheat_engine.h"
#include "cheat_entries.h"

#include <stdio.h>

static eli_context *g_ctx;
static eli_font_atlas *g_atlas;

/** Compose one frame of the cheatsheet UI (mirrors examples/cheatsheet/main.c). */
static void dogfood_build_ui(void)
{
    eli_vec2 display = eli_get_io()->display_size;
    eli_set_next_window_pos(eli_make_vec2(0.0f, 0.0f), ELI_COND_ALWAYS, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(display, ELI_COND_ALWAYS);
    eli_window_flags flags = ELI_WINDOW_NO_TITLEBAR | ELI_WINDOW_NO_RESIZE |
                             ELI_WINDOW_NO_MOVE | ELI_WINDOW_NO_COLLAPSE;
    if (eli_begin("elimgui Cheatsheet", NULL, flags)) {
        eli_text("elimgui Visual Cheatsheet");
        eli_separator();
        cheat_show(cheat_all_entries, cheat_all_entries_count);
    }
    eli_end();
}

/** Run one frame with the given mouse position, wheel delta, and left-button state. */
static void dogfood_frame(float mx, float my, float wheel, int mouse_down)
{
    eli_get_io()->delta_time = 1.0f / 60.0f;
    eli_io_add_mouse_pos_event(mx, my);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, mouse_down != 0);
    if (wheel != 0.0f)
        eli_io_add_mouse_wheel_event(0.0f, wheel);
    eli_frame_begin();
    dogfood_build_ui();
    eli_frame_end();
}

/** Dump every live window's geometry + scroll state. */
static void dogfood_dump(const char *tag)
{
    printf("== %s ==\n", tag);
    for (int i = 0; i < g_ctx->windows_count; i++) {
        eli_window *w = g_ctx->windows[i];
        printf("  '%-22s' pos(%4.0f,%4.0f) size(%4.0f,%4.0f) scroll.y=%7.1f max.y=%8.1f docked=%d\n",
               w->name ? w->name : "?", w->pos.x, w->pos.y, w->size.x, w->size.y,
               w->scroll.y, w->scroll_max.y, (w->dock_node != NULL));
    }
    printf("  hovered=%s\n",
           g_ctx->hovered_window && g_ctx->hovered_window->name ? g_ctx->hovered_window->name : "(none)");
}

int main(void)
{
    g_ctx = eli_create_context();
    eli_set_current_context(g_ctx);
    eli_style_colors_dark(&g_ctx->style);

    g_atlas = eli_font_atlas_create();
    eli_font_atlas_add_font_default(g_atlas, NULL);
    eli_font_atlas_build(g_atlas);
    eli_font_atlas_set_tex_id(g_atlas, 1);
    unsigned char *px; int aw, ah, bpp;
    eli_font_atlas_get_tex_data_as_rgba32(g_atlas, &px, &aw, &ah, &bpp);
    eli_push_font(g_atlas->fonts[0]);
    g_ctx->io.fonts = g_atlas;
    g_ctx->io.font_default = g_atlas->fonts[0];
    g_ctx->io.display_size = eli_make_vec2(1280.0f, 720.0f);

    for (int i = 0; i < 3; i++)
        dogfood_frame(900.0f, 400.0f, 0.0f, 0);
    dogfood_dump("settled (mouse over content pane)");

    for (int i = 0; i < 6; i++)
        dogfood_frame(900.0f, 400.0f, -2.0f, 0);
    dogfood_dump("after wheel-down over content pane");

    return 0;
}
