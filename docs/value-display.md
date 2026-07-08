# Value Display

Phase 24 adds four convenience functions for displaying labeled scalar values.
Each function formats `"prefix: value"` as a single plain-text item, exactly like
calling `eli_text("%s: ...", prefix, ...)` would — it advances the layout cursor
by the measured text size and records a last-item rect usable by the item-status
queries.

This mirrors Dear ImGui's `Value()` overloads in both behavior and output format.

---

## Include

```c
#include <eli/widgets/eli_value.h>
```

The header is standalone — including it pulls in the text widget and layout
dependencies automatically.

---

## Public API

### `eli_value_bool`

```c
void eli_value_bool(const char *prefix, bool b);
```

Displays `"prefix: true"` or `"prefix: false"`.

| Parameter | Description |
|-----------|-------------|
| `prefix`  | Label shown before the colon separator. |
| `b`       | Boolean value to display. |

**Thread-safe:** no &nbsp;**Reentrant:** no

---

### `eli_value_int`

```c
void eli_value_int(const char *prefix, int v);
```

Displays `"prefix: <v>"` using `%d`.

| Parameter | Description |
|-----------|-------------|
| `prefix`  | Label shown before the colon separator. |
| `v`       | Signed integer value to display. |

**Thread-safe:** no &nbsp;**Reentrant:** no

---

### `eli_value_uint`

```c
void eli_value_uint(const char *prefix, unsigned int v);
```

Displays `"prefix: <v>"` using `%u`.

| Parameter | Description |
|-----------|-------------|
| `prefix`  | Label shown before the colon separator. |
| `v`       | Unsigned integer value to display. |

**Thread-safe:** no &nbsp;**Reentrant:** no

---

### `eli_value_float`

```c
void eli_value_float(const char *prefix, float v, const char *float_format);
```

Displays `"prefix: <v>"` using the provided format specifier, or `"%.3f"` when
`float_format` is `NULL`.

| Parameter      | Description |
|----------------|-------------|
| `prefix`       | Label shown before the colon separator. |
| `v`            | Float value to display. |
| `float_format` | A `printf`-style format specifier for a single float/double (e.g., `"%.2f"`, `"%e"`), or `NULL` to use the default `"%.3f"`. |

**Thread-safe:** no &nbsp;**Reentrant:** no

---

## Usage Example

```c
#include <eli/widgets/eli_value.h>

void my_ui(bool active, int frame, unsigned int id, float speed)
{
    if (!eli_begin("Debug", NULL, 0))
        goto end;

    eli_value_bool("Active",  active);
    eli_value_int("Frame",   frame);
    eli_value_uint("ID",     id);
    eli_value_float("Speed", speed, NULL);       /* → "Speed: 3.142" */
    eli_value_float("Speed", speed, "%.1f");     /* → "Speed: 3.1"   */

end:
    eli_end();
}
```

---

## Output Format

| Function            | Output string              | Default precision |
|---------------------|---------------------------|-------------------|
| `eli_value_bool`    | `"prefix: true/false"`    | n/a               |
| `eli_value_int`     | `"prefix: %d"`            | n/a               |
| `eli_value_uint`    | `"prefix: %u"`            | n/a               |
| `eli_value_float`   | `"prefix: <float_format>"` | `%.3f`            |

---

## Implementation Notes

- All output is bounded to `ELI_VALUE_BUFFER_SIZE` (256 bytes) — no heap
  allocation occurs per frame.
- Each function calls `eli_text_unformatted` internally, so the item participates
  fully in the layout system: the cursor advances, `eli_get_item_rect_size` reflects
  the formatted string size, and `eli_same_line` / groups work as expected.
- `eli_value_float` with a custom format uses a two-step approach: format the float
  value first, then combine with the prefix. This keeps each `snprintf` call
  type-safe.

---

## Gotchas

- `float_format` must be a format specifier for a **single float/double** argument.
  Passing a multi-argument format string is undefined behavior.
- Output strings longer than 255 characters (excluding the NUL terminator) are
  silently truncated.
- These functions emit non-interactive text items (id = 0). They cannot be hovered,
  clicked, or focused — use them for display only.
