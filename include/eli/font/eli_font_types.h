/**
 * @file eli_font_types.h
 * @brief Font-system data types: glyph, font, font configuration, and the font
 *        atlas that owns rasterized texture data and the fonts baked into it.
 *
 * A glyph carries its advance plus the screen-space corner offsets (top-left
 * origin) and the atlas texture coordinates that a text quad is built from. A
 * font owns a flat glyph array plus a codepoint->glyph lookup table. The atlas
 * owns the single-channel (alpha8) texture, an optional RGBA32 copy, and the
 * list of fonts and configs that were baked into it.
 *
 * @status Phase 3 font types in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_FONT_ELI_FONT_TYPES_H
#define ELI_FONT_ELI_FONT_TYPES_H

#include "../core/eli_platform.h"
#include "../core/eli_types.h"

/* Forward typedefs. `eli_font` is also forward-declared by the core context;
 * C11 permits the identical typedef to be repeated here where it is defined. */
typedef struct eli_font        eli_font;
typedef struct eli_font_atlas  eli_font_atlas;
typedef struct eli_font_config eli_font_config;

/** Default pixel size used for the embedded ProggyClean font. */
#define ELI_FONT_DEFAULT_SIZE 13.0f

/** Default fallback codepoint rendered when a requested glyph is absent. */
#define ELI_FONT_FALLBACK_CHAR 0x003F /* '?' */

/** Length (in bytes) of the config name buffer. */
#define ELI_FONT_NAME_MAX 40

/* ---------------------------------------------------------------------------
 * Glyph
 * ------------------------------------------------------------------------- */

/**
 * A single baked glyph. Positions x0..y1 are pixel offsets from the pen origin
 * (which sits at the top-left of the text line); u0..v1 are normalized texture
 * coordinates into the atlas. `visible` is false for whitespace glyphs that
 * advance the pen but produce no geometry.
 */
typedef struct eli_font_glyph {
    uint32_t codepoint;
    bool     visible;
    float    advance_x;
    float    x0, y0, x1, y1;
    float    u0, v0, u1, v1;
} eli_font_glyph;

/* ---------------------------------------------------------------------------
 * Font
 * ------------------------------------------------------------------------- */

/**
 * A baked font: metrics plus a glyph array and a direct codepoint->glyph-index
 * lookup (index_lookup[c] holds the glyph index, or -1 when absent). Owned by
 * its container atlas; released by eli_font_atlas_clear_fonts.
 */
struct eli_font {
    eli_font_atlas *container_atlas;

    float font_size;          /* pixel size this font was baked at */
    float ascent;             /* pixels above the baseline (positive) */
    float descent;            /* pixels below the baseline (negative) */
    float line_height;        /* full line advance in pixels */
    float fallback_advance_x; /* advance of the fallback glyph */
    uint32_t fallback_char;   /* codepoint used when a glyph is missing */

    eli_font_glyph *glyphs;
    int glyph_count;
    int glyph_capacity;

    int *index_lookup;        /* codepoint -> glyph index, -1 if absent */
    int  index_lookup_size;

    const eli_font_glyph *fallback_glyph;
    eli_vec2 tex_uv_white_pixel; /* copied from the atlas for convenience */
};

/* ---------------------------------------------------------------------------
 * Font configuration
 * ------------------------------------------------------------------------- */

/**
 * Parameters describing one font source added to an atlas. `glyph_ranges` is a
 * zero-terminated list of inclusive [lo, hi] codepoint pairs (NULL selects the
 * atlas default range). `font_data_owned_by_atlas` marks TTF memory the atlas
 * must free on clear (used for decompressed/embedded data).
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
    ELI_FONT_ATLAS_NO_BAKED_LINES     = 1 << 1
};

/**
 * Owns the rasterized font texture and the fonts baked into it. The alpha8
 * buffer is the source of truth; the rgba32 buffer is produced on demand. Fonts
 * and configs are heap arrays grown as sources are added. Not built until
 * eli_font_atlas_build (or an implicit build inside a get_tex_data call).
 */
struct eli_font_atlas {
    eli_font_atlas_flags flags;
    uint32_t tex_id;

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

    bool built;
};

#endif /* ELI_FONT_ELI_FONT_TYPES_H */
