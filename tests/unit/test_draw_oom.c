/**
 * @file test_draw_oom.c
 * @brief Draw-buffer OOM safety (EfficiencyPlan Pre-work, spec §6): with a failing
 *        allocator, primitives bail without writing past the real capacity.
 *
 * The allocator hands out exact-size blocks and starts failing after a budget.
 * Invariants checked after every primitive: counts never exceed capacities and
 * every command's index range stays inside the index buffer. Run under
 * ELI_TEST_SANITIZE=1 to have ASan prove there is no out-of-bounds write.
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/elimgui.h>

static int g_allocs_left = 0;
static int g_failed_allocs = 0;

static void *budget_alloc(size_t sz, void *user_data)
{
    (void)user_data;
    if (g_allocs_left <= 0) {
        g_failed_allocs++;
        return NULL;
    }
    g_allocs_left--;
    return malloc(sz);
}

static void budget_free(void *ptr, void *user_data)
{
    (void)user_data;
    free(ptr);
}

/** @return number of violated buffer invariants. */
static int buffer_violations(const eli_draw_list *dl)
{
    int bad = 0;
    bad += dl->vtx_count > dl->vtx_capacity;
    bad += dl->idx_count > dl->idx_capacity;
    bad += dl->cmd_count > dl->cmd_capacity;
    uint32_t sum = 0;
    for (uint32_t c = 0; c < dl->cmd_count; c++) {
        bad += dl->cmds[c].idx_offset + dl->cmds[c].elem_count > dl->idx_count;
        sum += dl->cmds[c].elem_count;
    }
    bad += sum != dl->idx_count;
    return bad;
}

/** Fill a list with mixed primitives under an allocation budget. */
static int run_with_budget(int budget)
{
    eli_set_allocator_functions(budget_alloc, budget_free, NULL);
    g_allocs_left = budget;
    g_failed_allocs = 0;
    eli_draw_list dl;
    memset(&dl, 0, sizeof(dl));
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);
    int bad = 0;
    for (int i = 0; i < 2000; i++) {
        float x = (float)(i % 200);
        eli_draw_list_add_rect_filled(&dl, eli_make_vec2(x, 0), eli_make_vec2(x + 3, 3),
                                      ELI_COL32_WHITE, 0.0f, ELI_DRAW_NONE);
        eli_draw_list_add_line(&dl, eli_make_vec2(x, 0), eli_make_vec2(x, 9), ELI_COL32_WHITE, 1.0f);
        eli_draw_list_add_circle_filled(&dl, eli_make_vec2(x, 5), 4.0f, ELI_COL32_WHITE, 12);
        if (i % 50 == 0) {
            eli_draw_list_push_clip_rect(&dl, eli_make_vec2(0, 0), eli_make_vec2(x + 10, 10), true);
            eli_draw_list_push_texture_id(&dl, (eli_texture_id)(i + 1));
        }
        if (i % 50 == 49) {
            eli_draw_list_pop_texture_id(&dl);
            eli_draw_list_pop_clip_rect(&dl);
        }
        bad += buffer_violations(&dl);
    }
    eli_draw_list_clear(&dl);
    eli_set_allocator_functions(NULL, NULL, NULL);
    return bad;
}

ELI_TEST(failing_allocator_never_breaks_buffer_invariants) {
    int total_bad = 0, total_failed = 0;
    for (int budget = 0; budget <= 40; budget++) {
        total_bad += run_with_budget(budget);
        total_failed += g_failed_allocs;
    }
    ELI_ASSERT_EQ(total_bad, 0);
    ELI_ASSERT_GT(total_failed, 0);              /* the OOM path really ran */
}

ELI_TEST(grow_reports_failure_by_leaving_capacity_short) {
    eli_set_allocator_functions(budget_alloc, budget_free, NULL);
    g_allocs_left = 0;
    int cap = 0;
    void *p = eli_draw_buf_grow(NULL, &cap, 10, 4);
    ELI_ASSERT_NULL(p);
    ELI_ASSERT_LT(cap, 10);
    eli_set_allocator_functions(NULL, NULL, NULL);
}

ELI_TEST(reserve_returns_false_on_oom) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);
    eli_set_allocator_functions(budget_alloc, budget_free, NULL);
    g_allocs_left = 0;
    ELI_ASSERT_FALSE(eli_draw_list_prim_reserve(&dl, 6, 4));
    ELI_ASSERT_EQ(dl.vtx_count, 0u);
    ELI_ASSERT_EQ(dl.cmds[0].elem_count, 0u);
    eli_set_allocator_functions(NULL, NULL, NULL);
    eli_draw_list_clear(&dl);
}

ELI_TEST_MAIN()
