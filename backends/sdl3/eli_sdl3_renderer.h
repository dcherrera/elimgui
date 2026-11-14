/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_sdl3_renderer.h
 * @brief SDL3 renderer backend: eli_draw_data -> SDL_RenderGeometryRaw, plus
 *        RGBA texture helpers, the font-atlas upload and present-surface control
 *        (vsync, Metal drawable count, swapchain release while hidden).
 *
 * Header-only, `static inline`. Include after <eli/elimgui.h>. Uses SDL_Renderer
 * (D2): works on Metal, OpenGL, GLES2, Vulkan, D3D and the software renderer.
 * Draw data is in points; the renderer applies SDL_SetRenderScale(framebuffer
 * scale) itself, so the app never scales. The app owns clear and present.
 *
 *     eli_sdl3_renderer_config cfg = { .drawable_count = 2, .vsync = ELI_SDL3_VSYNC_ON,
 *                                      .release_when_hidden = true };
 *     eli_sdl3_renderer_init_ex(renderer, &cfg);      // or eli_sdl3_renderer_init(renderer)
 *     eli_sdl3_renderer_create_fonts_texture(atlas);
 *     ... each SDL event: eli_sdl3_renderer_process_event(&e); ...
 *     ... each frame: eli_sdl3_renderer_render_draw_data(eli_get_draw_data()); ...
 *     eli_sdl3_renderer_shutdown();
 *
 * Texture ids are `SDL_Texture *` cast to eli_texture_id (uintptr_t).
 *
 * @status In use (desktop example).
 * @issues SDL3 has no 8-bit vertex color path, so colors are expanded to
 *         SDL_FColor once per draw list per frame (cost recorded in the spec).
 * @todo None
 */
#ifndef ELI_SDL3_RENDERER_H
#define ELI_SDL3_RENDERER_H

#include <SDL3/SDL.h>

#include <eli/elimgui.h>

#include "eli_sdl3_metal.h"

/** Vsync choice for eli_sdl3_renderer_config (KEEP = leave the renderer's setting). */
typedef enum eli_sdl3_vsync {
    ELI_SDL3_VSYNC_KEEP = 0,
    ELI_SDL3_VSYNC_OFF,
    ELI_SDL3_VSYNC_ON,
    ELI_SDL3_VSYNC_ADAPTIVE   /* late frames tear instead of waiting (where supported) */
} eli_sdl3_vsync;

/**
 * Present-surface options (zero-initialized = SDL's defaults, nothing released).
 * Each Metal drawable is one window-sized IOSurface (2560x1600 px = 15.6 MiB).
 */
typedef struct eli_sdl3_renderer_config {
    int drawable_count;           /* Metal swapchain depth, 2 or 3; 0 = keep (3) */
    eli_sdl3_vsync vsync;
    bool release_when_hidden;     /* shrink the swapchain to 1x1 px while hidden or minimised */
    bool release_when_occluded;   /* ... and while fully covered (the last frame stays on
                                     screen until the next present) */
} eli_sdl3_renderer_config;

/** Renderer backend state. */
typedef struct eli_sdl3_renderer_state {
    SDL_Renderer *renderer;
    SDL_FColor *colors;           /* per-list vertex color scratch (eli_mem) */
    int colors_capacity;
    SDL_Texture *font_texture;
    eli_sdl3_renderer_config config;
    void *metal_layer;            /* CAMetalLayer (init_ex; NULL on other renderers) */
    bool surface_released;        /* the Metal drawables are shrunk to 1x1 px */
} eli_sdl3_renderer_state;

ELI_GLOBAL(eli_sdl3_renderer_state, g_eli_sdl3_renderer, {0});

/* ---------------------------------------------------------------------------
 * Lifecycle
 * ------------------------------------------------------------------------- */

/**
 * Attach the renderer backend to an SDL renderer and advertise vtx_offset support.
 *
 * @param renderer  The app's SDL renderer (must outlive shutdown).
 * @return          false if renderer is NULL.
 *
 * Thread-safe: no (render thread = main thread)  Reentrant: no
 */
