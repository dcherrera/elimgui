/**
 * @file eli_color.h
 * @brief Phase 13 color widgets: eli_color_button (preview swatch), eli_color_edit3 /
 *        _edit4 (drag/input rows + small preview that opens a picker popup),
 *        eli_color_picker3 / _picker4 (interactive saturation-value square, hue bar,
 *        optional alpha bar and side preview), and eli_set_color_edit_options. Colors
 *        are stored RGBA in 0..1; display honors eli_color_edit_flags (RGB/HSV/HEX,
 *        UINT8/FLOAT, no-alpha, no-inputs, no-picker, alpha bar, ...). Mirrors Dear
 *        ImGui's ColorEdit/ColorPicker/ColorButton contracts.
 *
 * @status Phase 13 color widgets in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WIDGETS_ELI_COLOR_H
#define ELI_WIDGETS_ELI_COLOR_H

#include "eli_drag.h"
#include "eli_button_widgets.h"
#include "eli_input_text_widget.h"
#include "eli_popup.h"
#include "eli_widget_behavior.h"
#include "eli_item_status.h"

#include "../core/eli_platform.h"
#include "../layout/eli_layout.h"
#include "../style/eli_style_color_utils.h"
#include "../window/eli_window.h"

#include <stdio.h>

/* ---------------------------------------------------------------------------
 * Color edit flags
 *
 * The public flag surface is defined here (guarded) so the header is usable via a
 * direct include without a matching enum in core. Mirrors Dear ImGui's
 * ImGuiColorEditFlags. Individual bits may already be defined by an earlier phase;
 * the #ifndef guard keeps this a no-op in that case.
 * ------------------------------------------------------------------------- */

#ifndef ELI_COLOR_EDIT_NONE
typedef int eli_color_edit_flags;
enum eli_color_edit_flags_ {
    ELI_COLOR_EDIT_NONE               = 0,
    ELI_COLOR_EDIT_NO_ALPHA           = 1 << 1,
    ELI_COLOR_EDIT_NO_PICKER          = 1 << 2,
    ELI_COLOR_EDIT_NO_OPTIONS         = 1 << 3,
    ELI_COLOR_EDIT_NO_SMALL_PREVIEW   = 1 << 4,
    ELI_COLOR_EDIT_NO_INPUTS          = 1 << 5,
    ELI_COLOR_EDIT_NO_TOOLTIP         = 1 << 6,
    ELI_COLOR_EDIT_NO_LABEL           = 1 << 7,
    ELI_COLOR_EDIT_NO_SIDE_PREVIEW    = 1 << 8,
    ELI_COLOR_EDIT_NO_DRAG_DROP       = 1 << 9,
    ELI_COLOR_EDIT_NO_BORDER          = 1 << 10,
    ELI_COLOR_EDIT_ALPHA_BAR          = 1 << 16,
    ELI_COLOR_EDIT_ALPHA_PREVIEW      = 1 << 17,
    ELI_COLOR_EDIT_ALPHA_PREVIEW_HALF = 1 << 18,
    ELI_COLOR_EDIT_HDR                = 1 << 19,
    ELI_COLOR_EDIT_DISPLAY_RGB        = 1 << 20,
    ELI_COLOR_EDIT_DISPLAY_HSV        = 1 << 21,
    ELI_COLOR_EDIT_DISPLAY_HEX        = 1 << 22,
    ELI_COLOR_EDIT_UINT8              = 1 << 23,
    ELI_COLOR_EDIT_FLOAT              = 1 << 24,
    ELI_COLOR_EDIT_PICKER_HUE_BAR     = 1 << 25,
    ELI_COLOR_EDIT_PICKER_HUE_WHEEL   = 1 << 26,
    ELI_COLOR_EDIT_INPUT_RGB          = 1 << 27,
    ELI_COLOR_EDIT_INPUT_HSV          = 1 << 28
};
#endif /* ELI_COLOR_EDIT_NONE */

/* Grouped option masks (only one bit of each group is meaningful at a time). */
#define ELI_COLOR_EDIT_DISPLAY_MASK                                                     \
    (ELI_COLOR_EDIT_DISPLAY_RGB | ELI_COLOR_EDIT_DISPLAY_HSV | ELI_COLOR_EDIT_DISPLAY_HEX)
