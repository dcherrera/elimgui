/**
 * @file eli_id_active.h
 * @brief Active-item and hot-item tracking for elimgui.
 *
 * The active id is the item currently being interacted with (e.g. a held
 * button/slider); the hot id is the item under the mouse this frame. These
 * internal helpers keep ctx->active_id / ctx->hot_id in sync and maintain the
 * previous-frame snapshots plus the "just activated" edge flag. Widgets set
 * these; the frame-advance helper rolls the previous-frame fields forward.
 *
 * @status Phase 5 active/hot ID tracking in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_ID_ELI_ID_ACTIVE_H
#define ELI_ID_ELI_ID_ACTIVE_H

#include "../core/eli_platform.h"
#include "../core/eli_types.h"
#include "../core/eli_context.h"

/** The reserved "no item" id (matches Dear ImGui's use of 0). */
#define ELI_ID_NONE 0u

/**
 * Make an item active. Sets the just-activated edge flag when the active id
 * actually changes, so a widget can detect the frame it takes focus.
 *
 * @param id  Item id to activate (ELI_ID_NONE to clear).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_set_active_id(eli_id id)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    ctx->active_id_is_just_activated = (ctx->active_id != id);
    ctx->active_id = id;
}

/**
 * Clear the active item (equivalent to eli_set_active_id(ELI_ID_NONE)).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_clear_active_id(void)
{
    eli_set_active_id(ELI_ID_NONE);
}

/**
 * Mark an item as hot (hovered) for this frame.
 *
 * @param id  Item id under the cursor (ELI_ID_NONE for none).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_set_hot_id(eli_id id)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    ctx->hot_id = id;
}

/**
 * @return  The current active id, or ELI_ID_NONE if none/no context.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline eli_id eli_get_active_id(void)
{
    const eli_context *ctx = eli_get_current_context();
    return ctx ? ctx->active_id : ELI_ID_NONE;
}

/**
 * @return  The current hot id, or ELI_ID_NONE if none/no context.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline eli_id eli_get_hot_id(void)
{
    const eli_context *ctx = eli_get_current_context();
    return ctx ? ctx->hot_id : ELI_ID_NONE;
}

/**
 * @param id  Id to test.
 * @return    true if id is the current active id (and non-zero).
 */
static inline bool eli_is_active_id(eli_id id)
{
    return id != ELI_ID_NONE && eli_get_active_id() == id;
}

/**
 * Roll the active/hot ids into their previous-frame snapshots and reset the
 * just-activated edge flag. Intended to be called once per frame by the
 * frame-advance path so widgets can compare this frame against the last.
 *
 * @param ctx  Context to advance (no-op if NULL).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_id_new_frame(eli_context *ctx)
{
    if (ctx == NULL)
        return;
    ctx->active_id_previous_frame = ctx->active_id;
    ctx->hot_id_previous_frame = ctx->hot_id;
    ctx->active_id_is_just_activated = false;
}

#endif /* ELI_ID_ELI_ID_ACTIVE_H */
