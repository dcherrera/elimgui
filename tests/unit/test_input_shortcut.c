/**
 * @file test_input_shortcut.c
 * @brief Unit tests for elimgui shortcuts and clipboard: predefined edit chords
 *        via eli_shortcut, the static-buffer clipboard fallback (hosted build,
 *        no JS interop), and custom clipboard callbacks.
 *
 * @status Phase 4 shortcut + clipboard coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/input/eli_input.h>

#define TEST_DT (1.0f / 60.0f)

ELI_TEST(shortcut_copy_fires_on_chord) {
    eli_context *ctx = eli_create_context();
    ctx->io.delta_time = TEST_DT;

    eli_io_add_key_event(ELI_KEY_LEFT_CTRL, true);
    eli_io_add_key_event(ELI_KEY_C, true);
    eli_input_update_begin_frame();
    ELI_ASSERT_TRUE(eli_shortcut(ELI_SHORTCUT_COPY));
    ELI_ASSERT_FALSE(eli_shortcut(ELI_SHORTCUT_PASTE));
    eli_input_update_end_frame();

    /* Held into the next frame -> no fresh press -> shortcut does not fire. */
    eli_input_update_begin_frame();
    ELI_ASSERT_FALSE(eli_shortcut(ELI_SHORTCUT_COPY));
    eli_input_update_end_frame();

    eli_destroy_context(ctx);
}

ELI_TEST(clipboard_fallback_round_trip) {
    eli_context *ctx = eli_create_context();

    eli_set_clipboard_text("hello world");
    ELI_ASSERT_STR_EQ(eli_get_clipboard_text(), "hello world");

    eli_set_clipboard_text("second value");
    ELI_ASSERT_STR_EQ(eli_get_clipboard_text(), "second value");

    /* NULL clears the buffer. */
    eli_set_clipboard_text(NULL);
    ELI_ASSERT_STR_EQ(eli_get_clipboard_text(), "");

    eli_destroy_context(ctx);
}

/* Callback-backed clipboard store for the override test. */
static char test_clipboard_store[64];

static const char *test_get_clipboard(void *user_data)
{
    (void)user_data;
    return test_clipboard_store;
}

static void test_set_clipboard(void *user_data, const char *text)
{
    int *calls = (int *)user_data;
    (*calls)++;
    size_t n = text ? strlen(text) : 0;
    if (n >= sizeof(test_clipboard_store))
        n = sizeof(test_clipboard_store) - 1;
    if (n > 0)
        memcpy(test_clipboard_store, text, n);
    test_clipboard_store[n] = '\0';
}

ELI_TEST(clipboard_callbacks_override_fallback) {
    eli_context *ctx = eli_create_context();
    int set_calls = 0;

    eli_set_clipboard_callbacks(test_get_clipboard, test_set_clipboard, &set_calls);
    eli_set_clipboard_text("via callback");
    ELI_ASSERT_EQ(set_calls, 1);
    ELI_ASSERT_STR_EQ(eli_get_clipboard_text(), "via callback");
    ELI_ASSERT_STR_EQ(test_clipboard_store, "via callback");

    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
