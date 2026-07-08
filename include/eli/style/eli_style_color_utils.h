/**
 * @file eli_style_color_utils.h
 * @brief Color conversion and lookup helpers for the elimgui style system:
 *        packed<->float conversion, RGB<->HSV, style-color-to-u32 resolution
 *        (applying style.alpha), per-slot color lookup, and slot names.
 *
 * @status Phase 6 color utilities in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_STYLE_ELI_STYLE_COLOR_UTILS_H
#define ELI_STYLE_ELI_STYLE_COLOR_UTILS_H

#include "../core/eli_platform.h"
#include "../core/eli_types.h"
#include "../core/eli_enums.h"
#include "../core/eli_context.h"

/* ---------------------------------------------------------------------------
 * Seam math helpers
 *
 * Ported from Dear ImGui's ImFabs/ImFmod. Defined locally so the HSV math does
 * not depend on libm symbols that may be unavailable in the wasm32/JAClibc
 * build. Prefixed to avoid clashing with any shared math utility header.
 * ------------------------------------------------------------------------- */

/** Absolute value of a float without pulling in libm. */
static inline float eli_style_fabsf(float x)
{
    return x < 0.0f ? -x : x;
}

/** Floating-point remainder of x/y (truncated toward zero), like fmodf. */
static inline float eli_style_fmodf(float x, float y)
{
    if (y == 0.0f)
        return 0.0f;
    float q = x / y;
    long int t = (long int)q;
    return x - (float)t * y;
}

/* ---------------------------------------------------------------------------
 * Packed <-> float conversion (thin wrappers over the core helpers)
 * ------------------------------------------------------------------------- */

/**
 * Unpack a 32-bit color into a float RGBA vector (each component in 0..1).
 *
 * @param in  Packed color in R,G,B,A byte order.
 * @return    Float RGBA vector.
 *
 * Thread-safe: yes (pure function)
 * Reentrant: yes
 */
static inline eli_vec4 eli_color_convert_u32_to_float4(eli_col32 in)
{
    return eli_color_u32_to_vec4(in);
}

/**
 * Pack a float RGBA vector (each component in 0..1) into a 32-bit color.
 * Components are saturated to 0..1 before conversion.
 *
 * @param in  Float RGBA vector.
 * @return    Packed color in R,G,B,A byte order.
 *
 * Thread-safe: yes (pure function)
 * Reentrant: yes
 */
static inline eli_col32 eli_color_convert_float4_to_u32(eli_vec4 in)
{
    return eli_color_vec4_to_u32(in);
}

/* ---------------------------------------------------------------------------
 * RGB <-> HSV conversion (ported from Dear ImGui)
 * ------------------------------------------------------------------------- */

/**
 * Convert an RGB color to HSV. Ported from Dear ImGui's ColorConvertRGBtoHSV
 * (a branch-minimizing form of Wikipedia's algorithm). All components are in
 * the 0..1 range.
 *
 * @param r      Red in 0..1.
 * @param g      Green in 0..1.
 * @param b      Blue in 0..1.
 * @param out_h  Receives hue in 0..1 (must be non-NULL).
 * @param out_s  Receives saturation in 0..1 (must be non-NULL).
 * @param out_v  Receives value in 0..1 (must be non-NULL).
 *
 * Thread-safe: yes (pure function)
 * Reentrant: yes
 */
static inline void eli_color_convert_rgb_to_hsv(float r, float g, float b,
                                                float *out_h, float *out_s, float *out_v)
{
    float k = 0.0f;
    if (g < b) {
        float tmp = g; g = b; b = tmp;
        k = -1.0f;
    }
    if (r < g) {
        float tmp = r; r = g; g = tmp;
        k = -2.0f / 6.0f - k;
    }

    const float chroma = r - (g < b ? g : b);
    *out_h = eli_style_fabsf(k + (g - b) / (6.0f * chroma + 1e-20f));
    *out_s = chroma / (r + 1e-20f);
    *out_v = r;
}

/**
 * Convert an HSV color to RGB. Ported from Dear ImGui's ColorConvertHSVtoRGB.
 * All components are in the 0..1 range.
 *
 * @param h      Hue in 0..1.
 * @param s      Saturation in 0..1.
 * @param v      Value in 0..1.
 * @param out_r  Receives red in 0..1 (must be non-NULL).
 * @param out_g  Receives green in 0..1 (must be non-NULL).
 * @param out_b  Receives blue in 0..1 (must be non-NULL).
 *
 * Thread-safe: yes (pure function)
 * Reentrant: yes
 */
