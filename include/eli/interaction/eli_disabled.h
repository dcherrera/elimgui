/**
 * @file eli_disabled.h
 * @brief Widget-disabling scope: eli_begin_disabled / eli_end_disabled. Pushes
 *        the ELI_ITEM_DISABLED item flag and dims style.alpha by disabled_alpha,
 *        nestable and honoring an already-disabled outer scope.
 *
 * Mirrors Dear ImGui's BeginDisabled/EndDisabled. Inside a disabled scope every
 * subsequently submitted widget sees ELI_ITEM_DISABLED in current_item_flags and
 * is drawn with the reduced alpha; the exact prior state is restored on end.
 *
 * @status Phase 25 disabling scope in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_INTERACTION_ELI_DISABLED_H
#define ELI_INTERACTION_ELI_DISABLED_H

#include "../core/eli_platform.h"
#include "../core/eli_enums.h"
#include "../core/eli_context.h"
#include "../style/eli_item_flags.h"
#include "../style/eli_style_stack.h"

/* Depth of nested eli_begin_disabled scopes we can track. Matches the item-flag
 * stack budget in spirit; disabling nesting is realistically shallow. */
#define ELI_DISABLED_STACK_MAX 64

/* For each open disabled scope, whether it was the transition into disabled and
 * therefore pushed the dimmed-alpha style var (so end knows to pop it). Nested
 * scopes that were already disabled push only the item flag. File-static: no
 * context field is added for this bookkeeping. */
static bool eli_disabled__pushed_alpha[ELI_DISABLED_STACK_MAX];
static int eli_disabled__stack_size = 0;

/**
 * Open a disabled scope. When @p disabled is true and the current scope is not
 * already disabled, sets ELI_ITEM_DISABLED for subsequent widgets and multiplies
 * style.alpha by style.disabled_alpha (via the style-var stack). Nestable: an
 * inner eli_begin_disabled(false) inside a disabled outer scope stays disabled.
 * Every call must be paired with eli_end_disabled.
 *
 * @param disabled  true to disable subsequently submitted widgets.
 *
 * Thread-safe: no (mutates the current context)
 * Reentrant: no
 */
static inline void eli_begin_disabled(bool disabled)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;

    bool was_disabled = (ctx->current_item_flags & ELI_ITEM_DISABLED) != 0;
    bool pushed_alpha = false;
    if (!was_disabled && disabled) {
        eli_push_style_var(ELI_STYLE_VAR_ALPHA, ctx->style.alpha * ctx->style.disabled_alpha);
        pushed_alpha = true;
    }

    /* Honor an already-disabled outer scope: once disabled, stay disabled. */
    eli_push_item_flag(ELI_ITEM_DISABLED, was_disabled || disabled);

    if (eli_disabled__stack_size < ELI_DISABLED_STACK_MAX)
        eli_disabled__pushed_alpha[eli_disabled__stack_size] = pushed_alpha;
    eli_disabled__stack_size++;
}

/**
 * Close the scope opened by the most recent eli_begin_disabled, restoring the
 * item flags and (if this scope dimmed it) the previous style.alpha exactly. A
 * no-op if no disabled scope is open.
 *
 * Thread-safe: no (mutates the current context)
 * Reentrant: no
 */
static inline void eli_end_disabled(void)
{
    if (eli_disabled__stack_size <= 0)
        return;

    eli_disabled__stack_size--;
    bool pushed_alpha = (eli_disabled__stack_size < ELI_DISABLED_STACK_MAX)
                            ? eli_disabled__pushed_alpha[eli_disabled__stack_size]
                            : false;

    eli_pop_item_flag();
    if (pushed_alpha)
        eli_pop_style_var(1);
}

#endif /* ELI_INTERACTION_ELI_DISABLED_H */
