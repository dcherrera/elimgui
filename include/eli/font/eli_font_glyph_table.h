/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_font_glyph_table.h
 * @brief A font's glyph storage: the two-level codepoint -> glyph lookup, glyph
 *        lookup with on-demand loading, the fallback (tofu box by default) and
 *        custom-glyph ranges.
 *
 * Lookup blocks cover 256 codepoints each and are allocated on first use, so a
 * font that only draws ASCII holds one block, and one emoji costs one more block
 * plus a pointer table up to its codepoint, not a table spanning all of Unicode.
 * On an on-demand atlas an unknown entry loads the glyph through the font's
 * fallback chain (eli_font__load_glyph, eli_font_atlas_on_demand.h) and records
 * the result, including "missing", so each codepoint is resolved once.
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#ifndef ELI_FONT_ELI_FONT_GLYPH_TABLE_H
#define ELI_FONT_ELI_FONT_GLYPH_TABLE_H

#include "eli_font_types.h"

#include "../core/eli_platform.h"
#include "../core/eli_types.h"
#include "../util/eli_mem.h"

/** Largest Unicode codepoint the lookup accepts. */
#define ELI_FONT_CODEPOINT_MAX 0x10FFFFu

/* Defined in eli_font_atlas_on_demand.h: rasterize `codepoint` through the font's
 * source chain; returns its glyph index or ELI_FONT_LOOKUP_MISSING. */
static inline int eli_font__load_glyph(eli_font *font, uint32_t codepoint);

/** Allocate `count` zeroed elements of `size` bytes through the eli_mem seam (NULL on
 *  overflow or failure). */
static inline void *eli_font__calloc(size_t count, size_t size)
{
    if (count == 0 || size == 0 || count > (size_t)-1 / size)
        return NULL;
    void *p = eli_mem_alloc(count * size);
    if (p)
        memset(p, 0, count * size);
    return p;
}

/** Grow a heap array to hold at least `need` elements; doubles capacity. */
static inline void *eli_font__grow(void *ptr, int *cap, int need, size_t elem)
{
    if (need <= *cap)
        return ptr;
    int new_cap = (*cap > 0) ? (*cap * 2) : 8;
    if (new_cap < need)
        new_cap = need;
    void *p = eli_mem_realloc(ptr, (size_t)*cap * elem, (size_t)new_cap * elem);
    if (p)
        *cap = new_cap;
    return p;
}

/* ---------------------------------------------------------------------------
 * Lookup table
 * ------------------------------------------------------------------------- */

/** @return the lookup entry for `cp`, or ELI_FONT_LOOKUP_UNKNOWN if no block. */
static inline int eli_font__lookup_get(const eli_font *font, uint32_t cp)
{
    uint32_t b = cp / ELI_FONT_LOOKUP_BLOCK;
    if (b >= (uint32_t)font->lookup_block_count || !font->lookup_blocks[b])
        return ELI_FONT_LOOKUP_UNKNOWN;
    return font->lookup_blocks[b][cp % ELI_FONT_LOOKUP_BLOCK];
}

/** Store `value` for `cp`, allocating its block; false on OOM or bad codepoint. */
static inline bool eli_font__lookup_set(eli_font *font, uint32_t cp, int value)
{
    if (cp > ELI_FONT_CODEPOINT_MAX)
        return false;
    int b = (int)(cp / ELI_FONT_LOOKUP_BLOCK);
    if (b >= font->lookup_block_count) {
        int want = b + 1;
        int **grown = (int **)eli_mem_realloc(font->lookup_blocks,
                                              (size_t)font->lookup_block_count * sizeof(*grown),
                                              (size_t)want * sizeof(*grown));
        if (!grown)
            return false;
        for (int k = font->lookup_block_count; k < want; k++)
            grown[k] = NULL;
        font->lookup_blocks = grown;
        font->lookup_block_count = want;
    }
    if (!font->lookup_blocks[b]) {
        int *block = (int *)eli_mem_alloc(ELI_FONT_LOOKUP_BLOCK * sizeof(*block));
        if (!block)
            return false;
        for (int k = 0; k < ELI_FONT_LOOKUP_BLOCK; k++)
            block[k] = ELI_FONT_LOOKUP_UNKNOWN;
        font->lookup_blocks[b] = block;
    }
    font->lookup_blocks[b][cp % ELI_FONT_LOOKUP_BLOCK] = value;
    return true;
}

