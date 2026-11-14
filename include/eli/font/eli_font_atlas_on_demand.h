/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_font_atlas_on_demand.h
 * @brief Per-config rasterizer sources, glyph composition shared by the baked
 *        and on-demand paths (merge-mode baseline and cell-advance fit), font
 *        metrics, and the on-demand glyph loader.
 *
 * A font's sources are the configs whose dst_font is that font, in the order they
 * were added: the primary face first, then merge-mode fallbacks. The first source
 * whose ranges include a codepoint and whose face has it supplies the glyph.
 * Merged glyphs sit on the font's baseline; with merge_match_advance they take
 * the font's cell advance (its 'M'), centred and shrunk about their centre when
 * wider, so monospace columns never shift.
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#ifndef ELI_FONT_ELI_FONT_ATLAS_ON_DEMAND_H
#define ELI_FONT_ELI_FONT_ATLAS_ON_DEMAND_H

#include "eli_font_types.h"
#include "eli_font_stb.h"
#include "eli_font_glyph_table.h"
#include "eli_font_glyph_ranges.h"
#include "eli_font_atlas_pages.h"

#include "../core/eli_platform.h"
#include "../core/eli_types.h"

/** One config's opened face and scale. */
typedef struct eli_font__source {
    stbtt_fontinfo info;
    float scale;      /* font units -> texels */
    float ascent_pt;  /* rounded ascent in points */
    float descent_pt; /* rounded descent in points */
} eli_font__source;

/* ---------------------------------------------------------------------------
 * Sources
 * ------------------------------------------------------------------------- */

/** @return a config's rasterizer density, treating <= 0 as 1. */
static inline float eli_font__density(const eli_font_config *cfg)
{
    return (cfg->rasterizer_density > 0.0f) ? cfg->rasterizer_density : 1.0f;
}

/** Free the per-config sources. */
static inline void eli_font__sources_free(eli_font_atlas *atlas)
{
    free(atlas->sources);
    atlas->sources = NULL;
}

/** Open every config's face; false (nothing kept) if one cannot be opened. */
static inline bool eli_font__sources_init(eli_font_atlas *atlas)
{
    eli_font__sources_free(atlas);
    eli_font__source *src = (eli_font__source *)calloc((size_t)atlas->configs_count,
                                                       sizeof(*src));
    if (!src)
        return false;
    for (int i = 0; i < atlas->configs_count; i++) {
        const eli_font_config *cfg = &atlas->configs[i];
        const unsigned char *data = (const unsigned char *)cfg->font_data;
        int off = stbtt_GetFontOffsetForIndex(data, cfg->font_no);
        if (off < 0 || !stbtt_InitFont(&src[i].info, data, off)) {
            free(src);
            return false;
        }
        src[i].scale = stbtt_ScaleForPixelHeight(&src[i].info,
                                                 cfg->size_pixels * eli_font__density(cfg));
        int ascent, descent, gap;
        stbtt_GetFontVMetrics(&src[i].info, &ascent, &descent, &gap);
        float pt_scale = src[i].scale / eli_font__density(cfg); /* font units -> points */
        src[i].ascent_pt = (float)((int)(ascent * pt_scale + 0.5f));
        src[i].descent_pt = (float)((int)(descent * pt_scale - 0.5f));
    }
    atlas->sources = src;
    return true;
}

/** @return a config's ranges: its own, else the default (baked) or NULL = all (on demand). */
static inline const uint16_t *eli_font__config_ranges(eli_font_atlas *atlas,
                                                      const eli_font_config *cfg)
{
    if (cfg->glyph_ranges)
        return cfg->glyph_ranges;
    return eli_font__on_demand(atlas) ? NULL : eli_font_atlas_get_glyph_ranges_default(atlas);
}

/** @return true when `cp` lies in a zero-terminated [lo, hi] range list (NULL = all). */
static inline bool eli_font__ranges_contain(const uint16_t *ranges, uint32_t cp)
{
    if (!ranges)
        return true;
    for (; ranges[0] && ranges[1]; ranges += 2)
        if (cp >= ranges[0] && cp <= ranges[1])
            return true;
    return false;
}

/** @return the index of the font's primary (first) config, or -1. */
static inline int eli_font__primary_config(const eli_font_atlas *atlas, const eli_font *font)
{
    for (int i = 0; i < atlas->configs_count; i++)
        if (atlas->configs[i].dst_font == font)
            return i;
    return -1;
}

/* ---------------------------------------------------------------------------
 * Glyph composition
 * ------------------------------------------------------------------------- */

