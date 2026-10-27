/**
 * @file eli_input_text_edit.h
 * @brief Editing core of the input-text family: flags and callback contract, the
 *        single active-field state, UTF-8 buffer helpers, char filtering, and the
 *        per-frame edit operations (typed characters, erase, caret navigation,
 *        clipboard/undo chords).
 *
 * Typed text arrives as UTF-32 code points (io.input_queue_characters) and is
 * stored UTF-8 (1-4 bytes per code point, so emoji survive). Chords use
 * ELI_MOD_SHORTCUT (Cmd on macOS, Ctrl elsewhere). Copy/cut and paste have no
 * size cap; undo/revert snapshots are capped (ELI_INPUT_TEXT_SNAPSHOT_MAX) and an
 * edit of a longer buffer disables them instead of truncating.
 *
 * @status In use by eli_input_text_widget.h.
 * @issues None
 * @todo Heap snapshots / multi-level undo (spec D20, Phase I).
 */
#ifndef ELI_WIDGETS_ELI_INPUT_TEXT_EDIT_H
#define ELI_WIDGETS_ELI_INPUT_TEXT_EDIT_H

#include "eli_widget_behavior.h"
#include "eli_item_status.h"

#include "../core/eli_platform.h"
#include "../interaction/eli_input_routing.h"
#include "../util/eli_mem.h"

/* ---------------------------------------------------------------------------
 * Flags + callback contract
 * ------------------------------------------------------------------------- */

/** Behavior flags for the input-text family (mirrors Dear ImGui's set). */
typedef int eli_input_text_flags;
enum eli_input_text_flags_ {
    ELI_INPUT_TEXT_NONE                   = 0,
    ELI_INPUT_TEXT_CHARS_DECIMAL          = 1 << 0,   /* allow 0-9 . + - * / */
    ELI_INPUT_TEXT_CHARS_HEXADECIMAL      = 1 << 1,   /* allow 0-9 a-f A-F */
    ELI_INPUT_TEXT_CHARS_SCIENTIFIC       = 1 << 2,   /* decimal plus e E */
    ELI_INPUT_TEXT_CHARS_UPPERCASE        = 1 << 3,   /* map a-z to A-Z */
    ELI_INPUT_TEXT_CHARS_NO_BLANK         = 1 << 4,   /* filter spaces/tabs */
    ELI_INPUT_TEXT_ALLOW_TAB_INPUT        = 1 << 5,   /* insert a tab character */
    ELI_INPUT_TEXT_ENTER_RETURNS_TRUE     = 1 << 6,   /* return true only on Enter */
    ELI_INPUT_TEXT_ESCAPE_CLEARS_ALL      = 1 << 7,   /* Escape empties instead of revert */
    ELI_INPUT_TEXT_CTRL_ENTER_FOR_NEWLINE = 1 << 8,   /* swap Enter/Ctrl+Enter roles */
    ELI_INPUT_TEXT_READ_ONLY              = 1 << 9,   /* no editing */
    ELI_INPUT_TEXT_PASSWORD               = 1 << 10,  /* render glyphs as '*' */
    ELI_INPUT_TEXT_ALWAYS_OVERWRITE       = 1 << 11,
    ELI_INPUT_TEXT_AUTO_SELECT_ALL        = 1 << 12,  /* select all on activation */
    ELI_INPUT_TEXT_PARSE_EMPTY_REF_VAL    = 1 << 13,
    ELI_INPUT_TEXT_DISPLAY_EMPTY_REF_VAL  = 1 << 14,
    ELI_INPUT_TEXT_NO_HORIZONTAL_SCROLL   = 1 << 15,
    ELI_INPUT_TEXT_NO_UNDO_REDO           = 1 << 16,
    ELI_INPUT_TEXT_ELIDE_LEFT             = 1 << 17,
    ELI_INPUT_TEXT_CALLBACK_COMPLETION    = 1 << 18,
    ELI_INPUT_TEXT_CALLBACK_HISTORY       = 1 << 19,
    ELI_INPUT_TEXT_CALLBACK_ALWAYS        = 1 << 20,
    ELI_INPUT_TEXT_CALLBACK_CHAR_FILTER   = 1 << 21,
    ELI_INPUT_TEXT_CALLBACK_RESIZE        = 1 << 22,
    ELI_INPUT_TEXT_CALLBACK_EDIT          = 1 << 23
};

