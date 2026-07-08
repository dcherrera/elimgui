/**
 * @file eli_layout_sizing.h
 * @brief Standard vertical-metric helpers used to size and space rows: text line
 *        height (with/without item spacing) and framed-widget height (with/without
 *        item spacing), all derived from the current font size and the style.
 *
 * @status Phase 8 sizing helpers in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_LAYOUT_ELI_LAYOUT_SIZING_H
#define ELI_LAYOUT_ELI_LAYOUT_SIZING_H

#include "../core/eli_platform.h"
#include "../core/eli_context.h"

/**
 * @return the height of one text line (the current font size), or 0 without a
 *         current context.
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline float eli_get_text_line_height(void)
{
    eli_context *ctx = eli_get_current_context();
    return ctx ? ctx->font_size : 0.0f;
}

/**
 * @return a text line's height plus one item spacing (row-to-row pitch for text).
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline float eli_get_text_line_height_with_spacing(void)
{
    eli_context *ctx = eli_get_current_context();
    return ctx ? (ctx->font_size + ctx->style.item_spacing.y) : 0.0f;
}

/**
 * @return the height of a framed widget: font size plus vertical frame padding on
 *         both edges.
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline float eli_get_frame_height(void)
{
    eli_context *ctx = eli_get_current_context();
    return ctx ? (ctx->font_size + ctx->style.frame_padding.y * 2.0f) : 0.0f;
}

/**
 * @return a framed widget's height plus one item spacing (row-to-row pitch for
 *         framed widgets).
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline float eli_get_frame_height_with_spacing(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return 0.0f;
    return ctx->font_size + ctx->style.frame_padding.y * 2.0f + ctx->style.item_spacing.y;
}

#endif /* ELI_LAYOUT_ELI_LAYOUT_SIZING_H */
