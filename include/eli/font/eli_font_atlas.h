/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_font_atlas.h
 * @brief Font atlas: lifecycle, font sources (including merge-mode fallback
 *        chains), the up-front bake into one alpha8 page, the on-demand setup,
 *        and the texture data accessors.
 *
 * A baked atlas gathers glyph boxes for every configured source (merge sources
 * only for codepoints their font still lacks), packs them plus one white block
 * with the skyline packer into page 0, rasterizes them and fills each font's
 * glyph table. An on-demand atlas (ELI_FONT_ATLAS_ON_DEMAND) only opens the
 * faces, fills font metrics and creates page 0 with the white block; glyphs are
 * rasterized on first lookup (eli_font_atlas_on_demand.h).
 *
 * @status Phase 3 atlas build; pages, on-demand and merge chains.
 * @issues None
 * @todo None
 */
#ifndef ELI_FONT_ELI_FONT_ATLAS_H
#define ELI_FONT_ELI_FONT_ATLAS_H

#include "eli_font_types.h"
#include "eli_font_proggy.h"
#include "eli_font_glyph_ranges.h"
#include "eli_font_stb.h"
#include "eli_font_glyph_table.h"
#include "eli_font_atlas_pages.h"
#include "eli_font_atlas_on_demand.h"

#include "../core/eli_platform.h"
#include "../core/eli_types.h"

/** Round up to the next power of two (>= 1). */
static inline int eli_font__next_pow2(int v)
{
    int p = 1;
    while (p < v)
        p <<= 1;
    return p;
}

/** Count the codepoints covered by a zero-terminated [lo, hi] range list. */
static inline int eli_font__count_range_glyphs(const uint16_t *ranges)
{
    int n = 0;
    for (; ranges[0] && ranges[1]; ranges += 2)
        n += (int)ranges[1] - (int)ranges[0] + 1;
    return n;
}

/* ---------------------------------------------------------------------------
 * Atlas lifecycle
 * ------------------------------------------------------------------------- */

/** Initialize an atlas in place with default padding and no fonts. */
static inline void eli_font_atlas_init(eli_font_atlas *atlas)
{
    memset(atlas, 0, sizeof(*atlas));
    atlas->tex_glyph_padding = ELI_FONT_ATLAS_DEFAULT_PADDING;
    atlas->tex_uv_white_pixel = eli_make_vec2(0.0f, 0.0f);
    atlas->tex_uv_scale = eli_make_vec2(0.0f, 0.0f);
}

/** Allocate and initialize a heap atlas; NULL on allocation failure. */
static inline eli_font_atlas *eli_font_atlas_create(void)
{
    eli_font_atlas *atlas = (eli_font_atlas *)calloc(1, sizeof(*atlas));
    if (atlas)
        eli_font_atlas_init(atlas);
    return atlas;
}

/**
 * Free every page (destroying page textures through the backend hooks), the
 * RGBA copy and the rasterizer sources, and mark the atlas unbuilt.
 */
static inline void eli_font_atlas_clear_tex_data(eli_font_atlas *atlas)
{
    if (!atlas)
        return;
    eli_texture_id legacy_id = atlas->tex_id;
    eli_font__pages_free_all(atlas);
    atlas->tex_id = atlas->backend.create_texture ? 0 : legacy_id; /* app-set id survives */
    free(atlas->tex_pixels_rgba32);
    atlas->tex_pixels_rgba32 = NULL;
    eli_font__sources_free(atlas);
    atlas->built = false;
}