static inline bool eli_sdl3_renderer_init(SDL_Renderer *renderer)
{
    if (!renderer)
        return false;
    memset(&g_eli_sdl3_renderer, 0, sizeof(g_eli_sdl3_renderer));
    g_eli_sdl3_renderer.renderer = renderer;
    eli_io *io = eli_get_io();
    if (io)
        io->backend_flags |= ELI_BACKEND_FLAGS_RENDERER_HAS_VTX_OFFSET;
    return true;
}

/* ---------------------------------------------------------------------------
 * Present surface: vsync, drawable count, release while hidden
 * ------------------------------------------------------------------------- */

/** @return true when the config wants the swapchain released for these window flags. */
static inline bool eli_sdl3__surface_should_release(const eli_sdl3_renderer_config *cfg,
                                                    SDL_WindowFlags flags)
{
    if (cfg->release_when_hidden && (flags & (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED)))
        return true;
    return cfg->release_when_occluded && (flags & SDL_WINDOW_OCCLUDED);
}

/**
 * Shrink the Metal drawables to 1x1 px, or give them back the window's pixel size,
 * to match the window's visibility. A pixel-size change while released re-grows
 * the drawables (SDL resizes the layer), so a released surface is shrunk again.
 * No-op on renderers without a CAMetalLayer.
 */
static inline void eli_sdl3__surface_update(void)
{
    eli_sdl3_renderer_state *st = &g_eli_sdl3_renderer;
    void *layer = st->metal_layer;
    SDL_Window *window = layer ? SDL_GetRenderWindow(st->renderer) : NULL;
    if (!window || !(st->config.release_when_hidden || st->config.release_when_occluded))
        return;
    bool release = eli_sdl3__surface_should_release(&st->config, SDL_GetWindowFlags(window));
    int w = 0, h = 0;
    eli_sdl3_metal_drawable_size(layer, &w, &h);
    if (release && (w != 1 || h != 1)) {
        eli_sdl3_metal_set_drawable_size(layer, 1, 1);
    } else if (!release && st->surface_released) {
        SDL_GetWindowSizeInPixels(window, &w, &h);
        eli_sdl3_metal_set_drawable_size(layer, w, h);
    }
    st->surface_released = release;
}

/**
 * Attach the renderer backend (like eli_sdl3_renderer_init) and apply present-surface
 * options: vsync, Metal drawable count, and the swapchain-release policy, which
 * eli_sdl3_renderer_process_event then follows.
 *
 * @param renderer  The app's SDL renderer (must outlive shutdown).
 * @param config    Options (NULL = all defaults, same as eli_sdl3_renderer_init).
 * @return          false if renderer is NULL. An option the renderer refuses is
 *                  logged and skipped (check eli_sdl3_renderer_drawable_count or
 *                  SDL_PROP_RENDERER_VSYNC_NUMBER); a drawable count on a renderer
 *                  without a CAMetalLayer is ignored.
 *
 * Thread-safe: no (main thread)  Reentrant: no
 */
static inline bool eli_sdl3_renderer_init_ex(SDL_Renderer *renderer, const eli_sdl3_renderer_config *config)
{
    if (!eli_sdl3_renderer_init(renderer))
        return false;
    if (!config)
        return true;
    g_eli_sdl3_renderer.config = *config;
    if (config->vsync != ELI_SDL3_VSYNC_KEEP) {
        int value = config->vsync == ELI_SDL3_VSYNC_ON ? 1 : config->vsync == ELI_SDL3_VSYNC_ADAPTIVE ? -1 : 0;
        if (!SDL_SetRenderVSync(renderer, value))
            SDL_Log("eli_sdl3: SDL_SetRenderVSync(%d) failed: %s", value, SDL_GetError());
    }
    void *layer = SDL_GetRenderMetalLayer(renderer);
    g_eli_sdl3_renderer.metal_layer = layer;
    if (config->drawable_count && layer && !eli_sdl3_metal_set_drawable_count(layer, config->drawable_count))
        SDL_Log("eli_sdl3: drawable count %d refused (Metal takes 2 or 3)", config->drawable_count);
    eli_sdl3__surface_update();
    return true;
}

