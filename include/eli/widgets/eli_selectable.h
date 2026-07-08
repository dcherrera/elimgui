/**
 * @file eli_selectable.h
 * @brief Phase 14 selectable rows: eli_selectable (caller-owned selected flag) and
 *        eli_selectable_bool (toggles a bool*). A selectable is a full-width,
 *        tightly-packed clickable row that highlights on hover/selection using the
 *        ELI_COL_HEADER* colors, honoring eli_selectable_flags (span-all-columns,
 *        disabled, allow-double-click, highlight) and auto-closing an enclosing
 *        popup when chosen. It is the row primitive combos, list boxes, and menus
 *        build on. Mirrors Dear ImGui's Selectable.
 *
 * @status Phase 14 selectable widget in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WIDGETS_ELI_SELECTABLE_H
#define ELI_WIDGETS_ELI_SELECTABLE_H

#include "eli_widget_behavior.h"
#include "eli_popup.h"

#include "../style/eli_item_flags.h"

#include "../core/eli_platform.h"

/* ---------------------------------------------------------------------------
 * Selectable flags
 *
 * Guarded so a later phase may promote these into core/eli_enums.h without a
 * redefinition clash; defined here because the selectable is the first consumer.
 * ------------------------------------------------------------------------- */

#ifndef ELI_SELECTABLE_NONE
typedef int eli_selectable_flags;
enum eli_selectable_flags_ {
    ELI_SELECTABLE_NONE                 = 0,
    ELI_SELECTABLE_NO_AUTO_CLOSE_POPUPS = 1 << 0,
    ELI_SELECTABLE_SPAN_ALL_COLUMNS     = 1 << 1,
    ELI_SELECTABLE_ALLOW_DOUBLE_CLICK   = 1 << 2,
    ELI_SELECTABLE_DISABLED             = 1 << 3,
    ELI_SELECTABLE_ALLOW_OVERLAP        = 1 << 4,
    ELI_SELECTABLE_HIGHLIGHT            = 1 << 5
};
#endif /* ELI_SELECTABLE_NONE */

/* ---------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

/**
 * Render a label clipped to [clip_min, clip_max] and positioned within that box
 * per the style's selectable_text_align, stopping the visible run at "##".
 *
 * @param clip_min    Top-left of the text clip/align box (screen space).
 * @param clip_max    Bottom-right of the text clip/align box.
 * @param label       UTF-8 label.
 * @param label_size  Measured size of the visible label.
 * @param col         Packed text color.
 */
static inline void eli_selectable__render_text(eli_vec2 clip_min, eli_vec2 clip_max,
                                               const char *label, eli_vec2 label_size, eli_col32 col)
{
    const eli_style *style = eli_get_style();
    eli_draw_list *dl = eli_get_window_draw_list();
    if (style == NULL || dl == NULL)
        return;

    eli_vec2 align = style->selectable_text_align;
    float avail_x = (clip_max.x - clip_min.x) - label_size.x;
    float avail_y = (clip_max.y - clip_min.y) - label_size.y;
    eli_vec2 pos = eli_make_vec2(clip_min.x + eli_max_f(0.0f, avail_x) * align.x,
                                 clip_min.y + eli_max_f(0.0f, avail_y) * align.y);

    const char *end = eli_find_rendered_text_end(label, NULL);
    bool clipped = pos.x < clip_min.x || (pos.x + label_size.x) > clip_max.x;
    if (clipped)
        eli_draw_list_push_clip_rect(dl, clip_min, clip_max, true);
    if (end > label)
        eli_draw_list_add_text(dl, pos, col, label, end);
    if (clipped)
        eli_draw_list_pop_clip_rect(dl);
}

/* ---------------------------------------------------------------------------
 * Selectable
 * ------------------------------------------------------------------------- */

