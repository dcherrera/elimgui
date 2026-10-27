/**
 * @file test_clipboard_uncapped.c
 * @brief Uncapped clipboard (D13, spec §12): 64 KB copy from input_text through
 *        the io callbacks and through the fallback buffer, 64 KB paste, UTF-8
 *        paste, and over-cap edits disabling undo/revert instead of truncating.
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#include "eli_test.h"
#include "eli_test_ui.h"

#define BIG (64 * 1024)

static char g_buf[BIG * 2 + 64];
static eli_vec2 g_field_center;
static char *g_cb_text = NULL;

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

static const char *cb_get(void *ud)
{
    (void)ud;
    return g_cb_text ? g_cb_text : "";
}

static void cb_set(void *ud, const char *text)
{
    (void)ud;
    free(g_cb_text);
    size_t n = strlen(text);
    g_cb_text = (char *)malloc(n + 1);
    memcpy(g_cb_text, text, n + 1);
}

static void fill_big(char *dst, int n)
{
    for (int i = 0; i < n; i++)
        dst[i] = (char)('A' + i % 23);
    dst[n] = '\0';
}

static void chord(eli_key key)
{
    eli_key keys[2] = {ELI_KEY_LEFT_CTRL, key};
    test_ui_chord(ui_field, 2, keys);
}

static eli_context *setup_field(void)
{
    eli_context *ctx = test_ui_setup();
    test_ui_frames(ui_field, 2);
    test_ui_click(ui_field, g_field_center);
    return ctx;
}

ELI_TEST(copy_64kb_through_callbacks) {
    fill_big(g_buf, BIG);
    eli_context *ctx = setup_field();
    eli_set_clipboard_callbacks(cb_get, cb_set, NULL);
    chord(ELI_KEY_A);
    chord(ELI_KEY_C);
    ELI_ASSERT_NOT_NULL(g_cb_text);
    ELI_ASSERT_EQ((int)strlen(g_cb_text), BIG);
    ELI_ASSERT_TRUE(strcmp(g_cb_text, g_buf) == 0);
    free(g_cb_text);
    g_cb_text = NULL;
    test_ui_teardown(ctx);
}

ELI_TEST(copy_64kb_through_fallback_then_paste_doubles) {
    fill_big(g_buf, BIG);
    eli_context *ctx = setup_field();
    chord(ELI_KEY_A);
    chord(ELI_KEY_C);
    ELI_ASSERT_EQ((int)strlen(eli_get_clipboard_text()), BIG);
    eli_io_add_key_event(ELI_KEY_END, true);                     /* collapse to the end */
    test_ui_frame(ui_field);
    eli_io_add_key_event(ELI_KEY_END, false);
    test_ui_frame(ui_field);
    chord(ELI_KEY_V);
    ELI_ASSERT_EQ((int)strlen(g_buf), 2 * BIG);
    test_ui_teardown(ctx);
}

ELI_TEST(paste_decodes_utf8) {
    g_buf[0] = '\0';
    eli_context *ctx = setup_field();
    eli_set_clipboard_text("\xC3\xA9\xF0\x9F\x98\x80");          /* e-acute + U+1F600 */
    chord(ELI_KEY_V);
    ELI_ASSERT_STR_EQ(g_buf, "\xC3\xA9\xF0\x9F\x98\x80");
    test_ui_teardown(ctx);
}

ELI_TEST(over_cap_edit_disables_undo_instead_of_truncating) {
    fill_big(g_buf, 1000);                                       /* > ELI_INPUT_TEXT_SNAPSHOT_MAX */
    eli_context *ctx = setup_field();
    eli_io_add_input_character('!');
    test_ui_frame(ui_field);
    ELI_ASSERT_EQ((int)strlen(g_buf), 1001);
    chord(ELI_KEY_Z);                                             /* undo: unavailable */
    ELI_ASSERT_EQ((int)strlen(g_buf), 1001);
    eli_io_add_key_event(ELI_KEY_ESCAPE, true);                  /* revert: unavailable */
    test_ui_frame(ui_field);
    eli_io_add_key_event(ELI_KEY_ESCAPE, false);
    test_ui_frame(ui_field);
    ELI_ASSERT_EQ((int)strlen(g_buf), 1001);
    test_ui_teardown(ctx);
}

ELI_TEST_MAIN()