/**
 * Follow the window's visibility for the swapchain-release policy of
 * eli_sdl3_renderer_init_ex. Feed every SDL event (cheap: only the renderer
 * window's own window events do anything).
 *
 * @param e  The event.
 *
 * Thread-safe: no (main thread)  Reentrant: no
 */
static inline void eli_sdl3_renderer_process_event(const SDL_Event *e)
{
    eli_sdl3_renderer_state *st = &g_eli_sdl3_renderer;
    if (!e || !st->metal_layer || e->type < SDL_EVENT_WINDOW_FIRST || e->type > SDL_EVENT_WINDOW_LAST)
        return;
    SDL_Window *window = SDL_GetRenderWindow(st->renderer);
    if (!window || e->window.windowID != SDL_GetWindowID(window))
        return;
    eli_sdl3__surface_update();
}

/** @return true while the swapchain is released (render_draw_data draws nothing then). */
static inline bool eli_sdl3_renderer_surface_released(void)
{
    return g_eli_sdl3_renderer.surface_released;
}

/** @return the Metal drawable count in effect, or 0 on renderers without a CAMetalLayer. */
static inline int eli_sdl3_renderer_drawable_count(void)
{
    SDL_Renderer *r = g_eli_sdl3_renderer.renderer;
    return r ? eli_sdl3_metal_drawable_count(SDL_GetRenderMetalLayer(r)) : 0;
}

/* ---------------------------------------------------------------------------
 * Textures
 * ------------------------------------------------------------------------- */

/**
 * Create a static RGBA32 texture (straight alpha, linear filtering).
 *
 * @param w, h    Size in texels.
 * @param pixels  w*h RGBA bytes (R first in memory), or NULL for uninitialized.
 * @return        Texture id (an SDL_Texture *), or 0 on failure (logged).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline eli_texture_id eli_sdl3_texture_create_rgba(int w, int h, const void *pixels)
{
    SDL_Texture *tex = SDL_CreateTexture(g_eli_sdl3_renderer.renderer, SDL_PIXELFORMAT_RGBA32,
                                         SDL_TEXTUREACCESS_STATIC, w, h);
    if (!tex) {
        SDL_Log("eli_sdl3: SDL_CreateTexture(%dx%d) failed: %s", w, h, SDL_GetError());
        return 0;
    }
    if (pixels && !SDL_UpdateTexture(tex, NULL, pixels, w * 4))
        SDL_Log("eli_sdl3: SDL_UpdateTexture failed: %s", SDL_GetError());
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);
    return (eli_texture_id)(uintptr_t)tex;
}

/**
 * Update a region of a texture from RGBA32 pixels.
 *
 * @param id      Texture from eli_sdl3_texture_create_rgba.
 * @param rect    Region in texels, or NULL for the whole texture.
 * @param pixels  RGBA bytes for the region.
 * @param pitch   Bytes per row of `pixels`.
 * @return        true on success.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_sdl3_texture_update_rgba(eli_texture_id id, const SDL_Rect *rect,
                                                const void *pixels, int pitch)
{
    SDL_Texture *tex = (SDL_Texture *)(uintptr_t)id;
    if (!tex || !pixels)
        return false;
    return SDL_UpdateTexture(tex, rect, pixels, pitch);
}

/**
 * Destroy a texture created by eli_sdl3_texture_create_rgba (0 is ignored).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_sdl3_texture_destroy(eli_texture_id id)
{
    if (id)
        SDL_DestroyTexture((SDL_Texture *)(uintptr_t)id);
}

/** Destroy the font texture uploaded by eli_sdl3_renderer_create_fonts_texture. */
static inline void eli_sdl3_renderer_destroy_fonts_texture(void)
{
    eli_sdl3_texture_destroy((eli_texture_id)(uintptr_t)g_eli_sdl3_renderer.font_texture);
    g_eli_sdl3_renderer.font_texture = NULL;
}

