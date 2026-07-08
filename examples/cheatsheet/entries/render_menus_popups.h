/**
 * @file render_menus_popups.h
 * @brief Live widget renderers for the "Menus", "Popups", and "Tooltips"
 *        cheatsheet categories. Each function draws exactly one showcase
 *        interaction and owns its own function-static state; they are called
 *        every frame from cheat_render_card() inside a live elimgui frame.
 *
 * Include once (before rows_menus_popups.h) in a translation unit that already
 * has a full elimgui frame orchestrator so popup / tooltip infrastructure is
 * available.
 *
 * @status Cheatsheet content — Menus, Popups, Tooltips.
 * @issues None
 * @todo None
 */
#ifndef CHEAT_RENDER_MENUS_POPUPS_H
#define CHEAT_RENDER_MENUS_POPUPS_H

#include <eli/elimgui.h>

/* =========================================================================
 * Menus
 * =========================================================================
 *
 * All three menu entries embed a small fixed-height child window that carries
 * ELI_WINDOW_MENU_BAR so eli_begin_menu_bar() can activate. This is the only
 * way to show a real menu bar inside a cheatsheet card; the card itself lives
 * in a plain scrollable child window that has no MENU_BAR flag.
 * ========================================================================= */

/**
 * Window Menu Bar — demonstrates the complete menu-bar sequence:
 * child window with ELI_WINDOW_MENU_BAR, eli_begin_menu_bar,
 * eli_begin_menu / eli_end_menu, eli_menu_item, eli_end_menu_bar.
 */
static void cheat_render_menu_bar(void)
{
    static bool show_grid = false;
    eli_vec2 sz = eli_make_vec2(-1.0f, 72.0f);
    bool open = eli_begin_child("##mb_demo", sz,
                                ELI_CHILD_BORDERS, ELI_WINDOW_MENU_BAR);
    if (open) {
        if (eli_begin_menu_bar()) {
            if (eli_begin_menu("File", true)) {
                eli_menu_item("New",     NULL,     false, true);
                eli_menu_item("Open...", "Ctrl+O", false, true);
                eli_end_menu();
            }
            if (eli_begin_menu("View", true)) {
                eli_menu_item_bool("Show Grid", NULL, &show_grid, true);
                eli_end_menu();
            }
            eli_end_menu_bar();
        }
        eli_text("Grid: %s", show_grid ? "on" : "off");
    }
    eli_end_child();
}

/**
 * Menu Item (with shortcut + checked state) — shows eli_menu_item with a
 * right-aligned shortcut label and a persistent check mark that toggles on
 * each activation. The caller manually tracks and toggles the bool.
 */
static void cheat_render_menu_item_checked(void)
{
    static bool checked = false;
    eli_vec2 sz = eli_make_vec2(-1.0f, 72.0f);
    bool open = eli_begin_child("##mi_chk", sz,
                                ELI_CHILD_BORDERS, ELI_WINDOW_MENU_BAR);
    if (open) {
        if (eli_begin_menu_bar()) {
            if (eli_begin_menu("Edit", true)) {
                eli_menu_item("Cut",  "Ctrl+X", false,   true);
                if (eli_menu_item("Copy", "Ctrl+C", checked, true))
                    checked = !checked;
                eli_end_menu();
            }
            eli_end_menu_bar();
        }
        eli_text("'Copy' checked: %s", checked ? "yes" : "no");
    }
    eli_end_child();
}

/**
 * Menu Item (bool toggle) — shows eli_menu_item_bool which automatically
 * reads, toggles, and writes back the bool on each activation, removing
 * the manual if/toggle from the caller.
 */
static void cheat_render_menu_item_bool(void)
{
    static bool dark_mode = true;
    eli_vec2 sz = eli_make_vec2(-1.0f, 72.0f);
    bool open = eli_begin_child("##mi_bool", sz,
                                ELI_CHILD_BORDERS, ELI_WINDOW_MENU_BAR);
    if (open) {
        if (eli_begin_menu_bar()) {
            if (eli_begin_menu("Prefs", true)) {
                eli_menu_item_bool("Dark Mode", NULL, &dark_mode, true);
                eli_end_menu();
            }
            eli_end_menu_bar();
        }
        eli_text("Dark Mode: %s", dark_mode ? "on" : "off");
    }
    eli_end_child();
}

/* =========================================================================
 * Popups
 * =========================================================================
 *
 * Popups are real windows managed by the global popup stack. The render
 * functions call eli_open_popup on the same frame the trigger fires, then
 * call the matching begin_popup* every frame; the stack decides whether to
 * show the window.
 * ========================================================================= */

