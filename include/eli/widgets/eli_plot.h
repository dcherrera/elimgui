/**
 * @file eli_plot.h
 * @brief Phase 23 data-plotting widgets: eli_plot_lines, eli_plot_lines_fn,
 *        eli_plot_histogram, and eli_plot_histogram_fn. Each draws a framed graph
 *        region from a float array or a value-getter callback, with auto or
 *        explicit y-scale, optional centred overlay text, and hovered-value
 *        highlighting with an inline value label near the cursor.
 *
 * @status Phase 23 plotting widgets.
 * @issues None
 * @todo None
 */
#ifndef ELI_WIDGETS_ELI_PLOT_H
#define ELI_WIDGETS_ELI_PLOT_H

#include "eli_widget_behavior.h"

#include "../core/eli_platform.h"

#include <stdio.h>

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */

/**
 * Sentinel for scale_min / scale_max: pass ELI_PLOT_SCALE_AUTO to ask the
 * widget to compute the value range from the data each frame. Mirrors Dear
 * ImGui's FLT_MAX convention; FLT_MAX is provided by <float.h> via eli_platform.h.
 */
#ifndef ELI_PLOT_SCALE_AUTO
#define ELI_PLOT_SCALE_AUTO FLT_MAX
#endif

/* ---------------------------------------------------------------------------
 * Internal types
 * --------------------------------------------------------------------------- */

/** Discriminates the two plot modes in the shared implementation. */
typedef enum {
    ELI_PLOT_TYPE_LINES     = 0,
    ELI_PLOT_TYPE_HISTOGRAM = 1
} eli_plot_type;

/** Bundles an array pointer and its byte stride for the array-backed getter. */
typedef struct {
    const float *values;
    int stride_bytes;
} eli_plot_array_data;

/* ---------------------------------------------------------------------------
 * Internal helpers
 * --------------------------------------------------------------------------- */