#define ELI_COLOR_EDIT_DATATYPE_MASK (ELI_COLOR_EDIT_UINT8 | ELI_COLOR_EDIT_FLOAT)
#define ELI_COLOR_EDIT_PICKER_MASK                                                      \
    (ELI_COLOR_EDIT_PICKER_HUE_BAR | ELI_COLOR_EDIT_PICKER_HUE_WHEEL)
#define ELI_COLOR_EDIT_INPUT_MASK (ELI_COLOR_EDIT_INPUT_RGB | ELI_COLOR_EDIT_INPUT_HSV)

/* ---------------------------------------------------------------------------
 * Module-private option + hue-preservation state
 * ------------------------------------------------------------------------- */

/** User-configurable default option bits, merged into every color widget's flags. */
static eli_color_edit_flags g_eli_color_edit_options =
    ELI_COLOR_EDIT_UINT8 | ELI_COLOR_EDIT_DISPLAY_RGB | ELI_COLOR_EDIT_INPUT_RGB |
    ELI_COLOR_EDIT_PICKER_HUE_BAR;

/* One-slot cache so a picker keeps its hue while dragging through a gray column
 * (where RGB->HSV cannot recover the hue). Keyed by the active picker's id. */
static eli_id g_eli_color_picker_hue_id = 0u;
static float g_eli_color_picker_hue = 0.0f;

/**
 * Override the default color-edit option bits (display mode, data type, picker
 * type, input color space) applied when a color widget leaves them unspecified.
 * Mirrors Dear ImGui's SetColorEditOptions.
 *
 * @param flags  Option bits to store as the new defaults.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_set_color_edit_options(eli_color_edit_flags flags)
{
    if ((flags & ELI_COLOR_EDIT_DISPLAY_MASK) == 0)
        flags |= (g_eli_color_edit_options & ELI_COLOR_EDIT_DISPLAY_MASK);
    if ((flags & ELI_COLOR_EDIT_DATATYPE_MASK) == 0)
        flags |= (g_eli_color_edit_options & ELI_COLOR_EDIT_DATATYPE_MASK);
    if ((flags & ELI_COLOR_EDIT_PICKER_MASK) == 0)
        flags |= (g_eli_color_edit_options & ELI_COLOR_EDIT_PICKER_MASK);
    if ((flags & ELI_COLOR_EDIT_INPUT_MASK) == 0)
        flags |= (g_eli_color_edit_options & ELI_COLOR_EDIT_INPUT_MASK);
    g_eli_color_edit_options = flags;
}

/** Fill in any unset option group of `flags` from the stored defaults. */
static inline eli_color_edit_flags eli_color__apply_options(eli_color_edit_flags flags)
{
    if ((flags & ELI_COLOR_EDIT_DISPLAY_MASK) == 0)
        flags |= (g_eli_color_edit_options & ELI_COLOR_EDIT_DISPLAY_MASK);
    if ((flags & ELI_COLOR_EDIT_DATATYPE_MASK) == 0)
        flags |= (g_eli_color_edit_options & ELI_COLOR_EDIT_DATATYPE_MASK);
    if ((flags & ELI_COLOR_EDIT_PICKER_MASK) == 0)
        flags |= (g_eli_color_edit_options & ELI_COLOR_EDIT_PICKER_MASK);
    if ((flags & ELI_COLOR_EDIT_INPUT_MASK) == 0)
        flags |= (g_eli_color_edit_options & ELI_COLOR_EDIT_INPUT_MASK);
    return flags;
}

/* ---------------------------------------------------------------------------
 * Small numeric helpers
 * ------------------------------------------------------------------------- */

/** Convert a 0..1 float to an 8-bit component (rounded, clamped). */
static inline int eli_color__f2i8(float x)
{
    x = eli_clamp_f(x, 0.0f, 1.0f);
    return (int)(x * 255.0f + 0.5f);
}

/** Pack an RGBA float color to a 32-bit color with full alpha override option. */
static inline eli_col32 eli_color__vec4_to_u32(float r, float g, float b, float a)
{
    return ELI_COL32(eli_color__f2i8(r), eli_color__f2i8(g), eli_color__f2i8(b),
                     eli_color__f2i8(a));
}

