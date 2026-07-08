/**
 * @file eli_style_types.h
 * @brief The eli_style structure: every sizing, spacing, rounding, alignment,
 *        and behavior value plus the per-slot color table that drives elimgui's
 *        appearance. Structure definition only; the style push/pop and theming
 *        API lives in the style phase.
 *
 * @status Phase 1 structure definition in use. Default values are populated by
 *         eli_create_context; theming (colors[]) is filled by the style phase.
 * @issues None
 * @todo None
 */
#ifndef ELI_CORE_ELI_STYLE_TYPES_H
#define ELI_CORE_ELI_STYLE_TYPES_H

#include "eli_types.h"
#include "eli_enums.h"

/**
 * The full appearance configuration for an elimgui context.
 *
 * Sizing/spacing/rounding fields carry the Dear ImGui default values (set by
 * eli_create_context). The colors[] table is indexed by eli_col and holds one
 * float RGBA entry per themeable slot; it is zero-initialized until a theme is
 * applied by the style phase.
 */
typedef struct eli_style {
    /* Main */
    float alpha;
    float disabled_alpha;
    eli_vec2 window_padding;
    float window_rounding;
    float window_border_size;
    eli_vec2 window_min_size;
    eli_vec2 window_title_align;
    eli_dir window_menu_button_position;
    float child_rounding;
    float child_border_size;
    float popup_rounding;
    float popup_border_size;
    eli_vec2 frame_padding;
    float frame_rounding;
    float frame_border_size;
    eli_vec2 item_spacing;
    eli_vec2 item_inner_spacing;
    eli_vec2 cell_padding;
    eli_vec2 touch_extra_padding;
    float indent_spacing;
    float columns_min_spacing;
    float scrollbar_size;
    float scrollbar_rounding;
    float grab_min_size;
    float grab_rounding;
    float log_slider_deadzone;
    float tab_rounding;
    float tab_border_size;
    float tab_min_width_for_close_button;
    float tab_bar_border_size;
    float tab_bar_overline_size;
    float table_angled_headers_angle;
    eli_vec2 table_angled_headers_text_align;
    eli_dir color_button_position;
    eli_vec2 button_text_align;
    eli_vec2 selectable_text_align;
    float separator_text_border_size;
    eli_vec2 separator_text_align;
    eli_vec2 separator_text_padding;
    eli_vec2 display_window_padding;
    eli_vec2 display_safe_area_padding;
    float docking_separator_size;
    float mouse_cursor_scale;
    bool anti_aliased_lines;
    bool anti_aliased_lines_use_tex;
    bool anti_aliased_fill;
    float curve_tessellation_tol;
    float circle_tessellation_max_error;

    /* Per-slot theme colors (indexed by eli_col; RGBA in 0..1). */
    eli_vec4 colors[ELI_COL_COUNT];

    /* Hover delay behavior */
    float hover_stationary_delay;
    float hover_delay_short;
    float hover_delay_normal;
    eli_hovered_flags hover_flags_for_tooltip_mouse;
    eli_hovered_flags hover_flags_for_tooltip_nav;
} eli_style;

#endif /* ELI_CORE_ELI_STYLE_TYPES_H */
