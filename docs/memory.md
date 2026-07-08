# Memory Management

**Phase 29** — `include/eli/util/eli_mem.h`

## Overview

elimgui routes every internal allocation through a replaceable function pair
(`eli_alloc_func` / `eli_free_func`), matching Dear ImGui's allocator-swap model.
By default the library uses the platform's `malloc`/`free` (JAClibc in WASM builds,
host libc in hosted test builds).  Host applications can install a custom allocator
at startup — for pool allocation, leak tracking, or a sanitizer — and restore the
default by passing `NULL`.

All elimgui code that needs heap memory calls `eli_mem_alloc()` / `eli_mem_free()`
rather than `malloc`/`free` directly, so a single call to `eli_set_allocator_functions()`
redirects the entire library.

## Public API

### Types

```c
typedef void *(*eli_alloc_func)(size_t sz, void *user_data);
typedef void  (*eli_free_func )(void *ptr, void *user_data);
```

Both share the same `user_data` pointer that was passed to
`eli_set_allocator_functions()`.  The value is forwarded verbatim on every call.

---

### `eli_set_allocator_functions`

```c
void eli_set_allocator_functions(
    eli_alloc_func alloc_fn,
    eli_free_func  free_fn,
    void          *user_data);
```

Installs a replacement allocator pair.  Pass `NULL` for either function to restore
the platform default (`malloc` / `free`) for that slot.

**Constraints:**
- Both functions should be replaced together; mixing a custom alloc with the
  default free causes undefined behaviour (the wrong function sees a pointer it
  did not allocate).
- Call only when no elimgui context is active (before `eli_create_context()` or
  after `eli_destroy_context()`), to avoid mismatched allocator lifetimes.
- Thread-safe: **no** — call from a single thread before any concurrent use.

---

### `eli_get_allocator_functions`

```c
void eli_get_allocator_functions(
    eli_alloc_func *out_alloc,
    eli_free_func  *out_free,
    void          **out_user_data);
```

Returns the currently installed allocator pair and user_data.  Any out-pointer
may be `NULL` if that value is not needed.

- Thread-safe: **no**

---

### `eli_mem_alloc`

```c
void *eli_mem_alloc(size_t sz);
```

Allocates `sz` bytes using the current allocator.  Returns `NULL` immediately when
`sz == 0` (avoids implementation-defined `malloc(0)` behaviour in downstream code).

- Thread-safe: depends on the installed allocator

---

### `eli_mem_free`

```c
void eli_mem_free(void *ptr);
```

Frees `ptr` through the current allocator.  No-op when `ptr == NULL`.

- Thread-safe: depends on the installed allocator

---

## Usage Example

```c
#include <eli/util/eli_mem.h>
#include <stdlib.h>   /* malloc, free, calloc */
#include <string.h>   /* memset */

/* --- Custom tracking allocator ----------------------------------------- */

typedef struct {
    size_t total_allocated;
    size_t total_freed;
} AllocStats;

static AllocStats g_stats = {0};

static void *tracking_alloc(size_t sz, void *user_data)
{
    AllocStats *stats = (AllocStats *)user_data;
    void *p = malloc(sz);
    if (p != NULL)
        stats->total_allocated += sz;
    return p;
}

static void tracking_free(void *ptr, void *user_data)
{
    AllocStats *stats = (AllocStats *)user_data;
    if (ptr != NULL)
        stats->total_freed++;
    free(ptr);
}

/* --- Setup ------------------------------------------------------------- */

int main(void)
{
    /* Install before creating the context. */
    eli_set_allocator_functions(tracking_alloc, tracking_free, &g_stats);

    /* ... create context, run frames ... */

    /* Restore defaults and destroy. */
    eli_set_allocator_functions(NULL, NULL, NULL);
    return 0;
}
```

## Gotchas

- **Single-TU state**: The allocator state uses file-scope statics, one copy per
  translation unit.  In the typical elimgui usage pattern (a single `.c` entry file
  that includes the whole library), this is correct.  Multi-TU hosts that need a
  shared allocator must call `eli_set_allocator_functions()` in every TU that
  independently includes `eli_mem.h`.

- **Alloc(0) returns NULL**: Unlike some libc implementations that return a unique
  pointer for zero-byte allocations, `eli_mem_alloc(0)` always returns `NULL`.
  Code that checks `ptr != NULL` as a "was it allocated?" guard should never call
  `eli_mem_alloc(0)`.

- **Lifetime ordering**: The allocator must outlive all pointers it allocated.
  Switch allocators only between context create/destroy cycles.