/* ---------------------------------------------------------------------------
 * Checkerboard + swatch rendering (shared by color button and alpha previews)
 * ------------------------------------------------------------------------- */

/** Draw an alpha checkerboard behind a rect (two-tone grid), for transparency. */
static inline void eli_color__render_checkerboard(eli_draw_list *dl, eli_vec2 p_min,
                                                  eli_vec2 p_max, float grid_step)
{
    eli_col32 col_a = ELI_COL32(204, 204, 204, 255);
    eli_col32 col_b = ELI_COL32(128, 128, 128, 255);
    eli_draw_list_add_rect_filled(dl, p_min, p_max, col_a, 0.0f, ELI_DRAW_ROUND_CORNERS_NONE);
    int row = 0;
    for (float y = p_min.y; y < p_max.y; y += grid_step, row++) {
        float y1 = eli_min_f(y + grid_step, p_max.y);
        int col = row & 1;
        for (float x = p_min.x; x < p_max.x; x += grid_step, col++) {
            if ((col & 1) == 0)
                continue;
            float x1 = eli_min_f(x + grid_step, p_max.x);
            eli_draw_list_add_rect_filled(dl, eli_make_vec2(x, y), eli_make_vec2(x1, y1), col_b,
                                          0.0f, ELI_DRAW_ROUND_CORNERS_NONE);
        }
    }
}

/**
 * Draw a color swatch: an alpha checkerboard (only when the color is translucent)
 * overlaid with the color itself, with optional rounding. Mirrors Dear ImGui's
 * RenderColorRectWithAlphaCheckerboard (simplified: full overlay, no split).
 */
static inline void eli_color__render_swatch(eli_draw_list *dl, eli_vec2 p_min, eli_vec2 p_max,
                                            eli_vec4 col, float rounding)
{
    if (col.w < 1.0f) {
        float grid_step = eli_min_f((p_max.x - p_min.x), (p_max.y - p_min.y)) * 0.5f;
        grid_step = eli_max_f(2.0f, grid_step);
        eli_color__render_checkerboard(dl, p_min, p_max, grid_step);
    }
    eli_col32 c = eli_color__vec4_to_u32(col.x, col.y, col.z, col.w);
    eli_draw_list_add_rect_filled(dl, p_min, p_max, c, rounding, ELI_DRAW_ROUND_CORNERS_ALL);
}

/* ---------------------------------------------------------------------------
 * Color button
 * ------------------------------------------------------------------------- */

/**
 * A clickable color swatch. Draws the color (with an alpha checkerboard when
 * translucent) and an optional border; returns true on click. Mirrors ColorButton.
 *
 * @param desc_id  Identity string (also the swatch's id).
 * @param col      Displayed RGBA color (0..1).
 * @param flags    eli_color_edit_flags (NO_ALPHA, NO_BORDER honored).
 * @param size     Swatch size (0 components fall back to one frame height).
 * @return         true on the frame the swatch is pressed.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_color_button(const char *desc_id, eli_vec4 col, eli_color_edit_flags flags,
                                    eli_vec2 size)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return false;
    const eli_style *style = &ctx->style;

    eli_id id = eli_get_id(desc_id);
    float default_sz = eli_get_frame_height();
    eli_vec2 sz = eli_calc_item_size(size, default_sz, default_sz);
    eli_vec2 pos = ctx->current_window->cursor_pos;
    eli_rect bb = eli_make_rect(pos.x, pos.y, sz.x, sz.y);
    eli_item_size(sz, (sz.y >= default_sz) ? style->frame_padding.y : -1.0f);
    if (!eli_item_add(id, bb, 0))
        return false;

    bool hovered = false, held = false;
    bool pressed = eli_button_behavior(bb, id, &hovered, &held, 0);

    if (flags & ELI_COLOR_EDIT_NO_ALPHA)
        col.w = 1.0f;

    eli_draw_list *dl = eli_get_window_draw_list();
    float rounding = style->frame_rounding;
    eli_render_nav_highlight(bb, id);
    eli_color__render_swatch(dl, eli_rect_min(bb), eli_rect_max(bb), col, rounding);
    if (!(flags & ELI_COLOR_EDIT_NO_BORDER)) {
        eli_col32 border = hovered ? eli_get_color_u32(ELI_COL_BORDER, 1.0f)
                                   : eli_get_color_u32(ELI_COL_BORDER, 0.6f);
        eli_draw_list_add_rect(dl, eli_rect_min(bb), eli_rect_max(bb), border, rounding,
                               ELI_DRAW_ROUND_CORNERS_ALL, 1.0f);
    }
    return pressed;
}

/* ---------------------------------------------------------------------------
 * Color edit — hex input row
 * ------------------------------------------------------------------------- */

