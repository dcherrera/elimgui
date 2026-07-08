/**
 * @file eli_widget_behavior.h
 * @brief The shared interaction core every interactive widget builds on:
 *        eli_button_behavior (hover/active/press semantics), the eli_button_flags
 *        press policies, item-edit marking, item-size resolution, and the low-level
 *        render helpers (frame, text, nav highlight, arrow, check mark, bullet).
 *
 * A widget measures itself, calls eli_item_size + eli_item_add to register its
 * rect/id, then drives eli_button_behavior for interaction and the render helpers
 * for output. Hover maps to the context hot_id (Dear ImGui's HoveredId) and a held
 * item to active_id (ActiveId), so the Phase 10 item-status queries can read state
 * back. Mirrors Dear ImGui's ButtonBehavior/RenderFrame/RenderText contracts.
 *
 * @status Phase 9 widget interaction core in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WIDGETS_ELI_WIDGET_BEHAVIOR_H
#define ELI_WIDGETS_ELI_WIDGET_BEHAVIOR_H

#include "../layout/eli_layout.h"
#include "../id/eli_id.h"
#include "../style/eli_style.h"
#include "../font/eli_font.h"

#include "../core/eli_platform.h"

/* ---------------------------------------------------------------------------
 * Button flags
 *
 * The public subset (mouse-button selection + nav) matches the spec; the private
 * press-policy bits mirror Dear ImGui's ImGuiButtonFlagsPrivate so widgets can
 * pick when a press fires (on down, on release-inside, or on any release).
 * ------------------------------------------------------------------------- */

typedef int eli_button_flags;
enum eli_button_flags_ {
    ELI_BUTTON_NONE                   = 0,
    ELI_BUTTON_MOUSE_BUTTON_LEFT      = 1 << 0,
    ELI_BUTTON_MOUSE_BUTTON_RIGHT     = 1 << 1,
    ELI_BUTTON_MOUSE_BUTTON_MIDDLE    = 1 << 2,
    ELI_BUTTON_ENABLE_NAV             = 1 << 3,

    /* Private press-policy bits (elimgui extension). */
    ELI_BUTTON_PRESSED_ON_CLICK         = 1 << 4,  /* fire on mouse-down */
    ELI_BUTTON_PRESSED_ON_CLICK_RELEASE = 1 << 5,  /* fire on release inside (default) */
    ELI_BUTTON_PRESSED_ON_RELEASE       = 1 << 6,  /* fire on any release while active */
    ELI_BUTTON_REPEAT                   = 1 << 7,  /* repeat press while held */

    ELI_BUTTON_MOUSE_BUTTON_MASK = ELI_BUTTON_MOUSE_BUTTON_LEFT | ELI_BUTTON_MOUSE_BUTTON_RIGHT |
                                   ELI_BUTTON_MOUSE_BUTTON_MIDDLE,
    ELI_BUTTON_PRESSED_ON_MASK   = ELI_BUTTON_PRESSED_ON_CLICK | ELI_BUTTON_PRESSED_ON_CLICK_RELEASE |
                                   ELI_BUTTON_PRESSED_ON_RELEASE
};

/* ---------------------------------------------------------------------------
 * Item edit marking + status flag record
 * ------------------------------------------------------------------------- */

/**
 * Flag the most-recently-added item as edited this frame and record that an edit
 * happened during the active item's lifetime (drives eli_is_item_edited and
 * eli_is_item_deactivated_after_edit). Mirrors Dear ImGui's MarkItemEdited.
 *
 * @param id  Id of the item that was edited (the active/last item).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_mark_item_edited(eli_id id)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    if (ctx->active_id == id || ctx->active_id == 0u)
        ctx->active_id_has_been_edited_this_frame = true;
    ctx->active_id_has_been_edited_before = true;
    ctx->last_item_status_flags |= ELI_ITEM_STATUS_EDITED;
}

/* ---------------------------------------------------------------------------
 * Item size resolution
 * ------------------------------------------------------------------------- */

