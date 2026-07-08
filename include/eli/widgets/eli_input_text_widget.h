/**
 * @file eli_input_text_widget.h
 * @brief Phase 12 text-input widgets: eli_input_text (single-line), _multiline,
 *        and _with_hint, plus the eli_input_text_flags set and the
 *        eli_input_text_callback_data / callback contract. Provides a self-contained
 *        single-line text editor (caret, insert/delete, arrow/home/end navigation,
 *        shift-selection, ctrl+A, copy/cut/paste via the clipboard API, char
 *        filtering, password display, one-level undo, escape-revert).
 *
 * The editing state for the one active field is module-private and file-static
 * (a single field edits at a time): id, caret, selection anchor, scroll, undo and
 * revert snapshots. A click sets the active id and snapshots the buffer; typing
 * mutates the caller's buffer in place and calls eli_mark_item_edited; Enter (single
 * line) or a click elsewhere commits and deactivates; Escape reverts. Mirrors Dear
 * ImGui's InputTextEx contract for the common single-line path.
 *
 * @status Phase 12 text input in use. Multiline shares the editor core; multiline
 *         selection-rectangle rendering is simplified to the caret line.
 * @issues None
 * @todo None
 */
#ifndef ELI_WIDGETS_ELI_INPUT_TEXT_WIDGET_H
#define ELI_WIDGETS_ELI_INPUT_TEXT_WIDGET_H

#include "eli_widget_behavior.h"
#include "eli_item_status.h"

#include "../core/eli_platform.h"

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

    uint16_t event_char;               /* CharFilter: rewrite (or 0 to drop) */

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

/* Cap on snapshot/display work buffers. Longer buffers still edit, but undo,
 * escape-revert, and password display beyond this cap are truncated. */
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

    char undo[ELI_INPUT_TEXT_SNAPSHOT_MAX];     /* one-level undo snapshot */
    int undo_len;
    int undo_cursor;
    bool has_undo;
} eli_input_text_state;

static eli_input_text_state g_eli_input_text_state;

/* Scratch buffer for the password/display render pass. */
static char g_eli_input_text_display[ELI_INPUT_TEXT_SNAPSHOT_MAX];

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

/** Encode a BMP code point as UTF-8 into out[0..2]; @return bytes written (1-3). */
static inline int eli_input_text__encode_utf8(uint32_t cp, char out[3])
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
    out[0] = (char)(0xE0u | (cp >> 12));
    out[1] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
    out[2] = (char)(0x80u | (cp & 0x3Fu));
    return 3;
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

