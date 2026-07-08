/**
 * @file test_p11_scalar.c
 * @brief Phase 11 unit tests for the slider/drag scalar core: the data-type
 *        read/write/clamp/format helpers, format-precision parsing,
 *        round-to-format, and the linear/logarithmic parametric mapping
 *        (eli_scale_ratio_from_value / eli_scale_value_from_ratio round-trips).
 *
 * @status Phase 11 scalar-core coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/widgets/eli_slider_behavior.h>

ELI_TEST(data_type_size_and_default_format) {
    ELI_ASSERT_EQ(1u, (unsigned)eli_data_type_size(ELI_DATA_TYPE_S8));
    ELI_ASSERT_EQ(4u, (unsigned)eli_data_type_size(ELI_DATA_TYPE_S32));
    ELI_ASSERT_EQ(8u, (unsigned)eli_data_type_size(ELI_DATA_TYPE_DOUBLE));
    ELI_ASSERT_TRUE(eli_data_type_is_float(ELI_DATA_TYPE_FLOAT));
    ELI_ASSERT_FALSE(eli_data_type_is_float(ELI_DATA_TYPE_S32));
    ELI_ASSERT_STR_EQ("%.3f", eli_data_type_default_format(ELI_DATA_TYPE_FLOAT));
    ELI_ASSERT_STR_EQ("%d", eli_data_type_default_format(ELI_DATA_TYPE_S32));
}

ELI_TEST(scalar_read_write_roundtrip_and_clamp) {
    int32_t i = 0;
    eli_scalar_write(ELI_DATA_TYPE_S32, &i, 42.0);
    ELI_ASSERT_EQ(42, i);
    ELI_ASSERT_FLT_NEAR(42.0f, (float)eli_scalar_read(ELI_DATA_TYPE_S32, &i), 0.0001f);

    /* Integer rounding is to nearest. */
    eli_scalar_write(ELI_DATA_TYPE_S32, &i, 2.6);
    ELI_ASSERT_EQ(3, i);
    eli_scalar_write(ELI_DATA_TYPE_S32, &i, 2.4);
    ELI_ASSERT_EQ(2, i);

    /* Writes clamp to the destination type's native range. */
    uint8_t u = 0;
    eli_scalar_write(ELI_DATA_TYPE_U8, &u, 999.0);
    ELI_ASSERT_EQ(255, u);
    int8_t s = 0;
    eli_scalar_write(ELI_DATA_TYPE_S8, &s, -999.0);
    ELI_ASSERT_EQ(-128, s);

    /* eli_scalar_clamp reports and applies out-of-range correction. */
    int32_t lo = 0, hi = 10, v = 25;
    ELI_ASSERT_TRUE(eli_scalar_clamp(ELI_DATA_TYPE_S32, &v, &lo, &hi));
    ELI_ASSERT_EQ(10, v);
    ELI_ASSERT_FALSE(eli_scalar_clamp(ELI_DATA_TYPE_S32, &v, &lo, &hi));
}

ELI_TEST(scalar_format_matches_printf) {
    float f = 3.14159f;
    char buf[64];
    int n = eli_scalar_format(buf, sizeof(buf), ELI_DATA_TYPE_FLOAT, &f, "%.2f");
    ELI_ASSERT_EQ(4, n);
    ELI_ASSERT_STR_EQ("3.14", buf);

    int32_t i = -7;
    n = eli_scalar_format(buf, sizeof(buf), ELI_DATA_TYPE_S32, &i, "%d");
    ELI_ASSERT_STR_EQ("-7", buf);
    ELI_ASSERT_EQ(2, n);
}

ELI_TEST(parse_format_precision_variants) {
    ELI_ASSERT_EQ(3, eli_parse_format_precision("%.3f", 99));
    ELI_ASSERT_EQ(0, eli_parse_format_precision("%.0f deg", 99));
    ELI_ASSERT_EQ(6, eli_parse_format_precision("%.6f", 99));
    /* No precision specifier -> default. */
    ELI_ASSERT_EQ(99, eli_parse_format_precision("%f", 99));
    ELI_ASSERT_EQ(99, eli_parse_format_precision("%d", 99));
}

