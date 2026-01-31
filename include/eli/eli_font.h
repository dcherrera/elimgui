/*
 * eli_font.h - Font System
 *
 * Font loading, atlas generation, and text rendering.
 */

#ifndef ELI_FONT_H
#define ELI_FONT_H

#include "elimgui.h"
#include "eli_draw.h"

/*============================================================================
 * STB TRUETYPE CONFIGURATION
 *===========================================================================*/

/* Configure stb_truetype to use our allocators */
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_malloc(x,u)  ((void)(u),malloc(x))
#define STBTT_free(x,u)    ((void)(u),free(x))
#define STBTT_assert(x)    ((void)(x))

/* Configure stb_rect_pack */
#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#define STBRP_ASSERT(x)    ((void)(x))

/* Include stb libraries */
#include "../../vendor/stb/stb_rect_pack.h"
#include "../../vendor/stb/stb_truetype.h"

/* Include embedded default font */
#include "eli_font_proggy.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * FORWARD DECLARATIONS
 *===========================================================================*/

typedef struct eli_font eli_font;
typedef struct eli_font_atlas eli_font_atlas;
typedef struct eli_font_glyph eli_font_glyph;
typedef struct eli_font_config eli_font_config;

/*============================================================================
 * FONT ATLAS FLAGS
 *===========================================================================*/

typedef enum eli_font_atlas_flags {
    ELI_FONT_ATLAS_FLAGS_NONE                = 0,
    ELI_FONT_ATLAS_FLAGS_NO_POWER_OF_TWO_HEIGHT = 1 << 0,
    ELI_FONT_ATLAS_FLAGS_NO_MOUSE_CURSORS    = 1 << 1,
    ELI_FONT_ATLAS_FLAGS_NO_BAKED_LINES      = 1 << 2
} eli_font_atlas_flags;

/*============================================================================
 * FONT GLYPH
 *===========================================================================*/

struct eli_font_glyph {
    /* Codepoint this glyph represents */
    uint32_t codepoint;

    /* Flags */
    bool visible;       /* Has visible pixels */
    bool colored;       /* Colored glyph (ignore tinting) */

    /* Advance and positioning */
    float advance_x;    /* Horizontal advance */
    float x0, y0;       /* Top-left corner relative to cursor */
    float x1, y1;       /* Bottom-right corner relative to cursor */

    /* Texture coordinates */
    float u0, v0;       /* Top-left UV */
    float u1, v1;       /* Bottom-right UV */
};

/*============================================================================
 * FONT CONFIG
 *===========================================================================*/

struct eli_font_config {
    /* Input */
    void* font_data;                /* TTF/OTF data */
    int font_data_size;             /* Size of font data */
    bool font_data_owned_by_atlas;  /* Atlas owns memory */

    /* Rendering options */
    float size_pixels;              /* Desired size in pixels */
    int oversample_h;               /* Horizontal oversampling (1-4) */
    int oversample_v;               /* Vertical oversampling (1-4) */
    bool pixel_snap_h;              /* Snap to pixel boundaries */

    /* Glyph options */
    eli_vec2 glyph_offset;          /* Offset all glyphs */
    float glyph_min_advance_x;      /* Minimum advance */
    float glyph_max_advance_x;      /* Maximum advance (FLT_MAX = no limit) */
    const uint16_t* glyph_ranges;   /* Unicode ranges (pairs, 0-terminated) */

    /* Merging */
    bool merge_mode;                /* Merge into previous font */
    float rasterizer_multiply;      /* Brighten/darken (1.0 = normal) */
    float rasterizer_density;       /* DPI scale (1.0 = normal) */

    /* Font selection */
    int font_no;                    /* Index within TTF/OTF file */

    /* Special characters */
    uint32_t ellipsis_char;         /* Codepoint for '...' (0 = auto) */

    /* Name for debugging */
    char name[40];

    /* Output (set by AddFont) */
    eli_font* dst_font;
};

/*============================================================================
 * FONT ATLAS CUSTOM RECT
 *===========================================================================*/

typedef struct eli_font_atlas_custom_rect {
    uint16_t width, height;     /* Input: desired dimensions */
    uint16_t x, y;              /* Output: position in texture */
    uint32_t glyph_id;          /* Optional: glyph identifier */
    float glyph_advance_x;      /* Optional: advance */
    eli_vec2 glyph_offset;      /* Optional: glyph offset */
    eli_font* font;             /* Optional: font for glyph */
} eli_font_atlas_custom_rect;

/*============================================================================
 * FONT ATLAS
 *===========================================================================*/

