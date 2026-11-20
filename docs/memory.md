# Memory Management

**Phase 29** — `include/eli/util/eli_mem.h`

## Overview

elimgui routes every internal allocation through a replaceable function pair
(`eli_alloc_func` / `eli_free_func`), matching Dear ImGui's allocator-swap model.
By default the library uses the platform's `malloc`/`free` (JAClibc in WASM builds,
host libc in hosted test builds).  Host applications can install a custom allocator
at startup — for pool allocation, leak tracking, or a sanitizer — and restore the
default by passing `NULL`.

All elimgui code that needs heap memory calls `eli_mem_alloc()` / `eli_mem_calloc()` /
`eli_mem_realloc()` / `eli_mem_free()` rather than libc directly, so a single call to
`eli_set_allocator_functions()` redirects the entire library: context, font atlas
(including stb_truetype's scratch), windows, draw lists, storage, tables, tab bars,
plots, docking, settings, logging, clipboard and drag-drop.

Allocations are not absent; they are amortized. Buffers grow geometrically while the
UI grows (new windows, longer text, more vertices) and are reused after, so
steady-state frames allocate nothing. Everything is returned to the allocator when
the context is destroyed (module-level buffers such as the table/tab pools, plot
scratch, settings store and log buffer are released by the context's shutdown hooks).

### Giving memory back

Buffers are kept between frames, but a one-off spike does not stay forever:

- **Draw lists shrink after a spike.** `eli_draw_list_reset` (once per frame drawn)
  tracks the high water of the last `ELI_DRAW_LIST_SHRINK_FRAMES` frames (600). At the
  end of each such window, a command, vertex or index buffer of at least
  `ELI_DRAW_LIST_SHRINK_MIN_BYTES` (64 KiB) that is more than 4x the high water is
  reallocated to 2x it. Growth is 1.5x, so a list that draws the same every frame never
  goes past 4x and never reallocates. Both macros can be overridden before including
  elimgui; `ELI_DRAW_LIST_SHRINK_FRAMES 0` turns shrinking off.
- **Unused windows give back their draw lists.** A window not begun for
  `io.config_memory_compact_frames` frames (default 600; 0 = never) has its draw list's
  buffers freed (`eli_draw_list_release_buffers`) at the next frame start; position,
  size, scroll and state stay, and its next begin regrows the list (Dear ImGui's
  `GcCompactTransientWindowBuffers`).
- **Text reserves in chunks.** `eli_draw_list_add_text_ex` reserves room for glyph
  quads as it emits them, `ELI_FONT_TEXT_RESERVE_CHUNK` (256) at a time and never more
  than the bytes left, instead of one quad per byte up front. A long run that is mostly
  clipped, multi-byte UTF-8 or whitespace no longer grows the list to its byte count,
  and a run longer than 16 384 bytes is no longer refused whole by the 16-bit index
  guard when only part of it is visible.

`tests/unit/test_draw_memory.c` covers all three; `tests/probe/red_proof_memory.sh`
mutates each and confirms the suite fails.

### Coverage

`tests/unit/test_mem_coverage.c` redefines `malloc`/`calloc`/`realloc`/`free` before
including elimgui, so any direct libc call from library code is counted as a bypass,
and checks every category allocates through a tracking allocator and frees every
block on destroy. `tests/bench/bench_alloc_seam.c` counts the same for a
representative app (40 windows with child panes, table, tab bar, plot, dock space,
overlays, ini save):

| | before | after |
|---|---|---|
| allocation calls the host allocator sees | 265 | 1564 |
| allocation calls bypassing it (malloc/calloc/realloc) | 1294 | 0 |
| coverage | 17.0% | 100% |
| frees bypassing it | 1268 | 0 |
| allocations per steady-state frame | 0 | 0 |

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

### `eli_mem_calloc`

```c
void *eli_mem_calloc(size_t count, size_t size);
```

Zeroed array allocation through the current allocator. Returns `NULL` when either
argument is 0, when `count * size` overflows, or on failure.

---

### `eli_mem_realloc`

```c
void *eli_mem_realloc(void *ptr, size_t old_size, size_t new_size);
```

Resizes a seam block. The allocator pair has no realloc hook, so this allocates the
new block, copies `min(old_size, new_size)` bytes and frees the old one; callers pass
the old size they already track (array capacity). On failure the old block is left
untouched and `NULL` is returned. `new_size == 0` frees `ptr`.

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

- **Single-TU state**: The allocator state is declared with `ELI_GLOBAL`: one copy
  per translation unit by default (the single-TU WASM build), one shared copy under
  `ELI_SHARED_STATE`.  Multi-TU hosts without `ELI_SHARED_STATE` must call
  `eli_set_allocator_functions()` in every TU that includes `eli_mem.h`.

- **Ownership across the API**: strings elimgui hands back for the caller to free
  (`eli_save_ini_settings_to_memory`, host file reads) come from the seam; free them
  with `eli_mem_free`, not `free`.

- **Alloc(0) returns NULL**: Unlike some libc implementations that return a unique
  pointer for zero-byte allocations, `eli_mem_alloc(0)` always returns `NULL`.
  Code that checks `ptr != NULL` as a "was it allocated?" guard should never call
  `eli_mem_alloc(0)`.

- **Lifetime ordering**: The allocator must outlive all pointers it allocated.
  Switch allocators only between context create/destroy cycles.
