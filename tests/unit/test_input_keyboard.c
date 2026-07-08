/**
 * @file test_input_keyboard.c
 * @brief Unit tests for elimgui keyboard input: down/pressed/released timing,
 *        modifier tracking, key-chord matching, typematic repeat amounts, and
 *        key names.
 *
 * @status Phase 4 keyboard input coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/input/eli_input.h>

#define TEST_DT (1.0f / 60.0f)

static eli_context *kb_setup(void)
{
    eli_context *ctx = eli_create_context();
    ctx->io.delta_time = TEST_DT;
    return ctx;
}

ELI_TEST(key_press_down_release_lifecycle) {
    eli_context *ctx = kb_setup();

    /* Frame 1: key goes down -> pressed this frame only. */
    eli_io_add_key_event(ELI_KEY_A, true);
    eli_input_update_begin_frame();
    ELI_ASSERT_TRUE(eli_is_key_down(ELI_KEY_A));
    ELI_ASSERT_TRUE(eli_is_key_pressed(ELI_KEY_A));
    ELI_ASSERT_FALSE(eli_is_key_released(ELI_KEY_A));
    eli_input_update_end_frame();

    /* Frame 2: still held -> down but not a fresh press. */
    eli_input_update_begin_frame();
    ELI_ASSERT_TRUE(eli_is_key_down(ELI_KEY_A));
    ELI_ASSERT_FALSE(eli_is_key_pressed(ELI_KEY_A));
    eli_input_update_end_frame();

    /* Frame 3: release -> released this frame only. */
    eli_io_add_key_event(ELI_KEY_A, false);
    eli_input_update_begin_frame();
    ELI_ASSERT_FALSE(eli_is_key_down(ELI_KEY_A));
    ELI_ASSERT_TRUE(eli_is_key_released(ELI_KEY_A));
    eli_input_update_end_frame();

    /* Frame 4: nothing. */
    eli_input_update_begin_frame();
    ELI_ASSERT_FALSE(eli_is_key_released(ELI_KEY_A));
    eli_input_update_end_frame();

    eli_destroy_context(ctx);
}

ELI_TEST(modifier_tracking_from_physical_keys) {
    eli_context *ctx = kb_setup();

    eli_io_add_key_event(ELI_KEY_LEFT_CTRL, true);
    eli_input_update_begin_frame();
    ELI_ASSERT_TRUE(ctx->io.key_ctrl);
    ELI_ASSERT_TRUE(eli_is_key_down(ELI_KEY_MOD_CTRL));
    ELI_ASSERT_FALSE(ctx->io.key_shift);
    eli_input_update_end_frame();

    eli_io_add_key_event(ELI_KEY_LEFT_CTRL, false);
    eli_input_update_begin_frame();
    ELI_ASSERT_FALSE(ctx->io.key_ctrl);
    ELI_ASSERT_FALSE(eli_is_key_down(ELI_KEY_MOD_CTRL));
    eli_input_update_end_frame();

    eli_destroy_context(ctx);
}

ELI_TEST(key_chord_pressed_requires_exact_mods) {
    eli_context *ctx = kb_setup();

    /* Ctrl+C pressed together this frame. */
    eli_io_add_key_event(ELI_KEY_LEFT_CTRL, true);
    eli_io_add_key_event(ELI_KEY_C, true);
    eli_input_update_begin_frame();
    ELI_ASSERT_TRUE(eli_is_key_chord_pressed(ELI_MOD_CTRL | ELI_KEY_C));
    /* Bare C (no mods) must NOT match while Ctrl is held. */
    ELI_ASSERT_FALSE(eli_is_key_chord_pressed(ELI_KEY_C));
    eli_input_update_end_frame();

    /* Next frame: C still held -> not a fresh press -> chord no longer fires. */
    eli_input_update_begin_frame();
    ELI_ASSERT_FALSE(eli_is_key_chord_pressed(ELI_MOD_CTRL | ELI_KEY_C));
    eli_input_update_end_frame();

    eli_destroy_context(ctx);
}

ELI_TEST(key_pressed_amount_repeat) {
    eli_context *ctx = kb_setup();

    /* First frame down: exactly one press registered. */
    eli_io_add_key_event(ELI_KEY_RIGHT_ARROW, true);
    eli_input_update_begin_frame();
    ELI_ASSERT_EQ(eli_get_key_pressed_amount(ELI_KEY_RIGHT_ARROW, -1.0f, -1.0f), 1);
    eli_input_update_end_frame();

    /* Hold well past the repeat delay using a large delta so a repeat lands.
     * With delay=0.275 and rate=0.050, jumping to ~1.0s of hold yields several
     * repeats across the interval. */
    ctx->io.delta_time = 1.0f;
    eli_input_update_begin_frame();
    int amount = eli_get_key_pressed_amount(ELI_KEY_RIGHT_ARROW, -1.0f, -1.0f);
    ELI_ASSERT_GT(amount, 0);
    eli_input_update_end_frame();

    eli_destroy_context(ctx);
}

ELI_TEST(typematic_repeat_amount_math) {
    /* No time elapsed at press -> single count. */
    ELI_ASSERT_EQ(eli_calc_typematic_repeat_amount(0.0f, 0.0f, 0.275f, 0.050f), 1);
    /* Before the delay -> no repeats. */
    ELI_ASSERT_EQ(eli_calc_typematic_repeat_amount(0.0f, 0.1f, 0.275f, 0.050f), 0);
    /* Crossing delay+one rate boundary -> one repeat. */
    int one = eli_calc_typematic_repeat_amount(0.30f, 0.36f, 0.275f, 0.050f);
    ELI_ASSERT_EQ(one, 1);
    /* t0 >= t1 -> zero. */
    ELI_ASSERT_EQ(eli_calc_typematic_repeat_amount(0.5f, 0.5f, 0.275f, 0.050f), 0);
}

ELI_TEST(key_names) {
    ELI_ASSERT_STR_EQ(eli_get_key_name(ELI_KEY_NONE), "None");
    ELI_ASSERT_STR_EQ(eli_get_key_name(ELI_KEY_A), "A");
    ELI_ASSERT_STR_EQ(eli_get_key_name(ELI_KEY_SPACE), "Space");
    ELI_ASSERT_STR_EQ(eli_get_key_name(ELI_KEY_ENTER), "Enter");
    ELI_ASSERT_STR_EQ(eli_get_key_name(ELI_KEY_F5), "F5");
    ELI_ASSERT_STR_EQ(eli_get_key_name(ELI_KEY_LEFT_CTRL), "LeftCtrl");
    ELI_ASSERT_STR_EQ(eli_get_key_name(ELI_KEY_KEYPAD_7), "Keypad7");
    ELI_ASSERT_STR_EQ(eli_get_key_name((eli_key)99999), "Unknown");
}

ELI_TEST(key_capture_override) {
    eli_context *ctx = kb_setup();

    eli_set_next_frame_want_capture_keyboard(true);
    eli_input_update_begin_frame();
    ELI_ASSERT_TRUE(ctx->io.want_capture_keyboard);
    eli_input_update_end_frame();

    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
