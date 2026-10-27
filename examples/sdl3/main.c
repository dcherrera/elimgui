/**
 * @file main.c
 * @brief SDL3 desktop example: window, renderer, fonts at the window's pixel
 *        density, and the main loop. The ELI_IMPLEMENTATION translation unit.
 *
 * Usage: elimgui_sdl3 [--frames N] [--screenshot out.bmp] [--font file.ttf]
 *                     [--font-size PX] [--driver NAME]
 *   --frames N        build and render N frames, then exit 0 (CI / sanitizer runs)
 *   --screenshot F    save the last rendered frame as a BMP
 *   --font F          use a TTF instead of the embedded ProggyClean
 *   --driver NAME     SDL render driver (metal, opengl, vulkan, software, ...)
 *
 * @status In use (spec §17). Loop is a plain vsync loop (event-driven redraw is Phase C).
 * @issues None
 * @todo Event-driven loop (Phase C).
 */
#define ELI_IMPLEMENTATION
#include <eli/elimgui.h>
#include <sdl3/eli_sdl3_platform.h>
#include <sdl3/eli_sdl3_renderer.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "example_app.h"

example_stats g_example_stats;

/** Parsed command line. */
typedef struct example_options {
    long frames;                /* 0 = run until quit */
    const char *screenshot;
    const char *font_path;
    float font_size;
    const char *driver;
} example_options;

/** App-owned SDL + elimgui handles. */
typedef struct example_app {
    SDL_Window *window;
    SDL_Renderer *renderer;
    eli_font_atlas *atlas;
    void *font_file;            /* TTF bytes from --font (SDL_free'd) */
} example_app;

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
        else {
            fprintf(stderr, "usage: %s [--frames N] [--screenshot F.bmp] [--font F.ttf] "
                            "[--font-size PX] [--driver NAME]\n", argv[0]);
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
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    if (opt->driver)
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, opt->driver);
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("example: SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    if (!SDL_CreateWindowAndRenderer("elimgui SDL3", 1280, 800,
                                     SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY,
                                     &app->window, &app->renderer)) {
        SDL_Log("example: window/renderer failed: %s", SDL_GetError());
        return false;
    }
    SDL_SetRenderVSync(app->renderer, 1);
    g_example_stats.renderer_name = SDL_GetRendererName(app->renderer);

    eli_context *ctx = eli_create_context();
    eli_style_colors_dark(&ctx->style);
    if (!eli_sdl3_platform_init(app->window) || !eli_sdl3_renderer_init(app->renderer))
        return false;
    return example_build_fonts(app, opt, SDL_GetWindowPixelDensity(app->window));
}

/** Drain pending events; returns false when the app should quit. */
static bool example_pump_events(example_app *app, const example_options *opt)
{
    SDL_Event e;
    bool running = true;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT || e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
            running = false;
        else if (e.type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED)
            example_build_fonts(app, opt, SDL_GetWindowPixelDensity(app->window));
        eli_sdl3_platform_process_event(&e);
    }
    return running;
}

/** Build, render and present one frame, recording CPU times. */
static void example_frame(example_app *app)
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

    SDL_SetRenderDrawColor(app->renderer, 20, 20, 24, 255);
    SDL_RenderClear(app->renderer);
    eli_sdl3_renderer_render_draw_data(eli_get_draw_data());
    Uint64 t2 = SDL_GetPerformanceCounter();

    double ms = 1000.0 / (double)SDL_GetPerformanceFrequency();
    g_example_stats.last_build_ms = (double)(t1 - t0) * ms;
    g_example_stats.last_submit_ms = (double)(t2 - t1) * ms;
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

int main(int argc, char **argv)
{
    example_options opt;
    example_app app = {0};
    if (!example_parse_args(argc, argv, &opt))
        return 2;
    if (!example_init(&app, &opt)) {
        example_shutdown(&app);
        return 1;
    }
    bool running = true;
    while (running) {
        running = example_pump_events(&app, &opt);
        example_frame(&app);
        bool last = opt.frames > 0 && (long)g_example_stats.frames_built >= opt.frames;
        if (last && opt.screenshot)
            example_save_screenshot(&app, opt.screenshot);
        SDL_RenderPresent(app.renderer);
        if (last)
            running = false;
    }
    SDL_Log("example: %llu frames, last build %.3f ms, submit %.3f ms, %d vertices, renderer %s",
            (unsigned long long)g_example_stats.frames_built, g_example_stats.last_build_ms,
            g_example_stats.last_submit_ms, eli_get_io()->metrics_render_vertices,
            g_example_stats.renderer_name);
    example_shutdown(&app);
    return 0;
}
