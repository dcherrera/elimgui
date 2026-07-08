/**
 * @file test_p28_logging.c
 * @brief Unit tests for Phase 28 logging: clipboard capture round-trip, buffer
 *        accumulation across multiple appends, inactive-session no-ops, and the
 *        auto-open depth parameter being accepted/stored.
 *
 * @status Phase 28 logging coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/util/eli_logging.h>
#include <eli/core/eli_core.h>

ELI_TEST(logging_to_clipboard_round_trip) {
    eli_context *ctx = eli_create_context();

    eli_log_to_clipboard(0);
    eli_log_text("x=%d", 5);
    eli_log_finish();

    /* In hosted builds eli_set_clipboard_text writes the fallback buffer, which
     * eli_get_clipboard_text reads back. */
    ELI_ASSERT_STR_EQ(eli_get_clipboard_text(), "x=5");

    eli_destroy_context(ctx);
}

ELI_TEST(logging_accumulates_multiple_appends) {
    eli_context *ctx = eli_create_context();

    eli_log_to_clipboard(-1);
    eli_log_text("a=%d ", 1);
    eli_log_text("b=%s", "two");
    ELI_ASSERT_EQ(eli_log_buffer_len, strlen("a=1 b=two"));
    eli_log_finish();

    ELI_ASSERT_STR_EQ(eli_get_clipboard_text(), "a=1 b=two");
    /* Buffer is reset after finishing. */
    ELI_ASSERT_EQ(eli_log_buffer_len, 0u);
    ELI_ASSERT_FALSE(eli_log_active);

    eli_destroy_context(ctx);
}

ELI_TEST(logging_text_is_noop_when_inactive) {
    ELI_ASSERT_FALSE(eli_log_active);
    /* No active session: append must not touch the buffer. */
    eli_log_text("ignored=%d", 99);
    ELI_ASSERT_EQ(eli_log_buffer_len, 0u);
    /* Finishing with no active session is a safe no-op. */
    eli_log_finish();
    ELI_ASSERT_FALSE(eli_log_active);
}

ELI_TEST(logging_auto_open_depth_accepted) {
    eli_log_to_tty(7);
    ELI_ASSERT_TRUE(eli_log_active);
    ELI_ASSERT_EQ(eli_log_active_target, ELI_LOG_TARGET_TTY);
    ELI_ASSERT_EQ(eli_log_auto_open_depth, 7);
    eli_log_finish();

    eli_log_to_file(3, "session.log");
    ELI_ASSERT_EQ(eli_log_active_target, ELI_LOG_TARGET_FILE);
    ELI_ASSERT_EQ(eli_log_auto_open_depth, 3);
    ELI_ASSERT_STR_EQ(eli_log_filename, "session.log");
    eli_log_finish();
    ELI_ASSERT_FALSE(eli_log_active);
}

ELI_TEST_MAIN()
