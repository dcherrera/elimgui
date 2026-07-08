/**
 * @file eli_item_status.h
 * @brief Phase 10 item-status queries: whether the last-submitted item is
 *        hovered / active / focused / clicked / visible / edited, the activation
 *        edges (activated / deactivated / deactivated-after-edit / toggled-open),
 *        the any-item aggregates, and the last-item id + rect accessors.
 *
 * These read the last-item record written by eli_item_add and the interaction
 * edges maintained by eli_button_behavior on the context. Call a query right
 * after submitting the widget it refers to (it always describes the most-recent
 * item). Mirrors Dear ImGui's IsItem / IsAnyItem contract.
 *
 * @status Phase 10 item-status queries in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WIDGETS_ELI_ITEM_STATUS_H
#define ELI_WIDGETS_ELI_ITEM_STATUS_H

#include "eli_widget_behavior.h"

#include "../core/eli_platform.h"
#include "../core/eli_context.h"

/* The last-item id + rect accessors (eli_get_item_id / eli_get_item_rect_min /
 * _max / _size) are provided by the layout item core and re-exported here via the
 * behavior include, so <eli/widgets/...> is the single entry point for all of them. */

/* ---------------------------------------------------------------------------
 * Per-item status
 * ------------------------------------------------------------------------- */

/**
 * @param flags  eli_hovered_flags controlling gating (blocked-by-active etc.).
 * @return       true if the most-recent item is hovered this frame.
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline bool eli_is_item_hovered(eli_hovered_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL)
        return false;
    if ((ctx->last_item_status_flags & ELI_ITEM_STATUS_HOVERED_RECT) == 0)
        return false;

    /* The mouse must be over this window (topmost), unless the caller opts out. */
    if ((flags & ELI_HOVERED_ALLOW_WHEN_OVERLAPPED_BY_WINDOW) == 0 &&
        !eli_widget__window_hovered(ctx, ctx->current_window))
        return false;

    /* A different active item blocks hover unless explicitly allowed. */
    if ((flags & ELI_HOVERED_ALLOW_WHEN_BLOCKED_BY_ACTIVE_ITEM) == 0 &&
        ctx->active_id != 0u && ctx->active_id != ctx->last_item_id)
        return false;

    /* Disabled items are not hovered unless the caller allows it. */
    if ((flags & ELI_HOVERED_ALLOW_WHEN_DISABLED) == 0 &&
        (ctx->current_item_flags & ELI_ITEM_DISABLED))
        return false;

    return true;
}

/** @return true if the most-recent item is the active item. */
static inline bool eli_is_item_active(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->active_id == 0u)
        return false;
    return ctx->active_id == ctx->last_item_id;
}

/** @return true if the most-recent item is the focused (nav) item. */
static inline bool eli_is_item_focused(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->nav_id == 0u)
        return false;
    return ctx->nav_id == ctx->last_item_id;
}

/**
 * @param mouse_button  Mouse button to test.
 * @return              true if the most-recent item is hovered and that button was
 *                      clicked this frame.
 */
static inline bool eli_is_item_clicked(eli_mouse_button mouse_button)
{
    return eli_is_item_hovered(ELI_HOVERED_NONE) && eli_is_mouse_clicked(mouse_button);
}

/** @return true if the most-recent item's rect overlapped the clip rect. */
static inline bool eli_is_item_visible(void)
{
    eli_context *ctx = eli_get_current_context();
    return ctx && (ctx->last_item_status_flags & ELI_ITEM_STATUS_VISIBLE);
}

/** @return true if the most-recent item's value changed this frame. */
static inline bool eli_is_item_edited(void)
{
    eli_context *ctx = eli_get_current_context();
    return ctx && (ctx->last_item_status_flags & ELI_ITEM_STATUS_EDITED);
}

/** @return true on the frame the most-recent item became active. */
static inline bool eli_is_item_activated(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->active_id == 0u)
        return false;
    return ctx->active_id == ctx->last_item_id &&
           ctx->active_id_previous_frame != ctx->last_item_id;
}

/** @return true on the frame the most-recent item stopped being active. */
static inline bool eli_is_item_deactivated(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return false;
    if (ctx->last_item_status_flags & ELI_ITEM_STATUS_HAS_DEACTIVATED)
        return (ctx->last_item_status_flags & ELI_ITEM_STATUS_DEACTIVATED) != 0;
    return ctx->active_id_previous_frame == ctx->last_item_id &&
           ctx->active_id_previous_frame != 0u && ctx->active_id != ctx->last_item_id;
}

/**
 * @return true on the frame the most-recent item stopped being active AND it was
 *         edited during that active spell.
 */
static inline bool eli_is_item_deactivated_after_edit(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return false;
    return eli_is_item_deactivated() &&
           (ctx->active_id_previous_frame_has_been_edited_before ||
            (ctx->active_id == 0u && ctx->active_id_has_been_edited_before));
}

/** @return true if the most-recent item (e.g. a tree node) was toggled open. */
static inline bool eli_is_item_toggled_open(void)
{
    eli_context *ctx = eli_get_current_context();
    return ctx && (ctx->last_item_status_flags & ELI_ITEM_STATUS_TOGGLED_OPEN);
}

/* ---------------------------------------------------------------------------
 * Any-item aggregates
 * ------------------------------------------------------------------------- */

/** @return true if any item is hovered this frame. */
static inline bool eli_is_any_item_hovered(void)
{
    eli_context *ctx = eli_get_current_context();
    return ctx && ctx->hot_id != 0u;
}

/** @return true if any item is active this frame. */
static inline bool eli_is_any_item_active(void)
{
    eli_context *ctx = eli_get_current_context();
    return ctx && ctx->active_id != 0u;
}

/** @return true if any item is focused (nav) this frame. */
static inline bool eli_is_any_item_focused(void)
{
    eli_context *ctx = eli_get_current_context();
    return ctx && ctx->nav_id != 0u;
}

#endif /* ELI_WIDGETS_ELI_ITEM_STATUS_H */
