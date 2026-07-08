/**
 * @file eli_input_number.h
 * @brief Phase 12 numeric input widgets built on the text editor: eli_input_float
 *        (+ _float2/3/4), eli_input_int (+ _int2/3/4), eli_input_double, and the
 *        generic eli_input_scalar / eli_input_scalar_n. Each formats the bound
 *        value into a text field, parses edits back, and offers optional +/- step
 *        buttons with type-range clamping.
 *
 * While a field is being edited its formatted text is left untouched so the user's
 * keystrokes survive across frames; when inactive it is reformatted from the bound
 * value each frame. The edit buffer is a single module-private static buffer (only
 * one field edits at a time, matching the text editor). Parsing applies on any text
 * change; the +/- buttons step by the caller's step (fast step with Ctrl held).
 *
 * @status Phase 12 numeric input in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WIDGETS_ELI_INPUT_NUMBER_H
#define ELI_WIDGETS_ELI_INPUT_NUMBER_H

#include "eli_input_text_widget.h"
#include "eli_button_widgets.h"
#include "eli_text_widgets.h"

#include "../core/eli_platform.h"

#include <stdio.h>
#include <stdlib.h>

/* Edit/format scratch buffer (single active numeric field at a time). */
#ifndef ELI_INPUT_NUMBER_BUF_SIZE
#define ELI_INPUT_NUMBER_BUF_SIZE 64
#endif

static char g_eli_input_number_buf[ELI_INPUT_NUMBER_BUF_SIZE];

/* ---------------------------------------------------------------------------
 * Scalar data-type helpers: format, parse, step, clamp
 * ------------------------------------------------------------------------- */

/** @return the default printf-style format string for a scalar data type. */
static inline const char *eli_input_number__default_format(eli_data_type type)
{
    switch (type) {
    case ELI_DATA_TYPE_FLOAT:
    case ELI_DATA_TYPE_DOUBLE:
        return "%.3f";
    case ELI_DATA_TYPE_U8:
    case ELI_DATA_TYPE_U16:
    case ELI_DATA_TYPE_U32:
    case ELI_DATA_TYPE_U64:
        return "%u";
    default:
        return "%d";
    }
}

/** Format the scalar at p_data into buf using the given printf format. */
static inline void eli_input_number__format(char *buf, int buf_size, eli_data_type type,
                                            const void *p_data, const char *format)
{
    switch (type) {
    case ELI_DATA_TYPE_S8:  snprintf(buf, (size_t)buf_size, format, (int)*(const int8_t *)p_data); break;
    case ELI_DATA_TYPE_U8:  snprintf(buf, (size_t)buf_size, format, (unsigned)*(const uint8_t *)p_data); break;
    case ELI_DATA_TYPE_S16: snprintf(buf, (size_t)buf_size, format, (int)*(const int16_t *)p_data); break;
    case ELI_DATA_TYPE_U16: snprintf(buf, (size_t)buf_size, format, (unsigned)*(const uint16_t *)p_data); break;
    case ELI_DATA_TYPE_S32: snprintf(buf, (size_t)buf_size, format, *(const int32_t *)p_data); break;
    case ELI_DATA_TYPE_U32: snprintf(buf, (size_t)buf_size, format, *(const uint32_t *)p_data); break;
    case ELI_DATA_TYPE_S64: snprintf(buf, (size_t)buf_size, format, (long long)*(const int64_t *)p_data); break;
    case ELI_DATA_TYPE_U64: snprintf(buf, (size_t)buf_size, format, (unsigned long long)*(const uint64_t *)p_data); break;
    case ELI_DATA_TYPE_FLOAT: snprintf(buf, (size_t)buf_size, format, (double)*(const float *)p_data); break;
    case ELI_DATA_TYPE_DOUBLE: snprintf(buf, (size_t)buf_size, format, *(const double *)p_data); break;
    default: buf[0] = '\0'; break;
    }
}

/** Clamp a double to the representable range of the target integer/real type. */
static inline double eli_input_number__clamp(eli_data_type type, double v)
{
    switch (type) {
    case ELI_DATA_TYPE_S8:  return v < -128.0 ? -128.0 : (v > 127.0 ? 127.0 : v);
    case ELI_DATA_TYPE_U8:  return v < 0.0 ? 0.0 : (v > 255.0 ? 255.0 : v);
    case ELI_DATA_TYPE_S16: return v < -32768.0 ? -32768.0 : (v > 32767.0 ? 32767.0 : v);
    case ELI_DATA_TYPE_U16: return v < 0.0 ? 0.0 : (v > 65535.0 ? 65535.0 : v);
    case ELI_DATA_TYPE_S32: return v < -2147483648.0 ? -2147483648.0 : (v > 2147483647.0 ? 2147483647.0 : v);
    case ELI_DATA_TYPE_U32: return v < 0.0 ? 0.0 : (v > 4294967295.0 ? 4294967295.0 : v);
    case ELI_DATA_TYPE_U64: return v < 0.0 ? 0.0 : v;
    default: return v;
    }
}

