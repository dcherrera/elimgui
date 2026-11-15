/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_text_widgets.h
 * @brief Phase 9 text-display widgets: plain/formatted text, colored and disabled
 *        variants, wrapped text, label-value pairs, bullet text, the bullet glyph,
 *        and separator text. Each measures itself, advances the layout cursor via
 *        eli_item_size/eli_item_add, and emits glyphs through the render helpers.
 *
 * Formatted variants route through vsnprintf into a bounded stack buffer, so no
 * per-frame heap allocation occurs. Text is non-interactive (id 0) except where a
 * variant needs a frame; it still records a last-item rect for the status queries.
 *
 * @status Phase 9 text widgets in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WIDGETS_ELI_TEXT_WIDGETS_H
#define ELI_WIDGETS_ELI_TEXT_WIDGETS_H

#include "eli_widget_behavior.h"

#include "../core/eli_platform.h"

#include <stdarg.h>
#include <stdio.h>

/** Size of the bounded stack buffer used by the formatted text variants. */
#define ELI_TEXT_FMT_BUFFER_SIZE 1024

/**
 * Format into a bounded buffer, always NUL-terminating.
 *
 * @param buf       Destination buffer (must be non-NULL, buf_size >= 1).
 * @param buf_size  Capacity of buf in bytes.
 * @param fmt       printf-style format string.
 * @param args      Variadic argument list.
 * @return          Pointer to the end of the written string within buf.
 */
static inline const char *eli_text__format_v(char *buf, size_t buf_size, const char *fmt,
                                             va_list args)
{
    int n = vsnprintf(buf, buf_size, fmt, args);
    if (n < 0) {
        buf[0] = '\0';
        return buf;
    }
    size_t len = (size_t)n < buf_size ? (size_t)n : buf_size - 1;
    return buf + len;
}

/* ---------------------------------------------------------------------------
 * Plain text
 * ------------------------------------------------------------------------- */

/**
 * Draw an unformatted UTF-8 text run at the cursor, advancing the layout.
 *
 * @param text      Start of the text.
 * @param text_end  End of the text, or NULL for NUL-terminated.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_text_unformatted(const char *text, const char *text_end)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items ||
        text == NULL)
        return;
    if (text_end == NULL)
        text_end = text + strlen(text);

    eli_vec2 pos = ctx->current_window->cursor_pos;
    eli_vec2 size = eli_calc_text_size(text, text_end);
    eli_rect bb = eli_make_rect(pos.x, pos.y, size.x, size.y);

    eli_item_size(size, 0.0f);
    if (!eli_item_add(0u, bb, 0))
        return;
    eli_render_text(pos, eli_get_color_u32(ELI_COL_TEXT, 1.0f), text, text_end, false);
}

/**
 * Draw formatted text (printf-style) at the cursor.
 *
 * @param fmt   Format string.
 * @param args  Variadic argument list.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_text_v(const char *fmt, va_list args)
{
    char buf[ELI_TEXT_FMT_BUFFER_SIZE];
    const char *end = eli_text__format_v(buf, sizeof(buf), fmt, args);
    eli_text_unformatted(buf, end);
}

/** Draw formatted text (printf-style) at the cursor. */
static inline void eli_text(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    eli_text_v(fmt, args);
    va_end(args);
}

/* ---------------------------------------------------------------------------
 * Colored / disabled text
 * ------------------------------------------------------------------------- */

/** Draw formatted text in an explicit color (va_list form). */
static inline void eli_text_colored_v(eli_vec4 col, const char *fmt, va_list args)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return;
    char buf[ELI_TEXT_FMT_BUFFER_SIZE];
    const char *end = eli_text__format_v(buf, sizeof(buf), fmt, args);

    eli_vec2 pos = ctx->current_window->cursor_pos;
    eli_vec2 size = eli_calc_text_size(buf, end);
    eli_rect bb = eli_make_rect(pos.x, pos.y, size.x, size.y);
    eli_item_size(size, 0.0f);
    if (!eli_item_add(0u, bb, 0))
        return;
    eli_render_text(pos, eli_get_color_u32_vec4(col), buf, end, false);
}

