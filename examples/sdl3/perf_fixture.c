/**
 * @file perf_fixture.c
 * @brief SDL3 example, third TU: the text-heavy perf fixture window (120 x 48
 *        monospace glyphs, ~23K vertices, spec §20.3) and the frame overlay.
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#include "example_app.h"

#include <eli/elimgui.h>

#include <stdio.h>

/* One line of the fixture: EXAMPLE_FIXTURE_COLS printable chars + NUL. */
static char g_fixture_lines[EXAMPLE_FIXTURE_LINES][EXAMPLE_FIXTURE_COLS + 1];
static bool g_fixture_ready = false;

/** Fill the fixture with deterministic code-like text. */
static void fixture_fill(void)
{
    static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz0123456789 (){};=+-*/<>";
    for (int y = 0; y < EXAMPLE_FIXTURE_LINES; y++) {
        for (int x = 0; x < EXAMPLE_FIXTURE_COLS; x++)
            g_fixture_lines[y][x] = alphabet[(x * 7 + y * 13) % (int)(sizeof(alphabet) - 1)];
        g_fixture_lines[y][EXAMPLE_FIXTURE_COLS] = '\0';
    }
    g_fixture_ready = true;
}

void example_perf_fixture_draw(void)
{
    if (!g_fixture_ready)
        fixture_fill();
    unsigned int right = example_shell_fixture_dock_id();
    if (right != 0)
        eli_set_next_window_dock_id((eli_id)right, ELI_COND_FIRST_USE_EVER);
    if (eli_begin("Perf fixture", NULL, ELI_WINDOW_HORIZONTAL_SCROLLBAR)) {
        for (int y = 0; y < EXAMPLE_FIXTURE_LINES; y++)
            eli_text_unformatted(g_fixture_lines[y], NULL);
    }
    eli_end();
}

void example_overlay_draw(void)
{
    eli_io *io = eli_get_io();
    char line[160];
    snprintf(line, sizeof(line), "frames %llu  build %.2f ms  submit %.2f ms  density %.1f  %s",
             (unsigned long long)g_example_stats.frames_built, g_example_stats.last_build_ms,
             g_example_stats.last_submit_ms, (double)g_example_stats.density,
             g_example_stats.renderer_name ? g_example_stats.renderer_name : "?");
    eli_vec2 size = eli_calc_text_size(line, NULL);
    eli_vec2 pos = eli_make_vec2(io->display_size.x - size.x - 8.0f, io->display_size.y - size.y - 6.0f);
    eli_draw_list *fg = eli_get_foreground_draw_list();
    eli_draw_list_add_rect_filled(fg, eli_make_vec2(pos.x - 4.0f, pos.y - 2.0f),
                                  eli_make_vec2(pos.x + size.x + 4.0f, pos.y + size.y + 2.0f),
                                  ELI_COL32(0, 0, 0, 180), 3.0f, ELI_DRAW_ROUND_CORNERS_ALL);
    eli_draw_list_add_text(fg, pos, ELI_COL32(255, 220, 120, 255), line, NULL);
}