struct eli_context;

/** Callback data passed to an eli_input_text_callback for a filter/edit event. */
typedef struct eli_input_text_callback_data {
    struct eli_context *ctx;
    eli_input_text_flags event_flag;   /* which callback event this is */
    eli_input_text_flags flags;        /* flags passed to the widget */
    void *user_data;

    uint32_t event_char;               /* CharFilter: code point; rewrite (or 0 to drop) */

    eli_key event_key;                 /* Completion/History key */
    char *buf;                         /* live edit buffer */
    int buf_text_len;                  /* current length in bytes */
    int buf_size;                      /* buffer capacity */
    bool buf_dirty;                    /* set true if the callback edited buf */
    int cursor_pos;                    /* caret byte offset */
    int selection_start;               /* selection anchor byte offset */
    int selection_end;                 /* selection end byte offset */
} eli_input_text_callback_data;

/** User callback for filter/completion/history/edit events; returns 0. */
typedef int (*eli_input_text_callback)(eli_input_text_callback_data *data);

/* ---------------------------------------------------------------------------
 * Module-private editing state (single active field at a time)
 * ------------------------------------------------------------------------- */

/* Cap on the undo/revert snapshots and the password display buffer. Longer
 * buffers still edit, copy and paste in full; undo and escape-revert are then
 * disabled for that edit (never truncated: a truncated revert would lose data),
 * and password display shows at most this many asterisks. */
#ifndef ELI_INPUT_TEXT_SNAPSHOT_MAX
#define ELI_INPUT_TEXT_SNAPSHOT_MAX 256
#endif

typedef struct eli_input_text_state {
    eli_id id;               /* active field id (0 = none active) */
    int cursor;              /* caret byte offset */
    int selection_anchor;    /* selection anchor byte offset */
    float scroll_x;          /* horizontal display scroll in pixels */
    double cursor_anim_base; /* time base for caret blink (reset on activity) */

    char initial[ELI_INPUT_TEXT_SNAPSHOT_MAX];  /* buffer at activation */
    int initial_len;
    bool initial_valid;      /* false when the buffer was too long to snapshot */

    char undo[ELI_INPUT_TEXT_SNAPSHOT_MAX];     /* one-level undo snapshot */
    int undo_len;
    int undo_cursor;
    bool has_undo;
} eli_input_text_state;

ELI_GLOBAL(eli_input_text_state, g_eli_input_text_state, {0});

/* Scratch buffer for the password/display render pass. */
ELI_GLOBAL_ARRAY(char, g_eli_input_text_display, ELI_INPUT_TEXT_SNAPSHOT_MAX);

/* ---------------------------------------------------------------------------
 * Byte/codepoint + buffer helpers
 * ------------------------------------------------------------------------- */

/** Bounded strlen: length of buf up to at most cap-1 bytes. */
static inline int eli_input_text__strnlen(const char *buf, int cap)
{
    int n = 0;
    while (n < cap - 1 && buf[n] != '\0')
        n++;
    return n;
}

/** @return true if the byte at pos is a UTF-8 continuation byte. */
static inline bool eli_input_text__is_cont(const char *buf, int pos)
{
    return ((unsigned char)buf[pos] & 0xC0u) == 0x80u;
}

/** @return the byte offset of the codepoint before pos (clamped to 0). */
static inline int eli_input_text__prev_cp(const char *buf, int pos)
{
    if (pos <= 0)
        return 0;
    pos--;
    while (pos > 0 && eli_input_text__is_cont(buf, pos))
        pos--;
    return pos;
}

/** @return the byte offset of the codepoint after pos (clamped to len). */
static inline int eli_input_text__next_cp(const char *buf, int len, int pos)
{
    if (pos >= len)
        return len;
    pos++;
    while (pos < len && eli_input_text__is_cont(buf, pos))
        pos++;
    return pos;
}

