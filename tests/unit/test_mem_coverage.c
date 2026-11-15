/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file test_mem_coverage.c
 * @brief Proves every elimgui heap allocation goes through the eli_mem seam.
 *
 * Before elimgui is included, malloc/calloc/realloc/free are redefined as
 * macros that count each call: any library code that calls libc directly
 * instead of eli_mem_* is counted as a bypass. A tracking allocator installed
 * with eli_set_allocator_functions() records every pointer it hands out, so
 * the tests can also check that each category (context, font atlas, window,
 * storage, draw list, table, tab bar, plot, docking, drag-drop, logging,
 * settings)
 * allocates through it, and that destroying the context returns every block
 * (no leaks, no block freed by the wrong allocator).
 *
 * @status Allocator seam coverage tests.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <stdbool.h>

/* ---------------------------------------------------------------------------
 * Tracking allocator (uses the real libc; defined before the redirect below)
 * ------------------------------------------------------------------------- */

#define AUDIT_SLOTS (1 << 16)

static void *g_audit_live[AUDIT_SLOTS];   /* open-addressing set of live blocks */
static int g_audit_live_count = 0;
static int g_audit_alloc_count = 0;
static int g_audit_foreign_frees = 0;     /* seam frees of blocks the seam never gave out */
static int g_audit_bypass_calls = 0;      /* direct libc calls from library code */

static size_t audit_slot_of(const void *p)
{
    return ((size_t)p >> 4) * 2654435761u & (AUDIT_SLOTS - 1);
}

static void audit_set_insert(void *p)
{
    size_t i = audit_slot_of(p);
    while (g_audit_live[i] != NULL)
        i = (i + 1) & (AUDIT_SLOTS - 1);
    g_audit_live[i] = p;
    g_audit_live_count++;
}

/** Remove p from the live set (backward-shift delete). @return false if absent. */
static bool audit_set_remove(void *p)
{
    size_t i = audit_slot_of(p);
    while (g_audit_live[i] != p) {
        if (g_audit_live[i] == NULL)
            return false;
        i = (i + 1) & (AUDIT_SLOTS - 1);
    }
    g_audit_live[i] = NULL;
    for (size_t j = (i + 1) & (AUDIT_SLOTS - 1); g_audit_live[j] != NULL;
         j = (j + 1) & (AUDIT_SLOTS - 1)) {
        void *moved = g_audit_live[j];
        g_audit_live[j] = NULL;
        size_t k = audit_slot_of(moved);
        while (g_audit_live[k] != NULL)
            k = (k + 1) & (AUDIT_SLOTS - 1);
        g_audit_live[k] = moved;
    }
    g_audit_live_count--;
    return true;
}

static void *audit_alloc(size_t sz, void *user_data)
{
    (void)user_data;
    void *p = malloc(sz);
    if (p != NULL) {
        audit_set_insert(p);
        g_audit_alloc_count++;
    }
    return p;
}

static void audit_free(void *ptr, void *user_data)
{
    (void)user_data;
    if (ptr == NULL)
        return;
    if (!audit_set_remove(ptr))
        g_audit_foreign_frees++;
    free(ptr);
}

/* Bypass detectors: library code reaching libc directly lands here. Marked unused
 * because a fully routed library leaves some of them with no callers at all. */
__attribute__((unused)) static void *audit_bypass_malloc(size_t n)
{
    g_audit_bypass_calls++;
    return malloc(n);
}

__attribute__((unused)) static void *audit_bypass_calloc(size_t n, size_t s)
{
    g_audit_bypass_calls++;
    return calloc(n, s);
}

__attribute__((unused)) static void *audit_bypass_realloc(void *p, size_t n)
{
    g_audit_bypass_calls++;
    if (p != NULL && audit_set_remove(p))
        g_audit_foreign_frees++;  /* a seam block resized behind the seam's back */
    return realloc(p, n);
}

__attribute__((unused)) static void audit_bypass_free(void *p)
{
    if (p == NULL)
        return;
    g_audit_bypass_calls++;
    if (audit_set_remove(p))
        g_audit_foreign_frees++;  /* a seam block released behind the seam's back */
    free(p);
}

#define malloc(n)     audit_bypass_malloc(n)
#define calloc(n, s)  audit_bypass_calloc(n, s)
#define realloc(p, n) audit_bypass_realloc(p, n)
#define free(p)       audit_bypass_free(p)

#include "eli_test_ui.h"

/* ---------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------- */

static void audit_reset(void)
{
    memset(g_audit_live, 0, sizeof(g_audit_live));
    g_audit_live_count = 0;
    g_audit_alloc_count = 0;
    g_audit_foreign_frees = 0;
    g_audit_bypass_calls = 0;
    eli_set_allocator_functions(audit_alloc, audit_free, NULL);
}