/** Free every lookup block and the block table. */
static inline void eli_font__lookup_free(eli_font *font)
{
    for (int b = 0; b < font->lookup_block_count; b++)
        eli_mem_free(font->lookup_blocks[b]);
    eli_mem_free(font->lookup_blocks);
    font->lookup_blocks = NULL;
    font->lookup_block_count = 0;
}

/* ---------------------------------------------------------------------------
 * Fallback and glyph storage
 * ------------------------------------------------------------------------- */

/** Point fallback_glyph at the fallback_char glyph, else the tofu box. */
static inline void eli_font__resolve_fallback(eli_font *font)
{
    const eli_font_glyph *fb = &font->tofu_glyph;
    if (font->fallback_char != ELI_FONT_FALLBACK_TOFU) {
        int idx = eli_font__lookup_get(font, font->fallback_char);
        if (idx >= 0)
            fb = &font->glyphs[idx];
    }
    font->fallback_glyph = fb;
    font->fallback_advance_x = fb->advance_x;
}

/** Release a font's glyph array and lookup table, leaving it empty. */
static inline void eli_font__reset(eli_font *font)
{
    eli_mem_free(font->glyphs);
    eli_font__lookup_free(font);
    font->glyphs = NULL;
    font->glyph_count = 0;
    font->glyph_capacity = 0;
    font->fallback_glyph = NULL;
    font->fallback_advance_x = 0.0f;
}

/**
 * Append a fully-computed glyph and record its codepoint in the lookup table.
 *
 * @param font   Target font.
 * @param glyph  Glyph to copy in (its codepoint keys the lookup).
 * @return       The new glyph index, or -1 on allocation failure.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline int eli_font_add_glyph(eli_font *font, const eli_font_glyph *glyph)
{
    eli_font_glyph *grown = (eli_font_glyph *)eli_font__grow(
        font->glyphs, &font->glyph_capacity, font->glyph_count + 1, sizeof(*font->glyphs));
    if (font->glyph_capacity < font->glyph_count + 1)
        return -1;
    font->glyphs = grown;
    if (!eli_font__lookup_set(font, glyph->codepoint, font->glyph_count))
        return -1;
    font->glyphs[font->glyph_count] = *glyph;
    eli_font__resolve_fallback(font); /* the array may have moved */
    return font->glyph_count++;
}

/* ---------------------------------------------------------------------------
 * Lookup
 * ------------------------------------------------------------------------- */

/** Stamp the glyph's atlas page as used this frame (keeps it from eviction). */
static inline void eli_font__touch_page(const eli_font *font, const eli_font_glyph *g)
{
    eli_font_atlas *atlas = font->container_atlas;
    if (g->kind == ELI_FONT_GLYPH_ATLAS && atlas && g->page < atlas->pages_count)
        atlas->pages[g->page].last_used_frame = atlas->frame_count;
}

/**
 * Look up a glyph by codepoint, returning NULL (not the fallback) if no source in
 * the font's chain has it. On an on-demand atlas this rasterizes the glyph on
 * first use (the font is a logically-const cache).
 *
 * Thread-safe: no (may rasterize)  Reentrant: no
 */
static inline const eli_font_glyph *eli_font_find_glyph_no_fallback(const eli_font *font,
                                                                    uint32_t codepoint)
{
    if (!font)
        return NULL;
    int idx = eli_font__lookup_get(font, codepoint);
    if (idx == ELI_FONT_LOOKUP_UNKNOWN && font->container_atlas &&
        (font->container_atlas->flags & ELI_FONT_ATLAS_ON_DEMAND) &&
        font->container_atlas->sources && codepoint <= ELI_FONT_CODEPOINT_MAX)
        idx = eli_font__load_glyph((eli_font *)font, codepoint);
    if (idx < 0)
        return NULL;
    const eli_font_glyph *g = &font->glyphs[idx];
    eli_font__touch_page(font, g);
    return g;
}

/* Test instrumentation: define ELI_FONT_GLYPH_LOOKUP_HOOK() before including
 * elimgui to count glyph lookups (text-measure complexity tests). No-op otherwise. */