/**
 * Array-backed value getter: reads the float at byte offset
 * idx * stride_bytes via memcpy so access is always alignment-safe.
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline float eli_plot__array_getter(void *data, int idx)
{
    const eli_plot_array_data *d = (const eli_plot_array_data *)data;
    const char *p = (const char *)d->values + (size_t)idx * (size_t)d->stride_bytes;
    float v = 0.0f;
    memcpy(&v, p, sizeof(v));
    return v;
}

/**
 * Walk all values_count samples through the getter and write the data-driven
 * [min, max] to *out_min / *out_max. Falls back to [0, 1] when values_count
 * is zero so callers never see an uninitialised range.
 *
 * @param getter        Sample accessor.
 * @param data          User data passed to each getter call.
 * @param values_count  Number of samples.
 * @param values_offset Circular start index.
 * @param out_min       Receives the minimum sample value.
 * @param out_max       Receives the maximum sample value.
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline void eli_plot__compute_scale(float (*getter)(void *, int), void *data,
                                           int values_count, int values_offset,
                                           float *out_min, float *out_max)
{
    if (values_count <= 0) {
        *out_min = 0.0f;
        *out_max = 1.0f;
        return;
    }
    float mn = FLT_MAX;
    float mx = -FLT_MAX;
    for (int i = 0; i < values_count; i++) {
        float v = getter(data, (values_offset + i) % values_count);
        if (v < mn) mn = v;
        if (v > mx) mx = v;
    }
    *out_min = (mn == FLT_MAX) ? 0.0f : mn;
    *out_max = (mx == -FLT_MAX) ? 1.0f : mx;
}

/**
 * Map a sample value to a screen-space y coordinate inside the inner frame
 * rect. scale_range must be > 0 (caller guarantees this).
 * y=inner_max.y corresponds to scale_min (bottom); y=inner_min.y to scale_max
 * (top), matching standard math-axis orientation.
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline float eli_plot__value_to_y(float v, float scale_min, float scale_range,
                                         float inner_max_y, float inner_h)
{
    float norm = eli_clamp_f((v - scale_min) / scale_range, 0.0f, 1.0f);
    return inner_max_y - norm * inner_h;
}

/**
 * Emit the line-strip geometry for values_count samples into the window draw
 * list. One heap allocation holds the sample-coordinate array; it is freed
 * before the function returns. The hovered segment (idx_hovered → idx_hovered+1)
 * is overdrawn with col_hovered at 2 px thickness.
 *
 * @param dl            Window draw list (non-NULL, caller checks).
 * @param getter        Sample accessor.
 * @param data          Getter user data.
 * @param values_count  Number of samples (>= 2, caller checks).
 * @param values_offset Circular start index.
 * @param scale_min     Y lower bound.
 * @param scale_range   scale_max - scale_min (> 0).
 * @param inner_min     Frame inner top-left.
 * @param inner_max     Frame inner bottom-right.
 * @param idx_hovered   Hovered segment start index (-1 = none).
 * @param col_normal    Base line colour.
 * @param col_hovered   Highlight colour for the hovered segment.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_plot__emit_lines(eli_draw_list *dl,
                                        float (*getter)(void *, int), void *data,
                                        int values_count, int values_offset,
                                        float scale_min, float scale_range,
                                        eli_vec2 inner_min, eli_vec2 inner_max,
                                        int idx_hovered,
                                        eli_col32 col_normal, eli_col32 col_hovered)
{
    float inner_w = inner_max.x - inner_min.x;
    float inner_h = inner_max.y - inner_min.y;
    if (inner_w <= 0.0f || inner_h <= 0.0f)
        return;

    eli_vec2 *pts = (eli_vec2 *)malloc((size_t)values_count * sizeof(*pts));
    if (pts == NULL)
        return;

    float x_step = inner_w / (float)(values_count - 1);
    for (int i = 0; i < values_count; i++) {
        float v = getter(data, (values_offset + i) % values_count);
        pts[i].x = inner_min.x + (float)i * x_step;
        pts[i].y = eli_plot__value_to_y(v, scale_min, scale_range, inner_max.y, inner_h);
    }

    eli_draw_list_add_polyline(dl, pts, values_count, col_normal, ELI_DRAW_NONE, 1.0f);

    /* Overdraw the hovered segment in the highlight colour and thicker. */
    if (idx_hovered >= 0 && idx_hovered + 1 < values_count)
        eli_draw_list_add_line(dl, pts[idx_hovered], pts[idx_hovered + 1], col_hovered, 2.0f);

    free(pts);
}

/**
 * Emit N filled rectangle bars into the window draw list for values_count
 * samples. Each bar occupies (inner_w / N) pixels of width. The bar at
 * idx_hovered uses col_hovered; all others use col_normal.
 * Zero-height bars (value at or below scale_min) are skipped.
 *
 * @param dl            Window draw list (non-NULL, caller checks).
 * @param getter        Sample accessor.
 * @param data          Getter user data.
 * @param values_count  Number of samples (>= 1, caller checks).
 * @param values_offset Circular start index.
 * @param scale_min     Y lower bound.
 * @param scale_range   scale_max - scale_min (> 0).
 * @param inner_min     Frame inner top-left.
 * @param inner_max     Frame inner bottom-right.
 * @param idx_hovered   Hovered bar index (-1 = none).
 * @param col_normal    Normal bar fill colour.
 * @param col_hovered   Hovered bar fill colour.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_plot__emit_histogram(eli_draw_list *dl,
                                            float (*getter)(void *, int), void *data,
                                            int values_count, int values_offset,
                                            float scale_min, float scale_range,
                                            eli_vec2 inner_min, eli_vec2 inner_max,
                                            int idx_hovered,
                                            eli_col32 col_normal, eli_col32 col_hovered)
{
    float inner_w = inner_max.x - inner_min.x;
    float inner_h = inner_max.y - inner_min.y;
    if (inner_w <= 0.0f || inner_h <= 0.0f)
        return;

    float bar_w = inner_w / (float)values_count;

    for (int i = 0; i < values_count; i++) {
        float v = getter(data, (values_offset + i) % values_count);
        float y_top = eli_plot__value_to_y(v, scale_min, scale_range, inner_max.y, inner_h);
        float x0 = inner_min.x + (float)i * bar_w;
        float x1 = inner_min.x + (float)(i + 1) * bar_w - 1.0f;

        /* Keep bars inside the inner region. */
        if (x1 > inner_max.x - 1.0f) x1 = inner_max.x - 1.0f;
        /* Skip zero-height bars (value normalises to 0). */
        if (x1 <= x0 || y_top >= inner_max.y)
            continue;

        eli_col32 col = (i == idx_hovered) ? col_hovered : col_normal;
        eli_draw_list_add_rect_filled(dl,
            eli_make_vec2(x0, y_top),
            eli_make_vec2(x1, inner_max.y),
            col, 0.0f, ELI_DRAW_ROUND_CORNERS_NONE);
    }
}