/* Dynamic array for fonts */
typedef struct eli_font_ptr_array {
    eli_font** data;
    int size;
    int capacity;
} eli_font_ptr_array;

/* Dynamic array for configs */
typedef struct eli_font_config_array {
    eli_font_config* data;
    int size;
    int capacity;
} eli_font_config_array;

/* Dynamic array for custom rects */
typedef struct eli_font_atlas_custom_rect_array {
    eli_font_atlas_custom_rect* data;
    int size;
    int capacity;
} eli_font_atlas_custom_rect_array;

struct eli_font_atlas {
    /* Flags */
    eli_font_atlas_flags flags;

    /* Texture settings */
    eli_texture_id tex_id;          /* User texture ID */
    int tex_desired_width;          /* Desired width (power of 2) */
    int tex_glyph_padding;          /* Padding between glyphs */

    /* Texture output */
    unsigned char* tex_pixels_alpha8;   /* 1-byte per pixel */
    unsigned char* tex_pixels_rgba32;   /* 4-bytes per pixel */
    int tex_width;                  /* Actual width */
    int tex_height;                 /* Actual height */
    eli_vec2 tex_uv_scale;          /* (1.0/w, 1.0/h) */
    eli_vec2 tex_uv_white_pixel;    /* White pixel UV */
    bool tex_is_built;              /* Texture matches font input */

    /* Fonts */
    eli_font_ptr_array fonts;

    /* Config data */
    eli_font_config_array config_data;

    /* Custom rectangles */
    eli_font_atlas_custom_rect_array custom_rects;

    /* For building */
    int pack_id_mouse_cursors;
    int pack_id_lines;
};

/*============================================================================
 * FONT
 *===========================================================================*/

/* Dynamic array for glyphs */
typedef struct eli_font_glyph_array {
    eli_font_glyph* data;
    int size;
    int capacity;
} eli_font_glyph_array;

struct eli_font {
    /* Glyph data */
    eli_font_glyph_array glyphs;

    /* Lookup tables */
    float* index_advance_x;         /* Sparse advance array */
    int index_advance_x_size;
    uint16_t* index_lookup;         /* Sparse glyph index lookup */
    int index_lookup_size;

    /* Fallback */
    float fallback_advance_x;
    uint16_t fallback_glyph;
    uint32_t fallback_char;

    /* Metrics */
    float size;                     /* Font size (height) */
    float ascent;                   /* Pixels above baseline */
    float descent;                  /* Pixels below baseline (negative) */
    float scale;                    /* 1.0 */

    /* Parent atlas */
    eli_font_atlas* container_atlas;

    /* Config data */
    eli_font_config* config_data;
    int config_data_count;

    /* Special characters */
    uint32_t ellipsis_char;
    int16_t ellipsis_char_count;
    float ellipsis_width;
    float ellipsis_char_step;

    /* Debug name */
    const char* debug_name;

    /* Metrics */
    int metrics_total_surface;
    bool dirty_lookup_tables;
    bool used_8k_pages_map[2];     /* Bitmap of used Unicode pages */
};

/*============================================================================
 * GLYPH RANGES
 *===========================================================================*/

/* Standard glyph ranges (0-terminated arrays of pairs) */
static inline const uint16_t* eli_font_atlas_get_glyph_ranges_default(void) {
    static const uint16_t ranges[] = {
        0x0020, 0x00FF,  /* Basic Latin + Latin Supplement */
        0
    };
    return ranges;
}

static inline const uint16_t* eli_font_atlas_get_glyph_ranges_greek(void) {
    static const uint16_t ranges[] = {
        0x0020, 0x00FF,  /* Basic Latin + Latin Supplement */
        0x0370, 0x03FF,  /* Greek and Coptic */
        0
    };
    return ranges;
}

static inline const uint16_t* eli_font_atlas_get_glyph_ranges_cyrillic(void) {
    static const uint16_t ranges[] = {
        0x0020, 0x00FF,  /* Basic Latin + Latin Supplement */
        0x0400, 0x052F,  /* Cyrillic + Cyrillic Supplement */
        0x2DE0, 0x2DFF,  /* Cyrillic Extended-A */
        0xA640, 0xA69F,  /* Cyrillic Extended-B */
        0
    };
    return ranges;
}

static inline const uint16_t* eli_font_atlas_get_glyph_ranges_korean(void) {
    static const uint16_t ranges[] = {
        0x0020, 0x00FF,  /* Basic Latin + Latin Supplement */
        0x3131, 0x3163,  /* Korean Hangul Compatibility Jamo */
        0xAC00, 0xD7A3,  /* Korean Hangul Syllables */
        0xFFFD, 0xFFFD,  /* Replacement character */
        0
    };
    return ranges;
}

