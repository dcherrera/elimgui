/**
 * @file eli_slider.h
 * @brief Slider widgets: eli_slider_scalar / _scalar_n and the float / int
 *        convenience wrappers (float, float2/3/4, int, int2/3/4), eli_slider_angle,
 *        and the vertical sliders (eli_v_slider_scalar / _float / _int). Each
 *        measures itself, registers via eli_item_size / eli_item_add, activates on
 *        click, drives eli_slider_behavior, then renders the frame, grab, centered
 *        value text and trailing label. Mirrors Dear ImGui's SliderScalar family.
 *
 * @status Phase 11 slider widgets in use.
 * @issues None
 * @todo Ctrl+click / tab-to-type text entry deferred to the input-text phase.
 */
#ifndef ELI_WIDGETS_ELI_SLIDER_H
#define ELI_WIDGETS_ELI_SLIDER_H

#include "eli_slider_behavior.h"
#include "eli_widget_behavior.h"
#include "eli_text_widgets.h"

#include "../core/eli_platform.h"
#include "../layout/eli_layout.h"
#include "../window/eli_window.h"

/* ---------------------------------------------------------------------------
 * Shared internal helpers
 * ------------------------------------------------------------------------- */

/**
 * Compute whether the frame is hovered for interaction (rect + hovered window +
 * not blocked by another active item + not disabled), and flag the hot id.
 */
static inline bool eli_slider__frame_hovered(eli_context *ctx, eli_rect frame_bb, eli_id id)
{
    bool disabled = (ctx->current_item_flags & ELI_ITEM_DISABLED) != 0;
    bool rect_h = eli_is_mouse_hovering_rect(eli_rect_min(frame_bb), eli_rect_max(frame_bb), true);
    bool win_h = eli_widget__window_hovered(ctx, ctx->current_window);
    bool blocked = (ctx->active_id != 0u && ctx->active_id != id);
    if (disabled || !rect_h || !win_h || blocked)
        return false;
    eli_set_hot_id(id);
    return true;
}

/** Render value text aligned inside the frame, clamped to stay within its left edge. */
static inline void eli_slider__render_value(eli_rect frame_bb, const char *buf, const char *buf_end,
                                            eli_vec2 align)
{
    eli_vec2 ts = eli_calc_text_size(buf, buf_end);
    eli_vec2 fmin = eli_rect_min(frame_bb);
    eli_vec2 fmax = eli_rect_max(frame_bb);
    float x = fmin.x + eli_max_f(0.0f, (fmax.x - fmin.x - ts.x) * align.x);
    float y = fmin.y + eli_max_f(0.0f, (fmax.y - fmin.y - ts.y) * align.y);
    eli_render_text(eli_make_vec2(x, y), eli_get_color_u32(ELI_COL_TEXT, 1.0f), buf, buf_end, false);
}

/**
 * The horizontal scalar slider used by every slider variant.
 *
 * @param label   Identity + trailing label (visible part stops at "##").
 * @param dt      Scalar data type.
 * @param p_data  In/out value pointer.
 * @param p_min   Range minimum pointer.
 * @param p_max   Range maximum pointer.
 * @param format  Display format, or NULL for the type default.
 * @param flags   eli_slider_flags.
 * @return        true if the value changed this frame.
 */
