/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file live_selftest.c
 * @brief SDL3 example `--live-selftest`: the Core Animation live widget backend
 *        through the real renderer and window (macOS, Metal).
 *
 * @status In use.
 * @issues None
 * @todo None
 *
 * Checks: a layer tree per widget under the container, with the pulse and spin
 * animations; the layers drawn where the widgets are and text upright (the layer
 * tree rendered offscreen with renderInContext: — not a screen capture: Metal
 * content is not included); no frame asked for while they animate and the
 * swapchain released meanwhile; a setter changing a layer without a frame;
 * occlusion by a window; removal; and the fallback once the backend is gone.
 * Elsewhere (no Core Animation) only the fallback is checked.
 */
#include "example_app.h"

#include <eli/elimgui.h>
#include <sdl3/eli_sdl3_live.h>
#include <sdl3/eli_sdl3_platform.h>
#include <sdl3/eli_sdl3_renderer.h>

#include <SDL3/SDL.h>

#include <stdio.h>

static int g_live_failed = 0;
static bool g_live_show = true;
static eli_vec2 g_live_cover = { 900.0f, 600.0f };   /* window B: away from the widgets */
static eli_id g_live_dot = 0, g_live_spin = 0, g_live_bar = 0, g_live_text = 0;

static void live_check(bool ok, const char *what)
{
    SDL_Log("live-selftest: %s %s", ok ? "PASS" : "FAIL", what);
    if (!ok)
        g_live_failed++;
}

/** The selftest UI: four live widgets, and a window that can cover them. */
static void live_ui(void)
{
    static const eli_live_pulse pulse = { 1.4f, 0.3f };
    eli_set_next_window_pos(eli_make_vec2(40, 40), ELI_COND_ALWAYS, eli_make_vec2(0, 0));
    eli_set_next_window_size(eli_make_vec2(420, 200), ELI_COND_ALWAYS);
    eli_begin("Live selftest", NULL, ELI_WINDOW_NO_SAVED_SETTINGS);
    if (g_live_show) {
        g_live_dot = eli_live_dot("dot", 8.0f, ELI_COL32(40, 220, 90, 255), &pulse);
        g_live_spin = eli_live_spinner("spin", 10.0f, ELI_COL32_WHITE);
        g_live_bar = eli_live_progress("bar", eli_make_vec2(200, 0), 0.4f);
        g_live_text = eli_live_text("text", "TTTT", 0.0f);
    }
    eli_end();
    eli_set_next_window_pos(g_live_cover, ELI_COND_ALWAYS, eli_make_vec2(0, 0));
    eli_set_next_window_size(eli_make_vec2(160, 120), ELI_COND_ALWAYS);
    eli_begin("Cover", NULL, ELI_WINDOW_NO_SAVED_SETTINGS);
    eli_text("on top");
    eli_end();
}

/** Build, render and present one selftest frame. */
static void live_frame(SDL_Renderer *r)
{
    eli_sdl3_platform_new_frame();
    eli_frame_begin();
    live_ui();
    eli_frame_end();
    eli_sdl3_platform_end_frame();
    SDL_SetRenderDrawColor(r, 20, 20, 24, 255);
    SDL_RenderClear(r);
    eli_sdl3_renderer_render_draw_data(eli_get_draw_data());
    SDL_RenderPresent(r);
    g_example_stats.frames_built++;
}

/** Frames until the library stops asking (at most `max`). */
static void live_settle(SDL_Renderer *r, int max)
{
    for (int i = 0; i < max; i++) {
        live_frame(r);
        SDL_PumpEvents();
        if (eli_get_redraw_timeout() < 0.0f)
            return;
    }
}

/**
 * Run the example's idle loop for `ms` with no frame unless the library asks:
 * @return frames built.
 */
static int live_idle(SDL_Renderer *r, Uint64 ms)
{
    int frames = 0;
    Uint64 end = SDL_GetTicks() + ms;
    while (SDL_GetTicks() < end) {
        int left = (int)(end - SDL_GetTicks());
        int lib = eli_sdl3_wait_timeout_ms();
        int wait = eli_sdl3_renderer_idle_wait_ms((lib >= 0 && lib < left) ? lib : left);
        SDL_Event e;
        if (SDL_WaitEventTimeout(&e, wait)) {
            do {   /* input from the desk is not the library asking: drop it */
                eli_sdl3_renderer_process_event(&e);
            } while (SDL_PollEvent(&e));
        }
        eli_sdl3_renderer_idle_step();
        if (lib >= 0 && lib <= wait) {
            live_frame(r);
            frames++;
        }
    }
    return frames;
}

