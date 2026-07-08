/**
 * @file eli_storage.h
 * @brief eli_storage: a sorted key/value map keyed by eli_id, used to persist
 *        small per-item UI state (open/closed, scroll, custom flags) across
 *        frames, plus the current-storage accessors on the context.
 *
 * Mirrors Dear ImGui's ImGuiStorage: pairs are held in a single array kept
 * sorted by key, so lookups are O(log n) binary search and inserts shift the
 * tail. Values share a union (int/float/void*); bool is stored as int 0/1.
 * Memory grows geometrically via the libc seam and is released by
 * eli_storage_clear.
 *
 * @status Phase 5 state storage in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_ID_ELI_STORAGE_H
#define ELI_ID_ELI_STORAGE_H

#include "../core/eli_platform.h"
#include "../core/eli_types.h"
#include "../core/eli_context.h"

/** Initial capacity used the first time a storage grows. */
#define ELI_STORAGE_INIT_CAPACITY 8

/** One key/value entry. The value union is selected by the accessor used. */
typedef struct eli_storage_pair {
    eli_id key;
    union {
        int   val_i;
        float val_f;
        void *val_p;
    };
} eli_storage_pair;

/**
 * Sorted key/value map. Zero-initialize (all fields 0/NULL) before first use;
 * the accessors grow it on demand. Release with eli_storage_clear.
 */
typedef struct eli_storage {
    eli_storage_pair *data;
    int size;
    int capacity;
} eli_storage;

/* ---------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

/**
 * First index i in [0, size] whose key is >= the search key (binary search).
 * If the key is present, data[i].key == key; otherwise i is the insertion slot.
 */
static inline int eli_storage_lower_bound_index(const eli_storage *st, eli_id key)
{
    int first = 0;
    int count = st->size;
    while (count > 0) {
        int step = count / 2;
        int mid = first + step;
        if (st->data[mid].key < key) {
            first = mid + 1;
            count -= step + 1;
        } else {
            count = step;
        }
    }
    return first;
}

/** Ensure capacity for at least `needed` entries. Returns false on OOM. */
static inline bool eli_storage_reserve(eli_storage *st, int needed)
{
    if (needed <= st->capacity)
        return true;
    int new_cap = st->capacity ? st->capacity * 2 : ELI_STORAGE_INIT_CAPACITY;
    if (new_cap < needed)
        new_cap = needed;
    eli_storage_pair *grown =
        (eli_storage_pair *)realloc(st->data, (size_t)new_cap * sizeof(*grown));
    if (grown == NULL)
        return false;
    st->data = grown;
    st->capacity = new_cap;
    return true;
}

/**
 * Find the entry for `key`, inserting a fresh (zeroed value) entry in sorted
 * position if absent. Returns the entry index, or -1 on allocation failure.
 */
static inline int eli_storage_find_or_insert(eli_storage *st, eli_id key)
{
    int idx = eli_storage_lower_bound_index(st, key);
    if (idx < st->size && st->data[idx].key == key)
        return idx;
    if (!eli_storage_reserve(st, st->size + 1))
        return -1;
    memmove(&st->data[idx + 1], &st->data[idx],
            (size_t)(st->size - idx) * sizeof(*st->data));
    st->data[idx].key = key;
    st->data[idx].val_p = NULL;
    st->size++;
    return idx;
}

/* ---------------------------------------------------------------------------
 * Lifetime
 * ------------------------------------------------------------------------- */

/**
 * Release the backing array and reset the map to empty. Safe on a zeroed or
 * already-cleared storage.
 *
 * @param st  Storage to clear (no-op if NULL).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_storage_clear(eli_storage *st)
{
    if (st == NULL)
        return;
    free(st->data);
    st->data = NULL;
    st->size = 0;
    st->capacity = 0;
}

/* ---------------------------------------------------------------------------
 * Typed accessors
 * ------------------------------------------------------------------------- */

