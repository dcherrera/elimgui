/*
 * elimgui demo application
 *
 * Demonstrates all elimgui widgets and features.
 */

#include <jaclibc.h>
#include <eli/elimgui.h>

/* Called once on startup */
JS_EXPORT(js_start)
void js_start(void) {
    eli_init();
}

/* Called each frame */
JS_EXPORT(frame)
void frame(void) {
    eli_new_frame();

    /* Demo window will go here */
    eli_begin("elimgui Demo", NULL, ELI_WINDOW_NONE);
    /* eli_text("Hello, elimgui!"); */
    eli_end();

    eli_render();

    /* TODO: Get draw data and render */
    /* eli_draw_data* data = eli_get_draw_data(); */
}
