/**
 * @file eli_font_atlas.h
 * @brief Font atlas: stb_truetype/stb_rect_pack integration, font sources, and
 *        the bake pipeline that rasterizes glyphs into a single alpha8 texture.
 *
 * This is the one translation unit that defines STB_TRUETYPE_IMPLEMENTATION and
 * STB_RECT_PACK_IMPLEMENTATION; every STBTT_/STBRP_ hook is routed through the
 * libc seam (eli_platform.h) so the same code compiles for wasm32 (JAClibc) and
 * for native hosted unit tests. The bake gathers glyph bitmap boxes for every
 * configured font, packs them (plus one white texel) with the skyline packer,
 * rasterizes each glyph, and fills per-font metrics and glyph tables.
 *
 * @status Phase 3 atlas build in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_FONT_ELI_FONT_ATLAS_H
#define ELI_FONT_ELI_FONT_ATLAS_H

#include "eli_font_types.h"
#include "eli_font_proggy.h"
#include "eli_font_glyph_ranges.h"

#include "../core/eli_platform.h"
#include "../core/eli_types.h"

/* Route stb's libc + math hooks through the seam headers (already included via
 * eli_platform.h). Defining these suppresses stb's own <stdlib.h>/<math.h>/etc. */
#define STBTT_malloc(x, u) ((void)(u), malloc(x))
#define STBTT_free(x, u)   ((void)(u), free(x))
#define STBTT_assert(x)    ((void)0)
#define STBTT_strlen(x)    strlen(x)
#define STBTT_memcpy       memcpy
#define STBTT_memset       memset
#define STBTT_ifloor(x)    ((int)floor(x))
#define STBTT_iceil(x)     ((int)ceil(x))
#define STBTT_sqrt(x)      sqrt(x)
#define STBTT_pow(x, y)    pow(x, y)
#define STBTT_fmod(x, y)   fmod(x, y)
#define STBTT_cos(x)       cos(x)
#define STBTT_acos(x)      acos(x)
#define STBTT_fabs(x)      fabs(x)

#define STBRP_SORT      qsort
#define STBRP_ASSERT(x) ((void)0)

/* stb_rect_pack must precede stb_truetype so the latter binds the real packer. */
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb/stb_rect_pack.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

/** Default spacing (in texels) left between packed glyphs. */
#define ELI_FONT_ATLAS_DEFAULT_PADDING 1

/** Side length (texels) of the opaque white block reserved for solid fills. */
#define ELI_FONT_ATLAS_WHITE_SIZE 3

/* ---------------------------------------------------------------------------
 * Small growable-buffer helper (returns NULL on allocation failure)
 * ------------------------------------------------------------------------- */

/** Grow a heap array to hold at least `need` elements; doubles capacity. */
static inline void *eli_font__grow(void *ptr, int *cap, int need, size_t elem)
{
    if (need <= *cap)
        return ptr;
    int new_cap = (*cap > 0) ? (*cap * 2) : 8;
    if (new_cap < need)
        new_cap = need;
    void *p = realloc(ptr, (size_t)new_cap * elem);
    if (p)
        *cap = new_cap;
    return p;
}

/** Round up to the next power of two (>= 1). */
static inline int eli_font__next_pow2(int v)
{
    int p = 1;
    while (p < v)
        p <<= 1;
    return p;
}

/* ---------------------------------------------------------------------------
 * Font glyph table helpers
 * ------------------------------------------------------------------------- */

/** Release a font's glyph array and lookup table, leaving it empty. */
static inline void eli_font__reset(eli_font *font)
{
    free(font->glyphs);
    free(font->index_lookup);
    font->glyphs = NULL;
    font->glyph_count = 0;
    font->glyph_capacity = 0;
    font->index_lookup = NULL;
    font->index_lookup_size = 0;
    font->fallback_glyph = NULL;
    font->fallback_advance_x = 0.0f;
}

