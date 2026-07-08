/**
 * @file eli_widgets.h
 * @brief Widgets-category aggregator (Phases 9-24). Including this header pulls
 *        in the whole widget surface: the shared interaction core
 *        (eli_button_behavior + render helpers), text/button/checkbox/radio/
 *        progress/link widgets, item-status queries, and every later-phase
 *        widget family — sliders, drags, inputs, color, combo/selectable/listbox,
 *        trees, menus, popups, tooltips, tables, tab bars, drag&drop, images,
 *        plots, and value display.
 *
 * These build directly on the layout system (item-size/item-add) and the id,
 * input, style, font, draw, and window categories. Drive them inside a window
 * scope opened by eli_begin and closed by eli_end:
 *     #include <eli/widgets/eli_widgets.h>
 *
 * @status Phases 9-24 widget aggregator in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WIDGETS_ELI_WIDGETS_H
#define ELI_WIDGETS_ELI_WIDGETS_H

#include "../layout/eli_layout.h"

/* Interaction core + basic widgets + item status (Phases 9-10). */
#include "eli_widget_behavior.h"
#include "eli_text_widgets.h"
#include "eli_button_widgets.h"
#include "eli_item_status.h"

/* Fan-out widget families (Phases 11-24). */
#include "eli_slider.h"             /* Phase 11: sliders          */
#include "eli_drag.h"               /* Phase 11: drags            */
#include "eli_input_text_widget.h"  /* Phase 12: text input       */
#include "eli_input_number.h"       /* Phase 12: numeric input    */
#include "eli_color.h"              /* Phase 13: color widgets    */
#include "eli_selectable.h"         /* Phase 14: selectable       */
#include "eli_listbox.h"            /* Phase 14: list box         */
#include "eli_combo.h"              /* Phase 14: combo            */
#include "eli_tree.h"               /* Phase 15: trees            */
#include "eli_menu.h"               /* Phase 16: menus            */
#include "eli_popup.h"              /* Phase 17: popups/modals    */
#include "eli_tooltip.h"            /* Phase 18: tooltips         */
#include "eli_table.h"              /* Phase 19: tables           */
#include "eli_tab.h"                /* Phase 20: tab bars         */
#include "eli_drag_drop.h"          /* Phase 21: drag & drop      */
#include "eli_image.h"              /* Phase 22: images           */
#include "eli_plot.h"               /* Phase 23: plots            */
#include "eli_value.h"              /* Phase 24: value display    */

#endif /* ELI_WIDGETS_ELI_WIDGETS_H */