/**
 * Render an inline value label adjacent to the mouse cursor so the user can
 * read the sample value under the pointer. Only called when in_frame && idx_hovered >= 0.
 * Lines mode shows two adjacent values; histogram mode shows one.
 *
 * @param dl            Window draw list.
 * @param getter        Sample accessor.
 * @param data          Getter user data.
 * @param values_count  Number of samples.
 * @param values_offset Circular start index.
 * @param idx_hovered   Hovered sample index.
 * @param plot_type     ELI_PLOT_TYPE_LINES or ELI_PLOT_TYPE_HISTOGRAM.
 * @param mouse_pos     Current mouse screen position.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_plot__draw_hover_label(eli_draw_list *dl,
                                              float (*getter)(void *, int), void *data,
                                              int values_count, int values_offset,
                                              int idx_hovered, eli_plot_type plot_type,
                                              eli_vec2 mouse_pos)
{
    char buf[64];
    if (plot_type == ELI_PLOT_TYPE_LINES && idx_hovered + 1 < values_count) {
        float v0 = getter(data, (values_offset + idx_hovered) % values_count);
        float v1 = getter(data, (values_offset + idx_hovered + 1) % values_count);
        snprintf(buf, sizeof(buf), "%d: %.4g\n%d: %.4g",
                 idx_hovered, (double)v0, idx_hovered + 1, (double)v1);
    } else {
        float v = getter(data, (values_offset + idx_hovered) % values_count);
        snprintf(buf, sizeof(buf), "%d: %.4g", idx_hovered, (double)v);
    }
    eli_col32 text_col = eli_get_color_u32(ELI_COL_TEXT, 1.0f);
    eli_draw_list_add_text(dl, eli_make_vec2(mouse_pos.x + 8.0f, mouse_pos.y),
                           text_col, buf, NULL);
}

/* ---------------------------------------------------------------------------
 * Shared implementation
 * --------------------------------------------------------------------------- */

