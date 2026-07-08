/**
 * @file eli_id_stack.h
 * @brief The elimgui ID stack: push/pop of hashing seeds and the eli_get_id
 *        family that derives a widget id from the current seed.
 *
 * The seed for any id is the value on top of ctx->id_stack (0 when the stack is
 * empty). eli_push_id_* hashes its argument against the current seed and pushes
 * the result, so nested scopes produce distinct, path-dependent ids. Pushes are
 * bounded by ELI_ID_STACK_MAX and become no-ops when the stack is full.
 *
 * @status Phase 5 ID stack in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_ID_ELI_ID_STACK_H
#define ELI_ID_ELI_ID_STACK_H

#include "eli_id_hash.h"

#include "../core/eli_platform.h"
#include "../core/eli_types.h"
#include "../core/eli_context.h"

/**
 * @param ctx  Context to read the seed from (must be non-NULL).
 * @return     The current seed: top of the id stack, or 0 when empty.
 */
static inline eli_id eli_id_stack_seed(const eli_context *ctx)
{
    return (ctx->id_stack_size > 0) ? ctx->id_stack[ctx->id_stack_size - 1] : 0u;
}

/* ---------------------------------------------------------------------------
 * Deriving ids from the current seed (no stack mutation)
 * ------------------------------------------------------------------------- */

/**
 * Derive an id from a byte range against the current seed.
 *
 * @param str_id_begin  Start of the range (must be non-NULL).
 * @param str_id_end    End of the range (one past the last byte; must be >= begin).
 * @return              The derived id, or 0 if there is no current context.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline eli_id eli_get_id_str(const char *str_id_begin, const char *str_id_end)
{
    const eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return 0u;
    return eli_hash_data(str_id_begin, (size_t)(str_id_end - str_id_begin),
                         eli_id_stack_seed(ctx));
}

/**
 * Derive an id from a NUL-terminated string against the current seed.
 *
 * @param str_id  NUL-terminated label/id (must be non-NULL).
 * @return        The derived id, or 0 if there is no current context.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline eli_id eli_get_id(const char *str_id)
{
    const eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return 0u;
    return eli_hash_str(str_id, eli_id_stack_seed(ctx));
}

/**
 * Derive an id from a pointer value against the current seed.
 *
 * @param ptr_id  Pointer whose bit pattern identifies the item.
 * @return        The derived id, or 0 if there is no current context.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline eli_id eli_get_id_ptr(const void *ptr_id)
{
    const eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return 0u;
    return eli_hash_data(&ptr_id, sizeof(ptr_id), eli_id_stack_seed(ctx));
}

/**
 * Derive an id from an integer against the current seed.
 *
 * @param int_id  Integer identifying the item.
 * @return        The derived id, or 0 if there is no current context.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline eli_id eli_get_id_int(int int_id)
{
    const eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return 0u;
    return eli_hash_data(&int_id, sizeof(int_id), eli_id_stack_seed(ctx));
}

/* ---------------------------------------------------------------------------
 * Pushing/popping seeds
 * ------------------------------------------------------------------------- */

/** Push an already-computed seed. No-op if the stack is full or ctx is NULL. */
static inline void eli_id_stack_push_raw(eli_context *ctx, eli_id id)
{
    if (ctx == NULL || ctx->id_stack_size >= ELI_ID_STACK_MAX)
        return;
    ctx->id_stack[ctx->id_stack_size++] = id;
}

/**
 * Hash a byte range against the current seed and push the result as the new seed.
 *
 * @param str_id_begin  Start of the range (must be non-NULL).
 * @param str_id_end    End of the range (one past the last byte; must be >= begin).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_push_id_str(const char *str_id_begin, const char *str_id_end)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    eli_id id = eli_hash_data(str_id_begin, (size_t)(str_id_end - str_id_begin),
                              eli_id_stack_seed(ctx));
    eli_id_stack_push_raw(ctx, id);
}

/**
 * Hash a NUL-terminated string against the current seed and push the result.
 *
 * @param str_id  NUL-terminated label/id (must be non-NULL).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_push_id(const char *str_id)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    eli_id_stack_push_raw(ctx, eli_hash_str(str_id, eli_id_stack_seed(ctx)));
}

/**
 * Hash a pointer against the current seed and push the result.
 *
 * @param ptr_id  Pointer whose bit pattern identifies the scope.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_push_id_ptr(const void *ptr_id)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    eli_id_stack_push_raw(ctx, eli_hash_data(&ptr_id, sizeof(ptr_id),
                                             eli_id_stack_seed(ctx)));
}

/**
 * Hash an integer against the current seed and push the result.
 *
 * @param int_id  Integer identifying the scope.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_push_id_int(int int_id)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    eli_id_stack_push_raw(ctx, eli_hash_data(&int_id, sizeof(int_id),
                                             eli_id_stack_seed(ctx)));
}

/**
 * Pop the top seed off the id stack. No-op if the stack is empty or ctx is NULL.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_pop_id(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->id_stack_size <= 0)
        return;
    ctx->id_stack_size--;
}

#endif /* ELI_ID_ELI_ID_STACK_H */
