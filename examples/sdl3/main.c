/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file main.c
 * @brief SDL3 desktop example: window, renderer, fonts at the window's pixel
 *        density, and the main loop. The ELI_IMPLEMENTATION translation unit.
 *
 * The loop is event-driven (spec §17): it sleeps in SDL_WaitEvent(Timeout) for as
 * long as eli_get_redraw_timeout() allows and builds a frame only for input, a
 * timed change (caret blink, tooltip delay, key repeat, spinner) or a worker wake.
 *
 * Usage: elimgui_sdl3 [--frames N] [--screenshot out.bmp] [--font file.ttf]
 *                     [--font-size PX] [--driver NAME] [--idle-report SECONDS]
 *                     [--focus-input] [--skip-unchanged]
 *   --frames N         build and render N frames, then exit 0 (CI / sanitizer runs)
 *   --screenshot F     save the last rendered frame as a BMP
 *   --font F           use a TTF instead of the embedded ProggyClean
 *   --driver NAME      SDL render driver (metal, opengl, vulkan, software, ...)
 *   --idle-report S    run S seconds, then print frames built and CPU used after a
 *                      2 s warm-up (moves the mouse outside the window first)
 *   --focus-input      click the Name field once it exists (caret-blink idle check)
 *   --skip-unchanged   skip render+present when the draw-data hash is unchanged
 *   --input-selftest   run the input checklist through the backend, print PASS/FAIL,
 *                      exit with the number of failures (restores the clipboard)
 *   --hidden           create the window hidden (idle measurements on a machine in use:
 *                      no stray pointer events from the person at the desk)
 *
 * @status In use (spec §17).
 * @issues None
 * @todo None
 */
#define _POSIX_C_SOURCE 200809L   /* sigwait/pthread_sigmask under -std=c11 (glibc) */
#define ELI_IMPLEMENTATION
#include <eli/elimgui.h>
#include <sdl3/eli_sdl3_platform.h>
#include <sdl3/eli_sdl3_quit_signals.h>
#include <sdl3/eli_sdl3_renderer.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

#include "example_app.h"

#define EXAMPLE_IDLE_WARMUP_S 2.0

example_stats g_example_stats;

/** Parsed command line. */
typedef struct example_options {
    long frames;                /* 0 = run until quit */
    const char *screenshot;
    const char *font_path;
    float font_size;
    const char *driver;
    double idle_report_s;       /* 0 = off */
    bool focus_input;
    bool skip_unchanged;
    bool hidden;
    bool input_selftest;
} example_options;

/** App-owned SDL + elimgui handles. */
typedef struct example_app {
    SDL_Window *window;
    SDL_Renderer *renderer;
    eli_font_atlas *atlas;
    void *font_file;            /* TTF bytes from --font (SDL_free'd) */
    uint64_t last_hash;         /* --skip-unchanged: previous draw-data hash */
    uint64_t skipped_presents;
    int focus_step;             /* --focus-input: 0 wait, 1 pressed, 2 released, 3 done */
} example_app;

/** --idle-report measurement window. */
typedef struct example_idle_probe {
    Uint64 start_ms;
    bool warm;                  /* warm-up done, counters captured */
    uint64_t frames_at_warm;
    double cpu_at_warm;
} example_idle_probe;

/** Events seen after the idle warm-up, by type (diagnoses unexpected wakes). */
#define EXAMPLE_EVENT_LOG_MAX 16
static struct { Uint32 type; int count; } g_example_event_log[EXAMPLE_EVENT_LOG_MAX];
static bool g_example_event_log_on = false;

/** Count one event type in the idle log (first EXAMPLE_EVENT_LOG_MAX types). */
static void example_log_event(Uint32 type)
{
    if (!g_example_event_log_on)
        return;
    for (int i = 0; i < EXAMPLE_EVENT_LOG_MAX; i++) {
        if (g_example_event_log[i].count == 0)
            g_example_event_log[i].type = type;
        if (g_example_event_log[i].type == type) {
            g_example_event_log[i].count++;
            return;
        }
    }
}

