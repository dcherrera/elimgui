/**
 * @file eli_input_text.h
 * @brief Text-input queue for elimgui: appends typed characters (as UTF-16 code
 *        units) to the per-frame IO character queue, decoding UTF-8 input.
 *
 * Widgets consume io.input_queue_characters during the frame; the queue is
 * cleared at end-frame by eli_input_backend.h. Code points outside the Basic
 * Multilingual Plane are stored as the U+FFFD replacement character (the queue
 * holds 16-bit code units).
 *
 * @status Phase 4 text queue in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_INPUT_ELI_INPUT_TEXT_H
#define ELI_INPUT_ELI_INPUT_TEXT_H

#include "../core/eli_core.h"

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

/** Append one 16-bit code unit to the IO character queue if space remains. */
static inline void eli_io_push_input_char_u16(eli_io *io, uint16_t c)
{
    if (io->input_queue_characters_count < ELI_INPUT_QUEUE_CHAR_SIZE)
        io->input_queue_characters[io->input_queue_characters_count++] = c;
}

/* ---------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

/**
 * Append a single typed character (Unicode code point) to the input queue.
 * Zero is ignored; non-BMP code points are stored as U+FFFD.
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
    eli_io_push_input_char_u16(io, (c <= 0xFFFFu) ? (uint16_t)c : (uint16_t)ELI_UNICODE_REPLACEMENT);
}

/**
 * Append a UTF-8 string of typed characters to the input queue, decoding each
 * code point.
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
            eli_io_push_input_char_u16(io, (cp <= 0xFFFFu) ? (uint16_t)cp
                                                           : (uint16_t)ELI_UNICODE_REPLACEMENT);
        utf8 += n;
    }
}

#endif /* ELI_INPUT_ELI_INPUT_TEXT_H */
