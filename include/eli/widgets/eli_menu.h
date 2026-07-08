/**
 * @file eli_menu.h
 * @brief Phase 16 menus: window menu bars (eli_begin_menu_bar / eli_end_menu_bar),
 *        the top-of-screen main menu bar (eli_begin_main_menu_bar /
 *        eli_end_main_menu_bar), menu entries that open submenu popups
 *        (eli_begin_menu / eli_end_menu), and selectable menu items with optional
 *        shortcut text and a selected check mark (eli_menu_item / eli_menu_item_bool).
 *
 * A menu dropdown (or submenu) IS a popup: this module reuses the Phase 17 popup
 * stack (eli_open_popup_id / eli_begin_popup / eli_end_popup) and only overrides the
 * appear position so a bar menu opens below its button and a submenu opens to the
 * right of its row. The one piece of file-static state is the menu-bar layout
 * scratch: because the core window does not reserve a menu-bar strip, this module
 * lays out the bar horizontally at the top of the window's work area and advances
 * the body cursor below it at eli_end_menu_bar.
 *
 * @status Phase 16 menus in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WIDGETS_ELI_MENU_H
#define ELI_WIDGETS_ELI_MENU_H

#include "eli_popup.h"
#include "eli_button_widgets.h"
#include "eli_widget_behavior.h"

#include "../layout/eli_layout.h"
#include "../core/eli_platform.h"

/* ---------------------------------------------------------------------------
 * Menu-bar layout scratch (file-static)
 * ------------------------------------------------------------------------- */

/**
 * State captured between eli_begin_menu_bar and eli_end_menu_bar so bar menu
 * buttons lay out horizontally and the body cursor can be advanced below the bar.
 * Only one menu bar can be appended at a time (bars never nest).
 */
typedef struct eli_menu_bar_scratch {
    bool        appending;   /* currently between begin_menu_bar and end_menu_bar */
    eli_window *bar_window;  /* window that owns the active menu bar */
    eli_rect    bar_rect;    /* the bar strip in screen space */
    eli_vec2    bar_start;   /* work-area cursor position captured at begin */
    bool        is_main;     /* the top-of-screen main menu bar */
} eli_menu_bar_scratch;

static eli_menu_bar_scratch g_eli_menu_bar = {0};

/* Identity of the bar menu whose dropdown is currently shown, so clicking the same
 * bar button toggles it closed instead of immediately reopening it (the popup
 * click-outside pass closes it before the button behavior runs). */
static eli_id g_eli_menu_open_root_id = 0u;

/* Number of eli_begin_menu dropdowns currently on the begin-popup stack, and the
 * open-popup level of the outermost one, so choosing a menu item closes the whole
 * menu chain rather than only the innermost submenu. */
static int g_eli_menu_depth = 0;
static int g_eli_menu_root_level = 0;

/* Height of the last main menu bar, published for a host that reserves screen
 * space for it (the core window system does not consume this yet). */
static float g_eli_main_menu_bar_height = 0.0f;

/* ---------------------------------------------------------------------------
 * Geometry helpers
 * ------------------------------------------------------------------------- */

