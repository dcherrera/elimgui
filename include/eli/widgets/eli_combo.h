/**
 * @file eli_combo.h
 * @brief Phase 14 combo boxes: eli_begin_combo / eli_end_combo (a preview button
 *        that opens a popup list the caller fills with selectables) plus the
 *        convenience eli_combo (items array), eli_combo_str (zero-separated items),
 *        and eli_combo_fn (getter callback) that render the list and report the
 *        chosen index. The dropdown IS a popup: it reuses the Phase 17 popup stack
 *        and window path, anchored just below the preview frame. Mirrors Dear
 *        ImGui's BeginCombo / Combo. Also aggregates the selectable and list-box
 *        row widgets that share this feature family.
 *
 * @status Phase 14 combo widget in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WIDGETS_ELI_COMBO_H
#define ELI_WIDGETS_ELI_COMBO_H

#include "eli_selectable.h"
#include "eli_listbox.h"
#include "eli_popup.h"

#include "../core/eli_platform.h"

#include <stdio.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * Combo flags
 *
 * Guarded so a later phase may promote these into core/eli_enums.h without a
 * redefinition clash; defined here because the combo is the first consumer.
 * ------------------------------------------------------------------------- */

#ifndef ELI_COMBO_NONE
typedef int eli_combo_flags;
enum eli_combo_flags_ {
    ELI_COMBO_NONE              = 0,
    ELI_COMBO_POPUP_ALIGN_LEFT  = 1 << 0,
    ELI_COMBO_HEIGHT_SMALL      = 1 << 1,
    ELI_COMBO_HEIGHT_REGULAR    = 1 << 2,
    ELI_COMBO_HEIGHT_LARGE      = 1 << 3,
    ELI_COMBO_HEIGHT_LARGEST    = 1 << 4,
    ELI_COMBO_NO_ARROW_BUTTON   = 1 << 5,
    ELI_COMBO_NO_PREVIEW        = 1 << 6,
    ELI_COMBO_WIDTH_FIT_PREVIEW = 1 << 7,

    ELI_COMBO_HEIGHT_MASK = ELI_COMBO_HEIGHT_SMALL | ELI_COMBO_HEIGHT_REGULAR |
                            ELI_COMBO_HEIGHT_LARGE | ELI_COMBO_HEIGHT_LARGEST
};
#endif /* ELI_COMBO_NONE */

/* ---------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

/** @return the popup id for a combo whose frame id is `combo_id`. */
static inline eli_id eli_combo__popup_id(eli_id combo_id)
{
    return eli_hash_str("##ComboPopup", combo_id);
}

/** Number of item rows a combo popup shows before scrolling, given its flags. */
static inline int eli_combo__height_in_items(eli_combo_flags flags)
{
    if (flags & ELI_COMBO_HEIGHT_SMALL)
        return 4;
    if (flags & ELI_COMBO_HEIGHT_LARGE)
        return 20;
    if (flags & ELI_COMBO_HEIGHT_LARGEST)
        return 30;
    return 8; /* HeightRegular (default) */
}

/** Max popup height in pixels for `count` item rows (<= 0 => unbounded). */
static inline float eli_combo__max_popup_height(int count)
{
    if (count <= 0)
        return -1.0f;
    const eli_style *style = eli_get_style();
    float line = eli_get_text_line_height_with_spacing();
    return line * (float)count + (style ? style->window_padding.y * 2.0f : 0.0f);
}

/** Anchor an open combo popup just below its preview frame. */
static inline void eli_combo__anchor_popup(eli_id popup_id, eli_vec2 below_left)
{
    for (int n = 0; n < g_eli_open_popup_count; n++) {
        if (g_eli_open_popup_stack[n].popup_id == popup_id) {
            g_eli_open_popup_stack[n].open_popup_pos = below_left;
            g_eli_open_popup_stack[n].open_mouse_pos = below_left;
            return;
        }
    }
}

/* ---------------------------------------------------------------------------
 * Begin / end combo
 * ------------------------------------------------------------------------- */

