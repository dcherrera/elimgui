/**
 * @file eli_slider_behavior.h
 * @brief Shared interaction core for the slider and drag widgets: the scalar
 *        data-type abstraction (read/write/format/clamp across every
 *        eli_data_type), the linear/logarithmic parametric mapping
 *        (eli_scale_ratio_from_value / eli_scale_value_from_ratio), and the two
 *        internal behaviors eli_slider_behavior (track-relative, mouse maps x/y to
 *        value) and eli_drag_behavior (delta-accumulating, mouse-drag adjusts
 *        value). Mirrors Dear ImGui's SliderBehavior / DragBehavior contracts.
 *
 * Values are carried through the math as double; on write they are rounded (for
 * integer types) and clamped to the destination type's native range. The 64-bit
 * integer types lose precision beyond 2^53 (the double mantissa), matching the
 * "half natural range" caveat Dear ImGui documents for large slider ranges.
 *
 * @status Phase 11 slider/drag behavior core in use.
 * @issues 64-bit integer values above 2^53 are imprecise (double carrier).
 * @todo None
 */
#ifndef ELI_WIDGETS_ELI_SLIDER_BEHAVIOR_H
#define ELI_WIDGETS_ELI_SLIDER_BEHAVIOR_H

#include "eli_widget_behavior.h"

#include "../core/eli_platform.h"
#include "../core/eli_enums.h"
#include "../id/eli_id.h"
#include "../input/eli_input.h"
#include "../style/eli_style.h"

#include <stdio.h>

/* ---------------------------------------------------------------------------
 * Private slider flag bits (elimgui extension, mirror ImGuiSliderFlagsPrivate).
 * Kept out of the public eli_slider_flags enum so the API surface stays clean.
 * ------------------------------------------------------------------------- */

enum eli_slider_flags_private_ {
    ELI_SLIDER_VERTICAL  = 1 << 20, /* slider runs along Y instead of X */
    ELI_SLIDER_READ_ONLY = 1 << 21  /* behavior computes a grab but never writes */
};

/* Drag speed default: fraction of the value range covered per pixel when v_speed
 * is left at 0 and the range is bounded. Matches Dear ImGui's DragSpeedDefaultRatio. */
#define ELI_DRAG_SPEED_DEFAULT_RATIO 0.01f

/* The drag must move past this fraction of the mouse drag threshold before it
 * starts adjusting, so a plain click does not nudge the value. */
#define ELI_DRAG_MOUSE_THRESHOLD_FACTOR 0.50f

/* Padding between the track edges and the grab travel, matching Dear ImGui. */
#define ELI_SLIDER_GRAB_PADDING 2.0f

/* ---------------------------------------------------------------------------
 * Module-private drag accumulator
 *
 * Drag adjustments accumulate here until they cross the value's display
 * precision, so slow dragging still moves sub-pixel amounts (matches Dear ImGui's
 * g.DragCurrentAccum). Only one widget is active at a time, so a single slot
 * keyed by the active id is sufficient; this replaces the per-context scratch
 * without touching eli_context.
 * ------------------------------------------------------------------------- */

static float eli_g_slider_drag_accum = 0.0f;
static bool  eli_g_slider_drag_accum_dirty = false;
static eli_id eli_g_slider_drag_accum_id = 0u;

/* ---------------------------------------------------------------------------
 * Scalar data-type abstraction
 * ------------------------------------------------------------------------- */

/** @return true for the floating-point data types (float / double). */
static inline bool eli_data_type_is_float(eli_data_type dt)
{
    return dt == ELI_DATA_TYPE_FLOAT || dt == ELI_DATA_TYPE_DOUBLE;
}

/** @return the size in bytes of one element of the given data type. */
static inline size_t eli_data_type_size(eli_data_type dt)
{
    switch (dt) {
    case ELI_DATA_TYPE_S8:
    case ELI_DATA_TYPE_U8:     return 1u;
    case ELI_DATA_TYPE_S16:
    case ELI_DATA_TYPE_U16:    return 2u;
    case ELI_DATA_TYPE_S32:
    case ELI_DATA_TYPE_U32:    return 4u;
    case ELI_DATA_TYPE_S64:
    case ELI_DATA_TYPE_U64:    return 8u;
    case ELI_DATA_TYPE_FLOAT:  return sizeof(float);
    case ELI_DATA_TYPE_DOUBLE: return sizeof(double);
    default:                   return 4u;
    }
}

