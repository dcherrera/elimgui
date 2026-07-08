/**
 * @file test_id_storage.c
 * @brief Unit tests for eli_storage: int/float/ptr/bool round-trips, defaults
 *        for missing keys, overwrite-in-place, sorted insertion, set_all_int,
 *        and the current-context storage accessors.
 *
 * @status Phase 5 storage coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/id/eli_storage.h>

ELI_TEST(storage_defaults_for_missing_keys) {
    eli_storage st = {0};
    ELI_ASSERT_EQ(eli_storage_get_int(&st, 1u, -7), -7);
    ELI_ASSERT_FLT_NEAR(eli_storage_get_float(&st, 1u, 2.5f), 2.5f, 1e-6f);
    ELI_ASSERT_NULL(eli_storage_get_void_ptr(&st, 1u, NULL));
    ELI_ASSERT_TRUE(eli_storage_get_bool(&st, 1u, true));
    ELI_ASSERT_FALSE(eli_storage_get_bool(&st, 1u, false));
    eli_storage_clear(&st);
}

ELI_TEST(storage_int_roundtrip_and_overwrite) {
    eli_storage st = {0};
    eli_storage_set_int(&st, 100u, 42);
    ELI_ASSERT_EQ(eli_storage_get_int(&st, 100u, 0), 42);
    ELI_ASSERT_EQ(st.size, 1);

    /* Overwrite in place: value updates, no new entry. */
    eli_storage_set_int(&st, 100u, 99);
    ELI_ASSERT_EQ(eli_storage_get_int(&st, 100u, 0), 99);
    ELI_ASSERT_EQ(st.size, 1);
    eli_storage_clear(&st);
}

ELI_TEST(storage_float_and_ptr_and_bool_roundtrip) {
    eli_storage st = {0};
    int target = 0;

    eli_storage_set_float(&st, 10u, 3.14f);
    eli_storage_set_void_ptr(&st, 20u, &target);
    eli_storage_set_bool(&st, 30u, true);

    ELI_ASSERT_FLT_NEAR(eli_storage_get_float(&st, 10u, 0.0f), 3.14f, 1e-6f);
    ELI_ASSERT_EQ(eli_storage_get_void_ptr(&st, 20u, NULL), &target);
    ELI_ASSERT_TRUE(eli_storage_get_bool(&st, 30u, false));

    eli_storage_set_bool(&st, 30u, false);
    ELI_ASSERT_FALSE(eli_storage_get_bool(&st, 30u, true));
    eli_storage_clear(&st);
}

ELI_TEST(storage_keeps_keys_sorted_and_retrievable) {
    eli_storage st = {0};
    /* Insert out of order; every key must remain retrievable. */
    eli_id keys[] = {50u, 10u, 40u, 20u, 30u, 5u};
    for (int i = 0; i < 6; i++)
        eli_storage_set_int(&st, keys[i], (int)keys[i] * 2);

    ELI_ASSERT_EQ(st.size, 6);
    for (int i = 0; i < 6; i++)
        ELI_ASSERT_EQ(eli_storage_get_int(&st, keys[i], -1), (int)keys[i] * 2);

    /* Backing array is sorted ascending by key. */
    for (int i = 1; i < st.size; i++)
        ELI_ASSERT_LT(st.data[i - 1].key, st.data[i].key);

    eli_storage_clear(&st);
}

ELI_TEST(storage_set_all_int) {
    eli_storage st = {0};
    eli_storage_set_int(&st, 1u, 1);
    eli_storage_set_int(&st, 2u, 2);
    eli_storage_set_int(&st, 3u, 3);

    eli_storage_set_all_int(&st, 0);
    ELI_ASSERT_EQ(eli_storage_get_int(&st, 1u, -1), 0);
    ELI_ASSERT_EQ(eli_storage_get_int(&st, 2u, -1), 0);
    ELI_ASSERT_EQ(eli_storage_get_int(&st, 3u, -1), 0);
    eli_storage_clear(&st);
}

ELI_TEST(storage_clear_resets_and_frees) {
    eli_storage st = {0};
    for (eli_id k = 0; k < 20; k++)
        eli_storage_set_int(&st, k, (int)k);
    ELI_ASSERT_EQ(st.size, 20);
    ELI_ASSERT_GE(st.capacity, 20);

    eli_storage_clear(&st);
    ELI_ASSERT_EQ(st.size, 0);
    ELI_ASSERT_EQ(st.capacity, 0);
    ELI_ASSERT_NULL(st.data);
    /* Reusable after clear. */
    eli_storage_set_int(&st, 1u, 5);
    ELI_ASSERT_EQ(eli_storage_get_int(&st, 1u, 0), 5);
    eli_storage_clear(&st);
}

ELI_TEST(storage_current_context_accessors) {
    eli_context *ctx = eli_create_context();
    ELI_ASSERT_NULL(eli_get_state_storage());

    eli_storage st = {0};
    eli_set_state_storage(&st);
    ELI_ASSERT_EQ(eli_get_state_storage(), &st);

    eli_set_state_storage(NULL);
    ELI_ASSERT_NULL(eli_get_state_storage());

    eli_storage_clear(&st);
    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
