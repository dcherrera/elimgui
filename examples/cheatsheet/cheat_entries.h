/**
 * @file cheat_entries.h
 * @brief Aggregated content registry for the elimgui visual cheatsheet.
 *
 * The cheatsheet's content is split by category into pairs of files under
 * entries/: a render_<cat>.h header (the live-widget renderer functions) and a
 * rows_<cat>.h fragment (comma-terminated cheat_entry initializers). This file
 * aggregates them into the single flat array the app renders: it #includes every
 * render_<cat>.h (so the renderer functions are declared), then builds
 * cheat_all_entries[] by #including every rows_<cat>.h fragment inside the array
 * initializer. cheat_all_entries_count is derived with sizeof.
 *
 * To add a category: create entries/render_<cat>.h + entries/rows_<cat>.h, then
 * add one line to each of the two include blocks below. Categories render in the
 * order listed here (first-seen order drives the left-pane category list).
 *
 * @status Cheatsheet content registry. Not library API.
 * @issues None
 * @todo None
 */
#ifndef CHEAT_ENTRIES_H
#define CHEAT_ENTRIES_H

#include <eli/elimgui.h>

#include "cheat_engine.h"

/* Live-widget renderer functions, one header per category. */
#include "entries/render_text_buttons.h"
#include "entries/render_sliders_input.h"
#include "entries/render_color.h"
#include "entries/render_combo_trees.h"
#include "entries/render_menus_popups.h"
#include "entries/render_tables_tabs.h"
#include "entries/render_layout.h"
#include "entries/render_dragdrop_media.h"
#include "entries/render_windows_dock.h"

/* The flat entry table, concatenated from each category's row fragment. Keep the
 * order in sync with the render includes above. */
static const cheat_entry cheat_all_entries[] = {
#include "entries/rows_text_buttons.h"
#include "entries/rows_sliders_input.h"
#include "entries/rows_color.h"
#include "entries/rows_combo_trees.h"
#include "entries/rows_menus_popups.h"
#include "entries/rows_tables_tabs.h"
#include "entries/rows_layout.h"
#include "entries/rows_dragdrop_media.h"
#include "entries/rows_windows_dock.h"
};

static const int cheat_all_entries_count =
    (int)(sizeof(cheat_all_entries) / sizeof(cheat_all_entries[0]));

#endif /* CHEAT_ENTRIES_H */
