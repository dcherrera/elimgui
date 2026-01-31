# Font System

This document covers font loading, atlas generation, glyph management, and text rendering in elimgui.

## Table of Contents

- [Overview](#overview)
- [Font Types](#font-types)
  - [eli_font_glyph](#eli_font_glyph)
  - [eli_font_config](#eli_font_config)
  - [eli_font](#eli_font)
  - [eli_font_atlas](#eli_font_atlas)
- [Font Atlas](#font-atlas)
  - [Initialization](#initialization)
  - [Adding Fonts](#adding-fonts)
  - [Building](#building)
  - [Texture Access](#texture-access)
- [Glyph Ranges](#glyph-ranges)
- [Text Rendering](#text-rendering)
  - [Calculating Text Size](#calculating-text-size)
  - [Drawing Text](#drawing-text)
- [Font Stack](#font-stack)
- [Font Atlas Flags](#font-atlas-flags)

---

## Overview

The font system provides:
- **Font Atlas** - Packs multiple fonts into a single texture
- **Glyph Data** - Per-character metrics and UV coordinates
- **Text Rendering** - Renders text to draw lists as textured quads

**Data flow:**
```
TTF/OTF Data → Font Atlas Build → Glyph Table + Texture → Text Rendering → Draw List
```

### Dependencies

The font system uses two vendored STB libraries:
- **stb_truetype.h** - TTF/OTF parsing and glyph rasterization
- **stb_rect_pack.h** - Efficient rectangle packing for the texture atlas

Both are included automatically via `eli_font.h`.

### Complete Example

```c
#include "eli/eli_font.h"

// Create and initialize atlas
eli_font_atlas atlas;
eli_font_atlas_init(&atlas);

// Add the default font
eli_font* font = eli_font_atlas_add_font_default(&atlas, NULL);

// Build the atlas (rasterizes glyphs)
eli_font_atlas_build(&atlas);

// Get texture data for GPU upload
unsigned char* pixels;
int width, height;
eli_font_atlas_get_tex_data_as_rgba32(&atlas, &pixels, &width, &height, NULL);

// Upload to GPU and set texture ID
// my_texture_id = upload_texture_to_gpu(pixels, width, height);
// eli_font_atlas_set_tex_id(&atlas, my_texture_id);

// Use font for rendering...

// Cleanup
eli_font_atlas_destroy(&atlas);
```

---

## Font Types

### eli_font_glyph

Rendering data for a single character.

```c
struct eli_font_glyph {
    uint32_t codepoint;     // Unicode codepoint

    bool visible;           // Has visible pixels
    bool colored;           // Colored glyph (no tinting)

    float advance_x;        // Horizontal advance for layout
    float x0, y0;           // Top-left corner (relative to cursor)
    float x1, y1;           // Bottom-right corner

    float u0, v0;           // Top-left UV in atlas
    float u1, v1;           // Bottom-right UV in atlas
};
```

### eli_font_config

Configuration for loading a font.

```c
struct eli_font_config {
    // Input
    void* font_data;                // TTF/OTF binary data
    int font_data_size;             // Data size
    bool font_data_owned_by_atlas;  // Atlas owns memory (default: true)

    // Rendering options
    float size_pixels;              // Output size in pixels
    int oversample_h;               // Horizontal oversampling (1-4)
    int oversample_v;               // Vertical oversampling (1-4)
    bool pixel_snap_h;              // Snap glyphs to pixel boundaries

    // Glyph options
    eli_vec2 glyph_offset;          // Offset all glyphs
    float glyph_min_advance_x;      // Minimum advance width
    float glyph_max_advance_x;      // Maximum advance width
    const uint16_t* glyph_ranges;   // Unicode ranges to include

    // Merging
    bool merge_mode;                // Merge into previous font

    // Special characters
    uint32_t ellipsis_char;         // Character for "..." (0 = auto)

    char name[40];                  // Debug name
};
```

**Initialization:**
```c
eli_font_config config;
eli_font_config_init(&config);

config.size_pixels = 16.0f;
config.glyph_ranges = eli_font_atlas_get_glyph_ranges_default();
```

### eli_font

Runtime font data with glyph lookup and metrics.

```c
struct eli_font {
    // Glyphs
    eli_font_glyph_array glyphs;

    // Lookup tables (sparse arrays)
    float* index_advance_x;
    uint16_t* index_lookup;

    // Fallback character
    float fallback_advance_x;
    uint16_t fallback_glyph;
    uint32_t fallback_char;         // Default: U+FFFD

    // Metrics
    float size;                     // Font height in pixels
    float ascent;                   // Pixels above baseline
    float descent;                  // Pixels below baseline
    float scale;                    // Scale factor (usually 1.0)

    // Parent atlas
    eli_font_atlas* container_atlas;

    // Ellipsis handling
    uint32_t ellipsis_char;
    float ellipsis_width;
};
```

### eli_font_atlas

Manages multiple fonts packed into a single texture.

```c
struct eli_font_atlas {
    // Flags
    eli_font_atlas_flags flags;

    // Texture settings
    eli_texture_id tex_id;
    int tex_desired_width;
    int tex_glyph_padding;

    // Texture output
    unsigned char* tex_pixels_alpha8;   // 1-byte per pixel
    unsigned char* tex_pixels_rgba32;   // 4-bytes per pixel
    int tex_width;
    int tex_height;
    eli_vec2 tex_uv_scale;              // (1/w, 1/h)
    eli_vec2 tex_uv_white_pixel;        // White pixel location
    bool tex_is_built;

    // Fonts
    eli_font_ptr_array fonts;           // fonts.data[0] is default
};
```

---

## Font Atlas

### Initialization

```c
eli_font_atlas atlas;
eli_font_atlas_init(&atlas);

// ... add fonts and build ...

eli_font_atlas_destroy(&atlas);
```

### Adding Fonts

**Default Font (ProggyClean):**
```c
// Add embedded ProggyClean font (13px, pixel-perfect)
eli_font* font = eli_font_atlas_add_font_default(&atlas, NULL);

// With custom config
eli_font_config config;
eli_font_config_init(&config);
config.size_pixels = 16.0f;
eli_font* font = eli_font_atlas_add_font_default(&atlas, &config);
```

**From Memory (TTF/OTF data):**
```c
// Add font from TTF data in memory
eli_font* font = eli_font_atlas_add_font_from_memory_ttf(
    &atlas,
    my_ttf_data,        // TTF data pointer
    my_ttf_size,        // TTF data size
    16.0f,              // Size in pixels
    NULL,               // Config (NULL = defaults)
    NULL);              // Glyph ranges (NULL = default Latin)

// With custom config
eli_font_config config;
eli_font_config_init(&config);
config.oversample_h = 2;
config.oversample_v = 2;
config.glyph_ranges = eli_font_atlas_get_glyph_ranges_japanese();

eli_font* font = eli_font_atlas_add_font_from_memory_ttf(
    &atlas, my_ttf_data, my_ttf_size, 18.0f, &config, config.glyph_ranges);
```

### Building

After adding fonts, build the atlas to generate the texture:

```c
bool success = eli_font_atlas_build(&atlas);
if (!success) {
    // Handle error - no fonts added or allocation failed
}
```

The build process:
1. Allocates texture memory (default 512x512)
2. Uses stb_truetype to rasterize glyphs
3. Packs glyphs into the texture using stb_rect_pack
4. Generates lookup tables for fast glyph access
5. Sets up fallback glyphs for missing characters

### Texture Access

Get the texture data for your renderer:

```c
// As Alpha8 (1 byte per pixel)
unsigned char* pixels;
int width, height;
eli_font_atlas_get_tex_data_as_alpha8(&atlas, &pixels, &width, &height, NULL);

// As RGBA32 (4 bytes per pixel)
eli_font_atlas_get_tex_data_as_rgba32(&atlas, &pixels, &width, &height, NULL);

// Set texture ID after uploading to GPU
eli_font_atlas_set_tex_id(&atlas, my_gl_texture);
```

---

## Glyph Ranges

Glyph ranges specify which Unicode codepoints to include. Each range is a pair of values (start, end), terminated by 0.

### Predefined Ranges

```c
// Basic Latin + Extended Latin
const uint16_t* ranges = eli_font_atlas_get_glyph_ranges_default();

// Language-specific ranges
eli_font_atlas_get_glyph_ranges_greek();
eli_font_atlas_get_glyph_ranges_korean();
eli_font_atlas_get_glyph_ranges_japanese();
eli_font_atlas_get_glyph_ranges_chinese_simplified_common();
eli_font_atlas_get_glyph_ranges_chinese_full();
eli_font_atlas_get_glyph_ranges_cyrillic();
eli_font_atlas_get_glyph_ranges_thai();
eli_font_atlas_get_glyph_ranges_vietnamese();
```

### Custom Ranges

```c
static const uint16_t my_ranges[] = {
    0x0020, 0x00FF,  // Basic Latin
    0x0400, 0x04FF,  // Cyrillic
    0x2000, 0x206F,  // General Punctuation
    0                 // Terminator
};

config.glyph_ranges = my_ranges;
```

---

## Text Rendering

### Calculating Text Size

```c
eli_font* font = eli_get_font();
float font_size = eli_get_font_size();
const char* text = "Hello, World!";

eli_vec2 size = eli_calc_text_size(font, font_size, 0.0f, 0.0f, text, NULL);
// size.x = text width, size.y = text height
```

### Drawing Text

```c
// Full version with all parameters
eli_draw_list_add_text(draw_list,
    font,                           // Font to use
    16.0f,                          // Font size
    eli_make_vec2(100, 100),        // Position
    ELI_COL32_WHITE,                // Color
    "Hello, World!",                // Text start
    NULL,                           // Text end (NULL = auto)
    0.0f,                           // Wrap width (0 = no wrap)
    NULL);                          // CPU clip rect (NULL = use draw list)

// Simple version using current context font
eli_draw_list_add_text_simple(draw_list,
    eli_make_vec2(100, 100),
    ELI_COL32_WHITE,
    "Hello, World!");
```

---

## Font Stack

Push and pop fonts for temporary changes:

```c
eli_push_font(my_bold_font);

// Widgets here use my_bold_font
eli_text("Bold text");

eli_pop_font();  // Restore previous font
```

**Access current font:**
```c
eli_font* current = eli_get_font();
float size = eli_get_font_size();
eli_vec2 uv = eli_get_font_tex_uv_white_pixel();
```

---

## Font Atlas Flags

```c
typedef enum eli_font_atlas_flags {
    ELI_FONT_ATLAS_FLAGS_NONE                   = 0,
    ELI_FONT_ATLAS_FLAGS_NO_POWER_OF_TWO_HEIGHT = 1 << 0,
    ELI_FONT_ATLAS_FLAGS_NO_MOUSE_CURSORS       = 1 << 1,
    ELI_FONT_ATLAS_FLAGS_NO_BAKED_LINES         = 1 << 2
} eli_font_atlas_flags;
```

| Flag | Description |
|------|-------------|
| `NO_POWER_OF_TWO_HEIGHT` | Don't round texture height to power of 2 |
| `NO_MOUSE_CURSORS` | Don't build mouse cursor shapes (saves texture space) |
| `NO_BAKED_LINES` | Don't bake thick line textures (use polygon rendering) |

---

## Glyph Lookup

```c
// Find glyph with fallback
const eli_font_glyph* glyph = eli_font_find_glyph(font, 'A');

// Find glyph without fallback (returns NULL if missing)
const eli_font_glyph* glyph = eli_font_find_glyph_no_fallback(font, codepoint);

// Get advance width
float advance = eli_font_get_char_advance(font, 'A');

// Check if glyph is loaded
bool loaded = eli_font_is_glyph_loaded(font, codepoint);

// Check if font is loaded into atlas
bool ready = eli_font_is_loaded(font);
```

---

## See Also

- [Core Types & Context](core-types.md)
- [Draw System](draw-system.md)
- [API Quick Reference](README.md#api-quick-reference)
