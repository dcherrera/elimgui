/**
 * @file test_core_enums.c
 * @brief Unit tests for elimgui core enums: count sentinels, ImGui-compatible
 *        values, and flag-combination correctness.
 *
 * @status Phase 1 core-enums coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/core/eli_enums.h>

ELI_TEST(count_sentinels_are_sane) {
    /* Exact sizes of the color and style-var tables (drives array sizing). */
    ELI_ASSERT_EQ(ELI_COL_COUNT, 56);
    ELI_ASSERT_EQ(ELI_STYLE_VAR_COUNT, 33);
    ELI_ASSERT_EQ(ELI_DATA_TYPE_COUNT, 10);
    ELI_ASSERT_EQ(ELI_DIR_COUNT, 4);
    ELI_ASSERT_EQ(ELI_MOUSE_CURSOR_COUNT, 9);

    /* First and last color indices bracket the range. */
    ELI_ASSERT_EQ(ELI_COL_TEXT, 0);
    ELI_ASSERT_GT(ELI_COL_MODAL_WINDOW_DIM_BG, 0);
    ELI_ASSERT_LT(ELI_COL_MODAL_WINDOW_DIM_BG, ELI_COL_COUNT);
    ELI_ASSERT_EQ(ELI_COL_MODAL_WINDOW_DIM_BG, ELI_COL_COUNT - 1);
}

ELI_TEST(direction_values) {
    ELI_ASSERT_EQ(ELI_DIR_NONE, -1);
    ELI_ASSERT_EQ(ELI_DIR_LEFT, 0);
    ELI_ASSERT_EQ(ELI_DIR_RIGHT, 1);
    ELI_ASSERT_EQ(ELI_DIR_UP, 2);
    ELI_ASSERT_EQ(ELI_DIR_DOWN, 3);
}

ELI_TEST(mouse_button_layout) {
    ELI_ASSERT_EQ(ELI_MOUSE_BUTTON_LEFT, 0);
    ELI_ASSERT_EQ(ELI_MOUSE_BUTTON_RIGHT, 1);
    ELI_ASSERT_EQ(ELI_MOUSE_BUTTON_MIDDLE, 2);
    ELI_ASSERT_EQ(ELI_MOUSE_BUTTON_COUNT, 5);
}

ELI_TEST(key_range_ordering) {
    /* NONE is 0 and named keys are a positive, ordered contiguous block. */
    ELI_ASSERT_EQ(ELI_KEY_NONE, 0);
    ELI_ASSERT_EQ(ELI_KEY_TAB, 1);
    ELI_ASSERT_LT(ELI_KEY_A, ELI_KEY_Z);
    ELI_ASSERT_LT(ELI_KEY_0, ELI_KEY_9);
    ELI_ASSERT_LT(ELI_KEY_F1, ELI_KEY_F24);
    /* Named-key sentinel precedes the mouse-as-key codes. */
    ELI_ASSERT_GT(ELI_KEY_COUNT, ELI_KEY_MOD_SUPER);
    ELI_ASSERT_GT(ELI_KEY_MOUSE_LEFT, ELI_KEY_COUNT);
    ELI_ASSERT_LT(ELI_KEY_MOUSE_LEFT, ELI_KEY_MOUSE_WHEEL_Y);
}

ELI_TEST(window_flag_combinations) {
    ELI_ASSERT_EQ(ELI_WINDOW_NONE, 0);
    ELI_ASSERT_EQ(ELI_WINDOW_NO_TITLEBAR, 1 << 0);
    ELI_ASSERT_EQ(ELI_WINDOW_MENU_BAR, 1 << 10);

    /* NO_NAV is the union of its two constituent flags. */
    ELI_ASSERT_EQ(ELI_WINDOW_NO_NAV, ELI_WINDOW_NO_NAV_INPUTS | ELI_WINDOW_NO_NAV_FOCUS);

    /* NO_DECORATION includes each decoration-suppressing flag. */
    ELI_ASSERT_TRUE((ELI_WINDOW_NO_DECORATION & ELI_WINDOW_NO_TITLEBAR) != 0);
    ELI_ASSERT_TRUE((ELI_WINDOW_NO_DECORATION & ELI_WINDOW_NO_RESIZE) != 0);
    ELI_ASSERT_TRUE((ELI_WINDOW_NO_DECORATION & ELI_WINDOW_NO_SCROLLBAR) != 0);
    ELI_ASSERT_TRUE((ELI_WINDOW_NO_DECORATION & ELI_WINDOW_NO_COLLAPSE) != 0);
    /* AUTO_RESIZE is not part of NO_DECORATION. */
    ELI_ASSERT_TRUE((ELI_WINDOW_NO_DECORATION & ELI_WINDOW_AUTO_RESIZE) == 0);
}

ELI_TEST(item_and_child_flags) {
    ELI_ASSERT_EQ(ELI_ITEM_NONE, 0);
    ELI_ASSERT_EQ(ELI_ITEM_DISABLED, 1 << 6);
    ELI_ASSERT_EQ(ELI_CHILD_NONE, 0);
    ELI_ASSERT_EQ(ELI_CHILD_BORDERS, 1 << 0);
    ELI_ASSERT_EQ(ELI_CHILD_NAV_FLATTENED, 1 << 8);
}

ELI_TEST(config_and_backend_flags) {
    ELI_ASSERT_EQ(ELI_CONFIG_FLAGS_NONE, 0);
    ELI_ASSERT_EQ(ELI_CONFIG_FLAGS_NAV_ENABLE_KEYBOARD, 1 << 0);
    ELI_ASSERT_EQ(ELI_CONFIG_FLAGS_NO_MOUSE, 1 << 4);
    ELI_ASSERT_EQ(ELI_BACKEND_FLAGS_NONE, 0);
    ELI_ASSERT_EQ(ELI_BACKEND_FLAGS_HAS_MOUSE_CURSORS, 1 << 1);
    ELI_ASSERT_EQ(ELI_BACKEND_FLAGS_RENDERER_HAS_TEXTURES, 1 << 4);
}

ELI_TEST_MAIN()