/** Persistent hex-edit buffer (single active field at a time). */
static char g_eli_color_hex_buf[16] = "#000000";

/** Parse a hex character to its 0..15 value, or -1 if not a hex digit. */
static inline int eli_color__hex_digit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

/** Parse up to 8 hex digits from `buf` into col[] (RGB or RGBA). @return true if applied. */
static inline bool eli_color__parse_hex(const char *buf, float col[4], int components)
{
    int digits[8];
    int n = 0;
    for (const char *p = buf; *p != '\0' && n < 8; p++) {
        int d = eli_color__hex_digit(*p);
        if (d >= 0)
            digits[n++] = d;
    }
    if (n < 6)
        return false;
    int v[4];
    v[0] = digits[0] * 16 + digits[1];
    v[1] = digits[2] * 16 + digits[3];
    v[2] = digits[4] * 16 + digits[5];
    v[3] = (n >= 8) ? digits[6] * 16 + digits[7] : 255;
    col[0] = (float)v[0] / 255.0f;
    col[1] = (float)v[1] / 255.0f;
    col[2] = (float)v[2] / 255.0f;
    if (components >= 4)
        col[3] = (float)v[3] / 255.0f;
    return true;
}

/** Render the "#RRGGBB[AA]" hex input field. @return true when the color changed. */
static inline bool eli_color__edit_hex(float col[4], int components, float w_inputs)
{
    eli_id fid = eli_get_id("##hex");
    if (eli_get_active_id() != fid) {
        int r = eli_color__f2i8(col[0]), g = eli_color__f2i8(col[1]), b = eli_color__f2i8(col[2]);
        if (components >= 4)
            snprintf(g_eli_color_hex_buf, sizeof(g_eli_color_hex_buf), "#%02X%02X%02X%02X", r, g, b,
                     eli_color__f2i8(col[3]));
        else
            snprintf(g_eli_color_hex_buf, sizeof(g_eli_color_hex_buf), "#%02X%02X%02X", r, g, b);
    }
    eli_set_next_item_width(w_inputs);
    eli_input_text_flags tf = ELI_INPUT_TEXT_CHARS_HEXADECIMAL | ELI_INPUT_TEXT_CHARS_UPPERCASE |
                              ELI_INPUT_TEXT_ENTER_RETURNS_TRUE;
    if (eli_input_text("##hex", g_eli_color_hex_buf, sizeof(g_eli_color_hex_buf), tf, NULL, NULL))
        return eli_color__parse_hex(g_eli_color_hex_buf, col, components);
    return false;
}

/* ---------------------------------------------------------------------------
 * Color edit — numeric drag rows
 * ------------------------------------------------------------------------- */

/**
 * Render the numeric component rows for a color edit and write any change back into
 * col[] (stored RGB). Honors DISPLAY_RGB / DISPLAY_HSV / DISPLAY_HEX and the
 * UINT8 / FLOAT data type. @return true when the color changed.
 */
