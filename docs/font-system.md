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
| stb seam | `eli_font_stb.h` | stb_truetype / stb_rect_pack implementation + libc hooks |
| Glyph table | `eli_font_glyph_table.h` | `eli_font_find_glyph[_no_fallback]`, fallback/tofu, custom glyphs |
| Pages | `eli_font_atlas_pages.h` | `eli_font_atlas_set_backend`, `eli_font_atlas_update`, LRU/idle pages |
| On-demand | `eli_font_atlas_on_demand.h` | per-source faces, merge-mode composition, lazy glyph load |
| Atlas build | `eli_font_atlas.h` | `eli_font_atlas_add_font_*`, `eli_font_atlas_build`, tex-data getters |
| Text + stack | `eli_font_text.h` | `eli_draw_list_add_text[_ex]`, `eli_calc_text_size`, `eli_push_font` |

A `eli_font_atlas` owns single-channel (alpha8) texture **pages** and the fonts
baked into them; a baked atlas has one page (the whole texture, mirrored in the
`tex_*` fields). A `eli_font` owns a flat `eli_font_glyph` array plus a two-level
codepoint→glyph lookup (256-codepoint blocks, allocated on use). Glyphs store the
pen-relative corner offsets (`x0..y1`, top-left origin), their `page` and UVs
(`u0..v1`) needed to build a text quad, plus `advance_x` and a `kind` (atlas
quad, tofu box, or custom).

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
| `eli_font_atlas_is_built(atlas)` | Whether the atlas has been baked (stays true after `release_cpu_pixels`) |
| `eli_font_atlas_release_cpu_pixels(atlas)` | Free the CPU pixel buffers after upload; fonts/UVs stay valid |
| `eli_font_atlas_get_tex_data_as_alpha8(...)` | Fetch 1-byte texels (builds on demand) |
| `eli_font_atlas_get_tex_data_as_rgba32(...)` | Fetch RGBA (white + alpha), cached |
| `eli_font_atlas_set_tex_id(atlas, id)` | Store the backend texture id |
| `eli_font_atlas_clear[_tex_data\|_input_data\|_fonts]()` | Release owned data |

`eli_font_config` (see `eli_font_types.h`) controls `size_pixels`,
`glyph_ranges` (a zero-terminated list of `[lo, hi]` codepoint pairs; `NULL`
selects the default Latin range), glyph offset/advance clamps, and pixel
snapping. Pass `NULL` for the config to accept defaults (13px, 1× oversample,
horizontal snap on).

### On-demand glyphs and pages

Set `ELI_FONT_ATLAS_ON_DEMAND` before building to skip the up-front bake: build only
opens the faces, fills font metrics and creates page 0 (with the white block). A
glyph is rasterized on its first lookup into a `page_size`-square page (default
512, 256 KB), so memory follows what is drawn, not how many faces and ranges are
loaded. Font data must stay alive for the atlas's lifetime. `glyph_ranges` then
limits which codepoints a source may supply (`NULL` = any it has).

```c
atlas->flags |= ELI_FONT_ATLAS_ON_DEMAND;      /* page_size, max_pages, page_idle_frames: 0 = defaults */
eli_font *font = eli_font_atlas_add_font_from_memory_ttf(atlas, ttf, size, 13.0f, NULL, NULL);
eli_font_atlas_build(atlas);                    /* fast: no rasterization */
eli_font_atlas_set_backend(atlas, &hooks);      /* create/update/destroy page textures */
/* each rendered frame, before drawing: */
eli_font_atlas_update(atlas);                   /* upload new glyphs, free idle pages */
```

- Every page has the white block at the same texels, so solid fills work with any
  page bound. Text switches the draw list's texture when a glyph's page differs and
  restores it after the run.
