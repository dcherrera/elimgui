# ID System & State (Phase 5)

The ID system gives every widget a stable, path-dependent identity, tracks which
item is currently active/hot, and persists small per-item state across frames. It
lives under `include/eli/id/` and is pulled in with:

```c
#include <eli/id/eli_id.h>
```

All functions operate on the current context (`eli_get_current_context()`), so a
context must exist before they are called.

---

## 1. ID hashing

Identity hashing uses the standard reflected **CRC-32** algorithm (polynomial
`0xEDB88320`), matching Dear ImGui's `ImHashStr`/`ImHashData` byte-for-byte. The
256-entry lookup table is built lazily on first use.

| Function | Purpose |
|----------|---------|
| `eli_id eli_hash_str(const char *str, eli_id seed)` | Hash a NUL-terminated string against `seed`. |
| `eli_id eli_hash_data(const void *data, size_t size, eli_id seed)` | Hash `size` bytes against `seed`. |

- **Deterministic:** the same input and seed always produce the same id.
- **Seed-dependent:** the same string under a different seed produces a different
  id — this is how nested scopes disambiguate reused labels.

### Label conventions (`##` and `###`)

The hash honors Dear ImGui's label markers:

- **`##`** is *not* special to the hash. The full string is hashed, so `"Label"`
  and `"Label##x"` produce **different** ids. (Hiding the visible portion after
  `##` is a text-rendering concern, handled elsewhere — not here.)
- **`###`** resets the hash accumulator to the seed, discarding everything before
  it. So `"A###X"`, `"B###X"`, and `"###X"` all hash **equal** — giving an item a
  stable id even when its visible label changes.

```c
eli_hash_str("Label",    0) != eli_hash_str("Label##x", 0);  // distinct
eli_hash_str("A###X",    0) == eli_hash_str("B###X",    0);  // stable
```

---

## 2. ID stack

The **seed** for any derived id is the value on top of the context's id stack
(`0` when the stack is empty). Pushing a scope hashes the pushed value against the
current seed and pushes the result, so nested scopes yield distinct ids for the
same label. The stack is bounded by `ELI_ID_STACK_MAX`; a push past that bound is
a no-op, as is a pop on an empty stack.

### Deriving ids (no stack change)

| Function | Derives an id from |
|----------|--------------------|
| `eli_id eli_get_id(const char *str_id)` | A NUL-terminated string. |
| `eli_id eli_get_id_str(const char *begin, const char *end)` | A byte range `[begin, end)`. |
| `eli_id eli_get_id_ptr(const void *ptr_id)` | A pointer's bit pattern. |
| `eli_id eli_get_id_int(int int_id)` | An integer. |

Each returns `0` if there is no current context.

### Pushing / popping scopes

| Function | Effect |
|----------|--------|
| `void eli_push_id(const char *str_id)` | Push string-derived seed. |
| `void eli_push_id_str(const char *begin, const char *end)` | Push range-derived seed. |
| `void eli_push_id_ptr(const void *ptr_id)` | Push pointer-derived seed. |
| `void eli_push_id_int(int int_id)` | Push integer-derived seed. |
| `void eli_pop_id(void)` | Pop the top seed. |

```c
eli_push_id("list");
for (int i = 0; i < n; i++) {
    eli_push_id_int(i);
    eli_id row_id = eli_get_id("row");   // unique per i, even with the same label
    eli_pop_id();
}
eli_pop_id();
```

---

## 3. Active / hot id tracking

Two internal helpers let widgets coordinate interaction state:

- **Active id** — the item being interacted with (a held button/slider).
- **Hot id** — the item under the cursor this frame.

| Function | Purpose |
|----------|---------|
| `void eli_set_active_id(eli_id id)` | Make `id` active; sets the just-activated edge flag when the active id changes. |
| `void eli_clear_active_id(void)` | Clear the active item (`eli_set_active_id(ELI_ID_NONE)`). |
| `void eli_set_hot_id(eli_id id)` | Mark `id` as hot for this frame. |
| `eli_id eli_get_active_id(void)` | Current active id (`ELI_ID_NONE` if none). |
| `eli_id eli_get_hot_id(void)` | Current hot id (`ELI_ID_NONE` if none). |
| `bool eli_is_active_id(eli_id id)` | True if `id` is the active, non-zero id. |
| `void eli_id_new_frame(eli_context *ctx)` | Roll active/hot into their previous-frame snapshots and clear the just-activated flag. |

`ELI_ID_NONE` is `0` and is never considered active. `eli_id_new_frame` is meant
to be called once per frame by the frame-advance path so widgets can compare this
frame against the last (`ctx->active_id_previous_frame`, `ctx->hot_id_previous_frame`,
`ctx->active_id_is_just_activated`).

---

## 4. State storage (`eli_storage`)

`eli_storage` is a sorted key/value map keyed by `eli_id`, used to persist small
per-item state (open/closed, scroll offsets, custom flags) across frames. It
mirrors Dear ImGui's `ImGuiStorage`: entries live in one array kept sorted by key,
so lookups are O(log n) binary search and inserts shift the tail. Values share a
union (`int` / `float` / `void *`); `bool` is stored as int `0`/`1`. Memory grows
geometrically and is released by `eli_storage_clear`.

```c
typedef struct eli_storage_pair {
    eli_id key;
    union { int val_i; float val_f; void *val_p; };
} eli_storage_pair;

typedef struct eli_storage {
    eli_storage_pair *data;
    int size;
    int capacity;
} eli_storage;
```

Zero-initialize a storage (`eli_storage st = {0};`) before first use.

| Function | Purpose |
|----------|---------|
| `int eli_storage_get_int(const eli_storage *st, eli_id key, int def)` | Read int or `def`. |
| `void eli_storage_set_int(eli_storage *st, eli_id key, int val)` | Write int (overwrite in place). |
| `float eli_storage_get_float(const eli_storage *st, eli_id key, float def)` | Read float or `def`. |
| `void eli_storage_set_float(eli_storage *st, eli_id key, float val)` | Write float. |
| `void *eli_storage_get_void_ptr(const eli_storage *st, eli_id key, void *def)` | Read pointer or `def`. |
| `void eli_storage_set_void_ptr(eli_storage *st, eli_id key, void *val)` | Write pointer. |
| `bool eli_storage_get_bool(const eli_storage *st, eli_id key, bool def)` | Read bool or `def`. |
| `void eli_storage_set_bool(eli_storage *st, eli_id key, bool val)` | Write bool. |
| `void eli_storage_set_all_int(eli_storage *st, int val)` | Set every existing entry's int value. |
| `void eli_storage_clear(eli_storage *st)` | Free the backing array and reset to empty. |

### Current-context storage

| Function | Purpose |
|----------|---------|
| `void eli_set_state_storage(eli_storage *storage)` | Make `storage` the current context's active storage (`NULL` to clear). |
| `eli_storage *eli_get_state_storage(void)` | The current context's active storage, or `NULL`. |

```c
eli_storage st = {0};
eli_id key = eli_get_id("panel_open");
eli_storage_set_bool(&st, key, true);
bool is_open = eli_storage_get_bool(&st, key, false);  // -> true
eli_storage_clear(&st);
```

---

## Gotchas

- All ID/stack/active accessors require a current context; they return `0`/`NULL`
  when none is set.
- `##` does **not** change the id — only `###` does. Reusing the same full label
  (with no `###` tail) in the same scope collides intentionally.
- `eli_storage` values are un-typed at runtime: reading a key with the wrong
  accessor reinterprets the union bytes. Use one value type per key.
- Always `eli_storage_clear` a storage you own; the map allocates on the heap
  through the libc seam.