static inline bool eli_color__edit_inputs(float col[4], int components, eli_color_edit_flags flags,
                                          float w_inputs)
{
    const eli_style *style = &eli_get_current_context()->style;
    if (flags & ELI_COLOR_EDIT_DISPLAY_HEX)
        return eli_color__edit_hex(col, components, w_inputs);

    bool hsv = (flags & ELI_COLOR_EDIT_DISPLAY_HSV) != 0;
    bool uint8 = (flags & ELI_COLOR_EDIT_UINT8) != 0;

    float f[4] = { col[0], col[1], col[2], components >= 4 ? col[3] : 1.0f };
    if (hsv)
        eli_color_convert_rgb_to_hsv(col[0], col[1], col[2], &f[0], &f[1], &f[2]);

    static const char *ids[4] = { "##X", "##Y", "##Z", "##W" };
    float spacing = style->item_inner_spacing.x;
    float w_one = eli_max_f(1.0f, (float)(int)((w_inputs - spacing * (float)(components - 1)) /
                                               (float)components));
    float w_last = eli_max_f(1.0f,
                             (float)(int)(w_inputs - (w_one + spacing) * (float)(components - 1)));
    bool changed = false;
    for (int i = 0; i < components; i++) {
        if (i > 0)
            eli_same_line(0.0f, spacing);
        eli_set_next_item_width(i == components - 1 ? w_last : w_one);
        if (uint8) {
            int iv = eli_color__f2i8(f[i]);
            if (eli_drag_int(ids[i], &iv, 1.0f, 0, 255, "%d", 0))
                changed = true;
            f[i] = (float)iv / 255.0f;
        } else {
            if (eli_drag_float(ids[i], &f[i], 0.004f, 0.0f, 1.0f, "%.3f", 0))
                changed = true;
        }
    }

    if (changed) {
        if (hsv)
            eli_color_convert_hsv_to_rgb(f[0], f[1], f[2], &col[0], &col[1], &col[2]);
        else {
            col[0] = f[0];
            col[1] = f[1];
            col[2] = f[2];
        }
        if (components >= 4)
            col[3] = f[3];
    }
    return changed;
}

/* ---------------------------------------------------------------------------
 * Picker geometry + rendering
 * ------------------------------------------------------------------------- */

/** Resolved layout rectangle math for a color picker. */
typedef struct eli_color_picker_geom {
    eli_vec2 pos;        /* top-left of the SV square (== picker origin) */
    float sv_size;       /* side of the saturation/value square */
    float bars_width;    /* width of the hue / alpha bars */
    float bar0_x;        /* left x of the hue bar */
    float bar1_x;        /* left x of the alpha bar (valid when alpha_bar) */
    float preview_x;     /* left x of the side preview swatch (valid when side_preview) */
    float square_sz;     /* one frame height (preview swatch side) */
    float total_w;       /* full width spanned by the picker (for layout advance) */
    bool alpha_bar;
    bool side_preview;
} eli_color_picker_geom;

/** Compute picker geometry from the current item width and style. */
static inline eli_color_picker_geom eli_color__picker_geom(eli_color_edit_flags flags, bool alpha)
{
    eli_context *ctx = eli_get_current_context();
    const eli_style *style = &ctx->style;
    eli_color_picker_geom g = {0};
    g.square_sz = eli_get_frame_height();
    g.bars_width = g.square_sz;
    g.alpha_bar = alpha && (flags & ELI_COLOR_EDIT_ALPHA_BAR) != 0;
    g.side_preview = (flags & ELI_COLOR_EDIT_NO_SIDE_PREVIEW) == 0;

    float spacing = style->item_inner_spacing.x;
    float width = eli_calc_item_width();
    int bars_count = g.alpha_bar ? 2 : 1;
    float reserved = (float)bars_count * (g.bars_width + spacing);
    if (g.side_preview)
        reserved += g.square_sz + spacing;
    g.sv_size = eli_max_f(g.bars_width, width - reserved);

    g.pos = ctx->current_window->cursor_pos;
    g.bar0_x = g.pos.x + g.sv_size + spacing;
    g.bar1_x = g.bar0_x + g.bars_width + spacing;
    float bars_right = g.bar0_x + (float)bars_count * g.bars_width + (float)(bars_count - 1) * spacing;
    g.preview_x = bars_right + spacing;
    g.total_w = (g.side_preview ? (g.preview_x + g.square_sz) : bars_right) - g.pos.x;
    return g;
}

