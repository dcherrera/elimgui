/**
 * @file eli_button_widgets.h
 * @brief Phase 9 interactive widgets: buttons (regular / ex / small / invisible /
 *        arrow), checkboxes (bool + int/uint flag variants), radio buttons, the
 *        progress bar, and text links. Each measures itself, registers via
 *        eli_item_size/eli_item_add, drives eli_button_behavior, and renders with
 *        the shared frame/text/arrow/check-mark helpers.
 *
 * @status Phase 9 button/checkbox/radio/progress/link widgets in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WIDGETS_ELI_BUTTON_WIDGETS_H
#define ELI_WIDGETS_ELI_BUTTON_WIDGETS_H

#include "eli_widget_behavior.h"

#include "../core/eli_platform.h"

#include <stdio.h>

#ifdef ELI_JSIO
/* Host page opens the URL in a new browser tab (browser-only; no-op under tests). */
JS_IMPORT(eli_host_open_url) void eli_host_open_url(const char *url);
#endif

/* ---------------------------------------------------------------------------
 * Internal render helper
 * ------------------------------------------------------------------------- */

/** Render a label aligned within an inner (frame-padded) rect. */
static inline void eli_button__render_label(eli_vec2 bb_min, eli_vec2 bb_max, eli_col32 col,
                                            const char *label, eli_vec2 label_size, eli_vec2 align)
{
    const eli_style *style = eli_get_style();
    if (style == NULL)
        return;
    eli_vec2 inner_min = eli_make_vec2(bb_min.x + style->frame_padding.x,
                                       bb_min.y + style->frame_padding.y);
    eli_vec2 inner_max = eli_make_vec2(bb_max.x - style->frame_padding.x,
                                       bb_max.y - style->frame_padding.y);
    float avail_x = (inner_max.x - inner_min.x) - label_size.x;
    float avail_y = (inner_max.y - inner_min.y) - label_size.y;
    eli_vec2 pos = eli_make_vec2(inner_min.x + eli_max_f(0.0f, avail_x) * align.x,
                                 inner_min.y + eli_max_f(0.0f, avail_y) * align.y);
    eli_render_text(pos, col, label, NULL, true);
}

/** Resolve the frame background color for the given hover/active state. */
static inline eli_col32 eli_widget__frame_bg(bool hovered, bool held)
{
    if (held && hovered)
        return eli_get_color_u32(ELI_COL_FRAME_BG_ACTIVE, 1.0f);
    if (hovered)
        return eli_get_color_u32(ELI_COL_FRAME_BG_HOVERED, 1.0f);
    return eli_get_color_u32(ELI_COL_FRAME_BG, 1.0f);
}

/* ---------------------------------------------------------------------------
 * Buttons
 * ------------------------------------------------------------------------- */

/**
 * The full button: a labelled, framed, clickable widget.
 *
 * @param label     Button label (its visible part stops at "##").
 * @param size_arg  Requested size (0 = fit label, < 0 = region-relative).
 * @param flags     eli_button_flags (mouse button + press policy).
 * @return          true on the frame the button is pressed.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_button_ex(const char *label, eli_vec2 size_arg, eli_button_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return false;
    const eli_style *style = &ctx->style;

    eli_id id = eli_get_id(label);
    eli_vec2 label_size = eli_calc_text_size(label, eli_find_rendered_text_end(label, NULL));
    eli_vec2 pos = ctx->current_window->cursor_pos;

    eli_vec2 size = eli_calc_item_size(size_arg, label_size.x + style->frame_padding.x * 2.0f,
                                       label_size.y + style->frame_padding.y * 2.0f);
    eli_rect bb = eli_make_rect(pos.x, pos.y, size.x, size.y);
    eli_item_size(size, style->frame_padding.y);
    if (!eli_item_add(id, bb, 0))
        return false;

    bool hovered = false, held = false;
    bool pressed = eli_button_behavior(bb, id, &hovered, &held, flags);

    eli_render_nav_highlight(bb, id);
    eli_col32 col = eli_get_color_u32(
        (held && hovered) ? ELI_COL_BUTTON_ACTIVE : hovered ? ELI_COL_BUTTON_HOVERED : ELI_COL_BUTTON,
        1.0f);
    eli_render_frame(eli_rect_min(bb), eli_rect_max(bb), col, true, style->frame_rounding);
    eli_button__render_label(eli_rect_min(bb), eli_rect_max(bb),
                             eli_get_color_u32(ELI_COL_TEXT, 1.0f), label, label_size,
                             style->button_text_align);
    return pressed;
}

/** A default button sized to fit its label. */
static inline bool eli_button(const char *label)
{
    return eli_button_ex(label, eli_make_vec2(0.0f, 0.0f), ELI_BUTTON_NONE);
}