/**
 * Upload a font atlas (building it if needed) and set its texture id. Replaces a
 * previously uploaded atlas texture. CPU pixels are kept; call
 * eli_font_atlas_release_cpu_pixels afterwards to free them.
 *
 * @param atlas  Atlas to upload.
 * @return       true on success.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_sdl3_renderer_create_fonts_texture(eli_font_atlas *atlas)
{
    unsigned char *pixels = NULL;
    int w = 0, h = 0, bpp = 0;
    eli_font_atlas_get_tex_data_as_rgba32(atlas, &pixels, &w, &h, &bpp);
    if (!pixels || w <= 0 || h <= 0)
        return false;
    eli_sdl3_renderer_destroy_fonts_texture();
    eli_texture_id id = eli_sdl3_texture_create_rgba(w, h, pixels);
    if (!id)
        return false;
    g_eli_sdl3_renderer.font_texture = (SDL_Texture *)(uintptr_t)id;
    eli_font_atlas_set_tex_id(atlas, id);
    return true;
}

/** Release the font texture and the color scratch. */
static inline void eli_sdl3_renderer_shutdown(void)
{
    eli_sdl3_renderer_destroy_fonts_texture();
    eli_mem_free(g_eli_sdl3_renderer.colors);
    memset(&g_eli_sdl3_renderer, 0, sizeof(g_eli_sdl3_renderer));
}

/* ---------------------------------------------------------------------------
 * Draw data
 * ------------------------------------------------------------------------- */

/** Expand a list's packed RGBA8 vertex colors into the SDL_FColor scratch. */
static inline bool eli_sdl3__expand_colors(const eli_draw_list *list)
{
    eli_sdl3_renderer_state *st = &g_eli_sdl3_renderer;
    int n = (int)list->vtx_count;
    if (n > st->colors_capacity) {
        int cap = n + n / 2;
        SDL_FColor *grown = (SDL_FColor *)eli_mem_realloc(st->colors, 0, (size_t)cap * sizeof(*grown));
        if (!grown)
            return false;
        st->colors = grown;
        st->colors_capacity = cap;
    }
    const float k = 1.0f / 255.0f;
    for (int i = 0; i < n; i++) {
        eli_col32 c = list->vtx[i].col;
        st->colors[i].r = (float)((c >> 0) & 0xFF) * k;
        st->colors[i].g = (float)((c >> 8) & 0xFF) * k;
        st->colors[i].b = (float)((c >> 16) & 0xFF) * k;
        st->colors[i].a = (float)((c >> 24) & 0xFF) * k;
    }
    return true;
}

/** Clip a command's rect (points) to the display; false if nothing is visible. */
static inline bool eli_sdl3__clip_rect(const eli_draw_cmd *cmd, eli_vec2 display, SDL_Rect *out)
{
    float x0 = cmd->clip_rect.x, y0 = cmd->clip_rect.y;
    float x1 = x0 + cmd->clip_rect.w, y1 = y0 + cmd->clip_rect.h;
    x0 = x0 < 0.0f ? 0.0f : x0;
    y0 = y0 < 0.0f ? 0.0f : y0;
    x1 = x1 > display.x ? display.x : x1;
    y1 = y1 > display.y ? display.y : y1;
    if (x1 <= x0 || y1 <= y0)
        return false;
    out->x = (int)x0;
    out->y = (int)y0;
    out->w = (int)(x1 + 0.999f) - out->x;
    out->h = (int)(y1 + 0.999f) - out->y;
    return true;
}

/** Draw one list's commands (colors already expanded). */
static inline void eli_sdl3__render_list(SDL_Renderer *r, const eli_draw_list *list, eli_vec2 display)
{
    const SDL_FColor *colors = g_eli_sdl3_renderer.colors;
    for (uint32_t ci = 0; ci < list->cmd_count; ci++) {
        const eli_draw_cmd *cmd = &list->cmds[ci];
        if (cmd->user_callback) {
            cmd->user_callback(list, cmd);
            continue;
        }
        SDL_Rect clip;
        if (cmd->elem_count == 0 || !eli_sdl3__clip_rect(cmd, display, &clip))
            continue;
        SDL_SetRenderClipRect(r, &clip);
        uint32_t vo = cmd->vtx_offset;
        const eli_draw_vert *v = &list->vtx[vo];
        SDL_RenderGeometryRaw(r, (SDL_Texture *)(uintptr_t)cmd->texture_id,
                              &v->x, (int)sizeof(*v), &colors[vo], (int)sizeof(*colors),
                              &v->u, (int)sizeof(*v), (int)(list->vtx_count - vo),
                              list->idx + cmd->idx_offset, (int)cmd->elem_count,
                              (int)sizeof(eli_draw_idx));
    }
}

