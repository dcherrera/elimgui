/**
 * @file eli_debug.h
 * @brief Debug and inspection windows (metrics, debug log, id-stack tool, about,
 *        style/font editors and selectors, user guide) plus eli_get_version.
 *        Mirrors Dear ImGui's debug tooling, composed from the public API.
 *
 * @status None
 * @issues None
 * @todo None
 */
#ifndef ELI_DEMO_ELI_DEBUG_H
#define ELI_DEMO_ELI_DEBUG_H

#include <eli/elimgui.h>

#include <stdarg.h>
#include <stdio.h>

/** Library version string returned by eli_get_version. Canonically defined in
 * the umbrella <eli/elimgui.h>; this guarded fallback covers standalone use. */
#ifndef ELI_VERSION
#define ELI_VERSION "1.0.0"
#endif

/** Capacity of the process-wide debug log ring buffer, in bytes. */
#define ELI_DEBUG_LOG_CAPACITY 8192

/**
 * @return  The elimgui version string (never NULL, never empty).
 *
 * Thread-safe: yes (returns a static string literal)
 * Reentrant: yes
 */
static inline const char *eli_get_version(void)
{
    return ELI_VERSION;
}

/* ---------------------------------------------------------------------------
 * Debug log buffer. A tiny append-only text store the debug-log window renders;
 * eli_debug_log lets application code record human-readable diagnostics.
 * ------------------------------------------------------------------------- */

typedef struct eli_debug_log_buffer {
    char   text[ELI_DEBUG_LOG_CAPACITY];
    size_t length;
} eli_debug_log_buffer;

/** @return Pointer to the singleton debug-log buffer (never NULL). */
static inline eli_debug_log_buffer *eli_debug__log_buffer(void)
{
    static eli_debug_log_buffer buf = {0};
    return &buf;
}

/**
 * Append a formatted line to the debug log. Silently drops the entry (after
 * resetting) if it would overflow the fixed capacity, so it never allocates.
 *
 * @param fmt  printf-style format string; a newline is appended automatically.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_debug_log(const char *fmt, ...)
{
    eli_debug_log_buffer *buf = eli_debug__log_buffer();
    size_t remaining = ELI_DEBUG_LOG_CAPACITY - buf->length;
    if (remaining < 2)
        buf->length = 0, remaining = ELI_DEBUG_LOG_CAPACITY;

    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf->text + buf->length, remaining, fmt, args);
    va_end(args);

    if (n < 0 || (size_t)n >= remaining) {
        buf->text[buf->length] = '\0';
        return;
    }
    buf->length += (size_t)n;
    if (buf->length + 1 < ELI_DEBUG_LOG_CAPACITY) {
        buf->text[buf->length++] = '\n';
        buf->text[buf->length] = '\0';
    }
}

/* ---------------------------------------------------------------------------
 * Version / about.
 * ------------------------------------------------------------------------- */

/**
 * Show the "About elimgui" window: version, a one-line description, and credits.
 *
 * @param p_open  Optional visibility flag (see eli_show_demo_window).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_show_about_window(bool *p_open)
{
    if (p_open != NULL && !*p_open)
        return;
    if (!eli_begin("About elimgui", p_open, ELI_WINDOW_AUTO_RESIZE | ELI_WINDOW_NO_RESIZE)) {
        eli_end();
        return;
    }
    eli_text("elimgui %s", eli_get_version());
    eli_separator();
    eli_text("A pure C11 reimplementation of Dear ImGui, targeting WebAssembly.");
    eli_bullet_text("Header-only, no Emscripten, built on JAClibc.");
    eli_bullet_text("Immediate-mode: no retained state except what is explicit.");
    eli_end();
}

/**
 * Show a short usage guide describing mouse/keyboard interaction conventions.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_show_user_guide(void)
{
    eli_bullet_text("Click and drag a title bar to move a window.");
    eli_bullet_text("Click and drag the lower-right corner to resize.");
    eli_bullet_text("Click a collapsing header to expand or collapse a section.");
    eli_bullet_text("Double-click a slider or drag to type an exact value.");
    eli_bullet_text("Drag a source widget onto a target to transfer a payload.");
}

/* ---------------------------------------------------------------------------
 * Style editor + selectors.
 * ------------------------------------------------------------------------- */

/**
 * Show a theme dropdown that applies Dark/Light/Classic to the current style.
 *
 * @param label  Widget label for the combo.
 * @return       true when the user picked a different theme this frame.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_show_style_selector(const char *label)
{
    static const char *themes[] = {"Dark", "Light", "Classic"};
    static int current = 0;
    eli_style *style = eli_get_style();
    if (style == NULL)
        return false;
    if (eli_combo(label, &current, themes, 3, -1)) {
        if (current == 0) eli_style_colors_dark(style);
        else if (current == 1) eli_style_colors_light(style);
        else eli_style_colors_classic(style);
        return true;
    }
    return false;
}

/**
 * Show a font dropdown. elimgui builds a single embedded default font, so this
 * reports the active font/size rather than switching typefaces.
 *
 * @param label  Widget label for the combo.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_show_font_selector(const char *label)
{
    static const char *fonts[] = {"Default"};
    static int current = 0;
    eli_combo(label, &current, fonts, 1, -1);
    eli_same_line(0.0f, -1.0f);
    eli_text("(%.1f px)", eli_get_font_size());
}

/** Render the editable color rows of the style editor. */
static inline void eli_debug__style_colors(eli_style *style)
{
    for (int i = 0; i < ELI_COL_COUNT; i++) {
        float col[4] = {style->colors[i].x, style->colors[i].y,
                        style->colors[i].z, style->colors[i].w};
        eli_push_id_int(i);
        if (eli_color_edit4(eli_get_style_color_name(i), col, ELI_COLOR_EDIT_ALPHA_BAR)) {
            style->colors[i] = eli_make_vec4(col[0], col[1], col[2], col[3]);
        }
        eli_pop_id();
    }
}