/** A compact button with no vertical frame padding, for inline placement. */
static inline bool eli_small_button(const char *label)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return false;
    float backup = ctx->style.frame_padding.y;
    ctx->style.frame_padding.y = 0.0f;
    bool pressed = eli_button_ex(label, eli_make_vec2(0.0f, 0.0f), ELI_BUTTON_NONE);
    ctx->style.frame_padding.y = backup;
    return pressed;
}

/**
 * A non-visual clickable region of an explicit size (useful for custom widgets).
 *
 * @param str_id  Identity string.
 * @param size    Region size (0 components fall back to the available region).
 * @param flags   eli_button_flags.
 * @return        true on the frame the region is pressed.
 */
static inline bool eli_invisible_button(const char *str_id, eli_vec2 size, eli_button_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return false;

    eli_id id = eli_get_id(str_id);
    eli_vec2 pos = ctx->current_window->cursor_pos;
    eli_vec2 resolved = eli_calc_item_size(size, 0.0f, 0.0f);
    eli_rect bb = eli_make_rect(pos.x, pos.y, resolved.x, resolved.y);
    eli_item_size(resolved, 0.0f);
    if (!eli_item_add(id, bb, 0))
        return false;

    bool hovered = false, held = false;
    return eli_button_behavior(bb, id, &hovered, &held, flags);
}

/**
 * A square button drawing a directional arrow.
 *
 * @param str_id  Identity string.
 * @param dir     Arrow direction.
 * @return        true on the frame the button is pressed.
 */
static inline bool eli_arrow_button(const char *str_id, eli_dir dir)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return false;
    const eli_style *style = &ctx->style;

    eli_id id = eli_get_id(str_id);
    float sz = eli_get_frame_height();
    eli_vec2 pos = ctx->current_window->cursor_pos;
    eli_rect bb = eli_make_rect(pos.x, pos.y, sz, sz);
    eli_item_size(eli_make_vec2(sz, sz), style->frame_padding.y);
    if (!eli_item_add(id, bb, 0))
        return false;

    bool hovered = false, held = false;
    bool pressed = eli_button_behavior(bb, id, &hovered, &held, ELI_BUTTON_NONE);

    eli_render_nav_highlight(bb, id);
    eli_col32 col = eli_get_color_u32(
        (held && hovered) ? ELI_COL_BUTTON_ACTIVE : hovered ? ELI_COL_BUTTON_HOVERED : ELI_COL_BUTTON,
        1.0f);
    eli_render_frame(eli_rect_min(bb), eli_rect_max(bb), col, true, style->frame_rounding);
    eli_render_arrow(eli_get_window_draw_list(),
                     eli_make_vec2(pos.x + eli_max_f(0.0f, (sz - eli_get_font_size()) * 0.5f),
                                   pos.y + eli_max_f(0.0f, (sz - eli_get_font_size()) * 0.5f)),
                     eli_get_color_u32(ELI_COL_TEXT, 1.0f), dir, 1.0f);
    return pressed;
}

/* ---------------------------------------------------------------------------
 * Checkboxes
 * ------------------------------------------------------------------------- */

/**
 * A labelled checkbox bound to a bool.
 *
 * @param label  Checkbox label.
 * @param v      Pointer to the bool to toggle (must be non-NULL).
 * @return       true on the frame the checkbox is toggled.
 */
static inline bool eli_checkbox(const char *label, bool *v)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items || v == NULL)
        return false;
    const eli_style *style = &ctx->style;

    eli_id id = eli_get_id(label);
    eli_vec2 label_size = eli_calc_text_size(label, eli_find_rendered_text_end(label, NULL));
    eli_vec2 pos = ctx->current_window->cursor_pos;
    float square_sz = eli_get_frame_height();

    float total_w = square_sz + (label_size.x > 0.0f ? style->item_inner_spacing.x + label_size.x
                                                     : 0.0f);
    float total_h = eli_max_f(square_sz, label_size.y + style->frame_padding.y * 2.0f);
    eli_rect bb = eli_make_rect(pos.x, pos.y, total_w, total_h);
    eli_item_size(eli_rect_size(bb), style->frame_padding.y);
    if (!eli_item_add(id, bb, 0))
        return false;

    bool hovered = false, held = false;
    bool pressed = eli_button_behavior(bb, id, &hovered, &held, ELI_BUTTON_NONE);
    if (pressed) {
        *v = !*v;
        eli_mark_item_edited(id);
    }

    eli_render_nav_highlight(bb, id);
    eli_vec2 check_min = pos;
    eli_vec2 check_max = eli_make_vec2(pos.x + square_sz, pos.y + square_sz);
    eli_render_frame(check_min, check_max, eli_widget__frame_bg(hovered, held), true,
                     style->frame_rounding);
    if (*v) {
        float pad = eli_max_f(1.0f, (float)(int)(square_sz / 6.0f));
        eli_render_check_mark(eli_get_window_draw_list(),
                              eli_make_vec2(check_min.x + pad, check_min.y + pad),
                              eli_get_color_u32(ELI_COL_CHECK_MARK, 1.0f), square_sz - pad * 2.0f);
    }
    if (label_size.x > 0.0f)
        eli_render_text(eli_make_vec2(check_max.x + style->item_inner_spacing.x,
                                      pos.y + style->frame_padding.y),
                        eli_get_color_u32(ELI_COL_TEXT, 1.0f), label, NULL, true);
    return pressed;
}