/** Encode a code point as UTF-8 into out[0..3]; @return bytes written (1-4). */
static inline int eli_input_text__encode_utf8(uint32_t cp, char out[4])
{
    if (cp < 0x80u) {
        out[0] = (char)cp;
        return 1;
    }
    if (cp < 0x800u) {
        out[0] = (char)(0xC0u | (cp >> 6));
        out[1] = (char)(0x80u | (cp & 0x3Fu));
        return 2;
    }
    if (cp < 0x10000u) {
        out[0] = (char)(0xE0u | (cp >> 12));
        out[1] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
        out[2] = (char)(0x80u | (cp & 0x3Fu));
        return 3;
    }
    out[0] = (char)(0xF0u | (cp >> 18));
    out[1] = (char)(0x80u | ((cp >> 12) & 0x3Fu));
    out[2] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
    out[3] = (char)(0x80u | (cp & 0x3Fu));
    return 4;
}

/** @return the low selection bound (min of caret and anchor). */
static inline int eli_input_text__sel_min(const eli_input_text_state *st)
{
    return st->cursor < st->selection_anchor ? st->cursor : st->selection_anchor;
}

/** @return the high selection bound (max of caret and anchor). */
static inline int eli_input_text__sel_max(const eli_input_text_state *st)
{
    return st->cursor > st->selection_anchor ? st->cursor : st->selection_anchor;
}

/** @return true if a non-empty selection is active. */
static inline bool eli_input_text__has_selection(const eli_input_text_state *st)
{
    return st->cursor != st->selection_anchor;
}

/** Save a one-level undo snapshot (or disable undo if the buffer is too long). */
static inline void eli_input_text__save_undo(eli_input_text_state *st, const char *buf, int len)
{
    if (len >= ELI_INPUT_TEXT_SNAPSHOT_MAX) {
        st->has_undo = false;    /* an older snapshot would undo the wrong edit */
        return;
    }
    memcpy(st->undo, buf, (size_t)len);
    st->undo[len] = '\0';
    st->undo_len = len;
    st->undo_cursor = st->cursor;
    st->has_undo = true;
}

/** Delete bytes [a,b) from buf, keeping it NUL-terminated. */
static inline void eli_input_text__delete_range(char *buf, int *len, int a, int b)
{
    if (a > b) {
        int t = a;
        a = b;
        b = t;
    }
    if (a < 0)
        a = 0;
    if (b > *len)
        b = *len;
    if (a >= b)
        return;
    memmove(buf + a, buf + b, (size_t)(*len - b));
    *len -= (b - a);
    buf[*len] = '\0';
}

/** Insert n bytes of s at offset at (truncating to capacity). @return bytes added. */
static inline int eli_input_text__insert(char *buf, int *len, int cap, int at, const char *s, int n)
{
    if (*len + n >= cap)
        n = cap - 1 - *len;
    if (n <= 0)
        return 0;
    memmove(buf + at + n, buf + at, (size_t)(*len - at));
    memcpy(buf + at, s, (size_t)n);
    *len += n;
    buf[*len] = '\0';
    return n;
}

/** Delete the current selection if any; @return true if something was removed. */
static inline bool eli_input_text__delete_selection(eli_input_text_state *st, char *buf, int *len)
{
    if (!eli_input_text__has_selection(st))
        return false;
    int a = eli_input_text__sel_min(st);
    int b = eli_input_text__sel_max(st);
    eli_input_text__delete_range(buf, len, a, b);
    st->cursor = a;
    st->selection_anchor = a;
    return true;
}

/** Move the caret to pos, collapsing the selection unless shift is held. */
static inline void eli_input_text__move_caret(eli_input_text_state *st, int pos, bool keep_sel)
{
    st->cursor = pos;
    if (!keep_sel)
        st->selection_anchor = pos;
}

/* ---------------------------------------------------------------------------
 * Character filtering
 * ------------------------------------------------------------------------- */

/**
 * Apply the char-filter flags to one typed code point.
 *
 * @param c      Typed code point (UTF-32).
 * @param flags  Active input-text flags.
 * @return       The (possibly rewritten) code point, or 0 to reject it.
 */
