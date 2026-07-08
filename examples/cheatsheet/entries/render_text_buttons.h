/**
 * @file render_text_buttons.h
 * @brief Live widget render functions for the Text and Basic Widgets cheatsheet entries.
 *
 * One `static void cheat_render_tb_<name>(void)` per entry. Each body is small,
 * draws exactly one widget (or a minimal group), and carries its own
 * function-static state so it can be called every frame (immediate-mode).
 *
 * Named with a `_tb_` infix so they coexist in the same TU as the starter
 * entries in cheat_entries.h without symbol collisions.
 *
 * @status Cheatsheet Text & Basic Widgets content.
 * @issues None
 * @todo None
 */
#ifndef CHEAT_RENDER_TEXT_BUTTONS_H
#define CHEAT_RENDER_TEXT_BUTTONS_H

#include <eli/elimgui.h>

/* ---------------------------------------------------------------------------
 * Text widget renderers
 * ------------------------------------------------------------------------- */

static void cheat_render_tb_text(void)
{
    eli_text("Hello, %s!", "elimgui");
}

static void cheat_render_tb_text_colored(void)
{
    eli_vec4 col = eli_make_vec4(0.2f, 0.85f, 0.4f, 1.0f);
    eli_text_colored(col, "Colored text");
}

static void cheat_render_tb_text_disabled(void)
{
    eli_text_disabled("Disabled (dimmed) text");
}

static void cheat_render_tb_text_wrapped(void)
{
    eli_text_wrapped("Wrapped: this sentence is intentionally long so it wraps to the available content width.");
}

static void cheat_render_tb_label_text(void)
{
    eli_label_text("Status", "%s", "active");
}

static void cheat_render_tb_bullet_text(void)
{
    eli_bullet_text("First item");
    eli_bullet_text("Second item");
}

static void cheat_render_tb_bullet(void)
{
    eli_bullet();
    eli_same_line(0.0f, -1.0f);
    eli_text("inline content");
}

static void cheat_render_tb_separator_text(void)
{
    eli_separator_text("Section");
}

/* ---------------------------------------------------------------------------
 * Basic Widget renderers
 * ------------------------------------------------------------------------- */

static void cheat_render_tb_button(void)
{
    static int clicks = 0;
    if (eli_button("Click me"))
        clicks++;
    eli_same_line(0.0f, -1.0f);
    eli_text("clicked %d", clicks);
}

static void cheat_render_tb_small_button(void)
{
    static int clicks = 0;
    if (eli_small_button("Small"))
        clicks++;
    eli_same_line(0.0f, -1.0f);
    eli_text("clicked %d", clicks);
}

static void cheat_render_tb_arrow_button(void)
{
    static int val = 0;
    if (eli_arrow_button("##arrowl", ELI_DIR_LEFT))
        val--;
    eli_same_line(0.0f, -1.0f);
    eli_text("%d", val);
    eli_same_line(0.0f, -1.0f);
    if (eli_arrow_button("##arrowr", ELI_DIR_RIGHT))
        val++;
}

static void cheat_render_tb_invisible_button(void)
{
    static bool hit = false;
    eli_vec2 sz = eli_make_vec2(80.0f, 24.0f);
    if (eli_invisible_button("##inv", sz, ELI_BUTTON_NONE))
        hit = !hit;
    eli_draw_list *dl = eli_get_window_draw_list();
    eli_vec2 p = eli_get_item_rect_min();
    eli_vec2 q = eli_get_item_rect_max();
    eli_col32 col = hit ? eli_get_color_u32(ELI_COL_BUTTON_ACTIVE, 1.0f)
                        : eli_get_color_u32(ELI_COL_BUTTON, 1.0f);
    eli_draw_list_add_rect(dl, p, q, col, 0.0f, ELI_DRAW_NONE, 1.0f);
    eli_same_line(0.0f, -1.0f);
    eli_text(hit ? "hit!" : "click invisible area");
}

static void cheat_render_tb_checkbox(void)
{
    static bool enabled = true;
    eli_checkbox("Enable feature", &enabled);
}

static void cheat_render_tb_checkbox_flags_int(void)
{
    static int flags = 0x01 | 0x04;
    eli_checkbox_flags_int("Bit 0", &flags, 0x01);
    eli_checkbox_flags_int("Bit 2", &flags, 0x04);
    eli_text("flags = 0x%02X", flags);
}

static void cheat_render_tb_radio_button(void)
{
    static int choice = 0;
    eli_radio_button_int("Alpha", &choice, 0);
    eli_same_line(0.0f, -1.0f);
    eli_radio_button_int("Beta",  &choice, 1);
    eli_same_line(0.0f, -1.0f);
    eli_radio_button_int("Gamma", &choice, 2);
}

static void cheat_render_tb_progress_bar(void)
{
    static float progress = 0.6f;
    eli_progress_bar(progress, eli_make_vec2(-1.0f, 0.0f), NULL);
}

static void cheat_render_tb_text_link(void)
{
    static int clicks = 0;
    if (eli_text_link("elimgui docs"))
        clicks++;
    eli_same_line(0.0f, -1.0f);
    eli_text("(%d clicks)", clicks);
}

#endif /* CHEAT_RENDER_TEXT_BUTTONS_H */