/** Look up a glyph by codepoint, returning NULL (not the fallback) if absent. */
static inline const eli_font_glyph *eli_font_find_glyph_no_fallback(const eli_font *font,
                                                                    uint32_t codepoint)
{
    if (!font || codepoint >= (uint32_t)font->index_lookup_size)
        return NULL;
    int idx = font->index_lookup[codepoint];
    if (idx < 0)
        return NULL;
    return &font->glyphs[idx];
}

/**
 * Look up a glyph by codepoint, falling back to the font's fallback glyph.
 *
 * @param font       Baked font.
 * @param codepoint  Unicode codepoint.
 * @return           Glyph pointer, the fallback glyph, or NULL if the font is
 *                   empty/NULL.
 *
 * Thread-safe: yes (read-only)  Reentrant: yes
 */
static inline const eli_font_glyph *eli_font_find_glyph(const eli_font *font, uint32_t codepoint)
{
    const eli_font_glyph *g = eli_font_find_glyph_no_fallback(font, codepoint);
    if (g)
        return g;
    return font ? font->fallback_glyph : NULL;
}

/** Append a fully-computed glyph and record its codepoint in the lookup table. */
static inline void eli_font_add_glyph(eli_font *font, const eli_font_glyph *glyph)
{
    font->glyphs = (eli_font_glyph *)eli_font__grow(font->glyphs, &font->glyph_capacity,
                                                    font->glyph_count + 1, sizeof(*font->glyphs));
    font->glyphs[font->glyph_count] = *glyph;

    if ((int)glyph->codepoint >= font->index_lookup_size) {
        int old = font->index_lookup_size;
        int want = (int)glyph->codepoint + 1;
        font->index_lookup = (int *)eli_font__grow(font->index_lookup, &font->index_lookup_size,
                                                   want, sizeof(*font->index_lookup));
        for (int k = old; k < font->index_lookup_size; k++)
            font->index_lookup[k] = -1;
    }
    font->index_lookup[glyph->codepoint] = font->glyph_count;
    font->glyph_count++;
}

