/**
 * @file test_shortcut_routing.c
 * @brief Shortcut routing and Cmd/Ctrl chords (D14, D15, spec §9.2-9.3):
 *        FOCUSED / GLOBAL / ACTIVE routes, an active input_text owning its edit
 *        keys, and ELI_MOD_SHORTCUT resolving to Cmd (mac) or Ctrl.
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#include "eli_test.h"
#include "eli_test_ui.h"

static char g_buf[64] = "hello";
static eli_vec2 g_field_center;
static bool g_fired_a, g_fired_b, g_fired_global, g_fired_save, g_fired_active;
static eli_key_chord g_chord;

/** Two windows: A has a field and tests chords; B tests the same chord. */
static void ui_two_windows(void)
{
    g_fired_a = g_fired_b = g_fired_global = g_fired_save = g_fired_active = false;
    eli_set_next_window_pos(eli_make_vec2(0, 0), ELI_COND_ALWAYS, eli_make_vec2(0, 0));
    eli_set_next_window_size(eli_make_vec2(300, 200), ELI_COND_ALWAYS);
    if (eli_begin("A", NULL, 0)) {
        eli_input_text("Field", g_buf, sizeof(g_buf), 0, NULL, NULL);
        g_fired_active = eli_shortcut_ex(g_chord, ELI_INPUT_ROUTE_ACTIVE);
        eli_vec2 mn = eli_get_item_rect_min(), mx = eli_get_item_rect_max();
        g_field_center = eli_make_vec2((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f);
        g_fired_a = eli_shortcut(g_chord);
    }
    eli_end();
    eli_set_next_window_pos(eli_make_vec2(400, 0), ELI_COND_ALWAYS, eli_make_vec2(0, 0));
    eli_set_next_window_size(eli_make_vec2(300, 200), ELI_COND_ALWAYS);
    if (eli_begin("B", NULL, 0))
        g_fired_b = eli_shortcut(g_chord);
    eli_end();
    g_fired_global = eli_shortcut_ex(g_chord, ELI_INPUT_ROUTE_GLOBAL);
    g_fired_save = eli_shortcut_ex(ELI_MOD_SHORTCUT | ELI_KEY_S, ELI_INPUT_ROUTE_GLOBAL);
}

/** Press mods+key in one frame and report what fired in that frame. */
static void press(eli_key mod, eli_key key)
{
    if (mod != ELI_KEY_NONE)
        eli_io_add_key_event(mod, true);
    eli_io_add_key_event(key, true);
    test_ui_frame(ui_two_windows);
}

/** Release mods+key and settle a frame. */
static void release(eli_key mod, eli_key key)
{
    eli_io_add_key_event(key, false);
    if (mod != ELI_KEY_NONE)
        eli_io_add_key_event(mod, false);
    test_ui_frame(ui_two_windows);
}

ELI_TEST(focused_route_fires_only_in_focused_window) {
    eli_context *ctx = test_ui_setup();
    g_chord = ELI_MOD_SHORTCUT | ELI_KEY_K;
    test_ui_frames(ui_two_windows, 2);
    test_ui_click(ui_two_windows, eli_make_vec2(500, 100));      /* focus B (empty area) */
    press(ELI_KEY_LEFT_CTRL, ELI_KEY_K);
    ELI_ASSERT_FALSE(g_fired_a);
    ELI_ASSERT_TRUE(g_fired_b);
    ELI_ASSERT_TRUE(g_fired_global);
    release(ELI_KEY_LEFT_CTRL, ELI_KEY_K);
    test_ui_teardown(ctx);
}

ELI_TEST(active_input_text_owns_edit_chords_but_not_app_chords) {
    eli_context *ctx = test_ui_setup();
    g_chord = ELI_SHORTCUT_COPY;
    test_ui_frames(ui_two_windows, 2);
    test_ui_click(ui_two_windows, g_field_center);              /* activate the field */
    ELI_ASSERT_NE(ctx->active_id, 0u);
    press(ELI_KEY_LEFT_CTRL, ELI_KEY_C);
    ELI_ASSERT_FALSE(g_fired_global);                            /* field owns C */
    ELI_ASSERT_FALSE(g_fired_a);
    ELI_ASSERT_TRUE(g_fired_active);                             /* the field itself */
    release(ELI_KEY_LEFT_CTRL, ELI_KEY_C);
    press(ELI_KEY_LEFT_CTRL, ELI_KEY_S);
    ELI_ASSERT_TRUE(g_fired_save);                               /* Ctrl+S still reaches the app */
    release(ELI_KEY_LEFT_CTRL, ELI_KEY_S);
    test_ui_teardown(ctx);
}

ELI_TEST(global_route_fires_again_after_field_deactivates) {
    eli_context *ctx = test_ui_setup();
    g_chord = ELI_SHORTCUT_COPY;
    test_ui_frames(ui_two_windows, 2);
    test_ui_click(ui_two_windows, g_field_center);
    eli_io_add_key_event(ELI_KEY_ESCAPE, true);                  /* deactivate */
    test_ui_frame(ui_two_windows);
    eli_io_add_key_event(ELI_KEY_ESCAPE, false);
    test_ui_frames(ui_two_windows, 2);
    ELI_ASSERT_EQ(ctx->active_id, 0u);
    press(ELI_KEY_LEFT_CTRL, ELI_KEY_C);
    ELI_ASSERT_TRUE(g_fired_global);
    release(ELI_KEY_LEFT_CTRL, ELI_KEY_C);
    test_ui_teardown(ctx);
}

ELI_TEST(shortcut_modifier_is_cmd_on_mac_and_ctrl_elsewhere) {
    eli_context *ctx = test_ui_setup();
    g_chord = ELI_MOD_SHORTCUT | ELI_KEY_K;
    test_ui_frames(ui_two_windows, 2);
    ctx->io.config_mac_os_behaviors = true;
    press(ELI_KEY_LEFT_SUPER, ELI_KEY_K);
    ELI_ASSERT_TRUE(g_fired_global);
    release(ELI_KEY_LEFT_SUPER, ELI_KEY_K);
    press(ELI_KEY_LEFT_CTRL, ELI_KEY_K);
    ELI_ASSERT_FALSE(g_fired_global);                           /* real Ctrl stays Ctrl */
    release(ELI_KEY_LEFT_CTRL, ELI_KEY_K);
    ctx->io.config_mac_os_behaviors = false;
    press(ELI_KEY_LEFT_SUPER, ELI_KEY_K);
    ELI_ASSERT_FALSE(g_fired_global);
    release(ELI_KEY_LEFT_SUPER, ELI_KEY_K);
    press(ELI_KEY_LEFT_CTRL, ELI_KEY_K);
    ELI_ASSERT_TRUE(g_fired_global);
    release(ELI_KEY_LEFT_CTRL, ELI_KEY_K);
    test_ui_teardown(ctx);
}

ELI_TEST(input_text_pastes_with_cmd_v_on_mac) {
    eli_context *ctx = test_ui_setup();
    ctx->io.config_mac_os_behaviors = true;
    g_chord = ELI_MOD_SHORTCUT | ELI_KEY_K;
    snprintf(g_buf, sizeof(g_buf), "%s", "x");
    test_ui_frames(ui_two_windows, 2);
    test_ui_click(ui_two_windows, g_field_center);
    eli_set_clipboard_text("Y");
    press(ELI_KEY_LEFT_CTRL, ELI_KEY_V);                         /* Ctrl+V: not paste on mac */
    release(ELI_KEY_LEFT_CTRL, ELI_KEY_V);
    ELI_ASSERT_STR_EQ(g_buf, "x");
    press(ELI_KEY_LEFT_SUPER, ELI_KEY_V);
    release(ELI_KEY_LEFT_SUPER, ELI_KEY_V);
    ELI_ASSERT_STR_EQ(g_buf, "xY");
    test_ui_teardown(ctx);
}

ELI_TEST_MAIN()