/** Draw the saturation/value square gradients for hue `h` and its selection cursor. */
static inline void eli_color__render_sv(eli_draw_list *dl, const eli_color_picker_geom *g, float h,
                                        float s, float v)
{
    float hr, hg, hb;
    eli_color_convert_hsv_to_rgb(h, 1.0f, 1.0f, &hr, &hg, &hb);
    eli_col32 hue32 = eli_color__vec4_to_u32(hr, hg, hb, 1.0f);
    eli_vec2 mn = g->pos;
    eli_vec2 mx = eli_make_vec2(g->pos.x + g->sv_size, g->pos.y + g->sv_size);
    eli_draw_list_add_rect_filled_multi_color(dl, mn, mx, ELI_COL32_WHITE, hue32, hue32,
                                              ELI_COL32_WHITE);
    eli_draw_list_add_rect_filled_multi_color(dl, mn, mx, ELI_COL32_BLACK_TRANS,
                                              ELI_COL32_BLACK_TRANS, ELI_COL32_BLACK,
                                              ELI_COL32_BLACK);
    eli_vec2 cur = eli_make_vec2(mn.x + s * g->sv_size, mn.y + (1.0f - v) * g->sv_size);
    eli_draw_list_add_circle(dl, cur, 5.0f, ELI_COL32_WHITE, 12, 2.0f);
    eli_draw_list_add_circle(dl, cur, 6.0f, ELI_COL32_BLACK, 12, 1.0f);
}

/** Draw the six-segment hue bar and its selection marker at hue `h`. */
static inline void eli_color__render_hue_bar(eli_draw_list *dl, const eli_color_picker_geom *g,
                                             float h)
{
    static const eli_col32 hues[7] = {
        ELI_COL32(255, 0, 0, 255),   ELI_COL32(255, 255, 0, 255), ELI_COL32(0, 255, 0, 255),
        ELI_COL32(0, 255, 255, 255), ELI_COL32(0, 0, 255, 255),   ELI_COL32(255, 0, 255, 255),
        ELI_COL32(255, 0, 0, 255)
    };
    float seg = g->sv_size / 6.0f;
    for (int i = 0; i < 6; i++) {
        eli_vec2 mn = eli_make_vec2(g->bar0_x, g->pos.y + (float)i * seg);
        eli_vec2 mx = eli_make_vec2(g->bar0_x + g->bars_width, g->pos.y + (float)(i + 1) * seg);
        eli_draw_list_add_rect_filled_multi_color(dl, mn, mx, hues[i], hues[i], hues[i + 1],
                                                  hues[i + 1]);
    }
    float y = g->pos.y + h * g->sv_size;
    eli_draw_list_add_rect(dl, eli_make_vec2(g->bar0_x - 1.0f, y - 2.0f),
                           eli_make_vec2(g->bar0_x + g->bars_width + 1.0f, y + 2.0f),
                           ELI_COL32_WHITE, 0.0f, ELI_DRAW_ROUND_CORNERS_NONE, 1.5f);
}

/** Draw the alpha bar (checkerboard + gradient) and its selection marker at alpha `a`. */
static inline void eli_color__render_alpha_bar(eli_draw_list *dl, const eli_color_picker_geom *g,
                                               eli_vec4 rgb, float a)
{
    eli_vec2 mn = eli_make_vec2(g->bar1_x, g->pos.y);
    eli_vec2 mx = eli_make_vec2(g->bar1_x + g->bars_width, g->pos.y + g->sv_size);
    eli_color__render_checkerboard(dl, mn, mx, eli_max_f(2.0f, g->bars_width * 0.5f));
    eli_col32 top = eli_color__vec4_to_u32(rgb.x, rgb.y, rgb.z, 1.0f);
    eli_col32 bot = eli_color__vec4_to_u32(rgb.x, rgb.y, rgb.z, 0.0f);
    eli_draw_list_add_rect_filled_multi_color(dl, mn, mx, top, top, bot, bot);
    float y = g->pos.y + (1.0f - a) * g->sv_size;
    eli_draw_list_add_rect(dl, eli_make_vec2(g->bar1_x - 1.0f, y - 2.0f),
                           eli_make_vec2(g->bar1_x + g->bars_width + 1.0f, y + 2.0f),
                           ELI_COL32_WHITE, 0.0f, ELI_DRAW_ROUND_CORNERS_NONE, 1.5f);
}

/* ---------------------------------------------------------------------------
 * Picker interaction
 * ------------------------------------------------------------------------- */

/**
 * Drive the SV square, hue bar and alpha bar interaction for one frame. Updates the
 * in/out H, S, V, A on drag and sets *changed. Uses press-on-click so a click acts
 * immediately (S/V/H/A snap toward the click point).
 */