- When a glyph fits no page and `max_pages` (default 8) are live, the least
  recently used page not used this frame (never page 0) is cleared and reused; its
  glyphs reload on next use. If every page was used this frame, one more is
  allocated (a soft cap bounded by the frame's working set).
- `eli_font_atlas_update` frees pages (other than page 0) unused for
  `page_idle_frames` (default 600) and advances the frame clock LRU uses. Without
  it, pages are never recycled or freed.
- A lookup may grow the glyph array: a glyph pointer stays valid only until the
  next lookup of a codepoint not yet loaded.
- `eli_font_atlas_release_cpu_pixels` is a no-op on an on-demand atlas.
- `eli_font_atlas_texture_bytes` / `eli_font_atlas_live_page_count` report what
  the pages hold.

Backend hooks (`eli_font_atlas_backend`) receive coverage bytes; the SDL3 backend
installs them in `eli_sdl3_renderer_create_fonts_texture` (INDEX8 pages, see
native-backends.md). A baked atlas with hooks also gets its page texture from them.

### Fallback chains (merge mode)

`eli_font_config.merge_mode` adds a source to the previously added font as a
fallback: it supplies only codepoints the earlier sources lack, aligned to that
font's baseline (baked atlases skip the duplicates entirely). With
`merge_match_advance`, each merged glyph takes the font's cell advance
(`cell_advance_x`, its `'M'`), centred and shrunk about its centre when wider, so
a borrowed symbol never shifts monospace columns.

```c
eli_font *term = eli_font_atlas_add_font_from_memory_ttf(atlas, mono, mono_size, 12.0f, NULL, ranges);
eli_font_config fb = eli_font_config_default();
fb.merge_mode = true;
fb.merge_match_advance = true;
eli_font_atlas_add_font_from_memory_ttf(atlas, symbols, symbols_size, 12.0f, &fb, symbol_ranges);
```

### Missing glyphs and custom glyphs

A codepoint no source has draws a **tofu box** (an outline inset in a cell as wide
as `'?'`, so layouts measured with the old `'?'` fallback do not move), not `'?'`. `eli_font_set_fallback_char(font, '?')` restores a glyph
fallback. `eli_font_add_custom_glyph_range(font, lo, hi, advance)` with
`eli_font_set_custom_glyph_fn(font, fn, user)` draws those codepoints through a
callback instead of any face (box drawing, spinners, icons): it gets the draw list
and the glyph's cell (advance wide, one line tall). Ranges survive rebuilds.

### HiDPI: rasterizer density

`eli_font_config.rasterizer_density` (default 1; `<= 0` means 1) bakes glyphs at
`size_pixels x density` texels while every metric (ascent, descent, line height,
advances, glyph quads) stays in **points**. Layout is identical at any density; on a
2x framebuffer each glyph texel maps to one device pixel, so text is sharp.

```c
eli_font_config cfg = eli_font_config_default();
cfg.rasterizer_density = SDL_GetWindowPixelDensity(window);   /* 2 on Retina */
eli_font *font = eli_font_atlas_add_font_default(atlas, &cfg);
eli_font_atlas_build(atlas);
/* upload, then free the CPU copy (a 1024^2 RGBA atlas is ~5 MB): */
eli_font_atlas_release_cpu_pixels(atlas);
eli_push_font(font);   /* after the bake: font_size is only known once built */
```

On a display-scale change, set the new density on every `atlas->configs[i]`, call
`eli_font_atlas_build` again (font pointers stay valid) and re-upload. Pushing a font
before the first bake leaves the context font size at 0 (nothing renders).

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

/* Draw with an explicit font/size (page 0 bound; other pages switch per glyph). */
eli_draw_list_add_text_ex(draw_list, font, font->font_size,
                          eli_make_vec2(x, y), ELI_COL32_WHITE,
                          "Hello", NULL, 0.0f, NULL);
```

- `text_end` may be `NULL` to use `strlen`. Text is UTF-8 decoded.
- Width is the widest line; height is `line_count * line_height`
  (`line_height == font_size`). `'\n'` starts a new line.
- Whitespace glyphs advance the pen but emit no geometry, so `add_text` emits
  exactly `4` vertices and `6` indices per **visible** atlas glyph (tofu and custom
  glyphs add their own shapes).
- The optional `cpu_fine_clip_rect` (`min.x, min.y, max.x, max.y`) skips glyphs
  that fall fully outside it.

### Word wrap

```c
/* Wrap at 300 px: lines stack one font line height apart. */
eli_vec2 block = eli_font_calc_text_size_wrapped(font, font->font_size, text, NULL, 300.0f);
eli_draw_list_add_text_ex(draw_list, font, font->font_size, eli_make_vec2(x, y),
                          ELI_COL32_WHITE, text, NULL, 300.0f, NULL);
```

- `wrap_width <= 0` breaks only on `'\n'`.
- Greedy: a line breaks at its last space once it grows past `wrap_width`; the
  space is dropped and the next line starts after it. A word wider than the line
  overflows. `'\n'` always breaks.
- `eli_font_calc_word_wrap_line(font, size, line_start, text_end, wrap_width,
  &width)` returns one line's end; `eli_text_wrapped` and `add_text_ex` share it.
  It keeps a running width (one glyph lookup per codepoint), so wrapping is O(n)
  instead of re-measuring each line prefix at every byte.

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

- `add_text[_ex]` expects page 0 bound (windows push `atlas->tex_id`); glyphs on
  other pages switch the texture for their quads and restore it after the run.
- The embedded ProggyClean blob (`eli_font_proggy.h`) is the big-data exception
  to the per-file size guideline. Its `eli_font_stb_decompress` also expands any
  stb-compressed TTF passed to `add_font_from_memory_compressed_ttf`.
