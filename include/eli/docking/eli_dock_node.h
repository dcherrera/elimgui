/**
 * @file eli_dock_node.h
 * @brief Dock-node engine (Phase 34): the context-owned node pool (find / create
 *        / free plus the destroy-time shutdown hook), the leaf window-list
 *        operations, top-down rect propagation over the split tree, leaf
 *        splitting, and the collapse/garbage-collection pass that prunes emptied
 *        nodes.
 *
 * These are the low-level pieces the window-docking integration and the dock
 * space build on. They depend on the context and window types but not on the
 * higher-level docking headers, so they can be included without cycles.
 *
 * @status Phase 34 dock-node engine in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_DOCKING_ELI_DOCK_NODE_H
#define ELI_DOCKING_ELI_DOCK_NODE_H

#include "eli_dock_types.h"

#include "../core/eli_platform.h"
#include "../core/eli_context.h"
#include "../id/eli_id.h"
#include "../window/eli_window_types.h"

/* ---------------------------------------------------------------------------
 * Small helpers
 * ------------------------------------------------------------------------- */

/** @return true when the node holds windows directly (no split children). */
static inline bool eli_dock_node_is_leaf(const eli_dock_node *node)
{
    return node->child[0] == NULL && node->child[1] == NULL;
}

/** @return the effective font size used to size dock decorations (never 0). */
static inline float eli_dock__effective_font_size(const eli_context *ctx)
{
    float fs = ctx->font_size;
    return (fs > 0.0f) ? fs : ELI_DOCK_DEFAULT_FONT_SIZE;
}

/** @return the height of a leaf node's tab strip for the current context. */
static inline float eli_dock_node_tab_bar_height(const eli_context *ctx)
{
    return eli_dock__effective_font_size(ctx) + ELI_DOCK_TAB_BAR_PADDING_Y * 2.0f;
}

/** @return the body region of a leaf node (its rect minus the top tab strip). */
static inline eli_rect eli_dock_node_body_rect(const eli_dock_node *node)
{
    float tb = node->tab_bar_height;
    float h = node->rect.h - tb;
    if (h < 0.0f)
        h = 0.0f;
    return eli_make_rect(node->rect.x, node->rect.y + tb, node->rect.w, h);
}

/* ---------------------------------------------------------------------------
 * Pool (find / create / free) + shutdown hook
 * ------------------------------------------------------------------------- */

/**
 * Release the whole dock-node pool owned by a context: every node's window-id
 * array and the node itself, then the pool array. Registered as
 * ctx->dock_shutdown_fn so eli_destroy_context can call it.
 *
 * @param ctx  Context whose dock state is freed (non-NULL).
 */
static inline void eli_dock_shutdown_context(eli_context *ctx)
{
    for (int i = 0; i < ctx->dock_nodes_count; i++) {
        eli_dock_node *node = ctx->dock_nodes[i];
        if (node == NULL)
            continue;
        free(node->window_ids);
        free(node);
    }
    free(ctx->dock_nodes);
    ctx->dock_nodes = NULL;
    ctx->dock_nodes_count = 0;
    ctx->dock_nodes_capacity = 0;
}

/**
 * Find a dock node by id.
 *
 * @param ctx  Context (non-NULL).
 * @param id   Node id to find (0 never matches).
 * @return     The node, or NULL if none matches or id is 0.
 */
static inline eli_dock_node *eli_dock_node_find(const eli_context *ctx, eli_id id)
{
    if (id == 0)
        return NULL;
    for (int i = 0; i < ctx->dock_nodes_count; i++)
        if (ctx->dock_nodes[i]->id == id)
            return ctx->dock_nodes[i];
    return NULL;
}

/**
 * Get an existing node by id or create and register a fresh (zeroed) leaf node.
 * Installs the dock shutdown hook on first creation.
 *
 * @param ctx    Context (non-NULL).
 * @param id     Node id (non-zero).
 * @param flags  Flags for a newly created node (ignored if it already exists).
 * @return       The node, or NULL on allocation failure.
 */
