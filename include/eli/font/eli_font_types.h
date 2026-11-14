/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_font_types.h
 * @brief Font-system data types: glyph, font, font configuration, and the font
 *        atlas that owns rasterized texture data and the fonts baked into it.
 *
 * A glyph carries its advance plus the screen-space corner offsets (top-left
 * origin), its atlas page and the texture coordinates a text quad is built from.
 * A font owns a flat glyph array plus a two-level codepoint->glyph lookup. The
 * atlas owns single-channel (alpha8) texture pages, an optional RGBA32 copy of
 * page 0, backend texture hooks, and the fonts and configs baked into it.
 *
 * @status Phase 3 font types; pages, on-demand glyphs, merge chains, tofu and
 *         custom glyphs.
 * @issues None
 * @todo None
 */
#ifndef ELI_FONT_ELI_FONT_TYPES_H
#define ELI_FONT_ELI_FONT_TYPES_H

#include "../core/eli_platform.h"
#include "../core/eli_types.h"
#include "../draw/eli_draw_types.h"

/* Forward typedefs. `eli_font` is also forward-declared by the core context;
 * C11 permits the identical typedef to be repeated here where it is defined. */
typedef struct eli_font        eli_font;
typedef struct eli_font_atlas  eli_font_atlas;
typedef struct eli_font_config eli_font_config;

/** Defined when the atlas has pages, on-demand glyphs and merge-mode fallback chains. */
#define ELI_FONT_HAS_PAGES 1

/** Default pixel size used for the embedded ProggyClean font. */
#define ELI_FONT_DEFAULT_SIZE 13.0f

/** fallback_char value meaning "draw a tofu box" for a missing glyph (the default). */
#define ELI_FONT_FALLBACK_TOFU 0u

/** Default fallback codepoint: none, so a missing glyph draws as a tofu box, not '?'. */
#define ELI_FONT_FALLBACK_CHAR ELI_FONT_FALLBACK_TOFU

/** Length (in bytes) of the config name buffer. */
#define ELI_FONT_NAME_MAX 40

/** On-demand page side length in texels when atlas->page_size is 0. */
#define ELI_FONT_ATLAS_DEFAULT_PAGE_SIZE 512

/** Pages an on-demand atlas keeps before it recycles the least recently used one. */
#define ELI_FONT_ATLAS_DEFAULT_MAX_PAGES 8

/** Frames a page (other than page 0) may go unused before it is freed. */
#define ELI_FONT_ATLAS_DEFAULT_PAGE_IDLE_FRAMES 600

/* ---------------------------------------------------------------------------
 * Glyph
 * ------------------------------------------------------------------------- */

/** How a glyph is drawn. */
enum eli_font_glyph_kind_ {
    ELI_FONT_GLYPH_ATLAS = 0, /* a quad sampling atlas page `page` */
    ELI_FONT_GLYPH_TOFU,      /* the missing-glyph box, drawn as outline rects */
    ELI_FONT_GLYPH_CUSTOM     /* drawn by the font's custom_glyph_fn */
};

/**
 * A single glyph. Positions x0..y1 are pixel offsets from the pen origin
 * (which sits at the top-left of the text line); u0..v1 are normalized texture
 * coordinates into atlas page `page`. `visible` is false for whitespace glyphs
 * that advance the pen but produce no geometry, and for tofu/custom glyphs.
 */
typedef struct eli_font_glyph {
    uint32_t codepoint;
    bool     visible;
    uint8_t  kind;            /* eli_font_glyph_kind_ */
    uint16_t page;            /* atlas page holding the bitmap (kind ATLAS) */
    float    advance_x;
    float    x0, y0, x1, y1;
    float    u0, v0, u1, v1;
} eli_font_glyph;

/**
 * Draws a custom glyph (eli_font_add_custom_glyph_range) into its cell. The cell is
 * the glyph's advance wide and one line tall, in screen units at the drawn size.
 * Use the eli_draw_list_add_* primitives; the list's current texture has the white
 * texel at the usual UV.
 */
typedef void (*eli_font_custom_glyph_fn)(eli_draw_list *list, const eli_font *font,
                                         uint32_t codepoint, eli_vec2 cell_min,
                                         eli_vec2 cell_max, eli_col32 col, void *user_data);