/**
 * A full-width clickable row that highlights when hovered or selected.
 *
 * The row measures its label (or an explicit size), stretches to fill the work
 * area width, registers a tightly-packed bounding box (extended by half the item
 * spacing so adjacent selectables have no click gap), and drives the shared
 * button behavior. When the row lives inside a popup and is pressed it closes that
 * popup unless ELI_SELECTABLE_NO_AUTO_CLOSE_POPUPS is set.
 *
 * @param label     Row label (its visible part stops at "##").
 * @param selected  Whether this row is currently the selected one.
 * @param flags     eli_selectable_flags controlling span/disable/double-click.
 * @param size_arg  Requested size (0 axis = fit label / fill width).
 * @return          true on the frame the row is pressed.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_selectable(const char *label, bool selected, eli_selectable_flags flags,
                                  eli_vec2 size_arg)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return false;
    eli_window *win = ctx->current_window;
    const eli_style *style = &ctx->style;

    eli_id id = eli_get_id(label);
    eli_vec2 label_size = eli_calc_text_size(label, eli_find_rendered_text_end(label, NULL));
    eli_vec2 pos = win->cursor_pos;

    eli_vec2 size = eli_make_vec2(size_arg.x != 0.0f ? size_arg.x : label_size.x,
                                  size_arg.y != 0.0f ? size_arg.y : label_size.y);
    eli_item_size(size, 0.0f);

    /* Fill horizontal space up to the work-area edge. */
    bool span_all = (flags & ELI_SELECTABLE_SPAN_ALL_COLUMNS) != 0;
    float region_min_x = win->content_region_rect.x;
    float region_max_x = win->content_region_rect.x + win->content_region_rect.w;
    float min_x = span_all ? region_min_x : pos.x;
    float max_x = region_max_x;
    if (size_arg.x == 0.0f)
        size.x = eli_max_f(label_size.x, max_x - min_x);

    /* Extend the box by half the item spacing so rows pack with no click gap. */
    eli_rect bb = eli_make_rect(min_x, pos.y, size.x, size.y);
    float spacing_x = span_all ? 0.0f : style->item_spacing.x;
    float spacing_y = style->item_spacing.y;
    float spacing_l = (float)(int)(spacing_x * 0.5f);
    float spacing_u = (float)(int)(spacing_y * 0.5f);
    bb.x -= spacing_l;
    bb.y -= spacing_u;
    bb.w += spacing_x;
    bb.h += spacing_y;

    bool disabled = (flags & ELI_SELECTABLE_DISABLED) != 0;
    if (disabled)
        eli_push_item_flag(ELI_ITEM_DISABLED, true);

    bool visible = eli_item_add(id, bb, 0);
    if (!visible) {
        if (disabled)
            eli_pop_item_flag();
        return false;
    }

    int button_flags = 0;
    if (flags & ELI_SELECTABLE_ALLOW_DOUBLE_CLICK)
        button_flags |= ELI_BUTTON_PRESSED_ON_CLICK_RELEASE;
    if (flags & ELI_SELECTABLE_ALLOW_OVERLAP)
        button_flags |= ELI_BUTTON_NONE; /* overlap handled by item status, not press policy */

    bool hovered = false, held = false;
    bool pressed = eli_button_behavior(bb, id, &hovered, &held, button_flags);
    if (pressed)
        eli_mark_item_edited(id);

    /* Render highlight + label. */
    eli_render_nav_highlight(bb, id);
    bool highlighted = hovered || (flags & ELI_SELECTABLE_HIGHLIGHT) != 0;
    if (highlighted || selected) {
        eli_col32 col = eli_get_color_u32((held && highlighted) ? ELI_COL_HEADER_ACTIVE
                                          : highlighted          ? ELI_COL_HEADER_HOVERED
                                                                 : ELI_COL_HEADER,
                                          1.0f);
        eli_draw_list_add_rect_filled(eli_get_window_draw_list(), eli_rect_min(bb), eli_rect_max(bb),
                                      col, 0.0f, 0);
    }

    eli_col32 text_col = eli_get_color_u32(disabled ? ELI_COL_TEXT_DISABLED : ELI_COL_TEXT, 1.0f);
    eli_vec2 text_min = pos;
    eli_vec2 text_max = eli_make_vec2(eli_min_f(pos.x + size.x, region_max_x), pos.y + size.y);
    eli_selectable__render_text(text_min, text_max, label, label_size, text_col);

    if (disabled)
        eli_pop_item_flag();

    /* Auto-close an enclosing popup when a row is chosen. */
    if (pressed && (win->flags & ELI_WINDOW_POPUP) &&
        !(flags & ELI_SELECTABLE_NO_AUTO_CLOSE_POPUPS))
        eli_close_current_popup();

    return pressed;
}

/**
 * A selectable bound to a bool: pressing it toggles *p_selected.
 *
 * @param label      Row label.
 * @param p_selected Pointer to the bool to toggle (must be non-NULL).
 * @param flags      eli_selectable_flags.
 * @param size_arg   Requested size (0 axis = fit label / fill width).
 * @return           true on the frame the row is pressed (after toggling).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_selectable_bool(const char *label, bool *p_selected,
                                       eli_selectable_flags flags, eli_vec2 size_arg)
{
    if (p_selected == NULL)
        return eli_selectable(label, false, flags, size_arg);
    if (eli_selectable(label, *p_selected, flags, size_arg)) {
        *p_selected = !*p_selected;
        return true;
    }
    return false;
}

#endif /* ELI_WIDGETS_ELI_SELECTABLE_H */
