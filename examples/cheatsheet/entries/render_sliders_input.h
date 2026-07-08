/**
 * @file render_sliders_input.h
 * @brief Live widget renderers for the "Sliders & Drags" and "Input" cheatsheet
 *        entries. Each function renders exactly one widget and owns its
 *        function-static state so it can be called every frame.
 *
 * Include this header before the row fragment (rows_sliders_input.h).
 * Do not include it more than once per translation unit.
 */
#ifndef CHEAT_RENDER_SLIDERS_INPUT_H
#define CHEAT_RENDER_SLIDERS_INPUT_H

#include <eli/elimgui.h>

/* ------------------------------------------------------------------ */
/* Sliders & Drags                                                      */
/* ------------------------------------------------------------------ */

static void cheat_render_slider_float(void)
{
    static float v = 0.5f;
    eli_slider_float("amount", &v, 0.0f, 1.0f, "%.2f", 0);
}

static void cheat_render_slider_int(void)
{
    static int v = 5;
    eli_slider_int("level", &v, 0, 10, "%d", 0);
}

static void cheat_render_slider_angle(void)
{
    static float angle = 0.0f;
    eli_slider_angle("rotation", &angle, -180.0f, 180.0f, "%.0f deg", 0);
}

static void cheat_render_slider_float3(void)
{
    static float rgb[3] = {0.4f, 0.7f, 1.0f};
    eli_slider_float3("color", rgb, 0.0f, 1.0f, "%.2f", 0);
}

static void cheat_render_v_slider_float(void)
{
    static float v = 0.5f;
    eli_v_slider_float("##vol", eli_make_vec2(18.0f, 80.0f), &v, 0.0f, 1.0f, "%.2f", 0);
}

static void cheat_render_drag_float(void)
{
    static float x = 10.0f;
    eli_drag_float("x pos", &x, 0.5f, -100.0f, 100.0f, "%.1f", 0);
}

static void cheat_render_drag_int(void)
{
    static int count = 8;
    eli_drag_int("count", &count, 0.2f, 0, 100, "%d", 0);
}

static void cheat_render_drag_float_range2(void)
{
    static float lo = 20.0f;
    static float hi = 80.0f;
    eli_drag_float_range2("range", &lo, &hi, 0.5f, 0.0f, 100.0f, "%.1f", NULL, 0);
}

/* ------------------------------------------------------------------ */
/* Input                                                                */
/* ------------------------------------------------------------------ */

static void cheat_render_input_text(void)
{
    static char buf[128] = "hello";
    eli_input_text("name", buf, sizeof(buf), ELI_INPUT_TEXT_NONE, NULL, NULL);
}

static void cheat_render_input_text_with_hint(void)
{
    static char buf[128] = "";
    eli_input_text_with_hint("search", "type here...", buf, sizeof(buf),
                             ELI_INPUT_TEXT_NONE, NULL, NULL);
}

static void cheat_render_input_text_multiline(void)
{
    static char buf[256] = "line one\nline two";
    eli_input_text_multiline("notes", buf, sizeof(buf),
                             eli_make_vec2(-1.0f, 60.0f),
                             ELI_INPUT_TEXT_NONE, NULL, NULL);
}

static void cheat_render_input_int(void)
{
    static int v = 42;
    eli_input_int("count", &v, 1, 10, ELI_INPUT_TEXT_NONE);
}

static void cheat_render_input_float(void)
{
    static float v = 1.5f;
    eli_input_float("scale", &v, 0.1f, 1.0f, "%.3f", ELI_INPUT_TEXT_NONE);
}

static void cheat_render_input_double(void)
{
    static double v = 3.14159;
    eli_input_double("pi", &v, 0.01, 1.0, "%.5f", ELI_INPUT_TEXT_NONE);
}

static void cheat_render_input_float3(void)
{
    static float v[3] = {0.0f, 0.0f, 0.0f};
    eli_input_float3("position", v, "%.2f", ELI_INPUT_TEXT_NONE);
}

#endif /* CHEAT_RENDER_SLIDERS_INPUT_H */