/**
 * Draw a combo preview button and, if open, begin its dropdown popup. When this
 * returns true the caller emits the item selectables and closes with
 * eli_end_combo. Clicking the preview toggles the dropdown open.
 *
 * @param label          Combo label (drawn to the right; "##" hides it).
 * @param preview_value  Text shown in the preview frame (may be NULL).
 * @param flags          eli_combo_flags (arrow/preview/height options).
 * @return               true if the dropdown is open and its body should be emitted.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_begin_combo(const char *label, const char *preview_value,
                                   eli_combo_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL)
        return false;

    /* Behave like Begin(): snapshot then clear pending next-window data so a stray
     * SetNextWindowSizeConstraints only reaches the popup (restored on open). */
    eli_next_window_data *nwd = eli_get_next_window_data(ctx);
    eli_next_window_data backup = *nwd;
    eli_clear_next_window_data(ctx);
    if (ctx->current_window->skip_items)
        return false;

    eli_window *win = ctx->current_window;
    const eli_style *style = &ctx->style;
    eli_id id = eli_get_id(label);

    float arrow_size = (flags & ELI_COMBO_NO_ARROW_BUTTON) ? 0.0f : eli_get_frame_height();
    eli_vec2 label_size = eli_calc_text_size(label, eli_find_rendered_text_end(label, NULL));
    float preview_w = 0.0f;
    if ((flags & ELI_COMBO_WIDTH_FIT_PREVIEW) && preview_value != NULL)
        preview_w = eli_calc_text_size(preview_value, eli_find_rendered_text_end(preview_value, NULL)).x;
    float w = (flags & ELI_COMBO_NO_PREVIEW)
                  ? arrow_size
                  : ((flags & ELI_COMBO_WIDTH_FIT_PREVIEW)
                         ? arrow_size + preview_w + style->frame_padding.x * 2.0f
                         : eli_calc_item_width());

    eli_vec2 pos = win->cursor_pos;
    float frame_h = label_size.y + style->frame_padding.y * 2.0f;
    eli_rect bb = eli_make_rect(pos.x, pos.y, w, frame_h);
    float total_w = w + (label_size.x > 0.0f ? style->item_inner_spacing.x + label_size.x : 0.0f);
    eli_rect total_bb = eli_make_rect(pos.x, pos.y, total_w, frame_h);
    eli_item_size(eli_rect_size(total_bb), style->frame_padding.y);
    if (!eli_item_add(id, total_bb, 0))
        return false;

    bool hovered = false, held = false;
    bool pressed = eli_button_behavior(bb, id, &hovered, &held, ELI_BUTTON_NONE);
    eli_id popup_id = eli_combo__popup_id(id);
    bool popup_open = eli_is_popup_open_id(popup_id, ELI_POPUP_NONE);
    if (pressed && !popup_open) {
        eli_open_popup_id(popup_id, ELI_POPUP_NONE);
        eli_combo__anchor_popup(popup_id, eli_make_vec2(bb.x, bb.y + bb.h));
        popup_open = true;
    }

    /* Render preview frame + arrow. */
    eli_draw_list *dl = eli_get_window_draw_list();
    eli_col32 frame_col = eli_get_color_u32(hovered ? ELI_COL_FRAME_BG_HOVERED : ELI_COL_FRAME_BG, 1.0f);
    float value_x2 = eli_max_f(bb.x, bb.x + bb.w - arrow_size);
    eli_render_nav_highlight(bb, id);
    if (!(flags & ELI_COMBO_NO_PREVIEW))
        eli_draw_list_add_rect_filled(dl, eli_make_vec2(bb.x, bb.y),
                                      eli_make_vec2(value_x2, bb.y + bb.h), frame_col,
                                      style->frame_rounding, ELI_DRAW_ROUND_CORNERS_ALL);
    if (!(flags & ELI_COMBO_NO_ARROW_BUTTON)) {
        eli_col32 bg = eli_get_color_u32((popup_open || hovered) ? ELI_COL_BUTTON_HOVERED
                                                                 : ELI_COL_BUTTON,
                                         1.0f);
        eli_draw_list_add_rect_filled(dl, eli_make_vec2(value_x2, bb.y),
                                      eli_make_vec2(bb.x + bb.w, bb.y + bb.h), bg,
                                      style->frame_rounding, ELI_DRAW_ROUND_CORNERS_ALL);
        if (value_x2 + arrow_size - style->frame_padding.x <= bb.x + bb.w)
            eli_render_arrow(dl, eli_make_vec2(value_x2 + style->frame_padding.y,
                                               bb.y + style->frame_padding.y),
                             eli_get_color_u32(ELI_COL_TEXT, 1.0f), ELI_DIR_DOWN, 1.0f);
    }
    if (style->frame_border_size > 0.0f)
        eli_draw_list_add_rect(dl, eli_make_vec2(bb.x, bb.y), eli_make_vec2(bb.x + bb.w, bb.y + bb.h),
                               eli_get_color_u32(ELI_COL_BORDER, 1.0f), style->frame_rounding,
                               ELI_DRAW_ROUND_CORNERS_ALL, style->frame_border_size);

    if (preview_value != NULL && !(flags & ELI_COMBO_NO_PREVIEW)) {
        eli_vec2 tmin = eli_make_vec2(bb.x + style->frame_padding.x, bb.y + style->frame_padding.y);
        eli_vec2 tmax = eli_make_vec2(value_x2, bb.y + bb.h);
        eli_selectable__render_text(tmin, tmax, preview_value,
                                    eli_calc_text_size(preview_value,
                                                       eli_find_rendered_text_end(preview_value, NULL)),
                                    eli_get_color_u32(ELI_COL_TEXT, 1.0f));
    }
    if (label_size.x > 0.0f)
        eli_render_text(eli_make_vec2(bb.x + bb.w + style->item_inner_spacing.x,
                                      bb.y + style->frame_padding.y),
                        eli_get_color_u32(ELI_COL_TEXT, 1.0f), label, NULL, true);

    if (!popup_open)
        return false;

    /* Restore any user next-window data (e.g. size constraints) for the popup and
     * ensure the popup is at least as wide as the preview frame. */
    *nwd = backup;
    eli_vec2 cmin = nwd->has_size_constraint ? nwd->size_constraint_min : eli_make_vec2(0.0f, 0.0f);
    eli_vec2 cmax = nwd->has_size_constraint ? nwd->size_constraint_max : eli_make_vec2(-1.0f, -1.0f);
    cmin.x = eli_max_f(cmin.x, w);
    eli_set_next_window_size_constraints(cmin, cmax);

    char name[24];
    snprintf(name, sizeof name, "##Combo_%08x", (unsigned)popup_id);
    return eli_popup__begin(popup_id, name, NULL, ELI_POPUP_WINDOW_FLAGS, false);
}