/** Draw formatted text in an explicit color. */
static inline void eli_text_colored(eli_vec4 col, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    eli_text_colored_v(col, fmt, args);
    va_end(args);
}

/** Draw formatted text in the disabled-text color (va_list form). */
static inline void eli_text_disabled_v(const char *fmt, va_list args)
{
    eli_text_colored_v(eli_get_style_color_vec4(ELI_COL_TEXT_DISABLED), fmt, args);
}

/** Draw formatted text in the disabled-text color. */
static inline void eli_text_disabled(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    eli_text_disabled_v(fmt, args);
    va_end(args);
}

/* ---------------------------------------------------------------------------
 * Wrapped text
 * ------------------------------------------------------------------------- */

/* Line ends are cached as uint16_t offsets into the format buffer. */
_Static_assert(ELI_TEXT_FMT_BUFFER_SIZE <= 65536, "wrap line offsets are uint16_t");

/** @return The wrapped-text line advance (never <= 0). */
static inline float eli_text__wrap_line_height(void)
{
    float line_h = eli_get_text_line_height();
    return line_h > 0.0f ? line_h : 1.0f;
}

/**
 * Greedy word-wrap layout of a formatted text run in one O(n) pass: breaks on
 * spaces and forced newlines (eli_font_calc_word_wrap_line), letting overlong
 * words overflow, and records each line's end so the render pass replays the
 * breaks instead of recomputing them.
 *
 * @param pos_y           Top of the block (lines advance from here).
 * @param text            Start of the text (at most ELI_TEXT_FMT_BUFFER_SIZE - 1 bytes).
 * @param text_end        End of the text.
 * @param wrap_width      Wrap width in pixels (<= 0 disables wrapping).
 * @param line_ends       Receives each line's end offset from `text`.
 * @param line_capacity   Entries available in line_ends (lines past it are measured,
 *                        not recorded; text_len + 1 entries always suffice).
 * @param out_line_count  Receives the number of recorded lines.
 * @return                The wrapped block size (widest line, total height).
 */
static inline eli_vec2 eli_text__wrap_layout(float pos_y, const char *text, const char *text_end,
                                             float wrap_width, uint16_t *line_ends,
                                             int line_capacity, int *out_line_count)
{
    eli_context *ctx = eli_get_current_context();
    const eli_font *font = ctx ? ctx->font : NULL;
    float font_size = ctx ? ctx->font_size : 0.0f;
    float line_h = eli_text__wrap_line_height();

    float max_w = 0.0f;
    float y = pos_y;
    int line_count = 0;
    for (const char *line = text;;) {
        float lw;
        const char *line_end =
            eli_font_calc_word_wrap_line(font, font_size, line, text_end, wrap_width, &lw);
        if (lw > max_w)
            max_w = lw;
        if (line_count < line_capacity)
            line_ends[line_count++] = (uint16_t)(line_end - text);
        y += line_h;
        if (line_end == text_end)
            break;
        line = line_end + 1;
    }
    *out_line_count = line_count;

    float total_h = y - pos_y;
    if (total_h < line_h)
        total_h = line_h;
    return eli_make_vec2(max_w, total_h);
}

/**
 * Emit the lines recorded by eli_text__wrap_layout into the window draw list.
 *
 * @param pos         Top-left pen position.
 * @param col         Packed text color.
 * @param text        Start of the text passed to the layout.
 * @param line_ends   Line end offsets from the layout.
 * @param line_count  Number of recorded lines.
 */