static inline void eli_color__picker_interact(const eli_color_picker_geom *g, eli_id base_id,
                                              float *h, float *s, float *v, float *a, bool *changed)
{
    eli_vec2 m = eli_get_mouse_pos();
    eli_rect sv_bb = eli_make_rect(g->pos.x, g->pos.y, g->sv_size, g->sv_size);
    bool hov = false, held = false;
    eli_button_behavior(sv_bb, base_id + 1u, &hov, &held, ELI_BUTTON_PRESSED_ON_CLICK);
    if (held) {
        *s = eli_clamp_f((m.x - g->pos.x) / g->sv_size, 0.0f, 1.0f);
        *v = 1.0f - eli_clamp_f((m.y - g->pos.y) / g->sv_size, 0.0f, 1.0f);
        *changed = true;
    }

    eli_rect hue_bb = eli_make_rect(g->bar0_x, g->pos.y, g->bars_width, g->sv_size);
    eli_button_behavior(hue_bb, base_id + 2u, &hov, &held, ELI_BUTTON_PRESSED_ON_CLICK);
    if (held) {
        *h = eli_clamp_f((m.y - g->pos.y) / g->sv_size, 0.0f, 1.0f);
        *changed = true;
    }

    if (g->alpha_bar) {
        eli_rect a_bb = eli_make_rect(g->bar1_x, g->pos.y, g->bars_width, g->sv_size);
        eli_button_behavior(a_bb, base_id + 3u, &hov, &held, ELI_BUTTON_PRESSED_ON_CLICK);
        if (held) {
            *a = 1.0f - eli_clamp_f((m.y - g->pos.y) / g->sv_size, 0.0f, 1.0f);
            *changed = true;
        }
    }
}

/* ---------------------------------------------------------------------------
 * Color picker
 * ------------------------------------------------------------------------- */

/**
 * The full color picker: an interactive saturation/value square, a hue bar, an
 * optional alpha bar, an optional side preview swatch, and (unless NO_INPUTS)
 * numeric edit rows. Mirrors Dear ImGui's ColorPicker4.
 *
 * @param label    Identity + label (visible part stops at "##").
 * @param col      In/out RGBA color (0..1); alpha ignored when NO_ALPHA.
 * @param flags    eli_color_edit_flags.
 * @param ref_col  Optional reference color to show for comparison (may be NULL).
 * @return         true when the color changed this frame.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_color_picker4(const char *label, float col[4], eli_color_edit_flags flags,
                                     const float *ref_col)
{
    (void)ref_col;
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items ||
        col == NULL)
        return false;

    flags = eli_color__apply_options(flags);
    bool alpha = !(flags & ELI_COLOR_EDIT_NO_ALPHA);
    int components = alpha ? 4 : 3;

    eli_id id = eli_get_id(label);
    eli_begin_group();
    eli_push_id(label);

    eli_color_picker_geom g = eli_color__picker_geom(flags, alpha);

    float h, s, v;
    eli_color_convert_rgb_to_hsv(col[0], col[1], col[2], &h, &s, &v);
    if (g_eli_color_picker_hue_id == id && s == 0.0f)
        h = g_eli_color_picker_hue;
    float a = alpha ? col[3] : 1.0f;

    bool changed = false;
    eli_color__picker_interact(&g, id, &h, &s, &v, &a, &changed);
    if (changed) {
        eli_color_convert_hsv_to_rgb(h, s, v, &col[0], &col[1], &col[2]);
        if (alpha)
            col[3] = a;
    }
    g_eli_color_picker_hue_id = id;
    g_eli_color_picker_hue = h;

    eli_draw_list *dl = eli_get_window_draw_list();
    eli_color__render_sv(dl, &g, h, s, v);
    eli_color__render_hue_bar(dl, &g, h);
    if (g.alpha_bar)
        eli_color__render_alpha_bar(dl, &g, eli_make_vec4(col[0], col[1], col[2], 1.0f), a);
    if (g.side_preview) {
        eli_vec2 pmn = eli_make_vec2(g.preview_x, g.pos.y);
        eli_vec2 pmx = eli_make_vec2(g.preview_x + g.square_sz, g.pos.y + g.square_sz);
        eli_color__render_swatch(dl, pmn, pmx, eli_make_vec4(col[0], col[1], col[2], a),
                                 ctx->style.frame_rounding);
    }

    eli_dummy(eli_make_vec2(g.total_w, g.sv_size));
    if (!(flags & ELI_COLOR_EDIT_NO_INPUTS))
        changed |= eli_color__edit_inputs(col, components, flags, g.sv_size);

    eli_pop_id();
    eli_end_group();
    if (changed)
        eli_mark_item_edited(id);
    return changed;
}

/** RGB color picker (no alpha). See eli_color_picker4. */
static inline bool eli_color_picker3(const char *label, float col[3], eli_color_edit_flags flags)
{
    float col4[4] = { col[0], col[1], col[2], 1.0f };
    bool changed = eli_color_picker4(label, col4, flags | ELI_COLOR_EDIT_NO_ALPHA, NULL);
    if (changed) {
        col[0] = col4[0];
        col[1] = col4[1];
        col[2] = col4[2];
    }
    return changed;
}

