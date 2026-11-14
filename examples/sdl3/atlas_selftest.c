/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file atlas_selftest.c
 * @brief SDL3 example, sixth TU: `--atlas-selftest`, the single-channel font
 *        page check run through the real renderer.
 *
 * Checks: every font page texture is SDL_PIXELFORMAT_INDEX8 (1 byte per texel)
 * when the renderer supports it; and the frame drawn with those pages is
 * identical to the same draw data drawn with white-RGBA32 copies of the pages
 * (the previous upload format), read back from an offscreen target. Prints PASS/FAIL per
 * check and the page bytes saved. Needs the pages' CPU pixels (main keeps them).
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#include "example_app.h"

#include <eli/elimgui.h>
#include <sdl3/eli_sdl3_renderer.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_atlas_selftest_failed;

/** Record one check. */
static void atlas_selftest_check(bool ok, const char *what)
{
    SDL_Log("atlas-selftest: %s %s", ok ? "PASS" : "FAIL", what);
    if (!ok)
        g_atlas_selftest_failed++;
}

/** Draw `dd` into a fresh offscreen target and read it back (NULL on failure). */
static SDL_Surface *atlas_selftest_capture(SDL_Renderer *r, const eli_draw_data *dd)
{
    int w = (int)(dd->display_size.x * dd->framebuffer_scale.x);
    int h = (int)(dd->display_size.y * dd->framebuffer_scale.y);
    SDL_Texture *target = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA32,
                                            SDL_TEXTUREACCESS_TARGET, w, h);
    if (!target)
        return NULL;
    SDL_SetRenderTarget(r, target);
    SDL_SetRenderDrawColor(r, 30, 30, 34, 255);
    SDL_RenderClear(r);
    eli_sdl3_renderer_render_draw_data(dd);
    SDL_Surface *shot = SDL_RenderReadPixels(r, NULL);
    SDL_SetRenderTarget(r, NULL);
    SDL_DestroyTexture(target);
    return shot;
}

/** Point every draw command using texture `from` at `to`. */
static void atlas_selftest_remap(const eli_draw_data *dd, eli_texture_id from, eli_texture_id to)
{
    for (int li = 0; li < dd->cmd_lists_count; li++) {
        eli_draw_list *list = dd->cmd_lists[li];
        for (uint32_t ci = 0; ci < list->cmd_count; ci++)
            if (list->cmds[ci].texture_id == from)
                list->cmds[ci].texture_id = to;
    }
}

/** Check page formats; returns the page bytes as uploaded now. */
static size_t atlas_selftest_formats(SDL_Renderer *r, const eli_font_atlas *atlas)
{
    bool index8 = eli_sdl3__renderer_has_index8(r), all = true;
    size_t bytes = 0;
    for (int i = 0; i < atlas->pages_count; i++) {
        const eli_font_atlas_page *p = &atlas->pages[i];
        SDL_Texture *tex = (SDL_Texture *)(uintptr_t)p->tex_id;
        if (!p->live || !tex)
            continue;
        all = all && (!index8 || tex->format == SDL_PIXELFORMAT_INDEX8);
        bytes += (size_t)tex->w * (size_t)tex->h * SDL_BYTESPERPIXEL(tex->format);
    }
    char what[160];
    snprintf(what, sizeof(what), "font pages are INDEX8 (renderer %s it): %zu bytes on the GPU",
             index8 ? "supports" : "lacks", bytes);
    atlas_selftest_check(all, what);
    return bytes;
}

int example_atlas_selftest(SDL_Renderer *r, eli_font_atlas *atlas, void (*frame)(void))
{
    for (int i = 0; i < 3; i++)
        frame(); /* settle layout, load every glyph the frame draws */
    const eli_draw_data *dd = eli_get_draw_data();
    size_t bytes = atlas_selftest_formats(r, atlas);

    SDL_Surface *a8 = atlas_selftest_capture(r, dd);
    eli_texture_id rgba[64] = { 0 };
    for (int i = 0; i < atlas->pages_count && i < 64; i++) {
        const eli_font_atlas_page *p = &atlas->pages[i];
        if (!p->live || !p->pixels)
            continue;
        uint32_t *px = (uint32_t *)malloc((size_t)p->width * p->height * sizeof(*px));
        for (int k = 0; px && k < p->width * p->height; k++)
            px[k] = ((uint32_t)p->pixels[k] << 24) | 0x00FFFFFFu;
        rgba[i] = px ? eli_sdl3_texture_create_rgba(p->width, p->height, px) : 0;
        free(px);
        atlas_selftest_remap(dd, p->tex_id, rgba[i]);
    }
    SDL_Surface *ref = atlas_selftest_capture(r, dd);
    for (int i = 0; i < atlas->pages_count && i < 64; i++) {
        if (!rgba[i])
            continue;
        atlas_selftest_remap(dd, rgba[i], atlas->pages[i].tex_id);
        eli_sdl3_texture_destroy(rgba[i]);
    }

    long differ = -1;
    int max_diff = 0;
    if (a8 && ref && a8->h == ref->h && a8->pitch == ref->pitch) {
        const unsigned char *x = (const unsigned char *)a8->pixels;
        const unsigned char *y = (const unsigned char *)ref->pixels;
        differ = 0;
        for (size_t k = 0; k < (size_t)a8->pitch * a8->h; k++) {
            int d = abs((int)x[k] - (int)y[k]);
            differ += d != 0;
            max_diff = d > max_diff ? d : max_diff;
        }
    }
    char what[200];
    snprintf(what, sizeof(what), "frame with INDEX8 pages == frame with RGBA32 pages "
             "(%ld bytes differ, max %d; RGBA32 pages would be %zu bytes)", differ, max_diff,
             bytes * 4);
    atlas_selftest_check(differ == 0, what);
    SDL_DestroySurface(a8);
    SDL_DestroySurface(ref);
    SDL_Log("atlas-selftest: %d failure(s)", g_atlas_selftest_failed);
    return g_atlas_selftest_failed;
}