static inline void eli_color_convert_hsv_to_rgb(float h, float s, float v,
                                                float *out_r, float *out_g, float *out_b)
{
    if (s == 0.0f) {
        *out_r = *out_g = *out_b = v;
        return;
    }

    h = eli_style_fmodf(h, 1.0f) / (60.0f / 360.0f);
    int i = (int)h;
    float f = h - (float)i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    switch (i) {
    case 0:  *out_r = v; *out_g = t; *out_b = p; break;
    case 1:  *out_r = q; *out_g = v; *out_b = p; break;
    case 2:  *out_r = p; *out_g = v; *out_b = t; break;
    case 3:  *out_r = p; *out_g = q; *out_b = v; break;
    case 4:  *out_r = t; *out_g = p; *out_b = v; break;
    default: *out_r = v; *out_g = p; *out_b = q; break;
    }
}

/* ---------------------------------------------------------------------------
 * Style color resolution
 * ------------------------------------------------------------------------- */

/**
 * Resolve a themed color slot to a packed color, applying the global
 * style.alpha and an extra alpha multiplier (matches ImGui::GetColorU32).
 *
 * @param idx        Color slot (see eli_col). Must be a valid enumerator.
 * @param alpha_mul  Extra alpha multiplier applied on top of style.alpha.
 * @return           Packed color, or 0 if there is no current context.
 *
 * Thread-safe: no (reads the current context)
 * Reentrant: yes
 */
static inline eli_col32 eli_get_color_u32(eli_col idx, float alpha_mul)
{
    const eli_style *style = eli_get_style();
    if (style == NULL || idx < 0 || idx >= ELI_COL_COUNT)
        return 0u;

    eli_vec4 c = style->colors[idx];
    c.w *= style->alpha * alpha_mul;
    return eli_color_convert_float4_to_u32(c);
}

/**
 * Resolve a caller-supplied float color to a packed color, applying the global
 * style.alpha (matches ImGui::GetColorU32(const ImVec4&)).
 *
 * @param col  Float RGBA color (each component in 0..1).
 * @return     Packed color, or the direct conversion if no context is current.
 *
 * Thread-safe: no (reads the current context)
 * Reentrant: yes
 */
static inline eli_col32 eli_get_color_u32_vec4(eli_vec4 col)
{
    const eli_style *style = eli_get_style();
    eli_vec4 c = col;
    if (style != NULL)
        c.w *= style->alpha;
    return eli_color_convert_float4_to_u32(c);
}

/**
 * Scale the alpha of an already-packed color by style.alpha and an extra
 * multiplier (matches ImGui::GetColorU32(ImU32, float)).
 *
 * @param col        Packed color in R,G,B,A byte order.
 * @param alpha_mul  Extra alpha multiplier applied on top of style.alpha.
 * @return           Packed color with scaled alpha.
 *
 * Thread-safe: no (reads the current context)
 * Reentrant: yes
 */
static inline eli_col32 eli_get_color_u32_col32(eli_col32 col, float alpha_mul)
{
    const eli_style *style = eli_get_style();
    if (style != NULL)
        alpha_mul *= style->alpha;
    if (alpha_mul >= 1.0f)
        return col;

    uint32_t a = (col & ELI_COL32_A_MASK) >> ELI_COL32_A_SHIFT;
    a = (uint32_t)((float)a * alpha_mul);
    return (col & ~ELI_COL32_A_MASK) | (a << ELI_COL32_A_SHIFT);
}

/**
 * Read the raw float color stored in a themed slot (no alpha scaling).
 *
 * @param idx  Color slot (see eli_col).
 * @return     The slot's float RGBA color, or a zeroed vector if invalid/no
 *             context.
 *
 * Thread-safe: no (reads the current context)
 * Reentrant: yes
 */
static inline eli_vec4 eli_get_style_color_vec4(eli_col idx)
{
    const eli_style *style = eli_get_style();
    if (style == NULL || idx < 0 || idx >= ELI_COL_COUNT)
        return eli_make_vec4(0.0f, 0.0f, 0.0f, 0.0f);
    return style->colors[idx];
}

/**
 * Return a human-readable name for a color slot (matches ImGui's names, minus
 * the enum prefix), e.g. ELI_COL_TITLE_BG_ACTIVE -> "TitleBgActive".
 *
 * @param idx  Color slot (see eli_col).
 * @return     Static string; "Unknown" for out-of-range values.
 *
 * Thread-safe: yes (pure function)
 * Reentrant: yes
 */