/**
 * Free the atlas's CPU-side pixel buffers (alpha8 and rgba32) after the texture
 * has been uploaded to the GPU, WITHOUT marking the atlas unbuilt: fonts, glyphs
 * and UVs stay valid. get_tex_data_* then returns NULL pixels until the next
 * eli_font_atlas_build (needed again after a density change). A no-op on an
 * on-demand atlas, whose pages receive new glyphs.
 *
 * @param atlas  Target atlas (NULL is ignored).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_font_atlas_release_cpu_pixels(eli_font_atlas *atlas)
{
    if (!atlas || eli_font__on_demand(atlas))
        return;
    for (int i = 0; i < atlas->pages_count; i++) {
        free(atlas->pages[i].pixels);
        atlas->pages[i].pixels = NULL;
    }
    free(atlas->tex_pixels_rgba32);
    atlas->tex_pixels_rgba32 = NULL;
    eli_font__sync_page0(atlas);
}

/** Free any TTF source memory the atlas owns (decompressed/embedded blobs). */
static inline void eli_font_atlas_clear_input_data(eli_font_atlas *atlas)
{
    if (!atlas)
        return;
    eli_font__sources_free(atlas); /* they point into the font data */
    for (int i = 0; i < atlas->configs_count; i++) {
        if (atlas->configs[i].font_data_owned_by_atlas) {
            free(atlas->configs[i].font_data);
            atlas->configs[i].font_data = NULL;
            atlas->configs[i].font_data_owned_by_atlas = false;
        }
    }
}

/** Free all fonts and their glyph tables (keeps input/tex handling separate). */
static inline void eli_font_atlas_clear_fonts(eli_font_atlas *atlas)
{
    if (!atlas)
        return;
    for (int i = 0; i < atlas->fonts_count; i++) {
        eli_font__reset(atlas->fonts[i]);
        free(atlas->fonts[i]->custom_ranges);
        free(atlas->fonts[i]);
    }
    free(atlas->fonts);
    atlas->fonts = NULL;
    atlas->fonts_count = 0;
    atlas->fonts_capacity = 0;
}

/** Release everything the atlas owns and reset it to the initialized state. */
static inline void eli_font_atlas_clear(eli_font_atlas *atlas)
{
    if (!atlas)
        return;
    eli_font_atlas_clear_tex_data(atlas);
    eli_font_atlas_clear_input_data(atlas);
    eli_font_atlas_clear_fonts(atlas);
    free(atlas->configs);
    atlas->configs = NULL;
    atlas->configs_count = 0;
    atlas->configs_capacity = 0;
    eli_font_atlas_init(atlas);
}

/** Destroy a heap atlas created with eli_font_atlas_create. */
static inline void eli_font_atlas_destroy(eli_font_atlas *atlas)
{
    if (!atlas)
        return;
    eli_font_atlas_clear(atlas);
    free(atlas);
}

/**
 * @return true once the atlas has been baked (fonts and UVs valid). Stays true
 *         after eli_font_atlas_release_cpu_pixels; clear_tex_data/clear reset it.
 */
static inline bool eli_font_atlas_is_built(const eli_font_atlas *atlas)
{
    return atlas && atlas->built;
}

/** Assign the backend texture identifier of page 0 (the whole baked texture). */
static inline void eli_font_atlas_set_tex_id(eli_font_atlas *atlas, eli_texture_id id)
{
    if (!atlas)
        return;
    atlas->tex_id = id;
    if (atlas->pages_count > 0)
        atlas->pages[0].tex_id = id;
}

/* ---------------------------------------------------------------------------
 * Adding font sources
 * ------------------------------------------------------------------------- */

/** Fill a config with library defaults (13px, 1x oversample, snap, no ranges). */
static inline eli_font_config eli_font_config_default(void)
{
    eli_font_config cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.size_pixels = ELI_FONT_DEFAULT_SIZE;
    cfg.oversample_h = 1;
    cfg.oversample_v = 1;
    cfg.pixel_snap_h = true;
    cfg.rasterizer_density = 1.0f;
    return cfg;
}