#ifndef ELI_FONT_GLYPH_LOOKUP_HOOK
#define ELI_FONT_GLYPH_LOOKUP_HOOK() ((void)0)
#endif

/**
 * Look up a glyph by codepoint, falling back to the font's fallback glyph (the
 * tofu box unless fallback_char names a glyph the font has).
 *
 * @param font       Font.
 * @param codepoint  Unicode codepoint.
 * @return           Glyph pointer, the fallback glyph, or NULL if the font is
 *                   NULL or not built.
 *
 * Thread-safe: no (may rasterize)  Reentrant: no
 */
static inline const eli_font_glyph *eli_font_find_glyph(const eli_font *font, uint32_t codepoint)
{
    ELI_FONT_GLYPH_LOOKUP_HOOK();
    const eli_font_glyph *g = eli_font_find_glyph_no_fallback(font, codepoint);
    if (g || !font)
        return g;
    if (font->fallback_char != ELI_FONT_FALLBACK_TOFU) {
        g = eli_font_find_glyph_no_fallback(font, font->fallback_char);
        if (g)
            return g;
    }
    return font->fallback_glyph;
}

/**
 * Choose what a missing glyph draws as: a codepoint the font has (e.g. '?'), or
 * ELI_FONT_FALLBACK_TOFU for the tofu box (the default).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_font_set_fallback_char(eli_font *font, uint32_t codepoint)
{
    if (!font)
        return;
    font->fallback_char = codepoint;
    if (codepoint != ELI_FONT_FALLBACK_TOFU)
        eli_font_find_glyph_no_fallback(font, codepoint); /* load it on demand */
    eli_font__resolve_fallback(font);
}

/* ---------------------------------------------------------------------------
 * Custom glyphs
 * ------------------------------------------------------------------------- */

/** Insert custom-glyph entries for one registered range (wins over any face). */
static inline void eli_font__apply_custom_range(eli_font *font, const eli_font_custom_range *r)
{
    eli_font_glyph g;
    memset(&g, 0, sizeof(g));
    g.kind = ELI_FONT_GLYPH_CUSTOM;
    g.advance_x = r->advance_x > 0.0f ? r->advance_x : font->cell_advance_x;
    for (uint32_t cp = r->lo; cp <= r->hi; cp++) {
        g.codepoint = cp;
        int idx = eli_font__lookup_get(font, cp);
        if (idx >= 0)
            font->glyphs[idx] = g; /* replace the face's glyph in place */
        else if (eli_font_add_glyph(font, &g) < 0)
            return;
    }
}

/** Re-insert every registered custom range (after a build reset the table). */
static inline void eli_font__apply_custom_ranges(eli_font *font)
{
    for (int i = 0; i < font->custom_range_count; i++)
        eli_font__apply_custom_range(font, &font->custom_ranges[i]);
}

/**
 * Set the callback that draws this font's custom glyphs.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_font_set_custom_glyph_fn(eli_font *font, eli_font_custom_glyph_fn fn,
                                                void *user_data)
{
    if (!font)
        return;
    font->custom_glyph_fn = fn;
    font->custom_glyph_user_data = user_data;
}

/**
 * Have the font's custom_glyph_fn draw codepoints [lo, hi] instead of any face.
 * Kept across rebuilds. Text measurement uses `advance_x` (<= 0: the font's cell
 * advance, i.e. its 'M').
 *
 * @return false on bad input or allocation failure.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_font_add_custom_glyph_range(eli_font *font, uint32_t lo, uint32_t hi,
                                                   float advance_x)
{
    if (!font || lo > hi || hi > ELI_FONT_CODEPOINT_MAX)
        return false;
    eli_font_custom_range *grown = (eli_font_custom_range *)eli_mem_realloc(
        font->custom_ranges, (size_t)font->custom_range_count * sizeof(*grown),
        (size_t)(font->custom_range_count + 1) * sizeof(*grown));
    if (!grown)
        return false;
    font->custom_ranges = grown;
    eli_font_custom_range *r = &font->custom_ranges[font->custom_range_count++];
    r->lo = lo;
    r->hi = hi;
    r->advance_x = advance_x;
    if (font->container_atlas && font->container_atlas->built)
        eli_font__apply_custom_range(font, r);
    return true;
}

#endif /* ELI_FONT_ELI_FONT_GLYPH_TABLE_H */