static inline const uint16_t* eli_font_atlas_get_glyph_ranges_japanese(void) {
    static const uint16_t ranges[] = {
        0x0020, 0x00FF,  /* Basic Latin + Latin Supplement */
        0x3000, 0x30FF,  /* CJK Symbols, Hiragana, Katakana */
        0x31F0, 0x31FF,  /* Katakana Phonetic Extensions */
        0xFF00, 0xFFEF,  /* Half and Full width characters */
        0xFFFD, 0xFFFD,  /* Replacement character */
        0
    };
    return ranges;
}

static inline const uint16_t* eli_font_atlas_get_glyph_ranges_chinese_simplified_common(void) {
    static const uint16_t ranges[] = {
        0x0020, 0x00FF,  /* Basic Latin + Latin Supplement */
        0x2000, 0x206F,  /* General Punctuation */
        0x3000, 0x30FF,  /* CJK Symbols, Hiragana, Katakana */
        0x31F0, 0x31FF,  /* Katakana Phonetic Extensions */
        0xFF00, 0xFFEF,  /* Half and Full width characters */
        0xFFFD, 0xFFFD,  /* Replacement character */
        /* Common Chinese simplified characters range */
        0x4E00, 0x9FFF,  /* CJK Unified Ideographs */
        0
    };
    return ranges;
}

static inline const uint16_t* eli_font_atlas_get_glyph_ranges_chinese_full(void) {
    static const uint16_t ranges[] = {
        0x0020, 0x00FF,  /* Basic Latin + Latin Supplement */
        0x2000, 0x206F,  /* General Punctuation */
        0x3000, 0x30FF,  /* CJK Symbols, Hiragana, Katakana */
        0x31F0, 0x31FF,  /* Katakana Phonetic Extensions */
        0xFF00, 0xFFEF,  /* Half and Full width characters */
        0xFFFD, 0xFFFD,  /* Replacement character */
        0x4E00, 0x9FFF,  /* CJK Unified Ideographs */
        0x2F00, 0x2FDF,  /* CJK Radicals Supplement */
        0x2E80, 0x2EFF,  /* CJK Radicals Supplement */
        0x3100, 0x312F,  /* Bopomofo */
        0x3400, 0x4DBF,  /* CJK Unified Ideographs Extension A */
        0
    };
    return ranges;
}

static inline const uint16_t* eli_font_atlas_get_glyph_ranges_thai(void) {
    static const uint16_t ranges[] = {
        0x0020, 0x00FF,  /* Basic Latin + Latin Supplement */
        0x0E00, 0x0E7F,  /* Thai */
        0
    };
    return ranges;
}

static inline const uint16_t* eli_font_atlas_get_glyph_ranges_vietnamese(void) {
    static const uint16_t ranges[] = {
        0x0020, 0x00FF,  /* Basic Latin + Latin Supplement */
        0x0102, 0x0103,  /* Latin Extended-A */
        0x0110, 0x0111,
        0x0128, 0x0129,
        0x0168, 0x0169,
        0x01A0, 0x01A1,
        0x01AF, 0x01B0,
        0x1EA0, 0x1EF9,  /* Latin Extended Additional */
        0
    };
    return ranges;
}

/*============================================================================
 * FONT CONFIG INITIALIZATION
 *===========================================================================*/

static inline void eli_font_config_init(eli_font_config* config) {
    config->font_data = NULL;
    config->font_data_size = 0;
    config->font_data_owned_by_atlas = true;
    config->size_pixels = 13.0f;
    config->oversample_h = 3;
    config->oversample_v = 1;
    config->pixel_snap_h = false;
    config->glyph_offset = eli_make_vec2(0, 0);
    config->glyph_min_advance_x = 0.0f;
    config->glyph_max_advance_x = 1e30f;
    config->glyph_ranges = NULL;
    config->merge_mode = false;
    config->rasterizer_multiply = 1.0f;
    config->rasterizer_density = 1.0f;
    config->font_no = 0;
    config->ellipsis_char = 0;
    config->name[0] = '\0';
    config->dst_font = NULL;
}

/*============================================================================
 * FONT ATLAS INITIALIZATION
 *===========================================================================*/

