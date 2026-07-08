/**
 * @file eli_logging.h
 * @brief Text logging/capture for elimgui: eli_log_to_tty/_file/_clipboard start
 *        a capture session, eli_log_text[_v] append formatted text to a
 *        file-static buffer, and eli_log_finish flushes it to the chosen target.
 *
 * Mirrors Dear ImGui's logging model. While a session is active, widget/user
 * text is appended to a growable file-static buffer. On finish the buffer is
 * flushed: TTY -> a JS/print host hook (ELI_JSIO) else no-op, FILE -> a JS host
 * hook, CLIPBOARD -> eli_set_clipboard_text (which uses the hosted fallback
 * buffer in tests). The auto-open depth is accepted and stored for callers that
 * want to expand tree nodes while logging; expansion itself is applied by the
 * tree widgets and is out of scope here.
 *
 * eli_log_buttons() renders the standard Log-To buttons + depth slider using the
 * widget/layout API; it is browser/demo-tested, not unit-tested.
 *
 * @status Capture buffer + memory/clipboard flush implemented; TTY/FILE JS-hooked.
 * @issues None
 * @todo None
 */
#ifndef ELI_UTIL_LOGGING_H
#define ELI_UTIL_LOGGING_H

#include "eli_mem.h"

#include "../core/eli_platform.h"

#include "../input/eli_input_shortcut.h"
#include "../layout/eli_layout_helpers.h"
#include "../layout/eli_layout_stack.h"
#include "../widgets/eli_button_widgets.h"
#include "../widgets/eli_slider.h"

#include <stdarg.h>
#include <stdio.h>

/* Initial capacity for the log capture buffer, grown geometrically as needed. */
#define ELI_LOG_BUFFER_INITIAL_CAPACITY 256

/* Bounded stack buffer used to format a single eli_log_text call. */
#define ELI_LOG_FORMAT_BUFFER 1024

/* Max stored destination path for eli_log_to_file (including NUL). */
#define ELI_LOG_FILENAME_MAX 256

/* ---------------------------------------------------------------------------
 * State
 * ------------------------------------------------------------------------- */

/** Where a finished log session is flushed. */
typedef enum eli_log_target {
    ELI_LOG_TARGET_NONE = 0,
    ELI_LOG_TARGET_TTY,
    ELI_LOG_TARGET_FILE,
    ELI_LOG_TARGET_CLIPBOARD
} eli_log_target;

/* Active-session flag and destination. */
static bool           eli_log_active = false;
static eli_log_target eli_log_active_target = ELI_LOG_TARGET_NONE;

/* Depth to which tree nodes are auto-opened while logging (-1 = unlimited). */
static int eli_log_auto_open_depth = -1;

/* Default depth exposed by the eli_log_buttons() slider. */
static int eli_log_depth_default = 2;

/* Destination path captured by eli_log_to_file. */
static char eli_log_filename[ELI_LOG_FILENAME_MAX];

/* Growable capture buffer (single-TU header-only build). */
static char  *eli_log_buffer = NULL;
static size_t eli_log_buffer_len = 0;
static size_t eli_log_buffer_capacity = 0;

/* ---------------------------------------------------------------------------
 * JS host hooks (browser only)
 * ------------------------------------------------------------------------- */

#ifdef ELI_JSIO

/* Host page provides these: print a string to the developer console/TTY, and
 * append a string to a file at the given path (returns 0 on success). */
JS_IMPORT(eli_host_log_print) void eli_host_log_print(const char *text);
JS_IMPORT(eli_host_log_write_file) int eli_host_log_write_file(const char *path,
                                                               const char *text);

#endif /* ELI_JSIO */

/* ---------------------------------------------------------------------------
 * Buffer management (internal)
 * ------------------------------------------------------------------------- */

/** Ensure the capture buffer can hold at least `needed` bytes plus a NUL. */
static inline bool eli_log_buffer_reserve(size_t needed)
{
    if (needed + 1 <= eli_log_buffer_capacity)
        return true;
    size_t new_cap = eli_log_buffer_capacity ? eli_log_buffer_capacity
                                             : ELI_LOG_BUFFER_INITIAL_CAPACITY;
    while (new_cap < needed + 1)
        new_cap *= 2;
    char *grown = (char *)eli_mem_alloc(new_cap);
    if (!grown)
        return false;
    if (eli_log_buffer) {
        memcpy(grown, eli_log_buffer, eli_log_buffer_len + 1);
        eli_mem_free(eli_log_buffer);
    } else {
        grown[0] = '\0';
    }
    eli_log_buffer = grown;
    eli_log_buffer_capacity = new_cap;
    return true;
}

/** Reset the capture buffer contents (retains any allocated capacity). */
static inline void eli_log_buffer_reset(void)
{
    eli_log_buffer_len = 0;
    if (eli_log_buffer)
        eli_log_buffer[0] = '\0';
}

/** Begin a capture session targeting `target` at `auto_open_depth`. */
static inline void eli_log_begin(eli_log_target target, int auto_open_depth)
{
    eli_log_active = true;
    eli_log_active_target = target;
    eli_log_auto_open_depth = auto_open_depth;
    eli_log_buffer_reset();
}

