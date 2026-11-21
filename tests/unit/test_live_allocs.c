/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file test_live_allocs.c
 * @brief Live widgets make no elimgui heap allocation: none per frame once warm
 *        (layers, fallback, setters, pool overflow), and a full pool leaves the
 *        heap exactly where it was without live widgets, inside the budget.
 *
 * The elimgui side of Argus's test_eli_frame_allocs / test_eli_mem_budget gates
 * (argus_table_spec.md §13.14: 0 allocations per idle frame, heap <= 8 MB).
 *
 * @status Live widgets coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"
#include "eli_test_live.h"

#define WARM_FRAMES 100
#define IDLE_FRAMES 100
#define HEAP_BUDGET_BYTES (8u * 1024u * 1024u)

/* ---------------------------------------------------------------------------
 * Counting allocator (calls and live bytes)
 * ------------------------------------------------------------------------- */

static long g_calls = 0;
static long long g_live_bytes = 0;

typedef union { size_t size; max_align_t align; } alloc_header;

static void *count_alloc(size_t sz, void *user)
{
    (void)user;
    g_calls++;
    alloc_header *h = malloc(sizeof(*h) + sz);
    if (!h)
        return NULL;
    h->size = sz;
    g_live_bytes += (long long)sz;
    return h + 1;
}

static void count_free(void *p, void *user)
{
    (void)user;
    if (!p)
        return;
    alloc_header *h = (alloc_header *)p - 1;
    g_live_bytes -= (long long)h->size;
    free(h);
}

static eli_context *setup_counted(void)
{
    eli_set_allocator_functions(count_alloc, count_free, NULL);
    return test_ui_setup();
}

static void teardown_counted(eli_context *ctx)
{
    test_ui_teardown(ctx);
    eli_set_allocator_functions(NULL, NULL, NULL);
}

/* ---------------------------------------------------------------------------
 * UIs
 * ------------------------------------------------------------------------- */

static const eli_live_pulse k_pulse = { 1.4f, 0.3f };
static int g_widgets = 4;        /* live widgets submitted (rows of four kinds) */
static bool g_set_each_frame = false;
static int g_frame = 0;
static eli_id g_first_bar = 0;

static bool g_dummies = false;   /* the same layout with eli_dummy instead */

/** g_widgets live widgets, four per row (dot, spinner, progress, text). */
static void ui_rows(void)
{
    static const float k_w[4] = { 8.0f, 12.0f, 120.0f, 60.0f };
    test_live_begin("Live", eli_make_vec2(10, 10), eli_make_vec2(700, 560));
    for (int i = 0; i < g_widgets; i++) {
        if (i % 4)
            eli_same_line(0.0f, -1.0f);
        if (g_dummies) {
            float h[4] = { 8.0f, 12.0f, eli_get_frame_height(), eli_get_font_size() };
            eli_dummy(eli_make_vec2(k_w[i % 4], h[i % 4]));
            continue;
        }
        eli_push_id_int(i);
        switch (i % 4) {
        case 0: eli_live_dot("d", 4.0f, ELI_COL32(80, 200, 120, 255), &k_pulse); break;
        case 1: eli_live_spinner("s", 6.0f, ELI_COL32_WHITE); break;
        case 2: {
            eli_id bar = eli_live_progress("p", eli_make_vec2(k_w[2], 0), 0.5f);
            if (i == 2)
                g_first_bar = bar;
            if (g_set_each_frame)
                eli_live_set_fraction(bar, (float)(g_frame % 100) / 100.0f);
            break;
        }
        default: {
            char clock[16];
            snprintf(clock, sizeof(clock), "t+%04d", g_frame);
            eli_live_text("t", clock, k_w[3]);
            break;
        }
        }
        eli_pop_id();
    }
    eli_end();
}

/** Calls during IDLE_FRAMES frames after WARM_FRAMES. */
static long count_idle_calls(void)
{
    for (int i = 0; i < WARM_FRAMES; i++, g_frame++)
        test_ui_frame(ui_rows);
    long before = g_calls;
    for (int i = 0; i < IDLE_FRAMES; i++, g_frame++)
        test_ui_frame(ui_rows);
    return g_calls - before;
}

/* ---------------------------------------------------------------------------
 * Tests
 * ------------------------------------------------------------------------- */

ELI_TEST(layers_make_zero_allocations_per_frame)
{
    eli_context *ctx = setup_counted();
    test_live_install();
    g_widgets = 8;
    g_set_each_frame = true;
    long n = count_idle_calls();
    printf("  layers (text changing every frame): %ld allocations in %d frames\n", n, IDLE_FRAMES);
    ELI_ASSERT_EQ(n, 0L);
    ELI_ASSERT_EQ(eli_live_get_stats()->live, 8);

    long before = g_calls;   /* setters between frames */
    for (int i = 0; i < 100; i++)
        ELI_ASSERT_TRUE(eli_live_set_fraction(g_first_bar, (float)i / 100.0f));
    ELI_ASSERT_EQ(g_calls - before, 0L);
    g_set_each_frame = false;
    eli_live_set_backend(NULL);
    teardown_counted(ctx);
}

ELI_TEST(fallback_and_overflow_make_zero_allocations_per_frame)
{
    eli_context *ctx = setup_counted();
    eli_live_set_backend(NULL);
    g_widgets = 8;
    long n = count_idle_calls();
    printf("  fallback: %ld allocations in %d frames\n", n, IDLE_FRAMES);
    ELI_ASSERT_EQ(n, 0L);

    test_live_install();
    g_widgets = ELI_LIVE_MAX + 8;   /* 8 past the pool: drawn by the fallback */
    n = count_idle_calls();
    printf("  pool overflow: %ld allocations in %d frames\n", n, IDLE_FRAMES);
    ELI_ASSERT_EQ(n, 0L);
    ELI_ASSERT_EQ(eli_live_get_stats()->count, ELI_LIVE_MAX);
    ELI_ASSERT_GT(eli_live_get_stats()->overflow, 0u);
    eli_live_set_backend(NULL);
    teardown_counted(ctx);
}

ELI_TEST(full_pool_adds_nothing_to_the_heap_within_budget)
{
    eli_context *ctx = setup_counted();
    test_live_install();
    g_widgets = ELI_LIVE_MAX;
    g_dummies = true;
    count_idle_calls();
    long long without = g_live_bytes;
    g_dummies = false;
    long long peak = g_live_bytes;
    g_widgets = ELI_LIVE_MAX;
    for (int i = 0; i < WARM_FRAMES; i++, g_frame++) {
        test_ui_frame(ui_rows);
        peak = g_live_bytes > peak ? g_live_bytes : peak;
    }
    long long with = g_live_bytes;
    printf("  heap: %lld bytes without live widgets, %lld with %d (peak %lld); pool %zu bytes "
           "static\n", without, with, ELI_LIVE_MAX, peak, sizeof(g_eli_live));
    ELI_ASSERT_EQ(eli_live_get_stats()->live, ELI_LIVE_MAX);
    ELI_ASSERT_EQ(with, without);
    ELI_ASSERT_LE(peak, (long long)HEAP_BUDGET_BYTES);
    eli_live_set_backend(NULL);
    teardown_counted(ctx);
}

ELI_TEST_MAIN()
