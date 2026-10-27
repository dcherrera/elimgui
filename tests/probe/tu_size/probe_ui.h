/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file probe_ui.h
 * @brief Shared widget-set body for the 1-TU vs 5-TU binary-size probe (D4).
 *
 * Each probe_ui_N.c defines PROBE_UI_FN and includes this, so every TU uses the
 * same broad widget set, as a multi-module app would.
 *
 * @status In use by tu_size_probe.sh.
 * @issues None
 * @todo None
 */
#include <eli/elimgui.h>

#include <stdio.h>

void PROBE_UI_FN(void)
{
    static char buf[128] = "text";
    static float f = 0.5f;
    static bool check = true;
    char title[32];
    snprintf(title, sizeof(title), "%s", __func__);
    if (eli_begin(title, NULL, 0)) {
        eli_text("Hello %d", 42);
        if (eli_button("Button"))
            check = !check;
        eli_checkbox("Check", &check);
        eli_slider_float("Slider", &f, 0.0f, 1.0f, "%.2f", 0);
        eli_input_text("Input", buf, sizeof(buf), 0, NULL, NULL);
        if (eli_begin_table("t", 2, ELI_TABLE_BORDERS)) {
            for (int i = 0; i < 4; i++) {
                eli_table_next_row();
                eli_table_next_column();
                eli_text("%d", i);
                eli_table_next_column();
                eli_text("row");
            }
            eli_end_table();
        }
        if (eli_tree_node("Tree")) {
            eli_text("leaf");
            eli_tree_pop();
        }
    }
    eli_end();
}
