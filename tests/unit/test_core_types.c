/**
 * @file test_core_types.c
 * @brief Unit tests for elimgui core value types: vector/rect helpers, color
 *        pack/unpack round-trips, and rect containment edge cases.
 *
 * @status Phase 1 core-types coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/core/eli_types.h>

ELI_TEST(vec2_construct_and_arithmetic) {
    eli_vec2 a = eli_make_vec2(3.0f, 4.0f);
    eli_vec2 b = eli_make_vec2(1.0f, 2.0f);
    ELI_ASSERT_FLT_NEAR(a.x, 3.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(a.y, 4.0f, 1e-6);

    eli_vec2 sum = eli_vec2_add(a, b);
    ELI_ASSERT_FLT_NEAR(sum.x, 4.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(sum.y, 6.0f, 1e-6);

    eli_vec2 diff = eli_vec2_sub(a, b);
    ELI_ASSERT_FLT_NEAR(diff.x, 2.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(diff.y, 2.0f, 1e-6);

    eli_vec2 scaled = eli_vec2_scale(a, 2.0f);
    ELI_ASSERT_FLT_NEAR(scaled.x, 6.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(scaled.y, 8.0f, 1e-6);
}

ELI_TEST(scalar_helpers) {
    ELI_ASSERT_FLT_NEAR(eli_min_f(2.0f, 5.0f), 2.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(eli_max_f(2.0f, 5.0f), 5.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(eli_clamp_f(7.0f, 0.0f, 5.0f), 5.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(eli_clamp_f(-1.0f, 0.0f, 5.0f), 0.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(eli_clamp_f(3.0f, 0.0f, 5.0f), 3.0f, 1e-6);
}

ELI_TEST(rect_dimensions_and_corners) {
    eli_rect r = eli_make_rect(10.0f, 20.0f, 100.0f, 50.0f);
    ELI_ASSERT_FLT_NEAR(eli_rect_width(r), 100.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(eli_rect_height(r), 50.0f, 1e-6);

    eli_vec2 sz = eli_rect_size(r);
    ELI_ASSERT_FLT_NEAR(sz.x, 100.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(sz.y, 50.0f, 1e-6);

    eli_vec2 mn = eli_rect_min(r);
    eli_vec2 mx = eli_rect_max(r);
    ELI_ASSERT_FLT_NEAR(mn.x, 10.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(mn.y, 20.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(mx.x, 110.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(mx.y, 70.0f, 1e-6);

    eli_vec2 c = eli_rect_center(r);
    ELI_ASSERT_FLT_NEAR(c.x, 60.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(c.y, 45.0f, 1e-6);
}

ELI_TEST(rect_contains_edges) {
    eli_rect r = eli_make_rect(0.0f, 0.0f, 10.0f, 10.0f);
    /* Interior point. */
    ELI_ASSERT_TRUE(eli_rect_contains(r, eli_make_vec2(5.0f, 5.0f)));
    /* Min edges are inclusive. */
    ELI_ASSERT_TRUE(eli_rect_contains(r, eli_make_vec2(0.0f, 0.0f)));
    /* Max edges are exclusive. */
    ELI_ASSERT_FALSE(eli_rect_contains(r, eli_make_vec2(10.0f, 5.0f)));
    ELI_ASSERT_FALSE(eli_rect_contains(r, eli_make_vec2(5.0f, 10.0f)));
    ELI_ASSERT_FALSE(eli_rect_contains(r, eli_make_vec2(10.0f, 10.0f)));
    /* Clearly outside. */
    ELI_ASSERT_FALSE(eli_rect_contains(r, eli_make_vec2(-1.0f, 5.0f)));
    ELI_ASSERT_FALSE(eli_rect_contains(r, eli_make_vec2(5.0f, 11.0f)));
}

ELI_TEST(col32_macro_layout) {
    /* R=0x11 G=0x22 B=0x33 A=0x44 packs A<<24 | B<<16 | G<<8 | R. */
    eli_col32 c = ELI_COL32(0x11, 0x22, 0x33, 0x44);
    ELI_ASSERT_EQ(c, 0x44332211u);
    ELI_ASSERT_EQ((c >> ELI_COL32_R_SHIFT) & 0xFFu, 0x11u);
    ELI_ASSERT_EQ((c >> ELI_COL32_G_SHIFT) & 0xFFu, 0x22u);
    ELI_ASSERT_EQ((c >> ELI_COL32_B_SHIFT) & 0xFFu, 0x33u);
    ELI_ASSERT_EQ((c >> ELI_COL32_A_SHIFT) & 0xFFu, 0x44u);

    ELI_ASSERT_EQ((eli_col32)ELI_COL32_WHITE, 0xFFFFFFFFu);
    ELI_ASSERT_EQ((eli_col32)ELI_COL32_BLACK, 0x000000FFu);
    ELI_ASSERT_EQ((eli_col32)ELI_COL32_BLACK_TRANS, 0x00000000u);
}

ELI_TEST(color_u32_to_vec4_values) {
    eli_vec4 v = eli_color_u32_to_vec4(ELI_COL32(255, 0, 128, 255));
    ELI_ASSERT_FLT_NEAR(v.x, 1.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(v.y, 0.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(v.z, 128.0f / 255.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(v.w, 1.0f, 1e-6);
}

ELI_TEST(color_vec4_to_u32_saturates) {
    /* Out-of-range components clamp to [0,255]. */
    eli_col32 c = eli_color_vec4_to_u32(eli_make_vec4(2.0f, -1.0f, 0.5f, 1.0f));
    ELI_ASSERT_EQ((c >> ELI_COL32_R_SHIFT) & 0xFFu, 255u);
    ELI_ASSERT_EQ((c >> ELI_COL32_G_SHIFT) & 0xFFu, 0u);
    ELI_ASSERT_EQ((c >> ELI_COL32_B_SHIFT) & 0xFFu, 128u); /* round(0.5*255+0.5)=128 */
    ELI_ASSERT_EQ((c >> ELI_COL32_A_SHIFT) & 0xFFu, 255u);
}

ELI_TEST(color_pack_unpack_roundtrip) {
    /* Integer-exact colors survive u32 -> vec4 -> u32 unchanged. */
    eli_col32 samples[] = {
        ELI_COL32(0, 0, 0, 0),
        ELI_COL32(255, 255, 255, 255),
        ELI_COL32(12, 34, 56, 78),
        ELI_COL32(200, 100, 50, 25),
        ELI_COL32_BLACK,
        ELI_COL32_WHITE
    };
    for (unsigned i = 0; i < sizeof(samples) / sizeof(samples[0]); i++) {
        eli_vec4 v = eli_color_u32_to_vec4(samples[i]);
        eli_col32 back = eli_color_vec4_to_u32(v);
        ELI_ASSERT_EQ(back, samples[i]);
    }
}

ELI_TEST_MAIN()