/** Parse argv; returns false (after printing usage) on a bad argument. */
static bool example_parse_args(int argc, char **argv, example_options *opt)
{
    memset(opt, 0, sizeof(*opt));
    opt->font_size = 15.0f;
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        bool has_value = i + 1 < argc;
        if (!strcmp(a, "--frames") && has_value)
            opt->frames = strtol(argv[++i], NULL, 10);
        else if (!strcmp(a, "--screenshot") && has_value)
            opt->screenshot = argv[++i];
        else if (!strcmp(a, "--font") && has_value)
            opt->font_path = argv[++i];
        else if (!strcmp(a, "--font-size") && has_value)
            opt->font_size = strtof(argv[++i], NULL);
        else if (!strcmp(a, "--driver") && has_value)
            opt->driver = argv[++i];
        else if (!strcmp(a, "--idle-report") && has_value)
            opt->idle_report_s = strtod(argv[++i], NULL);
        else if (!strcmp(a, "--focus-input"))
            opt->focus_input = true;
        else if (!strcmp(a, "--skip-unchanged"))
            opt->skip_unchanged = true;
        else if (!strcmp(a, "--hidden"))
            opt->hidden = true;
        else if (!strcmp(a, "--input-selftest"))
            opt->input_selftest = true;
        else {
            fprintf(stderr, "usage: %s [--frames N] [--screenshot F.bmp] [--font F.ttf] "
                            "[--font-size PX] [--driver NAME] [--idle-report S] "
                            "[--focus-input] [--skip-unchanged] [--hidden] "
                            "[--input-selftest]\n", argv[0]);
            return false;
        }
    }
    return true;
}

/**
 * (Re)bake the atlas at `density` texels per point and upload it. The first call
 * adds the font; later calls (display scale change) only rebake, so eli_font
 * pointers stay valid.
 */
static bool example_build_fonts(example_app *app, const example_options *opt, float density)
{
    if (!app->atlas) {
        app->atlas = eli_font_atlas_create();
        eli_font_config cfg = eli_font_config_default();
        cfg.rasterizer_density = density;
        eli_font *font = NULL;
        size_t size = 0;
        if (opt->font_path && (app->font_file = SDL_LoadFile(opt->font_path, &size)) != NULL)
            font = eli_font_atlas_add_font_from_memory_ttf(app->atlas, app->font_file, (int)size,
                                                           opt->font_size, &cfg, NULL);
        else if (opt->font_path)
            SDL_Log("example: cannot load font %s: %s", opt->font_path, SDL_GetError());
        if (!font)
            font = eli_font_atlas_add_font_default(app->atlas, &cfg);
        eli_context *ctx = eli_get_current_context();
        ctx->io.fonts = app->atlas;
        ctx->io.font_default = font;
    }
    for (int i = 0; i < app->atlas->configs_count; i++)
        app->atlas->configs[i].rasterizer_density = density;
    if (!eli_font_atlas_build(app->atlas) || !eli_sdl3_renderer_create_fonts_texture(app->atlas))
        return false;
    if (eli_get_font() == NULL)
        eli_push_font(eli_get_io()->font_default); /* after the bake: font_size is known */
    eli_font_atlas_release_cpu_pixels(app->atlas);
    g_example_stats.density = density;
    return true;
}

/** Create the window + renderer and attach the elimgui backends. */
static bool example_init(example_app *app, const example_options *opt)
{
    eli_sdl3_quit_signals_install();   /* SIGINT/SIGTERM wake the idle loop */
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    if (opt->driver)
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, opt->driver);
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("example: SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (opt->hidden)
        flags |= SDL_WINDOW_HIDDEN;
    if (!SDL_CreateWindowAndRenderer("elimgui SDL3", 1280, 800, flags, &app->window,
                                     &app->renderer)) {
        SDL_Log("example: window/renderer failed: %s", SDL_GetError());
        return false;
    }
    SDL_SetRenderVSync(app->renderer, 1);
    g_example_stats.renderer_name = SDL_GetRendererName(app->renderer);

    example_tasks_init();
    eli_context *ctx = eli_create_context();
    eli_style_colors_dark(&ctx->style);
    if (!eli_sdl3_platform_init(app->window) || !eli_sdl3_renderer_init(app->renderer))
        return false;
    return example_build_fonts(app, opt, SDL_GetWindowPixelDensity(app->window));
}

