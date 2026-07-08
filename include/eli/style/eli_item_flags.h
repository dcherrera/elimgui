/**
 * @file eli_item_flags.h
 * @brief Push/pop stack for per-item behavior flags (eli_item_flags), driving
 *        the context's current_item_flags. Widgets read current_item_flags to
 *        honor states like ELI_ITEM_DISABLED or ELI_ITEM_BUTTON_REPEAT.
 *
 * @status Phase 6 item-flag stack in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_STYLE_ELI_ITEM_FLAGS_H
#define ELI_STYLE_ELI_ITEM_FLAGS_H

#include "../core/eli_platform.h"
#include "../core/eli_enums.h"
#include "../core/eli_context.h"

/**
 * Enable or disable one or more item flags for subsequent widgets, saving the
 * previous flag state so eli_pop_item_flag can restore it.
 *
 * @param option   Item flag bits to modify (see eli_item_flags).
 * @param enabled  true to set the bits, false to clear them.
 *
 * Thread-safe: no (mutates the current context)
 * Reentrant: no
 */
static inline void eli_push_item_flag(int option, bool enabled)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->item_flags_stack_size >= ELI_ITEM_FLAG_STACK_MAX)
        return;

    ctx->item_flags_stack[ctx->item_flags_stack_size++] = ctx->current_item_flags;
    if (enabled)
        ctx->current_item_flags |= option;
    else
        ctx->current_item_flags &= ~option;
}

/**
 * Restore the item flags saved by the most recent eli_push_item_flag. A no-op
 * if the stack is empty.
 *
 * Thread-safe: no (mutates the current context)
 * Reentrant: no
 */
static inline void eli_pop_item_flag(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->item_flags_stack_size <= 0)
        return;

    ctx->current_item_flags = ctx->item_flags_stack[--ctx->item_flags_stack_size];
}

#endif /* ELI_STYLE_ELI_ITEM_FLAGS_H */