#if defined(__APPLE__)

static unsigned long live_sublayer_count(id layer)
{
    id arr = layer ? eli_sdl3__msg(layer, "sublayers") : NULL;
    return arr ? ((unsigned long (*)(id, SEL))objc_msgSend)(arr, eli_sdl3__sel("count")) : 0ul;
}

static bool live_has_animation(id layer, const char *key)
{
    if (!layer)
        return false;
    id k = eli_sdl3__string(key);
    id a = ((id (*)(id, SEL, id))objc_msgSend)(layer, eli_sdl3__sel("animationForKey:"), k);
    eli_sdl3__release(k);
    return a != NULL;
}

static bool live_hidden(eli_id wid)
{
    id clip = (id)eli_sdl3_live_layer(wid, 0);
    return !clip || eli_sdl3__msg_ret_b(clip, "isHidden");
}

/**
 * Render the window's layer tree (without Metal content) into RGBA8 and check:
 * the dot's center has its green; the text's top rows carry more coverage than
 * its bottom rows ("TTTT": upright, not flipped).
 */
static void live_check_render(void)
{
    id root = (id)eli_sdl3_live_container();
    while (root && eli_sdl3__msg(root, "superlayer"))
        root = eli_sdl3__msg(root, "superlayer");
    CGRect b = eli_sdl3__msg_ret_rect(root, "bounds");
    float s = g_eli_sdl3_live.scale;
    int w = (int)(b.size.width * s), h = (int)(b.size.height * s);
    CGColorSpaceRef cs = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    CGContextRef cg = CGBitmapContextCreate(NULL, (size_t)w, (size_t)h, 8, 0, cs,
                                            (CGBitmapInfo)kCGImageAlphaPremultipliedLast);
    CGColorSpaceRelease(cs);
    if (!cg) {
        live_check(false, "offscreen render context");
        return;
    }
    CGContextScaleCTM(cg, s, s);
    eli_sdl3__msg_p(root, "renderInContext:", cg);
    const unsigned char *px = CGBitmapContextGetData(cg);
    size_t stride = CGBitmapContextGetBytesPerRow(cg);
    /* The root (the window frame, title bar included) renders as on screen: memory
     * rows run top-down. A window point (y down from the content's top-left) maps
     * through the CAMetalLayer's own geometry, not the container's, so a container
     * with the wrong flip fails. */
    bool root_flipped = eli_sdl3__msg_ret_b(root, "contentsAreFlipped");
    id metal = g_eli_sdl3_live.metal;
    CGRect mb = eli_sdl3__msg_ret_rect(metal, "bounds");
    CGPoint tl = ((CGPoint (*)(id, SEL, CGPoint, id))objc_msgSend)(
        metal, eli_sdl3__sel("convertPoint:toLayer:"),
        CGPointMake(0.0, eli_sdl3__msg_ret_b(metal, "contentsAreFlipped") ? 0.0 : mb.size.height),
        root);
#define LIVE_PIXEL(px_x, px_y, cx, cy)                                                      \
    do {                                                                                   \
        double ry_ = root_flipped ? tl.y + (cy) : tl.y - (cy);                             \
        (px_x) = (int)((tl.x + (cx)) * s);                                                 \
        (px_y) = (int)((root_flipped ? ry_ : b.size.height - ry_) * s);                    \
    } while (0)

    eli_rect dr = eli_live_find(g_live_dot)->rect;
    int dx = 0, dy = 0;
    LIVE_PIXEL(dx, dy, dr.x + dr.w * 0.5f, dr.y + dr.h * 0.5f);
    const unsigned char *p = px + (size_t)dy * stride + (size_t)dx * 4;
    SDL_Log("live-selftest: dot center px (%d,%d) rgba %d %d %d %d", dx, dy, p[0], p[1], p[2], p[3]);
    live_check(p[1] > 150 && p[0] < 120, "the dot layer is drawn at the dot's place");

    /* "TTTT" in light text: the top half of its box is brighter than the bottom. */
    eli_rect tr = eli_live_find(g_live_text)->rect;
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    LIVE_PIXEL(x0, y0, tr.x, tr.y);
    LIVE_PIXEL(x1, y1, tr.x + tr.w, tr.y + tr.h);
#undef LIVE_PIXEL
    if (y1 < y0) {
        int t = y0;
        y0 = y1;
        y1 = t;
    }
    long top = 0, bottom = 0;
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            int lum = px[(size_t)y * stride + (size_t)x * 4];
            if (y < y0 + (y1 - y0) / 2)
                top += lum;
            else
                bottom += lum;
        }
    }
    SDL_Log("live-selftest: text brightness top %ld bottom %ld", top, bottom);
    live_check(top > 0 && top > bottom, "the text layer is drawn upright at the text's place");
    CGContextRelease(cg);
}