/** @return the default printf format for the data type (used when format==NULL). */
static inline const char *eli_data_type_default_format(eli_data_type dt)
{
    switch (dt) {
    case ELI_DATA_TYPE_S8:
    case ELI_DATA_TYPE_S16:
    case ELI_DATA_TYPE_S32:    return "%d";
    case ELI_DATA_TYPE_U8:
    case ELI_DATA_TYPE_U16:
    case ELI_DATA_TYPE_U32:    return "%u";
    case ELI_DATA_TYPE_S64:    return "%lld";
    case ELI_DATA_TYPE_U64:    return "%llu";
    case ELI_DATA_TYPE_FLOAT:  return "%.3f";
    case ELI_DATA_TYPE_DOUBLE: return "%f";
    default:                   return "%d";
    }
}

/** Read a scalar of the given type from memory as a double. */
static inline double eli_scalar_read(eli_data_type dt, const void *p)
{
    switch (dt) {
    case ELI_DATA_TYPE_S8:     return (double)*(const int8_t *)p;
    case ELI_DATA_TYPE_U8:     return (double)*(const uint8_t *)p;
    case ELI_DATA_TYPE_S16:    return (double)*(const int16_t *)p;
    case ELI_DATA_TYPE_U16:    return (double)*(const uint16_t *)p;
    case ELI_DATA_TYPE_S32:    return (double)*(const int32_t *)p;
    case ELI_DATA_TYPE_U32:    return (double)*(const uint32_t *)p;
    case ELI_DATA_TYPE_S64:    return (double)*(const int64_t *)p;
    case ELI_DATA_TYPE_U64:    return (double)*(const uint64_t *)p;
    case ELI_DATA_TYPE_FLOAT:  return (double)*(const float *)p;
    case ELI_DATA_TYPE_DOUBLE: return *(const double *)p;
    default:                   return 0.0;
    }
}

/** Round a double to the nearest integer (half away from zero). */
static inline double eli_scalar_round_nearest(double v)
{
    return v >= 0.0 ? (double)(int64_t)(v + 0.5) : (double)(int64_t)(v - 0.5);
}

/**
 * Write a double back into a scalar of the given type, rounding to nearest for
 * integer types and clamping to that type's representable range so the store
 * never overflows.
 */
static inline void eli_scalar_write(eli_data_type dt, void *p, double v)
{
    switch (dt) {
    case ELI_DATA_TYPE_S8:
        *(int8_t *)p = (int8_t)eli_scalar_round_nearest(eli_clamp_f((float)v, -128.0f, 127.0f));
        break;
    case ELI_DATA_TYPE_U8:
        *(uint8_t *)p = (uint8_t)eli_scalar_round_nearest(eli_clamp_f((float)v, 0.0f, 255.0f));
        break;
    case ELI_DATA_TYPE_S16:
        *(int16_t *)p = (int16_t)eli_scalar_round_nearest(eli_clamp_f((float)v, -32768.0f, 32767.0f));
        break;
    case ELI_DATA_TYPE_U16:
        *(uint16_t *)p = (uint16_t)eli_scalar_round_nearest(eli_clamp_f((float)v, 0.0f, 65535.0f));
        break;
    case ELI_DATA_TYPE_S32: {
        double r = eli_scalar_round_nearest(v);
        r = r < -2147483648.0 ? -2147483648.0 : (r > 2147483647.0 ? 2147483647.0 : r);
        *(int32_t *)p = (int32_t)r;
        break;
    }
    case ELI_DATA_TYPE_U32: {
        double r = eli_scalar_round_nearest(v);
        r = r < 0.0 ? 0.0 : (r > 4294967295.0 ? 4294967295.0 : r);
        *(uint32_t *)p = (uint32_t)r;
        break;
    }
    case ELI_DATA_TYPE_S64: {
        double r = eli_scalar_round_nearest(v);
        *(int64_t *)p = (int64_t)r;
        break;
    }
    case ELI_DATA_TYPE_U64: {
        double r = eli_scalar_round_nearest(v);
        *(uint64_t *)p = r < 0.0 ? 0u : (uint64_t)r;
        break;
    }
    case ELI_DATA_TYPE_FLOAT:  *(float *)p = (float)v; break;
    case ELI_DATA_TYPE_DOUBLE: *(double *)p = v; break;
    default: break;
    }
}