static inline void eli_text__wrap_render(eli_vec2 pos, eli_col32 col, const char *text,
                                         const uint16_t *line_ends, int line_count)
{
    eli_draw_list *dl = eli_get_window_draw_list();
    if (dl == NULL)
        return;
    float line_h = eli_text__wrap_line_height();
    float y = pos.y;
    const char *line_start = text;
    for (int i = 0; i < line_count; i++) {
        const char *line_end = text + line_ends[i];
        if (line_end > line_start)
            eli_draw_list_add_text(dl, eli_make_vec2(pos.x, y), col, line_start, line_end);
        y += line_h;
        line_start = line_end + 1;
    }
}

/** Draw formatted text wrapped to the content region width (va_list form). */
static inline void eli_text_wrapped_v(const char *fmt, va_list args)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return;
    char buf[ELI_TEXT_FMT_BUFFER_SIZE];
    const char *end = eli_text__format_v(buf, sizeof(buf), fmt, args);

    eli_vec2 pos = ctx->current_window->cursor_pos;
    float wrap_width = eli_get_content_region_avail().x;
    eli_col32 col = eli_get_color_u32(ELI_COL_TEXT, 1.0f);

    uint16_t line_ends[ELI_TEXT_FMT_BUFFER_SIZE];
    int line_count;
    eli_vec2 size = eli_text__wrap_layout(pos.y, buf, end, wrap_width, line_ends,
                                          ELI_TEXT_FMT_BUFFER_SIZE, &line_count);
    eli_rect bb = eli_make_rect(pos.x, pos.y, size.x, size.y);
    eli_item_size(size, 0.0f);
    if (!eli_item_add(0u, bb, 0))
        return;
    eli_text__wrap_render(pos, col, buf, line_ends, line_count);
}

/** Draw formatted text wrapped to the content region width. */
static inline void eli_text_wrapped(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    eli_text_wrapped_v(fmt, args);
    va_end(args);
}

/* ---------------------------------------------------------------------------
 * Label-value text
 * ------------------------------------------------------------------------- */

/** Draw a right-hand formatted value with a left-aligned label (va_list form). */
static inline void eli_label_text_v(const char *label, const char *fmt, va_list args)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return;
    const eli_style *style = &ctx->style;
    char buf[ELI_TEXT_FMT_BUFFER_SIZE];
    const char *value_end = eli_text__format_v(buf, sizeof(buf), fmt, args);

    eli_vec2 pos = ctx->current_window->cursor_pos;
    float w = eli_calc_item_width();
    eli_vec2 label_size = eli_calc_text_size(label, NULL);
    eli_vec2 value_size = eli_calc_text_size(buf, value_end);
    float pad_y = style->frame_padding.y;

    float total_w = w + (label_size.x > 0.0f ? style->item_inner_spacing.x + label_size.x : 0.0f);
    float total_h = eli_max_f(value_size.y, label_size.y) + pad_y * 2.0f;
    eli_rect bb = eli_make_rect(pos.x, pos.y, total_w, total_h);
    eli_item_size(eli_rect_size(bb), pad_y);
    if (!eli_item_add(0u, bb, 0))
        return;

    eli_col32 col = eli_get_color_u32(ELI_COL_TEXT, 1.0f);
    eli_render_text(eli_make_vec2(pos.x, pos.y + pad_y), col, buf, value_end, false);
    if (label_size.x > 0.0f)
        eli_render_text(eli_make_vec2(pos.x + w + style->item_inner_spacing.x, pos.y + pad_y), col,
                        label, NULL, true);
}

/** Draw a right-hand formatted value with a left-aligned label. */
static inline void eli_label_text(const char *label, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    eli_label_text_v(label, fmt, args);
    va_end(args);
}

/* ---------------------------------------------------------------------------
 * Bullets
 * ------------------------------------------------------------------------- */

