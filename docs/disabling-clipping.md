# Disabling & Clipping (Phase 25)

The `interaction` category (`include/eli/interaction/`) adds three widget-scope
mechanisms: disabling widgets, clipping their rendering/culling region, and
requesting focus. Pull it all in with:

```c
#include <eli/interaction/eli_interaction.h>
```

Individual headers are also usable standalone:

- `eli/interaction/eli_disabled.h` — `eli_begin_disabled` / `eli_end_disabled`
- `eli/interaction/eli_clip.h` — `eli_push_clip_rect` / `eli_pop_clip_rect`
- `eli/interaction/eli_focus.h` — focus requests

All state added by this phase is file-static; no core context field was added.

---

## Disabling

Wrap a block of widgets to render them dimmed and non-interactive. Mirrors Dear
ImGui's `BeginDisabled` / `EndDisabled`.

```c
void eli_begin_disabled(bool disabled);
void eli_end_disabled(void);
```

Inside a disabled scope:

- `ELI_ITEM_DISABLED` is set in `ctx->current_item_flags`, so widgets and item
  queries (e.g. `eli_is_item_hovered`) treat items as disabled.
- `style.alpha` is multiplied by `style.disabled_alpha` (default `0.60`) via the
  style-var stack, so subsequent draws are dimmed.

Behavior:

- **Nestable.** An inner `eli_begin_disabled(false)` inside an already-disabled
  outer scope stays disabled — once disabled, the scope remains disabled until the
  matching outer `eli_end_disabled`.
- **Dims once.** `style.alpha` is reduced only on the transition into disabled;
  nested scopes do not multiply it again. `eli_end_disabled` restores the exact
  prior alpha and item flags.
- Every `eli_begin_disabled` must be paired with an `eli_end_disabled`.

```c
eli_begin_disabled(!form_is_valid);
if (eli_button("Submit", size)) { /* ... */ }   // dimmed + inert when invalid
eli_end_disabled();
```

---

## Clipping

Push a clip rectangle for the current window. Mirrors Dear ImGui's
`PushClipRect` / `PopClipRect`.

```c
void eli_push_clip_rect(eli_vec2 clip_min, eli_vec2 clip_max,
                        bool intersect_with_current);
void eli_pop_clip_rect(void);
```

- `clip_min` / `clip_max` — the clip rectangle in screen space.
- `intersect_with_current` — when true, the new clip is intersected with the
  active one (the common case); when false it replaces it outright.

The wrapper pushes onto the current window's draw-list clip stack **and** mirrors
the resulting clip into `window->clip_rect`. Because `eli_item_add` tests item
visibility against `window->clip_rect`, an item lying fully outside the active
clip is culled: `eli_item_add` returns `false` and `eli_is_item_visible` reports
`false`. `eli_pop_clip_rect` restores both the draw-list clip and the window clip
rect. No-op when there is no current window.

```c
eli_push_clip_rect(rect_min, rect_max, true);
// items fully outside [rect_min, rect_max] are not drawn and report not-visible
eli_pop_clip_rect();
```

---

## Focus

Minimal, navigation-lite focus requests that drive `ctx->nav_id`.

```c
void eli_set_item_default_focus(void);
void eli_set_keyboard_focus_here(int offset);
bool eli_focus_has_keyboard_request(void);
bool eli_focus_consume_keyboard_request(eli_id id);
```

### `eli_set_item_default_focus`

Call immediately after submitting an item to make it the default navigation focus
for its window — but only on the frame the window is **appearing** (first shown),
so later user navigation is not overridden. Sets `ctx->nav_id` to the last item's
id; `eli_is_item_focused` then reports `true`. **Fully wired.**

### `eli_set_keyboard_focus_here(offset)`

Requests keyboard focus relative to submission order:

- `offset == 0` — focus the next item to be submitted.
- `offset  > 0` — focus the item submitted `offset` items after the call.
- `offset  < 0` — focus an already-submitted item; `-1` targets the last item and
  is applied to `ctx->nav_id` immediately.

A zero/positive request is stored file-static and consumed by a future item via
`eli_focus_consume_keyboard_request(id)`, which skips items until the requested
offset is reached, then sets `ctx->nav_id` to that item's id and clears the
request. `eli_focus_has_keyboard_request` reports whether one is pending. Only one
request is tracked at a time.

### Wired vs deferred

- **Wired:** the request storage/consume API, immediate `-1` targeting, and
  `eli_set_item_default_focus` on appearing windows.
- **Deferred:** automatic consumption from inside `eli_item_add` (the layout core
  is owned by an earlier phase and is not modified here), and full keyboard
  navigation / tabbing traversal (a later nav phase). Until `eli_item_add` calls
  `eli_focus_consume_keyboard_request`, a widget wanting to honor a forward request
  must call it explicitly with its own id.