/**
 * A checkbox that sets/clears a bit mask within an int.
 *
 * @param label        Checkbox label.
 * @param flags        Pointer to the int flags field (must be non-NULL).
 * @param flags_value  Bit mask this checkbox controls.
 * @return             true on the frame it is toggled.
 */
static inline bool eli_checkbox_flags_int(const char *label, int *flags, int flags_value)
{
    if (flags == NULL)
        return false;
    bool all_on = (*flags & flags_value) == flags_value;
    bool pressed = eli_checkbox(label, &all_on);
    if (pressed) {
        if (all_on)
            *flags |= flags_value;
        else
            *flags &= ~flags_value;
    }
    return pressed;
}

/**
 * A checkbox that sets/clears a bit mask within an unsigned int.
 *
 * @param label        Checkbox label.
 * @param flags        Pointer to the unsigned flags field (must be non-NULL).
 * @param flags_value  Bit mask this checkbox controls.
 * @return             true on the frame it is toggled.
 */
static inline bool eli_checkbox_flags_uint(const char *label, unsigned int *flags,
                                           unsigned int flags_value)
{
    if (flags == NULL)
        return false;
    bool all_on = (*flags & flags_value) == flags_value;
    bool pressed = eli_checkbox(label, &all_on);
    if (pressed) {
        if (all_on)
            *flags |= flags_value;
        else
            *flags &= ~flags_value;
    }
    return pressed;
}

/* ---------------------------------------------------------------------------
 * Radio buttons
 * ------------------------------------------------------------------------- */

/**
 * A labelled radio button showing an on/off state (caller owns the selection).
 *
 * @param label   Radio label.
 * @param active  Whether this radio is the selected one.
 * @return        true on the frame the radio is pressed.
 */
static inline bool eli_radio_button(const char *label, bool active)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return false;
    const eli_style *style = &ctx->style;

    eli_id id = eli_get_id(label);
    eli_vec2 label_size = eli_calc_text_size(label, eli_find_rendered_text_end(label, NULL));
    eli_vec2 pos = ctx->current_window->cursor_pos;
    float square_sz = eli_get_frame_height();

    float total_w = square_sz + (label_size.x > 0.0f ? style->item_inner_spacing.x + label_size.x
                                                     : 0.0f);
    float total_h = eli_max_f(square_sz, label_size.y + style->frame_padding.y * 2.0f);
    eli_rect bb = eli_make_rect(pos.x, pos.y, total_w, total_h);
    eli_item_size(eli_rect_size(bb), style->frame_padding.y);
    if (!eli_item_add(id, bb, 0))
        return false;

    bool hovered = false, held = false;
    bool pressed = eli_button_behavior(bb, id, &hovered, &held, ELI_BUTTON_NONE);
    if (pressed)
        eli_mark_item_edited(id);

    eli_draw_list *dl = eli_get_window_draw_list();
    eli_vec2 center = eli_make_vec2(pos.x + square_sz * 0.5f, pos.y + square_sz * 0.5f);
    float radius = square_sz * 0.5f;
    eli_render_nav_highlight(bb, id);
    eli_draw_list_add_circle_filled(dl, center, radius, eli_widget__frame_bg(hovered, held), 16);
    if (active) {
        float pad = eli_max_f(1.0f, (float)(int)(square_sz / 6.0f));
        eli_draw_list_add_circle_filled(dl, center, radius - pad,
                                        eli_get_color_u32(ELI_COL_CHECK_MARK, 1.0f), 16);
    }
    if (style->frame_border_size > 0.0f)
        eli_draw_list_add_circle(dl, center, radius, eli_get_color_u32(ELI_COL_BORDER, 1.0f), 16,
                                 style->frame_border_size);
    if (label_size.x > 0.0f)
        eli_render_text(eli_make_vec2(pos.x + square_sz + style->item_inner_spacing.x,
                                      pos.y + style->frame_padding.y),
                        eli_get_color_u32(ELI_COL_TEXT, 1.0f), label, NULL, true);
    return pressed;
}

/**
 * A radio button bound to an int selection.
 *
 * @param label     Radio label.
 * @param v         Pointer to the selection value (must be non-NULL).
 * @param v_button  Value this radio represents.
 * @return          true on the frame it is pressed.
 */