/**
 * Internal driver for all four public plot functions. Sizes the framed widget,
 * auto- or explicitly-scales the data, renders the frame background and border,
 * emits the line strip or histogram bars, handles hover detection and the inline
 * value label, and renders the centred overlay text and widget label.
 *
 * @param label         Widget label ("##" hides the visible portion).
 * @param getter        Sample accessor callback.
 * @param data          User data for the getter.
 * @param values_count  Number of samples.
 * @param values_offset Circular start index.
 * @param overlay_text  Text centred over the plot area, or NULL.
 * @param scale_min     Y lower bound, or ELI_PLOT_SCALE_AUTO.
 * @param scale_max     Y upper bound, or ELI_PLOT_SCALE_AUTO.
 * @param graph_size    Requested pixel size of the framed region (0 → default).
 * @param plot_type     ELI_PLOT_TYPE_LINES or ELI_PLOT_TYPE_HISTOGRAM.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_plot_ex(const char *label,
                               float (*getter)(void *, int), void *data,
                               int values_count, int values_offset,
                               const char *overlay_text,
                               float scale_min, float scale_max,
                               eli_vec2 graph_size, eli_plot_type plot_type)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return;

    const eli_style *style = &ctx->style;
    eli_window *win = ctx->current_window;

    /* Resolve the frame pixel size. */
    float default_w = eli_calc_item_width();
    float default_h = eli_get_text_line_height_with_spacing() * 4.0f;
    eli_vec2 size = eli_calc_item_size(graph_size, default_w, default_h);

    /* Label occupies space to the right of the frame; the id lives in the label. */
    eli_vec2 label_size = eli_calc_text_size(label, eli_find_rendered_text_end(label, NULL));

    eli_vec2 pos = win->cursor_pos;
    eli_rect frame_bb = eli_make_rect(pos.x, pos.y, size.x, size.y);
    eli_item_size(size, style->frame_padding.y);

    eli_id id = eli_get_id(label);
    if (!eli_item_add(id, frame_bb, 0))
        return;

    /* Resolve scale: auto-compute from data or use the caller's explicit bounds. */
    if (values_count > 0 &&
        (scale_min == ELI_PLOT_SCALE_AUTO || scale_max == ELI_PLOT_SCALE_AUTO)) {
        float auto_min = 0.0f, auto_max = 1.0f;
        eli_plot__compute_scale(getter, data, values_count, values_offset,
                                &auto_min, &auto_max);
        if (scale_min == ELI_PLOT_SCALE_AUTO) scale_min = auto_min;
        if (scale_max == ELI_PLOT_SCALE_AUTO) scale_max = auto_max;
    } else {
        if (scale_min == ELI_PLOT_SCALE_AUTO) scale_min = 0.0f;
        if (scale_max == ELI_PLOT_SCALE_AUTO) scale_max = 1.0f;
    }
    /* Prevent a zero-range divide. */
    if (scale_max == scale_min) scale_max = scale_min + 1.0f;
    float scale_range = scale_max - scale_min;

    /* Draw the framed background. */
    eli_vec2 fmin = eli_rect_min(frame_bb);
    eli_vec2 fmax = eli_rect_max(frame_bb);
    eli_render_frame(fmin, fmax,
                     eli_get_color_u32(ELI_COL_FRAME_BG, 1.0f),
                     true, style->frame_rounding);

    /* Inner plot area: 1 px inset from the frame edge on every side. */
    eli_vec2 inner_min = eli_make_vec2(fmin.x + 1.0f, fmin.y + 1.0f);
    eli_vec2 inner_max = eli_make_vec2(fmax.x - 1.0f, fmax.y - 1.0f);

    /* Hover detection: find which sample lies under the mouse. */
    eli_vec2 mouse = eli_get_mouse_pos();
    int idx_hovered = -1;
    bool in_frame = eli_is_mouse_hovering_rect(inner_min, inner_max, false);

    if (in_frame && values_count >= 1) {
        float inner_w = inner_max.x - inner_min.x;
        if (inner_w > 0.0f) {
            float t = eli_clamp_f((mouse.x - inner_min.x) / inner_w, 0.0f, 0.9999f);
            if (plot_type == ELI_PLOT_TYPE_LINES && values_count >= 2) {
                idx_hovered = (int)(t * (float)(values_count - 1));
            } else if (plot_type == ELI_PLOT_TYPE_HISTOGRAM) {
                idx_hovered = (int)(t * (float)values_count);
                if (idx_hovered >= values_count)
                    idx_hovered = values_count - 1;
            }
        }
    }

    /* Resolve plot colours from the style. */
    eli_col32 col_normal = eli_get_color_u32(
        (plot_type == ELI_PLOT_TYPE_LINES) ? ELI_COL_PLOT_LINES
                                           : ELI_COL_PLOT_HISTOGRAM, 1.0f);
    eli_col32 col_hov = eli_get_color_u32(
        (plot_type == ELI_PLOT_TYPE_LINES) ? ELI_COL_PLOT_LINES_HOVERED
                                           : ELI_COL_PLOT_HISTOGRAM_HOVERED, 1.0f);

    /* Emit plot geometry and optional inline hover label. */
    eli_draw_list *dl = eli_get_window_draw_list();
    if (dl != NULL && values_count >= 1) {
        if (plot_type == ELI_PLOT_TYPE_LINES && values_count >= 2) {
            eli_plot__emit_lines(dl, getter, data, values_count, values_offset,
                                 scale_min, scale_range, inner_min, inner_max,
                                 idx_hovered, col_normal, col_hov);
        } else if (plot_type == ELI_PLOT_TYPE_HISTOGRAM) {
            eli_plot__emit_histogram(dl, getter, data, values_count, values_offset,
                                     scale_min, scale_range, inner_min, inner_max,
                                     idx_hovered, col_normal, col_hov);
        }
        if (in_frame && idx_hovered >= 0) {
            eli_plot__draw_hover_label(dl, getter, data, values_count, values_offset,
                                       idx_hovered, plot_type, mouse);
        }
    }

    /* Overlay text centred over the inner plot area. */
    if (overlay_text != NULL) {
        eli_vec2 ov_size = eli_calc_text_size(overlay_text, NULL);
        float ov_w = inner_max.x - inner_min.x;
        float ov_h = inner_max.y - inner_min.y;
        float tx = inner_min.x + eli_max_f(0.0f, (ov_w - ov_size.x) * 0.5f);
        float ty = inner_min.y + eli_max_f(0.0f, (ov_h - ov_size.y) * 0.5f);
        eli_render_text(eli_make_vec2(tx, ty),
                        eli_get_color_u32(ELI_COL_TEXT, 1.0f),
                        overlay_text, NULL, false);
    }

    /* Widget label rendered to the right of the frame. */
    if (label_size.x > 0.0f) {
        eli_render_text(
            eli_make_vec2(fmax.x + style->item_inner_spacing.x,
                          fmin.y + style->frame_padding.y),
            eli_get_color_u32(ELI_COL_TEXT, 1.0f),
            label, NULL, true);
    }
}

