/**
 * @file eli_dock.h
 * @brief Aggregator for the elimgui docking category (Phase 34): dock-node types
 *        and engine, window-docking integration, and dock spaces.
 *
 * Include this to pull in the whole docking surface:
 *     #include <eli/docking/eli_dock.h>
 *
 * The docking begin hooks are installed on the context on first use of any dock
 * API (eli_dock_space, eli_dock_space_over_viewport, eli_set_next_window_dock_id),
 * so windows behave exactly as before until docking is actually used. Drive the
 * per-frame update with eli_dock_new_frame (already wired into eli_frame_begin).
 *
 * @status Phase 34 docking aggregator in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_DOCKING_ELI_DOCK_H
#define ELI_DOCKING_ELI_DOCK_H

#include "eli_dock_types.h"
#include "eli_dock_node.h"
#include "eli_docking.h"
#include "eli_dockspace.h"

#endif /* ELI_DOCKING_ELI_DOCK_H */