/** Store a double (parsed/stepped) back into the scalar at p_data, with clamping. */
static inline void eli_input_number__store(eli_data_type type, void *p_data, double v)
{
    v = eli_input_number__clamp(type, v);
    switch (type) {
    case ELI_DATA_TYPE_S8:  *(int8_t *)p_data = (int8_t)v; break;
    case ELI_DATA_TYPE_U8:  *(uint8_t *)p_data = (uint8_t)v; break;
    case ELI_DATA_TYPE_S16: *(int16_t *)p_data = (int16_t)v; break;
    case ELI_DATA_TYPE_U16: *(uint16_t *)p_data = (uint16_t)v; break;
    case ELI_DATA_TYPE_S32: *(int32_t *)p_data = (int32_t)v; break;
    case ELI_DATA_TYPE_U32: *(uint32_t *)p_data = (uint32_t)v; break;
    case ELI_DATA_TYPE_S64: *(int64_t *)p_data = (int64_t)v; break;
    case ELI_DATA_TYPE_U64: *(uint64_t *)p_data = (uint64_t)v; break;
    case ELI_DATA_TYPE_FLOAT: *(float *)p_data = (float)v; break;
    case ELI_DATA_TYPE_DOUBLE: *(double *)p_data = v; break;
    default: break;
    }
}

/** Read the scalar at p_data as a double for arithmetic/comparison. */
static inline double eli_input_number__read(eli_data_type type, const void *p_data)
{
    switch (type) {
    case ELI_DATA_TYPE_S8:  return (double)*(const int8_t *)p_data;
    case ELI_DATA_TYPE_U8:  return (double)*(const uint8_t *)p_data;
    case ELI_DATA_TYPE_S16: return (double)*(const int16_t *)p_data;
    case ELI_DATA_TYPE_U16: return (double)*(const uint16_t *)p_data;
    case ELI_DATA_TYPE_S32: return (double)*(const int32_t *)p_data;
    case ELI_DATA_TYPE_U32: return (double)*(const uint32_t *)p_data;
    case ELI_DATA_TYPE_S64: return (double)*(const int64_t *)p_data;
    case ELI_DATA_TYPE_U64: return (double)*(const uint64_t *)p_data;
    case ELI_DATA_TYPE_FLOAT: return (double)*(const float *)p_data;
    case ELI_DATA_TYPE_DOUBLE: return *(const double *)p_data;
    default: return 0.0;
    }
}

/**
 * Parse text into p_data for the given scalar type.
 *
 * @return true if the buffer held a parseable number that was stored.
 */
static inline bool eli_input_number__parse(eli_data_type type, void *p_data, const char *buf)
{
    while (*buf == ' ')
        buf++;
    if (*buf == '\0')
        return false;
    char *end = NULL;
    if (type == ELI_DATA_TYPE_FLOAT || type == ELI_DATA_TYPE_DOUBLE) {
        double v = strtod(buf, &end);
        if (end == buf)
            return false;
        eli_input_number__store(type, p_data, v);
        return true;
    }
    double v = (double)strtoll(buf, &end, 0);
    if (end == buf)
        return false;
    eli_input_number__store(type, p_data, v);
    return true;
}

/* ---------------------------------------------------------------------------
 * Generic scalar input
 * ------------------------------------------------------------------------- */

/** Character-filter flags appropriate for a scalar data type's text field. */
static inline eli_input_text_flags eli_input_number__char_flags(eli_data_type type)
{
    if (type == ELI_DATA_TYPE_FLOAT || type == ELI_DATA_TYPE_DOUBLE)
        return ELI_INPUT_TEXT_CHARS_SCIENTIFIC;
    return ELI_INPUT_TEXT_CHARS_DECIMAL;
}

