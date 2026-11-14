/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_font_atlas_pages.h
 * @brief Atlas texture pages: allocation, rect packing, LRU recycling, idle
 *        freeing, dirty-region tracking and the backend texture hooks.
 *
 * A baked atlas has one page sized to fit its glyphs. An on-demand atlas
 * allocates page_size-square pages as glyphs arrive; each starts with the white
 * block at texels (0,0)-(2,2), so the white UV is the same on every page. When
 * a glyph fits no live page and max_pages are live, the least recently used page
 * not used this frame (never page 0) is cleared and reused; if every page was
 * used this frame one more is allocated (a soft cap bounded by the frame's
 * working set). eli_font_atlas_update, called once per rendered frame, uploads
 * dirty regions through the backend hooks, frees pages idle for
 * page_idle_frames and advances the atlas frame counter.
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#ifndef ELI_FONT_ELI_FONT_ATLAS_PAGES_H
#define ELI_FONT_ELI_FONT_ATLAS_PAGES_H

#include "eli_font_types.h"
#include "eli_font_stb.h"
#include "eli_font_glyph_table.h"

#include "../core/eli_platform.h"
#include "../core/eli_types.h"

/** Default spacing (in texels) left between packed glyphs. */
#define ELI_FONT_ATLAS_DEFAULT_PADDING 1

/** Side length (texels) of the opaque white block reserved for solid fills. */
#define ELI_FONT_ATLAS_WHITE_SIZE 3

/** stb_rect_pack state of one on-demand page (nodes follow the context). */
typedef struct eli_font__page_packer {
    stbrp_context ctx;
    stbrp_node nodes[1]; /* page width entries */
} eli_font__page_packer;

/* ---------------------------------------------------------------------------
 * Settings
 * ------------------------------------------------------------------------- */

/** @return the on-demand page side length. */
static inline int eli_font__page_size(const eli_font_atlas *atlas)
{
    return atlas->page_size > 0 ? atlas->page_size : ELI_FONT_ATLAS_DEFAULT_PAGE_SIZE;
}

/** @return the glyph padding in texels. */
static inline int eli_font__padding(const eli_font_atlas *atlas)
{
    return atlas->tex_glyph_padding > 0 ? atlas->tex_glyph_padding
                                        : ELI_FONT_ATLAS_DEFAULT_PADDING;
}

/** @return true when the atlas rasterizes glyphs on first use. */
static inline bool eli_font__on_demand(const eli_font_atlas *atlas)
{
    return (atlas->flags & ELI_FONT_ATLAS_ON_DEMAND) != 0;
}

/* ---------------------------------------------------------------------------
 * Page state helpers
 * ------------------------------------------------------------------------- */

/** Mirror page 0 into the legacy tex_* fields. */
static inline void eli_font__sync_page0(eli_font_atlas *atlas)
{
    const eli_font_atlas_page *p = atlas->pages_count > 0 ? &atlas->pages[0] : NULL;
    bool live = p && p->live;
    atlas->tex_pixels_alpha8 = live ? p->pixels : NULL;
    atlas->tex_width = live ? p->width : 0;
    atlas->tex_height = live ? p->height : 0;
    if (live) {
        atlas->tex_id = p->tex_id;
        atlas->tex_uv_scale = eli_make_vec2(1.0f / (float)p->width, 1.0f / (float)p->height);
    }
}

/** Grow a page's dirty region by [x0,x1) x [y0,y1) and bump its version. */
static inline void eli_font__page_mark_dirty(eli_font_atlas_page *p, int x0, int y0, int x1,
                                             int y1)
{
    if (p->dirty_x1 <= p->dirty_x0) {
        p->dirty_x0 = x0;
        p->dirty_y0 = y0;
        p->dirty_x1 = x1;
        p->dirty_y1 = y1;
    } else {
        p->dirty_x0 = x0 < p->dirty_x0 ? x0 : p->dirty_x0;
        p->dirty_y0 = y0 < p->dirty_y0 ? y0 : p->dirty_y0;
        p->dirty_x1 = x1 > p->dirty_x1 ? x1 : p->dirty_x1;
        p->dirty_y1 = y1 > p->dirty_y1 ? y1 : p->dirty_y1;
    }
    p->version++;
}