/** Resolve a font's fallback glyph/advance once all glyphs have been added. */
static inline void eli_font__finish(eli_font *font)
{
    const eli_font_glyph *fb = eli_font_find_glyph_no_fallback(font, font->fallback_char);
    if (!fb)
        fb = eli_font_find_glyph_no_fallback(font, (uint32_t)' ');
    if (!fb && font->glyph_count > 0)
        fb = &font->glyphs[0];
    font->fallback_glyph = fb;
    font->fallback_advance_x = fb ? fb->advance_x : 0.0f;
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

/** Free the rasterized texture data and mark the atlas unbuilt. */
static inline void eli_font_atlas_clear_tex_data(eli_font_atlas *atlas)
{
    if (!atlas)
        return;
    free(atlas->tex_pixels_alpha8);
    free(atlas->tex_pixels_rgba32);
    atlas->tex_pixels_alpha8 = NULL;
    atlas->tex_pixels_rgba32 = NULL;
    atlas->tex_width = 0;
    atlas->tex_height = 0;
    atlas->built = false;
}

/** Free any TTF source memory the atlas owns (decompressed/embedded blobs). */
static inline void eli_font_atlas_clear_input_data(eli_font_atlas *atlas)
{
    if (!atlas)
        return;
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
    eli_font_atlas_clear_input_data(atlas);
    eli_font_atlas_clear_tex_data(atlas);
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

/** @return true once the atlas texture has been baked. */
static inline bool eli_font_atlas_is_built(const eli_font_atlas *atlas)
{
    return atlas && atlas->built && atlas->tex_pixels_alpha8 != NULL;
}

/** Assign the backend texture identifier the renderer will bind. */
static inline void eli_font_atlas_set_tex_id(eli_font_atlas *atlas, uint32_t id)
{
    if (atlas)
        atlas->tex_id = id;
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
    return cfg;
}

/**
 * Append a font source described by `cfg` and allocate the font it bakes into.
 *
 * @param atlas  Target atlas.
 * @param cfg    Source configuration (copied; must have valid font_data).
 * @return       The new (unbaked) font, or NULL on bad input/allocation failure.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline eli_font *eli_font_atlas_add_font(eli_font_atlas *atlas, const eli_font_config *cfg)
{
    if (!atlas || !cfg || !cfg->font_data || cfg->font_data_size <= 0)
        return NULL;

    eli_font *font = (eli_font *)calloc(1, sizeof(*font));
    if (!font)
        return NULL;
    font->container_atlas = atlas;
    font->fallback_char = ELI_FONT_FALLBACK_CHAR;

    atlas->configs = (eli_font_config *)eli_font__grow(atlas->configs, &atlas->configs_capacity,
                                                       atlas->configs_count + 1,
                                                       sizeof(*atlas->configs));
    atlas->fonts = (eli_font **)eli_font__grow(atlas->fonts, &atlas->fonts_capacity,
                                               atlas->fonts_count + 1, sizeof(*atlas->fonts));
    atlas->configs[atlas->configs_count] = *cfg;
    atlas->configs[atlas->configs_count].dst_font = font;
    atlas->configs_count++;
    atlas->fonts[atlas->fonts_count++] = font;
    atlas->built = false;
    return font;
}

/**
 * Add a font from an in-memory TTF/OTF buffer.
 *
 * @param atlas         Target atlas.
 * @param font_data     TTF bytes (not copied; caller keeps them alive until build).
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
 * Bake pipeline
 * ------------------------------------------------------------------------- */

/** Per-glyph working record threaded through the two-pass bake. */
typedef struct eli_font__build_glyph {
    int cfg_index;
    uint32_t codepoint;
    int glyph_index;
    int box_x0, box_y0;
    int w, h;
} eli_font__build_glyph;

/** Gather glyph boxes for every config into `out`; returns the glyph count. */
static inline int eli_font__gather_glyphs(eli_font_atlas *atlas, stbtt_fontinfo *finfo,
                                          const float *scales, eli_font__build_glyph *out,
                                          stbrp_rect *rects, int padding)
{
    int n = 0;
    for (int i = 0; i < atlas->configs_count; i++) {
        eli_font_config *cfg = &atlas->configs[i];
        const uint16_t *ranges =
            cfg->glyph_ranges ? cfg->glyph_ranges : eli_font_atlas_get_glyph_ranges_default(atlas);
        for (; ranges[0] && ranges[1]; ranges += 2) {
            for (uint32_t cp = ranges[0]; cp <= ranges[1]; cp++) {
                int g = stbtt_FindGlyphIndex(&finfo[i], (int)cp);
                if (g == 0)
                    continue;
                int x0, y0, x1, y1;
                stbtt_GetGlyphBitmapBox(&finfo[i], g, scales[i], scales[i], &x0, &y0, &x1, &y1);
                out[n].cfg_index = i;
                out[n].codepoint = cp;
                out[n].glyph_index = g;
                out[n].box_x0 = x0;
                out[n].box_y0 = y0;
                out[n].w = x1 - x0;
                out[n].h = y1 - y0;
                rects[n].id = n;
                rects[n].w = (stbrp_coord)(out[n].w + padding);
                rects[n].h = (stbrp_coord)(out[n].h + padding);
                n++;
            }
        }
    }
    return n;
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

/** Rasterize packed glyphs and the white texel into a fresh alpha8 buffer. */
static inline unsigned char *eli_font__rasterize(eli_font_atlas *atlas, stbtt_fontinfo *finfo,
                                                 const float *scales,
                                                 const eli_font__build_glyph *bg,
                                                 const stbrp_rect *rects, int count, int white_idx,
                                                 int tex_w, int tex_h)
{
    unsigned char *pixels = (unsigned char *)calloc((size_t)tex_w * (size_t)tex_h, 1);
    if (!pixels)
        return NULL;
    for (int k = 0; k < count; k++) {
        if (!rects[k].was_packed || bg[k].w <= 0 || bg[k].h <= 0)
            continue;
        int px = rects[k].x, py = rects[k].y;
        stbtt_MakeGlyphBitmap(&finfo[bg[k].cfg_index], pixels + (size_t)py * tex_w + px, bg[k].w,
                              bg[k].h, tex_w, scales[bg[k].cfg_index], scales[bg[k].cfg_index],
                              bg[k].glyph_index);
    }
    if (rects[white_idx].was_packed) {
        int wx = rects[white_idx].x, wy = rects[white_idx].y;
        for (int dy = 0; dy < ELI_FONT_ATLAS_WHITE_SIZE; dy++)
            for (int dx = 0; dx < ELI_FONT_ATLAS_WHITE_SIZE; dx++)
                pixels[(size_t)(wy + dy) * tex_w + (wx + dx)] = 0xFF;
        atlas->tex_uv_white_pixel =
            eli_make_vec2((wx + 1.0f) / (float)tex_w, (wy + 1.0f) / (float)tex_h);
    }
    return pixels;
}

/** Populate one font's metrics and glyph table from the bake results. */
static inline void eli_font__emit_font_glyphs(eli_font_atlas *atlas, stbtt_fontinfo *finfo,
                                              const float *scales, const int *ascents,
                                              const int *descents, const eli_font__build_glyph *bg,
                                              const stbrp_rect *rects, int count)
{
    for (int i = 0; i < atlas->configs_count; i++) {
        eli_font_config *cfg = &atlas->configs[i];
        eli_font *font = cfg->dst_font;
        eli_font__reset(font);
        font->container_atlas = atlas;
        font->font_size = cfg->size_pixels;
        font->ascent = (float)((int)(ascents[i] * scales[i] + 0.5f));
        font->descent = (float)((int)(descents[i] * scales[i] - 0.5f));
        font->line_height = cfg->size_pixels;
        font->fallback_char = ELI_FONT_FALLBACK_CHAR;
        font->tex_uv_white_pixel = atlas->tex_uv_white_pixel;
    }
    for (int k = 0; k < count; k++) {
        const eli_font__build_glyph *b = &bg[k];
        eli_font_config *cfg = &atlas->configs[b->cfg_index];
        eli_font *font = cfg->dst_font;
        int adv, lsb;
        stbtt_GetGlyphHMetrics(&finfo[b->cfg_index], b->glyph_index, &adv, &lsb);
        float advance = adv * scales[b->cfg_index];
        if (cfg->pixel_snap_h)
            advance = (float)((int)(advance + 0.5f));
        if (cfg->glyph_min_advance_x > 0.0f && advance < cfg->glyph_min_advance_x)
            advance = cfg->glyph_min_advance_x;
        if (cfg->glyph_max_advance_x > 0.0f && advance > cfg->glyph_max_advance_x)
            advance = cfg->glyph_max_advance_x;

        eli_font_glyph gl;
        memset(&gl, 0, sizeof(gl));
        gl.codepoint = b->codepoint;
        gl.advance_x = advance;
        if (rects[k].was_packed && b->w > 0 && b->h > 0) {
            gl.visible = true;
            gl.x0 = (float)b->box_x0 + cfg->glyph_offset_x;
            gl.y0 = font->ascent + (float)b->box_y0 + cfg->glyph_offset_y;
            gl.x1 = gl.x0 + (float)b->w;
            gl.y1 = gl.y0 + (float)b->h;
            gl.u0 = rects[k].x * atlas->tex_uv_scale.x;
            gl.v0 = rects[k].y * atlas->tex_uv_scale.y;
            gl.u1 = (rects[k].x + b->w) * atlas->tex_uv_scale.x;
            gl.v1 = (rects[k].y + b->h) * atlas->tex_uv_scale.y;
        }
        eli_font_add_glyph(font, &gl);
    }
    for (int i = 0; i < atlas->configs_count; i++)
        eli_font__finish(atlas->configs[i].dst_font);
}

/**
 * Bake all configured fonts into the atlas texture. Implicitly adds the default
 * font if none were configured. Re-bakeable (frees prior texture/glyphs).
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

    eli_font_atlas_clear_tex_data(atlas);
    int padding = (atlas->tex_glyph_padding > 0) ? atlas->tex_glyph_padding
                                                 : ELI_FONT_ATLAS_DEFAULT_PADDING;
    int nfonts = atlas->configs_count;

    bool ok = false;
    stbtt_fontinfo *finfo = (stbtt_fontinfo *)calloc(nfonts, sizeof(*finfo));
    float *scales = (float *)calloc(nfonts, sizeof(*scales));
    int *ascents = (int *)calloc(nfonts, sizeof(*ascents));
    int *descents = (int *)calloc(nfonts, sizeof(*descents));
    eli_font__build_glyph *bg = NULL;
    stbrp_rect *rects = NULL;
    stbrp_node *nodes = NULL;
    unsigned char *pixels = NULL;
    if (!finfo || !scales || !ascents || !descents)
        goto cleanup;

    int total_possible = 0;
    for (int i = 0; i < nfonts; i++) {
        eli_font_config *cfg = &atlas->configs[i];
        int off = stbtt_GetFontOffsetForIndex((const unsigned char *)cfg->font_data, cfg->font_no);
        if (off < 0 || !stbtt_InitFont(&finfo[i], (const unsigned char *)cfg->font_data, off))
            goto cleanup;
        scales[i] = stbtt_ScaleForPixelHeight(&finfo[i], cfg->size_pixels);
        int gap;
        stbtt_GetFontVMetrics(&finfo[i], &ascents[i], &descents[i], &gap);
        const uint16_t *ranges =
            cfg->glyph_ranges ? cfg->glyph_ranges : eli_font_atlas_get_glyph_ranges_default(atlas);
        total_possible += eli_font__count_range_glyphs(ranges);
    }

    bg = (eli_font__build_glyph *)calloc((size_t)total_possible, sizeof(*bg));
    rects = (stbrp_rect *)calloc((size_t)total_possible + 1, sizeof(*rects));
    if ((total_possible > 0 && !bg) || !rects)
        goto cleanup;

    int count = eli_font__gather_glyphs(atlas, finfo, scales, bg, rects, padding);
    int white_idx = count;
    rects[white_idx].id = -1;
    rects[white_idx].w = (stbrp_coord)(ELI_FONT_ATLAS_WHITE_SIZE + padding);
    rects[white_idx].h = (stbrp_coord)(ELI_FONT_ATLAS_WHITE_SIZE + padding);

    int tex_w = eli_font__pick_width(atlas, rects, count + 1);
    const int max_h = 1024 * 32;
    nodes = (stbrp_node *)calloc((size_t)tex_w, sizeof(*nodes));
    if (!nodes)
        goto cleanup;
    stbrp_context ctx;
    stbrp_init_target(&ctx, tex_w, max_h, nodes, tex_w);
    stbrp_pack_rects(&ctx, rects, count + 1);

    int tex_h = 0;
    for (int k = 0; k <= count; k++)
        if (rects[k].was_packed && rects[k].y + rects[k].h > tex_h)
            tex_h = rects[k].y + rects[k].h;
    if (tex_h <= 0)
        goto cleanup;
    if (!(atlas->flags & ELI_FONT_ATLAS_NO_POWER_OF_TWO_HEIGHT))
        tex_h = eli_font__next_pow2(tex_h);

    atlas->tex_width = tex_w;
    atlas->tex_height = tex_h;
    atlas->tex_uv_scale = eli_make_vec2(1.0f / (float)tex_w, 1.0f / (float)tex_h);

    pixels = eli_font__rasterize(atlas, finfo, scales, bg, rects, count, white_idx, tex_w, tex_h);
    if (!pixels)
        goto cleanup;
    atlas->tex_pixels_alpha8 = pixels;
    pixels = NULL;

    eli_font__emit_font_glyphs(atlas, finfo, scales, ascents, descents, bg, rects, count);
    atlas->built = true;
    ok = true;

cleanup:
    free(finfo);
    free(scales);
    free(ascents);
    free(descents);
    free(bg);
    free(rects);
    free(nodes);
    free(pixels);
    return ok;
}

/* ---------------------------------------------------------------------------
 * Texture data accessors (build on demand)
 * ------------------------------------------------------------------------- */

/**
 * Fetch the single-channel (alpha8) texture, building the atlas if needed.
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
 * Fetch the atlas as RGBA32 (white with per-texel alpha), building/converting on
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