/** @return a glyph's advance in points: snapped and clamped per its config. */
static inline float eli_font__advance(const eli_font__source *src, const eli_font_config *cfg,
                                      int glyph_index)
{
    int adv, lsb;
    stbtt_GetGlyphHMetrics(&src->info, glyph_index, &adv, &lsb);
    float advance = adv * src->scale / eli_font__density(cfg);
    if (cfg->pixel_snap_h)
        advance = (float)((int)(advance + 0.5f));
    if (cfg->glyph_min_advance_x > 0.0f && advance < cfg->glyph_min_advance_x)
        advance = cfg->glyph_min_advance_x;
    if (cfg->glyph_max_advance_x > 0.0f && advance > cfg->glyph_max_advance_x)
        advance = cfg->glyph_max_advance_x;
    return advance;
}

/** Fit a merged glyph into the font's cell: centred, shrunk about its centre if wider. */
static inline void eli_font__fit_cell(eli_font_glyph *g, float cell)
{
    float w = g->x1 - g->x0;
    if (w > cell) {
        float k = cell / w, cy = (g->y0 + g->y1) * 0.5f;
        g->y0 = cy + (g->y0 - cy) * k;
        g->y1 = cy + (g->y1 - cy) * k;
        w = cell;
    }
    g->x0 = (cell - w) * 0.5f;
    g->x1 = g->x0 + w;
    g->advance_x = cell;
}

/** Where a glyph's bitmap landed (packed false: no bitmap or no room). */
typedef struct eli_font__placement {
    int box_x0, box_y0, w, h; /* bitmap box from stb, texels */
    int page, x, y;
    bool packed;
} eli_font__placement;

/** Build the glyph record for config `ci`'s glyph `glyph_index` (codepoint `cp`). */
static inline eli_font_glyph eli_font__compose_glyph(const eli_font_atlas *atlas, int ci,
                                                     const eli_font *font, uint32_t cp,
                                                     int glyph_index,
                                                     const eli_font__placement *pl)
{
    const eli_font_config *cfg = &atlas->configs[ci];
    const eli_font__source *src = &((const eli_font__source *)atlas->sources)[ci];
    float density = eli_font__density(cfg);
    eli_font_glyph gl;
    memset(&gl, 0, sizeof(gl));
    gl.codepoint = cp;
    gl.kind = ELI_FONT_GLYPH_ATLAS;
    gl.advance_x = eli_font__advance(src, cfg, glyph_index);
    if (pl->packed && pl->w > 0 && pl->h > 0) {
        const eli_font_atlas_page *page = &atlas->pages[pl->page];
        float us = 1.0f / (float)page->width, vs = 1.0f / (float)page->height;
        gl.visible = true;
        gl.page = (uint16_t)pl->page;
        /* Bitmap boxes are in texels; quads are in points (texels / density). */
        gl.x0 = (float)pl->box_x0 / density + cfg->glyph_offset_x;
        gl.y0 = src->ascent_pt + (float)pl->box_y0 / density + cfg->glyph_offset_y;
        gl.x1 = gl.x0 + (float)pl->w / density;
        gl.y1 = gl.y0 + (float)pl->h / density;
        gl.u0 = pl->x * us;
        gl.v0 = pl->y * vs;
        gl.u1 = (pl->x + pl->w) * us;
        gl.v1 = (pl->y + pl->h) * vs;
    }
    if (cfg->merge_mode) {
        float dy = font->ascent - src->ascent_pt; /* glyph y is measured from the line top */
        gl.y0 += dy;
        gl.y1 += dy;
        if (cfg->merge_match_advance && font->cell_advance_x > 0.0f)
            eli_font__fit_cell(&gl, font->cell_advance_x);
    }
    return gl;
}

/* ---------------------------------------------------------------------------
 * Font metrics
 * ------------------------------------------------------------------------- */

/** @return the advance of `cp` in config `ci` (0 if outside its ranges or absent). */
static inline float eli_font__config_advance_of(eli_font_atlas *atlas, int ci, uint32_t cp)
{
    const eli_font_config *cfg = &atlas->configs[ci];
    const eli_font__source *src = &((const eli_font__source *)atlas->sources)[ci];
    if (!eli_font__ranges_contain(eli_font__config_ranges(atlas, cfg), cp))
        return 0.0f;
    int g = stbtt_FindGlyphIndex(&src->info, (int)cp);
    return g ? eli_font__advance(src, cfg, g) : 0.0f;
}