ELI_TEST(round_scalar_with_format) {
    /* Rounds a float to the format's precision by print-and-parse. */
    double r = eli_round_scalar_with_format("%.1f", ELI_DATA_TYPE_FLOAT, 1.26);
    ELI_ASSERT_FLT_NEAR(1.3f, (float)r, 0.0001f);
    r = eli_round_scalar_with_format("%.0f", ELI_DATA_TYPE_FLOAT, 2.4);
    ELI_ASSERT_FLT_NEAR(2.0f, (float)r, 0.0001f);
    /* Integer types are returned unchanged. */
    r = eli_round_scalar_with_format("%d", ELI_DATA_TYPE_S32, 7.0);
    ELI_ASSERT_FLT_NEAR(7.0f, (float)r, 0.0001f);
}

ELI_TEST(linear_ratio_mapping_float) {
    /* Value -> t. */
    float t = eli_scale_ratio_from_value(ELI_DATA_TYPE_FLOAT, 5.0, 0.0, 10.0, false, 0.0f, 0.0f);
    ELI_ASSERT_FLT_NEAR(0.5f, t, 0.0001f);
    t = eli_scale_ratio_from_value(ELI_DATA_TYPE_FLOAT, 0.0, 0.0, 10.0, false, 0.0f, 0.0f);
    ELI_ASSERT_FLT_NEAR(0.0f, t, 0.0001f);
    t = eli_scale_ratio_from_value(ELI_DATA_TYPE_FLOAT, 10.0, 0.0, 10.0, false, 0.0f, 0.0f);
    ELI_ASSERT_FLT_NEAR(1.0f, t, 0.0001f);

    /* t -> value (extents clamp to endpoints). */
    double v = eli_scale_value_from_ratio(ELI_DATA_TYPE_FLOAT, 0.25f, 0.0, 8.0, false, 0.0f, 0.0f);
    ELI_ASSERT_FLT_NEAR(2.0f, (float)v, 0.0001f);
    v = eli_scale_value_from_ratio(ELI_DATA_TYPE_FLOAT, 0.0f, -4.0, 4.0, false, 0.0f, 0.0f);
    ELI_ASSERT_FLT_NEAR(-4.0f, (float)v, 0.0001f);
    v = eli_scale_value_from_ratio(ELI_DATA_TYPE_FLOAT, 1.0f, -4.0, 4.0, false, 0.0f, 0.0f);
    ELI_ASSERT_FLT_NEAR(4.0f, (float)v, 0.0001f);
}

ELI_TEST(linear_mapping_integer_rounds_to_grab) {
    /* Integer t->value rounds to nearest so clicking a spot lands on a whole unit. */
    double v = eli_scale_value_from_ratio(ELI_DATA_TYPE_S32, 0.5f, 0.0, 10.0, false, 0.0f, 0.0f);
    ELI_ASSERT_FLT_NEAR(5.0f, (float)v, 0.0001f);
    v = eli_scale_value_from_ratio(ELI_DATA_TYPE_S32, 0.34f, 0.0, 10.0, false, 0.0f, 0.0f);
    ELI_ASSERT_FLT_NEAR(3.0f, (float)v, 0.0001f);
}

ELI_TEST(log_mapping_roundtrip) {
    /* A logarithmic map should round-trip value -> t -> value near the input. */
    float eps = eli_slider__log_epsilon(ELI_DATA_TYPE_FLOAT, "%.3f");
    double v0 = 10.0;
    float t = eli_scale_ratio_from_value(ELI_DATA_TYPE_FLOAT, v0, 1.0, 100.0, true, eps, 0.0f);
    ELI_ASSERT_FLT_NEAR(0.5f, t, 0.02f); /* 10 is the geometric mean of 1..100 */
    double v1 = eli_scale_value_from_ratio(ELI_DATA_TYPE_FLOAT, t, 1.0, 100.0, true, eps, 0.0f);
    ELI_ASSERT_FLT_NEAR((float)v0, (float)v1, 0.05f);
}

ELI_TEST_MAIN()
