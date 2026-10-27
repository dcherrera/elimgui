/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_style_scale.h
 * @brief eli_style_scale_all_sizes: multiply every size-like style field by a
 *        user UI scale (e.g. 1.25 for a Linux 125% desktop).
 *
 * Positions/sizes are in points; the framebuffer density is handled by the
 * renderer and the font rasterizer density, so this is only for a *user* content
 * scale. Fonts are scaled separately (bake them at size x scale).
 *
 * @status In use (native backends, spec §8).
 * @issues None
 * @todo None
 */
#ifndef ELI_STYLE_ELI_STYLE_SCALE_H
#define ELI_STYLE_ELI_STYLE_SCALE_H

#include "../core/eli_platform.h"
#include "../core/eli_style_types.h"

/** Scale both axes of a 2D size field. */
static inline eli_vec2 eli_style__scale_vec2(eli_vec2 v, float s)
{
    return eli_make_vec2(v.x * s, v.y * s);
}

/**
 * Multiply paddings, spacings, roundings, border-free sizes, scrollbar and grab
 * sizes, tab sizes, separator and touch padding by `scale_factor`. Alignment
 * fractions, alpha, angles, borders (kept crisp at 1 point) and colors are not
 * touched. Call once on a freshly-defaulted style; scaling twice compounds.
 *
 * @param style         Style to modify (NULL is ignored).
 * @param scale_factor  Multiplier (> 0).
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline void eli_style_scale_all_sizes(eli_style *style, float scale_factor)
{
    if (!style || scale_factor <= 0.0f)
        return;
    float s = scale_factor;
    style->window_padding = eli_style__scale_vec2(style->window_padding, s);
    style->window_rounding *= s;
    style->window_min_size = eli_style__scale_vec2(style->window_min_size, s);
    style->child_rounding *= s;
    style->popup_rounding *= s;
    style->frame_padding = eli_style__scale_vec2(style->frame_padding, s);
    style->frame_rounding *= s;
    style->item_spacing = eli_style__scale_vec2(style->item_spacing, s);
    style->item_inner_spacing = eli_style__scale_vec2(style->item_inner_spacing, s);
    style->cell_padding = eli_style__scale_vec2(style->cell_padding, s);
    style->touch_extra_padding = eli_style__scale_vec2(style->touch_extra_padding, s);
    style->indent_spacing *= s;
    style->columns_min_spacing *= s;
    style->scrollbar_size *= s;
    style->scrollbar_rounding *= s;
    style->grab_min_size *= s;
    style->grab_rounding *= s;
    style->log_slider_deadzone *= s;
    style->tab_rounding *= s;
    style->tab_min_width_for_close_button *= s;
    style->tab_bar_overline_size *= s;
    style->separator_text_padding = eli_style__scale_vec2(style->separator_text_padding, s);
    style->display_window_padding = eli_style__scale_vec2(style->display_window_padding, s);
    style->display_safe_area_padding = eli_style__scale_vec2(style->display_safe_area_padding, s);
    style->docking_separator_size *= s;
    style->mouse_cursor_scale *= s;
}

#endif /* ELI_STYLE_ELI_STYLE_SCALE_H */
