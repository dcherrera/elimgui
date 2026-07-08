/**
 * @file eli_style_colors.h
 * @brief Built-in color themes for elimgui: dark, light, and classic. Each
 *        setter fills a style's colors[] table with the exact Dear ImGui RGBA
 *        values (including the ImLerp-derived tab colors).
 *
 * @status Phase 6 theming in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_STYLE_ELI_STYLE_COLORS_H
#define ELI_STYLE_ELI_STYLE_COLORS_H

#include "../core/eli_platform.h"
#include "../core/eli_types.h"
#include "../core/eli_enums.h"
#include "../core/eli_style_types.h"
#include "../core/eli_context.h"

/**
 * Component-wise linear interpolation between two colors: a + (b - a) * t.
 * Matches Dear ImGui's ImLerp for ImVec4, used to derive the tab colors.
 *
 * @param a  Start color.
 * @param b  End color.
 * @param t  Interpolation factor (0 = a, 1 = b).
 * @return   Interpolated color.
 */
static inline eli_vec4 eli_style_lerp_vec4(eli_vec4 a, eli_vec4 b, float t)
{
    return eli_make_vec4(a.x + (b.x - a.x) * t,
                         a.y + (b.y - a.y) * t,
                         a.z + (b.z - a.z) * t,
                         a.w + (b.w - a.w) * t);
}

/** Resolve the destination style: the argument, or the current context style. */
static inline eli_style *eli_style_colors_resolve_dst(eli_style *dst)
{
    return dst ? dst : eli_get_style();
}

/**
 * Apply the default dark theme to a style's color table.
 *
 * @param dst  Style to write into, or NULL to use the current context's style.
 *
 * Thread-safe: no (may read/write the current context)
 * Reentrant: yes
 */