/* ---------------------------------------------------------------------------
 * Font
 * ------------------------------------------------------------------------- */

/** Codepoints per block of the two-level codepoint -> glyph lookup. */
#define ELI_FONT_LOOKUP_BLOCK 256

/** Lookup entry: never looked up (on-demand atlases load it on first use). */
#define ELI_FONT_LOOKUP_UNKNOWN (-1)

/** Lookup entry: no face in the font's chain has it (the fallback is used). */
#define ELI_FONT_LOOKUP_MISSING (-2)

/** One registered custom-glyph codepoint range (re-applied after each build). */
typedef struct eli_font_custom_range {
    uint32_t lo, hi;
    float    advance_x;       /* <= 0: the font's cell advance */
} eli_font_custom_range;

/**
 * A font: metrics plus a glyph array and a two-level codepoint -> glyph-index
 * lookup (lookup_blocks[cp / 256][cp % 256], blocks allocated on use). Owned by
 * its container atlas; released by eli_font_atlas_clear_fonts.
 *
 * On an on-demand atlas a lookup may rasterize the glyph and grow `glyphs`, so a
 * glyph pointer stays valid only until the next lookup of a codepoint not yet
 * loaded (or a page eviction).
 */
struct eli_font {
    eli_font_atlas *container_atlas;

    float font_size;          /* pixel size this font was baked at */
    float ascent;             /* pixels above the baseline (positive) */
    float descent;            /* pixels below the baseline (negative) */
    float line_height;        /* full line advance in pixels */
    float fallback_advance_x; /* advance of the fallback glyph */
    uint32_t fallback_char;   /* codepoint drawn for a missing glyph; 0 = tofu box */
    float cell_advance_x;     /* advance of 'M' in the primary face (merge cell width) */

    eli_font_glyph *glyphs;
    int glyph_count;
    int glyph_capacity;

    int **lookup_blocks;      /* [cp / 256] -> 256 entries, NULL until used */
    int  lookup_block_count;

    const eli_font_glyph *fallback_glyph;
    eli_font_glyph tofu_glyph;   /* the missing-glyph box (kind TOFU) */
    eli_vec2 tex_uv_white_pixel; /* copied from the atlas for convenience */

    eli_font_custom_range *custom_ranges;
    int custom_range_count;
    eli_font_custom_glyph_fn custom_glyph_fn;
    void *custom_glyph_user_data;
};

/* ---------------------------------------------------------------------------
 * Font configuration
 * ------------------------------------------------------------------------- */

/**
 * Parameters describing one font source added to an atlas. `glyph_ranges` is a
 * zero-terminated list of inclusive [lo, hi] codepoint pairs: the codepoints baked
 * up front, or on an on-demand atlas the codepoints this source may supply (NULL
 * selects the atlas default range, or every codepoint on an on-demand atlas).
 * `font_data_owned_by_atlas` marks TTF memory the atlas must free on clear (used
 * for decompressed/embedded data). `rasterizer_density` bakes glyphs at
 * size_pixels x density texels while every metric stays in points, so text is
 * sharp on a HiDPI framebuffer and layout does not change (<= 0 means 1).
 *
 * `merge_mode` adds this source to the previously added font as a fallback: it
 * supplies only codepoints the earlier sources of that font lack, aligned to that
 * font's baseline. `merge_match_advance` (merge sources only) gives each of its
 * glyphs the font's cell advance (`cell_advance_x`), centred and shrunk about its
 * centre when wider, so a borrowed symbol never shifts monospace columns.
 */
struct eli_font_config {
    void *font_data;
    int   font_data_size;
    bool  font_data_owned_by_atlas;
    int   font_no;                    /* face index inside a TTC collection */
    float size_pixels;
    int   oversample_h;
    int   oversample_v;
    bool  pixel_snap_h;
    const uint16_t *glyph_ranges;
    float glyph_offset_x;
    float glyph_offset_y;
    float glyph_min_advance_x;
    float glyph_max_advance_x;
    float rasterizer_density;         /* texels per point (1 = default; 2 on Retina) */
    bool  merge_mode;                 /* fallback source for the previous font */
    bool  merge_match_advance;        /* merged glyphs take the font's cell advance */
    eli_font *dst_font;               /* font this config bakes into */
    char  name[ELI_FONT_NAME_MAX];
};

