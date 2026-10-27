/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file test_platform_modes.c
 * @brief libc-seam mode macros (D3): one mode resolves, no JS interop natively.
 *
 * Runs in both runner passes: hosted (-DELI_TEST_HOSTED, the back-compat alias)
 * and shared (-DELI_LIBC_SYSTEM). Either way the seam must resolve to
 * ELI_LIBC_SYSTEM with ELI_JSIO off and stdio available.
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/core/eli_platform.h>

#if !defined(ELI_LIBC_SYSTEM)
#error "native build must resolve to ELI_LIBC_SYSTEM"
#endif

ELI_TEST(native_build_resolves_to_system_libc) {
#if defined(ELI_LIBC_SYSTEM) && !defined(ELI_LIBC_JACLIBC)
    ELI_ASSERT_TRUE(1);
#else
    ELI_FAIL("expected ELI_LIBC_SYSTEM only");
#endif
}

ELI_TEST(js_interop_is_off_natively) {
#ifdef ELI_JSIO
    ELI_FAIL("ELI_JSIO must be wasm32-only");
#endif
}

ELI_TEST(test_hosted_alias_maps_to_system) {
#if defined(ELI_TEST_HOSTED) && !defined(ELI_LIBC_SYSTEM)
    ELI_FAIL("ELI_TEST_HOSTED did not alias ELI_LIBC_SYSTEM");
#endif
}

ELI_TEST(system_mode_provides_stdio) {
    char buf[16];
    int n = snprintf(buf, sizeof(buf), "%d", 42);
    ELI_ASSERT_EQ(n, 2);
    ELI_ASSERT_STR_EQ(buf, "42");
}

ELI_TEST(wasm_math_overrides_are_not_applied_natively) {
    /* The wasm pow() override is a macro; the real libm pow is exact here. */
    ELI_ASSERT_TRUE(pow(2.0, 10.0) == 1024.0);
#ifdef pow
    ELI_FAIL("pow is still a macro outside wasm32");
#endif
}

ELI_TEST(eli_assert_is_off_without_eli_debug) {
#ifndef ELI_DEBUG
    ELI_ASSERT(0); /* must compile to nothing */
#endif
}

ELI_TEST_MAIN()