static inline void eli_style_colors_dark(eli_style *dst)
{
    eli_style *style = eli_style_colors_resolve_dst(dst);
    if (style == NULL)
        return;
    eli_vec4 *c = style->colors;

    c[ELI_COL_TEXT]                   = eli_make_vec4(1.00f, 1.00f, 1.00f, 1.00f);
    c[ELI_COL_TEXT_DISABLED]          = eli_make_vec4(0.50f, 0.50f, 0.50f, 1.00f);
    c[ELI_COL_WINDOW_BG]              = eli_make_vec4(0.06f, 0.06f, 0.06f, 0.94f);
    c[ELI_COL_CHILD_BG]               = eli_make_vec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ELI_COL_POPUP_BG]               = eli_make_vec4(0.08f, 0.08f, 0.08f, 0.94f);
    c[ELI_COL_BORDER]                 = eli_make_vec4(0.43f, 0.43f, 0.50f, 0.50f);
    c[ELI_COL_BORDER_SHADOW]          = eli_make_vec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ELI_COL_FRAME_BG]               = eli_make_vec4(0.16f, 0.29f, 0.48f, 0.54f);
    c[ELI_COL_FRAME_BG_HOVERED]       = eli_make_vec4(0.26f, 0.59f, 0.98f, 0.40f);
    c[ELI_COL_FRAME_BG_ACTIVE]        = eli_make_vec4(0.26f, 0.59f, 0.98f, 0.67f);
    c[ELI_COL_TITLE_BG]               = eli_make_vec4(0.04f, 0.04f, 0.04f, 1.00f);
    c[ELI_COL_TITLE_BG_ACTIVE]        = eli_make_vec4(0.16f, 0.29f, 0.48f, 1.00f);
    c[ELI_COL_TITLE_BG_COLLAPSED]     = eli_make_vec4(0.00f, 0.00f, 0.00f, 0.51f);
    c[ELI_COL_MENU_BAR_BG]            = eli_make_vec4(0.14f, 0.14f, 0.14f, 1.00f);
    c[ELI_COL_SCROLLBAR_BG]           = eli_make_vec4(0.02f, 0.02f, 0.02f, 0.53f);
    c[ELI_COL_SCROLLBAR_GRAB]         = eli_make_vec4(0.31f, 0.31f, 0.31f, 1.00f);
    c[ELI_COL_SCROLLBAR_GRAB_HOVERED] = eli_make_vec4(0.41f, 0.41f, 0.41f, 1.00f);
    c[ELI_COL_SCROLLBAR_GRAB_ACTIVE]  = eli_make_vec4(0.51f, 0.51f, 0.51f, 1.00f);
    c[ELI_COL_CHECK_MARK]             = eli_make_vec4(0.26f, 0.59f, 0.98f, 1.00f);
    c[ELI_COL_SLIDER_GRAB]            = eli_make_vec4(0.24f, 0.52f, 0.88f, 1.00f);
    c[ELI_COL_SLIDER_GRAB_ACTIVE]     = eli_make_vec4(0.26f, 0.59f, 0.98f, 1.00f);
    c[ELI_COL_BUTTON]                 = eli_make_vec4(0.26f, 0.59f, 0.98f, 0.40f);
    c[ELI_COL_BUTTON_HOVERED]         = eli_make_vec4(0.26f, 0.59f, 0.98f, 1.00f);
    c[ELI_COL_BUTTON_ACTIVE]          = eli_make_vec4(0.06f, 0.53f, 0.98f, 1.00f);
    c[ELI_COL_HEADER]                 = eli_make_vec4(0.26f, 0.59f, 0.98f, 0.31f);
    c[ELI_COL_HEADER_HOVERED]         = eli_make_vec4(0.26f, 0.59f, 0.98f, 0.80f);
    c[ELI_COL_HEADER_ACTIVE]          = eli_make_vec4(0.26f, 0.59f, 0.98f, 1.00f);
    c[ELI_COL_SEPARATOR]              = c[ELI_COL_BORDER];
    c[ELI_COL_SEPARATOR_HOVERED]      = eli_make_vec4(0.10f, 0.40f, 0.75f, 0.78f);
    c[ELI_COL_SEPARATOR_ACTIVE]       = eli_make_vec4(0.10f, 0.40f, 0.75f, 1.00f);
    c[ELI_COL_RESIZE_GRIP]            = eli_make_vec4(0.26f, 0.59f, 0.98f, 0.20f);
    c[ELI_COL_RESIZE_GRIP_HOVERED]    = eli_make_vec4(0.26f, 0.59f, 0.98f, 0.67f);
    c[ELI_COL_RESIZE_GRIP_ACTIVE]     = eli_make_vec4(0.26f, 0.59f, 0.98f, 0.95f);
    c[ELI_COL_TAB_HOVERED]            = c[ELI_COL_HEADER_HOVERED];
    c[ELI_COL_TAB]                    = eli_style_lerp_vec4(c[ELI_COL_HEADER], c[ELI_COL_TITLE_BG_ACTIVE], 0.80f);
    c[ELI_COL_TAB_SELECTED]           = eli_style_lerp_vec4(c[ELI_COL_HEADER_ACTIVE], c[ELI_COL_TITLE_BG_ACTIVE], 0.60f);
    c[ELI_COL_TAB_SELECTED_OVERLINE]  = c[ELI_COL_HEADER_ACTIVE];
    c[ELI_COL_TAB_DIMMED]             = eli_style_lerp_vec4(c[ELI_COL_TAB], c[ELI_COL_TITLE_BG], 0.80f);
    c[ELI_COL_TAB_DIMMED_SELECTED]    = eli_style_lerp_vec4(c[ELI_COL_TAB_SELECTED], c[ELI_COL_TITLE_BG], 0.40f);
    c[ELI_COL_TAB_DIMMED_SELECTED_OVERLINE] = eli_make_vec4(0.50f, 0.50f, 0.50f, 0.00f);
    c[ELI_COL_PLOT_LINES]             = eli_make_vec4(0.61f, 0.61f, 0.61f, 1.00f);
    c[ELI_COL_PLOT_LINES_HOVERED]     = eli_make_vec4(1.00f, 0.43f, 0.35f, 1.00f);
    c[ELI_COL_PLOT_HISTOGRAM]         = eli_make_vec4(0.90f, 0.70f, 0.00f, 1.00f);
    c[ELI_COL_PLOT_HISTOGRAM_HOVERED] = eli_make_vec4(1.00f, 0.60f, 0.00f, 1.00f);
    c[ELI_COL_TABLE_HEADER_BG]        = eli_make_vec4(0.19f, 0.19f, 0.20f, 1.00f);
    c[ELI_COL_TABLE_BORDER_STRONG]    = eli_make_vec4(0.31f, 0.31f, 0.35f, 1.00f);
    c[ELI_COL_TABLE_BORDER_LIGHT]     = eli_make_vec4(0.23f, 0.23f, 0.25f, 1.00f);
    c[ELI_COL_TABLE_ROW_BG]           = eli_make_vec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ELI_COL_TABLE_ROW_BG_ALT]       = eli_make_vec4(1.00f, 1.00f, 1.00f, 0.06f);
    c[ELI_COL_TEXT_LINK]              = c[ELI_COL_HEADER_ACTIVE];
    c[ELI_COL_TEXT_SELECTED_BG]       = eli_make_vec4(0.26f, 0.59f, 0.98f, 0.35f);
    c[ELI_COL_DRAG_DROP_TARGET]       = eli_make_vec4(1.00f, 1.00f, 0.00f, 0.90f);
    c[ELI_COL_NAV_CURSOR]             = eli_make_vec4(0.26f, 0.59f, 0.98f, 1.00f);
    c[ELI_COL_NAV_WINDOWING_HIGHLIGHT] = eli_make_vec4(1.00f, 1.00f, 1.00f, 0.70f);
    c[ELI_COL_NAV_WINDOWING_DIM_BG]   = eli_make_vec4(0.80f, 0.80f, 0.80f, 0.20f);
    c[ELI_COL_MODAL_WINDOW_DIM_BG]    = eli_make_vec4(0.80f, 0.80f, 0.80f, 0.35f);
}