/**
 * Append a font source described by `cfg`. A normal config allocates the font it
 * bakes into; a merge_mode config joins the most recently added font as a
 * fallback source (see eli_font_config) and returns that font.
 *
 * @param atlas  Target atlas.
 * @param cfg    Source configuration (copied; must have valid font_data).
 * @return       The (unbaked) font the source bakes into, or NULL on bad input,
 *               allocation failure, or a merge with no font to merge into.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline eli_font *eli_font_atlas_add_font(eli_font_atlas *atlas, const eli_font_config *cfg)
{
    if (!atlas || !cfg || !cfg->font_data || cfg->font_data_size <= 0)
        return NULL;
    if (cfg->merge_mode && atlas->fonts_count == 0)
        return NULL;
    eli_font_config *configs = (eli_font_config *)eli_font__grow(
        atlas->configs, &atlas->configs_capacity, atlas->configs_count + 1,
        sizeof(*atlas->configs));
    if (atlas->configs_capacity < atlas->configs_count + 1)
        return NULL;
    atlas->configs = configs;
    eli_font **fonts = (eli_font **)eli_font__grow(atlas->fonts, &atlas->fonts_capacity,
                                                   atlas->fonts_count + 1, sizeof(*atlas->fonts));
    if (atlas->fonts_capacity < atlas->fonts_count + 1)
        return NULL;
    atlas->fonts = fonts;

    eli_font *font = cfg->merge_mode ? atlas->fonts[atlas->fonts_count - 1]
                                     : (eli_font *)calloc(1, sizeof(*font));
    if (!font)
        return NULL;
    if (!cfg->merge_mode) {
        font->container_atlas = atlas;
        font->fallback_char = ELI_FONT_FALLBACK_CHAR;
        atlas->fonts[atlas->fonts_count++] = font;
    }
    atlas->configs[atlas->configs_count] = *cfg;
    atlas->configs[atlas->configs_count].dst_font = font;
    atlas->configs_count++;
    atlas->built = false;
    return font;
}

/**
 * Add a font from an in-memory TTF/OTF buffer.
 *
 * @param atlas         Target atlas.
 * @param font_data     TTF bytes (not copied; caller keeps them alive until build, or
 *                      for the atlas's lifetime on an on-demand atlas).
 * @param font_data_size Size of font_data in bytes.
 * @param size_pixels   Target pixel height.
 * @param cfg           Optional config template (NULL for defaults).
 * @param glyph_ranges  Optional zero-terminated range list (NULL for default).
 * @return              The new font, or NULL on failure.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline eli_font *eli_font_atlas_add_font_from_memory_ttf(eli_font_atlas *atlas,
                                                                void *font_data, int font_data_size,
                                                                float size_pixels,
                                                                const eli_font_config *cfg,
                                                                const uint16_t *glyph_ranges)
{
    eli_font_config c = cfg ? *cfg : eli_font_config_default();
    c.font_data = font_data;
    c.font_data_size = font_data_size;
    c.size_pixels = size_pixels;
    c.glyph_ranges = glyph_ranges;
    if (c.oversample_h <= 0)
        c.oversample_h = 1;
    if (c.oversample_v <= 0)
        c.oversample_v = 1;
    return eli_font_atlas_add_font(atlas, &c);
}

/**
 * Add a font from stb-compressed TTF data. The blob is decompressed into a
 * buffer the atlas owns and frees on clear.
 *
 * @return The new font, or NULL on failure.
 */
static inline eli_font *
eli_font_atlas_add_font_from_memory_compressed_ttf(eli_font_atlas *atlas,
                                                   const void *compressed_font_data,
                                                   int compressed_font_data_size, float size_pixels,
                                                   const eli_font_config *cfg,
                                                   const uint16_t *glyph_ranges)
{
    (void)compressed_font_data_size;
    if (!atlas || !compressed_font_data)
        return NULL;
    unsigned int size =
        eli_font_stb_decompress_length((const unsigned char *)compressed_font_data);
    unsigned char *buf = (unsigned char *)malloc(size);
    if (!buf)
        return NULL;
    if (eli_font_stb_decompress(buf, (const unsigned char *)compressed_font_data) != size) {
        free(buf);
        return NULL;
    }
    eli_font *font = eli_font_atlas_add_font_from_memory_ttf(atlas, buf, (int)size, size_pixels,
                                                             cfg, glyph_ranges);
    if (!font) {
        free(buf);
        return NULL;
    }
    atlas->configs[atlas->configs_count - 1].font_data_owned_by_atlas = true;
    return font;
}