static inline bool eli_radio_button_int(const char *label, int *v, int v_button)
{
    if (v == NULL)
        return false;
    bool pressed = eli_radio_button(label, *v == v_button);
    if (pressed)
        *v = v_button;
    return pressed;
}

/* ---------------------------------------------------------------------------
 * Progress bar
 * ------------------------------------------------------------------------- */

/**
 * A horizontal progress bar with an optional centered overlay label.
 *
 * @param fraction  Progress in [0,1] (clamped).
 * @param size_arg  Requested size (0 width = item width, 0 height = frame height).
 * @param overlay   Overlay text, or NULL for a "NN%" default.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_progress_bar(float fraction, eli_vec2 size_arg, const char *overlay)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return;
    const eli_style *style = &ctx->style;

    fraction = eli_clamp_f(fraction, 0.0f, 1.0f);
    eli_vec2 pos = ctx->current_window->cursor_pos;
    eli_vec2 size = eli_calc_item_size(size_arg, eli_calc_item_width(), eli_get_frame_height());
    eli_rect bb = eli_make_rect(pos.x, pos.y, size.x, size.y);
    eli_item_size(size, style->frame_padding.y);
    if (!eli_item_add(0u, bb, 0))
        return;

    eli_vec2 mn = eli_rect_min(bb);
    eli_vec2 mx = eli_rect_max(bb);
    eli_render_frame(mn, mx, eli_get_color_u32(ELI_COL_FRAME_BG, 1.0f), true, style->frame_rounding);

    eli_draw_list *dl = eli_get_window_draw_list();
    float fill_x = mn.x + fraction * (mx.x - mn.x);
    eli_draw_list_add_rect_filled(dl, mn, eli_make_vec2(fill_x, mx.y),
                                  eli_get_color_u32(ELI_COL_PLOT_HISTOGRAM, 1.0f),
                                  style->frame_rounding, ELI_DRAW_ROUND_CORNERS_ALL);

    char buf[32];
    const char *ov = overlay;
    const char *ov_end = NULL;
    if (ov == NULL) {
        int n = snprintf(buf, sizeof(buf), "%d%%", (int)(fraction * 100.0f + 0.5f));
        ov = buf;
        ov_end = buf + (n > 0 ? n : 0);
    }
    eli_vec2 ov_size = eli_calc_text_size(ov, ov_end);
    if (ov_size.x > 0.0f) {
        float tx = eli_clamp_f(fill_x + style->item_inner_spacing.x, mn.x,
                               mx.x - ov_size.x - style->item_inner_spacing.x);
        float ty = mn.y + (size.y - ov_size.y) * 0.5f;
        eli_render_text(eli_make_vec2(tx, ty), eli_get_color_u32(ELI_COL_TEXT, 1.0f), ov, ov_end,
                        false);
    }
}

/* ---------------------------------------------------------------------------
 * Text links
 * ------------------------------------------------------------------------- */

/**
 * A clickable, underlined text hyperlink.
 *
 * @param label  Link label (its visible part stops at "##").
 * @return       true on the frame the link is clicked.
 */
static inline bool eli_text_link(const char *label)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return false;

    eli_id id = eli_get_id(label);
    const char *label_end = eli_find_rendered_text_end(label, NULL);
    eli_vec2 label_size = eli_calc_text_size(label, label_end);
    eli_vec2 pos = ctx->current_window->cursor_pos;
    eli_rect bb = eli_make_rect(pos.x, pos.y, label_size.x, label_size.y);
    eli_item_size(label_size, 0.0f);
    if (!eli_item_add(id, bb, 0))
        return false;

    bool hovered = false, held = false;
    bool pressed = eli_button_behavior(bb, id, &hovered, &held, ELI_BUTTON_NONE);

    eli_col32 col = eli_get_color_u32(ELI_COL_TEXT_LINK, 1.0f);
    eli_draw_list *dl = eli_get_window_draw_list();
    float underline_y = pos.y + label_size.y - 1.0f;
    if (hovered || held)
        eli_draw_list_add_line(dl, eli_make_vec2(pos.x, underline_y),
                               eli_make_vec2(pos.x + label_size.x, underline_y), col, 1.0f);
    eli_render_text(pos, col, label, label_end, false);
    return pressed;
}

/**
 * A text link that opens a URL in the browser when clicked (browser-only; the
 * open is a no-op in hosted/test builds).
 *
 * @param label  Link label.
 * @param url    URL to open on click.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_text_link_open_url(const char *label, const char *url)
{
    (void)url;
    if (eli_text_link(label)) {
#ifdef ELI_JSIO
        if (url != NULL)
            eli_host_open_url(url);
#endif
    }
}

#endif /* ELI_WIDGETS_ELI_BUTTON_WIDGETS_H */