/**
 * Apply the light theme to a style's color table.
 *
 * @param dst  Style to write into, or NULL to use the current context's style.
 *
 * Thread-safe: no (may read/write the current context)
 * Reentrant: yes
 */
static inline void eli_style_colors_light(eli_style *dst)
{
    eli_style *style = eli_style_colors_resolve_dst(dst);
    if (style == NULL)
        return;
    eli_vec4 *c = style->colors;

    c[ELI_COL_TEXT]                   = eli_make_vec4(0.00f, 0.00f, 0.00f, 1.00f);
    c[ELI_COL_TEXT_DISABLED]          = eli_make_vec4(0.60f, 0.60f, 0.60f, 1.00f);
    c[ELI_COL_WINDOW_BG]              = eli_make_vec4(0.94f, 0.94f, 0.94f, 1.00f);
    c[ELI_COL_CHILD_BG]               = eli_make_vec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ELI_COL_POPUP_BG]               = eli_make_vec4(1.00f, 1.00f, 1.00f, 0.98f);
    c[ELI_COL_BORDER]                 = eli_make_vec4(0.00f, 0.00f, 0.00f, 0.30f);
    c[ELI_COL_BORDER_SHADOW]          = eli_make_vec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ELI_COL_FRAME_BG]               = eli_make_vec4(1.00f, 1.00f, 1.00f, 1.00f);
    c[ELI_COL_FRAME_BG_HOVERED]       = eli_make_vec4(0.26f, 0.59f, 0.98f, 0.40f);
    c[ELI_COL_FRAME_BG_ACTIVE]        = eli_make_vec4(0.26f, 0.59f, 0.98f, 0.67f);
    c[ELI_COL_TITLE_BG]               = eli_make_vec4(0.96f, 0.96f, 0.96f, 1.00f);
    c[ELI_COL_TITLE_BG_ACTIVE]        = eli_make_vec4(0.82f, 0.82f, 0.82f, 1.00f);
    c[ELI_COL_TITLE_BG_COLLAPSED]     = eli_make_vec4(1.00f, 1.00f, 1.00f, 0.51f);
    c[ELI_COL_MENU_BAR_BG]            = eli_make_vec4(0.86f, 0.86f, 0.86f, 1.00f);
    c[ELI_COL_SCROLLBAR_BG]           = eli_make_vec4(0.98f, 0.98f, 0.98f, 0.53f);
    c[ELI_COL_SCROLLBAR_GRAB]         = eli_make_vec4(0.69f, 0.69f, 0.69f, 0.80f);
    c[ELI_COL_SCROLLBAR_GRAB_HOVERED] = eli_make_vec4(0.49f, 0.49f, 0.49f, 0.80f);
    c[ELI_COL_SCROLLBAR_GRAB_ACTIVE]  = eli_make_vec4(0.49f, 0.49f, 0.49f, 1.00f);
    c[ELI_COL_CHECK_MARK]             = eli_make_vec4(0.26f, 0.59f, 0.98f, 1.00f);
    c[ELI_COL_SLIDER_GRAB]            = eli_make_vec4(0.26f, 0.59f, 0.98f, 0.78f);
    c[ELI_COL_SLIDER_GRAB_ACTIVE]     = eli_make_vec4(0.46f, 0.54f, 0.80f, 0.60f);
    c[ELI_COL_BUTTON]                 = eli_make_vec4(0.26f, 0.59f, 0.98f, 0.40f);
    c[ELI_COL_BUTTON_HOVERED]         = eli_make_vec4(0.26f, 0.59f, 0.98f, 1.00f);
    c[ELI_COL_BUTTON_ACTIVE]          = eli_make_vec4(0.06f, 0.53f, 0.98f, 1.00f);
    c[ELI_COL_HEADER]                 = eli_make_vec4(0.26f, 0.59f, 0.98f, 0.31f);
    c[ELI_COL_HEADER_HOVERED]         = eli_make_vec4(0.26f, 0.59f, 0.98f, 0.80f);
    c[ELI_COL_HEADER_ACTIVE]          = eli_make_vec4(0.26f, 0.59f, 0.98f, 1.00f);
    c[ELI_COL_SEPARATOR]              = eli_make_vec4(0.39f, 0.39f, 0.39f, 0.62f);
    c[ELI_COL_SEPARATOR_HOVERED]      = eli_make_vec4(0.14f, 0.44f, 0.80f, 0.78f);
    c[ELI_COL_SEPARATOR_ACTIVE]       = eli_make_vec4(0.14f, 0.44f, 0.80f, 1.00f);
    c[ELI_COL_RESIZE_GRIP]            = eli_make_vec4(0.35f, 0.35f, 0.35f, 0.17f);
    c[ELI_COL_RESIZE_GRIP_HOVERED]    = eli_make_vec4(0.26f, 0.59f, 0.98f, 0.67f);
    c[ELI_COL_RESIZE_GRIP_ACTIVE]     = eli_make_vec4(0.26f, 0.59f, 0.98f, 0.95f);
    c[ELI_COL_TAB_HOVERED]            = c[ELI_COL_HEADER_HOVERED];
    c[ELI_COL_TAB]                    = eli_style_lerp_vec4(c[ELI_COL_HEADER], c[ELI_COL_TITLE_BG_ACTIVE], 0.90f);
    c[ELI_COL_TAB_SELECTED]           = eli_style_lerp_vec4(c[ELI_COL_HEADER_ACTIVE], c[ELI_COL_TITLE_BG_ACTIVE], 0.60f);
    c[ELI_COL_TAB_SELECTED_OVERLINE]  = c[ELI_COL_HEADER_ACTIVE];
    c[ELI_COL_TAB_DIMMED]             = eli_style_lerp_vec4(c[ELI_COL_TAB], c[ELI_COL_TITLE_BG], 0.80f);
    c[ELI_COL_TAB_DIMMED_SELECTED]    = eli_style_lerp_vec4(c[ELI_COL_TAB_SELECTED], c[ELI_COL_TITLE_BG], 0.40f);
    c[ELI_COL_TAB_DIMMED_SELECTED_OVERLINE] = eli_make_vec4(0.26f, 0.59f, 1.00f, 0.00f);
    c[ELI_COL_PLOT_LINES]             = eli_make_vec4(0.39f, 0.39f, 0.39f, 1.00f);
    c[ELI_COL_PLOT_LINES_HOVERED]     = eli_make_vec4(1.00f, 0.43f, 0.35f, 1.00f);
    c[ELI_COL_PLOT_HISTOGRAM]         = eli_make_vec4(0.90f, 0.70f, 0.00f, 1.00f);
    c[ELI_COL_PLOT_HISTOGRAM_HOVERED] = eli_make_vec4(1.00f, 0.45f, 0.00f, 1.00f);
    c[ELI_COL_TABLE_HEADER_BG]        = eli_make_vec4(0.78f, 0.87f, 0.98f, 1.00f);
    c[ELI_COL_TABLE_BORDER_STRONG]    = eli_make_vec4(0.57f, 0.57f, 0.64f, 1.00f);
    c[ELI_COL_TABLE_BORDER_LIGHT]     = eli_make_vec4(0.68f, 0.68f, 0.74f, 1.00f);
    c[ELI_COL_TABLE_ROW_BG]           = eli_make_vec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ELI_COL_TABLE_ROW_BG_ALT]       = eli_make_vec4(0.30f, 0.30f, 0.30f, 0.09f);
    c[ELI_COL_TEXT_LINK]              = c[ELI_COL_HEADER_ACTIVE];
    c[ELI_COL_TEXT_SELECTED_BG]       = eli_make_vec4(0.26f, 0.59f, 0.98f, 0.35f);
    c[ELI_COL_DRAG_DROP_TARGET]       = eli_make_vec4(0.26f, 0.59f, 0.98f, 0.95f);
    c[ELI_COL_NAV_CURSOR]             = c[ELI_COL_HEADER_HOVERED];
    c[ELI_COL_NAV_WINDOWING_HIGHLIGHT] = eli_make_vec4(0.70f, 0.70f, 0.70f, 0.70f);
    c[ELI_COL_NAV_WINDOWING_DIM_BG]   = eli_make_vec4(0.20f, 0.20f, 0.20f, 0.20f);
    c[ELI_COL_MODAL_WINDOW_DIM_BG]    = eli_make_vec4(0.20f, 0.20f, 0.20f, 0.35f);
}

