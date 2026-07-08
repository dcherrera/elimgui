/**
 * @file eli_drag.h
 * @brief Drag widgets: eli_drag_scalar / _scalar_n and the float / int convenience
 *        wrappers (float, float2/3/4, int, int2/3/4), plus the paired-range widgets
 *        eli_drag_float_range2 / eli_drag_int_range2. Each measures itself,
 *        registers via eli_item_size / eli_item_add, activates on click, drives
 *        eli_drag_behavior (mouse-drag adjusts the value by v_speed per pixel),
 *        then renders the frame, centered value text and trailing label. Mirrors
 *        Dear ImGui's DragScalar family.
 *
 * @status Phase 11 drag widgets in use.
 * @issues None
 * @todo Ctrl+click / tab-to-type text entry deferred to the input-text phase.
 */
#ifndef ELI_WIDGETS_ELI_DRAG_H
#define ELI_WIDGETS_ELI_DRAG_H

#include "eli_slider_behavior.h"
#include "eli_slider.h"
#include "eli_widget_behavior.h"
#include "eli_text_widgets.h"

#include "../core/eli_platform.h"
#include "../layout/eli_layout.h"
#include "../window/eli_window.h"

#include <limits.h>

/* ---------------------------------------------------------------------------
 * Core drag widget
 * ------------------------------------------------------------------------- */

/**
 * The horizontal scalar drag used by every drag variant.
 *
 * @param label    Identity + trailing label (visible part stops at "##").
 * @param dt       Scalar data type.
 * @param p_data   In/out value pointer.
 * @param v_speed  Value change per pixel dragged (0 auto-derives from the range).
 * @param p_min    Range minimum pointer, or NULL for unbounded.
 * @param p_max    Range maximum pointer, or NULL for unbounded.
 * @param format   Display format, or NULL for the type default.
 * @param flags    eli_slider_flags.
 * @return         true if the value changed this frame.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_drag_scalar(const char *label, eli_data_type dt, void *p_data, float v_speed,
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
    eli_render_frame(eli_rect_min(frame_bb), eli_rect_max(frame_bb), frame_col, true,
                     style->frame_rounding);

    bool value_changed = eli_drag_behavior(id, dt, p_data, v_speed, p_min, p_max, format, flags);
    if (value_changed)
        eli_mark_item_edited(id);

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
 * A row of `components` horizontal drags sharing one speed/range and label.
 *
 * @return true if any component changed this frame.
 */
static inline bool eli_drag_scalar_n(const char *label, eli_data_type dt, void *p_data,
                                     int components, float v_speed, const void *p_min,
                                     const void *p_max, const char *format, eli_slider_flags flags)
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
        value_changed |= eli_drag_scalar("", dt, data + (size_t)i * type_size, v_speed, p_min,
                                         p_max, format, flags);
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
 * Float drags
 * ------------------------------------------------------------------------- */

/** A drag over a float value. */
static inline bool eli_drag_float(const char *label, float *v, float v_speed, float v_min,
                                  float v_max, const char *format, eli_slider_flags flags)
{
    return eli_drag_scalar(label, ELI_DATA_TYPE_FLOAT, v, v_speed, &v_min, &v_max,
                           format ? format : "%.3f", flags);
}

/** A row of two float drags. */
static inline bool eli_drag_float2(const char *label, float v[2], float v_speed, float v_min,
                                   float v_max, const char *format, eli_slider_flags flags)
{
    return eli_drag_scalar_n(label, ELI_DATA_TYPE_FLOAT, v, 2, v_speed, &v_min, &v_max,
                             format ? format : "%.3f", flags);
}

/** A row of three float drags. */
static inline bool eli_drag_float3(const char *label, float v[3], float v_speed, float v_min,
                                   float v_max, const char *format, eli_slider_flags flags)
{
    return eli_drag_scalar_n(label, ELI_DATA_TYPE_FLOAT, v, 3, v_speed, &v_min, &v_max,
                             format ? format : "%.3f", flags);
}

/** A row of four float drags. */
static inline bool eli_drag_float4(const char *label, float v[4], float v_speed, float v_min,
                                   float v_max, const char *format, eli_slider_flags flags)
{
    return eli_drag_scalar_n(label, ELI_DATA_TYPE_FLOAT, v, 4, v_speed, &v_min, &v_max,
                             format ? format : "%.3f", flags);
}

/**
 * Two float drags editing a [current_min, current_max] range side by side, each
 * clamped so the pair stays ordered. You typically pass ELI_SLIDER_ALWAYS_CLAMP.
 *
 * @return true if either endpoint changed this frame.
 */