/**
 * Popup (button open) — click the button to open a popup menu of items via
 * eli_open_popup + eli_begin_popup. Choosing an item records the action name
 * and the popup closes itself through eli_menu_item's built-in close.
 */
static void cheat_render_open_popup(void)
{
    static const char *last = "(none)";
    if (eli_button("Options..."))
        eli_open_popup("##opts", ELI_POPUP_NONE);
    eli_same_line(0.0f, -1.0f);
    eli_text("last: %s", last);
    if (eli_begin_popup("##opts", ELI_WINDOW_NONE)) {
        if (eli_menu_item("Cut",   "Ctrl+X", false, true)) last = "Cut";
        if (eli_menu_item("Copy",  "Ctrl+C", false, true)) last = "Copy";
        if (eli_menu_item("Paste", "Ctrl+V", false, true)) last = "Paste";
        eli_end_popup();
    }
}

/**
 * Context Popup (right-click item) — right-click the text label to open a
 * context popup via eli_begin_popup_context_item with no explicit
 * eli_open_popup call required; the function handles the open logic itself.
 */
static void cheat_render_context_item(void)
{
    static const char *action = "(right-click the label above)";
    eli_text("[Right-click me]");
    if (eli_begin_popup_context_item("##ctx", ELI_POPUP_MOUSE_BUTTON_RIGHT)) {
        if (eli_menu_item("Inspect", NULL, false, true)) action = "Inspect";
        if (eli_menu_item("Delete",  NULL, false, true)) action = "Delete";
        eli_end_popup();
    }
    eli_text_disabled("%s", action);
}

/**
 * Popup Modal — button opens a named modal dialog via eli_open_popup +
 * eli_begin_popup_modal. The modal dims the background and traps input until
 * the user clicks OK or Cancel, both of which call eli_close_current_popup.
 */
static void cheat_render_popup_modal(void)
{
    static bool result_shown = false;
    static bool confirmed    = false;
    if (eli_button("Delete item..."))
        eli_open_popup("Confirm Delete##modal", ELI_POPUP_NONE);
    if (result_shown)
        eli_text_disabled(confirmed ? "Deleted." : "Cancelled.");
    if (eli_begin_popup_modal("Confirm Delete##modal", NULL, ELI_WINDOW_NONE)) {
        eli_text("Delete this item? This cannot be undone.");
        eli_spacing();
        if (eli_button("OK")) {
            confirmed = true; result_shown = true;
            eli_close_current_popup();
        }
        eli_same_line(0.0f, -1.0f);
        if (eli_button("Cancel")) {
            confirmed = false; result_shown = true;
            eli_close_current_popup();
        }
        eli_end_popup();
    }
}

/* =========================================================================
 * Tooltips
 * =========================================================================
 *
 * Three escalating patterns: instant (caller checks hovered), delayed one-shot
 * (eli_set_item_tooltip gates on the hover delay automatically), and rich
 * (eli_begin_item_tooltip lets you place arbitrary widgets inside).
 * ========================================================================= */

/**
 * Tooltip (instant) — hover the button to immediately show a tooltip via a
 * manual eli_is_item_hovered check followed by eli_set_tooltip. No delay;
 * the tooltip appears on the first hovered frame.
 */
static void cheat_render_set_tooltip(void)
{
    eli_button("Hover me");
    if (eli_is_item_hovered(ELI_HOVERED_NONE))
        eli_set_tooltip("I am an instant tooltip!");
}

/**
 * Item Tooltip (delayed, one-shot) — eli_set_item_tooltip automatically gates
 * on the style hover delay (hover_flags_for_tooltip_mouse). No manual hovered
 * check; just call it right after the widget.
 */
static void cheat_render_set_item_tooltip(void)
{
    eli_button("Hover (with delay)");
    eli_set_item_tooltip("Appears after the style hover delay");
}

/**
 * Rich Tooltip — eli_begin_item_tooltip returns true once the previous item
 * has been hovered long enough; inside the scope you can emit any widgets.
 * Balance with eli_end_tooltip only when it returns true.
 */
static void cheat_render_begin_item_tooltip(void)
{
    eli_button("Rich tooltip");
    if (eli_begin_item_tooltip()) {
        eli_text("Tooltip Title");
        eli_text_disabled("Hover past the delay to see this.");
        eli_end_tooltip();
    }
}

#endif /* CHEAT_RENDER_MENUS_POPUPS_H */