static inline eli_dock_node *eli_dock_node_get_or_create(eli_context *ctx, eli_id id,
                                                         eli_dock_node_flags flags)
{
    eli_dock_node *node = eli_dock_node_find(ctx, id);
    if (node != NULL) {
        node->flags |= flags;
        return node;
    }

    node = (eli_dock_node *)calloc(1, sizeof(*node));
    if (node == NULL)
        return NULL;
    node->id = id;
    node->flags = flags;
    node->split_axis = ELI_DOCK_AXIS_NONE;
    node->split_ratio = 0.5f;
    node->selected_window_id = 0;
    node->last_frame_active = -1;

    if (ctx->dock_shutdown_fn == NULL)
        ctx->dock_shutdown_fn = eli_dock_shutdown_context;

    if (ctx->dock_nodes_count >= ctx->dock_nodes_capacity) {
        int cap = ctx->dock_nodes_capacity ? ctx->dock_nodes_capacity * 2 : 8;
        eli_dock_node **grown =
            (eli_dock_node **)realloc(ctx->dock_nodes, (size_t)cap * sizeof(*grown));
        if (grown == NULL) {
            free(node);
            return NULL;
        }
        ctx->dock_nodes = grown;
        ctx->dock_nodes_capacity = cap;
    }
    ctx->dock_nodes[ctx->dock_nodes_count++] = node;
    return node;
}

/** Remove a node from the pool (swap-remove) and free it. */
static inline void eli_dock__free_node(eli_context *ctx, eli_dock_node *node)
{
    for (int i = 0; i < ctx->dock_nodes_count; i++) {
        if (ctx->dock_nodes[i] != node)
            continue;
        ctx->dock_nodes[i] = ctx->dock_nodes[ctx->dock_nodes_count - 1];
        ctx->dock_nodes_count--;
        break;
    }
    free(node->window_ids);
    free(node);
}

/* ---------------------------------------------------------------------------
 * Leaf window list
 * ------------------------------------------------------------------------- */

/** @return true if window id `wid` is already in the node's leaf list. */
static inline bool eli_dock_node_has_window(const eli_dock_node *node, eli_id wid)
{
    for (int i = 0; i < node->window_count; i++)
        if (node->window_ids[i] == wid)
            return true;
    return false;
}

/** Append a window id to a leaf node (no-op if already present). */
static inline void eli_dock_node_add_window(eli_dock_node *node, eli_id wid)
{
    if (wid == 0 || eli_dock_node_has_window(node, wid))
        return;
    if (node->window_count >= node->window_capacity) {
        int cap = node->window_capacity ? node->window_capacity * 2 : 4;
        eli_id *grown = (eli_id *)realloc(node->window_ids, (size_t)cap * sizeof(*grown));
        if (grown == NULL)
            return;
        node->window_ids = grown;
        node->window_capacity = cap;
    }
    node->window_ids[node->window_count++] = wid;
    if (node->selected_window_id == 0)
        node->selected_window_id = wid;
}

/** Remove a window id from a leaf node, keeping the remaining order stable. */
static inline void eli_dock_node_remove_window(eli_dock_node *node, eli_id wid)
{
    int idx = -1;
    for (int i = 0; i < node->window_count; i++) {
        if (node->window_ids[i] == wid) {
            idx = i;
            break;
        }
    }
    if (idx < 0)
        return;
    for (int i = idx; i < node->window_count - 1; i++)
        node->window_ids[i] = node->window_ids[i + 1];
    node->window_count--;
    if (node->selected_window_id == wid)
        node->selected_window_id = (node->window_count > 0) ? node->window_ids[0] : 0;
}

/* ---------------------------------------------------------------------------
 * Rect propagation
 * ------------------------------------------------------------------------- */

/**
 * Recompute a subtree's rects top-down from `node->rect`. For a split node the
 * two children share the axis at split_ratio, separated by the separator size;
 * for a leaf the tab-strip height is refreshed. Call after setting the root rect.
 *
 * @param ctx   Context (non-NULL; supplies the tab-strip height).
 * @param node  Subtree root whose rect is already set (non-NULL).
 */