static void live_selftest_layers(SDL_Renderer *r)
{
    live_check(eli_sdl3_live_container() != NULL, "container layer on the CAMetalLayer");
    live_settle(r, 10);
    const eli_live_stats *st = eli_live_get_stats();
    live_check(st->live == 4, "4 live widgets drawn by layers (dot, spinner, progress, text)");
    live_check(live_sublayer_count((id)eli_sdl3_live_container()) == 4, "one layer tree per widget");
    live_check(live_has_animation((id)eli_sdl3_live_layer(g_live_dot, 1), "eli.pulse"),
               "the dot pulses (opacity animation)");
    live_check(live_has_animation((id)eli_sdl3_live_layer(g_live_spin, 1), "eli.spin"),
               "the spinner turns (rotation animation)");
    CGRect fill = eli_sdl3__msg_ret_rect((id)eli_sdl3_live_layer(g_live_bar, 2), "frame");
    live_check(fill.size.width > 79.0 && fill.size.width < 81.0, "progress fill 0.4 x 200 pt");
    live_check(eli_sdl3__msg((id)eli_sdl3_live_layer(g_live_text, 2), "contents") != NULL,
               "text mask rastered from the atlas");
    live_check(!live_hidden(g_live_dot) && !live_hidden(g_live_text), "layers shown");
    live_check_render();

    uint64_t before = g_example_stats.frames_built;
    int frames = live_idle(r, 2500);
    SDL_Log("live-selftest: %d frames asked for in 2.5 s while animating", frames);
    live_check(frames == 0 && g_example_stats.frames_built == before,
               "no frame while the widgets animate");
    live_check(eli_sdl3_renderer_idle_released(), "the swapchain released while they animate");
    live_check(live_has_animation((id)eli_sdl3_live_layer(g_live_dot, 1), "eli.pulse"),
               "the pulse keeps running after the release");

    eli_live_set_fraction(g_live_bar, 0.9f);
    fill = eli_sdl3__msg_ret_rect((id)eli_sdl3_live_layer(g_live_bar, 2), "frame");
    live_check(fill.size.width > 179.0 && fill.size.width < 181.0 &&
                   g_example_stats.frames_built == before && eli_sdl3_renderer_idle_released(),
               "set_fraction moves the fill with no frame (swapchain still released)");

    g_live_cover = eli_make_vec2(30.0f, 50.0f);   /* over the widgets */
    live_settle(r, 10);
    live_check(live_hidden(g_live_dot), "a window over the dot hides its layer");
    g_live_cover = eli_make_vec2(900.0f, 600.0f);
    live_settle(r, 10);
    live_check(!live_hidden(g_live_dot), "uncovered, the layer shows again");

    g_live_show = false;
    live_settle(r, 10);
    live_check(live_sublayer_count((id)eli_sdl3_live_container()) == 0 &&
                   eli_live_get_stats()->count == 0,
               "widgets not submitted lose their layers");
    g_live_show = true;
    live_settle(r, 10);
}

#endif /* __APPLE__ */

int example_live_selftest(SDL_Window *window, SDL_Renderer *renderer)
{
    (void)window;
    g_live_failed = 0;
#if defined(__APPLE__)
    live_selftest_layers(renderer);
#endif
    eli_sdl3_live_shutdown();   /* the fallback: widgets draw on the main surface */
    live_settle(renderer, 3);
    live_frame(renderer);
    live_check(eli_live_get_stats()->live == 0 &&
                   eli_get_redraw_timeout() == eli_live_get_fallback_interval(),
               "without layers the widgets redraw at the fallback interval");
    SDL_Log("live-selftest: %d failed", g_live_failed);
    return g_live_failed;
}