static inline uint32_t eli_input_text__filter(uint32_t c, eli_input_text_flags flags)
{
    if (c == '\t')
        return (flags & ELI_INPUT_TEXT_ALLOW_TAB_INPUT) ? c : 0;
    if (c < 0x20u)
        return 0;

    if (flags & ELI_INPUT_TEXT_CHARS_UPPERCASE) {
        if (c >= 'a' && c <= 'z')
            c = c - 'a' + 'A';
    }
    if (flags & ELI_INPUT_TEXT_CHARS_NO_BLANK) {
        if (c == ' ' || c == '\t')
            return 0;
    }
    if (flags & ELI_INPUT_TEXT_CHARS_DECIMAL) {
        bool ok = (c >= '0' && c <= '9') || c == '.' || c == '+' || c == '-' ||
                  c == '*' || c == '/';
        if (!ok)
            return 0;
    }
    if (flags & ELI_INPUT_TEXT_CHARS_SCIENTIFIC) {
        bool ok = (c >= '0' && c <= '9') || c == '.' || c == '+' || c == '-' ||
                  c == '*' || c == '/' || c == 'e' || c == 'E';
        if (!ok)
            return 0;
    }
    if (flags & ELI_INPUT_TEXT_CHARS_HEXADECIMAL) {
        bool ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
                  (c >= 'A' && c <= 'F') || c == '+' || c == '-';
        if (!ok)
            return 0;
    }
    return c;
}

/* ---------------------------------------------------------------------------
 * Editing: character insertion and key handling
 * ------------------------------------------------------------------------- */

/** Consume the IO character queue into buf; @return true if the buffer changed. */
static inline bool eli_input_text__process_chars(eli_input_text_state *st, char *buf, int *len,
                                                 int buf_size, eli_input_text_flags flags)
{
    eli_io *io = eli_get_io();
    if (io == NULL)
        return false;

    bool changed = false;
    for (int i = 0; i < io->input_queue_characters_count; i++) {
        uint32_t c = eli_input_text__filter(io->input_queue_characters[i], flags);
        if (c == 0)
            continue;
        if (!changed)
            eli_input_text__save_undo(st, buf, *len);
        eli_input_text__delete_selection(st, buf, len);
        char enc[4];
        int n = eli_input_text__encode_utf8(c, enc);
        int added = eli_input_text__insert(buf, len, buf_size, st->cursor, enc, n);
        if (added > 0) {
            eli_input_text__move_caret(st, st->cursor + added, false);
            changed = true;
        }
    }
    if (changed)
        io->input_queue_characters_count = 0;
    return changed;
}

/** Handle Backspace/Delete; @return true if the buffer changed. */
static inline bool eli_input_text__process_erase(eli_input_text_state *st, char *buf, int *len)
{
    bool changed = false;
    if (eli_is_key_pressed_ex(ELI_KEY_BACKSPACE, true)) {
        eli_input_text__save_undo(st, buf, *len);
        if (!eli_input_text__delete_selection(st, buf, len) && st->cursor > 0) {
            int p = eli_input_text__prev_cp(buf, st->cursor);
            eli_input_text__delete_range(buf, len, p, st->cursor);
            eli_input_text__move_caret(st, p, false);
        }
        changed = true;
    }
    if (eli_is_key_pressed_ex(ELI_KEY_DELETE, true)) {
        eli_input_text__save_undo(st, buf, *len);
        if (!eli_input_text__delete_selection(st, buf, len) && st->cursor < *len) {
            int nx = eli_input_text__next_cp(buf, *len, st->cursor);
            eli_input_text__delete_range(buf, len, st->cursor, nx);
        }
        changed = true;
    }
    return changed;
}

/** @return true for bytes that belong to a "word" (alnum, '_', any non-ASCII). */
static inline bool eli_input_text__is_word_byte(char c)
{
    unsigned char u = (unsigned char)c;
    return u >= 0x80u || u == '_' || (u >= '0' && u <= '9') || (u >= 'a' && u <= 'z') ||
           (u >= 'A' && u <= 'Z');
}

/** @return the start of the word before pos (skips separators first). */
static inline int eli_input_text__word_left(const char *buf, int pos)
{
    while (pos > 0 && !eli_input_text__is_word_byte(buf[pos - 1]))
        pos--;
    while (pos > 0 && eli_input_text__is_word_byte(buf[pos - 1]))
        pos--;
    return pos;
}

/** @return the end of the word at/after pos (skips the word, then separators). */
static inline int eli_input_text__word_right(const char *buf, int len, int pos)
{
    while (pos < len && eli_input_text__is_word_byte(buf[pos]))
        pos++;
    while (pos < len && !eli_input_text__is_word_byte(buf[pos]))
        pos++;
    return pos;
}