static inline void eli_dock_node_update_rects(eli_context *ctx, eli_dock_node *node)
{
    if (eli_dock_node_is_leaf(node)) {
        node->tab_bar_height = eli_dock_node_tab_bar_height(ctx);
        return;
    }

    float sep = ELI_DOCK_SEPARATOR_SIZE;
    eli_dock_node *a = node->child[0];
    eli_dock_node *b = node->child[1];
    if (node->split_axis == ELI_DOCK_AXIS_X) {
        float avail = node->rect.w - sep;
        if (avail < 0.0f) avail = 0.0f;
        float w0 = (float)(long)(avail * node->split_ratio);
        a->rect = eli_make_rect(node->rect.x, node->rect.y, w0, node->rect.h);
        b->rect = eli_make_rect(node->rect.x + w0 + sep, node->rect.y, avail - w0, node->rect.h);
    } else {
        float avail = node->rect.h - sep;
        if (avail < 0.0f) avail = 0.0f;
        float h0 = (float)(long)(avail * node->split_ratio);
        a->rect = eli_make_rect(node->rect.x, node->rect.y, node->rect.w, h0);
        b->rect = eli_make_rect(node->rect.x, node->rect.y + h0 + sep, node->rect.w, avail - h0);
    }
    eli_dock_node_update_rects(ctx, a);
    eli_dock_node_update_rects(ctx, b);
}

/* ---------------------------------------------------------------------------
 * Splitting
 * ------------------------------------------------------------------------- */

/** @return the id derived for a split child of `parent_id` at slot `index`. */
static inline eli_id eli_dock__child_id(eli_id parent_id, int index)
{
    int salt = index + 1;
    return eli_hash_data(&salt, sizeof(salt), parent_id);
}

/**
 * Split a leaf node in two along the axis implied by `dir`, moving its existing
 * windows into one child and returning the (empty) other child for the incoming
 * window. The parent keeps its id so any dock space bound to it is preserved.
 *
 * @param ctx  Context (non-NULL).
 * @param node Leaf node to split (non-NULL; must be a leaf).
 * @param dir  Drop direction (LEFT/RIGHT => X, UP/DOWN => Y). CENTER/NONE fail.
 * @return     The new empty child leaf for the incoming window, or NULL on
 *             failure (allocation, non-leaf node, or a center/none direction).
 */
static inline eli_dock_node *eli_dock_node_split(eli_context *ctx, eli_dock_node *node,
                                                 eli_dock_dir dir)
{
    if (!eli_dock_node_is_leaf(node) || dir == ELI_DOCK_DIR_CENTER || dir == ELI_DOCK_DIR_NONE)
        return NULL;

    eli_dock_axis axis =
        (dir == ELI_DOCK_DIR_LEFT || dir == ELI_DOCK_DIR_RIGHT) ? ELI_DOCK_AXIS_X : ELI_DOCK_AXIS_Y;
    bool new_first = (dir == ELI_DOCK_DIR_LEFT || dir == ELI_DOCK_DIR_UP);

    eli_dock_node *keep = eli_dock_node_get_or_create(ctx, eli_dock__child_id(node->id, 0), 0);
    eli_dock_node *fresh = eli_dock_node_get_or_create(ctx, eli_dock__child_id(node->id, 1), 0);
    if (keep == NULL || fresh == NULL)
        return NULL;

    /* Transfer the parent's window list to the "keep" child. */
    keep->window_ids = node->window_ids;
    keep->window_count = node->window_count;
    keep->window_capacity = node->window_capacity;
    keep->selected_window_id = node->selected_window_id;
    keep->last_frame_active = node->last_frame_active;
    node->window_ids = NULL;
    node->window_count = 0;
    node->window_capacity = 0;
    node->selected_window_id = 0;

    keep->parent = node;
    fresh->parent = node;
    node->split_axis = axis;
    node->split_ratio = 0.5f;
    node->child[0] = new_first ? fresh : keep;
    node->child[1] = new_first ? keep : fresh;

    eli_dock_node_update_rects(ctx, node);
    return fresh;
}