static inline void eli_font_atlas_init(eli_font_atlas* atlas) {
    atlas->flags = ELI_FONT_ATLAS_FLAGS_NONE;
    atlas->tex_id = NULL;
    atlas->tex_desired_width = 0;
    atlas->tex_glyph_padding = 1;
    atlas->tex_pixels_alpha8 = NULL;
    atlas->tex_pixels_rgba32 = NULL;
    atlas->tex_width = 0;
    atlas->tex_height = 0;
    atlas->tex_uv_scale = eli_make_vec2(0, 0);
    atlas->tex_uv_white_pixel = eli_make_vec2(0, 0);
    atlas->tex_is_built = false;

    eli_vector_init(&atlas->fonts);
    eli_vector_init(&atlas->config_data);
    eli_vector_init(&atlas->custom_rects);

    atlas->pack_id_mouse_cursors = -1;
    atlas->pack_id_lines = -1;
}

static inline void eli_font_atlas_destroy(eli_font_atlas* atlas) {
    /* Free texture data */
    if (atlas->tex_pixels_alpha8) {
        free(atlas->tex_pixels_alpha8);
        atlas->tex_pixels_alpha8 = NULL;
    }
    if (atlas->tex_pixels_rgba32) {
        free(atlas->tex_pixels_rgba32);
        atlas->tex_pixels_rgba32 = NULL;
    }

    /* Free fonts */
    for (int i = 0; i < atlas->fonts.size; i++) {
        eli_font* font = atlas->fonts.data[i];
        if (font) {
            eli_vector_free(&font->glyphs);
            if (font->index_advance_x) free(font->index_advance_x);
            if (font->index_lookup) free(font->index_lookup);
            free(font);
        }
    }
    eli_vector_free(&atlas->fonts);

    /* Free config data */
    for (int i = 0; i < atlas->config_data.size; i++) {
        if (atlas->config_data.data[i].font_data_owned_by_atlas &&
            atlas->config_data.data[i].font_data) {
            free(atlas->config_data.data[i].font_data);
        }
    }
    eli_vector_free(&atlas->config_data);

    eli_vector_free(&atlas->custom_rects);
}

static inline void eli_font_atlas_clear_input_data(eli_font_atlas* atlas) {
    for (int i = 0; i < atlas->config_data.size; i++) {
        if (atlas->config_data.data[i].font_data_owned_by_atlas &&
            atlas->config_data.data[i].font_data) {
            free(atlas->config_data.data[i].font_data);
            atlas->config_data.data[i].font_data = NULL;
        }
    }

    /* Clear fonts' config pointers */
    for (int i = 0; i < atlas->fonts.size; i++) {
        atlas->fonts.data[i]->config_data = NULL;
        atlas->fonts.data[i]->config_data_count = 0;
    }

    eli_vector_clear(&atlas->config_data);
    eli_vector_clear(&atlas->custom_rects);
    atlas->tex_is_built = false;
}

static inline void eli_font_atlas_clear_tex_data(eli_font_atlas* atlas) {
    if (atlas->tex_pixels_alpha8) {
        free(atlas->tex_pixels_alpha8);
        atlas->tex_pixels_alpha8 = NULL;
    }
    if (atlas->tex_pixels_rgba32) {
        free(atlas->tex_pixels_rgba32);
        atlas->tex_pixels_rgba32 = NULL;
    }
}

static inline void eli_font_atlas_clear_fonts(eli_font_atlas* atlas) {
    for (int i = 0; i < atlas->fonts.size; i++) {
        eli_font* font = atlas->fonts.data[i];
        if (font) {
            eli_vector_free(&font->glyphs);
            if (font->index_advance_x) free(font->index_advance_x);
            if (font->index_lookup) free(font->index_lookup);
            free(font);
        }
    }
    eli_vector_clear(&atlas->fonts);
}

static inline void eli_font_atlas_clear(eli_font_atlas* atlas) {
    eli_font_atlas_clear_input_data(atlas);
    eli_font_atlas_clear_tex_data(atlas);
    eli_font_atlas_clear_fonts(atlas);
}

/*============================================================================
 * FONT INITIALIZATION
 *===========================================================================*/

static inline void eli_font_init(eli_font* font) {
    eli_vector_init(&font->glyphs);
    font->index_advance_x = NULL;
    font->index_advance_x_size = 0;
    font->index_lookup = NULL;
    font->index_lookup_size = 0;
    font->fallback_advance_x = 0.0f;
    font->fallback_glyph = 0;
    font->fallback_char = 0xFFFD;  /* Replacement character */
    font->size = 0.0f;
    font->ascent = 0.0f;
    font->descent = 0.0f;
    font->scale = 1.0f;
    font->container_atlas = NULL;
    font->config_data = NULL;
    font->config_data_count = 0;
    font->ellipsis_char = 0x2026;  /* ... */
    font->ellipsis_char_count = 1;
    font->ellipsis_width = 0.0f;
    font->ellipsis_char_step = 0.0f;
    font->debug_name = NULL;
    font->metrics_total_surface = 0;
    font->dirty_lookup_tables = true;
    font->used_8k_pages_map[0] = 0;
    font->used_8k_pages_map[1] = 0;
}

