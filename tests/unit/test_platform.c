/**
 * @file test_platform.c
 * @brief Smoke test for the libc seam (eli_platform.h) in hosted test mode.
 *
 * Confirms that including an elimgui core header under -DELI_TEST_HOSTED pulls in
 * a working host libc (types, malloc, mem ops, math) so the native test harness is
 * usable before any library code exists. Phase agents replace/extend this with real
 * per-phase unit tests.
 *
 * @status Harness smoke test.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/core/eli_platform.h>

ELI_TEST(platform_provides_fixed_width_types) {
    ELI_ASSERT_EQ((int)sizeof(uint32_t), 4);
    ELI_ASSERT_EQ((int)sizeof(uint8_t), 1);
}

ELI_TEST(platform_provides_heap_and_mem_ops) {
    uint32_t *buf = (uint32_t *)malloc(4 * sizeof(*buf));
    ELI_ASSERT_NOT_NULL(buf);
    memset(buf, 0, 4 * sizeof(*buf));
    ELI_ASSERT_EQ(buf[2], 0u);
    buf[1] = 0xABCDu;
    ELI_ASSERT_EQ(buf[1], 0xABCDu);
    free(buf);
}

ELI_TEST(platform_provides_math) {
    ELI_ASSERT_FLT_NEAR(sqrtf(25.0f), 5.0f, 1e-6);
}

ELI_TEST_MAIN()