/** @return the start of the line containing pos. */
static inline int eli_input_text__line_start(const char *buf, int pos)
{
    while (pos > 0 && buf[pos - 1] != '\n')
        pos--;
    return pos;
}

/** @return the end of the line containing pos. */
static inline int eli_input_text__line_end(const char *buf, int len, int pos)
{
    while (pos < len && buf[pos] != '\n')
        pos++;
    return pos;
}

/**
 * Left/Right with modifiers. macOS: Cmd = line start/end, Alt = word. Elsewhere:
 * Ctrl = word. @return the new caret position.
 */
static inline int eli_input_text__horizontal_target(const eli_io *io, const char *buf, int len,
                                                    int pos, bool right)
{
    bool word = io->config_mac_os_behaviors ? io->key_alt : io->key_ctrl;
    if (io->config_mac_os_behaviors && io->key_super)
        return right ? eli_input_text__line_end(buf, len, pos) : eli_input_text__line_start(buf, pos);
    if (word)
        return right ? eli_input_text__word_right(buf, len, pos) : eli_input_text__word_left(buf, pos);
    return right ? eli_input_text__next_cp(buf, len, pos) : eli_input_text__prev_cp(buf, pos);
}

/**
 * Handle caret navigation: arrows (with word/line modifiers), Home/End (line;
 * Ctrl+Home/End = buffer elsewhere), Cmd+Up/Down = buffer start/end on macOS,
 * Up/Down between lines in multiline. Shift extends the selection.
 */
static inline void eli_input_text__process_nav(eli_input_text_state *st, char *buf, int len,
                                               bool multiline)
{
    const eli_io *io = eli_get_io();
    bool shift = io->key_shift;
    bool mac = io->config_mac_os_behaviors;
    if (eli_is_key_pressed_ex(ELI_KEY_LEFT_ARROW, true))
        eli_input_text__move_caret(st, eli_input_text__horizontal_target(io, buf, len, st->cursor,
                                                                         false), shift);
    if (eli_is_key_pressed_ex(ELI_KEY_RIGHT_ARROW, true))
        eli_input_text__move_caret(st, eli_input_text__horizontal_target(io, buf, len, st->cursor,
                                                                         true), shift);
    if (eli_is_key_pressed(ELI_KEY_HOME))
        eli_input_text__move_caret(st, (!mac && io->key_ctrl) ? 0
                                       : eli_input_text__line_start(buf, st->cursor), shift);
    if (eli_is_key_pressed(ELI_KEY_END))
        eli_input_text__move_caret(st, (!mac && io->key_ctrl) ? len
                                       : eli_input_text__line_end(buf, len, st->cursor), shift);
    bool up = eli_is_key_pressed_ex(ELI_KEY_UP_ARROW, true);
    bool down = eli_is_key_pressed_ex(ELI_KEY_DOWN_ARROW, true);
    if (mac && io->key_super && (up || down)) {
        eli_input_text__move_caret(st, up ? 0 : len, shift);
        return;
    }
    if (multiline && up) {
        int p = eli_input_text__line_start(buf, st->cursor);
        if (p > 0)
            eli_input_text__move_caret(st, p - 1, shift);
    }
    if (multiline && down) {
        int p = eli_input_text__line_end(buf, len, st->cursor);
        if (p < len)
            eli_input_text__move_caret(st, eli_input_text__next_cp(buf, len, p), shift);
    }
}

/**
 * Copy the selection (or the whole buffer) to the clipboard through a heap
 * temporary: no size cap. Logs nothing on OOM; the clipboard is left unchanged.
 */
static inline void eli_input_text__copy(const eli_input_text_state *st, const char *buf, int len)
{
    int a = eli_input_text__has_selection(st) ? eli_input_text__sel_min(st) : 0;
    int b = eli_input_text__has_selection(st) ? eli_input_text__sel_max(st) : len;
    char *tmp = (char *)eli_mem_alloc((size_t)(b - a) + 1);
    if (!tmp)
        return;
    memcpy(tmp, buf + a, (size_t)(b - a));
    tmp[b - a] = '\0';
    eli_set_clipboard_text(tmp);
    eli_mem_free(tmp);
}

/**
 * Insert clipboard text at the caret: decode UTF-8, filter per code point, then
 * insert once (a large paste is one memmove, not one per character).
 * @return true if anything was inserted.
 */