/** Create the page's texture through the backend hook if it has none. */
static inline void eli_font__page_create_texture(eli_font_atlas *atlas, eli_font_atlas_page *p)
{
    if (p->tex_id || !p->pixels || !atlas->backend.create_texture)
        return;
    p->tex_id = atlas->backend.create_texture(atlas->backend.user_data, p->width, p->height,
                                              p->pixels);
    p->dirty_x0 = p->dirty_y0 = p->dirty_x1 = p->dirty_y1 = 0; /* the create uploaded it */
}

/** Destroy the page's texture through the backend hook. */
static inline void eli_font__page_destroy_texture(eli_font_atlas *atlas, eli_font_atlas_page *p)
{
    if (p->tex_id && atlas->backend.destroy_texture)
        atlas->backend.destroy_texture(atlas->backend.user_data, p->tex_id);
    p->tex_id = 0;
}

/**
 * Drop every glyph on page `page` from every font (they reload on next use).
 * Compacts each font's glyph array and repoints the moved lookup entries.
 */
static inline void eli_font__drop_page_glyphs(eli_font_atlas *atlas, int page)
{
    for (int f = 0; f < atlas->fonts_count; f++) {
        eli_font *font = atlas->fonts[f];
        int kept = 0;
        for (int i = 0; i < font->glyph_count; i++) {
            eli_font_glyph *g = &font->glyphs[i];
            bool drop = g->kind == ELI_FONT_GLYPH_ATLAS && g->visible && g->page == page;
            if (drop) {
                eli_font__lookup_set(font, g->codepoint, ELI_FONT_LOOKUP_UNKNOWN);
                continue;
            }
            if (kept != i) {
                font->glyphs[kept] = *g;
                eli_font__lookup_set(font, g->codepoint, kept);
            }
            kept++;
        }
        font->glyph_count = kept;
        eli_font__resolve_fallback(font);
    }
}

/** Free a page slot: texture, pixels and packer (the slot is reused later). */
static inline void eli_font__page_free(eli_font_atlas *atlas, int idx)
{
    eli_font_atlas_page *p = &atlas->pages[idx];
    eli_font__page_destroy_texture(atlas, p);
    eli_mem_free(p->pixels);
    eli_mem_free(p->packer);
    memset(p, 0, sizeof(*p));
}

/** Free every page slot and the page array. */
static inline void eli_font__pages_free_all(eli_font_atlas *atlas)
{
    for (int i = 0; i < atlas->pages_count; i++)
        eli_font__page_free(atlas, i);
    eli_mem_free(atlas->pages);
    atlas->pages = NULL;
    atlas->pages_count = 0;
    atlas->pages_capacity = 0;
    eli_font__sync_page0(atlas);
}

/** @return a free page slot index (growing the array), or -1 on OOM. */
static inline int eli_font__page_alloc_slot(eli_font_atlas *atlas)
{
    for (int i = 0; i < atlas->pages_count; i++)
        if (!atlas->pages[i].live)
            return i;
    eli_font_atlas_page *grown = (eli_font_atlas_page *)eli_font__grow(
        atlas->pages, &atlas->pages_capacity, atlas->pages_count + 1, sizeof(*atlas->pages));
    if (atlas->pages_capacity < atlas->pages_count + 1)
        return -1;
    atlas->pages = grown;
    memset(&atlas->pages[atlas->pages_count], 0, sizeof(*atlas->pages));
    return atlas->pages_count++;
}

/* ---------------------------------------------------------------------------
 * On-demand pages
 * ------------------------------------------------------------------------- */

/** Pack one w x h rect into page `idx`; false when it does not fit. */
static inline bool eli_font__page_pack(eli_font_atlas *atlas, int idx, int w, int h, int *x,
                                       int *y)
{
    eli_font__page_packer *pk = (eli_font__page_packer *)atlas->pages[idx].packer;
    if (!pk)
        return false;
    stbrp_rect r;
    memset(&r, 0, sizeof(r));
    r.w = (stbrp_coord)w;
    r.h = (stbrp_coord)h;
    if (!stbrp_pack_rects(&pk->ctx, &r, 1) || !r.was_packed)
        return false;
    *x = r.x;
    *y = r.y;
    return true;
}