/**
 * Add the embedded default font (ProggyClean at 13px) to the atlas.
 *
 * @param atlas  Target atlas.
 * @param cfg    Optional config template (NULL for defaults). size_pixels from
 *               the template is honored when > 0.
 * @return       The new font, or NULL on failure.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline eli_font *eli_font_atlas_add_font_default(eli_font_atlas *atlas,
                                                        const eli_font_config *cfg)
{
    eli_font_config c = cfg ? *cfg : eli_font_config_default();
    float size = (c.size_pixels > 0.0f) ? c.size_pixels : ELI_FONT_DEFAULT_SIZE;
    c.oversample_h = 1;
    c.oversample_v = 1;
    c.pixel_snap_h = true;
    const uint16_t *ranges = c.glyph_ranges ? c.glyph_ranges
                                            : eli_font_atlas_get_glyph_ranges_default(atlas);
    return eli_font_atlas_add_font_from_memory_compressed_ttf(
        atlas, eli_font_proggy_compressed_data, ELI_FONT_PROGGY_COMPRESSED_SIZE, size, &c, ranges);
}

/* ---------------------------------------------------------------------------
 * Bake pipeline (up front, one page)
 * ------------------------------------------------------------------------- */

/** Per-glyph working record threaded through the two-pass bake. */
typedef struct eli_font__build_glyph {
    int cfg_index;
    uint32_t codepoint;
    int glyph_index;
    int box_x0, box_y0;
    int w, h;
} eli_font__build_glyph;

/** Scratch shared by the bake passes. */
typedef struct eli_font__bake {
    eli_font__build_glyph *glyphs;
    stbrp_rect *rects;
    int count;
    int white_idx;
    unsigned char *seen; /* per font: 8 KB bitset of codepoints already supplied */
} eli_font__bake;

/** @return the index of `font` in atlas->fonts. */
static inline int eli_font__font_index(const eli_font_atlas *atlas, const eli_font *font)
{
    for (int f = 0; f < atlas->fonts_count; f++)
        if (atlas->fonts[f] == font)
            return f;
    return 0;
}

/** Gather glyph boxes for every config (merge sources skip supplied codepoints). */
static inline void eli_font__gather_glyphs(eli_font_atlas *atlas, eli_font__bake *b, int padding)
{
    const eli_font__source *src = (const eli_font__source *)atlas->sources;
    for (int i = 0; i < atlas->configs_count; i++) {
        eli_font_config *cfg = &atlas->configs[i];
        unsigned char *seen = b->seen + (size_t)eli_font__font_index(atlas, cfg->dst_font) * 8192;
        const uint16_t *ranges = eli_font__config_ranges(atlas, cfg);
        for (; ranges[0] && ranges[1]; ranges += 2) {
            for (uint32_t cp = ranges[0]; cp <= ranges[1]; cp++) {
                if (seen[cp >> 3] & (1u << (cp & 7)))
                    continue;
                int g = stbtt_FindGlyphIndex(&src[i].info, (int)cp);
                if (g == 0)
                    continue;
                seen[cp >> 3] |= (unsigned char)(1u << (cp & 7));
                int x0, y0, x1, y1;
                stbtt_GetGlyphBitmapBox(&src[i].info, g, src[i].scale, src[i].scale, &x0, &y0, &x1,
                                        &y1);
                eli_font__build_glyph *out = &b->glyphs[b->count];
                out->cfg_index = i;
                out->codepoint = cp;
                out->glyph_index = g;
                out->box_x0 = x0;
                out->box_y0 = y0;
                out->w = x1 - x0;
                out->h = y1 - y0;
                b->rects[b->count].id = b->count;
                b->rects[b->count].w = (stbrp_coord)(out->w + padding);
                b->rects[b->count].h = (stbrp_coord)(out->h + padding);
                b->count++;
            }
        }
    }
}