/*============================================================================
 * FONT GLYPH LOOKUP
 *===========================================================================*/

static inline const eli_font_glyph* eli_font_find_glyph(eli_font* font, uint32_t c) {
    if (c < (uint32_t)font->index_lookup_size) {
        uint16_t idx = font->index_lookup[c];
        if (idx != 0xFFFF) {
            return &font->glyphs.data[idx];
        }
    }
    /* Return fallback glyph */
    if (font->fallback_glyph < font->glyphs.size) {
        return &font->glyphs.data[font->fallback_glyph];
    }
    return NULL;
}

static inline const eli_font_glyph* eli_font_find_glyph_no_fallback(eli_font* font, uint32_t c) {
    if (c < (uint32_t)font->index_lookup_size) {
        uint16_t idx = font->index_lookup[c];
        if (idx != 0xFFFF) {
            return &font->glyphs.data[idx];
        }
    }
    return NULL;
}

static inline float eli_font_get_char_advance(eli_font* font, uint32_t c) {
    if (c < (uint32_t)font->index_advance_x_size) {
        return font->index_advance_x[c];
    }
    return font->fallback_advance_x;
}

static inline bool eli_font_is_glyph_loaded(eli_font* font, uint32_t c) {
    return eli_font_find_glyph_no_fallback(font, c) != NULL;
}

static inline bool eli_font_is_loaded(eli_font* font) {
    return font->container_atlas != NULL;
}

/*============================================================================
 * FONT ATLAS TEXTURE ACCESS
 *===========================================================================*/

static inline void eli_font_atlas_get_tex_data_as_alpha8(eli_font_atlas* atlas,
    unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    /* Build if needed */
    if (!atlas->tex_pixels_alpha8) {
        /* TODO: Build texture */
    }

    *out_pixels = atlas->tex_pixels_alpha8;
    if (out_width) *out_width = atlas->tex_width;
    if (out_height) *out_height = atlas->tex_height;
    if (out_bytes_per_pixel) *out_bytes_per_pixel = 1;
}

static inline void eli_font_atlas_get_tex_data_as_rgba32(eli_font_atlas* atlas,
    unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    /* Convert to RGBA32 if needed */
    if (!atlas->tex_pixels_rgba32) {
        if (!atlas->tex_pixels_alpha8) {
            /* TODO: Build texture */
        }

        if (atlas->tex_pixels_alpha8) {
            int size = atlas->tex_width * atlas->tex_height;
            atlas->tex_pixels_rgba32 = (unsigned char*)malloc(size * 4);
            if (atlas->tex_pixels_rgba32) {
                const unsigned char* src = atlas->tex_pixels_alpha8;
                unsigned char* dst = atlas->tex_pixels_rgba32;
                for (int i = 0; i < size; i++) {
                    dst[0] = 255;
                    dst[1] = 255;
                    dst[2] = 255;
                    dst[3] = src[0];
                    src++;
                    dst += 4;
                }
            }
        }
    }

    *out_pixels = atlas->tex_pixels_rgba32;
    if (out_width) *out_width = atlas->tex_width;
    if (out_height) *out_height = atlas->tex_height;
    if (out_bytes_per_pixel) *out_bytes_per_pixel = 4;
}

static inline bool eli_font_atlas_is_built(eli_font_atlas* atlas) {
    return atlas->tex_is_built;
}

static inline void eli_font_atlas_set_tex_id(eli_font_atlas* atlas, eli_texture_id id) {
    atlas->tex_id = id;
}

/*============================================================================
 * TEXT SIZE CALCULATION
 *===========================================================================*/

static inline eli_vec2 eli_calc_text_size(eli_font* font, float size, float max_width,
    float wrap_width, const char* text, const char* text_end)
{
    if (!text) text = "";
    if (!text_end) text_end = text + strlen(text);

    float scale = size / font->size;
    float line_height = size;

    float text_width = 0.0f;
    float max_text_width = 0.0f;
    float line_width = 0.0f;
    int line_count = 1;

    const char* s = text;
    while (s < text_end) {
        unsigned int c = (unsigned char)*s;

        if (c == '\n') {
            if (line_width > max_text_width) max_text_width = line_width;
            line_width = 0.0f;
            line_count++;
            s++;
            continue;
        }

        /* Simple ASCII for now */
        if (c < 128) {
            float char_width = eli_font_get_char_advance(font, c) * scale;
            line_width += char_width;
        }

        s++;
    }

    if (line_width > max_text_width) max_text_width = line_width;

    eli_vec2 result;
    result.x = max_text_width;
    result.y = line_height * line_count;
    return result;
}