/* ---------------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------------- */

/**
 * Draw a line-graph from a contiguous float array.
 *
 * Samples are read circularly starting at values_offset, using byte stride
 * between them. Passing ELI_PLOT_SCALE_AUTO for scale_min or scale_max lets
 * the widget compute that bound from the data each frame.
 *
 * @param label         Widget label ("##" hides the visible portion).
 * @param values        Array of float samples (non-NULL).
 * @param values_count  Number of samples (must be >= 2 for any lines to appear).
 * @param values_offset First sample index for circular access.
 * @param overlay_text  Text centred over the plot, or NULL.
 * @param scale_min     Y lower bound, or ELI_PLOT_SCALE_AUTO.
 * @param scale_max     Y upper bound, or ELI_PLOT_SCALE_AUTO.
 * @param graph_size    Pixel size of the framed region (0 → content default).
 * @param stride        Byte stride between values (0 → sizeof(float)).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_plot_lines(const char *label, const float *values,
                                  int values_count, int values_offset,
                                  const char *overlay_text,
                                  float scale_min, float scale_max,
                                  eli_vec2 graph_size, int stride)
{
    if (values == NULL || values_count <= 0)
        return;
    eli_plot_array_data d = {
        values,
        (stride > 0) ? stride : (int)sizeof(float)
    };
    eli_plot_ex(label, eli_plot__array_getter, &d,
                values_count, values_offset,
                overlay_text, scale_min, scale_max,
                graph_size, ELI_PLOT_TYPE_LINES);
}

/**
 * Draw a line-graph using a value-getter callback.
 *
 * The getter is called as getter(data, idx) for each sample index in
 * [0, values_count). Circular access is handled by the implementation.
 *
 * @param label          Widget label ("##" hides the visible portion).
 * @param values_getter  Callback returning the float at logical index idx.
 * @param data           User data passed to each getter invocation.
 * @param values_count   Number of samples.
 * @param values_offset  Ring-buffer start index.
 * @param overlay_text   Text centred over the plot, or NULL.
 * @param scale_min      Y lower bound, or ELI_PLOT_SCALE_AUTO.
 * @param scale_max      Y upper bound, or ELI_PLOT_SCALE_AUTO.
 * @param graph_size     Pixel size of the framed region.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_plot_lines_fn(const char *label,
                                     float (*values_getter)(void *data, int idx),
                                     void *data, int values_count, int values_offset,
                                     const char *overlay_text,
                                     float scale_min, float scale_max,
                                     eli_vec2 graph_size)
{
    if (values_getter == NULL || values_count <= 0)
        return;
    eli_plot_ex(label, values_getter, data,
                values_count, values_offset,
                overlay_text, scale_min, scale_max,
                graph_size, ELI_PLOT_TYPE_LINES);
}

/**
 * Draw a histogram (vertical bars) from a contiguous float array.
 *
 * Each sample occupies 1/N of the frame width; bar height is proportional to
 * the normalised sample value. The hovered bar is drawn in the
 * ELI_COL_PLOT_HISTOGRAM_HOVERED colour. Zero-height bars are skipped.
 *
 * @param label         Widget label ("##" hides the visible portion).
 * @param values        Array of float samples (non-NULL).
 * @param values_count  Number of samples.
 * @param values_offset First sample index for circular access.
 * @param overlay_text  Text centred over the plot, or NULL.
 * @param scale_min     Y lower bound, or ELI_PLOT_SCALE_AUTO.
 * @param scale_max     Y upper bound, or ELI_PLOT_SCALE_AUTO.
 * @param graph_size    Pixel size of the framed region (0 → content default).
 * @param stride        Byte stride between values (0 → sizeof(float)).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_plot_histogram(const char *label, const float *values,
                                      int values_count, int values_offset,
                                      const char *overlay_text,
                                      float scale_min, float scale_max,
                                      eli_vec2 graph_size, int stride)
{
    if (values == NULL || values_count <= 0)
        return;
    eli_plot_array_data d = {
        values,
        (stride > 0) ? stride : (int)sizeof(float)
    };
    eli_plot_ex(label, eli_plot__array_getter, &d,
                values_count, values_offset,
                overlay_text, scale_min, scale_max,
                graph_size, ELI_PLOT_TYPE_HISTOGRAM);
}

/**
 * Draw a histogram using a value-getter callback.
 *
 * @param label          Widget label ("##" hides the visible portion).
 * @param values_getter  Callback returning the float at logical index idx.
 * @param data           User data for the callback.
 * @param values_count   Number of samples.
 * @param values_offset  Ring-buffer start index.
 * @param overlay_text   Text centred over the plot, or NULL.
 * @param scale_min      Y lower bound, or ELI_PLOT_SCALE_AUTO.
 * @param scale_max      Y upper bound, or ELI_PLOT_SCALE_AUTO.
 * @param graph_size     Pixel size of the framed region.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_plot_histogram_fn(const char *label,
                                         float (*values_getter)(void *data, int idx),
                                         void *data, int values_count, int values_offset,
                                         const char *overlay_text,
                                         float scale_min, float scale_max,
                                         eli_vec2 graph_size)
{
    if (values_getter == NULL || values_count <= 0)
        return;
    eli_plot_ex(label, values_getter, data,
                values_count, values_offset,
                overlay_text, scale_min, scale_max,
                graph_size, ELI_PLOT_TYPE_HISTOGRAM);
}

#endif /* ELI_WIDGETS_ELI_PLOT_H */