/* ---------------------------------------------------------------------------
 * Atlas
 * ------------------------------------------------------------------------- */

/** Atlas construction flags (mirrors a subset of Dear ImGui's flags). */
typedef int eli_font_atlas_flags;
enum eli_font_atlas_flags_ {
    ELI_FONT_ATLAS_NONE               = 0,
    ELI_FONT_ATLAS_NO_POWER_OF_TWO_HEIGHT = 1 << 0,
    ELI_FONT_ATLAS_NO_BAKED_LINES     = 1 << 1,
    /** Rasterize glyphs on first use into fixed-size pages (LRU-recycled, idle
     *  pages freed) instead of baking every range up front. Font data must stay
     *  alive for the atlas's lifetime. */
    ELI_FONT_ATLAS_ON_DEMAND          = 1 << 2
};

/**
 * Backend texture hooks. Set them (eli_font_atlas_set_backend) to let the atlas
 * create a page's texture the moment the page is allocated (mid-frame, so draw
 * commands get a valid id), upload changed regions in eli_font_atlas_update, and
 * destroy textures of freed pages. Pixels are single-channel coverage (A8).
 */
typedef struct eli_font_atlas_backend {
    void *user_data;
    eli_texture_id (*create_texture)(void *user_data, int width, int height,
                                     const unsigned char *alpha8);
    void (*update_texture)(void *user_data, eli_texture_id id, int x, int y, int width,
                           int height, const unsigned char *alpha8, int pitch);
    void (*destroy_texture)(void *user_data, eli_texture_id id);
} eli_font_atlas_backend;

/**
 * One atlas texture page. Page 0 always exists once built and holds the white
 * texel; on an on-demand atlas every page is page_size square with the white
 * block at the same texels, so the white UV is valid whichever page is bound.
 * `pixels` is A8 (NULL for a free slot, or after release_cpu_pixels on a
 * baked atlas). `version` bumps on every pixel change (for backends that mirror
 * the page onto more than one renderer).
 */
typedef struct eli_font_atlas_page {
    unsigned char *pixels;
    int width, height;
    eli_texture_id tex_id;
    void *packer;                     /* on-demand rect packer (opaque) */
    int dirty_x0, dirty_y0, dirty_x1, dirty_y1;  /* region to upload; empty if x1 <= x0 */
    uint32_t version;
    uint64_t last_used_frame;
    int glyph_count;
    bool live;                        /* false: a free slot */
} eli_font_atlas_page;

/**
 * Owns the rasterized font pages and the fonts baked into them. tex_* fields
 * mirror page 0 (the whole texture of a baked atlas). The rgba32 buffer is
 * produced on demand from page 0. Fonts and configs are heap arrays grown as
 * sources are added. Not built until eli_font_atlas_build (or an implicit build
 * inside a get_tex_data call).
 */
struct eli_font_atlas {
    eli_font_atlas_flags flags;
    eli_texture_id tex_id;

    int tex_width;
    int tex_height;
    int tex_desired_width;   /* 0 => choose automatically */
    int tex_glyph_padding;   /* spacing between packed glyphs */

    unsigned char *tex_pixels_alpha8;
    unsigned int  *tex_pixels_rgba32;

    eli_vec2 tex_uv_scale;      /* (1/width, 1/height) */
    eli_vec2 tex_uv_white_pixel;/* UV of an opaque white texel */

    eli_font **fonts;
    int fonts_count;
    int fonts_capacity;

    eli_font_config *configs;
    int configs_count;
    int configs_capacity;

    eli_font_atlas_page *pages;
    int pages_count;          /* slots, live or free */
    int pages_capacity;
    int page_size;            /* on-demand page side (0 => default) */
    int max_pages;            /* recycle threshold (0 => default) */
    int page_idle_frames;     /* free idle pages after this many frames (0 => default) */
    uint64_t frame_count;     /* advanced by eli_font_atlas_update */
    void *sources;            /* per-config rasterizer state (opaque) */
    eli_font_atlas_backend backend;

    bool built;
};

#endif /* ELI_FONT_ELI_FONT_TYPES_H */