/**
 * Natural minimum width of a menu row: leading padding, a check-mark icon column,
 * the label, plus (when present) the right-aligned shortcut and a submenu arrow.
 * Used both to size the dropdown popup and to right-align the shortcut text.
 *
 * @param label      Row label (its visible part stops at "##").
 * @param shortcut   Optional shortcut text, or NULL/empty for none.
 * @param has_arrow  true to reserve room for a submenu arrow on the right.
 * @return           Minimum row width in pixels.
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline float eli_menu__calc_min_width(const char *label, const char *shortcut,
                                             bool has_arrow)
{
    const eli_style *style = eli_get_style();
    if (style == NULL || label == NULL)
        return 0.0f;
    float font = eli_get_font_size();
    eli_vec2 ls = eli_calc_text_size(label, eli_find_rendered_text_end(label, NULL));
    float w = style->frame_padding.x * 2.0f + font + style->item_inner_spacing.x + ls.x;
    if (shortcut != NULL && shortcut[0] != '\0') {
        eli_vec2 ss = eli_calc_text_size(shortcut, NULL);
        w += style->item_spacing.x * 2.0f + ss.x;
    }
    if (has_arrow)
        w += style->item_inner_spacing.x + font;
    return w;
}

/**
 * Emit a full-width menu row: hover highlight, an optional selected check mark in
 * the icon column, the label, a right-aligned shortcut, and an optional submenu
 * arrow. Advances the layout cursor by the row and registers the item.
 *
 * @param id           Row item id (0 for a non-interactive row).
 * @param label        Row label.
 * @param shortcut     Optional shortcut text drawn right-aligned, or NULL.
 * @param selected     true to draw a check mark in the icon column.
 * @param enabled      false to render disabled (no interaction, dimmed text).
 * @param has_arrow    true to draw a submenu arrow on the right edge.
 * @param out_hovered  Receives whether the row is hovered (may be NULL).
 * @return             true on the frame the row is activated (released inside).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_menu__item_row(eli_id id, const char *label, const char *shortcut,
                                      bool selected, bool enabled, bool has_arrow,
                                      bool *out_hovered)
{
    if (out_hovered != NULL)
        *out_hovered = false;
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return false;
    eli_window *win = ctx->current_window;
    const eli_style *style = &ctx->style;

    const char *label_end = eli_find_rendered_text_end(label, NULL);
    eli_vec2 label_size = eli_calc_text_size(label, label_end);
    float min_w = eli_menu__calc_min_width(label, shortcut, has_arrow);
    float w = eli_max_f(min_w, eli_get_content_region_avail().x);
    float h = eli_max_f(label_size.y, eli_get_font_size()) + style->frame_padding.y * 2.0f;

    eli_vec2 pos = win->cursor_pos;
    eli_rect bb = eli_make_rect(pos.x, pos.y, w, h);
    eli_item_size(eli_make_vec2(w, h), style->frame_padding.y);
    bool visible = eli_item_add(id, bb, 0);

    bool hovered = false, held = false, pressed = false;
    if (enabled && visible)
        pressed = eli_button_behavior(bb, id, &hovered, &held, ELI_BUTTON_PRESSED_ON_CLICK_RELEASE);
    if (out_hovered != NULL)
        *out_hovered = hovered;

    eli_draw_list *dl = eli_get_window_draw_list();
    if (hovered)
        eli_draw_list_add_rect_filled(dl, eli_rect_min(bb), eli_rect_max(bb),
                                      eli_get_color_u32(ELI_COL_HEADER_HOVERED, 1.0f), 0.0f, 0);

    float text_y = pos.y + style->frame_padding.y;
    float icon_w = eli_get_font_size();
    eli_col32 text_col = eli_get_color_u32(enabled ? ELI_COL_TEXT : ELI_COL_TEXT_DISABLED, 1.0f);

    if (selected)
        eli_render_check_mark(dl, eli_make_vec2(pos.x + style->frame_padding.x, text_y),
                              eli_get_color_u32(ELI_COL_CHECK_MARK, 1.0f), icon_w);

    float label_x = pos.x + style->frame_padding.x + icon_w + style->item_inner_spacing.x;
    eli_render_text(eli_make_vec2(label_x, text_y), text_col, label, label_end, false);

    if (shortcut != NULL && shortcut[0] != '\0') {
        eli_vec2 ss = eli_calc_text_size(shortcut, NULL);
        float arrow_reserve = has_arrow ? (icon_w + style->item_inner_spacing.x) : 0.0f;
        float sx = bb.x + bb.w - style->frame_padding.x - arrow_reserve - ss.x;
        eli_render_text(eli_make_vec2(sx, text_y),
                        eli_get_color_u32(ELI_COL_TEXT_DISABLED, 1.0f), shortcut, NULL, false);
    }

    if (has_arrow)
        eli_render_arrow(dl, eli_make_vec2(bb.x + bb.w - icon_w - style->frame_padding.x, pos.y),
                         text_col, ELI_DIR_RIGHT, 1.0f);
    return pressed;
}

/**
 * Open a menu/submenu popup at an explicit screen position, overriding the popup
 * system's default "appear at mouse" placement. The position is applied on the
 * appearing frame; the dropdown then keeps that position.
 *
 * @param id   Menu popup id (same id used for the menu button/row).
 * @param pos  Preferred top-left position of the dropdown.
 */