/**
 * Format a scalar value into buf using the printf format string.
 *
 * @return the number of characters written (excluding the NUL), clamped to the
 *         buffer size.
 */
static inline int eli_scalar_format(char *buf, size_t buf_size, eli_data_type dt, const void *p,
                                    const char *format)
{
    int n = 0;
    switch (dt) {
    case ELI_DATA_TYPE_S8:     n = snprintf(buf, buf_size, format, (int)*(const int8_t *)p); break;
    case ELI_DATA_TYPE_U8:     n = snprintf(buf, buf_size, format, (unsigned)*(const uint8_t *)p); break;
    case ELI_DATA_TYPE_S16:    n = snprintf(buf, buf_size, format, (int)*(const int16_t *)p); break;
    case ELI_DATA_TYPE_U16:    n = snprintf(buf, buf_size, format, (unsigned)*(const uint16_t *)p); break;
    case ELI_DATA_TYPE_S32:    n = snprintf(buf, buf_size, format, *(const int32_t *)p); break;
    case ELI_DATA_TYPE_U32:    n = snprintf(buf, buf_size, format, *(const uint32_t *)p); break;
    case ELI_DATA_TYPE_S64:    n = snprintf(buf, buf_size, format, (long long)*(const int64_t *)p); break;
    case ELI_DATA_TYPE_U64:    n = snprintf(buf, buf_size, format, (unsigned long long)*(const uint64_t *)p); break;
    case ELI_DATA_TYPE_FLOAT:  n = snprintf(buf, buf_size, format, (double)*(const float *)p); break;
    case ELI_DATA_TYPE_DOUBLE: n = snprintf(buf, buf_size, format, *(const double *)p); break;
    default: break;
    }
    if (n < 0)
        n = 0;
    if ((size_t)n >= buf_size)
        n = (int)(buf_size - 1u);
    return n;
}

/** Clamp *p into [*p_min,*p_max]; @return true if the value changed. */
static inline bool eli_scalar_clamp(eli_data_type dt, void *p, const void *p_min, const void *p_max)
{
    double v = eli_scalar_read(dt, p);
    double lo = p_min ? eli_scalar_read(dt, p_min) : v;
    double hi = p_max ? eli_scalar_read(dt, p_max) : v;
    double c = v < lo ? lo : (v > hi ? hi : v);
    if (c == v)
        return false;
    eli_scalar_write(dt, p, c);
    return true;
}

/* ---------------------------------------------------------------------------
 * Format-precision parsing (for logarithmic epsilon + round-to-format)
 * ------------------------------------------------------------------------- */

/**
 * Parse the number of decimal places from a printf float format (e.g. "%.3f" -> 3).
 *
 * @param format         Format string (may include prefix/suffix decoration).
 * @param default_prec   Precision to return when no ".N" is present.
 * @return               The parsed decimal precision.
 */
static inline int eli_parse_format_precision(const char *format, int default_prec)
{
    if (format == NULL)
        return default_prec;
    const char *p = format;
    while (*p && *p != '%')
        p++;
    if (*p == '\0')
        return default_prec;
    p++;
    while (*p && *p != '.' && *p != '%' &&
           !(*p >= 'a' && *p <= 'z') && !(*p >= 'A' && *p <= 'Z'))
        p++;
    if (*p != '.')
        return default_prec;
    p++;
    int prec = 0;
    bool has_digit = false;
    while (*p >= '0' && *p <= '9') {
        prec = prec * 10 + (*p - '0');
        has_digit = true;
        p++;
    }
    return has_digit ? prec : default_prec;
}

/**
 * Round a floating-point value to the precision implied by its display format, by
 * formatting it and reading it back (mirrors Dear ImGui's RoundScalarWithFormatT).
 * Integer types are returned unchanged.
 */
static inline double eli_round_scalar_with_format(const char *format, eli_data_type dt, double v)
{
    if (!eli_data_type_is_float(dt) || format == NULL)
        return v;
    char buf[64];
    int n = snprintf(buf, sizeof(buf), format, v);
    if (n <= 0)
        return v;
    const char *s = buf;
    while (*s == ' ')
        s++;
    /* Skip any non-numeric prefix so decorated formats like "x=%.2f" still parse. */
    const char *num = s;
    while (*num && !((*num >= '0' && *num <= '9') || *num == '-' || *num == '+' || *num == '.'))
        num++;
    return atof(num);
}

