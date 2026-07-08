# Font System

Phase 3 adds font loading, atlas generation, text measurement, and text
rendering. Everything lives under `include/eli/font/` and is pulled in with:

```c
#include <eli/font/eli_font.h>
```

The font system builds on top of the draw system (Phase 2): text is emitted as
textured quads into an `eli_draw_list`. Glyph rasterization is done with the
vendored `stb_truetype.h` and `stb_rect_pack.h` (routed through the libc seam so
the same code compiles for wasm32 and for native host tests).

## Overview

| Concern | Header | Key types / functions |
|---------|--------|-----------------------|
| Data types | `eli_font_types.h` | `eli_font`, `eli_font_atlas`, `eli_font_config`, `eli_font_glyph` |
| Embedded font | `eli_font_proggy.h` | ProggyClean blob + `eli_font_stb_decompress` |
| Glyph ranges | `eli_font_glyph_ranges.h` | `eli_font_atlas_get_glyph_ranges_*` |
| Atlas build | `eli_font_atlas.h` | `eli_font_atlas_add_font_*`, `eli_font_atlas_build`, tex-data getters |
| Text + stack | `eli_font_text.h` | `eli_draw_list_add_text[_ex]`, `eli_calc_text_size`, `eli_push_font` |

A `eli_font_atlas` owns a single-channel (alpha8) texture and the fonts baked
into it. A `eli_font` owns a flat `eli_font_glyph` array plus a codepoint→glyph
lookup. Glyphs store the pen-relative corner offsets (`x0..y1`, top-left origin)
and atlas UVs (`u0..v1`) needed to build a text quad, plus `advance_x`.

## Building an atlas

```c
eli_font_atlas *atlas = eli_font_atlas_create();

/* Embedded ProggyClean at 13px (the default). */
eli_font *font = eli_font_atlas_add_font_default(atlas, NULL);

/* Or from your own TTF/OTF bytes: */
/* eli_font *font = eli_font_atlas_add_font_from_memory_ttf(
 *     atlas, ttf_bytes, ttf_size, 16.0f, NULL,
 *     eli_font_atlas_get_glyph_ranges_cyrillic(atlas)); */

eli_font_atlas_build(atlas);            /* rasterize + pack all fonts */

unsigned char *pixels; int w, h, bpp;
eli_font_atlas_get_tex_data_as_alpha8(atlas, &pixels, &w, &h, &bpp);
/* upload `pixels` (w*h, 1 byte/texel) to your renderer, then: */
eli_font_atlas_set_tex_id(atlas, my_backend_texture_id);
```

`eli_font_atlas_build` gathers the glyph bitmap boxes for every configured font,
packs them (plus one reserved opaque white texel) with the skyline packer,
chooses a texture width from the total surface area, rounds the height up to a
power of two, rasterizes each glyph, then fills per-font metrics and glyph
tables. It is safe to call again (it frees prior texture/glyph data first).

### Atlas API

| Function | Purpose |
|----------|---------|
| `eli_font_atlas_create()` / `_destroy()` | Allocate / free a heap atlas |
| `eli_font_atlas_init(atlas)` | Initialize an atlas in place |
| `eli_font_atlas_add_font(atlas, cfg)` | Add a source from a full config |
| `eli_font_atlas_add_font_default(atlas, cfg)` | Add embedded ProggyClean |
| `eli_font_atlas_add_font_from_memory_ttf(...)` | Add from raw TTF/OTF bytes |
| `eli_font_atlas_add_font_from_memory_compressed_ttf(...)` | Add from stb-compressed TTF |
| `eli_font_atlas_build(atlas)` | Bake the texture (returns `bool`) |
| `eli_font_atlas_is_built(atlas)` | Whether a texture has been baked |
| `eli_font_atlas_get_tex_data_as_alpha8(...)` | Fetch 1-byte texels (builds on demand) |
| `eli_font_atlas_get_tex_data_as_rgba32(...)` | Fetch RGBA (white + alpha), cached |
| `eli_font_atlas_set_tex_id(atlas, id)` | Store the backend texture id |
| `eli_font_atlas_clear[_tex_data\|_input_data\|_fonts]()` | Release owned data |

`eli_font_config` (see `eli_font_types.h`) controls `size_pixels`,
`glyph_ranges` (a zero-terminated list of `[lo, hi]` codepoint pairs; `NULL`
selects the default Latin range), glyph offset/advance clamps, and pixel
snapping. Pass `NULL` for the config to accept defaults (13px, 1× oversample,
horizontal snap on).

### Glyph ranges

Each getter returns a static, immutable range table. The `atlas` argument is
accepted for API symmetry and is unused:

`eli_font_atlas_get_glyph_ranges_default` / `_greek` / `_korean` / `_japanese` /
`_chinese_full` / `_chinese_simplified_common` / `_cyrillic` / `_thai` /
`_vietnamese`. Large-script ranges cover the relevant Unicode blocks (favoring
completeness over atlas size).

## Measuring and drawing text

```c
/* Measure with an explicit font/size. */
eli_vec2 size = eli_font_calc_text_size(font, font->font_size, "Hello", NULL);

/* Draw with an explicit font/size (caller binds the atlas texture). */
eli_draw_list_add_text_ex(draw_list, font, font->font_size,
                          eli_make_vec2(x, y), ELI_COL32_WHITE,
                          "Hello", NULL, 0.0f, NULL);
```

- `text_end` may be `NULL` to use `strlen`. Text is UTF-8 decoded.
- Width is the widest line; height is `line_count * line_height`
  (`line_height == font_size`). `'\n'` starts a new line.
- Whitespace glyphs advance the pen but emit no geometry, so `add_text` emits
  exactly `4` vertices and `6` indices per **visible** glyph.
- The optional `cpu_fine_clip_rect` (`min.x, min.y, max.x, max.y`) skips glyphs
  that fall fully outside it.

## Font stack

The current font lives on the active context. These operate on
`ctx->font` / `ctx->font_size` / `ctx->font_stack`:

| Function | Purpose |
|----------|---------|
| `eli_push_font(font)` | Make `font` current, saving the previous one |
| `eli_pop_font()` | Restore the saved font |
| `eli_get_font()` | Current font (or `NULL`) |
| `eli_get_font_size()` | Current size = `font_size * io.font_global_scale` |
| `eli_get_font_tex_uv_white_pixel()` | Current atlas's opaque white-texel UV |

`eli_draw_list_add_text` and `eli_calc_text_size` (the no-font-argument forms)
use the current context font and size.

## Compiling example

wasm32 (production) — `stb` is on the include path via `-Ivendor`:

```bash
/opt/homebrew/opt/llvm/bin/clang --target=wasm32 -nostdlib \
    -Ivendor/jaclibc/include -Iinclude -Ivendor -O2 -c -o app.o app.c
```

Native unit tests build with `-DELI_TEST_HOSTED` (host libc) and also need
`-Ivendor` for the `stb` headers; `./build.sh test` handles this.

## Notes / gotchas

- `add_text[_ex]` does **not** manage the draw list's texture stack; the caller
  is expected to have the font atlas texture bound (matches Dear ImGui).
- `wrap_width` is currently accepted but unwrapped (reserved for a later pass).
- The embedded ProggyClean blob (`eli_font_proggy.h`) is the big-data exception
  to the per-file size guideline. Its `eli_font_stb_decompress` also expands any
  stb-compressed TTF passed to `add_font_from_memory_compressed_ttf`.