static inline bool eli_input_text__paste(eli_input_text_state *st, char *buf, int *len,
                                         int buf_size, eli_input_text_flags flags)
{
    const char *clip = eli_get_clipboard_text();
    if (clip == NULL || clip[0] == '\0')
        return false;
    size_t clip_len = strlen(clip);
    char *filtered = (char *)eli_mem_alloc(clip_len + 1);   /* filtering never grows UTF-8 */
    if (!filtered)
        return false;
    int n = 0;
    for (const char *p = clip; *p != '\0';) {
        unsigned int cp = 0;
        p += eli_text_decode_utf8(p, &cp);
        uint32_t c = eli_input_text__filter((uint32_t)cp, flags);
        if (c != 0)
            n += eli_input_text__encode_utf8(c, filtered + n);
    }
    eli_input_text__save_undo(st, buf, *len);
    eli_input_text__delete_selection(st, buf, len);
    int added = eli_input_text__insert(buf, len, buf_size, st->cursor, filtered, n);
    eli_mem_free(filtered);
    st->cursor += added;
    st->selection_anchor = st->cursor;
    return added > 0;
}

/** Handle clipboard + select-all + undo chords; @return true if buffer changed. */
static inline bool eli_input_text__process_clipboard(eli_input_text_state *st, char *buf, int *len,
                                                     int buf_size, eli_input_text_flags flags)
{
    bool changed = false;
    bool read_only = (flags & ELI_INPUT_TEXT_READ_ONLY) != 0;
    if (eli_is_key_chord_pressed(ELI_SHORTCUT_SELECT_ALL)) {
        st->selection_anchor = 0;
        st->cursor = *len;
    }
    bool cut = eli_is_key_chord_pressed(ELI_SHORTCUT_CUT);
    if (eli_is_key_chord_pressed(ELI_SHORTCUT_COPY) || cut)
        eli_input_text__copy(st, buf, *len);
    if (cut && !read_only) {
        eli_input_text__save_undo(st, buf, *len);
        if (!eli_input_text__delete_selection(st, buf, len)) {
            *len = 0;
            buf[0] = '\0';
            st->cursor = 0;
            st->selection_anchor = 0;
        }
        changed = true;
    }
    if (eli_is_key_chord_pressed(ELI_SHORTCUT_PASTE) && !read_only)
        changed = eli_input_text__paste(st, buf, len, buf_size, flags) || changed;
    if (!(flags & ELI_INPUT_TEXT_NO_UNDO_REDO) && eli_is_key_chord_pressed(ELI_SHORTCUT_UNDO) &&
        st->has_undo && st->undo_len < buf_size) {
        memcpy(buf, st->undo, (size_t)st->undo_len);
        buf[st->undo_len] = '\0';
        *len = st->undo_len;
        st->cursor = st->undo_cursor > *len ? *len : st->undo_cursor;
        st->selection_anchor = st->cursor;
        st->has_undo = false;
        changed = true;
    }
    return changed;
}

/**
 * Claim the keys an active field edits with (spec §9.3), so GLOBAL/FOCUSED app
 * shortcuts on them do not fire while typing. Renewed every active frame.
 */
static inline void eli_input_text__claim_keys(eli_id id, eli_input_text_flags flags)
{
    static const eli_key edit_keys[] = {
        ELI_KEY_LEFT_ARROW, ELI_KEY_RIGHT_ARROW, ELI_KEY_UP_ARROW, ELI_KEY_DOWN_ARROW,
        ELI_KEY_HOME, ELI_KEY_END, ELI_KEY_BACKSPACE, ELI_KEY_DELETE, ELI_KEY_ENTER,
        ELI_KEY_KEYPAD_ENTER, ELI_KEY_ESCAPE,
        ELI_KEY_A, ELI_KEY_C, ELI_KEY_V, ELI_KEY_X, ELI_KEY_Y, ELI_KEY_Z  /* edit chords */
    };
    for (size_t i = 0; i < sizeof(edit_keys) / sizeof(edit_keys[0]); i++)
        eli_set_key_owner(edit_keys[i], id);
    if (flags & ELI_INPUT_TEXT_ALLOW_TAB_INPUT)
        eli_set_key_owner(ELI_KEY_TAB, id);
}

#endif /* ELI_WIDGETS_ELI_INPUT_TEXT_EDIT_H */
