/**
 * @file eli_frame.h
 * @brief Frame-lifecycle composition: eli_frame_begin / eli_frame_end run every
 *        subsystem's per-frame hook in the correct order, and register a widget
 *        shutdown hook so eli_destroy_context releases all subsystem heap state.
 *
 * This header sits on top of every category, so it is the umbrella's final
 * include. Use it to drive a frame without hand-ordering the individual hooks:
 *     eli_frame_begin();
 *       ... eli_begin(...) / widgets / eli_end() ...
 *     eli_frame_end();
 *     eli_draw_data *dd = eli_get_draw_data();
 *
 * @status Frame-lifecycle composition in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_FRAME_H
#define ELI_FRAME_H

#include "core/eli_platform.h"
#include "core/eli_core.h"
#include "input/eli_input.h"
#include "window/eli_window.h"
#include "widgets/eli_widgets.h"
#include "util/eli_util.h"
#include "docking/eli_dock.h"

/**
 * Release all widget-subsystem heap state (table pool, tab-bar pool, and the
 * pending drag-drop payload). Registered as the context's widget_shutdown_fn by
 * eli_frame_begin so eli_destroy_context can reach it without a core->widget
 * include cycle.
 *
 * @param ctx  Owning context (unused; the widget pools are module globals).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_frame__shutdown_widgets(struct eli_context *ctx)
{
    (void)ctx;
    eli_table_shutdown();
    eli_tab_shutdown();
    eli_clear_drag_drop();
}

/**
 * Begin a full UI frame: advances the frame and rolls every subsystem's
 * begin-of-frame state in dependency order. Call once per frame before any
 * eli_begin/eli_end or widget calls.
 *
 * Order: eli_new_frame -> eli_util_new_frame -> eli_input_update_begin_frame ->
 * eli_window_new_frame -> eli_dock_new_frame -> eli_popup_new_frame ->
 * eli_drag_drop_new_frame -> eli_table_new_frame -> eli_tab_new_frame.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_frame_begin(void)
{
    eli_new_frame();

    eli_context *ctx = eli_get_current_context();
    if (ctx != NULL && ctx->widget_shutdown_fn == NULL)
        ctx->widget_shutdown_fn = eli_frame__shutdown_widgets;

    /* Clear the persistent background/foreground draw lists so user draws this
     * frame start fresh (Phase 27 util). No-op until first requested. */
    eli_util_new_frame();

    eli_input_update_begin_frame();
    eli_window_new_frame();
    eli_dock_new_frame();
    eli_popup_new_frame();
    eli_drag_drop_new_frame();
    eli_table_new_frame();
    eli_tab_new_frame();
}

/**
 * End a full UI frame: flushes end-of-frame subsystem state, assembles the
 * window draw data, closes the frame scope, and rolls input to its next-frame
 * baseline. Call once per frame after all eli_begin/eli_end pairs; afterwards
 * eli_get_draw_data returns this frame's populated draw data.
 *
 * Order: eli_popup_end_frame -> eli_drag_drop_end_frame -> eli_window_render
 * -> eli_render -> eli_input_update_end_frame.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_frame_end(void)
{
    eli_popup_end_frame();
    eli_drag_drop_end_frame();
    eli_window_render();
    eli_render();
    eli_input_update_end_frame();
}

#endif /* ELI_FRAME_H */
