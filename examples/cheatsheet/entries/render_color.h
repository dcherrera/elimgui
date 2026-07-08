/**
 * @file render_color.h
 * @brief Live render functions for the Color category cheatsheet entries.
 *
 * Each function renders exactly one widget and owns its function-static
 * retained state, matching elimgui's immediate-mode model. This header is
 * designed to be included by the cheatsheet app alongside rows_color.h.
 *
 * @status Cheatsheet content (Color category). Not part of the elimgui library API.
 * @issues None
 * @todo None
 */
#ifndef CHEAT_RENDER_COLOR_H
#define CHEAT_RENDER_COLOR_H

#include <eli/elimgui.h>

/* --- Color Edit (RGB, no alpha) --- */

static void cheat_render_color_edit3(void)
{
    static float col[3] = {0.26f, 0.59f, 0.98f};
    eli_color_edit3("Background", col, ELI_COLOR_EDIT_NONE);
}

/* --- Color Edit (RGBA) --- */

static void cheat_render_color_edit4_rgba(void)
{
    static float col[4] = {0.26f, 0.59f, 0.98f, 0.80f};
    eli_color_edit4("Tint", col, ELI_COLOR_EDIT_NONE);
}

/* --- Color Picker (inline SV square + hue bar) --- */

static void cheat_render_color_picker4(void)
{
    static float col[4] = {0.80f, 0.20f, 0.30f, 1.0f};
    eli_color_picker4("##picker_demo", col, ELI_COLOR_EDIT_NONE, NULL);
}

/* --- Color Button (clickable swatch, toggles red channel on click) --- */

static void cheat_render_color_button(void)
{
    static bool toggled = false;
    eli_vec4 col = {toggled ? 0.85f : 0.4f, 0.7f, 0.2f, 1.0f};
    eli_vec2 sz  = {40.0f, 40.0f};
    if (eli_color_button("##swatch_demo", col, ELI_COLOR_EDIT_NONE, sz))
        toggled = !toggled;
}

/* --- Color Edit with HSV display mode and alpha bar --- */

static void cheat_render_color_edit4_hsv(void)
{
    static float col[4] = {0.55f, 0.85f, 0.40f, 0.90f};
    eli_color_edit4("Accent", col,
                    ELI_COLOR_EDIT_DISPLAY_HSV | ELI_COLOR_EDIT_ALPHA_BAR);
}

#endif /* CHEAT_RENDER_COLOR_H */