/** Clear page `idx` to an empty on-demand page: fresh packer and the white block. */
static inline bool eli_font__page_clear_on_demand(eli_font_atlas *atlas, int idx)
{
    eli_font_atlas_page *p = &atlas->pages[idx];
    int size = eli_font__page_size(atlas);
    memset(p->pixels, 0, (size_t)size * (size_t)size);
    eli_font__page_packer *pk = (eli_font__page_packer *)p->packer;
    stbrp_init_target(&pk->ctx, size, size, pk->nodes, size);
    int pad = eli_font__padding(atlas), wx = 0, wy = 0;
    if (!eli_font__page_pack(atlas, idx, ELI_FONT_ATLAS_WHITE_SIZE + pad,
                             ELI_FONT_ATLAS_WHITE_SIZE + pad, &wx, &wy) || wx != 0 || wy != 0)
        return false;
    for (int dy = 0; dy < ELI_FONT_ATLAS_WHITE_SIZE; dy++)
        memset(p->pixels + (size_t)dy * (size_t)size, 0xFF, ELI_FONT_ATLAS_WHITE_SIZE);
    p->glyph_count = 0;
    p->last_used_frame = atlas->frame_count;
    eli_font__page_mark_dirty(p, 0, 0, size, size);
    return true;
}

/** Allocate and clear a new on-demand page; its index, or -1 on failure. */
static inline int eli_font__page_new_on_demand(eli_font_atlas *atlas)
{
    int idx = eli_font__page_alloc_slot(atlas);
    if (idx < 0)
        return -1;
    int size = eli_font__page_size(atlas);
    eli_font_atlas_page *p = &atlas->pages[idx];
    p->pixels = (unsigned char *)eli_mem_alloc((size_t)size * (size_t)size);
    p->packer = eli_mem_alloc(sizeof(eli_font__page_packer) + (size_t)size * sizeof(stbrp_node));
    p->width = size;
    p->height = size;
    p->live = true;
    if (!p->pixels || !p->packer || !eli_font__page_clear_on_demand(atlas, idx)) {
        eli_font__page_free(atlas, idx);
        return -1;
    }
    eli_font__page_create_texture(atlas, p);
    if (idx == 0)
        eli_font__sync_page0(atlas);
    return idx;
}

/** @return the number of live pages. */
static inline int eli_font_atlas_live_page_count(const eli_font_atlas *atlas)
{
    int n = 0;
    for (int i = 0; atlas && i < atlas->pages_count; i++)
        n += atlas->pages[i].live ? 1 : 0;
    return n;
}

/** @return the LRU live page (> 0) not used this frame, or -1. */
static inline int eli_font__page_pick_victim(const eli_font_atlas *atlas)
{
    int victim = -1;
    for (int i = 1; i < atlas->pages_count; i++) {
        const eli_font_atlas_page *p = &atlas->pages[i];
        if (!p->live || p->last_used_frame >= atlas->frame_count)
            continue;
        if (victim < 0 || p->last_used_frame < atlas->pages[victim].last_used_frame)
            victim = i;
    }
    return victim;
}

/**
 * Find room for a w x h rect (padding included) on an on-demand atlas: a live
 * page, else a new page, else a recycled LRU page, else a new page past the cap.
 *
 * @return true with page, x and y set, false when the rect exceeds a page or OOM.
 */
static inline bool eli_font__atlas_place(eli_font_atlas *atlas, int w, int h, int *page, int *x,
                                         int *y)
{
    int size = eli_font__page_size(atlas);
    if (w > size || h > size)
        return false;
    for (int i = 0; i < atlas->pages_count; i++)
        if (atlas->pages[i].live && eli_font__page_pack(atlas, i, w, h, x, y)) {
            *page = i;
            return true;
        }
    int max_pages = atlas->max_pages > 0 ? atlas->max_pages : ELI_FONT_ATLAS_DEFAULT_MAX_PAGES;
    int idx = -1;
    if (eli_font_atlas_live_page_count(atlas) >= max_pages)
        idx = eli_font__page_pick_victim(atlas);
    if (idx >= 0) {
        eli_font__drop_page_glyphs(atlas, idx);
        if (!eli_font__page_clear_on_demand(atlas, idx))
            return false;
    } else {
        idx = eli_font__page_new_on_demand(atlas);
    }
    if (idx < 0 || !eli_font__page_pack(atlas, idx, w, h, x, y))
        return false;
    *page = idx;
    return true;
}