static inline void eli_menu__open_at(eli_id id, eli_vec2 pos)
{
    int level = g_eli_begin_popup_count;
    eli_open_popup_id(id, ELI_POPUP_NO_REOPEN);
    if (g_eli_open_popup_count > level && g_eli_open_popup_stack[level].popup_id == id) {
        g_eli_open_popup_stack[level].open_popup_pos = pos;
        g_eli_open_popup_stack[level].open_mouse_pos = pos;
    }
}

/* ---------------------------------------------------------------------------
 * Menu bar
 * ------------------------------------------------------------------------- */

/**
 * Begin the current window's menu bar. The window must carry ELI_WINDOW_MENU_BAR.
 * Lays the bar out horizontally at the top of the window's work area; balance with
 * eli_end_menu_bar only when this returns true.
 *
 * @return true if the menu bar is open and its menus should be emitted.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_begin_menu_bar(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return false;
    eli_window *win = ctx->current_window;
    if ((win->flags & ELI_WINDOW_MENU_BAR) == 0 || g_eli_menu_bar.appending)
        return false;

    const eli_style *style = &ctx->style;
    float bar_h = eli_get_frame_height();
    eli_vec2 start = win->cursor_pos;
    float right = win->content_region_rect.x + win->content_region_rect.w;
    eli_rect bar = eli_make_rect(start.x, start.y, eli_max_f(0.0f, right - start.x), bar_h);

    eli_draw_list_add_rect_filled(&win->draw_list, eli_rect_min(bar), eli_rect_max(bar),
                                  eli_get_color_u32(ELI_COL_MENU_BAR_BG, 1.0f), 0.0f, 0);

    g_eli_menu_bar.appending = true;
    g_eli_menu_bar.bar_window = win;
    g_eli_menu_bar.bar_rect = bar;
    g_eli_menu_bar.bar_start = start;
    g_eli_menu_bar.is_main = false;

    win->cursor_pos.x = start.x + style->item_spacing.x;
    win->cursor_pos.y = start.y;
    return true;
}

/**
 * End the current window's menu bar and drop the body cursor below the bar strip so
 * subsequent widgets flow underneath it.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_end_menu_bar(void)
{
    if (!g_eli_menu_bar.appending)
        return;
    eli_context *ctx = eli_get_current_context();
    eli_window *win = g_eli_menu_bar.bar_window;
    g_eli_menu_bar.appending = false;
    if (ctx == NULL || win == NULL) {
        g_eli_menu_bar.bar_window = NULL;
        return;
    }
    const eli_style *style = &ctx->style;
    win->cursor_pos = eli_make_vec2(g_eli_menu_bar.bar_start.x,
                                    g_eli_menu_bar.bar_start.y + g_eli_menu_bar.bar_rect.h +
                                        style->item_spacing.y);
    win->cursor_pos_prev_line = win->cursor_pos;
    win->curr_line_size = eli_make_vec2(0.0f, 0.0f);
    win->prev_line_size = eli_make_vec2(0.0f, 0.0f);
    win->is_same_line = false;
    g_eli_menu_bar.bar_window = NULL;
}

/**
 * Begin the top-of-screen main menu bar, hosting it in a borderless full-width
 * window pinned at the display origin. Balance with eli_end_main_menu_bar only when
 * this returns true.
 *
 * @return true if the main menu bar is open and its menus should be emitted.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_begin_main_menu_bar(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return false;
    const eli_style *style = &ctx->style;
    eli_vec2 disp = ctx->io.display_size;
    float bar_h = eli_get_frame_height();
    float win_h = bar_h + style->window_padding.y * 2.0f;

    eli_set_next_window_pos(eli_make_vec2(0.0f, 0.0f), ELI_COND_ALWAYS, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(disp.x, win_h), ELI_COND_ALWAYS);
    eli_window_flags flags = ELI_WINDOW_NO_TITLEBAR | ELI_WINDOW_NO_RESIZE | ELI_WINDOW_NO_MOVE |
                             ELI_WINDOW_NO_SCROLLBAR | ELI_WINDOW_NO_SAVED_SETTINGS |
                             ELI_WINDOW_NO_COLLAPSE | ELI_WINDOW_MENU_BAR;
    if (!eli_begin("##MainMenuBar", NULL, flags)) {
        eli_end();
        return false;
    }
    if (!eli_begin_menu_bar()) {
        eli_end();
        return false;
    }
    g_eli_menu_bar.is_main = true;
    g_eli_main_menu_bar_height = g_eli_menu_bar.bar_rect.h;
    return true;
}

/**
 * End the main menu bar and its host window.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_end_main_menu_bar(void)
{
    eli_end_menu_bar();
    eli_end();
}

/* ---------------------------------------------------------------------------
 * Menus (dropdowns / submenus)
 * ------------------------------------------------------------------------- */

