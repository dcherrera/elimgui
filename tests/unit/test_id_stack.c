/**
 * @file test_id_stack.c
 * @brief Unit tests for the elimgui ID stack: seed derivation, push/pop effects
 *        on derived ids, nested-scope distinctness, and the ptr/int/str variants.
 *
 * @status Phase 5 ID stack coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/id/eli_id_stack.h>

ELI_TEST(stack_root_seed_is_zero) {
    eli_context *ctx = eli_create_context();
    ELI_ASSERT_EQ(ctx->id_stack_size, 0);
    /* With an empty stack the seed is 0, so get_id == hash_str(seed 0). */
    ELI_ASSERT_EQ(eli_get_id("root"), eli_hash_str("root", 0));
    eli_destroy_context(ctx);
}

ELI_TEST(stack_push_changes_derived_id) {
    eli_context *ctx = eli_create_context();

    eli_id before = eli_get_id("child");
    eli_push_id("parent");
    ELI_ASSERT_EQ(ctx->id_stack_size, 1);
    eli_id after = eli_get_id("child");

    /* Same label resolves differently once a scope is pushed. */
    ELI_ASSERT_NE(before, after);
    /* The pushed seed is hash of "parent" at seed 0; derived id honors it. */
    ELI_ASSERT_EQ(ctx->id_stack[0], eli_hash_str("parent", 0));
    ELI_ASSERT_EQ(after, eli_hash_str("child", eli_hash_str("parent", 0)));

    eli_pop_id();
    ELI_ASSERT_EQ(ctx->id_stack_size, 0);
    /* Popping restores the original derivation. */
    ELI_ASSERT_EQ(eli_get_id("child"), before);

    eli_destroy_context(ctx);
}

ELI_TEST(stack_nested_push_is_distinct) {
    eli_context *ctx = eli_create_context();

    eli_push_id("a");
    eli_id lvl1 = eli_get_id("leaf");
    eli_push_id("b");
    eli_id lvl2 = eli_get_id("leaf");
    ELI_ASSERT_EQ(ctx->id_stack_size, 2);

    /* Same leaf label under different nesting paths -> distinct ids. */
    ELI_ASSERT_NE(lvl1, lvl2);

    eli_pop_id();
    ELI_ASSERT_EQ(eli_get_id("leaf"), lvl1);
    eli_pop_id();

    eli_destroy_context(ctx);
}

ELI_TEST(stack_int_and_ptr_variants) {
    eli_context *ctx = eli_create_context();

    /* Distinct integers under the same seed produce distinct ids. */
    ELI_ASSERT_NE(eli_get_id_int(1), eli_get_id_int(2));
    ELI_ASSERT_EQ(eli_get_id_int(7), eli_get_id_int(7));

    int a = 0, b = 0;
    ELI_ASSERT_NE(eli_get_id_ptr(&a), eli_get_id_ptr(&b));
    ELI_ASSERT_EQ(eli_get_id_ptr(&a), eli_get_id_ptr(&a));

    /* push_id_int nests: seed becomes hash of the int against seed 0. */
    int five = 5;
    eli_push_id_int(5);
    ELI_ASSERT_EQ(ctx->id_stack[0], eli_hash_data(&five, sizeof(five), 0u));
    eli_pop_id();

    eli_destroy_context(ctx);
}

ELI_TEST(stack_str_range_matches_cstr) {
    eli_context *ctx = eli_create_context();
    const char *buf = "HelloWorld";
    /* Hashing the first 5 bytes as a range equals hashing "Hello". */
    ELI_ASSERT_EQ(eli_get_id_str(buf, buf + 5), eli_get_id("Hello"));
    eli_destroy_context(ctx);
}

ELI_TEST(stack_pop_on_empty_is_safe) {
    eli_context *ctx = eli_create_context();
    eli_pop_id();  /* must not underflow */
    ELI_ASSERT_EQ(ctx->id_stack_size, 0);
    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
