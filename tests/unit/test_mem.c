/**
 * @file test_mem.c
 * @brief Unit tests for the eli_mem.h memory management API (Phase 29).
 *
 * Covers: default alloc returns non-NULL usable memory and free works; alloc(0)
 * returns NULL; free(NULL) is a no-op; installing a custom allocator routes all
 * alloc/free calls through it with the correct user_data; and restoring defaults
 * with NULL after a custom allocator was installed.
 *
 * @status Phase 29 — all tests pass.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/util/eli_mem.h>

/* -------------------------------------------------------------------------
 * File-scope state for custom allocator tests
 * ---------------------------------------------------------------------- */

static int g_custom_alloc_call_count = 0;
static int g_custom_free_call_count  = 0;

/* A sentinel tag used as user_data so the custom functions can verify the
   pointer arrived intact. */
static int g_userdata_tag = 0xBEEF;

static void *counting_alloc(size_t sz, void *user_data)
{
    int *tag = (int *)user_data;
    if (tag != NULL && *tag == 0xBEEF)
        g_custom_alloc_call_count++;
    return malloc(sz);
}

static void counting_free(void *ptr, void *user_data)
{
    int *tag = (int *)user_data;
    if (tag != NULL && *tag == 0xBEEF)
        g_custom_free_call_count++;
    free(ptr);
}

/* Helper: reset custom counters and install the counting allocator. */
static void install_counting_allocator(void)
{
    g_custom_alloc_call_count = 0;
    g_custom_free_call_count  = 0;
    eli_set_allocator_functions(counting_alloc, counting_free, &g_userdata_tag);
}

/* -------------------------------------------------------------------------
 * Tests
 * ---------------------------------------------------------------------- */

ELI_TEST(mem_default_alloc_returns_usable_memory)
{
    /* Ensure defaults are active regardless of prior test order. */
    eli_set_allocator_functions(NULL, NULL, NULL);

    void *p = eli_mem_alloc(64);
    ELI_ASSERT_NOT_NULL(p);

    /* Write every byte then read back to confirm the memory is fully usable. */
    unsigned char *bytes = (unsigned char *)p;
    int i;
    for (i = 0; i < 64; i++)
        bytes[i] = (unsigned char)i;
    for (i = 0; i < 64; i++)
        ELI_ASSERT_EQ((int)bytes[i], i);

    eli_mem_free(p);
}

ELI_TEST(mem_alloc_zero_returns_null)
{
    eli_set_allocator_functions(NULL, NULL, NULL);
    void *p = eli_mem_alloc(0);
    ELI_ASSERT_NULL(p);
}

ELI_TEST(mem_free_null_is_noop)
{
    eli_set_allocator_functions(NULL, NULL, NULL);
    /* Must not crash or reach an assertion failure. */
    eli_mem_free(NULL);
    ELI_ASSERT_TRUE(1); /* Reaching this line proves no crash occurred. */
}

ELI_TEST(mem_custom_allocator_routes_alloc_and_free)
{
    install_counting_allocator();

    void *p = eli_mem_alloc(32);
    ELI_ASSERT_NOT_NULL(p);
    ELI_ASSERT_EQ(g_custom_alloc_call_count, 1);
    ELI_ASSERT_EQ(g_custom_free_call_count,  0);

    eli_mem_free(p);
    ELI_ASSERT_EQ(g_custom_alloc_call_count, 1);
    ELI_ASSERT_EQ(g_custom_free_call_count,  1);

    eli_set_allocator_functions(NULL, NULL, NULL);
}

ELI_TEST(mem_custom_allocator_receives_correct_user_data)
{
    /* The counting functions only increment their counters when they observe the
       0xBEEF tag, proving that user_data was forwarded intact. */
    install_counting_allocator();

    void *p = eli_mem_alloc(16);
    eli_mem_free(p);

    ELI_ASSERT_EQ(g_custom_alloc_call_count, 1);
    ELI_ASSERT_EQ(g_custom_free_call_count,  1);

    eli_set_allocator_functions(NULL, NULL, NULL);
}

ELI_TEST(mem_get_allocator_functions_reflects_current_state)
{
    install_counting_allocator();

    eli_alloc_func got_alloc    = NULL;
    eli_free_func  got_free     = NULL;
    void          *got_userdata = NULL;
    eli_get_allocator_functions(&got_alloc, &got_free, &got_userdata);

    ELI_ASSERT_TRUE(got_alloc    == counting_alloc);
    ELI_ASSERT_TRUE(got_free     == counting_free);
    ELI_ASSERT_TRUE(got_userdata == (void *)&g_userdata_tag);

    eli_set_allocator_functions(NULL, NULL, NULL);
}

ELI_TEST(mem_get_allocator_functions_accepts_null_out_pointers)
{
    install_counting_allocator();
    /* Must not crash when any out-pointer is NULL. */
    eli_get_allocator_functions(NULL, NULL, NULL);
    ELI_ASSERT_TRUE(1);
    eli_set_allocator_functions(NULL, NULL, NULL);
}

ELI_TEST(mem_restore_defaults_with_null)
{
    /* Install the counting allocator, then restore defaults. */
    install_counting_allocator();

    eli_alloc_func got_alloc = NULL;
    eli_get_allocator_functions(&got_alloc, NULL, NULL);
    ELI_ASSERT_TRUE(got_alloc == counting_alloc); /* Custom is active. */

    /* Restore defaults — both slots reset by passing NULL for both. */
    eli_set_allocator_functions(NULL, NULL, NULL);

    /* After restore the counters must NOT increase — default malloc/free is used. */
    g_custom_alloc_call_count = 0;
    g_custom_free_call_count  = 0;
    void *p = eli_mem_alloc(8);
    ELI_ASSERT_NOT_NULL(p);
    eli_mem_free(p);
    ELI_ASSERT_EQ(g_custom_alloc_call_count, 0);
    ELI_ASSERT_EQ(g_custom_free_call_count,  0);
}

ELI_TEST_MAIN()
