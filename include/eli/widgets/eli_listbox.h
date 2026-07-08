/**
 * @file eli_listbox.h
 * @brief Phase 14 list boxes: eli_begin_list_box / eli_end_list_box (a framed,
 *        scrollable child region the caller fills with selectables) and the
 *        convenience eli_list_box (items array) / eli_list_box_fn (getter callback)
 *        that render one selectable row per item and report the chosen index.
 *        Mirrors Dear ImGui's BeginListBox / ListBox.
 *
 * @status Phase 14 list box widget in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WIDGETS_ELI_LISTBOX_H
#define ELI_WIDGETS_ELI_LISTBOX_H

#include "eli_selectable.h"

#include "../window/eli_window_child.h"
#include "../layout/eli_layout.h"

#include "../core/eli_platform.h"

/* ---------------------------------------------------------------------------
 * List box container
 * ------------------------------------------------------------------------- */

/**
 * Open a framed, scrollable list box region. Fill it with eli_selectable rows and
 * close it with eli_end_list_box only when this returns true. A group wraps the
 * frame and its trailing label so whole-widget item queries work.
 *
 * @param label     List box label (drawn to the right of the frame; "##" hides it).
 * @param size_arg  Frame size; a zero axis defaults to the item width (x) or ~7.25
 *                  text lines tall (y).
 * @return          true if the list box body should be emitted.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_begin_list_box(const char *label, eli_vec2 size_arg)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return false;
    eli_window *win = ctx->current_window;
    const eli_style *style = &ctx->style;

    eli_id id = eli_get_id(label);
    eli_vec2 label_size = eli_calc_text_size(label, eli_find_rendered_text_end(label, NULL));

    float default_h = eli_get_text_line_height_with_spacing() * 7.25f + style->frame_padding.y * 2.0f;
    eli_vec2 size = eli_calc_item_size(size_arg, eli_calc_item_width(), default_h);
    eli_vec2 frame_size = eli_make_vec2(size.x, eli_max_f(size.y, label_size.y));
    eli_vec2 pos = win->cursor_pos;
    eli_rect frame_bb = eli_make_rect(pos.x, pos.y, frame_size.x, frame_size.y);

    eli_begin_group();
    if (label_size.x > 0.0f) {
        /* Render the trailing label; the child fills the frame to its left. */
        eli_vec2 label_pos = eli_make_vec2(frame_bb.x + frame_size.x + style->item_inner_spacing.x,
                                           frame_bb.y + style->frame_padding.y);
        eli_render_text(label_pos, eli_get_color_u32(ELI_COL_TEXT, 1.0f), label, NULL, true);
    }

    bool visible = eli_begin_child_id(id, frame_size, ELI_CHILD_FRAME_STYLE, 0);
    if (!visible) {
        eli_end_child();
        eli_end_group();
        return false;
    }
    return true;
}

/**
 * Close the list box opened by a eli_begin_list_box that returned true.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_end_list_box(void)
{
    eli_end_child();
    eli_end_group();
}

/* ---------------------------------------------------------------------------
 * Convenience list boxes
 * ------------------------------------------------------------------------- */

/**
 * A self-contained list box driven by a getter callback: renders one selectable
 * per item and writes the chosen index back through current_item.
 *
 * @param label          List box label.
 * @param current_item   In/out selected index (must be non-NULL).
 * @param getter         Returns the label for item idx (NULL entry -> placeholder).
 * @param user_data      Opaque pointer forwarded to the getter.
 * @param items_count    Number of items.
 * @param height_in_items Visible item rows (< 0 = min(items,7)).
 * @return               true on the frame the selection changes.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_list_box_fn(const char *label, int *current_item,
                                   const char *(*getter)(void *user_data, int idx), void *user_data,
                                   int items_count, int height_in_items)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || current_item == NULL || getter == NULL)
        return false;

    if (height_in_items < 0)
        height_in_items = items_count < 7 ? items_count : 7;
    float height_f = (float)height_in_items + 0.25f;
    eli_vec2 size = eli_make_vec2(0.0f, (float)(int)(eli_get_text_line_height_with_spacing() *
                                                         height_f +
                                                     ctx->style.frame_padding.y * 2.0f));

    if (!eli_begin_list_box(label, size))
        return false;

    bool value_changed = false;
    for (int i = 0; i < items_count; i++) {
        const char *item_text = getter(user_data, i);
        if (item_text == NULL)
            item_text = "*Unknown item*";
        eli_push_id_int(i);
        bool item_selected = (i == *current_item);
        if (eli_selectable(item_text, item_selected, ELI_SELECTABLE_NONE,
                           eli_make_vec2(0.0f, 0.0f))) {
            *current_item = i;
            value_changed = true;
        }
        eli_pop_id();
    }
    eli_end_list_box();
    return value_changed;
}

/** Array getter used by eli_list_box / eli_combo (const char* const items[]). */
static inline const char *eli_widget__array_getter(void *user_data, int idx)
{
    const char *const *items = (const char *const *)user_data;
    return items[idx];
}

/**
 * A self-contained list box over an array of C strings.
 *
 * @param label           List box label.
 * @param current_item    In/out selected index (must be non-NULL).
 * @param items           Array of item labels.
 * @param items_count     Number of items.
 * @param height_in_items Visible item rows (< 0 = min(items,7)).
 * @return                true on the frame the selection changes.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_list_box(const char *label, int *current_item, const char *const items[],
                                int items_count, int height_in_items)
{
    return eli_list_box_fn(label, current_item, eli_widget__array_getter, (void *)items, items_count,
                           height_in_items);
}

#endif /* ELI_WIDGETS_ELI_LISTBOX_H */