static inline bool eli_slider_scalar(const char *label, eli_data_type dt, void *p_data,
                                     const void *p_min, const void *p_max, const char *format,
                                     eli_slider_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items ||
        p_data == NULL)
        return false;
    const eli_style *style = &ctx->style;
    if (format == NULL)
        format = eli_data_type_default_format(dt);

    eli_id id = eli_get_id(label);
    float w = eli_calc_item_width();
    const char *label_end = eli_find_rendered_text_end(label, NULL);
    eli_vec2 label_size = eli_calc_text_size(label, label_end);
    eli_vec2 pos = ctx->current_window->cursor_pos;

    eli_rect frame_bb = eli_make_rect(pos.x, pos.y, w, label_size.y + style->frame_padding.y * 2.0f);
    float extra = label_size.x > 0.0f ? style->item_inner_spacing.x + label_size.x : 0.0f;
    eli_rect total_bb = eli_make_rect(pos.x, pos.y, frame_bb.w + extra, frame_bb.h);
    eli_item_size(eli_rect_size(total_bb), style->frame_padding.y);
    if (!eli_item_add(id, total_bb, 0))
        return false;

    bool hovered = eli_slider__frame_hovered(ctx, frame_bb, id);
    if (hovered && eli_is_mouse_clicked(ELI_MOUSE_BUTTON_LEFT))
        eli_set_active_id(id);

    eli_col32 frame_col = eli_get_color_u32(ctx->active_id == id ? ELI_COL_FRAME_BG_ACTIVE
                                            : hovered ? ELI_COL_FRAME_BG_HOVERED : ELI_COL_FRAME_BG,
                                            1.0f);
    eli_render_nav_highlight(frame_bb, id);
    eli_render_frame(eli_rect_min(frame_bb), eli_rect_max(frame_bb), frame_col, false,
                     style->frame_rounding);

    eli_rect grab_bb = {0};
    bool value_changed = eli_slider_behavior(frame_bb, id, dt, p_data, p_min, p_max, format, flags,
                                             &grab_bb);
    if (value_changed)
        eli_mark_item_edited(id);

    if (grab_bb.w > 0.0f && grab_bb.h > 0.0f)
        eli_draw_list_add_rect_filled(eli_get_window_draw_list(), eli_rect_min(grab_bb),
                                      eli_rect_max(grab_bb),
                                      eli_get_color_u32(ctx->active_id == id ? ELI_COL_SLIDER_GRAB_ACTIVE
                                                                            : ELI_COL_SLIDER_GRAB, 1.0f),
                                      style->grab_rounding, ELI_DRAW_ROUND_CORNERS_ALL);

    char value_buf[64];
    int n = eli_scalar_format(value_buf, sizeof(value_buf), dt, p_data, format);
    eli_slider__render_value(frame_bb, value_buf, value_buf + n, eli_make_vec2(0.5f, 0.5f));

    if (label_size.x > 0.0f)
        eli_render_text(eli_make_vec2(frame_bb.x + frame_bb.w + style->item_inner_spacing.x,
                                      frame_bb.y + style->frame_padding.y),
                        eli_get_color_u32(ELI_COL_TEXT, 1.0f), label, label_end, false);
    return value_changed;
}

/**
 * A row of `components` horizontal sliders sharing one range and label.
 *
 * @return true if any component changed this frame.
 */
static inline bool eli_slider_scalar_n(const char *label, eli_data_type dt, void *p_data,
                                       int components, const void *p_min, const void *p_max,
                                       const char *format, eli_slider_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items ||
        p_data == NULL || components <= 0)
        return false;
    const eli_style *style = &ctx->style;

    bool value_changed = false;
    eli_begin_group();
    eli_push_id(label);
    float w_full = eli_calc_item_width();
    float spacing = style->item_inner_spacing.x;
    float w_one = eli_max_f(1.0f, (float)(int)((w_full - spacing * (float)(components - 1)) /
                                               (float)components));
    float w_last = eli_max_f(1.0f, (float)(int)(w_full - (w_one + spacing) * (float)(components - 1)));
    size_t type_size = eli_data_type_size(dt);
    char *data = (char *)p_data;
    for (int i = 0; i < components; i++) {
        eli_push_id_int(i);
        if (i > 0)
            eli_same_line(0.0f, spacing);
        eli_set_next_item_width(i == components - 1 ? w_last : w_one);
        value_changed |= eli_slider_scalar("", dt, data + (size_t)i * type_size, p_min, p_max,
                                           format, flags);
        eli_pop_id();
    }
    eli_pop_id();

    const char *label_end = eli_find_rendered_text_end(label, NULL);
    if (label != label_end) {
        eli_same_line(0.0f, spacing);
        eli_text_unformatted(label, label_end);
    }
    eli_end_group();
    return value_changed;
}

