# Docking (Phase 34)

Docking lets windows attach to one another inside a shared region: they stack as
tabs in a leaf **dock node**, or split a node into two resizable panes. Nodes form
a binary **split tree** rooted at a **dock space**. A window docks either
programmatically (`eli_set_next_window_dock_id`) or interactively by dragging its
title bar over a node and dropping on one of five drop zones (center = tab, the
four edges = split). Dragging a docked tab back out tears the window off as a
floating window again.

Header: `#include <eli/docking/eli_dock.h>` (pulls in the dock-node engine, the
window-docking integration, and dock spaces). The per-frame update
`eli_dock_new_frame()` is already wired into `eli_frame_begin`, so no manual
driving is needed when you use the frame-lifecycle composition.

## The tree model

- **Dock node** (`eli_dock_node`): a rectangle in screen space. A **leaf** holds
  an ordered list of docked window ids plus the selected/visible tab. An
  **internal** node has two children and a `split_axis` (`ELI_DOCK_AXIS_X` =
  left|right, `ELI_DOCK_AXIS_Y` = top|bottom) at a `split_ratio` (the fraction of
  the axis given to `child[0]`), separated by a draggable separator.
- **Dock space**: a root node flagged `ELI_DOCK_NODE_IS_DOCK_SPACE` so it is never
  garbage-collected when empty. Windows dock into it and its split descendants.
- Nodes are pooled in the context (each heap-allocated individually so pointers
  stay valid across pool growth) and freed on `eli_destroy_context`. Empty
  non-dock-space nodes are collapsed/pruned each frame by `eli_dock_new_frame`.

Rects propagate top-down from the root each frame: setting a dock space's rect
lays out the whole subtree. A docked window fills its leaf node's **body** (the
node rect minus the shared tab strip at the top); only the node's **selected** tab
emits its contents — the other docked windows are hidden that frame.

## Quick start

```c
#include <eli/elimgui.h>

/* Each frame, inside the frame scope: */
eli_frame_begin();

/* A host window that provides a dock space filling its content region. */
eli_set_next_window_size(eli_make_vec2(800.0f, 600.0f), ELI_COND_FIRST_USE_EVER);
if (eli_begin("Host", NULL, ELI_WINDOW_NO_DOCKING)) {
    eli_dock_space(eli_get_id("MyDockSpace"), eli_make_vec2(0.0f, 0.0f), 0);
}
eli_end();

/* Two windows docked into that space share a tab bar. Dock them on first use
   only, so the user can rearrange them afterwards. */
eli_id space = eli_get_id("MyDockSpace");
eli_set_next_window_dock_id(space, ELI_COND_FIRST_USE_EVER);
if (eli_begin("Tools", NULL, 0)) {
    eli_text("docked window A");
}
eli_end();

eli_set_next_window_dock_id(space, ELI_COND_FIRST_USE_EVER);
if (eli_begin("Log", NULL, 0)) {
    eli_text("docked window B");
}
eli_end();

eli_frame_end();
```

Or fill the whole viewport with a background dock space:

```c
eli_dock_space_over_viewport(eli_get_main_viewport(), 0);
```

## Public API

### Dock spaces

- `eli_id eli_dock_space(eli_id id, eli_vec2 size, int flags)` — build or update a
  dock space filling a region of the **current window** at the layout cursor. A
  `<= 0` size axis fills the available content region. Reserves the region in the
  host window's layout. Returns the dock space's node id (equal to `id`), or `0`
  on failure. Call inside a window scope.
- `eli_id eli_dock_space_over_viewport(const eli_viewport *vp, int flags)` — build
  or update a dock space filling the viewport's work area (NULL = main viewport),
  painted onto the background draw list so docked windows render on top. Returns
  its node id, or `0` on failure.

### Docking windows

- `void eli_set_next_window_dock_id(eli_id dock_id, eli_cond cond)` — request that
  the next `eli_begin` dock into node `dock_id` (`0` detaches / floats). `cond`
  gates the write (e.g. `ELI_COND_FIRST_USE_EVER` to set the initial dock only).
- `eli_id eli_get_window_dock_id(void)` — the node id the current window is
  attached to (`0` if floating).
- `bool eli_is_window_docked(void)` — `true` if the current window is docked this
  frame.
- `ELI_WINDOW_NO_DOCKING` — window flag; a window with it refuses to dock (both
  programmatically and via drag-and-drop) and is a safe choice for a dock-space
  host window.

### Interactive docking

While a floating, dockable window is dragged by its title bar over a node, the
five drop zones are drawn on the foreground draw list; releasing over a zone tabs
the window in (center) or splits the node along the chosen edge (left/right =
X split, up/down = Y split), each new pane taking half the axis. Dragging a docked
tab past a small threshold tears the window out as a floating window. The
separator between two split children is draggable to re-balance `split_ratio`.

### Per-frame hook

- `void eli_dock_new_frame(void)` — draws/commits an in-progress drag-to-dock,
  applies a pending drop (so the dropped window begins docked the same frame), and
  garbage-collects emptied nodes. Wired into `eli_frame_begin` after
  `eli_window_new_frame`; call it yourself only if you drive subsystem hooks
  manually.

### Engine access (advanced)

- `eli_dock_node *eli_dock_node_find(const eli_context *ctx, eli_id id)` — look up
  a node by id.
- `eli_dock_node *eli_dock_node_split(eli_context *ctx, eli_dock_node *node,
  eli_dock_dir dir)` — split a leaf node in two along the axis implied by `dir`,
  returning the empty child for an incoming window.
- `eli_rect eli_dock_node_body_rect(const eli_dock_node *node)` and
  `bool eli_dock_node_is_leaf(const eli_dock_node *node)` — geometry/shape queries.

## Gotchas

- Submit a window's dock space **before** the windows that dock into it in the
  same frame, so the node rect is current when the docked windows resolve their
  geometry.
- Use a condition (`ELI_COND_FIRST_USE_EVER`) with `eli_set_next_window_dock_id`
  so a user's later re-arrangement isn't overwritten every frame.
- Docking into an id with no existing node leaves the window floating; create the
  node first via a dock space (or a split).

## Deferred

- **Programmatic `DockBuilder_*` layout API** (splitting/docking a layout up front
  from code) is not implemented; build layouts by docking windows and letting the
  user split interactively, or with `eli_dock_node_split`.
- **Multi-viewport / OS-window docking** (dragging a window out to its own
  platform window) is out of scope — elimgui targets a single browser canvas.
- Per-node config flags (no-tab-bar, no-split, no-resize, auto-hide bar) beyond the
  minimal set are not yet exposed.