/*============================================================================
 * TEXT RENDERING
 *===========================================================================*/

static inline void eli_draw_list_add_text(eli_draw_list* list, eli_font* font,
    float font_size, eli_vec2 pos, uint32_t col, const char* text, const char* text_end,
    float wrap_width, const eli_vec4* cpu_fine_clip_rect)
{
    if ((col & ELI_COL32_A_MASK) == 0) return;
    if (!text) return;
    if (!text_end) text_end = text + strlen(text);
    if (text == text_end) return;
    if (!font) return;
    if (!font->container_atlas) return;

    float scale = font_size / font->size;
    float x = pos.x;
    float y = pos.y;

    /* Apply clipping if provided */
    eli_vec4 clip;
    if (cpu_fine_clip_rect) {
        clip = *cpu_fine_clip_rect;
    } else {
        clip = list->_cmd_header.clip_rect;
    }

    const char* s = text;
    while (s < text_end) {
        unsigned int c = (unsigned char)*s;

        if (c == '\n') {
            x = pos.x;
            y += font_size;
            s++;
            continue;
        }

        const eli_font_glyph* glyph = eli_font_find_glyph(font, c);
        if (!glyph) {
            s++;
            continue;
        }

        if (glyph->visible) {
            /* Calculate glyph rectangle */
            float x1 = x + glyph->x0 * scale;
            float y1 = y + glyph->y0 * scale;
            float x2 = x + glyph->x1 * scale;
            float y2 = y + glyph->y1 * scale;

            /* Clip check */
            if (x2 >= clip.x && x1 <= clip.z && y2 >= clip.y && y1 <= clip.w) {
                /* Add textured quad */
                eli_draw_list_prim_reserve(list, 6, 4);
                eli_draw_list_prim_rect_uv(list,
                    eli_make_vec2(x1, y1),
                    eli_make_vec2(x2, y2),
                    eli_make_vec2(glyph->u0, glyph->v0),
                    eli_make_vec2(glyph->u1, glyph->v1),
                    col);
            }
        }

        x += glyph->advance_x * scale;
        s++;
    }
}

/* Simplified version that uses current font from context */
static inline void eli_draw_list_add_text_simple(eli_draw_list* list,
    eli_vec2 pos, uint32_t col, const char* text)
{
    eli_context* ctx = eli_get_current_context();
    if (!ctx || !ctx->font) return;
    eli_draw_list_add_text(list, ctx->font, ctx->font_size, pos, col, text, NULL, 0.0f, NULL);
}

/*============================================================================
 * FONT STACK (context-based)
 *===========================================================================*/

/* These require the context to have a font stack - defined as stubs for now */
static inline void eli_push_font(eli_font* font) {
    eli_context* ctx = eli_get_current_context();
    if (ctx && font) {
        ctx->font = font;
        ctx->font_size = font->size;
    }
}

static inline void eli_pop_font(void) {
    /* Would restore from stack - simplified for now */
}

static inline eli_font* eli_get_font(void) {
    eli_context* ctx = eli_get_current_context();
    return ctx ? ctx->font : NULL;
}

static inline float eli_get_font_size(void) {
    eli_context* ctx = eli_get_current_context();
    return ctx ? ctx->font_size : 13.0f;
}

static inline eli_vec2 eli_get_font_tex_uv_white_pixel(void) {
    eli_context* ctx = eli_get_current_context();
    if (ctx && ctx->font && ctx->font->container_atlas) {
        return ctx->font->container_atlas->tex_uv_white_pixel;
    }
    return eli_make_vec2(0, 0);
}

/*============================================================================
 * FONT ATLAS ADD FONT
 *===========================================================================*/