/** Choose an atlas width from the total packed surface area (ImGui heuristic). */
static inline int eli_font__pick_width(const eli_font_atlas *atlas, const stbrp_rect *rects,
                                       int count)
{
    if (atlas->tex_desired_width > 0)
        return atlas->tex_desired_width;
    long surface = 0;
    for (int k = 0; k < count; k++)
        surface += (long)rects[k].w * (long)rects[k].h;
    int s = (int)sqrt((double)surface) + 1;
    if (s >= (int)(4096 * 0.7f))
        return 4096;
    if (s >= (int)(2048 * 0.7f))
        return 2048;
    if (s >= (int)(1024 * 0.7f))
        return 1024;
    return 512;
}

/** Pack the gathered rects into a tex_w-wide strip; returns the used height (0 = fail). */
static inline int eli_font__pack_baked(eli_font_atlas *atlas, eli_font__bake *b, int tex_w)
{
    const int max_h = 1024 * 32;
    stbrp_node *nodes = (stbrp_node *)calloc((size_t)tex_w, sizeof(*nodes));
    if (!nodes)
        return 0;
    stbrp_context ctx;
    stbrp_init_target(&ctx, tex_w, max_h, nodes, tex_w);
    stbrp_pack_rects(&ctx, b->rects, b->count + 1);
    free(nodes);
    int tex_h = 0;
    for (int k = 0; k <= b->count; k++)
        if (b->rects[k].was_packed && b->rects[k].y + b->rects[k].h > tex_h)
            tex_h = b->rects[k].y + b->rects[k].h;
    if (tex_h > 0 && !(atlas->flags & ELI_FONT_ATLAS_NO_POWER_OF_TWO_HEIGHT))
        tex_h = eli_font__next_pow2(tex_h);
    return tex_h;
}

/** Rasterize the packed glyphs and the white block into page 0's pixels. */
static inline void eli_font__rasterize_baked(eli_font_atlas *atlas, const eli_font__bake *b)
{
    eli_font_atlas_page *page = &atlas->pages[0];
    eli_font__source *src = (eli_font__source *)atlas->sources;
    int tex_w = page->width, tex_h = page->height;
    for (int k = 0; k < b->count; k++) {
        const eli_font__build_glyph *g = &b->glyphs[k];
        if (!b->rects[k].was_packed || g->w <= 0 || g->h <= 0)
            continue;
        eli_font__source *s = &src[g->cfg_index];
        stbtt_MakeGlyphBitmap(&s->info,
                              page->pixels + (size_t)b->rects[k].y * tex_w + b->rects[k].x, g->w,
                              g->h, tex_w, s->scale, s->scale, g->glyph_index);
    }
    const stbrp_rect *white = &b->rects[b->white_idx];
    if (white->was_packed) {
        for (int dy = 0; dy < ELI_FONT_ATLAS_WHITE_SIZE; dy++)
            for (int dx = 0; dx < ELI_FONT_ATLAS_WHITE_SIZE; dx++)
                page->pixels[(size_t)(white->y + dy) * tex_w + (white->x + dx)] = 0xFF;
        atlas->tex_uv_white_pixel =
            eli_make_vec2((white->x + 1.0f) / (float)tex_w, (white->y + 1.0f) / (float)tex_h);
    }
}

