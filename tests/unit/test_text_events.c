/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file test_text_events.c
 * @brief Ordered UTF-32 text events (D12, spec §9.1): emoji survive into
 *        input_text as 4 UTF-8 bytes, a 10 KB paste in one frame arrives whole,
 *        text/key order is preserved, UTF-16 surrogates pair, mouse positions
 *        coalesce, and the event queue grows past its initial capacity.
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#include "eli_test.h"
#include "eli_test_ui.h"

static char g_buf[16384];
static eli_vec2 g_field_center;

/** A window with one input_text over g_buf; records its center. */
static void ui_field(void)
{
    eli_set_next_window_pos(eli_make_vec2(0, 0), ELI_COND_ALWAYS, eli_make_vec2(0, 0));
    eli_set_next_window_size(eli_make_vec2(400, 200), ELI_COND_ALWAYS);
    if (eli_begin("W", NULL, 0)) {
        eli_input_text("Field", g_buf, sizeof(g_buf), 0, NULL, NULL);
        eli_vec2 mn = eli_get_item_rect_min(), mx = eli_get_item_rect_max();
        g_field_center = eli_make_vec2((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f);
    }
    eli_end();
}

/** Setup + activate the field with an empty buffer. */
static eli_context *setup_active_field(void)
{
    eli_context *ctx = test_ui_setup();
    g_buf[0] = '\0';
    test_ui_frames(ui_field, 2);
    test_ui_click(ui_field, g_field_center);
    return ctx;
}

ELI_TEST(emoji_round_trips_as_four_utf8_bytes) {
    eli_context *ctx = setup_active_field();
    eli_io_add_input_characters_utf8("a\xF0\x9F\x98\x80" "b");    /* a U+1F600 b */
    test_ui_frames(ui_field, 2);
    ELI_ASSERT_EQ((int)strlen(g_buf), 6);
    ELI_ASSERT_TRUE(memcmp(g_buf, "a\xF0\x9F\x98\x80" "b", 6) == 0);
    test_ui_teardown(ctx);
}

ELI_TEST(ten_kb_in_one_frame_arrives_whole) {
    eli_context *ctx = setup_active_field();
    static char big[10241];
    for (int i = 0; i < 10240; i++)
        big[i] = (char)('a' + i % 26);
    big[10240] = '\0';
    eli_io_add_input_characters_utf8(big);
    test_ui_frame(ui_field);
    ELI_ASSERT_EQ((int)strlen(g_buf), 10240);
    ELI_ASSERT_TRUE(strcmp(g_buf, big) == 0);
    test_ui_teardown(ctx);
}

ELI_TEST(text_and_key_order_is_preserved) {
    eli_context *ctx = setup_active_field();
    /* All in one batch: "a", Backspace, "b"  ->  "b" (not "a" with b deleted). */
    eli_io_add_input_character('a');
    eli_io_add_key_event(ELI_KEY_BACKSPACE, true);
    eli_io_add_key_event(ELI_KEY_BACKSPACE, false);
    eli_io_add_input_character('b');
    test_ui_frames(ui_field, 4);
    ELI_ASSERT_STR_EQ(g_buf, "b");
    test_ui_teardown(ctx);
}

ELI_TEST(utf16_surrogates_pair_and_unpaired_become_fffd) {
    eli_context *ctx = test_ui_setup();
    eli_io *io = &ctx->io;
    eli_io_add_input_character_utf16(0xD83D);
    eli_io_add_input_character_utf16(0xDE00);   /* U+1F600 */
    eli_io_add_input_character_utf16(0xDE00);   /* lone low surrogate */
    eli_io_add_input_character_utf16('x');
    eli_input_process_events(io);
    ELI_ASSERT_EQ(io->input_queue_characters_count, 3);
    ELI_ASSERT_EQ(io->input_queue_characters[0], 0x1F600u);
    ELI_ASSERT_EQ(io->input_queue_characters[1], 0xFFFDu);
    ELI_ASSERT_EQ(io->input_queue_characters[2], (uint32_t)'x');
    test_ui_teardown(ctx);
}

ELI_TEST(mouse_positions_coalesce_until_another_event) {
    eli_context *ctx = test_ui_setup();
    eli_io *io = &ctx->io;
    eli_io_add_mouse_pos_event(1, 1);
    eli_io_add_mouse_pos_event(2, 2);
    eli_io_add_mouse_pos_event(3, 3);
    ELI_ASSERT_EQ(io->input_events_count, 1);
    eli_io_add_mouse_button_event(0, true);
    eli_io_add_mouse_pos_event(4, 4);
    ELI_ASSERT_EQ(io->input_events_count, 3);
    ELI_ASSERT_FLT_NEAR(io->input_events[0].data.mouse_pos.x, 3.0f, 1e-6);
    test_ui_teardown(ctx);
}

ELI_TEST(event_queue_grows_without_dropping) {
    eli_context *ctx = test_ui_setup();
    eli_io *io = &ctx->io;
    int n = ELI_INPUT_EVENT_QUEUE_SIZE * 4;
    for (int i = 0; i < n; i++)
        eli_io_add_key_event((i & 1) ? ELI_KEY_A : ELI_KEY_B, (i & 2) != 0);
    ELI_ASSERT_EQ(io->input_events_count, n);
    test_ui_teardown(ctx);
}

ELI_TEST_MAIN()
