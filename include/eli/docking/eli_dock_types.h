/**
 * @file eli_dock_types.h
 * @brief Docking data types (Phase 34): the split-axis / drop-zone enums, dock
 *        node flags, layout constants, and the eli_dock_node structure that forms
 *        the binary split tree windows dock into.
 *
 * These are pure data definitions with no behavior, so the context can forward
 * declare eli_dock_node and store a pool of them without depending on the dock
 * engine headers.
 *
 * @status Phase 34 docking types in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_DOCKING_ELI_DOCK_TYPES_H
#define ELI_DOCKING_ELI_DOCK_TYPES_H

#include "../core/eli_types.h"

/* ---------------------------------------------------------------------------
 * Constants
 * ------------------------------------------------------------------------- */

/* Fallback font size used to size the dock tab strip when no font is bound yet,
 * so a node's tab bar has a deterministic height in headless (test) contexts. */
#define ELI_DOCK_DEFAULT_FONT_SIZE 13.0f

/* Vertical padding added above and below the tab-strip label to get its height. */
#define ELI_DOCK_TAB_BAR_PADDING_Y 4.0f

/* Horizontal padding on each side of a tab label inside the tab strip. */
#define ELI_DOCK_TAB_PADDING_X 8.0f

/* Approximate glyph advance as a fraction of the font size, used to measure tab
 * widths deterministically without requiring a built font atlas. */
#define ELI_DOCK_APPROX_CHAR_W_MULT 0.5f

/* Thickness (px) of the draggable separator drawn between split children. */
#define ELI_DOCK_SEPARATOR_SIZE 2.0f

/* Side length (px) of each drop-zone hit square drawn while dragging to dock.
 * ~36px visual per docking.md's dock-guide metric. */
#define ELI_DOCK_PREVIEW_ZONE_SIZE 36.0f

/* Distance (px) from a node's center to the center of an edge drop zone. */
#define ELI_DOCK_PREVIEW_ZONE_OFFSET 46.0f

/* Mouse travel (px) past which pressing a docked tab tears it out (undocks).
 * docking.md tear-out threshold. */
#define ELI_DOCK_UNDOCK_THRESHOLD 12.0f

/* Minimum on-screen size (px) of a dock pane (docking.md's ~96px min pane). A
 * node is only offered as a SPLIT target when both resulting panes would clear
 * this, so tiny nodes can only be tabbed into. */
#define ELI_DOCK_MIN_PANE 96.0f

/* Width of the outer edge split band as a fraction of the node's side. The
 * remaining (dominant) central area is the "tab here" region, so aiming for a
 * tab does not accidentally split. Capped by ELI_DOCK_EDGE_BAND_MAX so large
 * nodes keep a big center. */
#define ELI_DOCK_EDGE_BAND_FRAC 0.25f
#define ELI_DOCK_EDGE_BAND_MAX  64.0f

/* ---------------------------------------------------------------------------
 * Enums
 * ------------------------------------------------------------------------- */

/** Axis a dock node is split along. NONE marks a leaf that holds windows. */
typedef int eli_dock_axis;
enum eli_dock_axis_ {
    ELI_DOCK_AXIS_NONE = -1,
    ELI_DOCK_AXIS_X    = 0,   /* split left | right */
    ELI_DOCK_AXIS_Y    = 1    /* split top  | bottom */
};

/** Drop zone selected while dragging a window over a dock target. */
typedef int eli_dock_dir;
enum eli_dock_dir_ {
    ELI_DOCK_DIR_NONE   = -1,
    ELI_DOCK_DIR_CENTER = 0,  /* add as a tab to the target node */
    ELI_DOCK_DIR_LEFT   = 1,  /* split X, new window on the left */
    ELI_DOCK_DIR_RIGHT  = 2,  /* split X, new window on the right */
    ELI_DOCK_DIR_UP     = 3,  /* split Y, new window on the top */
    ELI_DOCK_DIR_DOWN   = 4   /* split Y, new window on the bottom */
};

/** Flags describing a dock node's role (see enum eli_dock_node_flags_). */
typedef int eli_dock_node_flags;
enum eli_dock_node_flags_ {
    ELI_DOCK_NODE_NONE          = 0,
    ELI_DOCK_NODE_IS_DOCK_SPACE = 1 << 0,  /* root created by eli_dock_space */
    ELI_DOCK_NODE_KEEP_ALIVE    = 1 << 1   /* persist even when it holds no windows */
};

/* ---------------------------------------------------------------------------
 * Dock node
 * ------------------------------------------------------------------------- */

/**
 * One node of a dock split tree. A leaf (split_axis == ELI_DOCK_AXIS_NONE) holds
 * an ordered list of docked window ids and a selected/visible tab; an internal
 * node has two children and is split along split_axis at split_ratio (the
 * fraction of the axis given to child[0]). Nodes are heap allocated individually
 * and pooled in the context, so parent/child pointers stay valid across pool
 * growth. `rect` is recomputed top-down each frame from the root's rect.
 */
typedef struct eli_dock_node {
    eli_id                id;
    eli_dock_node_flags   flags;

    struct eli_dock_node *parent;
    struct eli_dock_node *child[2];    /* both NULL for a leaf */
    eli_dock_axis         split_axis;
    float                 split_ratio; /* fraction of the axis for child[0] */

    eli_rect              rect;        /* full node rect in screen space */
    float                 tab_bar_height;

    /* Leaf window list (persisted across frames). */
    eli_id               *window_ids;
    int                   window_count;
    int                   window_capacity;
    eli_id                selected_window_id;  /* the visible tab */

    int                   last_frame_active;   /* frame this node was touched */
} eli_dock_node;

#endif /* ELI_DOCKING_ELI_DOCK_TYPES_H */