/** Handle one SDL event; returns false when the app should quit. */
static bool example_handle_event(example_app *app, const example_options *opt, const SDL_Event *e)
{
    example_log_event(e->type);
    if (e->type == SDL_EVENT_QUIT || e->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
        return false;
    if (e->type == g_example_wake_event)
        example_tasks_apply_wake();
    else if (e->type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED)
        example_build_fonts(app, opt, SDL_GetWindowPixelDensity(app->window));
    eli_sdl3_platform_process_event(e);
    return true;
}

/**
 * Sleep until an event or the redraw deadline (capped at `cap_ms` when >= 0),
 * then drain all pending events.
 *
 * @param need_frame  Set when a frame should be built (events, or the library's
 *                    deadline expired rather than the cap).
 * @return            false when the app should quit.
 */
static bool example_wait_events(example_app *app, const example_options *opt, int cap_ms,
                                bool *need_frame)
{
    int lib_ms = eli_sdl3_wait_timeout_ms();
    int wait_ms = lib_ms;
    if (cap_ms >= 0 && (wait_ms < 0 || cap_ms < wait_ms))
        wait_ms = cap_ms;
    SDL_Event e;
    bool got = (wait_ms < 0) ? SDL_WaitEvent(&e) : SDL_WaitEventTimeout(&e, wait_ms);
    bool running = true;
    *need_frame = got || (lib_ms >= 0 && wait_ms == lib_ms);
    while (got) {
        running = example_handle_event(app, opt, &e) && running;
        got = SDL_PollEvent(&e);
    }
    return running;
}

/** --focus-input: click the Name field once laid out, then move the mouse away. */
static void example_drive_focus(example_app *app)
{
    float x = 0.0f, y = 0.0f;
    if (app->focus_step >= 3 || !example_shell_name_field_center(&x, &y))
        return;
    if (app->focus_step == 0) {
        eli_io_add_mouse_pos_event(x, y);
        eli_io_add_mouse_button_event(0, true);
    } else if (app->focus_step == 1) {
        eli_io_add_mouse_button_event(0, false);
    } else {
        eli_io_add_mouse_pos_event(-FLT_MAX, -FLT_MAX);
    }
    app->focus_step++;
    eli_request_redraw();
}

/** Build one elimgui frame, recording its CPU time. */
static void example_frame(void)
{
    Uint64 t0 = SDL_GetPerformanceCounter();
    eli_sdl3_platform_new_frame();
    eli_frame_begin();
    example_shell_draw();
    example_perf_fixture_draw();
    example_overlay_draw();
    eli_frame_end();
    eli_sdl3_platform_end_frame();
    Uint64 t1 = SDL_GetPerformanceCounter();
    g_example_stats.last_build_ms = (double)(t1 - t0) * 1000.0 / (double)SDL_GetPerformanceFrequency();
    g_example_stats.frames_built++;
}

/** Save the current backbuffer (before present) as a BMP. */
static void example_save_screenshot(example_app *app, const char *path)
{
    SDL_Surface *shot = SDL_RenderReadPixels(app->renderer, NULL);
    if (!shot || !SDL_SaveBMP(shot, path))
        SDL_Log("example: screenshot to %s failed: %s", path, SDL_GetError());
    else
        SDL_Log("example: saved %s (%dx%d)", path, shot->w, shot->h);
    SDL_DestroySurface(shot);
}

/** Release everything example_init created. */
static void example_shutdown(example_app *app)
{
    eli_sdl3_renderer_shutdown();
    eli_sdl3_platform_shutdown();
    eli_destroy_context(NULL);
    eli_font_atlas_destroy(app->atlas);
    SDL_free(app->font_file);
    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow(app->window);
    SDL_Quit();
}

/** @return user + system CPU seconds used by this process. */
static double example_cpu_seconds(void)
{
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return (double)ru.ru_utime.tv_sec + (double)ru.ru_utime.tv_usec * 1e-6 +
           (double)ru.ru_stime.tv_sec + (double)ru.ru_stime.tv_usec * 1e-6;
}

/**
 * --idle-report bookkeeping: captures counters after the warm-up, prints the
 * result at the end. Returns ms until the next checkpoint (-1 when off) and sets
 * *done when the report has been printed.
 */
static int example_idle_probe_step(example_idle_probe *p, const example_options *opt, bool *done)
{
    *done = false;
    if (opt->idle_report_s <= 0.0)
        return -1;
    double elapsed = (double)(SDL_GetTicks() - p->start_ms) / 1000.0;
    if (!p->warm && elapsed >= EXAMPLE_IDLE_WARMUP_S) {
        p->warm = true;
        p->frames_at_warm = g_example_stats.frames_built;
        p->cpu_at_warm = example_cpu_seconds();
        g_example_event_log_on = true;
    }
    if (p->warm && elapsed >= opt->idle_report_s) {
        double window = elapsed - EXAMPLE_IDLE_WARMUP_S;
        double cpu = example_cpu_seconds() - p->cpu_at_warm;
        SDL_Log("idle-report: %.1f s window: %llu frames, CPU %.3f s = %.3f%% of one core",
                window, (unsigned long long)(g_example_stats.frames_built - p->frames_at_warm),
                cpu, 100.0 * cpu / window);
        for (int i = 0; i < EXAMPLE_EVENT_LOG_MAX && g_example_event_log[i].count; i++)
            SDL_Log("idle-report: event 0x%x x%d", (unsigned)g_example_event_log[i].type,
                    g_example_event_log[i].count);
        *done = true;
        return 0;
    }
    double next = p->warm ? opt->idle_report_s : EXAMPLE_IDLE_WARMUP_S;
    return (int)((next - elapsed) * 1000.0) + 1;
}

/** Render + present the built frame, or skip both if unchanged (--skip-unchanged). */
static void example_present(example_app *app, const example_options *opt, bool last)
{
    if (opt->skip_unchanged) {
        uint64_t h = eli_sdl3_draw_data_hash(eli_get_draw_data());
        if (h == app->last_hash && !last) {
            app->skipped_presents++;
            return;
        }
        app->last_hash = h;
    }
    Uint64 t0 = SDL_GetPerformanceCounter();
    SDL_SetRenderDrawColor(app->renderer, 20, 20, 24, 255);
    SDL_RenderClear(app->renderer);
    eli_sdl3_renderer_render_draw_data(eli_get_draw_data());
    g_example_stats.last_submit_ms =
        (double)(SDL_GetPerformanceCounter() - t0) * 1000.0 / (double)SDL_GetPerformanceFrequency();
    if (last && opt->screenshot)
        example_save_screenshot(app, opt->screenshot);
    SDL_RenderPresent(app->renderer);
}

/* --input-selftest: the selftest drives frames through this callback. */
static example_app *g_example_present_app = NULL;
static const example_options g_example_selftest_opt = {0};

/** Build, render and present one frame (selftest frame callback). */
static void example_frame_and_present(void)
{
    example_frame();
    example_present(g_example_present_app, &g_example_selftest_opt, false);
}

int main(int argc, char **argv)
{
    example_options opt;
    example_app app = {0};
    example_idle_probe probe = {0};
    if (!example_parse_args(argc, argv, &opt))
        return 2;
    if (!example_init(&app, &opt)) {
        example_shutdown(&app);
        return 1;
    }
    if (opt.input_selftest) {
        g_example_present_app = &app;
        int failed = example_input_selftest(app.window, example_frame_and_present);
        example_shutdown(&app);
        return failed ? 3 : 0;
    }
    if (opt.idle_report_s > 0.0)
        SDL_WarpMouseGlobal(0.0f, 0.0f);   /* spec §20.3: mouse outside the window */
    probe.start_ms = SDL_GetTicks();
    bool running = true;
    while (running) {
        bool done = false, need_frame = true;
        if (opt.frames > 0)
            eli_request_redraw();          /* --frames: render back to back */
        int cap = example_idle_probe_step(&probe, &opt, &done);
        if (done)
            break;
        running = example_wait_events(&app, &opt, cap, &need_frame);
        if (!need_frame)
            continue;
        if (opt.focus_input)
            example_drive_focus(&app);
        example_frame();
        bool last = opt.frames > 0 && (long)g_example_stats.frames_built >= opt.frames;
        example_present(&app, &opt, last);
        if (last)
            running = false;
    }
    SDL_Log("example: %llu frames (%llu presents skipped), last build %.3f ms, submit %.3f ms, "
            "%d vertices, renderer %s", (unsigned long long)g_example_stats.frames_built,
            (unsigned long long)app.skipped_presents, g_example_stats.last_build_ms,
            g_example_stats.last_submit_ms, eli_get_io()->metrics_render_vertices,
            g_example_stats.renderer_name);
    example_shutdown(&app);
    return 0;
}
