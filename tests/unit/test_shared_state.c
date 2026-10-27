/**
 * @file test_shared_state.c
 * @brief Multi-TU state test (D4): context, clipboard and allocator across two TUs.
 *
 * Linked with tests/unit/companion/test_shared_state_b.c. Under ELI_SHARED_STATE the
 * second TU must see this TU's context, clipboard text and allocator hooks. In the
 * default static mode each TU has its own copy; the test then asserts that (so the
 * hosted pass documents the single-TU limitation instead of silently passing).
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/elimgui.h>

eli_context *shared_state_b_current_context(void);
void shared_state_b_set_clipboard(const char *text);
const char *shared_state_b_get_clipboard(void);
void shared_state_b_alloc_and_free(void);

static int g_count_allocs = 0;

static void *counting_alloc(size_t sz, void *user_data)
{
    (void)user_data;
    g_count_allocs++;
    return malloc(sz);
}

static void counting_free(void *ptr, void *user_data)
{
    (void)user_data;
    free(ptr);
}

#ifdef ELI_SHARED_STATE
#define EXPECT_SHARED 1
#else
#define EXPECT_SHARED 0
#endif

ELI_TEST(context_is_shared_across_tus) {
    eli_context *ctx = eli_create_context();
    ELI_ASSERT_NOT_NULL(ctx);
    if (EXPECT_SHARED)
        ELI_ASSERT_TRUE(shared_state_b_current_context() == ctx);
    else
        ELI_ASSERT_NULL(shared_state_b_current_context());
    eli_destroy_context(ctx);
}

ELI_TEST(clipboard_fallback_is_shared_across_tus) {
    eli_set_clipboard_text("from-a");
    if (EXPECT_SHARED) {
        ELI_ASSERT_STR_EQ(shared_state_b_get_clipboard(), "from-a");
        shared_state_b_set_clipboard("from-b");
        ELI_ASSERT_STR_EQ(eli_get_clipboard_text(), "from-b");
    } else {
        ELI_ASSERT_STR_EQ(shared_state_b_get_clipboard(), "");
    }
}

ELI_TEST(allocator_hooks_are_shared_across_tus) {
    eli_set_allocator_functions(counting_alloc, counting_free, NULL);
    g_count_allocs = 0;
    shared_state_b_alloc_and_free();
    ELI_ASSERT_EQ(g_count_allocs, EXPECT_SHARED ? 1 : 0);
    eli_set_allocator_functions(NULL, NULL, NULL);
}

ELI_TEST_MAIN()