/**
 * Close the combo dropdown opened by a eli_begin_combo that returned true.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_end_combo(void)
{
    eli_end_popup();
}

/* ---------------------------------------------------------------------------
 * Convenience combos
 * ------------------------------------------------------------------------- */

/**
 * A self-contained combo driven by a getter callback: shows the current item as
 * the preview and, while open, one selectable per item; writes the chosen index
 * back through current_item.
 *
 * @param label          Combo label.
 * @param current_item   In/out selected index (must be non-NULL).
 * @param getter         Returns the label for item idx (NULL entry -> placeholder).
 * @param user_data      Opaque pointer forwarded to the getter.
 * @param items_count    Number of items.
 * @param popup_max_height_in_items  Visible rows before scroll (< 0 = default).
 * @return               true on the frame the selection changes.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_combo_fn(const char *label, int *current_item,
                                const char *(*getter)(void *user_data, int idx), void *user_data,
                                int items_count, int popup_max_height_in_items)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || current_item == NULL || getter == NULL)
        return false;

    const char *preview = NULL;
    if (*current_item >= 0 && *current_item < items_count)
        preview = getter(user_data, *current_item);

    eli_id combo_id = eli_get_id(label);

    if (popup_max_height_in_items != -1) {
        float max_h = eli_combo__max_popup_height(popup_max_height_in_items);
        eli_set_next_window_size_constraints(eli_make_vec2(0.0f, 0.0f),
                                             eli_make_vec2(-1.0f, max_h));
    }

    if (!eli_begin_combo(label, preview, ELI_COMBO_NONE))
        return false;

    bool value_changed = false;
    for (int i = 0; i < items_count; i++) {
        const char *item_text = getter(user_data, i);
        if (item_text == NULL)
            item_text = "*Unknown item*";
        eli_push_id_int(i);
        bool item_selected = (i == *current_item);
        if (eli_selectable(item_text, item_selected, ELI_SELECTABLE_NONE, eli_make_vec2(0.0f, 0.0f)) &&
            *current_item != i) {
            value_changed = true;
            *current_item = i;
        }
        eli_pop_id();
    }
    eli_end_combo();

    if (value_changed)
        eli_mark_item_edited(combo_id);
    return value_changed;
}

/**
 * A self-contained combo over an array of C strings.
 *
 * @param label          Combo label.
 * @param current_item   In/out selected index (must be non-NULL).
 * @param items          Array of item labels.
 * @param items_count    Number of items.
 * @param popup_max_height_in_items  Visible rows before scroll (< 0 = default).
 * @return               true on the frame the selection changes.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_combo(const char *label, int *current_item, const char *const items[],
                             int items_count, int popup_max_height_in_items)
{
    return eli_combo_fn(label, current_item, eli_widget__array_getter, (void *)items, items_count,
                        popup_max_height_in_items);
}

/** Zero-separated string getter used by eli_combo_str. */
static inline const char *eli_combo__zero_sep_getter(void *user_data, int idx)
{
    const char *p = (const char *)user_data;
    int i = 0;
    while (*p) {
        if (i == idx)
            return p;
        p += strlen(p) + 1;
        i++;
    }
    return NULL;
}

/**
 * A self-contained combo over a single "a\0b\0c\0\0" zero-separated item string.
 *
 * @param label          Combo label.
 * @param current_item   In/out selected index (must be non-NULL).
 * @param items_separated_by_zeros  Items joined by NULs, terminated by an empty item.
 * @param popup_max_height_in_items  Visible rows before scroll (< 0 = default).
 * @return               true on the frame the selection changes.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_combo_str(const char *label, int *current_item,
                                 const char *items_separated_by_zeros, int popup_max_height_in_items)
{
    if (items_separated_by_zeros == NULL)
        return false;
    int items_count = 0;
    const char *p = items_separated_by_zeros;
    while (*p) {
        p += strlen(p) + 1;
        items_count++;
    }
    return eli_combo_fn(label, current_item, eli_combo__zero_sep_getter,
                        (void *)items_separated_by_zeros, items_count, popup_max_height_in_items);
}

#endif /* ELI_WIDGETS_ELI_COMBO_H */