/**
 * Reset a font and fill its metrics from its primary config: size, ascent,
 * descent, line height, cell advance ('M') and the tofu box (as wide as '0').
 */
static inline void eli_font__init_metrics(eli_font_atlas *atlas, eli_font *font)
{
    eli_font__reset(font);
    font->container_atlas = atlas;
    int ci = eli_font__primary_config(atlas, font);
    if (ci < 0)
        return;
    const eli_font_config *cfg = &atlas->configs[ci];
    const eli_font__source *src = &((const eli_font__source *)atlas->sources)[ci];
    font->font_size = cfg->size_pixels;
    font->ascent = src->ascent_pt;
    font->descent = src->descent_pt;
    font->line_height = cfg->size_pixels;
    font->tex_uv_white_pixel = atlas->tex_uv_white_pixel;
    font->cell_advance_x = eli_font__config_advance_of(atlas, ci, 'M');

    float tofu = eli_font__config_advance_of(atlas, ci, '0');
    if (tofu <= 0.0f)
        tofu = cfg->pixel_snap_h ? (float)((int)(cfg->size_pixels * 0.5f + 0.5f))
                                 : cfg->size_pixels * 0.5f;
    memset(&font->tofu_glyph, 0, sizeof(font->tofu_glyph));
    font->tofu_glyph.kind = ELI_FONT_GLYPH_TOFU;
    font->tofu_glyph.advance_x = tofu;
    eli_font__resolve_fallback(font);
}

/** Finish a font after its glyphs are in: custom ranges, then the fallback. */
static inline void eli_font__finish(eli_font *font)
{
    eli_font__apply_custom_ranges(font);
    eli_font__resolve_fallback(font);
}

/* ---------------------------------------------------------------------------
 * On-demand loading
 * ------------------------------------------------------------------------- */

/** Rasterize config `ci`'s glyph into a page; fills the placement. */
static inline void eli_font__rasterize_on_demand(eli_font_atlas *atlas, int ci, int glyph_index,
                                                 eli_font__placement *pl)
{
    eli_font__source *src = &((eli_font__source *)atlas->sources)[ci];
    int x0, y0, x1, y1;
    stbtt_GetGlyphBitmapBox(&src->info, glyph_index, src->scale, src->scale, &x0, &y0, &x1, &y1);
    memset(pl, 0, sizeof(*pl));
    pl->box_x0 = x0;
    pl->box_y0 = y0;
    pl->w = x1 - x0;
    pl->h = y1 - y0;
    if (pl->w <= 0 || pl->h <= 0)
        return;
    int pad = eli_font__padding(atlas);
    if (!eli_font__atlas_place(atlas, pl->w + pad, pl->h + pad, &pl->page, &pl->x, &pl->y))
        return;
    eli_font_atlas_page *p = &atlas->pages[pl->page];
    stbtt_MakeGlyphBitmap(&src->info, p->pixels + (size_t)pl->y * (size_t)p->width + pl->x,
                          pl->w, pl->h, p->width, src->scale, src->scale, glyph_index);
    eli_font__page_mark_dirty(p, pl->x, pl->y, pl->x + pl->w, pl->y + pl->h);
    p->glyph_count++;
    p->last_used_frame = atlas->frame_count;
    pl->packed = true;
}

/* Declared in eli_font_glyph_table.h. */
static inline int eli_font__load_glyph(eli_font *font, uint32_t codepoint)
{
    eli_font_atlas *atlas = font->container_atlas;
    for (int ci = 0; ci < atlas->configs_count; ci++) {
        const eli_font_config *cfg = &atlas->configs[ci];
        if (cfg->dst_font != font ||
            !eli_font__ranges_contain(eli_font__config_ranges(atlas, cfg), codepoint))
            continue;
        eli_font__source *src = &((eli_font__source *)atlas->sources)[ci];
        int glyph_index = stbtt_FindGlyphIndex(&src->info, (int)codepoint);
        if (glyph_index == 0)
            continue;
        eli_font__placement pl;
        eli_font__rasterize_on_demand(atlas, ci, glyph_index, &pl);
        eli_font_glyph g = eli_font__compose_glyph(atlas, ci, font, codepoint, glyph_index, &pl);
        int idx = eli_font_add_glyph(font, &g);
        return idx >= 0 ? idx : ELI_FONT_LOOKUP_MISSING;
    }
    eli_font__lookup_set(font, codepoint, ELI_FONT_LOOKUP_MISSING);
    return ELI_FONT_LOOKUP_MISSING;
}

#endif /* ELI_FONT_ELI_FONT_ATLAS_ON_DEMAND_H */
