# Drag & Drop (Phase 21)

Immediate-mode drag and drop for elimgui, mirroring Dear ImGui's source/target
contract. Any item that becomes the active (held) item can start a drag; any later
item can accept the dragged payload. All drag state (the active flag, the payload,
target/accept bookkeeping, and the payload heap buffer) is module-private and lives
entirely in `include/eli/widgets/eli_drag_drop.h`.

```c
#include <eli/widgets/eli_drag_drop.h>
```

## Concepts

- **Source** — the widget you drag from. A drag becomes active when the last item
  submitted is the active item (mouse held on it) and the mouse is dragging past the
  drag threshold. The source sets a **payload**.
- **Payload** (`eli_payload`) — a type-tagged blob copied into module-private storage
  (allocated via the `eli_mem` seam). It carries the source id, the blob pointer and
  size, the type tag, and per-frame `preview`/`delivery` state.
- **Target** — the widget you drop onto. A target is live when the mouse hovers its
  rect while a payload is active. It **accepts** payloads of a given type; the payload
  is *delivered* on the frame the mouse is released over the target.

## Lifecycle hooks

Because the phase does not touch the core frame path, two hooks must be called by the
host/backend around the frame:

| Function | When | Effect |
|----------|------|--------|
| `eli_drag_drop_new_frame()` | right after `eli_new_frame()` | rolls the accept-id snapshot; clears per-frame markers |
| `eli_drag_drop_end_frame()` | just before `eli_end_frame()` | clears the payload once delivered or once the drag expires |

`eli_clear_drag_drop()` force-resets all drag state and releases the payload buffer;
it is called automatically by the end-of-frame hook and is also useful in tests.

## Payload

```c
typedef struct {
    void   *data;             // pointer to the copied blob (valid while dragging)
    int     data_size;        // blob size in bytes
    eli_id  source_id;        // id of the source item
    eli_id  source_parent_id; // id of the source's parent, if any
    int     data_frame_count; // frame the payload was last set
    char    data_type[33];    // NUL-terminated type tag (<= 32 chars)
    bool    preview;          // target is hovering (peek) this frame
    bool    delivery;         // payload delivered this frame
} eli_payload;
```

The `data` pointer is only valid while a drag is active. Copy it out during delivery
if it must outlive the drop.

## API

### Source

- `bool eli_begin_drag_drop_source(eli_drag_drop_flags flags)` — open a source scope on
  the last-submitted item. Returns `true` while the drag is active. Draws a preview
  tooltip near the mouse unless `ELI_DRAG_DROP_SOURCE_NO_PREVIEW_TOOLTIP` is set.
- `bool eli_set_drag_drop_payload(const char *type, const void *data, size_t sz, eli_cond cond)`
  — copy `data` into the payload, tagged with `type` (truncated to 32 bytes). With
  `ELI_COND_ALWAYS` (the default when `cond == ELI_COND_NONE`) the blob is refreshed
  every frame. Returns `true` if a target accepted the payload this or last frame.
- `void eli_end_drag_drop_source(void)` — close the source scope.

### Target

- `bool eli_begin_drag_drop_target(void)` — turn the last-submitted item into a drop
  target. Succeeds only while a payload is active and the mouse hovers the item inside
  the hovered window.
- `const eli_payload *eli_accept_drag_drop_payload(const char *type, eli_drag_drop_flags flags)`
  — accept the payload if it matches `type` (or any type when `type == NULL`). Returns
  the payload on delivery (mouse released over the target after a hover), or while
  hovered when `ELI_DRAG_DROP_ACCEPT_BEFORE_DELIVERY` is set. When targets nest, the
  smallest rect wins. Draws the default highlight rect unless
  `ELI_DRAG_DROP_ACCEPT_NO_DRAW_DEFAULT_RECT` is set.
- `void eli_end_drag_drop_target(void)` — close the target scope.
- `const eli_payload *eli_get_drag_drop_payload(void)` — peek at the active payload
  outside a target scope (NULL when no drag is active).

## Flags (`eli_drag_drop_flags`)

Source: `SOURCE_NO_PREVIEW_TOOLTIP`, `SOURCE_NO_DISABLE_HOVER`,
`SOURCE_NO_HOLD_TO_OPEN_OTHERS`, `SOURCE_ALLOW_NULL_ID`, `SOURCE_EXTERN`,
`SOURCE_AUTO_EXPIRE_PAYLOAD`.

Target: `ACCEPT_BEFORE_DELIVERY`, `ACCEPT_NO_DRAW_DEFAULT_RECT`,
`ACCEPT_NO_PREVIEW_TOOLTIP`, and the convenience `ACCEPT_PEEK_ONLY`
(= `ACCEPT_BEFORE_DELIVERY | ACCEPT_NO_DRAW_DEFAULT_RECT`).

## Usage example

```c
eli_new_frame();
eli_drag_drop_new_frame();

eli_begin("Palette", NULL, 0);

eli_button("Color A");
if (eli_begin_drag_drop_source(ELI_DRAG_DROP_NONE)) {
    int color = 0xFF3366;
    eli_set_drag_drop_payload("COLOR", &color, sizeof(color), ELI_COND_ALWAYS);
    eli_end_drag_drop_source();
}

eli_button("Drop here");
if (eli_begin_drag_drop_target()) {
    const eli_payload *p = eli_accept_drag_drop_payload("COLOR", ELI_DRAG_DROP_NONE);
    if (p != NULL) {
        int dropped = *(const int *)p->data;   // delivered on release
        (void)dropped;
    }
    eli_end_drag_drop_target();
}

eli_end();

eli_drag_drop_end_frame();
eli_end_frame();
```

## Delivery timing (gotcha)

Delivery requires the target to have been accepted on the **previous** frame, so a
real drop needs the target hovered for at least one frame before release (frame N:
hover accepts and sets preview; frame N+1: release delivers). A drag with no delivery
expires one frame after the button is released and the source stops refreshing the
payload — this one-frame grace matches Dear ImGui.

## Notes / deferred

- The drag preview is rendered onto the current window's draw list (there is no
  dedicated tooltip/overlay layer yet), so it follows the mouse but is clipped to the
  source window. A future tooltip-window phase can replace this without API changes.
- `SOURCE_NO_HOLD_TO_OPEN_OTHERS` and `SOURCE_NO_DISABLE_HOVER` are accepted but not
  yet acted upon (there is no hold-to-open-others behavior in the current widget set).
