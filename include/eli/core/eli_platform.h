/**
 * @file eli_platform.h
 * @brief Libc dependency seam for elimgui.
 *
 * Every elimgui header includes this instead of <jaclibc.h> directly, so the
 * library's dependency on a C runtime lives behind a single, swappable boundary
 * (library-first design). Two modes:
 *
 *   - Production (default): pulls in JAClibc, which provides the standard libc
 *     surface AND the C-to-JS interop (jsio.h) used by the WASM/browser backend.
 *   - Hosted tests (-DELI_TEST_HOSTED): pulls in the host system libc instead, so
 *     unit tests compile and run as native binaries with real stdio/stdlib/math,
 *     independent of JAClibc's per-OS native support. JS-interop code paths are
 *     excluded from hosted builds (they are browser-only and covered by the demo).
 *
 * Use ELI_JSIO to guard JS-interop-only code so it compiles out under tests:
 *     #ifdef ELI_JSIO
 *         JS_EXPORT(on_mouse_move) void on_mouse_move(float x, float y) { ... }
 *     #endif
 *
 * @status In use as the single libc include point for all elimgui headers.
 * @issues None
 * @todo None
 */
#ifndef ELI_PLATFORM_H
#define ELI_PLATFORM_H

#ifdef ELI_TEST_HOSTED

    /* Native unit-test build: use the host system libc. No JS interop. */
    #include <stdint.h>
    #include <stddef.h>
    #include <stdbool.h>
    #include <stdlib.h>
    #include <string.h>
    #include <math.h>
    #include <float.h>

#else

    /* Production build: JAClibc (libc + jsio.h JS interop) for wasm32. */
    #include <jaclibc.h>
    #define ELI_JSIO 1

#endif /* ELI_TEST_HOSTED */

#endif /* ELI_PLATFORM_H */
