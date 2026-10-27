/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file test_input_text.c
 * @brief Unit tests for elimgui text input: ASCII and UTF-8 characters become
 *        TEXT events that fill the UTF-32 queue at begin-frame, non-BMP code points
 *        are preserved, and the queue clears at end-frame.
 *
 * @status Phase 4 text input coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/input/eli_input.h>

ELI_TEST(add_single_characters) {
    eli_context *ctx = eli_create_context();
    eli_io *io = &ctx->io;

    eli_io_add_input_character('A');
    eli_io_add_input_character('z');
    eli_io_add_input_character(0);              /* zero code point is ignored */
    ELI_ASSERT_EQ(io->input_events_count, 2);   /* queued as ordered TEXT events */
    eli_input_process_events(io);
    ELI_ASSERT_EQ(io->input_queue_characters_count, 2);
    ELI_ASSERT_EQ(io->input_queue_characters[0], (uint32_t)'A');
    ELI_ASSERT_EQ(io->input_queue_characters[1], (uint32_t)'z');

    eli_destroy_context(ctx);
}

ELI_TEST(add_utf8_string_decodes_codepoints) {
    eli_context *ctx = eli_create_context();
    eli_io *io = &ctx->io;

    /* "hi" + U+00E9 (e-acute, 2-byte UTF-8: 0xC3 0xA9). */
    eli_io_add_input_characters_utf8("hi\xC3\xA9");
    eli_input_process_events(io);
    ELI_ASSERT_EQ(io->input_queue_characters_count, 3);
    ELI_ASSERT_EQ(io->input_queue_characters[0], (uint32_t)'h');
    ELI_ASSERT_EQ(io->input_queue_characters[1], (uint32_t)'i');
    ELI_ASSERT_EQ(io->input_queue_characters[2], (uint32_t)0x00E9);

    eli_destroy_context(ctx);
}

ELI_TEST(add_utf8_three_byte_codepoint) {
    eli_context *ctx = eli_create_context();
    eli_io *io = &ctx->io;

    /* U+20AC euro sign (3-byte UTF-8: 0xE2 0x82 0xAC). */
    eli_io_add_input_characters_utf8("\xE2\x82\xAC");
    eli_input_process_events(io);
    ELI_ASSERT_EQ(io->input_queue_characters_count, 1);
    ELI_ASSERT_EQ(io->input_queue_characters[0], (uint32_t)0x20AC);

    eli_destroy_context(ctx);
}

ELI_TEST(add_utf8_non_bmp_is_preserved) {
    eli_context *ctx = eli_create_context();
    eli_io *io = &ctx->io;

    /* U+1F600 grinning face (4-byte UTF-8) is kept as one UTF-32 code point. */
    eli_io_add_input_characters_utf8("\xF0\x9F\x98\x80");
    eli_input_process_events(io);
    ELI_ASSERT_EQ(io->input_queue_characters_count, 1);
    ELI_ASSERT_EQ(io->input_queue_characters[0], (uint32_t)0x1F600);

    eli_destroy_context(ctx);
}

ELI_TEST(text_queue_clears_at_end_frame) {
    eli_context *ctx = eli_create_context();
    eli_io *io = &ctx->io;
    io->delta_time = 1.0f / 60.0f;

    eli_io_add_input_characters_utf8("abc");
    eli_input_update_begin_frame();
    ELI_ASSERT_EQ(io->input_queue_characters_count, 3);
    eli_input_update_end_frame();
    ELI_ASSERT_EQ(io->input_queue_characters_count, 0);

    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
