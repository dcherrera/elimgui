/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file swapchain_selftest.c
 * @brief SDL3 example, sixth TU: `--swapchain-selftest`, the present-surface
 *        controls checked through the real backend and a real window.
 *
 * Checks: Metal drawable count 2 and vsync on as configured; while the window is
 * hidden (then minimised) the swapchain is released to 1x1 px and a frame draws
 * nothing, and on show/restore it comes back at the window's pixel size; a timed
 * redraw is dropped from the wait timeout while hidden (unless the app opts out)
 * and showing the window wakes a frame. Metal checks are skipped on other renderers.
 *
 * @status In use.
 * @issues Occlusion (a covered window) is not driven here: SDL cannot occlude its
 *         own window. It shares the code path of the hidden check.
 * @todo None
 */
#include "example_app.h"

#include <eli/elimgui.h>
#include <sdl3/eli_sdl3_platform.h>
#include <sdl3/eli_sdl3_renderer.h>

#include <string.h>

#define SWAPCHAIN_WAIT_MS 3000   /* window state changes animate on macOS */
#define SWAPCHAIN_IDLE_SUBMIT_MS 0.2   /* submit with nothing drawn: measured ~0.002 ms (drawing: >= 0.6) */

static int g_swapchain_failed;

/** Record one check. */
static void swapchain_check(bool ok, const char *what)
{
    SDL_Log("swapchain-selftest: %s %s", ok ? "PASS" : "FAIL", what);
    if (!ok)
        g_swapchain_failed++;
}

/** Feed every pending SDL event to both backends. */
static void swapchain_drain_events(void)
{
    SDL_Event e;
    SDL_PumpEvents();
    while (SDL_PollEvent(&e)) {
        eli_sdl3_renderer_process_event(&e);
        eli_sdl3_platform_process_event(&e);
    }
}

/** Drain events until the swapchain's released state is `want` (or the wait ends). */
static bool swapchain_wait_released(SDL_Window *window, bool want)
{
    Uint64 end = SDL_GetTicks() + SWAPCHAIN_WAIT_MS;
    SDL_SyncWindow(window);
    do {
        swapchain_drain_events();
        if (eli_sdl3_renderer_surface_released() == want)
            return true;
        SDL_Delay(10);
    } while (SDL_GetTicks() < end);
    return false;
}

/** @return true if the Metal drawables are `w` x `h` pixels. */
static bool swapchain_drawable_is(SDL_Renderer *renderer, int w, int h)
{
    int dw = 0, dh = 0;
    return eli_sdl3_metal_drawable_size(SDL_GetRenderMetalLayer(renderer), &dw, &dh) && dw == w && dh == h;
}

/** @return true if the Metal drawables match the window's pixel size. */
static bool swapchain_drawable_full(SDL_Window *window, SDL_Renderer *renderer)
{
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    return w > 1 && h > 1 && swapchain_drawable_is(renderer, w, h);
}

/** Build frames until the UI settles, then request a 0.5 s timed redraw. */
static void swapchain_settle_then_timer(void (*frame)(void))
{
    for (int i = 0; i < 4; i++)
        frame();
    eli_request_redraw_after(0.5f);
}

/** Hide/show: release, nothing drawn, timers paused, restore, show wakes a frame. */
static void swapchain_check_hide_show(SDL_Window *window, SDL_Renderer *renderer, bool metal,
                                      void (*frame)(void))
{
    SDL_HideWindow(window);
    bool released = swapchain_wait_released(window, true);
    if (metal) {
        swapchain_check(released && swapchain_drawable_is(renderer, 1, 1), "hidden: swapchain released to 1x1 px");
        frame();
        swapchain_check(swapchain_drawable_is(renderer, 1, 1), "hidden: a frame keeps it released");
        swapchain_check(g_example_stats.last_submit_ms < SWAPCHAIN_IDLE_SUBMIT_MS, "hidden: a frame draws nothing");
    }
    swapchain_settle_then_timer(frame);
    swapchain_check(eli_sdl3_wait_timeout_ms() == -1, "hidden: timed redraw paused (wait -1)");
    eli_sdl3_platform_set_run_timers_while_hidden(true);
    swapchain_check(eli_sdl3_wait_timeout_ms() == 500, "hidden + run_timers_while_hidden: wait 500 ms");
    eli_sdl3_platform_set_run_timers_while_hidden(false);

    SDL_ShowWindow(window);
    bool restored = swapchain_wait_released(window, false);
    if (metal)
        swapchain_check(restored && swapchain_drawable_full(window, renderer),
                        "shown: swapchain back at the window's pixel size");
    swapchain_check(eli_sdl3_wait_timeout_ms() == 0, "shown: a frame is due at once");
    swapchain_settle_then_timer(frame);
    swapchain_check(eli_sdl3_wait_timeout_ms() == 500, "shown: timed redraw runs (wait 500 ms)");
}

/** Minimise/restore: release and restore like hide/show. */
static void swapchain_check_minimize(SDL_Window *window, SDL_Renderer *renderer, void (*frame)(void))
{
    SDL_MinimizeWindow(window);
    bool released = swapchain_wait_released(window, true);
    swapchain_check(released && swapchain_drawable_is(renderer, 1, 1), "minimised: swapchain released");
    SDL_RestoreWindow(window);
    bool restored = swapchain_wait_released(window, false);
    frame();
    swapchain_check(restored && swapchain_drawable_full(window, renderer), "restored: swapchain back");
}

int example_swapchain_selftest(SDL_Window *window, SDL_Renderer *renderer, void (*frame)(void))
{
    g_swapchain_failed = 0;
    bool metal = SDL_GetRenderMetalLayer(renderer) != NULL;
    swapchain_drain_events();
    frame();
    if (metal) {
        swapchain_check(eli_sdl3_renderer_drawable_count() == 2, "drawable count is 2");
        swapchain_check(swapchain_drawable_full(window, renderer) && !eli_sdl3_renderer_surface_released(),
                        "visible: swapchain at the window's pixel size");
    } else {
        SDL_Log("swapchain-selftest: SKIP Metal checks (renderer %s)", SDL_GetRendererName(renderer));
    }
    Sint64 vsync = SDL_GetNumberProperty(SDL_GetRendererProperties(renderer), SDL_PROP_RENDERER_VSYNC_NUMBER, -9);
    swapchain_check(vsync == 1, "vsync on");

    swapchain_check_hide_show(window, renderer, metal, frame);
    if (metal)
        swapchain_check_minimize(window, renderer, frame);
    SDL_Log("swapchain-selftest: %d failed", g_swapchain_failed);
    return g_swapchain_failed;
}