/* ---------------------------------------------------------------------------
 * Session start
 * ------------------------------------------------------------------------- */

/**
 * Start logging to the TTY (developer console under ELI_JSIO).
 *
 * @param auto_open_depth  Tree depth to auto-expand while logging (-1 = all).
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_log_to_tty(int auto_open_depth)
{
    eli_log_begin(ELI_LOG_TARGET_TTY, auto_open_depth);
}

/**
 * Start logging to a file.
 *
 * @param auto_open_depth  Tree depth to auto-expand while logging (-1 = all).
 * @param filename         Destination path (stored; flushed on finish).
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_log_to_file(int auto_open_depth, const char *filename)
{
    eli_log_begin(ELI_LOG_TARGET_FILE, auto_open_depth);
    if (filename) {
        size_t n = strlen(filename);
        if (n >= ELI_LOG_FILENAME_MAX)
            n = ELI_LOG_FILENAME_MAX - 1;
        memcpy(eli_log_filename, filename, n);
        eli_log_filename[n] = '\0';
    } else {
        eli_log_filename[0] = '\0';
    }
}

/**
 * Start logging to the clipboard.
 *
 * @param auto_open_depth  Tree depth to auto-expand while logging (-1 = all).
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_log_to_clipboard(int auto_open_depth)
{
    eli_log_begin(ELI_LOG_TARGET_CLIPBOARD, auto_open_depth);
}

/* ---------------------------------------------------------------------------
 * Append text
 * ------------------------------------------------------------------------- */

/**
 * Append formatted text to the active log buffer (va_list form). No-op when no
 * session is active.
 *
 * @param fmt   printf-style format string.
 * @param args  Variadic argument list.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_log_text_v(const char *fmt, va_list args)
{
    if (!eli_log_active || !fmt)
        return;
    char stack_buf[ELI_LOG_FORMAT_BUFFER];
    int n = vsnprintf(stack_buf, sizeof(stack_buf), fmt, args);
    if (n <= 0)
        return;
    size_t add = (size_t)n;
    if (add >= sizeof(stack_buf))
        add = sizeof(stack_buf) - 1; /* output was truncated to the stack buffer */
    if (!eli_log_buffer_reserve(eli_log_buffer_len + add))
        return;
    memcpy(eli_log_buffer + eli_log_buffer_len, stack_buf, add);
    eli_log_buffer_len += add;
    eli_log_buffer[eli_log_buffer_len] = '\0';
}

/**
 * Append formatted text to the active log buffer. No-op when no session active.
 *
 * @param fmt  printf-style format string, followed by its arguments.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_log_text(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    eli_log_text_v(fmt, args);
    va_end(args);
}

/* ---------------------------------------------------------------------------
 * Finish / flush
 * ------------------------------------------------------------------------- */

/**
 * End the active logging session and flush the captured text to its target:
 * TTY -> host print hook (ELI_JSIO) else no-op, FILE -> host write hook,
 * CLIPBOARD -> eli_set_clipboard_text (hosted fallback buffer in tests).
 * No-op when no session is active.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_log_finish(void)
{
    if (!eli_log_active)
        return;

    const char *text = eli_log_buffer ? eli_log_buffer : "";
    switch (eli_log_active_target) {
    case ELI_LOG_TARGET_CLIPBOARD:
        eli_set_clipboard_text(text);
        break;
    case ELI_LOG_TARGET_TTY:
#ifdef ELI_JSIO
        eli_host_log_print(text);
#endif
        break;
    case ELI_LOG_TARGET_FILE:
#ifdef ELI_JSIO
        eli_host_log_write_file(eli_log_filename, text);
#endif
        break;
    case ELI_LOG_TARGET_NONE:
    default:
        break;
    }

    eli_log_active = false;
    eli_log_active_target = ELI_LOG_TARGET_NONE;
    eli_log_buffer_reset();
}

/* ---------------------------------------------------------------------------
 * UI helper (demo/browser-tested)
 * ------------------------------------------------------------------------- */

/**
 * Render the standard row of logging controls: "Log To TTY", "Log To File",
 * "Log To Clipboard" buttons plus a default-depth slider. Clicking a button
 * starts the corresponding session at the current default depth.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_log_buttons(void)
{
    bool to_tty = eli_button("Log To TTY");
    eli_same_line(0.0f, -1.0f);
    bool to_file = eli_button("Log To File");
    eli_same_line(0.0f, -1.0f);
    bool to_clipboard = eli_button("Log To Clipboard");
    eli_same_line(0.0f, -1.0f);

    eli_set_next_item_width(80.0f);
    eli_slider_int("Default Depth", &eli_log_depth_default, 0, 9, "%d", 0);

    if (to_tty)
        eli_log_to_tty(eli_log_depth_default);
    if (to_file)
        eli_log_to_file(eli_log_depth_default, eli_log_filename);
    if (to_clipboard)
        eli_log_to_clipboard(eli_log_depth_default);
}

#endif /* ELI_UTIL_LOGGING_H */