/* ---------------------------------------------------------------------------
 * Collapse / garbage collection
 * ------------------------------------------------------------------------- */

/** Repoint every window whose dock_id is `from` to `to` (used on collapse). */
static inline void eli_dock__redock_windows(eli_context *ctx, eli_id from, eli_id to)
{
    for (int i = 0; i < ctx->windows_count; i++) {
        eli_window *win = ctx->windows[i];
        if (win != NULL && win->dock_id == from)
            win->dock_id = to;
    }
}

/** Absorb `other` into `node`, adopting its split/children/windows, then free it. */
static inline void eli_dock__adopt(eli_context *ctx, eli_dock_node *node, eli_dock_node *other)
{
    free(node->window_ids);
    node->window_ids = other->window_ids;
    node->window_count = other->window_count;
    node->window_capacity = other->window_capacity;
    node->selected_window_id = other->selected_window_id;
    node->split_axis = other->split_axis;
    node->split_ratio = other->split_ratio;
    node->child[0] = other->child[0];
    node->child[1] = other->child[1];
    if (node->child[0]) node->child[0]->parent = node;
    if (node->child[1]) node->child[1]->parent = node;

    /* If `other` was a leaf, windows referenced it by id; repoint them to node. */
    if (other->child[0] == NULL && other->child[1] == NULL)
        eli_dock__redock_windows(ctx, other->id, node->id);

    other->window_ids = NULL;
    for (int i = 0; i < ctx->dock_nodes_count; i++) {
        if (ctx->dock_nodes[i] != other)
            continue;
        ctx->dock_nodes[i] = ctx->dock_nodes[ctx->dock_nodes_count - 1];
        ctx->dock_nodes_count--;
        break;
    }
    free(other);
}

/** @return true if a child leaf is empty and eligible to be pruned. */
static inline bool eli_dock__child_is_prunable(const eli_dock_node *child)
{
    return child->child[0] == NULL && child->child[1] == NULL && child->window_count == 0 &&
           (child->flags & ELI_DOCK_NODE_IS_DOCK_SPACE) == 0;
}

/**
 * Prune emptied dock nodes: collapse any split whose one child is an empty leaf
 * (the surviving child is absorbed into the parent) and free orphan empty
 * non-dock-space leaves. Run once per frame from eli_dock_new_frame.
 *
 * @param ctx  Context (non-NULL).
 */
static inline void eli_dock_node_gc(eli_context *ctx)
{
    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 0; i < ctx->dock_nodes_count; i++) {
            eli_dock_node *node = ctx->dock_nodes[i];
            if (eli_dock_node_is_leaf(node))
                continue;
            for (int c = 0; c < 2; c++) {
                eli_dock_node *child = node->child[c];
                eli_dock_node *other = node->child[1 - c];
                if (child == NULL || other == NULL || !eli_dock__child_is_prunable(child))
                    continue;
                free(child->window_ids);
                for (int k = 0; k < ctx->dock_nodes_count; k++) {
                    if (ctx->dock_nodes[k] != child)
                        continue;
                    ctx->dock_nodes[k] = ctx->dock_nodes[ctx->dock_nodes_count - 1];
                    ctx->dock_nodes_count--;
                    break;
                }
                free(child);
                node->child[0] = node->child[1] = NULL;
                eli_dock__adopt(ctx, node, other);
                changed = true;
                break;
            }
            if (changed)
                break;
        }
    }

    /* Free orphan empty non-dock-space leaves that lost their last window. */
    for (int i = ctx->dock_nodes_count - 1; i >= 0; i--) {
        eli_dock_node *node = ctx->dock_nodes[i];
        if (node->parent == NULL && eli_dock_node_is_leaf(node) && node->window_count == 0 &&
            (node->flags & ELI_DOCK_NODE_IS_DOCK_SPACE) == 0)
            eli_dock__free_node(ctx, node);
    }
}

#endif /* ELI_DOCKING_ELI_DOCK_NODE_H */
