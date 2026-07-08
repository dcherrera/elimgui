# Trees & Collapsing Headers (Phase 15)

Hierarchical, expandable widgets: **tree nodes** and **collapsing headers**. Both
persist their open/closed state across frames in the id key/value storage, render a
triangle arrow (or bullet), and toggle when the arrow or label is clicked. An open
tree node indents its children by pushing onto the indent and id stacks; a
collapsing header does not indent.

Header: `include/eli/widgets/eli_tree.h` (usable via direct include; routes libc
through `core/eli_platform.h`).

```c
#include <eli/widgets/eli_tree.h>
```

## Concepts

- **Open state** is stored per node, keyed by the node's id (or a caller-supplied
  storage id via `eli_set_next_item_storage_id`). It is kept in the current context
  storage (`eli_set_state_storage`) when one is set, otherwise in a
  process-lifetime fallback so trees "just work" without any per-window setup.
- **Toggling**: clicking the arrow toggles on mouse-down; clicking the label toggles
  on mouse-release. Flags refine this (`ELI_TREE_NODE_OPEN_ON_ARROW`,
  `ELI_TREE_NODE_OPEN_ON_DOUBLE_CLICK`).
- **Indentation**: an open, non-leaf tree node that is not a collapsing header
  pushes a child scope (`eli_tree_push`) — you must call `eli_tree_pop()` when the
  node returns `true`. Leaf nodes and collapsing headers never push, so they need no
  `eli_tree_pop`.

## Flags — `eli_tree_node_flags`

Defined in the header (guarded with `#ifndef`). Common ones:

| Flag | Effect |
|------|--------|
| `ELI_TREE_NODE_NONE` | Default behavior. |
| `ELI_TREE_NODE_SELECTED` | Draw the node with a selected/highlight background. |
| `ELI_TREE_NODE_FRAMED` | Full-width framed look (collapsing-header style). |
| `ELI_TREE_NODE_NO_TREE_PUSH_ON_OPEN` | Open but do not indent / push an id scope. |
| `ELI_TREE_NODE_DEFAULT_OPEN` | Start open the first time it is seen. |
| `ELI_TREE_NODE_OPEN_ON_ARROW` | Only the arrow toggles (label click selects). |
| `ELI_TREE_NODE_OPEN_ON_DOUBLE_CLICK` | Require a double-click to toggle. |
| `ELI_TREE_NODE_LEAF` | No arrow; always "open"; never pushes a scope. |
| `ELI_TREE_NODE_BULLET` | Draw a bullet instead of an arrow. |
| `ELI_TREE_NODE_SPAN_AVAIL_WIDTH` / `_SPAN_FULL_WIDTH` | Widen the interaction/frame rect. |
| `ELI_TREE_NODE_COLLAPSING_HEADER` | `FRAMED \| NO_TREE_PUSH_ON_OPEN \| NO_AUTO_OPEN_ON_LOG`. |

## API

### Tree nodes

```c
bool eli_tree_node(const char *label);
bool eli_tree_node_ex(const char *label, eli_tree_node_flags flags);
bool eli_tree_node_str(const char *str_id, const char *fmt, ...);
bool eli_tree_node_ptr(const void *ptr_id, const char *fmt, ...);
bool eli_tree_node_v(const char *str_id, const char *fmt, va_list args);
bool eli_tree_node_ex_str(const char *str_id, eli_tree_node_flags flags, const char *fmt, ...);
bool eli_tree_node_ex_ptr(const void *ptr_id, eli_tree_node_flags flags, const char *fmt, ...);
bool eli_tree_node_ex_v(const char *str_id, eli_tree_node_flags flags, const char *fmt, va_list args);
```

- **Plain / `_ex`**: the `label` is both the visible text and the id (its visible
  part stops at `##`).
- **`_str` / `_ptr`**: `str_id` / `ptr_id` is a stable identity, and the label is
  produced by printf-formatting `fmt` — use these when labels can repeat or change.
- **Returns** `true` when the node is open. For an open, non-leaf,
  non-collapsing-header node you must balance it with `eli_tree_pop()`.

### Push / pop

```c
void  eli_tree_push(const char *str_id);   // indent + push str_id onto the id stack
void  eli_tree_push_ptr(const void *ptr_id);
void  eli_tree_pop(void);                  // unindent + pop the id stack
float eli_get_tree_node_to_label_spacing(void);  // arrow-column width (font_size + frame_padding.x*2)
```

### Collapsing headers

```c
bool eli_collapsing_header(const char *label, eli_tree_node_flags flags);
bool eli_collapsing_header_bool(const char *label, bool *p_visible, eli_tree_node_flags flags);
```

- `eli_collapsing_header` is a framed section header that does **not** indent or push
  an id scope (so no `eli_tree_pop` is needed).
- `eli_collapsing_header_bool`: when `p_visible` is non-NULL and `*p_visible` is
  `true`, a small close button is drawn on the header's right edge; clicking it sets
  `*p_visible = false`. When `*p_visible` is `false`, the header is not shown and the
  function returns `false`. (This is separate from the *open* state, which is still
  controlled by clicking the header, `DEFAULT_OPEN`, or `eli_set_next_item_open`.)

### Next-item state

```c
void eli_set_next_item_open(bool is_open, eli_cond cond);
void eli_set_next_item_storage_id(eli_id storage_id);
```

- `eli_set_next_item_open` forces the next node's open state. `ELI_COND_ALWAYS`
  applies every frame; `ELI_COND_ONCE` / `ELI_COND_FIRST_USE_EVER` apply only until
  the state has been stored once (a value of `0` is treated as `ELI_COND_ALWAYS`).
- `eli_set_next_item_storage_id` overrides the storage **key** for the next node's
  open state, letting nodes share or relocate their persisted flag.

## Usage example

```c
if (eli_begin("Outliner", NULL, 0)) {
    // A framed section header (no tree_pop needed).
    if (eli_collapsing_header("Settings", ELI_TREE_NODE_DEFAULT_OPEN)) {
        eli_text("Some settings...");
    }

    // A tree node with children. Must pair with eli_tree_pop().
    if (eli_tree_node("Scene")) {
        for (int i = 0; i < 3; i++) {
            // Leaf: no arrow, always "open", never pushes -> no tree_pop.
            eli_tree_node_ex_str("obj", ELI_TREE_NODE_LEAF | ELI_TREE_NODE_BULLET,
                                 "Object %d", i);
        }
        eli_tree_pop();
    }

    // Force a node open this frame regardless of stored state.
    eli_set_next_item_open(true, ELI_COND_ALWAYS);
    if (eli_tree_node("Always Open")) {
        eli_text("child");
        eli_tree_pop();
    }
}
eli_end();
```

## Gotchas

- **Balance `eli_tree_pop`**: call it exactly when a tree node returns `true` and it
  actually pushed — i.e. non-leaf, non-`NO_TREE_PUSH_ON_OPEN`, non-collapsing-header.
  Leaves, collapsing headers, and `NO_TREE_PUSH_ON_OPEN` nodes must **not** be popped.
- **Identity**: use the `_str` / `_ptr` variants when labels repeat or are dynamic,
  otherwise two nodes with the same visible text would share one id (and one open
  state). Alternatively disambiguate with a `##suffix` in the label.
- **State container**: by default open state lives in a shared fallback storage. Call
  `eli_set_state_storage` if you want per-window/document isolation.