static inline bool eli_drag_float_range2(const char *label, float *v_current_min,
                                         float *v_current_max, float v_speed, float v_min,
                                         float v_max, const char *format, const char *format_max,
                                         eli_slider_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return false;
    const eli_style *style = &ctx->style;
    float spacing = style->item_inner_spacing.x;

    eli_push_id(label);
    eli_begin_group();
    float w_full = eli_calc_item_width();
    float w_one = eli_max_f(1.0f, (float)(int)((w_full - spacing) / 2.0f));
    float w_last = eli_max_f(1.0f, (float)(int)(w_full - (w_one + spacing)));

    float min_min = (v_min >= v_max) ? -FLT_MAX : v_min;
    float min_max = (v_min >= v_max) ? *v_current_max : eli_min_f(v_max, *v_current_max);
    eli_slider_flags min_flags = flags | ((min_min == min_max) ? ELI_SLIDER_READ_ONLY : 0);
    eli_set_next_item_width(w_one);
    bool changed = eli_drag_scalar("##min", ELI_DATA_TYPE_FLOAT, v_current_min, v_speed, &min_min,
                                   &min_max, format ? format : "%.3f", min_flags);
    eli_same_line(0.0f, spacing);

    float max_min = (v_min >= v_max) ? *v_current_min : eli_max_f(v_min, *v_current_min);
    float max_max = (v_min >= v_max) ? FLT_MAX : v_max;
    eli_slider_flags max_flags = flags | ((max_min == max_max) ? ELI_SLIDER_READ_ONLY : 0);
    eli_set_next_item_width(w_last);
    changed |= eli_drag_scalar("##max", ELI_DATA_TYPE_FLOAT, v_current_max, v_speed, &max_min,
                               &max_max, format_max ? format_max : (format ? format : "%.3f"),
                               max_flags);
    eli_same_line(0.0f, spacing);

    eli_text_unformatted(label, eli_find_rendered_text_end(label, NULL));
    eli_end_group();
    eli_pop_id();
    return changed;
}

/* ---------------------------------------------------------------------------
 * Int drags
 * ------------------------------------------------------------------------- */

/** A drag over an int value. */
static inline bool eli_drag_int(const char *label, int *v, float v_speed, int v_min, int v_max,
                                const char *format, eli_slider_flags flags)
{
    return eli_drag_scalar(label, ELI_DATA_TYPE_S32, v, v_speed, &v_min, &v_max, format, flags);
}

/** A row of two int drags. */
static inline bool eli_drag_int2(const char *label, int v[2], float v_speed, int v_min, int v_max,
                                 const char *format, eli_slider_flags flags)
{
    return eli_drag_scalar_n(label, ELI_DATA_TYPE_S32, v, 2, v_speed, &v_min, &v_max, format, flags);
}

/** A row of three int drags. */
static inline bool eli_drag_int3(const char *label, int v[3], float v_speed, int v_min, int v_max,
                                 const char *format, eli_slider_flags flags)
{
    return eli_drag_scalar_n(label, ELI_DATA_TYPE_S32, v, 3, v_speed, &v_min, &v_max, format, flags);
}

/** A row of four int drags. */
static inline bool eli_drag_int4(const char *label, int v[4], float v_speed, int v_min, int v_max,
                                 const char *format, eli_slider_flags flags)
{
    return eli_drag_scalar_n(label, ELI_DATA_TYPE_S32, v, 4, v_speed, &v_min, &v_max, format, flags);
}

/**
 * Two int drags editing a [current_min, current_max] range side by side, each
 * clamped so the pair stays ordered. You typically pass ELI_SLIDER_ALWAYS_CLAMP.
 *
 * @return true if either endpoint changed this frame.
 */
static inline bool eli_drag_int_range2(const char *label, int *v_current_min, int *v_current_max,
                                       float v_speed, int v_min, int v_max, const char *format,
                                       const char *format_max, eli_slider_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return false;
    const eli_style *style = &ctx->style;
    float spacing = style->item_inner_spacing.x;

    eli_push_id(label);
    eli_begin_group();
    float w_full = eli_calc_item_width();
    float w_one = eli_max_f(1.0f, (float)(int)((w_full - spacing) / 2.0f));
    float w_last = eli_max_f(1.0f, (float)(int)(w_full - (w_one + spacing)));

    int min_min = (v_min >= v_max) ? INT_MIN : v_min;
    int min_max = (v_min >= v_max) ? *v_current_max : (v_max < *v_current_max ? v_max : *v_current_max);
    eli_slider_flags min_flags = flags | ((min_min == min_max) ? ELI_SLIDER_READ_ONLY : 0);
    eli_set_next_item_width(w_one);
    bool changed = eli_drag_scalar("##min", ELI_DATA_TYPE_S32, v_current_min, v_speed, &min_min,
                                   &min_max, format ? format : "%d", min_flags);
    eli_same_line(0.0f, spacing);

    int max_min = (v_min >= v_max) ? *v_current_min : (v_min > *v_current_min ? v_min : *v_current_min);
    int max_max = (v_min >= v_max) ? INT_MAX : v_max;
    eli_slider_flags max_flags = flags | ((max_min == max_max) ? ELI_SLIDER_READ_ONLY : 0);
    eli_set_next_item_width(w_last);
    changed |= eli_drag_scalar("##max", ELI_DATA_TYPE_S32, v_current_max, v_speed, &max_min,
                               &max_max, format_max ? format_max : (format ? format : "%d"),
                               max_flags);
    eli_same_line(0.0f, spacing);

    eli_text_unformatted(label, eli_find_rendered_text_end(label, NULL));
    eli_end_group();
    eli_pop_id();
    return changed;
}

#endif /* ELI_WIDGETS_ELI_DRAG_H */
