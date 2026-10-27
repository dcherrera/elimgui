/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_input_shortcut.h
 * @brief Predefined edit chords (copy/cut/paste/undo/redo/select-all, on
 *        ELI_MOD_SHORTCUT) and clipboard access for elimgui, plus
 *        clipboard get/set with a pluggable callback, a browser JS-interop path
 *        (ELI_JSIO), and an internal static-buffer fallback for hosted builds.
 *
 * @status In use. eli_shortcut / eli_shortcut_ex (routing, key ownership) live in
 *         interaction/eli_input_routing.h, above the window system they need.
 * @issues None
 * @todo None
 */
#ifndef ELI_INPUT_ELI_INPUT_SHORTCUT_H
#define ELI_INPUT_ELI_INPUT_SHORTCUT_H

#include "eli_input_keyboard.h"
#include "../util/eli_mem.h"

/* ---------------------------------------------------------------------------
 * Predefined edit shortcuts: ELI_MOD_SHORTCUT = Cmd on macOS, Ctrl elsewhere.
 * Redo is Shortcut+Shift+Z everywhere, plus Ctrl+Y off macOS (both accepted by
 * eli_is_redo_chord_pressed).
 * ------------------------------------------------------------------------- */

#define ELI_SHORTCUT_COPY       (ELI_MOD_SHORTCUT | ELI_KEY_C)
#define ELI_SHORTCUT_CUT        (ELI_MOD_SHORTCUT | ELI_KEY_X)
#define ELI_SHORTCUT_PASTE      (ELI_MOD_SHORTCUT | ELI_KEY_V)
#define ELI_SHORTCUT_UNDO       (ELI_MOD_SHORTCUT | ELI_KEY_Z)
#define ELI_SHORTCUT_REDO       (ELI_MOD_SHORTCUT | ELI_MOD_SHIFT | ELI_KEY_Z)
#define ELI_SHORTCUT_REDO_ALT   (ELI_MOD_CTRL | ELI_KEY_Y)
#define ELI_SHORTCUT_SELECT_ALL (ELI_MOD_SHORTCUT | ELI_KEY_A)

/** @return true if a redo chord fired (Shortcut+Shift+Z, or Ctrl+Y off macOS). */
static inline bool eli_is_redo_chord_pressed(void)
{
    const eli_io *io = eli_get_io();
    if (!io)
        return false;
    return eli_is_key_chord_pressed(ELI_SHORTCUT_REDO) ||
           (!io->config_mac_os_behaviors && eli_is_key_chord_pressed(ELI_SHORTCUT_REDO_ALT));
}


/* ---------------------------------------------------------------------------
 * Clipboard
 *
 * Resolution order for both get and set:
 *   1. If the IO clipboard callbacks are set, they are used.
 *   2. Otherwise an internal heap buffer (eli_mem, grows to fit, no cap) holds the
 *      text. Under ELI_JSIO the
 *      set path also forwards to the host page (eli_host_set_clipboard) and the
 *      host page delivers OS clipboard text back via eli_host_provide_clipboard.
 * ------------------------------------------------------------------------- */

/* Fallback clipboard storage: heap (eli_mem), grown to fit, never truncated. */
ELI_GLOBAL(char *, eli_clipboard_fallback_buffer, NULL);
ELI_GLOBAL(size_t, eli_clipboard_fallback_capacity, 0);

/** Copy text into the fallback buffer, growing it to fit (no cap). On OOM the
 *  previous text is kept. */
static inline void eli_clipboard_store_fallback(const char *text)
{
    size_t n = text ? strlen(text) : 0;
    if (n + 1 > eli_clipboard_fallback_capacity) {
        char *grown = (char *)eli_mem_alloc(n + 1);
        if (!grown)
            return;
        eli_mem_free(eli_clipboard_fallback_buffer);
        eli_clipboard_fallback_buffer = grown;
        eli_clipboard_fallback_capacity = n + 1;
    }
    if (n > 0)
        memcpy(eli_clipboard_fallback_buffer, text, n);
    eli_clipboard_fallback_buffer[n] = '\0';
}

/** Free the fallback clipboard buffer (called on context destroy via eli_frame.h). */
static inline void eli_clipboard_shutdown(void)
{
    eli_mem_free(eli_clipboard_fallback_buffer);
    eli_clipboard_fallback_buffer = NULL;
    eli_clipboard_fallback_capacity = 0;
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
    if (io && io->get_clipboard_text_fn) {
        const char *t = io->get_clipboard_text_fn(io->clipboard_user_data);
        return t ? t : "";
    }
    return eli_clipboard_fallback_buffer ? eli_clipboard_fallback_buffer : "";
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
    eli_host_set_clipboard(eli_get_clipboard_text());
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
