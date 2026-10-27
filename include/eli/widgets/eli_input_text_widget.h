/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
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

#include "eli_input_text_edit.h"

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
        double phase = fmod(elapsed, 1.20);
        bool blink = phase <= 0.80;
        /* Wake the loop at the next blink edge (on at [0, 0.8), off at [0.8, 1.2)). */
        eli_request_redraw_after((float)((blink ? 0.80 : 1.20) - phase) + 0.001f);
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
    /* Escape-revert needs the whole original text; a longer buffer gets no revert
     * rather than a truncated one. */
    st->initial_valid = len < ELI_INPUT_TEXT_SNAPSHOT_MAX;
    int snap = st->initial_valid ? len : 0;
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
 * Enter (newline in multiline, else commit) and Escape (revert or clear, then
 * deactivate). In multiline, Shortcut+Enter (Cmd on macOS, else Ctrl) commits;
 * ELI_INPUT_TEXT_CTRL_ENTER_FOR_NEWLINE swaps the two.
 * @return true if the buffer changed.
 */
static inline bool eli_input_text__process_enter_escape(eli_input_text_state *st, char *buf,
                                                        int *len, int buf_size,
                                                        eli_input_text_flags flags, bool multiline,
                                                        bool *enter_pressed, bool *deactivate)
{
    bool changed = false;
    bool read_only = (flags & ELI_INPUT_TEXT_READ_ONLY) != 0;
    if (eli_is_key_pressed(ELI_KEY_ENTER) || eli_is_key_pressed(ELI_KEY_KEYPAD_ENTER)) {
        bool shortcut = eli_is_shortcut_mod_down(eli_get_io());
        bool want_newline = multiline &&
                            ((flags & ELI_INPUT_TEXT_CTRL_ENTER_FOR_NEWLINE) ? shortcut : !shortcut);
        if (want_newline && !read_only) {
            eli_input_text__save_undo(st, buf, *len);
            eli_input_text__delete_selection(st, buf, len);
            int added = eli_input_text__insert(buf, len, buf_size, st->cursor, "\n", 1);
            st->cursor += added;
            st->selection_anchor = st->cursor;
            changed = added > 0;
        } else {
            *enter_pressed = true;
            *deactivate = true;
        }
    }
    if (eli_is_key_pressed(ELI_KEY_ESCAPE)) {
        if (flags & ELI_INPUT_TEXT_ESCAPE_CLEARS_ALL) {
            if (*len > 0) {
                buf[0] = '\0';
                *len = 0;
                changed = true;
            }
        } else if (st->initial_valid && (*len != st->initial_len ||
                                         memcmp(buf, st->initial, (size_t)st->initial_len) != 0)) {
            memcpy(buf, st->initial, (size_t)st->initial_len);
            buf[st->initial_len] = '\0';
            *len = st->initial_len;
            changed = true;
        }
        *deactivate = true;
    }
    return changed;
}

/** Invoke the ALWAYS/EDIT callback; @return the (possibly changed) text length. */
static inline int eli_input_text__run_callback(const eli_input_text_state *st, char *buf, int len,
                                               int buf_size, eli_input_text_flags flags,
                                               bool value_changed, eli_input_text_callback callback,
                                               void *user_data)
{
    eli_input_text_callback_data cb = {0};
    cb.ctx = eli_get_current_context();
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
    return cb.buf_dirty ? eli_input_text__strnlen(buf, buf_size) : len;
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

        value_changed = eli_input_text__process_enter_escape(st, buf, &len, buf_size, flags,
                                                             multiline, &enter_pressed,
                                                             &deactivate) || value_changed;
        if (value_changed)
            st->cursor_anim_base = ctx->time;
    }

    if (is_active && eli_is_mouse_clicked(ELI_MOUSE_BUTTON_LEFT) && !hovered)
        deactivate = true;
    if (is_active && !deactivate) {
        eli_input_text__claim_keys(id, flags);    /* app shortcuts yield while typing */
        if (!read_only)
            eli_input_mark_text_input_active();
    }

    if (callback != NULL && (flags & (ELI_INPUT_TEXT_CALLBACK_ALWAYS | ELI_INPUT_TEXT_CALLBACK_EDIT))
        && is_active)
        len = eli_input_text__run_callback(st, buf, len, buf_size, flags, value_changed, callback,
                                           user_data);

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
