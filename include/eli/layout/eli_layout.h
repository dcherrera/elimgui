/**
 * @file eli_layout.h
 * @brief Layout-system aggregator (Phase 8). Including this header pulls in the
 *        full layout API: the item-layout core (eli_item_size / eli_item_add and
 *        content-region queries), the cursor get/set API, the flow helpers
 *        (separator, same-line, spacing, dummy, indent, groups), the item-width /
 *        text-wrap stacks, and the vertical sizing helpers.
 *
 * These primitives underpin every widget: a widget measures itself, calls
 * eli_item_size to advance the window's layout cursor, then eli_item_add to
 * register its id/rect/status. Drive them inside a window scope opened by
 * eli_begin and closed by eli_end.
 *
 * @status Phase 8 layout system in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_LAYOUT_ELI_LAYOUT_H
#define ELI_LAYOUT_ELI_LAYOUT_H

#include "../window/eli_window.h"

#include "eli_layout_item.h"
#include "eli_layout_cursor.h"
#include "eli_layout_helpers.h"
#include "eli_layout_stack.h"
#include "eli_layout_sizing.h"

#endif /* ELI_LAYOUT_ELI_LAYOUT_H */