/* ---------------------------------------------------------------------------
 * Parametric mapping between value space and the 0..1 track position
 * ------------------------------------------------------------------------- */

/**
 * Convert a value into its parametric position t in [0,1] along the slider track.
 * Supports linear and logarithmic mapping. Mirrors ScaleRatioFromValueT.
 */
static inline float eli_scale_ratio_from_value(eli_data_type dt, double v, double v_min, double v_max,
                                               bool is_logarithmic, float log_zero_epsilon,
                                               float zero_deadzone_halfsize)
{
    (void)dt;
    if (v_min == v_max)
        return 0.0f;

    double v_clamped = (v_min < v_max) ? (v < v_min ? v_min : (v > v_max ? v_max : v))
                                       : (v < v_max ? v_max : (v > v_min ? v_min : v));
    if (!is_logarithmic)
        return (float)((v_clamped - v_min) / (v_max - v_min));

    bool flipped = v_max < v_min;
    if (flipped) {
        double tmp = v_min;
        v_min = v_max;
        v_max = tmp;
    }
    double eps = (double)log_zero_epsilon;
    double v_min_f = (fabs(v_min) < eps) ? (v_min < 0.0 ? -eps : eps) : v_min;
    double v_max_f = (fabs(v_max) < eps) ? (v_max < 0.0 ? -eps : eps) : v_max;
    if (v_min == 0.0 && v_max < 0.0)
        v_min_f = -eps;
    else if (v_max == 0.0 && v_min < 0.0)
        v_max_f = -eps;

    float result;
    if (v_clamped <= v_min_f) {
        result = 0.0f;
    } else if (v_clamped >= v_max_f) {
        result = 1.0f;
    } else if ((v_min * v_max) < 0.0) {
        float zero_center = (float)(-v_min / (v_max - v_min));
        float zero_l = zero_center - zero_deadzone_halfsize;
        float zero_r = zero_center + zero_deadzone_halfsize;
        if (v == 0.0)
            result = zero_center;
        else if (v < 0.0)
            result = (1.0f - (float)(log(-v_clamped / eps) / log(-v_min_f / eps))) * zero_l;
        else
            result = zero_r + ((float)(log(v_clamped / eps) / log(v_max_f / eps)) * (1.0f - zero_r));
    } else if (v_min < 0.0 || v_max < 0.0) {
        result = 1.0f - (float)(log(-v_clamped / -v_max_f) / log(-v_min_f / -v_max_f));
    } else {
        result = (float)(log(v_clamped / v_min_f) / log(v_max_f / v_min_f));
    }
    return flipped ? (1.0f - result) : result;
}

/**
 * Convert a parametric position t in [0,1] back into a value. Mirrors
 * ScaleValueFromRatioT, including the integer-rounding edge behavior.
 */
static inline double eli_scale_value_from_ratio(eli_data_type dt, float t, double v_min, double v_max,
                                                 bool is_logarithmic, float log_zero_epsilon,
                                                 float zero_deadzone_halfsize)
{
    if (t <= 0.0f || v_min == v_max)
        return v_min;
    if (t >= 1.0f)
        return v_max;

    if (is_logarithmic) {
        double eps = (double)log_zero_epsilon;
        double v_min_f = (fabs(v_min) < eps) ? (v_min < 0.0 ? -eps : eps) : v_min;
        double v_max_f = (fabs(v_max) < eps) ? (v_max < 0.0 ? -eps : eps) : v_max;
        bool flipped = v_max < v_min;
        if (flipped) {
            double tmp = v_min_f;
            v_min_f = v_max_f;
            v_max_f = tmp;
        }
        if (v_max == 0.0 && v_min < 0.0)
            v_max_f = -eps;
        float tf = flipped ? (1.0f - t) : t;

        if ((v_min * v_max) < 0.0) {
            double lo = v_min < v_max ? v_min : v_max;
            float zero_center = (float)(-lo / fabs(v_max - v_min));
            float zero_l = zero_center - zero_deadzone_halfsize;
            float zero_r = zero_center + zero_deadzone_halfsize;
            if (tf >= zero_l && tf <= zero_r)
                return 0.0;
            if (tf < zero_center)
                return -(eps * pow(-v_min_f / eps, (double)(1.0f - (tf / zero_l))));
            return eps * pow(v_max_f / eps, (double)((tf - zero_r) / (1.0f - zero_r)));
        }
        if (v_min < 0.0 || v_max < 0.0)
            return -(-v_max_f * pow(-v_min_f / -v_max_f, (double)(1.0f - tf)));
        return v_min_f * pow(v_max_f / v_min_f, (double)tf);
    }

    if (eli_data_type_is_float(dt))
        return v_min + (v_max - v_min) * (double)t;

    /* Integer: round toward the grab so clicking a position matches the grab box. */
    double off = (v_max - v_min) * (double)t;
    return v_min + eli_scalar_round_nearest(off);
}

