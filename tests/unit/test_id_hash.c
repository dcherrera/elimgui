/**
 * @file test_id_hash.c
 * @brief Unit tests for elimgui CRC32 identity hashing: determinism, seed
 *        dependence, snapshot values that lock the algorithm, and the
 *        Dear ImGui "##"/"###" label semantics.
 *
 * @status Phase 5 hash coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/id/eli_id_hash.h>

/* Snapshot values captured from the reference CRC32 (poly 0xEDB88320). These
 * lock the algorithm so accidental drift is caught, and they match Dear ImGui. */
ELI_TEST(hash_snapshot_values_are_stable) {
    ELI_ASSERT_EQ(eli_hash_str("hello", 0),  0x3610A686u);
    ELI_ASSERT_EQ(eli_hash_str("Button", 0), 0x3DAAA90Bu);
}

ELI_TEST(hash_is_deterministic) {
    ELI_ASSERT_EQ(eli_hash_str("widget", 0), eli_hash_str("widget", 0));
    ELI_ASSERT_EQ(eli_hash_str("widget", 99), eli_hash_str("widget", 99));

    /* Sized hash of the same bytes equals the NUL-terminated hash. */
    const char *s = "widget";
    ELI_ASSERT_EQ(eli_hash_data(s, 6, 0), eli_hash_str("widget", 0));
}

ELI_TEST(hash_depends_on_seed) {
    eli_id a = eli_hash_str("Button", 0);
    eli_id b = eli_hash_str("Button", 42);
    ELI_ASSERT_NE(a, b);
    ELI_ASSERT_EQ(b, 0x9B9D8F5Bu);
}

ELI_TEST(hash_double_hash_keeps_distinct_ids) {
    /* "##" is NOT special to the hash: full string is hashed, so a "##x" suffix
     * yields a different id than the bare label. */
    eli_id label = eli_hash_str("Label", 0);
    eli_id label_hidden = eli_hash_str("Label##x", 0);
    ELI_ASSERT_NE(label, label_hidden);
}

ELI_TEST(hash_triple_hash_resets_to_seed) {
    /* "###" discards everything before it, so ids sharing the "###" tail match
     * regardless of the visible prefix -> stable ids across label changes. */
    eli_id a = eli_hash_str("A###X", 0);
    eli_id b = eli_hash_str("B###X", 0);
    eli_id bare = eli_hash_str("###X", 0);
    ELI_ASSERT_EQ(a, b);
    ELI_ASSERT_EQ(a, bare);
    ELI_ASSERT_EQ(a, 0x72D189D2u);

    /* A different tail after "###" still changes the id. */
    ELI_ASSERT_NE(eli_hash_str("A###X", 0), eli_hash_str("A###Y", 0));
}

ELI_TEST(hash_empty_string_differs_by_seed) {
    ELI_ASSERT_NE(eli_hash_str("", 0), eli_hash_str("", 1));
    ELI_ASSERT_EQ(eli_hash_str("", 0), eli_hash_data("", 0, 0));
}

ELI_TEST_MAIN()
