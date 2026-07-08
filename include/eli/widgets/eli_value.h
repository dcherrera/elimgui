/**
 * @file eli_value.h
 * @brief Phase 24 value-display widgets: labeled value output for bool, int, unsigned
 *        int, and float scalars. Each formats "prefix: value" as a plain text item
 *        via eli_text_unformatted, advancing the layout cursor exactly as eli_text
 *        would. Mirrors Dear ImGui's Value() convenience overloads.
 *
 * Format contract:
 *   eli_value_bool  → "prefix: true"  or  "prefix: false"
 *   eli_value_int   → "prefix: %d"
 *   eli_value_uint  → "prefix: %u"
 *   eli_value_float → "prefix: <float_format>" (NULL → "%.3f")
 *
 * All output is bounded to ELI_VALUE_BUFFER_SIZE bytes. No heap allocation occurs.
 *
 * @status Phase 24 value-display widgets implemented.
 * @issues None
 * @todo None
 */
#ifndef ELI_WIDGETS_ELI_VALUE_H
#define ELI_WIDGETS_ELI_VALUE_H

#include "eli_text_widgets.h"

#include "../core/eli_platform.h"

/** Maximum bytes for the formatted "prefix: value" output string. */
#define ELI_VALUE_BUFFER_SIZE 256

/* ---------------------------------------------------------------------------
 * Value display widgets
 * ------------------------------------------------------------------------- */

/**
 * Display a bool value as "prefix: true" or "prefix: false".
 *
 * @param prefix  Label string shown before the colon separator.
 * @param b       Value to display.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_value_bool(const char *prefix, bool b)
{
    char buf[ELI_VALUE_BUFFER_SIZE];
    snprintf(buf, sizeof(buf), "%s: %s", prefix, b ? "true" : "false");
    eli_text_unformatted(buf, NULL);
}

/**
 * Display a signed integer value as "prefix: <v>".
 *
 * @param prefix  Label string shown before the colon separator.
 * @param v       Value to display.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_value_int(const char *prefix, int v)
{
    char buf[ELI_VALUE_BUFFER_SIZE];
    snprintf(buf, sizeof(buf), "%s: %d", prefix, v);
    eli_text_unformatted(buf, NULL);
}

/**
 * Display an unsigned integer value as "prefix: <v>".
 *
 * @param prefix  Label string shown before the colon separator.
 * @param v       Value to display.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_value_uint(const char *prefix, unsigned int v)
{
    char buf[ELI_VALUE_BUFFER_SIZE];
    snprintf(buf, sizeof(buf), "%s: %u", prefix, v);
    eli_text_unformatted(buf, NULL);
}

/**
 * Display a float value as "prefix: <v>" using an optional custom format specifier.
 *
 * When float_format is NULL the default "%.3f" is used, matching Dear ImGui's
 * Value() default. When provided, float_format must be a printf format specifier
 * for a single float/double argument (e.g., "%.1f", "%e", "%g").
 *
 * @param prefix        Label string shown before the colon separator.
 * @param v             Value to display.
 * @param float_format  printf format specifier for the float, or NULL for "%.3f".
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_value_float(const char *prefix, float v, const char *float_format)
{
    char buf[ELI_VALUE_BUFFER_SIZE];
    if (float_format != NULL) {
        /* Two-step: format the float value first, then combine with the prefix.
         * float_format is caller-controlled and follows ImGui's Value() contract:
         * a single %f-family specifier applied to v. */
        char val_str[64];
        int vn = snprintf(val_str, sizeof(val_str), float_format, v);
        if (vn < 0)
            val_str[0] = '\0';
        else if ((size_t)vn >= sizeof(val_str))
            val_str[sizeof(val_str) - 1] = '\0';
        snprintf(buf, sizeof(buf), "%s: %s", prefix, val_str);
    } else {
        snprintf(buf, sizeof(buf), "%s: %.3f", prefix, v);
    }
    eli_text_unformatted(buf, NULL);
}

#endif /* ELI_WIDGETS_ELI_VALUE_H */
