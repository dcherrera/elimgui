/**
 * @file eli_input_shortcut.h
 * @brief Keyboard shortcuts and clipboard access for elimgui: eli_shortcut() and
 *        the predefined edit chords (copy/cut/paste/undo/redo/select-all), plus
 *        clipboard get/set with a pluggable callback, a browser JS-interop path
 *        (ELI_JSIO), and an internal static-buffer fallback for hosted builds.
 *
 * @status Phase 4 shortcuts + clipboard in use. Shortcut routing/focus (which
 *         needs the window/focus system) is deferred; eli_shortcut currently
 *         reports the raw chord state.
 * @issues None
 * @todo None
 */
#ifndef ELI_INPUT_ELI_INPUT_SHORTCUT_H
#define ELI_INPUT_ELI_INPUT_SHORTCUT_H

#include "eli_input_keyboard.h"

/* ---------------------------------------------------------------------------
 * Predefined edit shortcuts (Ctrl-based; a macOS-aware backend may remap Ctrl
 * to Super before dispatch).
 * ------------------------------------------------------------------------- */

#define ELI_SHORTCUT_COPY       (ELI_MOD_CTRL | ELI_KEY_C)
#define ELI_SHORTCUT_CUT        (ELI_MOD_CTRL | ELI_KEY_X)
#define ELI_SHORTCUT_PASTE      (ELI_MOD_CTRL | ELI_KEY_V)
#define ELI_SHORTCUT_UNDO       (ELI_MOD_CTRL | ELI_KEY_Z)
#define ELI_SHORTCUT_REDO       (ELI_MOD_CTRL | ELI_KEY_Y)
#define ELI_SHORTCUT_SELECT_ALL (ELI_MOD_CTRL | ELI_KEY_A)

/**
 * Test whether a shortcut chord fired this frame.
 *
 * Routing and focus arbitration (which require the window/focus system) are not
 * implemented yet, so this is currently equivalent to eli_is_key_chord_pressed:
 * it reports the raw chord state without ownership checks.
 *
 * @param key_chord  ELI_MOD_* flags OR'd with an eli_key (e.g. ELI_SHORTCUT_COPY).
 * @return           true if the chord is active this frame.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline bool eli_shortcut(eli_key_chord key_chord)
{
    return eli_is_key_chord_pressed(key_chord);
}

/* ---------------------------------------------------------------------------
 * Clipboard
 *
 * Resolution order for both get and set:
 *   1. If the IO clipboard callbacks are set, they are used.
 *   2. Otherwise an internal static buffer holds the text. Under ELI_JSIO the
 *      set path also forwards to the host page (eli_host_set_clipboard) and the
 *      host page delivers OS clipboard text back via eli_host_provide_clipboard.
 * ------------------------------------------------------------------------- */

#ifndef ELI_CLIPBOARD_BUFFER_SIZE
#define ELI_CLIPBOARD_BUFFER_SIZE 1024
#endif

/* Fallback clipboard storage (single-TU header-only build). */
static char eli_clipboard_fallback_buffer[ELI_CLIPBOARD_BUFFER_SIZE];

/** Copy text into the fallback buffer, truncating to its capacity. */
static inline void eli_clipboard_store_fallback(const char *text)
{
    size_t n = text ? strlen(text) : 0;
    if (n >= ELI_CLIPBOARD_BUFFER_SIZE)
        n = ELI_CLIPBOARD_BUFFER_SIZE - 1;
    if (n > 0)
        memcpy(eli_clipboard_fallback_buffer, text, n);
    eli_clipboard_fallback_buffer[n] = '\0';
}

#ifdef ELI_JSIO

/* Host page provides these: the C side pushes copied text out to the OS
 * clipboard, and the host pushes OS clipboard text in on paste. */
JS_IMPORT(eli_host_set_clipboard) void eli_host_set_clipboard(const char *text);

/** Receive OS clipboard text from the host page into the fallback buffer. */
JS_EXPORT(eli_host_provide_clipboard)
void eli_host_provide_clipboard(const char *text)
{
    eli_clipboard_store_fallback(text);
}

#endif /* ELI_JSIO */

/**
 * @return  The current clipboard text. Never NULL; the returned pointer is owned
 *          by elimgui/the callback and remains valid until the next clipboard
 *          operation.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline const char *eli_get_clipboard_text(void)
{
    eli_io *io = eli_get_io();
    if (io && io->get_clipboard_text_fn)
        return io->get_clipboard_text_fn(io->clipboard_user_data);
    return eli_clipboard_fallback_buffer;
}

/**
 * Set the clipboard text.
 *
 * @param text  NUL-terminated text to place on the clipboard (NULL clears it).
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_set_clipboard_text(const char *text)
{
    eli_io *io = eli_get_io();
    if (io && io->set_clipboard_text_fn) {
        io->set_clipboard_text_fn(io->clipboard_user_data, text);
        return;
    }
    eli_clipboard_store_fallback(text);
#ifdef ELI_JSIO
    eli_host_set_clipboard(eli_clipboard_fallback_buffer);
#endif
}

/**
 * Install custom clipboard callbacks on the current context's IO.
 *
 * @param get_fn     Returns clipboard text (may be NULL to clear the override).
 * @param set_fn     Stores clipboard text (may be NULL to clear the override).
 * @param user_data  Opaque pointer passed back to both callbacks.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_set_clipboard_callbacks(const char *(*get_fn)(void *user_data),
                                               void (*set_fn)(void *user_data, const char *text),
                                               void *user_data)
{
    eli_io *io = eli_get_io();
    if (!io)
        return;
    io->get_clipboard_text_fn = get_fn;
    io->set_clipboard_text_fn = set_fn;
    io->clipboard_user_data = user_data;
}

#endif /* ELI_INPUT_ELI_INPUT_SHORTCUT_H */