/**
 * A single-component scalar input with optional +/- step buttons.
 *
 * @param label        Widget label (identity + visible text).
 * @param data_type    Element type of p_data.
 * @param p_data       Pointer to the bound scalar (mutated on edit).
 * @param p_step       Step for a single +/- click, or NULL to hide the buttons.
 * @param p_step_fast  Step applied while Ctrl is held, or NULL.
 * @param format       printf format, or NULL for the type default.
 * @param flags        eli_input_text_flags for the text field.
 * @return             true when the bound value changed this frame.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_input_scalar(const char *label, eli_data_type data_type, void *p_data,
                                    const void *p_step, const void *p_step_fast, const char *format,
                                    eli_input_text_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items ||
        p_data == NULL)
        return false;
    const eli_style *style = &ctx->style;

    if (format == NULL)
        format = eli_input_number__default_format(data_type);

    eli_push_id(label);
    eli_id field_id = eli_get_id("##field");
    bool active = (eli_get_active_id() == field_id);
    if (!active)
        eli_input_number__format(g_eli_input_number_buf, ELI_INPUT_NUMBER_BUF_SIZE, data_type,
                                 p_data, format);

    bool has_step = (p_step != NULL);
    float button_sz = eli_get_frame_height();
    float full_w = eli_calc_item_width();
    if (has_step) {
        float field_w = full_w - 2.0f * (button_sz + style->item_inner_spacing.x);
        eli_set_next_item_width(field_w > 1.0f ? field_w : 1.0f);
    }

    eli_input_text_flags text_flags = flags | eli_input_number__char_flags(data_type) |
                                      ELI_INPUT_TEXT_AUTO_SELECT_ALL;
    bool text_edited = eli_input_text_impl("##field", NULL, g_eli_input_number_buf,
                                           ELI_INPUT_NUMBER_BUF_SIZE, eli_make_vec2(0.0f, 0.0f),
                                           text_flags, false, NULL, NULL);

    bool changed = false;
    double old_v = eli_input_number__read(data_type, p_data);
    if (text_edited) {
        if (eli_input_number__parse(data_type, p_data, g_eli_input_number_buf))
            changed = eli_input_number__read(data_type, p_data) != old_v;
    }

    if (has_step) {
        bool ctrl = eli_is_key_down(ELI_KEY_LEFT_CTRL) || eli_is_key_down(ELI_KEY_RIGHT_CTRL);
        const void *step = (ctrl && p_step_fast != NULL) ? p_step_fast : p_step;
        double step_v = eli_input_number__read(data_type, step);
        eli_same_line(0.0f, style->item_inner_spacing.x);
        if (eli_button_ex("-", eli_make_vec2(button_sz, button_sz), ELI_BUTTON_NONE)) {
            eli_input_number__store(data_type, p_data, old_v - step_v);
            eli_input_number__format(g_eli_input_number_buf, ELI_INPUT_NUMBER_BUF_SIZE, data_type,
                                     p_data, format);
            changed = true;
        }
        eli_same_line(0.0f, style->item_inner_spacing.x);
        if (eli_button_ex("+", eli_make_vec2(button_sz, button_sz), ELI_BUTTON_NONE)) {
            eli_input_number__store(data_type, p_data, old_v + step_v);
            eli_input_number__format(g_eli_input_number_buf, ELI_INPUT_NUMBER_BUF_SIZE, data_type,
                                     p_data, format);
            changed = true;
        }
    }

    const char *label_end = eli_find_rendered_text_end(label, NULL);
    if (label_end != label) {
        eli_same_line(0.0f, style->item_inner_spacing.x);
        eli_text_unformatted(label, label_end);
    }

    eli_pop_id();
    if (changed)
        eli_mark_item_edited(field_id);
    return changed;
}

/**
 * A row of `components` scalar inputs sharing one label.
 *
 * @param label        Widget label.
 * @param data_type    Element type of each component.
 * @param p_data       Pointer to the first component (contiguous array).
 * @param components   Number of components (1..4 typical).
 * @param p_step       Per-component step, or NULL.
 * @param p_step_fast  Fast step (Ctrl held), or NULL.
 * @param format       printf format, or NULL for the type default.
 * @param flags        eli_input_text_flags.
 * @return             true if any component changed this frame.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_input_scalar_n(const char *label, eli_data_type data_type, void *p_data,
                                      int components, const void *p_step, const void *p_step_fast,
                                      const char *format, eli_input_text_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || p_data == NULL || components < 1)
        return false;
    static const size_t type_sizes[ELI_DATA_TYPE_COUNT] = {
        sizeof(int8_t), sizeof(uint8_t), sizeof(int16_t), sizeof(uint16_t), sizeof(int32_t),
        sizeof(uint32_t), sizeof(int64_t), sizeof(uint64_t), sizeof(float), sizeof(double)
    };
    if (data_type < 0 || data_type >= ELI_DATA_TYPE_COUNT)
        return false;
    size_t elem = type_sizes[data_type];

    bool changed = false;
    eli_push_id(label);
    float full_w = eli_calc_item_width();
    float spacing = ctx->style.item_inner_spacing.x;
    float each_w = (full_w - spacing * (float)(components - 1)) / (float)components;
    for (int i = 0; i < components; i++) {
        eli_push_id_int(i);
        if (i > 0)
            eli_same_line(0.0f, spacing);
        eli_set_next_item_width(each_w > 1.0f ? each_w : 1.0f);
        void *comp = (char *)p_data + (size_t)i * elem;
        changed |= eli_input_scalar("", data_type, comp, p_step, p_step_fast, format, flags);
        eli_pop_id();
    }
    const char *label_end = eli_find_rendered_text_end(label, NULL);
    if (label_end != label) {
        eli_same_line(0.0f, ctx->style.item_inner_spacing.x);
        eli_text_unformatted(label, label_end);
    }
    eli_pop_id();
    return changed;
}

/* ---------------------------------------------------------------------------
 * Typed convenience wrappers
 * ------------------------------------------------------------------------- */