/**
 * Resolve a caller-supplied size against defaults and the available content
 * region, matching Dear ImGui's CalcItemSize: a zero component takes the default,
 * a negative component is measured back from the region edge (min 4px).
 *
 * @param size       Requested size (0 = default, < 0 = region-relative).
 * @param default_w  Default width when size.x == 0.
 * @param default_h  Default height when size.y == 0.
 * @return           The resolved pixel size.
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline eli_vec2 eli_calc_item_size(eli_vec2 size, float default_w, float default_h)
{
    eli_vec2 region = eli_get_content_region_avail();
    if (size.x == 0.0f)
        size.x = default_w;
    else if (size.x < 0.0f)
        size.x = eli_max_f(4.0f, region.x + size.x);
    if (size.y == 0.0f)
        size.y = default_h;
    else if (size.y < 0.0f)
        size.y = eli_max_f(4.0f, region.y + size.y);
    return size;
}

/* ---------------------------------------------------------------------------
 * Button behavior — the shared hover/active/press primitive
 * ------------------------------------------------------------------------- */

/** @return true if the root of `win` is the hovered window's root this frame. */
static inline bool eli_widget__window_hovered(const eli_context *ctx, const eli_window *win)
{
    if (ctx->hovered_window == NULL || win == NULL)
        return false;
    const eli_window *hov = ctx->hovered_window->root_window ? ctx->hovered_window->root_window
                                                             : ctx->hovered_window;
    const eli_window *w = win->root_window ? win->root_window : win;
    return hov == w;
}

/** Resolve the single mouse button a button-flags mask refers to. */
static inline eli_mouse_button eli_widget__mouse_button(int flags)
{
    if (flags & ELI_BUTTON_MOUSE_BUTTON_RIGHT)
        return ELI_MOUSE_BUTTON_RIGHT;
    if (flags & ELI_BUTTON_MOUSE_BUTTON_MIDDLE)
        return ELI_MOUSE_BUTTON_MIDDLE;
    return ELI_MOUSE_BUTTON_LEFT;
}

/**
 * The shared interaction primitive: compute hover, drive active-id set/clear on
 * press/release, and return whether the item was "pressed" per its press policy.
 * Sets the context hot_id while hovered so hover propagates to the item-status
 * queries. Honors ELI_ITEM_DISABLED (no hover/press when disabled).
 *
 * @param bb           Item bounding box in screen space.
 * @param id           Item id (0 disables interaction).
 * @param out_hovered  Receives whether the item is hovered (may be NULL).
 * @param out_held     Receives whether the item is held/active (may be NULL).
 * @param flags        eli_button_flags (mouse button + press policy; defaults to
 *                     left mouse button, press-on-release-inside).
 * @return             true on the frame the item registers a press.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_button_behavior(eli_rect bb, eli_id id, bool *out_hovered, bool *out_held,
                                       int flags)
{
    bool hovered_local = false, held_local = false;
    if (out_hovered == NULL)
        out_hovered = &hovered_local;
    if (out_held == NULL)
        out_held = &held_local;
    *out_hovered = false;
    *out_held = false;

    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || id == 0u)
        return false;

    if ((flags & ELI_BUTTON_PRESSED_ON_MASK) == 0)
        flags |= ELI_BUTTON_PRESSED_ON_CLICK_RELEASE;
    if ((flags & ELI_BUTTON_MOUSE_BUTTON_MASK) == 0)
        flags |= ELI_BUTTON_MOUSE_BUTTON_LEFT;

    eli_window *win = ctx->current_window;
    eli_mouse_button mb = eli_widget__mouse_button(flags);
    bool disabled = (ctx->current_item_flags & ELI_ITEM_DISABLED) != 0;

    eli_vec2 mn = eli_rect_min(bb);
    eli_vec2 mx = eli_rect_max(bb);
    bool rect_hovered = eli_is_mouse_hovering_rect(mn, mx, true);
    bool window_hovered = eli_widget__window_hovered(ctx, win);
    bool blocked = (ctx->active_id != 0u && ctx->active_id != id);

    bool hovered = !disabled && rect_hovered && window_hovered && !blocked;
    if (hovered)
        eli_set_hot_id(id);

    bool pressed = false;
    if (hovered) {
        if ((flags & ELI_BUTTON_PRESSED_ON_CLICK) && eli_is_mouse_clicked(mb)) {
            pressed = true;
            eli_set_active_id(id);
        } else if ((flags & (ELI_BUTTON_PRESSED_ON_CLICK_RELEASE | ELI_BUTTON_PRESSED_ON_RELEASE)) &&
                   eli_is_mouse_clicked(mb)) {
            eli_set_active_id(id);
        }
        /* A fresh activation starts a clean edit spell. */
        if (ctx->active_id_is_just_activated && ctx->active_id == id)
            ctx->active_id_has_been_edited_before = false;
    }

    bool held = false;
    if (ctx->active_id == id && !disabled) {
        held = true;
        if (eli_is_mouse_released(mb)) {
            bool release_in = eli_is_mouse_hovering_rect(mn, mx, true);
            if ((flags & ELI_BUTTON_PRESSED_ON_CLICK_RELEASE) && release_in &&
                !(flags & ELI_BUTTON_PRESSED_ON_CLICK))
                pressed = true;
            if (flags & ELI_BUTTON_PRESSED_ON_RELEASE)
                pressed = true;
            eli_clear_active_id();
        } else if (!eli_is_mouse_down(mb)) {
            /* Active but the button is neither down nor released this frame: the
             * press was lost (e.g. focus change) -> drop the active state. */
            eli_clear_active_id();
        }
    }

    *out_hovered = hovered;
    *out_held = held;
    return pressed;
}

