/**
 * @file eli_platform.h
 * @brief Libc dependency seam for elimgui.
 *
 * Every elimgui header includes this instead of a libc header directly, so the
 * library's dependency on a C runtime lives behind a single, swappable boundary
 * (library-first design). Two libc modes, picked by macro:
 *
 *   - ELI_LIBC_SYSTEM: the host system libc (stdint ... float, stdio). Default for
 *     every non-wasm32 target (native apps, unit tests). ELI_TEST_HOSTED is kept as
 *     a back-compat alias for it.
 *   - ELI_LIBC_JACLIBC: JAClibc. Default for wasm32. On wasm32 it also turns on
 *     ELI_JSIO (C-to-JS interop) and the double-only pow/log/exp overrides. On a
 *     native target it is opt-in and JS interop stays off.
 *
 * Defining both is an error. Use ELI_JSIO to guard JS-interop-only code:
 *     #ifdef ELI_JSIO
 *         JS_EXPORT(on_mouse_move) void on_mouse_move(float x, float y) { ... }
 *     #endif
 *
 * Also pulls in core/eli_state.h (ELI_GLOBAL, multi-TU shared state).
 *
 * @status In use as the single libc include point for all elimgui headers.
 * @issues None
 * @todo None
 */
#ifndef ELI_PLATFORM_H
#define ELI_PLATFORM_H

#if defined(ELI_TEST_HOSTED) && !defined(ELI_LIBC_SYSTEM)
    #define ELI_LIBC_SYSTEM 1                   /* back-compat alias */
#endif
#if !defined(ELI_LIBC_SYSTEM) && !defined(ELI_LIBC_JACLIBC)
    #if defined(__wasm32__)
        #define ELI_LIBC_JACLIBC 1
    #else
        #define ELI_LIBC_SYSTEM 1               /* native default */
    #endif
#endif
#if defined(ELI_LIBC_SYSTEM) && defined(ELI_LIBC_JACLIBC)
    #error "elimgui: define only one of ELI_LIBC_SYSTEM / ELI_LIBC_JACLIBC"
#endif

#if defined(ELI_LIBC_SYSTEM)

    /* Host system libc. No JS interop. */
    #include <stdint.h>
    #include <stddef.h>
    #include <stdbool.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <math.h>
    #include <float.h>

#else

    /* JAClibc. JS interop and the soft-float workarounds apply only on wasm32. */
    #include <jaclibc.h>
#endif

/* Library invariant checks: on only with ELI_DEBUG and the system libc. Apps may
 * define ELI_ASSERT themselves before including elimgui. */
#ifndef ELI_ASSERT
    #if defined(ELI_DEBUG) && defined(ELI_LIBC_SYSTEM)
        #include <assert.h>
        #define ELI_ASSERT(x) assert(x)
    #else
        #define ELI_ASSERT(x) ((void)0)
    #endif
#endif

#if defined(ELI_LIBC_JACLIBC) && defined(__wasm32__)
    #define ELI_JSIO 1

    /* ----------------------------------------------------------------------
     * Double-only pow() for the wasm build.
     *
     * JAClibc implements pow()/powf() with 128-bit `long double`, which needs
     * soft-float compiler-rt builtins (__multf3, __addtf3, __floatsitf, ...)
     * that the `-nostdlib` wasm toolchain does not provide, so any use of a
     * logarithmic slider or font gamma fails to link. This self-contained
     * double-precision implementation keeps the library linkable. The macros
     * rewrite only call sites (JAClibc's inline definitions were already parsed
     * above and become unused / dead-code-eliminated). Not applied to hosted
     * test builds, which use the real libm.
     * -------------------------------------------------------------------- */
    static inline double eli__platform_log(double x) {   /* natural log, x > 0 */
        if (x <= 0.0) return -708.0;                     /* domain guard (~log of tiny) */
        union { double d; unsigned long long u; } v;
        v.d = x;
        int e = (int)((v.u >> 52) & 0x7FF) - 1023;       /* unbiased exponent */
        v.u = (v.u & ~(0x7FFULL << 52)) | (1023ULL << 52); /* mantissa in [1,2) */
        double m = v.d;
        double s = (m - 1.0) / (m + 1.0);                /* atanh series arg */
        double s2 = s * s, term = s, sum = 0.0;
        for (int k = 1; k <= 15; k += 2) { sum += term / (double)k; term *= s2; }
        return (double)e * 0.69314718055994530942 + 2.0 * sum;
    }

    static inline double eli__platform_exp(double x) {   /* e^x */
        double kf = x * 1.44269504088896340736;          /* x / ln2 */
        long k = (long)kf;
        if ((double)k > kf) k--;                          /* floor */
        double r = x - (double)k * 0.69314718055994530942;
        double term = 1.0, sum = 1.0;                     /* Taylor, r in [0,ln2) */
        for (int n = 1; n <= 16; n++) { term *= r / (double)n; sum += term; }
        long ex = k + 1023;                               /* build 2^k */
        if (ex <= 0) return 0.0;
        if (ex >= 2047) ex = 2046;
        union { double d; unsigned long long u; } v;
        v.u = ((unsigned long long)ex) << 52;
        return sum * v.d;
    }

    static inline double eli__platform_pow(double b, double e) {
        if (e == 0.0) return 1.0;
        if (b == 0.0) return 0.0;
        double base = b;
        int negate = 0;
        if (b < 0.0) {                                    /* only real for integer e */
            base = -b;
            long ei = (long)e;
            if ((double)ei == e && (ei & 1L)) negate = 1;
        }
        double r = eli__platform_exp(e * eli__platform_log(base));
        return negate ? -r : r;
    }

    #define pow(b, e)  eli__platform_pow((double)(b), (double)(e))
    #define powf(b, e) ((float)eli__platform_pow((double)(b), (double)(e)))
    #define log(x)     eli__platform_log((double)(x))
    #define logf(x)    ((float)eli__platform_log((double)(x)))
    #define exp(x)     eli__platform_exp((double)(x))
    #define expf(x)    ((float)eli__platform_exp((double)(x)))

#endif /* ELI_LIBC_JACLIBC && __wasm32__ */

#include "eli_state.h"

#endif /* ELI_PLATFORM_H */
