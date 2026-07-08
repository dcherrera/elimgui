/**
 * @file eli_test.h
 * @brief Minimal host-libc unit-test framework for elimgui.
 *
 * Uses the HOST system libc (stdio/stdlib/string/math) — deliberately NOT jaclibc —
 * so tests compile and run as native binaries on any dev machine, independent of
 * jaclibc's per-OS native support. Tests build elimgui with -DELI_TEST_HOSTED, which
 * routes the library's libc includes to the host (see include/eli/core/eli_platform.h).
 *
 * Tests self-register via __attribute__((constructor)); ELI_TEST_MAIN() runs them all
 * and returns non-zero if any assertion failed (so CI / run_tests.sh can gate on it).
 *
 * @status Framework in use for per-phase unit tests.
 * @issues None
 * @todo None
 */
#ifndef ELI_TEST_H
#define ELI_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef ELI_TEST_MAX
#define ELI_TEST_MAX 4096
#endif

typedef void (*eli_test_fn)(void);

typedef struct eli_test_entry {
    const char *name;
    eli_test_fn fn;
} eli_test_entry;

static eli_test_entry eli_test_registry[ELI_TEST_MAX];
static int  eli_test_count = 0;
static int  eli_test_failed_tests = 0;
static int  eli_test_assert_failures = 0;
static int  eli_test_current_failed = 0;
static const char *eli_test_current_name = "";

/** Register one test function. Called automatically by the ELI_TEST macro. */
static inline void eli_test_register(const char *name, eli_test_fn fn)
{
    if (eli_test_count < ELI_TEST_MAX) {
        eli_test_registry[eli_test_count].name = name;
        eli_test_registry[eli_test_count].fn = fn;
        eli_test_count++;
    }
}

/* Define a test and auto-register it before main() runs. */
#define ELI_TEST(test_name)                                                    \
    static void test_name(void);                                               \
    __attribute__((constructor))                                              \
    static void eli_test_ctor_##test_name(void)                                \
    {                                                                          \
        eli_test_register(#test_name, test_name);                              \
    }                                                                          \
    static void test_name(void)

/* Record an assertion failure in the current test. */
#define ELI_FAIL(msg)                                                          \
    do {                                                                       \
        eli_test_current_failed = 1;                                           \
        eli_test_assert_failures++;                                            \
        fprintf(stderr, "    FAIL: %s  (%s:%d)\n", (msg), __FILE__, __LINE__); \
    } while (0)

#define ELI_ASSERT_TRUE(x)        do { if (!(x))            ELI_FAIL("ASSERT_TRUE("  #x ")");        } while (0)
#define ELI_ASSERT_FALSE(x)       do { if  (x)             ELI_FAIL("ASSERT_FALSE(" #x ")");        } while (0)
#define ELI_ASSERT_EQ(a, b)       do { if ((a) != (b))      ELI_FAIL("ASSERT_EQ("    #a ", " #b ")"); } while (0)
#define ELI_ASSERT_NE(a, b)       do { if ((a) == (b))      ELI_FAIL("ASSERT_NE("    #a ", " #b ")"); } while (0)
#define ELI_ASSERT_GT(a, b)       do { if (!((a) >  (b)))   ELI_FAIL("ASSERT_GT("    #a ", " #b ")"); } while (0)
#define ELI_ASSERT_LT(a, b)       do { if (!((a) <  (b)))   ELI_FAIL("ASSERT_LT("    #a ", " #b ")"); } while (0)
#define ELI_ASSERT_GE(a, b)       do { if (!((a) >= (b)))   ELI_FAIL("ASSERT_GE("    #a ", " #b ")"); } while (0)
#define ELI_ASSERT_LE(a, b)       do { if (!((a) <= (b)))   ELI_FAIL("ASSERT_LE("    #a ", " #b ")"); } while (0)
#define ELI_ASSERT_NULL(p)        do { if ((p) != NULL)     ELI_FAIL("ASSERT_NULL("  #p ")");        } while (0)
#define ELI_ASSERT_NOT_NULL(p)    do { if ((p) == NULL)     ELI_FAIL("ASSERT_NOT_NULL(" #p ")");     } while (0)
#define ELI_ASSERT_STR_EQ(a, b)   do { if (strcmp((a),(b)) != 0) ELI_FAIL("ASSERT_STR_EQ(" #a ", " #b ")"); } while (0)
#define ELI_ASSERT_FLT_NEAR(a, b, tol)                                         \
    do { if (fabs((double)(a) - (double)(b)) > (double)(tol))                  \
             ELI_FAIL("ASSERT_FLT_NEAR(" #a ", " #b ")"); } while (0)

/** Run every registered test; print a summary; return 1 if any failed, else 0. */
static inline int eli_test_run_all(void)
{
    /* Unbuffered so stdout pass/fail lines interleave correctly with stderr FAILs. */
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Running %d test(s)...\n", eli_test_count);

    for (int i = 0; i < eli_test_count; i++) {
        eli_test_current_name = eli_test_registry[i].name;
        eli_test_current_failed = 0;
        eli_test_registry[i].fn();
        if (eli_test_current_failed) {
            eli_test_failed_tests++;
            printf("  [FAIL] %s\n", eli_test_current_name);
        } else {
            printf("  [pass] %s\n", eli_test_current_name);
        }
    }

    printf("\n%d/%d test(s) passed", eli_test_count - eli_test_failed_tests, eli_test_count);
    if (eli_test_assert_failures)
        printf("  (%d assertion failure(s))", eli_test_assert_failures);
    printf("\n");

    return eli_test_failed_tests ? 1 : 0;
}

#define ELI_TEST_MAIN() int main(void) { return eli_test_run_all(); }

#endif /* ELI_TEST_H */
