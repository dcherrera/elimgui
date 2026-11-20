/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_font_text_style.h
 * @brief Faux bold text and single-line cuts with an ellipsis.
 *
 * Faux bold draws the glyphs twice, ELI_TEXT_BOLD_OFFSET pixels apart, so a
 * bold look costs no baked bold face (no atlas memory). The cut finds the
 * longest prefix that fits a width, always at a UTF-8 codepoint boundary; the
 * ellipsis draw pairs it with "…" (or "..." when the font has no U+2026 glyph).
 * Nothing here allocates: geometry goes into the draw list as eli_draw_list_add_text_ex
 * does, and the cut is reported as a pointer into the caller's text.
 *
 * @status In use (Argus Desktop bold labels, Argus Table).
 * @issues None
 * @todo None
 */
#ifndef ELI_FONT_ELI_FONT_TEXT_STYLE_H
#define ELI_FONT_ELI_FONT_TEXT_STYLE_H

#include "eli_font_types.h"
#include "eli_font_text.h"

#include "../core/eli_platform.h"
#include "../core/eli_types.h"
#include "../draw/eli_draw.h"

/** Horizontal distance between the two passes of faux bold text, in pixels. */
#define ELI_TEXT_BOLD_OFFSET 1.0f

/* ---------------------------------------------------------------------------
 * Faux bold
 * ------------------------------------------------------------------------- */

/**
 * Draw single-line text in faux bold: the glyphs drawn at `pos` and again
 * ELI_TEXT_BOLD_OFFSET pixels to the right (two glyph runs, same color).
 *
 * @param list        Target draw list.
 * @param font        Baked font.
 * @param font_size   Rendering size.
 * @param pos         Top-left pen position.
 * @param col         Text color.
 * @param text_begin  Start of text.
 * @param text_end    End of text, or NULL for strlen.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_draw_list_add_text_bold(eli_draw_list *list, const eli_font *font,
                                               float font_size, eli_vec2 pos, eli_col32 col,
                                               const char *text_begin, const char *text_end)
{
    eli_draw_list_add_text_ex(list, font, font_size, pos, col, text_begin, text_end, 0.0f, NULL);
    eli_draw_list_add_text_ex(list, font, font_size,
                              eli_make_vec2(pos.x + ELI_TEXT_BOLD_OFFSET, pos.y), col, text_begin,
                              text_end, 0.0f, NULL);
}

/**
 * Width of text drawn with eli_draw_list_add_text_bold: the regular width plus
 * ELI_TEXT_BOLD_OFFSET, or 0 for empty text.
 *
 * @param font        Baked font.
 * @param font_size   Rendering size.
 * @param text_begin  Start of text.
 * @param text_end    End of text, or NULL for strlen.
 * @return            Width in pixels (the widest line plus the offset).
 *
 * Thread-safe: yes (read-only)  Reentrant: yes
 */
static inline float eli_font_calc_text_width_bold(const eli_font *font, float font_size,
                                                  const char *text_begin, const char *text_end)
{
    if (!text_begin)
        return 0.0f;
    if (text_end == NULL)
        text_end = text_begin + strlen(text_begin);
    if (text_end == text_begin)
        return 0.0f;
    return eli_font_calc_text_size(font, font_size, text_begin, text_end).x +
           ELI_TEXT_BOLD_OFFSET;
}

/* ---------------------------------------------------------------------------
 * Single-line cut
 * ------------------------------------------------------------------------- */

/**
 * Find the longest prefix of the first line of `text` that fits `max_w`. The
 * cut always falls on a codepoint boundary, and a '\n' ends the line.
 *
 * @param font        Baked font (NULL measures every glyph as zero width).
 * @param font_size   Rendering size.
 * @param text_begin  Start of text.
 * @param text_end    End of text, or NULL for strlen.
 * @param max_w       Width available in pixels.
 * @param out_end     Receives one past the prefix's last byte (`text_end` when the
 *                    whole text fits on one line). May be NULL.
 * @return            The prefix's width in pixels.
 *
 * Thread-safe: yes (read-only)  Reentrant: yes
 */
static inline float eli_text_cut_to_width(const eli_font *font, float font_size,
                                          const char *text_begin, const char *text_end,
                                          float max_w, const char **out_end)
{
    if (!text_begin) {
        if (out_end)
            *out_end = text_begin;
        return 0.0f;
    }
    if (text_end == NULL)
        text_end = text_begin + strlen(text_begin);
    float scale = (font && font->font_size > 0.0f) ? font_size / font->font_size : 0.0f;
    float w = 0.0f;
    const char *s = text_begin;
    while (s < text_end) {
        uint32_t cp;
        int n = eli_text_utf8_decode(s, text_end, &cp);
        if (cp == '\n')
            break;
        float next = eli_font__add_advance(scale > 0.0f ? font : NULL, scale, cp, w);
        if (next > max_w)
            break;
        w = next;
        s += n;
    }
    if (out_end)
        *out_end = s;
    return w;
}

/** The ellipsis this font can draw: "…" when it has the glyph, else "...". */
static inline const char *eli_font__ellipsis(const eli_font *font)
{
    return eli_font_find_glyph_no_fallback(font, 0x2026) ? "\xE2\x80\xA6" : "...";
}

/**
 * Draw the first line of `text` cut to `max_w`: whole when it fits, else the
 * longest prefix that leaves room for an ellipsis, then the ellipsis. Text past a
 * '\n' counts as cut. If not even the ellipsis fits, it is drawn alone.
 *
 * @param list        Target draw list.
 * @param font        Baked font.
 * @param font_size   Rendering size.
 * @param pos         Top-left pen position.
 * @param col         Text color.
 * @param text_begin  Start of text.
 * @param text_end    End of text, or NULL for strlen.
 * @param max_w       Width available in pixels.
 * @return            The drawn width in pixels (prefix plus ellipsis).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline float eli_draw_list_add_text_ellipsis(eli_draw_list *list, const eli_font *font,
                                                    float font_size, eli_vec2 pos, eli_col32 col,
                                                    const char *text_begin, const char *text_end,
                                                    float max_w)
{
    if (!list || !font || !text_begin)
        return 0.0f;
    if (text_end == NULL)
        text_end = text_begin + strlen(text_begin);
    const char *cut;
    float w = eli_text_cut_to_width(font, font_size, text_begin, text_end, max_w, &cut);
    if (cut == text_end) {
        eli_draw_list_add_text_ex(list, font, font_size, pos, col, text_begin, text_end, 0.0f,
                                  NULL);
        return w;
    }
    const char *ell = eli_font__ellipsis(font);
    float ell_w = eli_font_calc_text_size(font, font_size, ell, NULL).x;
    w = eli_text_cut_to_width(font, font_size, text_begin, cut, max_w - ell_w, &cut);
    if (cut > text_begin)
        eli_draw_list_add_text_ex(list, font, font_size, pos, col, text_begin, cut, 0.0f, NULL);
    eli_draw_list_add_text_ex(list, font, font_size, eli_make_vec2(pos.x + w, pos.y), col, ell,
                              NULL, 0.0f, NULL);
    return w + ell_w;
}

#endif /* ELI_FONT_ELI_FONT_TEXT_STYLE_H */