static inline eli_font* eli_font_atlas_add_font_from_memory_ttf(
    eli_font_atlas* atlas,
    void* font_data,
    int font_data_size,
    float size_pixels,
    const eli_font_config* font_cfg_template,
    const uint16_t* glyph_ranges)
{
    /* Create config */
    eli_font_config cfg;
    if (font_cfg_template) {
        cfg = *font_cfg_template;
    } else {
        eli_font_config_init(&cfg);
    }

    cfg.font_data = font_data;
    cfg.font_data_size = font_data_size;
    cfg.size_pixels = size_pixels;
    if (!cfg.glyph_ranges) {
        cfg.glyph_ranges = glyph_ranges ? glyph_ranges : eli_font_atlas_get_glyph_ranges_default();
    }

    /* Create new font */
    eli_font* font = (eli_font*)malloc(sizeof(eli_font));
    if (!font) return NULL;
    eli_font_init(font);

    font->size = size_pixels;
    font->container_atlas = atlas;

    /* Store config */
    cfg.dst_font = font;
    eli_vector_push(&atlas->config_data, cfg, eli_font_config);
    font->config_data = &atlas->config_data.data[atlas->config_data.size - 1];
    font->config_data_count = 1;

    /* Add font to atlas */
    eli_vector_push(&atlas->fonts, font, eli_font*);

    /* Mark atlas as needing rebuild */
    atlas->tex_is_built = false;

    return font;
}

/*============================================================================
 * FONT ATLAS BUILD
 *===========================================================================*/