/**
 * Draw a standalone bullet glyph, advancing the cursor by one line height so a
 * following same-line item aligns beside it.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_bullet(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return;
    const eli_style *style = &ctx->style;
    float line_h = eli_max_f(eli_get_font_size(), eli_get_text_line_height());
    eli_vec2 pos = ctx->current_window->cursor_pos;
    eli_rect bb = eli_make_rect(pos.x, pos.y, line_h + style->frame_padding.x * 2.0f, line_h);
    eli_item_size(eli_rect_size(bb), 0.0f);
    if (!eli_item_add(0u, bb, 0))
        return;
    eli_render_bullet(eli_get_window_draw_list(), pos, eli_get_color_u32(ELI_COL_TEXT, 1.0f));
}

/** Draw a bullet glyph followed by formatted text on the same line (va_list form). */
static inline void eli_bullet_text_v(const char *fmt, va_list args)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return;
    char buf[ELI_TEXT_FMT_BUFFER_SIZE];
    const char *end = eli_text__format_v(buf, sizeof(buf), fmt, args);

    float line_h = eli_max_f(eli_get_font_size(), eli_get_text_line_height());
    eli_vec2 pos = ctx->current_window->cursor_pos;
    eli_vec2 text_size = eli_calc_text_size(buf, end);
    float bullet_w = line_h + ctx->style.frame_padding.x * 2.0f;

    eli_vec2 size = eli_make_vec2(bullet_w + text_size.x, eli_max_f(line_h, text_size.y));
    eli_rect bb = eli_make_rect(pos.x, pos.y, size.x, size.y);
    eli_item_size(size, 0.0f);
    if (!eli_item_add(0u, bb, 0))
        return;

    eli_col32 col = eli_get_color_u32(ELI_COL_TEXT, 1.0f);
    eli_render_bullet(eli_get_window_draw_list(), pos, col);
    eli_render_text(eli_make_vec2(pos.x + bullet_w, pos.y), col, buf, end, false);
}

/** Draw a bullet glyph followed by formatted text on the same line. */
static inline void eli_bullet_text(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    eli_bullet_text_v(fmt, args);
    va_end(args);
}

/* ---------------------------------------------------------------------------
 * Separator text
 * ------------------------------------------------------------------------- */

/**
 * Draw a horizontal separator carrying a left-aligned text label, with the rule
 * line filling the remaining width. Mirrors Dear ImGui's SeparatorText.
 *
 * @param label  Label to render (its visible part stops at "##").
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_separator_text(const char *label)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items ||
        label == NULL)
        return;
    const eli_style *style = &ctx->style;

    eli_vec2 label_size = eli_calc_text_size(label, eli_find_rendered_text_end(label, NULL));
    eli_vec2 pos = ctx->current_window->cursor_pos;
    float avail_w = eli_get_content_region_avail().x;
    float sep_h = eli_max_f(label_size.y, style->separator_text_padding.y * 2.0f);

    eli_rect bb = eli_make_rect(pos.x, pos.y, avail_w, sep_h);
    eli_item_size(eli_rect_size(bb), 0.0f);
    if (!eli_item_add(0u, bb, 0))
        return;

    eli_draw_list *dl = eli_get_window_draw_list();
    float center_y = pos.y + sep_h * 0.5f;
    float label_x = pos.x + style->separator_text_padding.x;
    eli_col32 line_col = eli_get_color_u32(ELI_COL_SEPARATOR, 1.0f);

    /* A short rule to the left of the label, then the label, then a long rule. */
    eli_draw_list_add_line(dl, eli_make_vec2(pos.x, center_y),
                           eli_make_vec2(label_x - style->item_inner_spacing.x, center_y), line_col,
                           style->separator_text_border_size);
    eli_render_text(eli_make_vec2(label_x, center_y - label_size.y * 0.5f),
                    eli_get_color_u32(ELI_COL_TEXT, 1.0f), label, NULL, true);
    float after_x = label_x + label_size.x + style->item_inner_spacing.x;
    eli_draw_list_add_line(dl, eli_make_vec2(after_x, center_y),
                           eli_make_vec2(pos.x + avail_w, center_y), line_col,
                           style->separator_text_border_size);
}

#endif /* ELI_WIDGETS_ELI_TEXT_WIDGETS_H */
