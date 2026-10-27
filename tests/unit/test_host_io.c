/**
 * @file test_host_io.c
 * @brief Native host services (spec §4.4): settings and log files via stdio, and
 *        the io.platform_open_url_fn seam.
 *
 * Files are written under ELI_TEST_OUT_DIR (the gitignored build dir).
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/elimgui.h>

#define HOST_IO_INI ELI_TEST_OUT_DIR "/host_io_test.ini"
#define HOST_IO_LOG ELI_TEST_OUT_DIR "/host_io_test.log"

static int g_open_url_calls = 0;
static char g_open_url_last[128];

static bool record_open_url(void *user_data, const char *url)
{
    (void)user_data;
    g_open_url_calls++;
    snprintf(g_open_url_last, sizeof(g_open_url_last), "%s", url);
    return true;
}

ELI_TEST(host_io_write_then_read_round_trips) {
    remove(HOST_IO_INI);
    ELI_ASSERT_NULL(eli_host_io_read_text_file(HOST_IO_INI));
    ELI_ASSERT_TRUE(eli_host_io_write_text_file(HOST_IO_INI, "abc\n", false));
    ELI_ASSERT_TRUE(eli_host_io_write_text_file(HOST_IO_INI, "def\n", true));
    char *text = eli_host_io_read_text_file(HOST_IO_INI);
    ELI_ASSERT_NOT_NULL(text);
    if (text)
        ELI_ASSERT_STR_EQ(text, "abc\ndef\n");
    eli_mem_free(text);
}

ELI_TEST(host_io_reads_files_larger_than_one_chunk) {
    size_t n = 20000;
    char *big = (char *)malloc(n + 1);
    for (size_t i = 0; i < n; i++)
        big[i] = (char)('a' + (int)(i % 26));
    big[n] = '\0';
    ELI_ASSERT_TRUE(eli_host_io_write_text_file(HOST_IO_INI, big, false));
    char *text = eli_host_io_read_text_file(HOST_IO_INI);
    ELI_ASSERT_NOT_NULL(text);
    if (text) {
        ELI_ASSERT_EQ(strlen(text), n);
        ELI_ASSERT_TRUE(memcmp(text, big, n) == 0);
    }
    eli_mem_free(text);
    free(big);
}

ELI_TEST(settings_disk_round_trip_uses_stdio) {
    const char *ini = "[Window][Disk]\nPos=12,34\nSize=56,78\nCollapsed=1\n";
    ELI_ASSERT_TRUE(eli_host_io_write_text_file(HOST_IO_INI, ini, false));
    eli_settings_clear();
    eli_load_ini_settings_from_disk(HOST_IO_INI);
    eli_vec2 pos = {0}, size = {0};
    bool collapsed = false;
    ELI_ASSERT_TRUE(eli_settings_get_window("Disk", &pos, &size, &collapsed));
    ELI_ASSERT_FLT_NEAR(pos.x, 12.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(size.y, 78.0f, 1e-4);
    ELI_ASSERT_TRUE(collapsed);
    eli_settings_clear();
}

ELI_TEST(log_to_file_writes_the_captured_text) {
    remove(HOST_IO_LOG);
    eli_log_to_file(1, HOST_IO_LOG);
    eli_log_text("hello %d", 42);
    eli_log_finish();
    char *text = eli_host_io_read_text_file(HOST_IO_LOG);
    ELI_ASSERT_NOT_NULL(text);
    if (text)
        ELI_ASSERT_TRUE(strstr(text, "hello 42") != NULL);
    eli_mem_free(text);
}

ELI_TEST(open_url_goes_through_io_callback) {
    eli_context *ctx = eli_create_context();
    ELI_ASSERT_FALSE(eli_platform_open_url("https://example.com"));
    ctx->io.platform_open_url_fn = record_open_url;
    ELI_ASSERT_TRUE(eli_platform_open_url("https://example.com"));
    ELI_ASSERT_EQ(g_open_url_calls, 1);
    ELI_ASSERT_STR_EQ(g_open_url_last, "https://example.com");
    eli_destroy_context(ctx);
}

ELI_TEST(ini_filename_defaults_to_null) {
    eli_context *ctx = eli_create_context();
    ELI_ASSERT_NULL(ctx->io.ini_filename);
    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