/* ---------------------------------------------------------------------------
 * Public: backend hooks, per-frame update, accounting
 * ------------------------------------------------------------------------- */

/**
 * Install backend texture hooks and create textures for live pages that have
 * pixels but no texture. NULL clears the hooks without destroying textures.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_font_atlas_set_backend(eli_font_atlas *atlas,
                                              const eli_font_atlas_backend *backend)
{
    if (!atlas)
        return;
    if (!backend) {
        memset(&atlas->backend, 0, sizeof(atlas->backend));
        return;
    }
    atlas->backend = *backend;
    for (int i = 0; i < atlas->pages_count; i++)
        if (atlas->pages[i].live)
            eli_font__page_create_texture(atlas, &atlas->pages[i]);
    eli_font__sync_page0(atlas);
}

/**
 * Destroy every page texture through the backend hook (pages keep their pixels,
 * so eli_font_atlas_set_backend / update can create them again).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_font_atlas_release_textures(eli_font_atlas *atlas)
{
    if (!atlas)
        return;
    for (int i = 0; i < atlas->pages_count; i++)
        eli_font__page_destroy_texture(atlas, &atlas->pages[i]);
    eli_font__sync_page0(atlas);
}

/**
 * Once per rendered frame, before drawing: create missing page textures, upload
 * dirty regions, free on-demand pages (other than page 0) unused for
 * page_idle_frames, then advance the frame counter that LRU recycling uses.
 *
 * @param atlas  Atlas (NULL or unbuilt is a no-op).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_font_atlas_update(eli_font_atlas *atlas)
{
    if (!atlas || !atlas->built)
        return;
    uint64_t idle = atlas->page_idle_frames > 0 ? (uint64_t)atlas->page_idle_frames
                                                : ELI_FONT_ATLAS_DEFAULT_PAGE_IDLE_FRAMES;
    for (int i = 0; i < atlas->pages_count; i++) {
        eli_font_atlas_page *p = &atlas->pages[i];
        if (!p->live)
            continue;
        if (i > 0 && eli_font__on_demand(atlas) && atlas->frame_count - p->last_used_frame > idle) {
            eli_font__drop_page_glyphs(atlas, i);
            eli_font__page_free(atlas, i);
            continue;
        }
        if (!p->tex_id) {
            eli_font__page_create_texture(atlas, p);
        } else if (p->dirty_x1 > p->dirty_x0 && p->pixels && atlas->backend.update_texture) {
            atlas->backend.update_texture(
                atlas->backend.user_data, p->tex_id, p->dirty_x0, p->dirty_y0,
                p->dirty_x1 - p->dirty_x0, p->dirty_y1 - p->dirty_y0,
                p->pixels + (size_t)p->dirty_y0 * (size_t)p->width + (size_t)p->dirty_x0,
                p->width);
            p->dirty_x0 = p->dirty_y0 = p->dirty_x1 = p->dirty_y1 = 0;
        }
    }
    eli_font__sync_page0(atlas);
    atlas->frame_count++;
}

/**
 * @return bytes of single-channel texture held by live pages (what an A8/R8
 *         upload of every page costs on the GPU).
 */
static inline size_t eli_font_atlas_texture_bytes(const eli_font_atlas *atlas)
{
    size_t bytes = 0;
    for (int i = 0; atlas && i < atlas->pages_count; i++)
        if (atlas->pages[i].live)
            bytes += (size_t)atlas->pages[i].width * (size_t)atlas->pages[i].height;
    return bytes;
}

#endif /* ELI_FONT_ELI_FONT_ATLAS_PAGES_H */
