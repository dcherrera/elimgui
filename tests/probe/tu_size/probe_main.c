/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file probe_main.c
 * @brief Size-probe implementation TU: owns the context and calls the four UI TUs.
 *
 * @status In use by tu_size_probe.sh.
 * @issues None
 * @todo None
 */
#define ELI_IMPLEMENTATION
#include <eli/elimgui.h>

void probe_ui_1(void);
void probe_ui_2(void);
void probe_ui_3(void);
void probe_ui_4(void);

int main(void)
{
    eli_context *ctx = eli_create_context();
    for (int frame = 0; frame < 3; frame++) {
        ctx->io.display_size = eli_make_vec2(1280.0f, 800.0f);
        ctx->io.delta_time = 1.0f / 60.0f;
        eli_frame_begin();
        probe_ui_1();
        probe_ui_2();
        probe_ui_3();
        probe_ui_4();
        eli_frame_end();
    }
    eli_destroy_context(ctx);
    return 0;
}
