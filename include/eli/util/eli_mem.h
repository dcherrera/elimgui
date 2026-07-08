/**
 * @file eli_mem.h
 * @brief User-overridable allocator API for elimgui, matching Dear ImGui's memory model.
 *
 * Provides eli_alloc_func / eli_free_func typedefs and a file-scope allocator state
 * that all elimgui internals route through.  Host applications may install a custom
 * allocator (pool, leak-tracker, sanitizer) via eli_set_allocator_functions() and
 * restore the platform default (malloc/free) by passing NULL for either function.
 *
 * Design note: The allocator state uses file-scope statics, which is correct for
 * elimgui's single-TU WASM build model and single-TU test binaries.  Multi-TU
 * hosts that need a shared allocator should install it in every TU that includes
 * this header.
 *
 * @status Complete — allocator swap, query, alloc, and free implemented.
 * @issues None
 * @todo None
 */
#ifndef ELI_UTIL_MEM_H
#define ELI_UTIL_MEM_H

#include "../core/eli_platform.h"

/* -------------------------------------------------------------------------
 * Public types
 * ---------------------------------------------------------------------- */

/**
 * Custom allocator function signature.
 *
 * @param sz         Requested allocation size in bytes.
 * @param user_data  Opaque context forwarded verbatim from eli_set_allocator_functions().
 * @return           Pointer to at least sz usable bytes, or NULL on failure.
 *
 * Thread-safe: caller-defined
 * Reentrant:   caller-defined
 */
typedef void *(*eli_alloc_func)(size_t sz, void *user_data);

/**
 * Custom deallocation function signature.
 *
 * @param ptr        Pointer to free (from a matching eli_alloc_func call), or NULL.
 * @param user_data  Opaque context forwarded verbatim from eli_set_allocator_functions().
 *
 * Thread-safe: caller-defined
 * Reentrant:   caller-defined
 */
typedef void (*eli_free_func)(void *ptr, void *user_data);

/* -------------------------------------------------------------------------
 * Internal: default allocator wrappers around the platform seam
 *
 * These are static functions stored by address in eli_s_alloc_func /
 * eli_s_free_func as the initial default.  The (void)user_data silences
 * unused-parameter warnings since the default wrappers have no user context.
 * ---------------------------------------------------------------------- */

static void *eli_default_alloc(size_t sz, void *user_data)
{
    (void)user_data;
    return malloc(sz);
}

static void eli_default_free(void *ptr, void *user_data)
{
    (void)user_data;
    free(ptr);
}

/* -------------------------------------------------------------------------
 * Module-private allocator state
 * ---------------------------------------------------------------------- */

static eli_alloc_func eli_s_alloc_func     = eli_default_alloc;
static eli_free_func  eli_s_free_func      = eli_default_free;
static void          *eli_s_alloc_userdata = NULL;

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

/**
 * Install a custom allocator pair for all elimgui internals.
 *
 * Pass NULL for alloc_fn or free_fn to restore the platform default (malloc/free)
 * for that slot.  Both should be replaced together; mixing a custom alloc with the
 * default free (or vice-versa) produces undefined behaviour because the wrong
 * function will receive pointers it did not allocate.
 *
 * Must be called when no elimgui context is active (before eli_create_context() or
 * after eli_destroy_context()), so no live pointer was allocated by a different
 * allocator than the one now being installed.
 *
 * @param alloc_fn   Replacement allocator, or NULL to restore the default.
 * @param free_fn    Replacement deallocator, or NULL to restore the default.
 * @param user_data  Opaque value forwarded verbatim to every alloc/free call.
 *
 * Thread-safe: no
 * Reentrant:   no
 */
static inline void eli_set_allocator_functions(
    eli_alloc_func alloc_fn,
    eli_free_func  free_fn,
    void          *user_data)
{
    eli_s_alloc_func     = (alloc_fn != NULL) ? alloc_fn : eli_default_alloc;
    eli_s_free_func      = (free_fn  != NULL) ? free_fn  : eli_default_free;
    eli_s_alloc_userdata = user_data;
}

/**
 * Retrieve the currently installed allocator functions and user_data.
 *
 * Any out-pointer may be NULL if the caller does not need that value.
 *
 * @param out_alloc      Written with the current eli_alloc_func; ignored when NULL.
 * @param out_free       Written with the current eli_free_func; ignored when NULL.
 * @param out_user_data  Written with the current user_data pointer; ignored when NULL.
 *
 * Thread-safe: no
 * Reentrant:   no
 */
static inline void eli_get_allocator_functions(
    eli_alloc_func *out_alloc,
    eli_free_func  *out_free,
    void          **out_user_data)
{
    if (out_alloc     != NULL) *out_alloc     = eli_s_alloc_func;
    if (out_free      != NULL) *out_free      = eli_s_free_func;
    if (out_user_data != NULL) *out_user_data = eli_s_alloc_userdata;
}

/**
 * Allocate sz bytes using the currently installed allocator.
 *
 * Returns NULL immediately when sz == 0, matching Dear ImGui behaviour and
 * avoiding implementation-defined malloc(0) semantics in downstream code.
 *
 * @param sz  Number of bytes to allocate.
 * @return    Pointer to allocated memory, or NULL if sz == 0 or allocation fails.
 *
 * Thread-safe: depends on the installed allocator
 * Reentrant:   depends on the installed allocator
 */
static inline void *eli_mem_alloc(size_t sz)
{
    if (sz == 0)
        return NULL;
    return eli_s_alloc_func(sz, eli_s_alloc_userdata);
}

/**
 * Free a pointer previously returned by eli_mem_alloc().
 *
 * No-op when ptr is NULL — safe to call unconditionally on nullable pointers.
 *
 * @param ptr  Pointer to free, or NULL.
 *
 * Thread-safe: depends on the installed allocator
 * Reentrant:   depends on the installed allocator
 */
static inline void eli_mem_free(void *ptr)
{
    if (ptr == NULL)
        return;
    eli_s_free_func(ptr, eli_s_alloc_userdata);
}

#endif /* ELI_UTIL_MEM_H */