/* ---------------------------------------------------------------------------
 * Rendering helpers
 * ------------------------------------------------------------------------- */

/**
 * @param text      Start of a UTF-8 label.
 * @param text_end  End of the label, or NULL to scan to NUL.
 * @return          The first byte of a "##" hidden-id marker, or the real end of
 *                  the visible label (mirrors Dear ImGui's FindRenderedTextEnd).
 */
static inline const char *eli_find_rendered_text_end(const char *text, const char *text_end)
{
    const char *p = text;
    if (text_end == NULL)
        text_end = text + strlen(text);
    while (p < text_end && (p[0] != '#' || (p + 1 >= text_end) || p[1] != '#'))
        p++;
    return p;
}

/**
 * Draw a widget frame: a filled background rect and, when the style enables a
 * frame border, an outline. Mirrors Dear ImGui's RenderFrame.
 *
 * @param p_min   Top-left corner.
 * @param p_max   Bottom-right corner.
 * @param fill    Packed background color.
 * @param border  true to draw the style's frame border.
 * @param rounding Corner rounding in pixels.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_render_frame(eli_vec2 p_min, eli_vec2 p_max, eli_col32 fill, bool border,
                                    float rounding)
{
    eli_draw_list *dl = eli_get_window_draw_list();
    const eli_style *style = eli_get_style();
    if (dl == NULL || style == NULL)
        return;
    eli_draw_list_add_rect_filled(dl, p_min, p_max, fill, rounding, ELI_DRAW_ROUND_CORNERS_ALL);
    if (border && style->frame_border_size > 0.0f) {
        eli_col32 col = eli_get_color_u32(ELI_COL_BORDER, 1.0f);
        eli_draw_list_add_rect(dl, p_min, p_max, col, rounding, ELI_DRAW_ROUND_CORNERS_ALL,
                               style->frame_border_size);
    }
}

/**
 * Draw a text label into the current window's draw list, stopping the visible run
 * at a "##" hidden-id marker (mirrors Dear ImGui's RenderText).
 *
 * @param pos                    Top-left pen position.
 * @param col                    Packed text color.
 * @param text                   Start of the UTF-8 label.
 * @param text_end               End of the label, or NULL for NUL-terminated.
 * @param hide_text_after_hash   true to stop the visible run at "##".
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_render_text(eli_vec2 pos, eli_col32 col, const char *text,
                                   const char *text_end, bool hide_text_after_hash)
{
    eli_draw_list *dl = eli_get_window_draw_list();
    if (dl == NULL || text == NULL)
        return;
    const char *end = hide_text_after_hash ? eli_find_rendered_text_end(text, text_end)
                                           : (text_end ? text_end : text + strlen(text));
    if (end > text)
        eli_draw_list_add_text(dl, pos, col, text, end);
}

/**
 * Draw the nav focus highlight rect around an item when it is the focused (nav)
 * item. A no-op until keyboard nav lands (nav_id stays 0). Mirrors RenderNavHighlight.
 *
 * @param bb  Item bounding box.
 * @param id  Item id.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_render_nav_highlight(eli_rect bb, eli_id id)
{
    eli_context *ctx = eli_get_current_context();
    eli_draw_list *dl = eli_get_window_draw_list();
    if (ctx == NULL || dl == NULL || id == 0u || ctx->nav_id != id)
        return;
    eli_col32 col = eli_get_color_u32(ELI_COL_NAV_CURSOR, 1.0f);
    eli_vec2 mn = eli_make_vec2(bb.x - 2.0f, bb.y - 2.0f);
    eli_vec2 mx = eli_make_vec2(bb.x + bb.w + 2.0f, bb.y + bb.h + 2.0f);
    eli_draw_list_add_rect(dl, mn, mx, col, ctx->style.frame_rounding, ELI_DRAW_ROUND_CORNERS_ALL,
                           2.0f);
}

/**
 * Draw a directional arrow glyph (used by arrow buttons, tree nodes, combos).
 * Mirrors Dear ImGui's RenderArrow triangle geometry.
 *
 * @param dl     Target draw list.
 * @param pos    Top-left of the arrow's bounding cell.
 * @param col    Packed fill color.
 * @param dir    Arrow direction.
 * @param scale  Size scale relative to the current font size.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_render_arrow(eli_draw_list *dl, eli_vec2 pos, eli_col32 col, eli_dir dir,
                                    float scale)
{
    if (dl == NULL)
        return;
    float h = eli_get_font_size();
    float r = h * 0.40f * scale;
    eli_vec2 center = eli_make_vec2(pos.x + h * 0.50f, pos.y + h * 0.50f * scale);

    eli_vec2 a, b, c;
    switch (dir) {
    case ELI_DIR_UP:
    case ELI_DIR_DOWN:
        if (dir == ELI_DIR_UP)
            r = -r;
        a = eli_make_vec2(+0.000f, +0.750f * r);
        b = eli_make_vec2(-0.866f, -0.750f * r);
        c = eli_make_vec2(+0.866f, -0.750f * r);
        break;
    case ELI_DIR_LEFT:
    case ELI_DIR_RIGHT:
    default:
        if (dir == ELI_DIR_LEFT)
            r = -r;
        a = eli_make_vec2(+0.750f * r, +0.000f);
        b = eli_make_vec2(-0.750f * r, +0.866f);
        c = eli_make_vec2(-0.750f * r, -0.866f);
        break;
    }
    eli_draw_list_add_triangle_filled(dl, eli_vec2_add(center, a), eli_vec2_add(center, b),
                                      eli_vec2_add(center, c), col);
}

/**
 * Draw a checkbox check mark inside a square of side `sz` at `pos`. Mirrors Dear
 * ImGui's RenderCheckMark two-segment polyline.
 *
 * @param dl   Target draw list.
 * @param pos  Top-left of the check-mark cell.
 * @param col  Packed color.
 * @param sz   Cell side length in pixels.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_render_check_mark(eli_draw_list *dl, eli_vec2 pos, eli_col32 col, float sz)
{
    if (dl == NULL)
        return;
    float thickness = eli_max_f(sz / 5.0f, 1.0f);
    sz -= thickness * 0.5f;
    pos = eli_make_vec2(pos.x + thickness * 0.25f, pos.y + thickness * 0.25f);

    float third = sz / 3.0f;
    float bx = pos.x + third;
    float by = pos.y + sz - third * 0.5f;
    eli_vec2 p0 = eli_make_vec2(bx - third, by - third);
    eli_vec2 p1 = eli_make_vec2(bx, by);
    eli_vec2 p2 = eli_make_vec2(bx + third * 2.0f, by - third * 2.0f);
    eli_draw_list_add_line(dl, p0, p1, col, thickness);
    eli_draw_list_add_line(dl, p1, p2, col, thickness);
}

/**
 * Draw a bullet point (small filled circle) centered in a font-size cell.
 *
 * @param dl   Target draw list.
 * @param pos  Top-left of the cell.
 * @param col  Packed color.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_render_bullet(eli_draw_list *dl, eli_vec2 pos, eli_col32 col)
{
    if (dl == NULL)
        return;
    float h = eli_get_font_size();
    eli_vec2 center = eli_make_vec2(pos.x + h * 0.5f, pos.y + h * 0.5f);
    eli_draw_list_add_circle_filled(dl, center, eli_max_f(2.0f, h * 0.20f), col, 0);
}

#endif /* ELI_WIDGETS_ELI_WIDGET_BEHAVIOR_H */