/** A float input with +/- step buttons (buttons hidden when step == 0). */
static inline bool eli_input_float(const char *label, float *v, float step, float step_fast,
                                   const char *format, eli_input_text_flags flags)
{
    return eli_input_scalar(label, ELI_DATA_TYPE_FLOAT, v, step != 0.0f ? &step : NULL,
                            step_fast != 0.0f ? &step_fast : NULL,
                            format ? format : "%.3f", flags);
}

/** A row of 2 float inputs. */
static inline bool eli_input_float2(const char *label, float v[2], const char *format,
                                    eli_input_text_flags flags)
{
    return eli_input_scalar_n(label, ELI_DATA_TYPE_FLOAT, v, 2, NULL, NULL,
                              format ? format : "%.3f", flags);
}

/** A row of 3 float inputs. */
static inline bool eli_input_float3(const char *label, float v[3], const char *format,
                                    eli_input_text_flags flags)
{
    return eli_input_scalar_n(label, ELI_DATA_TYPE_FLOAT, v, 3, NULL, NULL,
                              format ? format : "%.3f", flags);
}

/** A row of 4 float inputs. */
static inline bool eli_input_float4(const char *label, float v[4], const char *format,
                                    eli_input_text_flags flags)
{
    return eli_input_scalar_n(label, ELI_DATA_TYPE_FLOAT, v, 4, NULL, NULL,
                              format ? format : "%.3f", flags);
}

/** An int input with +/- step buttons (buttons hidden when step == 0). */
static inline bool eli_input_int(const char *label, int *v, int step, int step_fast,
                                 eli_input_text_flags flags)
{
    return eli_input_scalar(label, ELI_DATA_TYPE_S32, v, step != 0 ? &step : NULL,
                            step_fast != 0 ? &step_fast : NULL, "%d", flags);
}

/** A row of 2 int inputs. */
static inline bool eli_input_int2(const char *label, int v[2], eli_input_text_flags flags)
{
    return eli_input_scalar_n(label, ELI_DATA_TYPE_S32, v, 2, NULL, NULL, "%d", flags);
}

/** A row of 3 int inputs. */
static inline bool eli_input_int3(const char *label, int v[3], eli_input_text_flags flags)
{
    return eli_input_scalar_n(label, ELI_DATA_TYPE_S32, v, 3, NULL, NULL, "%d", flags);
}

/** A row of 4 int inputs. */
static inline bool eli_input_int4(const char *label, int v[4], eli_input_text_flags flags)
{
    return eli_input_scalar_n(label, ELI_DATA_TYPE_S32, v, 4, NULL, NULL, "%d", flags);
}

/** A double input with +/- step buttons (buttons hidden when step == 0). */
static inline bool eli_input_double(const char *label, double *v, double step, double step_fast,
                                    const char *format, eli_input_text_flags flags)
{
    return eli_input_scalar(label, ELI_DATA_TYPE_DOUBLE, v, step != 0.0 ? &step : NULL,
                            step_fast != 0.0 ? &step_fast : NULL,
                            format ? format : "%.6f", flags);
}

#endif /* ELI_WIDGETS_ELI_INPUT_NUMBER_H */