/** Context + font under the tracking allocator. */
static eli_context *audit_setup(void)
{
    audit_reset();
    return test_ui_setup();
}

/** Tear down and assert nothing bypassed the seam or leaked. */
#define AUDIT_TEARDOWN_AND_CHECK(ctx)                     \
    do {                                                  \
        test_ui_teardown(ctx);                            \
        eli_set_allocator_functions(NULL, NULL, NULL);    \
        ELI_ASSERT_EQ(g_audit_bypass_calls, 0);           \
        ELI_ASSERT_EQ(g_audit_foreign_frees, 0);          \
        ELI_ASSERT_EQ(g_audit_live_count, 0);             \
    } while (0)

/* ---------------------------------------------------------------------------
 * Per-category workloads
 * ------------------------------------------------------------------------- */

static void ui_many_windows(void)
{
    char name[32];
    for (int i = 0; i < 40; i++) {
        snprintf(name, sizeof(name), "Window %d", i);
        eli_set_next_window_pos(eli_make_vec2((float)(i * 7), (float)(i * 5)), ELI_COND_ONCE,
                                eli_make_vec2(0.0f, 0.0f));
        eli_set_next_window_size(eli_make_vec2(120.0f, 90.0f), ELI_COND_ONCE);
        eli_begin(name, NULL, 0);
        if (eli_begin_child("pane", eli_make_vec2(60.0f, 40.0f), 0, 0))
            eli_text("child %d", i);
        eli_end_child();
        eli_end();
    }
}

static void ui_table(void)
{
    eli_begin("Tables", NULL, 0);
    if (eli_begin_table("Grid", 3, ELI_TABLE_SORTABLE | ELI_TABLE_BORDERS)) {
        for (int r = 0; r < 20; r++) {
            eli_table_next_row();
            for (int c = 0; c < 3; c++) {
                eli_table_next_column();
                eli_text("%d,%d", r, c);
            }
        }
        eli_end_table();
    }
    eli_end();
}

static void ui_tabs(void)
{
    char label[16];
    eli_begin("Tabs", NULL, 0);
    if (eli_begin_tab_bar("Bar", 0)) {
        for (int i = 0; i < 12; i++) {
            snprintf(label, sizeof(label), "Tab %d", i);
            if (eli_begin_tab_item(label, NULL, 0))
                eli_end_tab_item();
        }
        eli_end_tab_bar();
    }
    eli_end();
}

static void ui_plot(void)
{
    static float vals[300];
    for (int i = 0; i < 300; i++)
        vals[i] = (float)(i % 17);
    eli_begin("Plot", NULL, 0);
    eli_plot_lines("##lines", vals, 300, 0, NULL, ELI_PLOT_SCALE_AUTO, ELI_PLOT_SCALE_AUTO,
                   eli_make_vec2(200.0f, 100.0f), 0);
    eli_end();
}

static const eli_id AUDIT_DOCK_ID = 0x00A0D0C5u;