/**
 * Apply the classic theme to a style's color table.
 *
 * @param dst  Style to write into, or NULL to use the current context's style.
 *
 * Thread-safe: no (may read/write the current context)
 * Reentrant: yes
 */
static inline void eli_style_colors_classic(eli_style *dst)
{
    eli_style *style = eli_style_colors_resolve_dst(dst);
    if (style == NULL)
        return;
    eli_vec4 *c = style->colors;

    c[ELI_COL_TEXT]                   = eli_make_vec4(0.90f, 0.90f, 0.90f, 1.00f);
    c[ELI_COL_TEXT_DISABLED]          = eli_make_vec4(0.60f, 0.60f, 0.60f, 1.00f);
    c[ELI_COL_WINDOW_BG]              = eli_make_vec4(0.00f, 0.00f, 0.00f, 0.85f);
    c[ELI_COL_CHILD_BG]               = eli_make_vec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ELI_COL_POPUP_BG]               = eli_make_vec4(0.11f, 0.11f, 0.14f, 0.92f);
    c[ELI_COL_BORDER]                 = eli_make_vec4(0.50f, 0.50f, 0.50f, 0.50f);
    c[ELI_COL_BORDER_SHADOW]          = eli_make_vec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ELI_COL_FRAME_BG]               = eli_make_vec4(0.43f, 0.43f, 0.43f, 0.39f);
    c[ELI_COL_FRAME_BG_HOVERED]       = eli_make_vec4(0.47f, 0.47f, 0.69f, 0.40f);
    c[ELI_COL_FRAME_BG_ACTIVE]        = eli_make_vec4(0.42f, 0.41f, 0.64f, 0.69f);
    c[ELI_COL_TITLE_BG]               = eli_make_vec4(0.27f, 0.27f, 0.54f, 0.83f);
    c[ELI_COL_TITLE_BG_ACTIVE]        = eli_make_vec4(0.32f, 0.32f, 0.63f, 0.87f);
    c[ELI_COL_TITLE_BG_COLLAPSED]     = eli_make_vec4(0.40f, 0.40f, 0.80f, 0.20f);
    c[ELI_COL_MENU_BAR_BG]            = eli_make_vec4(0.40f, 0.40f, 0.55f, 0.80f);
    c[ELI_COL_SCROLLBAR_BG]           = eli_make_vec4(0.20f, 0.25f, 0.30f, 0.60f);
    c[ELI_COL_SCROLLBAR_GRAB]         = eli_make_vec4(0.40f, 0.40f, 0.80f, 0.30f);
    c[ELI_COL_SCROLLBAR_GRAB_HOVERED] = eli_make_vec4(0.40f, 0.40f, 0.80f, 0.40f);
    c[ELI_COL_SCROLLBAR_GRAB_ACTIVE]  = eli_make_vec4(0.41f, 0.39f, 0.80f, 0.60f);
    c[ELI_COL_CHECK_MARK]             = eli_make_vec4(0.90f, 0.90f, 0.90f, 0.50f);
    c[ELI_COL_SLIDER_GRAB]            = eli_make_vec4(1.00f, 1.00f, 1.00f, 0.30f);
    c[ELI_COL_SLIDER_GRAB_ACTIVE]     = eli_make_vec4(0.41f, 0.39f, 0.80f, 0.60f);
    c[ELI_COL_BUTTON]                 = eli_make_vec4(0.35f, 0.40f, 0.61f, 0.62f);
    c[ELI_COL_BUTTON_HOVERED]         = eli_make_vec4(0.40f, 0.48f, 0.71f, 0.79f);
    c[ELI_COL_BUTTON_ACTIVE]          = eli_make_vec4(0.46f, 0.54f, 0.80f, 1.00f);
    c[ELI_COL_HEADER]                 = eli_make_vec4(0.40f, 0.40f, 0.90f, 0.45f);
    c[ELI_COL_HEADER_HOVERED]         = eli_make_vec4(0.45f, 0.45f, 0.90f, 0.80f);
    c[ELI_COL_HEADER_ACTIVE]          = eli_make_vec4(0.53f, 0.53f, 0.87f, 0.80f);
    c[ELI_COL_SEPARATOR]              = eli_make_vec4(0.50f, 0.50f, 0.50f, 0.60f);
    c[ELI_COL_SEPARATOR_HOVERED]      = eli_make_vec4(0.60f, 0.60f, 0.70f, 1.00f);
    c[ELI_COL_SEPARATOR_ACTIVE]       = eli_make_vec4(0.70f, 0.70f, 0.90f, 1.00f);
    c[ELI_COL_RESIZE_GRIP]            = eli_make_vec4(1.00f, 1.00f, 1.00f, 0.10f);
    c[ELI_COL_RESIZE_GRIP_HOVERED]    = eli_make_vec4(0.78f, 0.82f, 1.00f, 0.60f);
    c[ELI_COL_RESIZE_GRIP_ACTIVE]     = eli_make_vec4(0.78f, 0.82f, 1.00f, 0.90f);
    c[ELI_COL_TAB_HOVERED]            = c[ELI_COL_HEADER_HOVERED];
    c[ELI_COL_TAB]                    = eli_style_lerp_vec4(c[ELI_COL_HEADER], c[ELI_COL_TITLE_BG_ACTIVE], 0.80f);
    c[ELI_COL_TAB_SELECTED]           = eli_style_lerp_vec4(c[ELI_COL_HEADER_ACTIVE], c[ELI_COL_TITLE_BG_ACTIVE], 0.60f);
    c[ELI_COL_TAB_SELECTED_OVERLINE]  = c[ELI_COL_HEADER_ACTIVE];
    c[ELI_COL_TAB_DIMMED]             = eli_style_lerp_vec4(c[ELI_COL_TAB], c[ELI_COL_TITLE_BG], 0.80f);
    c[ELI_COL_TAB_DIMMED_SELECTED]    = eli_style_lerp_vec4(c[ELI_COL_TAB_SELECTED], c[ELI_COL_TITLE_BG], 0.40f);
    c[ELI_COL_TAB_DIMMED_SELECTED_OVERLINE] = eli_make_vec4(0.53f, 0.53f, 0.87f, 0.00f);
    c[ELI_COL_PLOT_LINES]             = eli_make_vec4(1.00f, 1.00f, 1.00f, 1.00f);
    c[ELI_COL_PLOT_LINES_HOVERED]     = eli_make_vec4(0.90f, 0.70f, 0.00f, 1.00f);
    c[ELI_COL_PLOT_HISTOGRAM]         = eli_make_vec4(0.90f, 0.70f, 0.00f, 1.00f);
    c[ELI_COL_PLOT_HISTOGRAM_HOVERED] = eli_make_vec4(1.00f, 0.60f, 0.00f, 1.00f);
    c[ELI_COL_TABLE_HEADER_BG]        = eli_make_vec4(0.27f, 0.27f, 0.38f, 1.00f);
    c[ELI_COL_TABLE_BORDER_STRONG]    = eli_make_vec4(0.31f, 0.31f, 0.45f, 1.00f);
    c[ELI_COL_TABLE_BORDER_LIGHT]     = eli_make_vec4(0.26f, 0.26f, 0.28f, 1.00f);
    c[ELI_COL_TABLE_ROW_BG]           = eli_make_vec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ELI_COL_TABLE_ROW_BG_ALT]       = eli_make_vec4(1.00f, 1.00f, 1.00f, 0.07f);
    c[ELI_COL_TEXT_LINK]              = c[ELI_COL_HEADER_ACTIVE];
    c[ELI_COL_TEXT_SELECTED_BG]       = eli_make_vec4(0.00f, 0.00f, 1.00f, 0.35f);
    c[ELI_COL_DRAG_DROP_TARGET]       = eli_make_vec4(1.00f, 1.00f, 0.00f, 0.90f);
    c[ELI_COL_NAV_CURSOR]             = c[ELI_COL_HEADER_HOVERED];
    c[ELI_COL_NAV_WINDOWING_HIGHLIGHT] = eli_make_vec4(1.00f, 1.00f, 1.00f, 0.70f);
    c[ELI_COL_NAV_WINDOWING_DIM_BG]   = eli_make_vec4(0.80f, 0.80f, 0.80f, 0.20f);
    c[ELI_COL_MODAL_WINDOW_DIM_BG]    = eli_make_vec4(0.20f, 0.20f, 0.20f, 0.35f);
}

#endif /* ELI_STYLE_ELI_STYLE_COLORS_H */