/** Emit a horizontal bar-menu button; sets *want_open / *want_close per the click,
 *  toggle guard, and hover-switch, and returns the dropdown's appear position. */
static inline eli_vec2 eli_menu__bar_button(eli_context *ctx, eli_window *win, eli_id id,
                                            const char *label, bool enabled, bool menu_is_open,
                                            bool *want_open, bool *want_close)
{
    const eli_style *style = &ctx->style;
    const char *label_end = eli_find_rendered_text_end(label, NULL);
    eli_vec2 label_size = eli_calc_text_size(label, label_end);
    float w = label_size.x + style->frame_padding.x * 2.0f;
    eli_rect bb = eli_make_rect(win->cursor_pos.x, g_eli_menu_bar.bar_rect.y, w,
                                g_eli_menu_bar.bar_rect.h);

    bool hovered = false, held = false, pressed = false;
    if (enabled) {
        eli_item_add(id, bb, 0);
        pressed = eli_button_behavior(bb, id, &hovered, &held, ELI_BUTTON_PRESSED_ON_CLICK);
    }

    bool was_open = (g_eli_menu_open_root_id == id);
    if (pressed) {
        if (was_open)
            *want_close = true;
        else
            *want_open = true;
    } else if (!menu_is_open && hovered && g_eli_menu_open_root_id != 0u &&
               g_eli_menu_open_root_id != id) {
        *want_open = true; /* slide across the bar to a sibling menu */
    }

    if (menu_is_open || hovered)
        eli_draw_list_add_rect_filled(&win->draw_list, eli_rect_min(bb), eli_rect_max(bb),
                                      eli_get_color_u32(menu_is_open ? ELI_COL_HEADER
                                                                     : ELI_COL_HEADER_HOVERED,
                                                        1.0f),
                                      0.0f, 0);
    eli_col32 col = eli_get_color_u32(enabled ? ELI_COL_TEXT : ELI_COL_TEXT_DISABLED, 1.0f);
    eli_render_text(eli_make_vec2(bb.x + style->frame_padding.x, bb.y + style->frame_padding.y),
                    col, label, label_end, false);

    win->cursor_pos.x = bb.x + w;
    win->cursor_pos.y = g_eli_menu_bar.bar_rect.y;
    return eli_make_vec2(bb.x, bb.y + bb.h);
}