static inline bool eli_font_atlas_build(eli_font_atlas* atlas) {
    if (atlas->config_data.size == 0) return false;

    /* Determine texture size */
    int tex_width = atlas->tex_desired_width > 0 ? atlas->tex_desired_width : 512;
    int tex_height = 512;

    /* Allocate texture */
    atlas->tex_pixels_alpha8 = (unsigned char*)malloc(tex_width * tex_height);
    if (!atlas->tex_pixels_alpha8) return false;
    memset(atlas->tex_pixels_alpha8, 0, tex_width * tex_height);

    /* Initialize packing context */
    stbtt_pack_context spc;
    if (!stbtt_PackBegin(&spc, atlas->tex_pixels_alpha8, tex_width, tex_height, 0, atlas->tex_glyph_padding, NULL)) {
        free(atlas->tex_pixels_alpha8);
        atlas->tex_pixels_alpha8 = NULL;
        return false;
    }

    /* Process each font config */
    for (int cfg_i = 0; cfg_i < atlas->config_data.size; cfg_i++) {
        eli_font_config* cfg = &atlas->config_data.data[cfg_i];
        eli_font* font = cfg->dst_font;
        if (!cfg->font_data || !font) continue;

        /* Set oversampling */
        stbtt_PackSetOversampling(&spc, cfg->oversample_h, cfg->oversample_v);

        /* Count glyphs needed */
        const uint16_t* ranges = cfg->glyph_ranges;
        if (!ranges) ranges = eli_font_atlas_get_glyph_ranges_default();

        int total_glyphs = 0;
        for (const uint16_t* r = ranges; r[0] && r[1]; r += 2) {
            total_glyphs += (r[1] - r[0]) + 1;
        }

        /* Allocate packed char data */
        stbtt_packedchar* chardata = (stbtt_packedchar*)malloc(total_glyphs * sizeof(stbtt_packedchar));
        if (!chardata) continue;

        /* Build pack ranges */
        int range_count = 0;
        for (const uint16_t* r = ranges; r[0] && r[1]; r += 2) range_count++;

        stbtt_pack_range* pack_ranges = (stbtt_pack_range*)malloc(range_count * sizeof(stbtt_pack_range));
        if (!pack_ranges) { free(chardata); continue; }

        int char_idx = 0;
        int range_idx = 0;
        for (const uint16_t* r = ranges; r[0] && r[1]; r += 2) {
            pack_ranges[range_idx].font_size = cfg->size_pixels;
            pack_ranges[range_idx].first_unicode_codepoint_in_range = r[0];
            pack_ranges[range_idx].array_of_unicode_codepoints = NULL;
            pack_ranges[range_idx].num_chars = (r[1] - r[0]) + 1;
            pack_ranges[range_idx].chardata_for_range = &chardata[char_idx];
            char_idx += pack_ranges[range_idx].num_chars;
            range_idx++;
        }

        /* Pack font ranges */
        stbtt_PackFontRanges(&spc, (unsigned char*)cfg->font_data, cfg->font_no, pack_ranges, range_count);

        /* Get font metrics */
        stbtt_fontinfo info;
        stbtt_InitFont(&info, (unsigned char*)cfg->font_data, stbtt_GetFontOffsetForIndex((unsigned char*)cfg->font_data, cfg->font_no));
        float scale = stbtt_ScaleForPixelHeight(&info, cfg->size_pixels);
        int ascent, descent, line_gap;
        stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);
        font->ascent = ascent * scale;
        font->descent = descent * scale;

        /* Build glyph data */
        char_idx = 0;
        for (int ri = 0; ri < range_count; ri++) {
            for (int ci = 0; ci < pack_ranges[ri].num_chars; ci++) {
                stbtt_packedchar* pc = &chardata[char_idx++];
                uint32_t codepoint = pack_ranges[ri].first_unicode_codepoint_in_range + ci;

                eli_font_glyph glyph;
                glyph.codepoint = codepoint;
                glyph.visible = (pc->x1 > pc->x0);
                glyph.colored = false;
                glyph.advance_x = pc->xadvance;
                glyph.x0 = pc->xoff;
                glyph.y0 = pc->yoff + font->ascent;
                glyph.x1 = pc->xoff2;
                glyph.y1 = pc->yoff2 + font->ascent;
                glyph.u0 = (float)pc->x0 / tex_width;
                glyph.v0 = (float)pc->y0 / tex_height;
                glyph.u1 = (float)pc->x1 / tex_width;
                glyph.v1 = (float)pc->y1 / tex_height;

                eli_vector_push(&font->glyphs, glyph, eli_font_glyph);
            }
        }

        free(pack_ranges);
        free(chardata);
    }

    stbtt_PackEnd(&spc);

    /* Build lookup tables for all fonts */
    for (int fi = 0; fi < atlas->fonts.size; fi++) {
        eli_font* font = atlas->fonts.data[fi];
        if (font->glyphs.size == 0) continue;

        /* Find max codepoint */
        uint32_t max_cp = 0;
        for (int gi = 0; gi < font->glyphs.size; gi++) {
            if (font->glyphs.data[gi].codepoint > max_cp)
                max_cp = font->glyphs.data[gi].codepoint;
        }

        /* Allocate lookup tables */
        font->index_lookup_size = max_cp + 1;
        font->index_lookup = (uint16_t*)malloc(font->index_lookup_size * sizeof(uint16_t));
        font->index_advance_x_size = max_cp + 1;
        font->index_advance_x = (float*)malloc(font->index_advance_x_size * sizeof(float));

        if (font->index_lookup && font->index_advance_x) {
            memset(font->index_lookup, 0xFF, font->index_lookup_size * sizeof(uint16_t));
            for (int gi = 0; gi < font->glyphs.size; gi++) {
                uint32_t cp = font->glyphs.data[gi].codepoint;
                font->index_lookup[cp] = gi;
                font->index_advance_x[cp] = font->glyphs.data[gi].advance_x;
            }
        }

        /* Set fallback glyph */
        const eli_font_glyph* fallback = eli_font_find_glyph_no_fallback(font, '?');
        if (!fallback) fallback = eli_font_find_glyph_no_fallback(font, ' ');
        if (fallback) {
            font->fallback_glyph = font->index_lookup[fallback->codepoint];
            font->fallback_advance_x = fallback->advance_x;
        }

        font->dirty_lookup_tables = false;
    }

    /* Set atlas properties */
    atlas->tex_width = tex_width;
    atlas->tex_height = tex_height;
    atlas->tex_uv_scale = eli_make_vec2(1.0f / tex_width, 1.0f / tex_height);
    atlas->tex_uv_white_pixel = eli_make_vec2(0.5f / tex_width, 0.5f / tex_height);
    atlas->tex_is_built = true;

    return true;
}

/*============================================================================
 * FONT ATLAS ADD DEFAULT FONT
 *===========================================================================*/

static inline eli_font* eli_font_atlas_add_font_default(eli_font_atlas* atlas, const eli_font_config* font_cfg) {
    /* Get decompressed ProggyClean TTF data */
    int font_data_size = 0;
    void* font_data = eli_get_default_font_data(&font_data_size);
    if (!font_data) return NULL;

    /* Create config */
    eli_font_config cfg;
    if (font_cfg) {
        cfg = *font_cfg;
    } else {
        eli_font_config_init(&cfg);
        cfg.size_pixels = 13.0f;  /* Default size for ProggyClean */
    }

    /* ProggyClean is a pixel-perfect font, disable oversampling */
    cfg.oversample_h = 1;
    cfg.oversample_v = 1;
    cfg.pixel_snap_h = true;

    /* Atlas owns this memory */
    cfg.font_data_owned_by_atlas = true;

    return eli_font_atlas_add_font_from_memory_ttf(atlas, font_data, font_data_size,
        cfg.size_pixels, &cfg, cfg.glyph_ranges);
}

#ifdef __cplusplus
}
#endif

#endif /* ELI_FONT_H */