/* ---------------------------------------------------------------------------
 * Logarithmic constant helpers
 * ------------------------------------------------------------------------- */

/** @return 0.1^precision, the fudge epsilon used by logarithmic sliders/drags. */
static inline float eli_slider__log_epsilon(eli_data_type dt, const char *format)
{
    int prec = eli_data_type_is_float(dt) ? eli_parse_format_precision(format, 3) : 1;
    return (float)pow(0.1, (double)prec);
}

/* ---------------------------------------------------------------------------
 * Slider behavior (track-relative)
 * ------------------------------------------------------------------------- */

/**
 * The internal slider interaction: while the item is the active id, map the mouse
 * position along the track to a new value; always compute the grab rect for the
 * caller to draw. Mirrors Dear ImGui's SliderBehavior.
 *
 * @param bb           Track bounding box (the frame rect).
 * @param id           Active id owner.
 * @param dt           Scalar data type of p_data / p_min / p_max.
 * @param p_data       In/out value pointer.
 * @param p_min        Range minimum pointer.
 * @param p_max        Range maximum pointer.
 * @param format       Display format (for round-to-format + log precision).
 * @param flags        eli_slider_flags plus private axis/read-only bits.
 * @param out_grab_bb  Receives the grab rectangle in screen space.
 * @return             true if the value changed this frame.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_slider_behavior(eli_rect bb, eli_id id, eli_data_type dt, void *p_data,
                                       const void *p_min, const void *p_max, const char *format,
                                       int flags, eli_rect *out_grab_bb)
{
    eli_context *ctx = eli_get_current_context();
    const eli_style *style = eli_get_style();
    if (ctx == NULL || style == NULL || p_data == NULL || out_grab_bb == NULL)
        return false;

    bool is_vertical = (flags & ELI_SLIDER_VERTICAL) != 0;
    bool is_log = (flags & ELI_SLIDER_LOGARITHMIC) != 0;
    bool is_float = eli_data_type_is_float(dt);

    double v_min = eli_scalar_read(dt, p_min);
    double v_max = eli_scalar_read(dt, p_max);
    float v_range = (float)(v_min < v_max ? v_max - v_min : v_min - v_max);

    float bb_min = is_vertical ? bb.y : bb.x;
    float bb_max = is_vertical ? bb.y + bb.h : bb.x + bb.w;
    float slider_sz = (bb_max - bb_min) - ELI_SLIDER_GRAB_PADDING * 2.0f;
    float grab_sz = style->grab_min_size;
    if (!is_float && v_range >= 0.0f)
        grab_sz = eli_max_f(slider_sz / (v_range + 1.0f), style->grab_min_size);
    grab_sz = eli_min_f(grab_sz, slider_sz);
    float usable_sz = slider_sz - grab_sz;
    float usable_pos_min = bb_min + ELI_SLIDER_GRAB_PADDING + grab_sz * 0.5f;
    float usable_pos_max = bb_max - ELI_SLIDER_GRAB_PADDING - grab_sz * 0.5f;

    float log_eps = 0.0f, deadzone = 0.0f;
    if (is_log) {
        log_eps = eli_slider__log_epsilon(dt, format);
        deadzone = (style->log_slider_deadzone * 0.5f) / eli_max_f(usable_sz, 1.0f);
    }

    bool value_changed = false;
    if (ctx->active_id == id) {
        if (!eli_is_mouse_down(ELI_MOUSE_BUTTON_LEFT)) {
            eli_clear_active_id();
        } else {
            float mouse_abs = is_vertical ? eli_get_mouse_pos().y : eli_get_mouse_pos().x;
            float clicked_t = 0.0f;
            if (usable_sz > 0.0f)
                clicked_t = eli_clamp_f((mouse_abs - usable_pos_min) / usable_sz, 0.0f, 1.0f);
            if (is_vertical)
                clicked_t = 1.0f - clicked_t;

            if (!(flags & ELI_SLIDER_READ_ONLY)) {
                double v_new = eli_scale_value_from_ratio(dt, clicked_t, v_min, v_max, is_log,
                                                          log_eps, deadzone);
                if (is_float && !(flags & ELI_SLIDER_NO_ROUND_TO_FORMAT))
                    v_new = eli_round_scalar_with_format(format, dt, v_new);
                if (v_new != eli_scalar_read(dt, p_data)) {
                    eli_scalar_write(dt, p_data, v_new);
                    value_changed = true;
                }
            }
        }
    }

    if (slider_sz < 1.0f) {
        *out_grab_bb = eli_make_rect(bb.x, bb.y, 0.0f, 0.0f);
    } else {
        float grab_t = eli_scale_ratio_from_value(dt, eli_scalar_read(dt, p_data), v_min, v_max,
                                                  is_log, log_eps, deadzone);
        if (is_vertical)
            grab_t = 1.0f - grab_t;
        float grab_pos = usable_pos_min + (usable_pos_max - usable_pos_min) * grab_t;
        if (is_vertical)
            *out_grab_bb = eli_make_rect(bb.x + ELI_SLIDER_GRAB_PADDING, grab_pos - grab_sz * 0.5f,
                                         bb.w - ELI_SLIDER_GRAB_PADDING * 2.0f, grab_sz);
        else
            *out_grab_bb = eli_make_rect(grab_pos - grab_sz * 0.5f, bb.y + ELI_SLIDER_GRAB_PADDING,
                                         grab_sz, bb.h - ELI_SLIDER_GRAB_PADDING * 2.0f);
    }
    return value_changed;
}

/* ---------------------------------------------------------------------------
 * Drag behavior (delta-accumulating)
 * ------------------------------------------------------------------------- */

