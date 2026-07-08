/**
 * @file eli_focus.h
 * @brief Focus requests: eli_set_item_default_focus and eli_set_keyboard_focus_here.
 *        Records a focus target the next (or just-submitted) item consumes, driving
 *        the navigation focus id (ctx->nav_id).
 *
 * Minimal, nav-lite implementation. eli_set_item_default_focus lands nav_id on the
 * just-submitted item when its window is appearing. eli_set_keyboard_focus_here
 * stores a file-static request keyed by how many upcoming items to skip; a negative
 * offset targets an already-submitted item immediately, a zero/positive offset is
 * consumed by a future item via eli_focus_consume_keyboard_request. Full keyboard
 * navigation (tabbing traversal) is a later phase; see docs.
 *
 * @status Phase 25 focus requests: default-focus + keyboard-focus request wired.
 * @issues None
 * @todo None
 */
#ifndef ELI_INTERACTION_ELI_FOCUS_H
#define ELI_INTERACTION_ELI_FOCUS_H

#include "../core/eli_platform.h"
#include "../core/eli_context.h"
#include "../window/eli_window.h"

/* Pending keyboard-focus request. Lives file-static rather than on the context so
 * no core struct field is added; the next matching item consumes it. */
static bool eli_focus__request_active = false;
static int eli_focus__request_countdown = 0;

/**
 * Make the just-submitted item the default navigation focus for its window, but
 * only on the frame the window is appearing (first shown). Later frames are left
 * untouched so user navigation is not overridden. Call immediately after the item.
 *
 * Thread-safe: no (mutates the current context)
 * Reentrant: no
 */
static inline void eli_set_item_default_focus(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL)
        return;
    if (!ctx->current_window->appearing)
        return;
    ctx->nav_id = ctx->last_item_id;
}

/**
 * Request keyboard focus relative to the current position in submission order.
 *   offset == 0  focuses the next item to be submitted.
 *   offset  > 0  focuses the item submitted @p offset items after this call.
 *   offset  < 0  focuses an already-submitted item; -1 is the last item, applied
 *                immediately to nav_id.
 * A zero/positive request is stored file-static and consumed by a future item
 * through eli_focus_consume_keyboard_request. Only one request is tracked at a time.
 *
 * @param offset  Item offset relative to this call (see above).
 *
 * Thread-safe: no (mutates the current context / file-static request)
 * Reentrant: no
 */
static inline void eli_set_keyboard_focus_here(int offset)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;

    if (offset < 0) {
        /* Target an already-submitted item. Minimal: -1 == the last item. */
        if (offset == -1 && ctx->last_item_id != 0u)
            ctx->nav_id = ctx->last_item_id;
        eli_focus__request_active = false;
        return;
    }

    eli_focus__request_active = true;
    eli_focus__request_countdown = offset;
}

/** @return true if a zero/positive keyboard-focus request is pending. */
static inline bool eli_focus_has_keyboard_request(void)
{
    return eli_focus__request_active;
}

/**
 * Offer a newly-added item to the pending keyboard-focus request. Skips items
 * until the requested offset is reached, then lands nav_id on that item's id and
 * clears the request. A widget's eli_item_add path is expected to call this once
 * per added item.
 *
 * @param id  Id of the item just added (0 is ignored).
 * @return    true if this item consumed the request (nav_id was set to @p id).
 *
 * Thread-safe: no (mutates the current context / file-static request)
 * Reentrant: no
 */
static inline bool eli_focus_consume_keyboard_request(eli_id id)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || !eli_focus__request_active || id == 0u)
        return false;

    if (eli_focus__request_countdown > 0) {
        eli_focus__request_countdown--;
        return false;
    }

    eli_focus__request_active = false;
    ctx->nav_id = id;
    return true;
}

#endif /* ELI_INTERACTION_ELI_FOCUS_H */