/* ---------------------------------------------------------------------------
 * Float sliders
 * ------------------------------------------------------------------------- */

/** A slider over a float value. @return true when the value changes. */
static inline bool eli_slider_float(const char *label, float *v, float v_min, float v_max,
                                    const char *format, eli_slider_flags flags)
{
    return eli_slider_scalar(label, ELI_DATA_TYPE_FLOAT, v, &v_min, &v_max,
                             format ? format : "%.3f", flags);
}

/** A row of two float sliders. */
static inline bool eli_slider_float2(const char *label, float v[2], float v_min, float v_max,
                                     const char *format, eli_slider_flags flags)
{
    return eli_slider_scalar_n(label, ELI_DATA_TYPE_FLOAT, v, 2, &v_min, &v_max,
                               format ? format : "%.3f", flags);
}

/** A row of three float sliders. */
static inline bool eli_slider_float3(const char *label, float v[3], float v_min, float v_max,
                                     const char *format, eli_slider_flags flags)
{
    return eli_slider_scalar_n(label, ELI_DATA_TYPE_FLOAT, v, 3, &v_min, &v_max,
                               format ? format : "%.3f", flags);
}

/** A row of four float sliders. */
static inline bool eli_slider_float4(const char *label, float v[4], float v_min, float v_max,
                                     const char *format, eli_slider_flags flags)
{
    return eli_slider_scalar_n(label, ELI_DATA_TYPE_FLOAT, v, 4, &v_min, &v_max,
                               format ? format : "%.3f", flags);
}

/**
 * A slider that edits an angle stored in radians but displayed/edited in degrees.
 *
 * @param v_rad  In/out angle in radians.
 * @return       true when the angle changes.
 */
static inline bool eli_slider_angle(const char *label, float *v_rad, float v_degrees_min,
                                    float v_degrees_max, const char *format, eli_slider_flags flags)
{
    if (format == NULL)
        format = "%.0f deg";
    float v_deg = (*v_rad) * 360.0f / (2.0f * ELI_PI);
    bool changed = eli_slider_float(label, &v_deg, v_degrees_min, v_degrees_max, format, flags);
    if (changed)
        *v_rad = v_deg * (2.0f * ELI_PI) / 360.0f;
    return changed;
}

/* ---------------------------------------------------------------------------
 * Int sliders
 * ------------------------------------------------------------------------- */

/** A slider over an int value. */
static inline bool eli_slider_int(const char *label, int *v, int v_min, int v_max,
                                  const char *format, eli_slider_flags flags)
{
    return eli_slider_scalar(label, ELI_DATA_TYPE_S32, v, &v_min, &v_max, format, flags);
}

/** A row of two int sliders. */
static inline bool eli_slider_int2(const char *label, int v[2], int v_min, int v_max,
                                   const char *format, eli_slider_flags flags)
{
    return eli_slider_scalar_n(label, ELI_DATA_TYPE_S32, v, 2, &v_min, &v_max, format, flags);
}

/** A row of three int sliders. */
static inline bool eli_slider_int3(const char *label, int v[3], int v_min, int v_max,
                                   const char *format, eli_slider_flags flags)
{
    return eli_slider_scalar_n(label, ELI_DATA_TYPE_S32, v, 3, &v_min, &v_max, format, flags);
}

/** A row of four int sliders. */
static inline bool eli_slider_int4(const char *label, int v[4], int v_min, int v_max,
                                   const char *format, eli_slider_flags flags)
{
    return eli_slider_scalar_n(label, ELI_DATA_TYPE_S32, v, 4, &v_min, &v_max, format, flags);
}

/* ---------------------------------------------------------------------------
 * Vertical sliders
 * ------------------------------------------------------------------------- */

/**
 * A vertical scalar slider of an explicit pixel size (higher = larger value).
 *
 * @param size    Explicit widget size (width x height).
 * @return        true when the value changes.
 */
