/**
 * @file eli_dockspace.h
 * @brief Dock spaces (Phase 34): eli_dock_space builds/updates a root dock node
 *        filling a region inside the current window, and
 *        eli_dock_space_over_viewport fills the main viewport. Both walk the
 *        node tree each frame to paint empty-node backgrounds and the draggable
 *        separators between split children.
 *
 * A dock space is just a root node flagged ELI_DOCK_NODE_IS_DOCK_SPACE so it is
 * never garbage collected when empty; windows dock into it (and its split
 * descendants) via eli_set_next_window_dock_id or by being dragged over it.
 *
 * @status Phase 34 dock space in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_DOCKING_ELI_DOCKSPACE_H
#define ELI_DOCKING_ELI_DOCKSPACE_H

#include "eli_dock_types.h"
#include "eli_dock_node.h"
#include "eli_docking.h"

#include "../core/eli_platform.h"
#include "../layout/eli_layout.h"
#include "../util/eli_viewport.h"

/* Subtle fill painted over an empty leaf node so its region reads as a target. */
#define ELI_DOCK_EMPTY_BG_COL ELI_COL32(20, 20, 22, 160)

/* ---------------------------------------------------------------------------
 * Node-tree rendering + separator interaction
 * ------------------------------------------------------------------------- */

/** @return the separator rect between the two children of a split node. */
static inline eli_rect eli_dock__separator_rect(const eli_dock_node *node)
{
    float sep = ELI_DOCK_SEPARATOR_SIZE;
    const eli_dock_node *a = node->child[0];
    if (node->split_axis == ELI_DOCK_AXIS_X)
        return eli_make_rect(a->rect.x + a->rect.w, node->rect.y, sep, node->rect.h);
    return eli_make_rect(node->rect.x, a->rect.y + a->rect.h, node->rect.w, sep);
}

/** Drag the separator of a split node to adjust its split ratio. */
static inline void eli_dock__handle_separator(eli_context *ctx, eli_dock_node *node)
{
    eli_rect sr = eli_dock__separator_rect(node);
    eli_id sep_id = node->id ^ 0x53455031u; /* "SEP1" salt */
    const eli_io *io = &ctx->io;
    bool over = eli_is_mouse_hovering_rect(eli_rect_min(sr), eli_rect_max(sr), false);

    if (over && eli_is_mouse_clicked(ELI_MOUSE_BUTTON_LEFT))
        eli_set_active_id(sep_id);
    if (eli_get_active_id() != sep_id)
        return;

    if (!io->mouse_down[ELI_MOUSE_BUTTON_LEFT]) {
        eli_clear_active_id();
        return;
    }
    float avail, moved;
    if (node->split_axis == ELI_DOCK_AXIS_X) {
        avail = node->rect.w - ELI_DOCK_SEPARATOR_SIZE;
        moved = io->mouse_delta.x;
    } else {
        avail = node->rect.h - ELI_DOCK_SEPARATOR_SIZE;
        moved = io->mouse_delta.y;
    }
    if (avail > 0.0f) {
        node->split_ratio = eli_clamp_f(node->split_ratio + moved / avail, 0.05f, 0.95f);
        eli_dock_node_update_rects(ctx, node);
    }
}

/**
 * Recursively paint a subtree: fills empty leaf regions and, for split nodes,
 * draws + drives the draggable separator then descends into both children.
 *
 * @param ctx   Context (non-NULL).
 * @param dl    Draw list to paint decorations onto (non-NULL).
 * @param node  Subtree root (non-NULL).
 */