/**
 * @param st           Storage to read (must be non-NULL).
 * @param key          Lookup key.
 * @param default_val  Value returned when the key is absent.
 * @return             Stored int, or default_val if the key is not present.
 */
static inline int eli_storage_get_int(const eli_storage *st, eli_id key, int default_val)
{
    int idx = eli_storage_lower_bound_index(st, key);
    if (idx < st->size && st->data[idx].key == key)
        return st->data[idx].val_i;
    return default_val;
}

/**
 * Store an int, overwriting in place if the key already exists.
 *
 * @param st   Storage to write (must be non-NULL).
 * @param key  Key to set.
 * @param val  Value to store.
 */
static inline void eli_storage_set_int(eli_storage *st, eli_id key, int val)
{
    int idx = eli_storage_find_or_insert(st, key);
    if (idx >= 0)
        st->data[idx].val_i = val;
}

/**
 * @param st           Storage to read (must be non-NULL).
 * @param key          Lookup key.
 * @param default_val  Value returned when the key is absent.
 * @return             Stored bool, or default_val if the key is not present.
 */
static inline bool eli_storage_get_bool(const eli_storage *st, eli_id key, bool default_val)
{
    return eli_storage_get_int(st, key, default_val ? 1 : 0) != 0;
}

/** Store a bool (as int 0/1), overwriting in place if the key exists. */
static inline void eli_storage_set_bool(eli_storage *st, eli_id key, bool val)
{
    eli_storage_set_int(st, key, val ? 1 : 0);
}

/**
 * @param st           Storage to read (must be non-NULL).
 * @param key          Lookup key.
 * @param default_val  Value returned when the key is absent.
 * @return             Stored float, or default_val if the key is not present.
 */
static inline float eli_storage_get_float(const eli_storage *st, eli_id key, float default_val)
{
    int idx = eli_storage_lower_bound_index(st, key);
    if (idx < st->size && st->data[idx].key == key)
        return st->data[idx].val_f;
    return default_val;
}

/** Store a float, overwriting in place if the key exists. */
static inline void eli_storage_set_float(eli_storage *st, eli_id key, float val)
{
    int idx = eli_storage_find_or_insert(st, key);
    if (idx >= 0)
        st->data[idx].val_f = val;
}

/**
 * @param st           Storage to read (must be non-NULL).
 * @param key          Lookup key.
 * @param default_val  Value returned when the key is absent.
 * @return             Stored pointer, or default_val if the key is not present.
 */
static inline void *eli_storage_get_void_ptr(const eli_storage *st, eli_id key, void *default_val)
{
    int idx = eli_storage_lower_bound_index(st, key);
    if (idx < st->size && st->data[idx].key == key)
        return st->data[idx].val_p;
    return default_val;
}

/** Store a pointer, overwriting in place if the key exists. */
static inline void eli_storage_set_void_ptr(eli_storage *st, eli_id key, void *val)
{
    int idx = eli_storage_find_or_insert(st, key);
    if (idx >= 0)
        st->data[idx].val_p = val;
}

/**
 * Set every existing entry's int value to `val` (does not add keys).
 *
 * @param st   Storage to modify (must be non-NULL).
 * @param val  Value written to each entry's val_i.
 */
static inline void eli_storage_set_all_int(eli_storage *st, int val)
{
    for (int i = 0; i < st->size; i++)
        st->data[i].val_i = val;
}

/* ---------------------------------------------------------------------------
 * Current-context storage
 * ------------------------------------------------------------------------- */

/**
 * Set the storage that subsequent state lookups on the current context use.
 *
 * @param storage  Storage to make current (may be NULL to clear).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_set_state_storage(eli_storage *storage)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx != NULL)
        ctx->state_storage = storage;
}

/**
 * @return  The current context's active storage, or NULL if none/no context.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline eli_storage *eli_get_state_storage(void)
{
    eli_context *ctx = eli_get_current_context();
    return ctx ? ctx->state_storage : NULL;
}

#endif /* ELI_ID_ELI_STORAGE_H */