/**
 * The internal drag interaction: while the item is the active id and the mouse is
 * dragging, accumulate the horizontal (or vertical) mouse delta scaled by v_speed
 * and flush it into the value once it crosses the display precision. Mirrors Dear
 * ImGui's DragBehavior.
 *
 * @param id      Active id owner.
 * @param dt      Scalar data type.
 * @param p_data  In/out value pointer.
 * @param v_speed Value change per pixel dragged (0 auto-derives from the range).
 * @param p_min   Range minimum pointer, or NULL for unbounded.
 * @param p_max   Range maximum pointer, or NULL for unbounded.
 * @param format  Display format (for round-to-format + log precision).
 * @param flags   eli_slider_flags plus private axis bits.
 * @return        true if the value changed this frame.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_drag_behavior(eli_id id, eli_data_type dt, void *p_data, float v_speed,
                                     const void *p_min, const void *p_max, const char *format,
                                     int flags)
{
    eli_context *ctx = eli_get_current_context();
    const eli_io *io = eli_get_io();
    if (ctx == NULL || io == NULL || p_data == NULL || ctx->active_id != id)
        return false;
    if (!eli_is_mouse_down(ELI_MOUSE_BUTTON_LEFT)) {
        eli_clear_active_id();
        return false;
    }
    if (flags & ELI_SLIDER_READ_ONLY)
        return false;

    /* Reset the accumulator when a different widget becomes active. */
    if (eli_g_slider_drag_accum_id != id) {
        eli_g_slider_drag_accum_id = id;
        eli_g_slider_drag_accum = 0.0f;
        eli_g_slider_drag_accum_dirty = false;
    }

    bool is_vertical = (flags & ELI_SLIDER_VERTICAL) != 0;
    bool is_log = (flags & ELI_SLIDER_LOGARITHMIC) != 0;
    bool is_float = eli_data_type_is_float(dt);

    double v_min = p_min ? eli_scalar_read(dt, p_min) : 0.0;
    double v_max = p_max ? eli_scalar_read(dt, p_max) : 0.0;
    bool is_bounded = (p_min && p_max) &&
        ((v_min < v_max) || ((v_min == v_max) && (v_min != 0.0 ||
                             (flags & ELI_SLIDER_CLAMP_ZERO_RANGE))));
    bool is_wrapped = is_bounded && (flags & ELI_SLIDER_WRAP_AROUND);

    if (v_speed == 0.0f && is_bounded && (v_max - v_min < FLT_MAX))
        v_speed = (float)((v_max - v_min) * ELI_DRAG_SPEED_DEFAULT_RATIO);

    float adjust_delta = 0.0f;
    if (eli_is_mouse_pos_valid(NULL) &&
        eli_is_mouse_dragging(ELI_MOUSE_BUTTON_LEFT,
                              io->mouse_drag_threshold * ELI_DRAG_MOUSE_THRESHOLD_FACTOR)) {
        adjust_delta = is_vertical ? io->mouse_delta.y : io->mouse_delta.x;
        if (io->key_alt)
            adjust_delta *= 1.0f / 100.0f;
        if (io->key_shift)
            adjust_delta *= 10.0f;
    }
    adjust_delta *= v_speed;
    if (is_vertical)
        adjust_delta = -adjust_delta;
    if (is_log && is_bounded && (v_max - v_min < FLT_MAX) && (v_max - v_min > 0.000001))
        adjust_delta /= (float)(v_max - v_min);

    double v_cur0 = eli_scalar_read(dt, p_data);
    bool past_limits_outward = is_bounded && !is_wrapped &&
        ((v_cur0 >= v_max && adjust_delta > 0.0f) || (v_cur0 <= v_min && adjust_delta < 0.0f));
    if (ctx->active_id_is_just_activated || past_limits_outward) {
        eli_g_slider_drag_accum = 0.0f;
        eli_g_slider_drag_accum_dirty = false;
    } else if (adjust_delta != 0.0f) {
        eli_g_slider_drag_accum += adjust_delta;
        eli_g_slider_drag_accum_dirty = true;
    }
    if (!eli_g_slider_drag_accum_dirty)
        return false;

    double v_cur = v_cur0;
    float log_eps = is_log ? eli_slider__log_epsilon(dt, format) : 0.0f;
    double v_old_parametric = 0.0;
    if (is_log) {
        float p_old = eli_scale_ratio_from_value(dt, v_cur, v_min, v_max, true, log_eps, 0.0f);
        float p_new = p_old + eli_g_slider_drag_accum;
        v_cur = eli_scale_value_from_ratio(dt, p_new, v_min, v_max, true, log_eps, 0.0f);
        v_old_parametric = p_old;
    } else if (is_float) {
        v_cur += (double)eli_g_slider_drag_accum;
    } else {
        v_cur += (double)(int64_t)eli_g_slider_drag_accum;
    }

    if (is_float && !(flags & ELI_SLIDER_NO_ROUND_TO_FORMAT))
        v_cur = eli_round_scalar_with_format(format, dt, v_cur);

    eli_g_slider_drag_accum_dirty = false;
    if (is_log) {
        float p_new = eli_scale_ratio_from_value(dt, v_cur, v_min, v_max, true, log_eps, 0.0f);
        eli_g_slider_drag_accum -= (float)((double)p_new - v_old_parametric);
    } else {
        eli_g_slider_drag_accum -= (float)(v_cur - v_cur0);
    }

    if (v_cur != v_cur0 && is_bounded) {
        if (is_wrapped) {
            double span = v_max - v_min + (is_float ? 0.0 : 1.0);
            if (v_cur < v_min)
                v_cur += span;
            if (v_cur > v_max)
                v_cur -= span;
        } else {
            if (v_cur < v_min || (v_cur > v_cur0 && adjust_delta < 0.0f && !is_float))
                v_cur = v_min;
            if (v_cur > v_max || (v_cur < v_cur0 && adjust_delta > 0.0f && !is_float))
                v_cur = v_max;
        }
    }

    if (v_cur == v_cur0)
        return false;
    eli_scalar_write(dt, p_data, v_cur);
    return true;
}

#endif /* ELI_WIDGETS_ELI_SLIDER_BEHAVIOR_H */