/** Fill every font's metrics and glyph table from the bake results. */
static inline void eli_font__emit_font_glyphs(eli_font_atlas *atlas, const eli_font__bake *b)
{
    for (int f = 0; f < atlas->fonts_count; f++)
        eli_font__init_metrics(atlas, atlas->fonts[f]);
    for (int k = 0; k < b->count; k++) {
        const eli_font__build_glyph *g = &b->glyphs[k];
        eli_font__placement pl = { g->box_x0, g->box_y0, g->w, g->h, 0, b->rects[k].x,
                                   b->rects[k].y, b->rects[k].was_packed != 0 };
        eli_font *font = atlas->configs[g->cfg_index].dst_font;
        eli_font_glyph gl = eli_font__compose_glyph(atlas, g->cfg_index, font, g->codepoint,
                                                    g->glyph_index, &pl);
        eli_font_add_glyph(font, &gl);
    }
    for (int f = 0; f < atlas->fonts_count; f++)
        eli_font__finish(atlas->fonts[f]);
}

/** Bake every configured source into page 0 (sources already open). */
static inline bool eli_font__build_baked(eli_font_atlas *atlas)
{
    int padding = eli_font__padding(atlas);
    int total_possible = 0;
    for (int i = 0; i < atlas->configs_count; i++)
        total_possible += eli_font__count_range_glyphs(
            eli_font__config_ranges(atlas, &atlas->configs[i]));

    bool ok = false;
    eli_font__bake b;
    memset(&b, 0, sizeof(b));
    b.glyphs = (eli_font__build_glyph *)calloc((size_t)total_possible + 1, sizeof(*b.glyphs));
    b.rects = (stbrp_rect *)calloc((size_t)total_possible + 1, sizeof(*b.rects));
    b.seen = (unsigned char *)calloc((size_t)atlas->fonts_count, 8192);
    int slot = eli_font__page_alloc_slot(atlas);
    if (!b.glyphs || !b.rects || !b.seen || slot != 0)
        goto cleanup;

    eli_font__gather_glyphs(atlas, &b, padding);
    b.white_idx = b.count;
    b.rects[b.white_idx].id = -1;
    b.rects[b.white_idx].w = (stbrp_coord)(ELI_FONT_ATLAS_WHITE_SIZE + padding);
    b.rects[b.white_idx].h = (stbrp_coord)(ELI_FONT_ATLAS_WHITE_SIZE + padding);

    int tex_w = eli_font__pick_width(atlas, b.rects, b.count + 1);
    int tex_h = eli_font__pack_baked(atlas, &b, tex_w);
    eli_font_atlas_page *page = &atlas->pages[0];
    page->pixels = tex_h > 0 ? (unsigned char *)calloc((size_t)tex_w * (size_t)tex_h, 1) : NULL;
    if (!page->pixels)
        goto cleanup;
    page->width = tex_w;
    page->height = tex_h;
    page->live = true;
    page->version = 1;
    eli_font__rasterize_baked(atlas, &b);
    eli_font__emit_font_glyphs(atlas, &b);
    ok = true;

cleanup:
    free(b.glyphs);
    free(b.rects);
    free(b.seen);
    return ok;
}

/** Set up an on-demand atlas: page 0 with the white block, font metrics only. */
static inline bool eli_font__build_on_demand(eli_font_atlas *atlas)
{
    if (eli_font__page_new_on_demand(atlas) != 0)
        return false;
    float uv = 1.0f / (float)eli_font__page_size(atlas);
    atlas->tex_uv_white_pixel = eli_make_vec2(uv, uv);
    for (int f = 0; f < atlas->fonts_count; f++) {
        eli_font__init_metrics(atlas, atlas->fonts[f]);
        eli_font__finish(atlas->fonts[f]);
    }
    return true;
}

