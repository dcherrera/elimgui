/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_live_text.h
 * @brief CPU rasterizer for live text layers: alpha8 coverage of a live text
 *        widget, sampled from the font atlas pages, at any pixel scale.
 *
 * @status Live widgets core.
 * @issues Tofu and custom glyphs are left blank in the layer.
 * @todo None
 *
 * Backends call eli_live_text_raster on create and on a text change, into a
 * buffer they own, and show it as a mask of the text color. The glyph bitmaps
 * are the ones the main surface draws (nearest sampling). It needs the atlas's
 * CPU pixels: an on-demand atlas keeps them; a baked atlas loses them after
 * eli_font_atlas_release_cpu_pixels, and live text then uses the fallback.
 */
#ifndef ELI_LIVE_TEXT_H
#define ELI_LIVE_TEXT_H

#include "eli_live_types.h"
#include "../font/eli_font.h"

/**
 * @return true when text in `font` can be rasterized on the CPU (its atlas keeps
 *         page pixels).
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline bool eli_live_text_can_raster(const eli_font *font)
{
    const eli_font_atlas *atlas = font ? font->container_atlas : NULL;
    return atlas && atlas->pages_count > 0 && atlas->pages[0].pixels != NULL;
}

/** Copy one glyph's texels into `dst` (nearest sampling, max-combined). */
static inline bool eli_live__blit_glyph(const eli_font_atlas *atlas, const eli_font_glyph *g,
                                        float x0, float y0, float x1, float y1,
                                        unsigned char *dst, int width, int height, int stride)
{
    if (g->page >= atlas->pages_count)
        return false;
    const eli_font_atlas_page *page = &atlas->pages[g->page];
    if (!page->pixels)
        return false;
    int dx0 = (int)floorf(x0 + 0.5f), dy0 = (int)floorf(y0 + 0.5f);
    int dx1 = (int)floorf(x1 + 0.5f), dy1 = (int)floorf(y1 + 0.5f);
    if (dx1 <= dx0 || dy1 <= dy0)
        return true;
    float su0 = g->u0 * (float)page->width, sv0 = g->v0 * (float)page->height;
    float su = (g->u1 - g->u0) * (float)page->width / (float)(dx1 - dx0);
    float sv = (g->v1 - g->v0) * (float)page->height / (float)(dy1 - dy0);
    for (int y = dy0 < 0 ? 0 : dy0; y < dy1 && y < height; y++) {
        int ty = (int)(sv0 + ((float)(y - dy0) + 0.5f) * sv);
        ty = ty < 0 ? 0 : (ty >= page->height ? page->height - 1 : ty);
        const unsigned char *src = page->pixels + (size_t)ty * (size_t)page->width;
        unsigned char *row = dst + (size_t)y * (size_t)stride;
        for (int x = dx0 < 0 ? 0 : dx0; x < dx1 && x < width; x++) {
            int tx = (int)(su0 + ((float)(x - dx0) + 0.5f) * su);
            tx = tx < 0 ? 0 : (tx >= page->width ? page->width - 1 : tx);
            if (src[tx] > row[x])
                row[x] = src[tx];
        }
    }
    return true;
}

/**
 * Rasterize a live text widget's text into an alpha8 buffer covering its rect
 * at `scale` pixels per point (row 0 = top). The text starts at the rect's
 * top-left like the main surface draws it, and is cut at the buffer's edge.
 *
 * @param w       A live text widget (font, font_size, text).
 * @param dst     width x height bytes, `stride` bytes per row; cleared first.
 * @param scale   Pixels per point (the layer's contents scale).
 * @return        false when the atlas has no CPU pixels (dst is then blank).
 *
 * Thread-safe: no (may rasterize on-demand glyphs)  Reentrant: no
 */
static inline bool eli_live_text_raster(const eli_live_widget *w, unsigned char *dst, int width,
                                        int height, int stride, float scale)
{
    if (!dst || width <= 0 || height <= 0 || stride < width)
        return false;
    for (int y = 0; y < height; y++)
        memset(dst + (size_t)y * (size_t)stride, 0, (size_t)width);
    const eli_font *font = w->font;
    if (!eli_live_text_can_raster(font))
        return false;
    const eli_font_atlas *atlas = font->container_atlas;
    float fs = (w->font_size > 0.0f ? w->font_size : font->font_size) / font->font_size;
    float x = 0.0f;
    const char *s = w->text;
    const char *end = s + strlen(s);
    bool ok = true;
    while (s < end && x * scale < (float)width) {
        uint32_t cp = 0;
        s += eli_text_utf8_decode(s, end, &cp);
        if (cp == '\n' || cp == '\r')
            break;
        const eli_font_glyph *g = eli_font_find_glyph(font, cp);
        if (!g)
            continue;
        if (g->visible && g->kind == ELI_FONT_GLYPH_ATLAS)
            ok = eli_live__blit_glyph(atlas, g, (x + g->x0 * fs) * scale, g->y0 * fs * scale,
                                      (x + g->x1 * fs) * scale, g->y1 * fs * scale, dst, width,
                                      height, stride) && ok;
        x += g->advance_x * fs;
    }
    return ok;
}

#endif /* ELI_LIVE_TEXT_H */
