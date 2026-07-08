/**
 * @file render_dragdrop_media.h
 * @brief Live render functions for cheatsheet categories: Drag & Drop, Images,
 *        Plots, Value. Each function is static and owns its own function-static
 *        state so it can be called every frame in immediate-mode style.
 *
 * @status Cheatsheet content (drag-drop / media group). Not library API.
 * @issues None
 * @todo None
 */
#ifndef CHEAT_RENDER_DRAGDROP_MEDIA_H
#define CHEAT_RENDER_DRAGDROP_MEDIA_H

#include <eli/elimgui.h>

/* ---------------------------------------------------------------------------
 * Drag & Drop
 * ------------------------------------------------------------------------- */

/**
 * A working source-to-target drag-and-drop demo. Left-drag "Drag me" onto
 * "Drop here" to transfer an int payload; the received value is shown below.
 */
static void cheat_render_drag_drop_demo(void)
{
    static int s_item     = 42;
    static int s_received = -1;

    /* Source: make the button draggable. */
    eli_button("Drag me");
    if (eli_begin_drag_drop_source(ELI_DRAG_DROP_NONE)) {
        eli_set_drag_drop_payload("INT", &s_item, sizeof(s_item), ELI_COND_ALWAYS);
        eli_text("Dragging: %d", s_item);
        eli_end_drag_drop_source();
    }

    eli_same_line(0.0f, -1.0f);

    /* Target: accept the payload when released over this button. */
    eli_button("Drop here");
    if (eli_begin_drag_drop_target()) {
        const eli_payload *p =
            eli_accept_drag_drop_payload("INT", ELI_DRAG_DROP_NONE);
        if (p != NULL && p->data_size == (int)sizeof(int))
            s_received = *(const int *)p->data;
        eli_end_drag_drop_target();
    }

    if (s_received >= 0)
        eli_text("Last drop: %d", s_received);
}

/* ---------------------------------------------------------------------------
 * Images
 *
 * Tex id 1 is the font atlas assigned in js_start() via
 * eli_font_atlas_set_tex_id(g_atlas, 1).  Using uv0=(0,0)/uv1=(1,1) shows
 * the full atlas (glyph texture); it renders visibly but looks like glyphs.
 * Replace 1u with your own backend texture handle for real image content.
 * ------------------------------------------------------------------------- */

/** Display a 64x64 image with a semi-transparent white border. */
static void cheat_render_image(void)
{
    eli_image(1u,
              eli_make_vec2(64.0f, 64.0f),
              eli_make_vec2(0.0f, 0.0f),
              eli_make_vec2(1.0f, 1.0f),
              eli_make_vec4(1.0f, 1.0f, 1.0f, 1.0f),
              eli_make_vec4(1.0f, 1.0f, 1.0f, 0.5f));
}

/** A 64x64 image button; click count is shown to the right. */
static void cheat_render_image_button(void)
{
    static int s_clicks = 0;
    if (eli_image_button("##imgbtn", 1u,
                         eli_make_vec2(64.0f, 64.0f),
                         eli_make_vec2(0.0f, 0.0f),
                         eli_make_vec2(1.0f, 1.0f),
                         eli_make_vec4(0.0f, 0.0f, 0.0f, 0.0f),
                         eli_make_vec4(1.0f, 1.0f, 1.0f, 1.0f)))
        s_clicks++;
    eli_same_line(0.0f, -1.0f);
    eli_text("clicks: %d", s_clicks);
}

/* ---------------------------------------------------------------------------
 * Plots
 * ------------------------------------------------------------------------- */

/**
 * Line graph of one full period of a sine wave (32 pre-computed samples,
 * no <math.h> required).
 */
static void cheat_render_plot_lines(void)
{
    static const float s_sine[32] = {
         0.000f,  0.195f,  0.383f,  0.556f,  0.707f,  0.831f,  0.924f,  0.981f,
         1.000f,  0.981f,  0.924f,  0.831f,  0.707f,  0.556f,  0.383f,  0.195f,
         0.000f, -0.195f, -0.383f, -0.556f, -0.707f, -0.831f, -0.924f, -0.981f,
        -1.000f, -0.981f, -0.924f, -0.831f, -0.707f, -0.556f, -0.383f, -0.195f
    };
    eli_plot_lines("Sine##lines", s_sine, 32, 0,
                   NULL, -1.1f, 1.1f,
                   eli_make_vec2(0.0f, 0.0f), 0);
}

/** Histogram of a bell-shaped distribution (10 bars). */
static void cheat_render_plot_histogram(void)
{
    static const float s_hist[10] = {
        0.10f, 0.30f, 0.60f, 0.90f, 1.00f,
        0.80f, 0.50f, 0.20f, 0.10f, 0.05f
    };
    eli_plot_histogram("Dist##hist", s_hist, 10, 0,
                       NULL, 0.0f, 1.1f,
                       eli_make_vec2(0.0f, 0.0f), 0);
}

/* ---------------------------------------------------------------------------
 * Value
 * ------------------------------------------------------------------------- */

/** Display a bool with a paired toggle. */
static void cheat_render_value_bool(void)
{
    static bool s_flag = true;
    eli_value_bool("Active", s_flag);
    eli_checkbox("##toggle_vb", &s_flag);
}

/** Display an int with a paired drag control. */
static void cheat_render_value_int(void)
{
    static int s_count = 7;
    eli_value_int("Count", s_count);
    eli_drag_int("##n_vi", &s_count, 1.0f, 0, 100, "%d", 0);
}

/** Display a float with a paired drag and custom-format variant. */
static void cheat_render_value_float(void)
{
    static float s_val = 3.14159f;
    eli_value_float("Pi", s_val, NULL);
    eli_value_float("Pi (5dp)", s_val, "%.5f");
    eli_drag_float("##v_vf", &s_val, 0.01f, 0.0f, 10.0f, "%.3f", 0);
}

#endif /* CHEAT_RENDER_DRAGDROP_MEDIA_H */