/**
 * Build the atlas: bake every configured source up front into page 0, or on an
 * on-demand atlas open the faces and create page 0 (glyphs load on first use).
 * Implicitly adds the default font if none were configured. Re-buildable (frees
 * prior pages and glyphs; font pointers stay valid).
 *
 * @param atlas  Target atlas.
 * @return       true on success, false on empty/failed build.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_font_atlas_build(eli_font_atlas *atlas)
{
    if (!atlas)
        return false;
    if (atlas->configs_count == 0 && eli_font_atlas_add_font_default(atlas, NULL) == NULL)
        return false;
    eli_texture_id legacy_id = atlas->tex_id;
    eli_font_atlas_clear_tex_data(atlas);
    if (!eli_font__sources_init(atlas))
        return false;
    bool ok = eli_font__on_demand(atlas) ? eli_font__build_on_demand(atlas)
                                         : eli_font__build_baked(atlas);
    if (!ok) {
        eli_font_atlas_clear_tex_data(atlas);
        return false;
    }
    if (!eli_font__on_demand(atlas))
        eli_font__sources_free(atlas); /* every glyph is baked */
    if (atlas->backend.create_texture)
        eli_font__page_create_texture(atlas, &atlas->pages[0]);
    else
        atlas->pages[0].tex_id = legacy_id; /* an app-assigned id carries over */
    eli_font__sync_page0(atlas);
    for (int f = 0; f < atlas->fonts_count; f++)
        atlas->fonts[f]->tex_uv_white_pixel = atlas->tex_uv_white_pixel;
    atlas->built = true;
    return true;
}

/* ---------------------------------------------------------------------------
 * Texture data accessors (build on demand)
 * ------------------------------------------------------------------------- */

/**
 * Fetch page 0 as single-channel (alpha8) data, building the atlas if needed. On
 * a baked atlas page 0 is the whole texture.
 *
 * @param atlas           Target atlas.
 * @param out_pixels      Receives the alpha8 pixel pointer (may be NULL).
 * @param out_width       Receives texture width (may be NULL).
 * @param out_height      Receives texture height (may be NULL).
 * @param out_bytes_per_pixel Receives 1 (may be NULL).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_font_atlas_get_tex_data_as_alpha8(eli_font_atlas *atlas,
                                                         unsigned char **out_pixels, int *out_width,
                                                         int *out_height, int *out_bytes_per_pixel)
{
    if (!atlas)
        return;
    if (!eli_font_atlas_is_built(atlas))
        eli_font_atlas_build(atlas);
    if (out_pixels)
        *out_pixels = atlas->tex_pixels_alpha8;
    if (out_width)
        *out_width = atlas->tex_width;
    if (out_height)
        *out_height = atlas->tex_height;
    if (out_bytes_per_pixel)
        *out_bytes_per_pixel = 1;
}

/**
 * Fetch page 0 as RGBA32 (white with per-texel alpha), building/converting on
 * demand. The RGBA buffer is cached and owned by the atlas.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_font_atlas_get_tex_data_as_rgba32(eli_font_atlas *atlas,
                                                         unsigned char **out_pixels, int *out_width,
                                                         int *out_height, int *out_bytes_per_pixel)
{
    if (!atlas)
        return;
    if (!eli_font_atlas_is_built(atlas))
        eli_font_atlas_build(atlas);
    if (!atlas->tex_pixels_rgba32 && atlas->tex_pixels_alpha8) {
        size_t n = (size_t)atlas->tex_width * (size_t)atlas->tex_height;
        atlas->tex_pixels_rgba32 = (unsigned int *)malloc(n * sizeof(unsigned int));
        if (atlas->tex_pixels_rgba32) {
            for (size_t k = 0; k < n; k++) {
                unsigned int a = atlas->tex_pixels_alpha8[k];
                atlas->tex_pixels_rgba32[k] = (a << 24) | 0x00FFFFFFu;
            }
        }
    }
    if (out_pixels)
        *out_pixels = (unsigned char *)atlas->tex_pixels_rgba32;
    if (out_width)
        *out_width = atlas->tex_width;
    if (out_height)
        *out_height = atlas->tex_height;
    if (out_bytes_per_pixel)
        *out_bytes_per_pixel = 4;
}

#endif /* ELI_FONT_ELI_FONT_ATLAS_H */