static inline void eli_dock__render_node(eli_context *ctx, eli_draw_list *dl, eli_dock_node *node)
{
    if (eli_dock_node_is_leaf(node)) {
        if (node->window_count == 0)
            eli_draw_list_add_rect_filled(dl, eli_rect_min(node->rect), eli_rect_max(node->rect),
                                          ELI_DOCK_EMPTY_BG_COL, 0.0f, ELI_DRAW_ROUND_CORNERS_NONE);
        return;
    }

    eli_rect sr = eli_dock__separator_rect(node);
    eli_col32 col = eli_get_color_u32(ELI_COL_SEPARATOR, 1.0f);
    eli_draw_list_add_rect_filled(dl, eli_rect_min(sr), eli_rect_max(sr), col, 0.0f,
                                  ELI_DRAW_ROUND_CORNERS_NONE);
    eli_dock__handle_separator(ctx, node);

    eli_dock__render_node(ctx, dl, node->child[0]);
    eli_dock__render_node(ctx, dl, node->child[1]);
}

/* ---------------------------------------------------------------------------
 * Public dock-space API
 * ------------------------------------------------------------------------- */

/**
 * Build or update a dock space: a root dock node filling a region of the current
 * window that windows can dock into. A zero size axis fills the available
 * content region. Reserves the region in the host window's layout.
 *
 * @param id     Stable id identifying this dock space (its root node id).
 * @param size   Requested size; a <= 0 axis fills the available content region.
 * @param flags  Dock node flags (reserved; pass 0).
 * @return       The dock space's node id (equal to `id`), or 0 on failure.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline eli_id eli_dock_space(eli_id id, eli_vec2 size, int flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || id == 0)
        return 0;
    eli_dock__install_hooks(ctx);

    eli_window *host = ctx->current_window;
    eli_vec2 pos = host ? host->cursor_pos : eli_make_vec2(0.0f, 0.0f);
    if (size.x <= 0.0f || size.y <= 0.0f) {
        eli_vec2 avail = eli_get_content_region_avail();
        if (size.x <= 0.0f) size.x = avail.x;
        if (size.y <= 0.0f) size.y = avail.y;
    }

    eli_dock_node *node = eli_dock_node_get_or_create(ctx, id, (eli_dock_node_flags)flags);
    if (node == NULL)
        return 0;
    node->flags |= ELI_DOCK_NODE_IS_DOCK_SPACE | ELI_DOCK_NODE_KEEP_ALIVE;
    node->rect = eli_make_rect(pos.x, pos.y, size.x, size.y);
    node->tab_bar_height = eli_dock_node_tab_bar_height(ctx);
    node->last_frame_active = ctx->frame_count;
    eli_dock_node_update_rects(ctx, node);

    if (host != NULL) {
        eli_dock__render_node(ctx, &host->draw_list, node);
        eli_dummy(size);
    }
    return id;
}

/**
 * Build or update a dock space that fills the main viewport's work area. Painted
 * onto the background draw list so docked windows render on top of it.
 *
 * @param vp     Viewport to fill; NULL uses the main viewport.
 * @param flags  Dock node flags (reserved; pass 0).
 * @return       The viewport dock space's node id, or 0 on failure.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline eli_id eli_dock_space_over_viewport(const eli_viewport *vp, int flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return 0;
    if (vp == NULL)
        vp = eli_get_main_viewport();
    if (vp == NULL)
        return 0;
    eli_dock__install_hooks(ctx);

    eli_id id = vp->id ^ 0x44535056u; /* "DSPV" salt: distinct from viewport id */
    eli_dock_node *node = eli_dock_node_get_or_create(ctx, id, (eli_dock_node_flags)flags);
    if (node == NULL)
        return 0;
    node->flags |= ELI_DOCK_NODE_IS_DOCK_SPACE | ELI_DOCK_NODE_KEEP_ALIVE;
    node->rect = eli_make_rect(vp->work_pos.x, vp->work_pos.y, vp->work_size.x, vp->work_size.y);
    node->tab_bar_height = eli_dock_node_tab_bar_height(ctx);
    node->last_frame_active = ctx->frame_count;
    eli_dock_node_update_rects(ctx, node);

    eli_draw_list *bg = eli_get_background_draw_list();
    if (bg != NULL)
        eli_dock__render_node(ctx, bg, node);
    return id;
}

#endif /* ELI_DOCKING_ELI_DOCKSPACE_H */
