/**
 * @file test_p28_settings.c
 * @brief Unit tests for Phase 28 INI settings: memory round-trip (save two live
 *        windows, reload, look them up), collapsed persistence, and graceful
 *        handling of malformed INI.
 *
 * @status Phase 28 settings coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/util/eli_settings.h>
#include <eli/window/eli_window.h>

#define TEST_DT (1.0f / 60.0f)

static eli_context *settings_setup(void)
{
    eli_context *ctx = eli_create_context();
    ctx->io.delta_time = TEST_DT;
    ctx->io.display_size = eli_make_vec2(1280.0f, 900.0f);
    eli_style_colors_dark(&ctx->style);
    return ctx;
}

static void frame_begin(void)
{
    eli_new_frame();
    eli_input_update_begin_frame();
    eli_window_new_frame();
}

static void frame_end(void)
{
    eli_window_render();
    eli_render();
    eli_input_update_end_frame();
}

ELI_TEST(settings_memory_round_trip_two_windows) {
    eli_settings_clear();
    eli_context *ctx = settings_setup();

    frame_begin();
    eli_set_next_window_pos(eli_make_vec2(10.0f, 20.0f), ELI_COND_ALWAYS,
                            eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(300.0f, 200.0f), ELI_COND_ALWAYS);
    eli_begin("Alpha", NULL, 0);
    eli_end();

    eli_set_next_window_pos(eli_make_vec2(400.0f, 50.0f), ELI_COND_ALWAYS,
                            eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(150.0f, 120.0f), ELI_COND_ALWAYS);
    eli_begin("Beta", NULL, 0);
    eli_end();
    frame_end();

    /* Save emits a section for each named, saveable window. */
    size_t len = 0;
    char *ini = eli_save_ini_settings_to_memory(&len);
    ELI_ASSERT_NOT_NULL(ini);
    ELI_ASSERT_GT(len, 0u);
    ELI_ASSERT_NOT_NULL(strstr(ini, "[Window][Alpha]"));
    ELI_ASSERT_NOT_NULL(strstr(ini, "[Window][Beta]"));
    ELI_ASSERT_NOT_NULL(strstr(ini, "Pos=10,20"));
    ELI_ASSERT_NOT_NULL(strstr(ini, "Size=300,200"));

    /* Reload into a fresh store and confirm both entries survive verbatim. */
    eli_settings_clear();
    eli_load_ini_settings_from_memory(ini, len);

    eli_vec2 pos = {0}, size = {0};
    bool collapsed = true;
    ELI_ASSERT_TRUE(eli_settings_get_window("Alpha", &pos, &size, &collapsed));
    ELI_ASSERT_FLT_NEAR(pos.x, 10.0f, 0.5f);
    ELI_ASSERT_FLT_NEAR(pos.y, 20.0f, 0.5f);
    ELI_ASSERT_FLT_NEAR(size.x, 300.0f, 0.5f);
    ELI_ASSERT_FLT_NEAR(size.y, 200.0f, 0.5f);
    ELI_ASSERT_FALSE(collapsed);

    ELI_ASSERT_TRUE(eli_settings_get_window("Beta", &pos, &size, &collapsed));
    ELI_ASSERT_FLT_NEAR(pos.x, 400.0f, 0.5f);
    ELI_ASSERT_FLT_NEAR(size.y, 120.0f, 0.5f);

    /* Unknown window is reported absent. */
    ELI_ASSERT_FALSE(eli_settings_get_window("Missing", &pos, &size, &collapsed));

    eli_mem_free(ini);
    eli_settings_clear();
    eli_destroy_context(ctx);
}

ELI_TEST(settings_collapsed_persisted) {
    eli_settings_clear();

    const char *ini =
        "[Window][Panel]\n"
        "Pos=5,6\n"
        "Size=42,24\n"
        "Collapsed=1\n\n";
    eli_load_ini_settings_from_memory(ini, strlen(ini));

    eli_vec2 pos = {0}, size = {0};
    bool collapsed = false;
    ELI_ASSERT_TRUE(eli_settings_get_window("Panel", &pos, &size, &collapsed));
    ELI_ASSERT_TRUE(collapsed);
    ELI_ASSERT_FLT_NEAR(pos.x, 5.0f, 0.5f);
    ELI_ASSERT_FLT_NEAR(size.x, 42.0f, 0.5f);

    eli_settings_clear();
}

ELI_TEST(settings_malformed_ini_ignored) {
    eli_settings_clear();

    /* Junk, orphan values before any section, and a broken header. */
    const char *garbage =
        "not an ini line\n"
        "Pos=1,2\n"                 /* no current section -> dropped */
        "[Window][Broken\n"         /* missing closing bracket -> ignored */
        "Size=9,9\n"
        "=====\n";
    eli_load_ini_settings_from_memory(garbage, strlen(garbage));

    eli_vec2 pos = {0}, size = {0};
    bool collapsed = false;
    ELI_ASSERT_FALSE(eli_settings_get_window("Broken", &pos, &size, &collapsed));
    /* Store stays empty: no valid section was produced. */
    ELI_ASSERT_EQ(eli_settings_store_count, 0u);

    /* Empty / NULL inputs are safe no-ops. */
    eli_load_ini_settings_from_memory(NULL, 10);
    eli_load_ini_settings_from_memory("", 0);
    ELI_ASSERT_EQ(eli_settings_store_count, 0u);

    eli_settings_clear();
}

ELI_TEST(settings_save_with_no_context_returns_null) {
    ELI_ASSERT_NULL(eli_get_current_context());
    size_t len = 123;
    char *ini = eli_save_ini_settings_to_memory(&len);
    ELI_ASSERT_NULL(ini);
    ELI_ASSERT_EQ(len, 0u);
}

ELI_TEST_MAIN()