static inline bool eli_v_slider_scalar(const char *label, eli_vec2 size, eli_data_type dt,
                                       void *p_data, const void *p_min, const void *p_max,
                                       const char *format, eli_slider_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items ||
        p_data == NULL)
        return false;
    const eli_style *style = &ctx->style;
    if (format == NULL)
        format = eli_data_type_default_format(dt);

    eli_id id = eli_get_id(label);
    const char *label_end = eli_find_rendered_text_end(label, NULL);
    eli_vec2 label_size = eli_calc_text_size(label, label_end);
    eli_vec2 pos = ctx->current_window->cursor_pos;

    eli_rect frame_bb = eli_make_rect(pos.x, pos.y, size.x, size.y);
    float extra = label_size.x > 0.0f ? style->item_inner_spacing.x + label_size.x : 0.0f;
    eli_rect total_bb = eli_make_rect(pos.x, pos.y, size.x + extra, size.y);
    eli_item_size(eli_rect_size(total_bb), style->frame_padding.y);
    if (!eli_item_add(id, frame_bb, 0))
        return false;

    bool hovered = eli_slider__frame_hovered(ctx, frame_bb, id);
    if (hovered && eli_is_mouse_clicked(ELI_MOUSE_BUTTON_LEFT))
        eli_set_active_id(id);

    eli_col32 frame_col = eli_get_color_u32(ctx->active_id == id ? ELI_COL_FRAME_BG_ACTIVE
                                            : hovered ? ELI_COL_FRAME_BG_HOVERED : ELI_COL_FRAME_BG,
                                            1.0f);
    eli_render_nav_highlight(frame_bb, id);
    eli_render_frame(eli_rect_min(frame_bb), eli_rect_max(frame_bb), frame_col, true,
                     style->frame_rounding);

    eli_rect grab_bb = {0};
    bool value_changed = eli_slider_behavior(frame_bb, id, dt, p_data, p_min, p_max, format,
                                             flags | ELI_SLIDER_VERTICAL, &grab_bb);
    if (value_changed)
        eli_mark_item_edited(id);

    if (grab_bb.w > 0.0f && grab_bb.h > 0.0f)
        eli_draw_list_add_rect_filled(eli_get_window_draw_list(), eli_rect_min(grab_bb),
                                      eli_rect_max(grab_bb),
                                      eli_get_color_u32(ctx->active_id == id ? ELI_COL_SLIDER_GRAB_ACTIVE
                                                                            : ELI_COL_SLIDER_GRAB, 1.0f),
                                      style->grab_rounding, ELI_DRAW_ROUND_CORNERS_ALL);

    char value_buf[64];
    int n = eli_scalar_format(value_buf, sizeof(value_buf), dt, p_data, format);
    eli_rect text_bb = eli_make_rect(frame_bb.x, frame_bb.y + style->frame_padding.y, frame_bb.w,
                                     frame_bb.h - style->frame_padding.y);
    eli_slider__render_value(text_bb, value_buf, value_buf + n, eli_make_vec2(0.5f, 0.0f));

    if (label_size.x > 0.0f)
        eli_render_text(eli_make_vec2(frame_bb.x + frame_bb.w + style->item_inner_spacing.x,
                                      frame_bb.y + style->frame_padding.y),
                        eli_get_color_u32(ELI_COL_TEXT, 1.0f), label, label_end, false);
    return value_changed;
}

/** A vertical float slider of an explicit size. */
static inline bool eli_v_slider_float(const char *label, eli_vec2 size, float *v, float v_min,
                                      float v_max, const char *format, eli_slider_flags flags)
{
    return eli_v_slider_scalar(label, size, ELI_DATA_TYPE_FLOAT, v, &v_min, &v_max,
                               format ? format : "%.3f", flags);
}

/** A vertical int slider of an explicit size. */
static inline bool eli_v_slider_int(const char *label, eli_vec2 size, int *v, int v_min, int v_max,
                                    const char *format, eli_slider_flags flags)
{
    return eli_v_slider_scalar(label, size, ELI_DATA_TYPE_S32, v, &v_min, &v_max,
                               format ? format : "%d", flags);
}

#endif /* ELI_WIDGETS_ELI_SLIDER_H */