/* ---------------------------------------------------------------------------
 * Color edit
 * ------------------------------------------------------------------------- */

/**
 * A color editor row: numeric drag/input components, a small preview swatch that
 * opens a picker popup, and a trailing label. Mirrors Dear ImGui's ColorEdit4.
 *
 * @param label  Identity + label (visible part stops at "##").
 * @param col    In/out RGBA color (0..1); alpha ignored when NO_ALPHA.
 * @param flags  eli_color_edit_flags.
 * @return       true when the color changed this frame.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_color_edit4(const char *label, float col[4], eli_color_edit_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items ||
        col == NULL)
        return false;
    const eli_style *style = &ctx->style;

    flags = eli_color__apply_options(flags);
    bool alpha = !(flags & ELI_COLOR_EDIT_NO_ALPHA);
    int components = alpha ? 4 : 3;

    eli_id id = eli_get_id(label);
    const char *label_end = eli_find_rendered_text_end(label, NULL);
    float square_sz = eli_get_frame_height();
    float spacing = style->item_inner_spacing.x;
    bool show_preview = !(flags & ELI_COLOR_EDIT_NO_SMALL_PREVIEW);
    float w_inputs = eli_calc_item_width();
    if (show_preview)
        w_inputs -= (square_sz + spacing);

    eli_begin_group();
    eli_push_id(label);

    bool changed = false;
    if (!(flags & ELI_COLOR_EDIT_NO_INPUTS))
        changed |= eli_color__edit_inputs(col, components, flags, w_inputs);

    if (show_preview) {
        if (!(flags & ELI_COLOR_EDIT_NO_INPUTS))
            eli_same_line(0.0f, spacing);
        eli_vec4 cv = eli_make_vec4(col[0], col[1], col[2], alpha ? col[3] : 1.0f);
        if (eli_color_button("##swatch", cv, flags, eli_make_vec2(square_sz, square_sz)) &&
            !(flags & ELI_COLOR_EDIT_NO_PICKER))
            eli_open_popup("picker", 0);
    }

    if (label != label_end && !(flags & ELI_COLOR_EDIT_NO_LABEL)) {
        eli_same_line(0.0f, spacing);
        eli_text_unformatted(label, label_end);
    }

    if (!(flags & ELI_COLOR_EDIT_NO_PICKER) && eli_begin_popup("picker", 0)) {
        eli_color_edit_flags pf = flags & (ELI_COLOR_EDIT_NO_ALPHA | ELI_COLOR_EDIT_ALPHA_BAR |
                                           ELI_COLOR_EDIT_PICKER_MASK | ELI_COLOR_EDIT_INPUT_MASK);
        changed |= eli_color_picker4("picker", col, pf, NULL);
        eli_end_popup();
    }

    eli_pop_id();
    eli_end_group();
    if (changed)
        eli_mark_item_edited(id);
    return changed;
}

/** RGB color editor (no alpha). See eli_color_edit4. */
static inline bool eli_color_edit3(const char *label, float col[3], eli_color_edit_flags flags)
{
    float col4[4] = { col[0], col[1], col[2], 1.0f };
    bool changed = eli_color_edit4(label, col4, flags | ELI_COLOR_EDIT_NO_ALPHA);
    if (changed) {
        col[0] = col4[0];
        col[1] = col4[1];
        col[2] = col4[2];
    }
    return changed;
}

#endif /* ELI_WIDGETS_ELI_COLOR_H */