static void ui_dock(void)
{
    eli_set_next_window_pos(eli_make_vec2(10.0f, 10.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(700.0f, 500.0f), 0);
    eli_begin("Host", NULL, ELI_WINDOW_NO_DOCKING);
    eli_dock_space(AUDIT_DOCK_ID, eli_make_vec2(600.0f, 400.0f), 0);
    eli_end();
    eli_set_next_window_dock_id(AUDIT_DOCK_ID, 0);
    eli_begin("Docked A", NULL, 0);
    eli_end();
    eli_set_next_window_dock_id(AUDIT_DOCK_ID, 0);
    eli_begin("Docked B", NULL, 0);
    eli_end();
}

static void ui_overlays(void)
{
    eli_draw_list_add_rect_filled(eli_get_background_draw_list(), eli_make_vec2(0, 0),
                                  eli_make_vec2(10, 10), ELI_COL32_WHITE, 0.0f, ELI_DRAW_NONE);
    eli_draw_list_add_rect_filled(eli_get_foreground_draw_list(), eli_make_vec2(0, 0),
                                  eli_make_vec2(10, 10), ELI_COL32_WHITE, 0.0f, ELI_DRAW_NONE);
}

static void ui_drag_source(void)
{
    eli_begin("Drag", NULL, 0);
    eli_button("src");
    if (eli_begin_drag_drop_source(ELI_DRAG_DROP_SOURCE_ALLOW_NULL_ID)) {
        int value = 7;
        eli_set_drag_drop_payload("INT", &value, sizeof(value), ELI_COND_ALWAYS);
        eli_end_drag_drop_source();
    }
    eli_end();
}

/* Run `ui` for a few frames and return how many seam allocations it made. */
static int audit_allocs_during(void (*ui)(void), int frames)
{
    int before = g_audit_alloc_count;
    test_ui_frames(ui, frames);
    return g_audit_alloc_count - before;
}

/* ---------------------------------------------------------------------------
 * Tests
 * ------------------------------------------------------------------------- */

ELI_TEST(mem_coverage_context_and_font)
{
    eli_context *ctx = audit_setup();
    ELI_ASSERT_GT(g_audit_alloc_count, 2);   /* context + atlas + font + pixels */
    test_ui_frames(NULL, 2);
    AUDIT_TEARDOWN_AND_CHECK(ctx);
}

ELI_TEST(mem_coverage_windows)
{
    eli_context *ctx = audit_setup();
    ELI_ASSERT_GT(audit_allocs_during(ui_many_windows, 3), 80);  /* window + name per window */
    AUDIT_TEARDOWN_AND_CHECK(ctx);
}

ELI_TEST(mem_coverage_storage)
{
    eli_context *ctx = audit_setup();
    eli_storage st;
    memset(&st, 0, sizeof(st));
    int before = g_audit_alloc_count;
    for (int i = 0; i < 100; i++)
        eli_storage_set_int(&st, (eli_id)(i * 977u + 1u), i);
    ELI_ASSERT_GT(g_audit_alloc_count - before, 0);
    ELI_ASSERT_EQ(eli_storage_get_int(&st, 977u * 42u + 1u, -1), 42);
    eli_storage_clear(&st);
    AUDIT_TEARDOWN_AND_CHECK(ctx);
}

ELI_TEST(mem_coverage_draw_list)
{
    eli_context *ctx = audit_setup();
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);
    int before = g_audit_alloc_count;
    for (int i = 0; i < 2000; i++)
        eli_draw_list_add_rect_filled(&dl, eli_make_vec2(0, 0), eli_make_vec2(4, 4),
                                      ELI_COL32_WHITE, 0.0f, ELI_DRAW_NONE);
    ELI_ASSERT_GT(g_audit_alloc_count - before, 0);
    eli_draw_list_clear(&dl);
    AUDIT_TEARDOWN_AND_CHECK(ctx);
}

ELI_TEST(mem_coverage_table)
{
    eli_context *ctx = audit_setup();
    ELI_ASSERT_GT(audit_allocs_during(ui_table, 3), 0);
    AUDIT_TEARDOWN_AND_CHECK(ctx);
}

ELI_TEST(mem_coverage_tab_bar)
{
    eli_context *ctx = audit_setup();
    ELI_ASSERT_GT(audit_allocs_during(ui_tabs, 3), 0);
    AUDIT_TEARDOWN_AND_CHECK(ctx);
}

ELI_TEST(mem_coverage_plot)
{
    eli_context *ctx = audit_setup();
    ELI_ASSERT_GT(audit_allocs_during(ui_plot, 3), 0);
    AUDIT_TEARDOWN_AND_CHECK(ctx);
}

ELI_TEST(mem_coverage_docking)
{
    eli_context *ctx = audit_setup();
    ELI_ASSERT_GT(audit_allocs_during(ui_dock, 4), 0);
    ELI_ASSERT_NOT_NULL(eli_dock_node_find(ctx, AUDIT_DOCK_ID));
    AUDIT_TEARDOWN_AND_CHECK(ctx);
}

ELI_TEST(mem_coverage_overlays_and_drag_drop)
{
    eli_context *ctx = audit_setup();
    ELI_ASSERT_GT(audit_allocs_during(ui_overlays, 2), 0);
    eli_io_add_mouse_pos_event(20.0f, 40.0f);
    test_ui_frame(ui_drag_source);
    test_ui_frames(ui_drag_source, 2);
    AUDIT_TEARDOWN_AND_CHECK(ctx);
}

static void ui_log_capture(void)
{
    eli_begin("Log", NULL, 0);
    eli_log_to_clipboard(-1);
    for (int i = 0; i < 50; i++)
        eli_log_text("line %d of the captured log\n", i);
    eli_log_finish();
    eli_end();
}

ELI_TEST(mem_coverage_logging_and_clipboard)
{
    eli_context *ctx = audit_setup();
    ELI_ASSERT_GT(audit_allocs_during(ui_log_capture, 2), 0);
    AUDIT_TEARDOWN_AND_CHECK(ctx);
}

ELI_TEST(mem_coverage_settings_round_trip)
{
    eli_context *ctx = audit_setup();
    test_ui_frames(ui_many_windows, 2);
    size_t len = 0;
    char *ini = eli_save_ini_settings_to_memory(&len);
    ELI_ASSERT_NOT_NULL(ini);
    eli_load_ini_settings_from_memory(ini, len);
    eli_mem_free(ini);
    AUDIT_TEARDOWN_AND_CHECK(ctx);
}

ELI_TEST_MAIN()
