/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_input_text.h
 * @brief Text input for elimgui: typed characters become ordered TEXT events
 *        (UTF-32 code points) in the input queue, decoded from UTF-8 or UTF-16.
 *
 * At begin-frame the TEXT events are applied, in order with key and mouse events,
 * into io.input_queue_characters (UTF-32, heap, no cap); widgets consume it during
 * the frame and it is cleared at end-frame. Emoji and other non-BMP characters are
 * preserved; only invalid input becomes U+FFFD.
 *
 * @status Phase 4 text queue in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_INPUT_ELI_INPUT_TEXT_H
#define ELI_INPUT_ELI_INPUT_TEXT_H

#include "../core/eli_core.h"
#include "../util/eli_mem.h"

#include "eli_input_queue.h"

/* Unicode replacement character, stored for non-BMP or malformed input. */
#define ELI_UNICODE_REPLACEMENT 0xFFFDu

/* ---------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

/**
 * Decode one UTF-8 code point from a buffer.
 *
 * @param s        Input bytes (NUL-terminated).
 * @param out_cp   Receives the decoded code point (or U+FFFD on error).
 * @return         Number of bytes consumed (>= 1); advances past one byte on a
 *                 malformed sequence so callers make progress.
 */
static inline int eli_text_decode_utf8(const char *s, unsigned int *out_cp)
{
    const unsigned char *b = (const unsigned char *)s;
    unsigned int c0 = b[0];

    if (c0 < 0x80u) {
        *out_cp = c0;
        return 1;
    }
    if ((c0 & 0xE0u) == 0xC0u && (b[1] & 0xC0u) == 0x80u) {
        *out_cp = ((c0 & 0x1Fu) << 6) | (b[1] & 0x3Fu);
        return 2;
    }
    if ((c0 & 0xF0u) == 0xE0u && (b[1] & 0xC0u) == 0x80u && (b[2] & 0xC0u) == 0x80u) {
        *out_cp = ((c0 & 0x0Fu) << 12) | ((b[1] & 0x3Fu) << 6) | (b[2] & 0x3Fu);
        return 3;
    }
    if ((c0 & 0xF8u) == 0xF0u && (b[1] & 0xC0u) == 0x80u &&
        (b[2] & 0xC0u) == 0x80u && (b[3] & 0xC0u) == 0x80u) {
        *out_cp = ((c0 & 0x07u) << 18) | ((b[1] & 0x3Fu) << 12) |
                  ((b[2] & 0x3Fu) << 6) | (b[3] & 0x3Fu);
        return 4;
    }

    *out_cp = ELI_UNICODE_REPLACEMENT;
    return 1;
}

/** @return cp if it is a valid Unicode scalar value, else U+FFFD. */
static inline uint32_t eli_text_sanitize_cp(uint32_t cp)
{
    if (cp > 0x10FFFFu || (cp >= 0xD800u && cp <= 0xDFFFu))
        return ELI_UNICODE_REPLACEMENT;
    return cp;
}

/**
 * Append one code point to this frame's character queue, growing it on the heap.
 * Called at begin-frame when TEXT events are applied.
 *
 * @return false if the queue could not grow (the character is dropped).
 */
static inline bool eli_io_push_input_char(eli_io *io, uint32_t c)
{
    if (io->input_queue_characters_count >= io->input_queue_characters_capacity) {
        int cap = io->input_queue_characters_capacity > 0 ? io->input_queue_characters_capacity * 2
                                                          : ELI_INPUT_QUEUE_CHAR_SIZE;
        uint32_t *grown = (uint32_t *)eli_mem_realloc(
            io->input_queue_characters,
            (size_t)io->input_queue_characters_capacity * sizeof(*grown),
            (size_t)cap * sizeof(*grown));
        if (!grown)
            return false;
        io->input_queue_characters = grown;
        io->input_queue_characters_capacity = cap;
    }
    io->input_queue_characters[io->input_queue_characters_count++] = c;
    return true;
}

/** Queue one TEXT event carrying a (sanitized) code point. */
static inline void eli_io__queue_text_event(eli_io *io, uint32_t cp)
{
    eli_input_event *e = eli_input_push_event(io, ELI_INPUT_EVENT_TYPE_TEXT);
    if (e)
        e->data.text.c = eli_text_sanitize_cp(cp);
}

/* ---------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

/**
 * Queue one typed character (a Unicode code point) as an ordered TEXT event.
 * Zero is ignored; surrogates and values past U+10FFFF become U+FFFD. Non-BMP
 * characters (emoji) are kept as-is.
 *
 * @param c  Unicode code point of the typed character.
 *
 * Thread-safe: no (mutates current-context IO)
 * Reentrant: yes
 */
static inline void eli_io_add_input_character(unsigned int c)
{
    eli_io *io = eli_get_io();
    if (!io || c == 0)
        return;
    eli_io__queue_text_event(io, (uint32_t)c);
}

/**
 * Queue one UTF-16 code unit (browsers, some IMEs). A high surrogate is held
 * until its low surrogate arrives and the pair becomes one code point; an
 * unpaired surrogate becomes U+FFFD.
 *
 * @param c  UTF-16 code unit.
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline void eli_io_add_input_character_utf16(uint16_t c)
{
    eli_io *io = eli_get_io();
    if (!io || c == 0)
        return;
    if (c >= 0xD800u && c <= 0xDBFFu) {          /* high surrogate */
        if (io->input_queue_surrogate != 0)
            eli_io__queue_text_event(io, ELI_UNICODE_REPLACEMENT);
        io->input_queue_surrogate = c;
        return;
    }
    uint32_t cp = c;
    if (c >= 0xDC00u && c <= 0xDFFFu) {          /* low surrogate */
        cp = (io->input_queue_surrogate != 0)
                 ? 0x10000u + (((uint32_t)io->input_queue_surrogate - 0xD800u) << 10) + (c - 0xDC00u)
                 : ELI_UNICODE_REPLACEMENT;
    } else if (io->input_queue_surrogate != 0) {
        eli_io__queue_text_event(io, ELI_UNICODE_REPLACEMENT);
    }
    io->input_queue_surrogate = 0;
    eli_io__queue_text_event(io, cp);
}

/**
 * Queue a UTF-8 string of typed characters, one ordered TEXT event per code
 * point (malformed bytes become U+FFFD). No length limit: a paste or dictation
 * of any size arrives whole.
 *
 * @param utf8  NUL-terminated UTF-8 text (NULL is ignored).
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_io_add_input_characters_utf8(const char *utf8)
{
    eli_io *io = eli_get_io();
    if (!io || !utf8)
        return;

    while (*utf8) {
        unsigned int cp = 0;
        int n = eli_text_decode_utf8(utf8, &cp);
        if (cp != 0)
            eli_io__queue_text_event(io, (uint32_t)cp);
        utf8 += n;
    }
}

#endif /* ELI_INPUT_ELI_INPUT_TEXT_H */