/**
 * Begin a menu entry. In a menu bar this is a horizontal button whose click opens a
 * dropdown below it; inside a menu it is a row with a submenu arrow whose hover or
 * click opens a dropdown to the right. Balance with eli_end_menu only when this
 * returns true.
 *
 * @param label    Menu label (its visible part stops at "##").
 * @param enabled  false to render the entry disabled and non-interactive.
 * @return         true if the dropdown is open and its items should be emitted.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_begin_menu(const char *label, bool enabled)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items ||
        label == NULL)
        return false;
    eli_window *win = ctx->current_window;

    eli_id id = eli_get_id(label);
    bool menu_is_open = eli_is_popup_open_id(id, ELI_POPUP_NONE);
    bool in_menu_bar = (g_eli_menu_bar.appending && win == g_eli_menu_bar.bar_window);

    bool want_open = false, want_close = false;
    eli_vec2 popup_pos;
    if (in_menu_bar) {
        popup_pos = eli_menu__bar_button(ctx, win, id, label, enabled, menu_is_open, &want_open,
                                         &want_close);
    } else {
        bool hovered = false;
        bool pressed = eli_menu__item_row(id, label, NULL, false, enabled, true, &hovered);
        if (enabled && !menu_is_open && (pressed || hovered))
            want_open = true;
        eli_rect r = eli_get_item_rect();
        popup_pos = eli_make_vec2(win->pos.x + win->size.x - ctx->style.item_spacing.x, r.y);
    }

    if (want_close)
        eli_close_popup_to_level(g_eli_begin_popup_count);
    if (want_open && enabled)
        eli_menu__open_at(id, popup_pos);

    menu_is_open = eli_is_popup_open_id(id, ELI_POPUP_NONE);
    if (menu_is_open) {
        bool open = eli_begin_popup(label, ELI_WINDOW_NONE);
        if (open) {
            if (g_eli_menu_depth == 0)
                g_eli_menu_root_level = g_eli_begin_popup_count - 1;
            g_eli_menu_depth++;
        }
        if (in_menu_bar)
            g_eli_menu_open_root_id = id;
        return open;
    }
    if (in_menu_bar && g_eli_menu_open_root_id == id)
        g_eli_menu_open_root_id = 0u;
    return false;
}

/**
 * End a menu dropdown opened by an eli_begin_menu that returned true.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_end_menu(void)
{
    if (g_eli_menu_depth > 0)
        g_eli_menu_depth--;
    eli_end_popup();
}

/* ---------------------------------------------------------------------------
 * Menu items
 * ------------------------------------------------------------------------- */

/**
 * A selectable menu item with an optional shortcut label and selected check mark.
 * Choosing it closes the whole open menu chain.
 *
 * @param label     Item label (its visible part stops at "##").
 * @param shortcut  Optional shortcut text drawn right-aligned (display only), or NULL.
 * @param selected  true to show a check mark next to the label.
 * @param enabled   false to render disabled and non-interactive.
 * @return          true on the frame the item is activated.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_menu_item(const char *label, const char *shortcut, bool selected,
                                 bool enabled)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items ||
        label == NULL)
        return false;

    eli_id id = eli_get_id(label);
    bool pressed = eli_menu__item_row(id, label, shortcut, selected, enabled, false, NULL);
    if (pressed && enabled) {
        if (g_eli_menu_depth > 0)
            eli_close_popup_to_level(g_eli_menu_root_level);
        else if (g_eli_begin_popup_count > 0)
            eli_close_current_popup();
    }
    return pressed && enabled;
}

/**
 * A menu item bound to a bool: choosing it toggles the bool and the item shows a
 * check mark while the bool is set.
 *
 * @param label       Item label.
 * @param shortcut    Optional shortcut text drawn right-aligned, or NULL.
 * @param p_selected  Pointer to the bool to toggle (must be non-NULL).
 * @param enabled     false to render disabled and non-interactive.
 * @return            true on the frame the item is activated.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_menu_item_bool(const char *label, const char *shortcut, bool *p_selected,
                                      bool enabled)
{
    if (p_selected == NULL)
        return false;
    bool pressed = eli_menu_item(label, shortcut, *p_selected, enabled);
    if (pressed)
        *p_selected = !*p_selected;
    return pressed;
}

#endif /* ELI_WIDGETS_ELI_MENU_H */