static inline const char *eli_get_style_color_name(eli_col idx)
{
    switch (idx) {
    case ELI_COL_TEXT:                        return "Text";
    case ELI_COL_TEXT_DISABLED:               return "TextDisabled";
    case ELI_COL_WINDOW_BG:                   return "WindowBg";
    case ELI_COL_CHILD_BG:                    return "ChildBg";
    case ELI_COL_POPUP_BG:                    return "PopupBg";
    case ELI_COL_BORDER:                      return "Border";
    case ELI_COL_BORDER_SHADOW:               return "BorderShadow";
    case ELI_COL_FRAME_BG:                    return "FrameBg";
    case ELI_COL_FRAME_BG_HOVERED:            return "FrameBgHovered";
    case ELI_COL_FRAME_BG_ACTIVE:             return "FrameBgActive";
    case ELI_COL_TITLE_BG:                    return "TitleBg";
    case ELI_COL_TITLE_BG_ACTIVE:             return "TitleBgActive";
    case ELI_COL_TITLE_BG_COLLAPSED:          return "TitleBgCollapsed";
    case ELI_COL_MENU_BAR_BG:                 return "MenuBarBg";
    case ELI_COL_SCROLLBAR_BG:                return "ScrollbarBg";
    case ELI_COL_SCROLLBAR_GRAB:              return "ScrollbarGrab";
    case ELI_COL_SCROLLBAR_GRAB_HOVERED:      return "ScrollbarGrabHovered";
    case ELI_COL_SCROLLBAR_GRAB_ACTIVE:       return "ScrollbarGrabActive";
    case ELI_COL_CHECK_MARK:                  return "CheckMark";
    case ELI_COL_SLIDER_GRAB:                 return "SliderGrab";
    case ELI_COL_SLIDER_GRAB_ACTIVE:          return "SliderGrabActive";
    case ELI_COL_BUTTON:                      return "Button";
    case ELI_COL_BUTTON_HOVERED:              return "ButtonHovered";
    case ELI_COL_BUTTON_ACTIVE:               return "ButtonActive";
    case ELI_COL_HEADER:                      return "Header";
    case ELI_COL_HEADER_HOVERED:              return "HeaderHovered";
    case ELI_COL_HEADER_ACTIVE:               return "HeaderActive";
    case ELI_COL_SEPARATOR:                   return "Separator";
    case ELI_COL_SEPARATOR_HOVERED:           return "SeparatorHovered";
    case ELI_COL_SEPARATOR_ACTIVE:            return "SeparatorActive";
    case ELI_COL_RESIZE_GRIP:                 return "ResizeGrip";
    case ELI_COL_RESIZE_GRIP_HOVERED:         return "ResizeGripHovered";
    case ELI_COL_RESIZE_GRIP_ACTIVE:          return "ResizeGripActive";
    case ELI_COL_TAB_HOVERED:                 return "TabHovered";
    case ELI_COL_TAB:                         return "Tab";
    case ELI_COL_TAB_SELECTED:                return "TabSelected";
    case ELI_COL_TAB_SELECTED_OVERLINE:       return "TabSelectedOverline";
    case ELI_COL_TAB_DIMMED:                  return "TabDimmed";
    case ELI_COL_TAB_DIMMED_SELECTED:         return "TabDimmedSelected";
    case ELI_COL_TAB_DIMMED_SELECTED_OVERLINE: return "TabDimmedSelectedOverline";
    case ELI_COL_PLOT_LINES:                  return "PlotLines";
    case ELI_COL_PLOT_LINES_HOVERED:          return "PlotLinesHovered";
    case ELI_COL_PLOT_HISTOGRAM:              return "PlotHistogram";
    case ELI_COL_PLOT_HISTOGRAM_HOVERED:      return "PlotHistogramHovered";
    case ELI_COL_TABLE_HEADER_BG:             return "TableHeaderBg";
    case ELI_COL_TABLE_BORDER_STRONG:         return "TableBorderStrong";
    case ELI_COL_TABLE_BORDER_LIGHT:          return "TableBorderLight";
    case ELI_COL_TABLE_ROW_BG:                return "TableRowBg";
    case ELI_COL_TABLE_ROW_BG_ALT:            return "TableRowBgAlt";
    case ELI_COL_TEXT_LINK:                   return "TextLink";
    case ELI_COL_TEXT_SELECTED_BG:            return "TextSelectedBg";
    case ELI_COL_DRAG_DROP_TARGET:            return "DragDropTarget";
    case ELI_COL_NAV_CURSOR:                  return "NavCursor";
    case ELI_COL_NAV_WINDOWING_HIGHLIGHT:     return "NavWindowingHighlight";
    case ELI_COL_NAV_WINDOWING_DIM_BG:        return "NavWindowingDimBg";
    case ELI_COL_MODAL_WINDOW_DIM_BG:         return "ModalWindowDimBg";
    default:                                  return "Unknown";
    }
}

#endif /* ELI_STYLE_ELI_STYLE_COLOR_UTILS_H */