/** Save a one-level undo snapshot of the current buffer/caret. */
static inline void eli_input_text__save_undo(eli_input_text_state *st, const char *buf, int len)
{
    if (len >= ELI_INPUT_TEXT_SNAPSHOT_MAX)
        return;
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
 * Apply the char-filter flags to one typed code unit.
 *
 * @param c      Typed UTF-16 code unit.
 * @param flags  Active input-text flags.
 * @return       The (possibly rewritten) code unit, or 0 to reject it.
 */
static inline uint16_t eli_input_text__filter(uint16_t c, eli_input_text_flags flags)
{
    if (c == '\t')
        return (flags & ELI_INPUT_TEXT_ALLOW_TAB_INPUT) ? c : 0;
    if (c < 0x20u)
        return 0;

    if (flags & ELI_INPUT_TEXT_CHARS_UPPERCASE) {
        if (c >= 'a' && c <= 'z')
            c = (uint16_t)(c - 'a' + 'A');
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
        uint16_t c = eli_input_text__filter(io->input_queue_characters[i], flags);
        if (c == 0)
            continue;
        if (!changed)
            eli_input_text__save_undo(st, buf, *len);
        eli_input_text__delete_selection(st, buf, len);
        char enc[3];
        int n = eli_input_text__encode_utf8((uint32_t)c, enc);
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

/** Handle caret navigation keys (arrows / home / end). */
static inline void eli_input_text__process_nav(eli_input_text_state *st, char *buf, int len,
                                               bool multiline)
{
    bool shift = eli_is_key_down(ELI_KEY_LEFT_SHIFT) || eli_is_key_down(ELI_KEY_RIGHT_SHIFT);
    if (eli_is_key_pressed_ex(ELI_KEY_LEFT_ARROW, true))
        eli_input_text__move_caret(st, eli_input_text__prev_cp(buf, st->cursor), shift);
    if (eli_is_key_pressed_ex(ELI_KEY_RIGHT_ARROW, true))
        eli_input_text__move_caret(st, eli_input_text__next_cp(buf, len, st->cursor), shift);
    if (eli_is_key_pressed(ELI_KEY_HOME)) {
        int p = st->cursor;
        while (p > 0 && buf[p - 1] != '\n')
            p--;
        eli_input_text__move_caret(st, p, shift);
    }
    if (eli_is_key_pressed(ELI_KEY_END)) {
        int p = st->cursor;
        while (p < len && buf[p] != '\n')
            p++;
        eli_input_text__move_caret(st, p, shift);
    }
    if (multiline && eli_is_key_pressed_ex(ELI_KEY_UP_ARROW, true)) {
        int p = st->cursor;
        while (p > 0 && buf[p - 1] != '\n')
            p--;
        if (p > 0)
            eli_input_text__move_caret(st, p - 1, shift);
    }
    if (multiline && eli_is_key_pressed_ex(ELI_KEY_DOWN_ARROW, true)) {
        int p = st->cursor;
        while (p < len && buf[p] != '\n')
            p++;
        if (p < len)
            eli_input_text__move_caret(st, eli_input_text__next_cp(buf, len, p), shift);
    }
}

/** Handle clipboard + select-all + undo chords; @return true if buffer changed. */
static inline bool eli_input_text__process_clipboard(eli_input_text_state *st, char *buf, int *len,
                                                     int buf_size, eli_input_text_flags flags)
{
    bool changed = false;
    if (eli_is_key_chord_pressed(ELI_SHORTCUT_SELECT_ALL)) {
        st->selection_anchor = 0;
        st->cursor = *len;
    }
    if (eli_is_key_chord_pressed(ELI_SHORTCUT_COPY) || eli_is_key_chord_pressed(ELI_SHORTCUT_CUT)) {
        int a = eli_input_text__has_selection(st) ? eli_input_text__sel_min(st) : 0;
        int b = eli_input_text__has_selection(st) ? eli_input_text__sel_max(st) : *len;
        char tmp[ELI_INPUT_TEXT_SNAPSHOT_MAX];
        int n = b - a;
        if (n >= ELI_INPUT_TEXT_SNAPSHOT_MAX)
            n = ELI_INPUT_TEXT_SNAPSHOT_MAX - 1;
        memcpy(tmp, buf + a, (size_t)n);
        tmp[n] = '\0';
        eli_set_clipboard_text(tmp);
        if (eli_is_key_chord_pressed(ELI_SHORTCUT_CUT) && !(flags & ELI_INPUT_TEXT_READ_ONLY)) {
            eli_input_text__save_undo(st, buf, *len);
            if (!eli_input_text__delete_selection(st, buf, len)) {
                *len = 0;
                buf[0] = '\0';
                st->cursor = 0;
                st->selection_anchor = 0;
            }
            changed = true;
        }
    }
    if (eli_is_key_chord_pressed(ELI_SHORTCUT_PASTE) && !(flags & ELI_INPUT_TEXT_READ_ONLY)) {
        const char *clip = eli_get_clipboard_text();
        if (clip != NULL && clip[0] != '\0') {
            eli_input_text__save_undo(st, buf, *len);
            eli_input_text__delete_selection(st, buf, len);
            for (const char *p = clip; *p != '\0'; p++) {
                uint16_t c = eli_input_text__filter((uint16_t)(unsigned char)*p, flags);
                if (c == 0)
                    continue;
                char enc[3];
                int en = eli_input_text__encode_utf8((uint32_t)c, enc);
                int added = eli_input_text__insert(buf, len, buf_size, st->cursor, enc, en);
                st->cursor += added;
            }
            st->selection_anchor = st->cursor;
            changed = true;
        }
    }
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

/* ---------------------------------------------------------------------------
 * Rendering
 * ------------------------------------------------------------------------- */

/** Pixel width of buf[from,to), rendering as asterisks when password is set. */
static inline float eli_input_text__sub_width(const char *buf, int from, int to, bool password)
{
    if (from >= to)
        return 0.0f;
    if (password) {
        int cps = 0;
        int i = from;
        int len = to;
        while (i < to) {
            i = eli_input_text__next_cp(buf, len, i);
            cps++;
        }
        return (float)cps * eli_calc_text_size("*", NULL).x;
    }
    return eli_calc_text_size(buf + from, buf + to).x;
}

/** Build the visible string (asterisks for password) into g_eli_input_text_display. */
static inline const char *eli_input_text__display(const char *buf, int len, bool password,
                                                  const char **out_end)
{
    if (!password) {
        *out_end = buf + len;
        return buf;
    }
    int n = 0;
    int i = 0;
    while (i < len && n < ELI_INPUT_TEXT_SNAPSHOT_MAX - 1) {
        i = eli_input_text__next_cp(buf, len, i);
        g_eli_input_text_display[n++] = '*';
    }
    g_eli_input_text_display[n] = '\0';
    *out_end = g_eli_input_text_display + n;
    return g_eli_input_text_display;
}

/**
 * Draw the frame, text/hint, selection highlight and blinking caret.
 * Single-line selection is drawn as one rectangle; the multiline path draws the
 * caret only.
 */
static inline void eli_input_text__render(const eli_input_text_state *st, eli_rect frame_bb,
                                          const char *buf, int len, const char *hint,
                                          eli_input_text_flags flags, bool is_active, bool multiline)
{
    eli_context *ctx = eli_get_current_context();
    eli_draw_list *dl = eli_get_window_draw_list();
    const eli_style *style = eli_get_style();
    if (ctx == NULL || dl == NULL || style == NULL)
        return;

    eli_vec2 fmin = eli_rect_min(frame_bb);
    eli_vec2 fmax = eli_rect_max(frame_bb);
    eli_col32 frame_col = is_active ? eli_get_color_u32(ELI_COL_FRAME_BG_ACTIVE, 1.0f)
                                    : eli_get_color_u32(ELI_COL_FRAME_BG, 1.0f);
    eli_render_frame(fmin, fmax, frame_col, true, style->frame_rounding);

    eli_vec2 pad = style->frame_padding;
    eli_vec2 text_pos = eli_make_vec2(fmin.x + pad.x - st->scroll_x, fmin.y + pad.y);
    eli_draw_list_push_clip_rect(dl, eli_make_vec2(fmin.x + 1.0f, fmin.y + 1.0f),
                                 eli_make_vec2(fmax.x - 1.0f, fmax.y - 1.0f), true);

    bool password = (flags & ELI_INPUT_TEXT_PASSWORD) != 0;
    if (len == 0 && !is_active && hint != NULL && hint[0] != '\0') {
        eli_draw_list_add_text(dl, text_pos, eli_get_color_u32(ELI_COL_TEXT_DISABLED, 1.0f),
                               hint, NULL);
    } else {
        const char *disp_end = NULL;
        const char *disp = eli_input_text__display(buf, len, password, &disp_end);
        if (is_active && eli_input_text__has_selection(st) && !multiline) {
            float x0 = text_pos.x + eli_input_text__sub_width(buf, 0, eli_input_text__sel_min(st),
                                                              password);
            float x1 = text_pos.x + eli_input_text__sub_width(buf, 0, eli_input_text__sel_max(st),
                                                              password);
            eli_draw_list_add_rect_filled(dl, eli_make_vec2(x0, fmin.y + pad.y),
                                          eli_make_vec2(x1, fmax.y - pad.y),
                                          eli_get_color_u32(ELI_COL_TEXT_SELECTED_BG, 1.0f), 0.0f,
                                          ELI_DRAW_ROUND_CORNERS_NONE);
        }
        eli_draw_list_add_text(dl, text_pos, eli_get_color_u32(ELI_COL_TEXT, 1.0f), disp, disp_end);
    }

    if (is_active) {
        double elapsed = ctx->time - st->cursor_anim_base;
        bool blink = fmod(elapsed, 1.20) <= 0.80;
        if (blink) {
            int line_start = st->cursor;
            while (line_start > 0 && buf[line_start - 1] != '\n')
                line_start--;
            int newlines = 0;
            for (int i = 0; i < st->cursor; i++)
                if (buf[i] == '\n')
                    newlines++;
            float cx = text_pos.x + eli_input_text__sub_width(buf, line_start, st->cursor, password);
            float cy = text_pos.y + (float)newlines * eli_get_text_line_height();
            eli_draw_list_add_line(dl, eli_make_vec2(cx, cy),
                                   eli_make_vec2(cx, cy + eli_get_text_line_height()),
                                   eli_get_color_u32(ELI_COL_TEXT, 1.0f), 1.0f);
        }
    }
    eli_draw_list_pop_clip_rect(dl);
}

/** Keep the caret visible by adjusting the horizontal scroll (single line). */
static inline void eli_input_text__update_scroll(eli_input_text_state *st, eli_rect frame_bb,
                                                 const char *buf, eli_input_text_flags flags)
{
    if (flags & ELI_INPUT_TEXT_NO_HORIZONTAL_SCROLL)
        return;
    const eli_style *style = eli_get_style();
    if (style == NULL)
        return;
    bool password = (flags & ELI_INPUT_TEXT_PASSWORD) != 0;
    float inner_w = frame_bb.w - style->frame_padding.x * 2.0f;
    float caret_x = eli_input_text__sub_width(buf, 0, st->cursor, password);
    if (caret_x - st->scroll_x > inner_w)
        st->scroll_x = caret_x - inner_w;
    if (caret_x - st->scroll_x < 0.0f)
        st->scroll_x = caret_x;
    if (st->scroll_x < 0.0f)
        st->scroll_x = 0.0f;
}

/* ---------------------------------------------------------------------------
 * Core implementation
 * ------------------------------------------------------------------------- */

/** Snapshot the buffer and reset the editing state when a field is activated. */
static inline void eli_input_text__activate(eli_input_text_state *st, eli_id id, const char *buf,
                                            int len, eli_input_text_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    st->id = id;
    st->scroll_x = 0.0f;
    st->has_undo = false;
    st->cursor_anim_base = ctx ? ctx->time : 0.0;
    int snap = len < ELI_INPUT_TEXT_SNAPSHOT_MAX ? len : ELI_INPUT_TEXT_SNAPSHOT_MAX - 1;
    memcpy(st->initial, buf, (size_t)snap);
    st->initial[snap] = '\0';
    st->initial_len = snap;
    if (flags & ELI_INPUT_TEXT_AUTO_SELECT_ALL) {
        st->selection_anchor = 0;
        st->cursor = len;
    } else {
        st->cursor = len;
        st->selection_anchor = len;
    }
}

/**
 * Shared implementation for the input-text family.
 *
 * @param label      Widget label (identity + visible text right of the frame).
 * @param hint       Placeholder shown when empty and inactive, or NULL.
 * @param buf        Caller-owned, NUL-terminated edit buffer (mutated in place).
 * @param buf_size   Capacity of buf in bytes.
 * @param size_arg   Frame size (multiline height; 0 selects defaults).
 * @param flags      eli_input_text_flags.
 * @param multiline  true for the multiline editor.
 * @param callback   Optional char-filter/edit callback (may be NULL).
 * @param user_data  Opaque pointer passed to the callback.
 * @return           true per the return policy (see file header).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_input_text_impl(const char *label, const char *hint, char *buf,
                                       int buf_size, eli_vec2 size_arg, eli_input_text_flags flags,
                                       bool multiline, eli_input_text_callback callback,
                                       void *user_data)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items ||
        buf == NULL || buf_size <= 1)
        return false;
    const eli_style *style = &ctx->style;

    eli_id id = eli_get_id(label);
    const char *label_end = eli_find_rendered_text_end(label, NULL);
    eli_vec2 label_size = eli_calc_text_size(label, label_end);
    eli_vec2 pos = ctx->current_window->cursor_pos;

    float frame_w = eli_calc_item_width();
    float line_h = eli_get_text_line_height();
    float default_h = multiline ? (line_h * 8.0f + style->frame_padding.y * 2.0f)
                                : eli_get_frame_height();
    eli_vec2 frame_size = eli_calc_item_size(multiline ? size_arg : eli_make_vec2(0.0f, 0.0f),
                                             frame_w, default_h);
    eli_rect frame_bb = eli_make_rect(pos.x, pos.y, frame_size.x, frame_size.y);

    float total_w = frame_size.x +
                    (label_size.x > 0.0f ? style->item_inner_spacing.x + label_size.x : 0.0f);
    eli_item_size(eli_make_vec2(total_w, frame_size.y), style->frame_padding.y);
    if (!eli_item_add(id, frame_bb, 0))
        return false;

    bool read_only = (flags & ELI_INPUT_TEXT_READ_ONLY) != 0;
    eli_vec2 mn = eli_rect_min(frame_bb);
    eli_vec2 mx = eli_rect_max(frame_bb);
    bool hovered = eli_is_mouse_hovering_rect(mn, mx, true) &&
                   eli_widget__window_hovered(ctx, ctx->current_window) &&
                   (ctx->active_id == 0u || ctx->active_id == id);
    if (hovered)
        eli_set_hot_id(id);

    int len = eli_input_text__strnlen(buf, buf_size);
    eli_input_text_state *st = &g_eli_input_text_state;

    bool clicked = hovered && eli_is_mouse_clicked(ELI_MOUSE_BUTTON_LEFT);
    if (clicked && !read_only && ctx->active_id != id) {
        eli_set_active_id(id);
        eli_input_text__activate(st, id, buf, len, flags);
    }
    bool is_active = (id != 0u) && (ctx->active_id == id) && (st->id == id);

    bool value_changed = false;
    bool enter_pressed = false;
    bool deactivate = false;
    if (is_active) {
        if (st->cursor > len)
            st->cursor = len;
        if (st->selection_anchor > len)
            st->selection_anchor = len;
        if (!read_only) {
            bool ch = eli_input_text__process_chars(st, buf, &len, buf_size, flags);
            bool er = eli_input_text__process_erase(st, buf, &len);
            bool cl = eli_input_text__process_clipboard(st, buf, &len, buf_size, flags);
            value_changed = ch || er || cl;
        }
        eli_input_text__process_nav(st, buf, len, multiline);

        bool enter = eli_is_key_pressed(ELI_KEY_ENTER) || eli_is_key_pressed(ELI_KEY_KEYPAD_ENTER);
        if (enter) {
            bool want_newline = multiline;
            if (multiline && (flags & ELI_INPUT_TEXT_CTRL_ENTER_FOR_NEWLINE))
                want_newline = eli_is_key_down(ELI_KEY_LEFT_CTRL) ||
                               eli_is_key_down(ELI_KEY_RIGHT_CTRL);
            else if (multiline)
                want_newline = !(eli_is_key_down(ELI_KEY_LEFT_CTRL) ||
                                 eli_is_key_down(ELI_KEY_RIGHT_CTRL));
            if (want_newline && !read_only) {
                eli_input_text__save_undo(st, buf, len);
                eli_input_text__delete_selection(st, buf, &len);
                int added = eli_input_text__insert(buf, &len, buf_size, st->cursor, "\n", 1);
                st->cursor += added;
                st->selection_anchor = st->cursor;
                value_changed = value_changed || added > 0;
            } else {
                enter_pressed = true;
                deactivate = true;
            }
        }
        if (eli_is_key_pressed(ELI_KEY_ESCAPE)) {
            if (flags & ELI_INPUT_TEXT_ESCAPE_CLEARS_ALL) {
                if (len > 0) {
                    buf[0] = '\0';
                    len = 0;
                    value_changed = true;
                }
            } else if (memcmp(buf, st->initial, (size_t)st->initial_len) != 0 ||
                       len != st->initial_len) {
                memcpy(buf, st->initial, (size_t)st->initial_len);
                buf[st->initial_len] = '\0';
                len = st->initial_len;
                value_changed = true;
            }
            deactivate = true;
        }
        if (value_changed)
            st->cursor_anim_base = ctx->time;
    }

    if (is_active && eli_is_mouse_clicked(ELI_MOUSE_BUTTON_LEFT) && !hovered)
        deactivate = true;

    if (callback != NULL && (flags & (ELI_INPUT_TEXT_CALLBACK_ALWAYS | ELI_INPUT_TEXT_CALLBACK_EDIT))
        && is_active) {
        eli_input_text_callback_data cb = {0};
        cb.ctx = ctx;
        cb.flags = flags;
        cb.event_flag = value_changed ? ELI_INPUT_TEXT_CALLBACK_EDIT : ELI_INPUT_TEXT_CALLBACK_ALWAYS;
        cb.user_data = user_data;
        cb.buf = buf;
        cb.buf_text_len = len;
        cb.buf_size = buf_size;
        cb.cursor_pos = st->cursor;
        cb.selection_start = eli_input_text__sel_min(st);
        cb.selection_end = eli_input_text__sel_max(st);
        callback(&cb);
        if (cb.buf_dirty)
            len = eli_input_text__strnlen(buf, buf_size);
    }

    if (is_active)
        eli_input_text__update_scroll(st, frame_bb, buf, flags);

    eli_render_nav_highlight(frame_bb, id);
    eli_input_text__render(st, frame_bb, buf, len, hint, flags, is_active, multiline);

    if (label_size.x > 0.0f)
        eli_render_text(eli_make_vec2(mx.x + style->item_inner_spacing.x, mn.y + style->frame_padding.y),
                        eli_get_color_u32(ELI_COL_TEXT, 1.0f), label, label_end, false);

    if (deactivate) {
        eli_clear_active_id();
        st->id = 0u;
    }
    if (value_changed)
        eli_mark_item_edited(id);

    if (flags & ELI_INPUT_TEXT_ENTER_RETURNS_TRUE)
        return enter_pressed;
    return value_changed;
}

/* ---------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

/**
 * A single-line text input bound to a caller-owned buffer.
 *
 * @param label      Widget label (identity + visible text; hidden after "##").
 * @param buf        NUL-terminated edit buffer, mutated in place.
 * @param buf_size   Capacity of buf in bytes.
 * @param flags      eli_input_text_flags.
 * @param callback   Optional filter/edit callback (may be NULL).
 * @param user_data  Opaque pointer passed to the callback.
 * @return           true when edited (or, with ENTER_RETURNS_TRUE, on Enter).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_input_text(const char *label, char *buf, size_t buf_size,
                                  eli_input_text_flags flags, eli_input_text_callback callback,
                                  void *user_data)
{
    return eli_input_text_impl(label, NULL, buf, (int)buf_size, eli_make_vec2(0.0f, 0.0f), flags,
                               false, callback, user_data);
}

/**
 * A multi-line text input.
 *
 * @param label      Widget label.
 * @param buf        NUL-terminated edit buffer, mutated in place.
 * @param buf_size   Capacity of buf in bytes.
 * @param size       Frame size (0 components select defaults).
 * @param flags      eli_input_text_flags.
 * @param callback   Optional filter/edit callback (may be NULL).
 * @param user_data  Opaque pointer passed to the callback.
 * @return           true when edited.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_input_text_multiline(const char *label, char *buf, size_t buf_size,
                                            eli_vec2 size, eli_input_text_flags flags,
                                            eli_input_text_callback callback, void *user_data)
{
    return eli_input_text_impl(label, NULL, buf, (int)buf_size, size, flags, true, callback,
                               user_data);
}

/**
 * A single-line text input that shows a placeholder hint while empty and inactive.
 *
 * @param label      Widget label.
 * @param hint       Placeholder text shown when empty and not being edited.
 * @param buf        NUL-terminated edit buffer, mutated in place.
 * @param buf_size   Capacity of buf in bytes.
 * @param flags      eli_input_text_flags.
 * @param callback   Optional filter/edit callback (may be NULL).
 * @param user_data  Opaque pointer passed to the callback.
 * @return           true when edited (or, with ENTER_RETURNS_TRUE, on Enter).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_input_text_with_hint(const char *label, const char *hint, char *buf,
                                            size_t buf_size, eli_input_text_flags flags,
                                            eli_input_text_callback callback, void *user_data)
{
    return eli_input_text_impl(label, hint, buf, (int)buf_size, eli_make_vec2(0.0f, 0.0f), flags,
                               false, callback, user_data);
}

#endif /* ELI_WIDGETS_ELI_INPUT_TEXT_WIDGET_H */