/**
 * Show the style editor: theme/font selectors, common sizing/rounding vars, and
 * an editable table of every themeable color. When a reference style is given,
 * a "Revert" button restores the live style from it.
 *
 * @param ref  Optional reference style to revert to (may be NULL).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_show_style_editor(eli_style *ref)
{
    eli_style *style = eli_get_style();
    if (style == NULL)
        return;

    eli_show_style_selector("Theme");
    eli_show_font_selector("Font");
    if (ref != NULL) {
        eli_same_line(0.0f, -1.0f);
        if (eli_button("Revert"))
            *style = *ref;
    }

    eli_separator_text("Sizes");
    eli_slider_float("Alpha", &style->alpha, 0.2f, 1.0f, "%.2f", 0);
    eli_slider_float("WindowRounding", &style->window_rounding, 0.0f, 12.0f, "%.0f", 0);
    eli_slider_float("FrameRounding", &style->frame_rounding, 0.0f, 12.0f, "%.0f", 0);
    eli_slider_float("GrabRounding", &style->grab_rounding, 0.0f, 12.0f, "%.0f", 0);

    eli_separator_text("Colors");
    eli_debug__style_colors(style);
}

/* ---------------------------------------------------------------------------
 * Metrics / debug log / id-stack windows.
 * ------------------------------------------------------------------------- */

/**
 * Show the metrics/debugger window: frame count, draw-list vertex/index totals,
 * active and hovered ids, and a list of live windows with geometry and state.
 *
 * @param p_open  Optional visibility flag (see eli_show_demo_window).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_show_metrics_window(bool *p_open)
{
    if (p_open != NULL && !*p_open)
        return;
    eli_context *ctx = eli_get_current_context();
    if (!eli_begin("elimgui Metrics", p_open, 0)) {
        eli_end();
        return;
    }
    if (ctx == NULL) {
        eli_text("No active context.");
        eli_end();
        return;
    }

    eli_text("elimgui %s", eli_get_version());
    eli_text("Frame: %d", ctx->frame_count);
    eli_text("Framerate: %.1f FPS", (double)ctx->io.framerate);

    eli_draw_data *dd = eli_get_draw_data();
    if (dd != NULL)
        eli_text("Draw data: %d vtx, %d idx, %d cmd-lists", dd->total_vtx_count,
                 dd->total_idx_count, dd->cmd_lists_count);
    else
        eli_text("Draw data: (not assembled yet)");

    eli_text("Active id: 0x%08X", (unsigned)ctx->active_id);
    eli_text("Hovered id: 0x%08X", (unsigned)ctx->hot_id);
    eli_separator();

    if (eli_tree_node("Windows")) {
        eli_text("%d window(s)", ctx->windows_count);
        for (int i = 0; i < ctx->windows_count; i++) {
            const eli_window *w = ctx->windows[i];
            if (w == NULL)
                continue;
            if (eli_tree_node_ptr(w, "%s", w->name ? w->name : "<unnamed>")) {
                eli_text("pos (%.0f, %.0f)  size (%.0f, %.0f)", (double)w->pos.x,
                         (double)w->pos.y, (double)w->size.x, (double)w->size.y);
                eli_text("active=%d collapsed=%d hidden=%d", w->active, w->collapsed, w->hidden);
                eli_tree_pop();
            }
        }
        eli_tree_pop();
    }
    eli_end();
}

/**
 * Show the debug-log window: the accumulated eli_debug_log text with buttons to
 * clear it or copy it to the clipboard log.
 *
 * @param p_open  Optional visibility flag (see eli_show_demo_window).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_show_debug_log_window(bool *p_open)
{
    if (p_open != NULL && !*p_open)
        return;
    eli_debug_log_buffer *buf = eli_debug__log_buffer();
    if (!eli_begin("elimgui Debug Log", p_open, 0)) {
        eli_end();
        return;
    }
    if (eli_button("Clear"))
        buf->length = 0, buf->text[0] = '\0';
    eli_same_line(0.0f, -1.0f);
    eli_text("%zu bytes", buf->length);
    eli_separator();
    if (eli_begin_child("log_scroll", eli_make_vec2(0.0f, 0.0f), ELI_CHILD_BORDERS, 0)) {
        if (buf->length > 0)
            eli_text_unformatted(buf->text, buf->text + buf->length);
        else
            eli_text_disabled("(empty)");
    }
    eli_end_child();
    eli_end();
}

/**
 * Show the ID-stack tool window: the current id-stack seed values, top to
 * bottom, plus the last-item id. Useful for diagnosing id collisions.
 *
 * @param p_open  Optional visibility flag (see eli_show_demo_window).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_show_id_stack_tool_window(bool *p_open)
{
    if (p_open != NULL && !*p_open)
        return;
    eli_context *ctx = eli_get_current_context();
    if (!eli_begin("elimgui ID Stack", p_open, 0)) {
        eli_end();
        return;
    }
    if (ctx == NULL) {
        eli_text("No active context.");
        eli_end();
        return;
    }
    eli_text("ID stack depth: %d", ctx->id_stack_size);
    eli_separator();
    for (int i = 0; i < ctx->id_stack_size; i++)
        eli_text("[%d] seed 0x%08X", i, (unsigned)ctx->id_stack[i]);
    eli_separator();
    eli_text("Last item id: 0x%08X", (unsigned)ctx->last_item_id);
    eli_end();
}

#endif /* ELI_DEMO_ELI_DEBUG_H */