/**
 * Render a frame's draw data with the attached SDL renderer. Saves and restores
 * the renderer's viewport, clip rect and scale. The app clears before and
 * presents after. Draws nothing while the swapchain is released (hidden window).
 *
 * @param dd  Draw data from eli_get_draw_data() (NULL/invalid/zero size: no-op).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_sdl3_renderer_render_draw_data(const eli_draw_data *dd)
{
    SDL_Renderer *r = g_eli_sdl3_renderer.renderer;
    if (!r || !dd || !dd->valid || dd->display_size.x <= 0.0f || dd->display_size.y <= 0.0f ||
        g_eli_sdl3_renderer.surface_released)
        return;
    SDL_Rect old_viewport, old_clip;
    bool had_viewport = SDL_RenderViewportSet(r), had_clip = SDL_RenderClipEnabled(r);
    float old_sx = 1.0f, old_sy = 1.0f;
    SDL_GetRenderViewport(r, &old_viewport);
    SDL_GetRenderClipRect(r, &old_clip);
    SDL_GetRenderScale(r, &old_sx, &old_sy);

    SDL_SetRenderViewport(r, NULL);
    SDL_SetRenderScale(r, dd->framebuffer_scale.x, dd->framebuffer_scale.y);
    for (int li = 0; li < dd->cmd_lists_count; li++) {
        const eli_draw_list *list = dd->cmd_lists[li];
        if (list->vtx_count == 0 || !eli_sdl3__expand_colors(list))
            continue;
        eli_sdl3__render_list(r, list, dd->display_size);
    }

    SDL_SetRenderScale(r, old_sx, old_sy);
    SDL_SetRenderViewport(r, had_viewport ? &old_viewport : NULL);
    SDL_SetRenderClipRect(r, had_clip ? &old_clip : NULL);
}

/**
 * FNV-1a 64-bit hash of a frame's draw data (vertices, indices, commands, display
 * size). Optional present-skip check (D6b): if it equals the previous frame's hash
 * the app may skip render + present. Costs a pass over the geometry; off unless
 * the app calls it.
 *
 * @param dd  Draw data (NULL/invalid hashes to 0).
 * @return    Hash value.
 *
 * Thread-safe: yes (read-only)  Reentrant: yes
 */
static inline uint64_t eli_sdl3_draw_data_hash(const eli_draw_data *dd)
{
    uint64_t h = 1469598103934665603ull;
    if (!dd || !dd->valid)
        return 0;
#define ELI_SDL3_FNV(ptr, n)                                                    \
    do {                                                                        \
        const unsigned char *b_ = (const unsigned char *)(ptr);                 \
        for (size_t i_ = 0; i_ < (size_t)(n); i_++)                             \
            h = (h ^ b_[i_]) * 1099511628211ull;                                \
    } while (0)
    ELI_SDL3_FNV(&dd->display_size, sizeof(dd->display_size));
    for (int li = 0; li < dd->cmd_lists_count; li++) {
        const eli_draw_list *l = dd->cmd_lists[li];
        ELI_SDL3_FNV(l->vtx, (size_t)l->vtx_count * sizeof(*l->vtx));
        ELI_SDL3_FNV(l->idx, (size_t)l->idx_count * sizeof(*l->idx));
        for (uint32_t ci = 0; ci < l->cmd_count; ci++) {
            const eli_draw_cmd *c = &l->cmds[ci];
            ELI_SDL3_FNV(&c->clip_rect, sizeof(c->clip_rect));
            ELI_SDL3_FNV(&c->texture_id, sizeof(c->texture_id));
            ELI_SDL3_FNV(&c->elem_count, sizeof(c->elem_count));
        }
    }
#undef ELI_SDL3_FNV
    return h;
}

#endif /* ELI_SDL3_RENDERER_H */
