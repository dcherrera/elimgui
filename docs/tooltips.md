# Tooltips (Phase 18)

Header: `include/eli/widgets/eli_tooltip.h` — usable via direct include
(`#include <eli/widgets/eli_tooltip.h>`). It is not pulled in by
`eli/widgets/eli_widgets.h`.

A tooltip is a small, auto-sized, input-less, chrome-less window positioned just
off the mouse cursor. It reuses the window system (`eli_begin`/`eli_end`) for
layout and draw output, and paints its background with `ELI_COL_POPUP_BG`. Emit
tooltip contents from inside a normal window frame, right after the item you want
to describe.

## Overview

The phase adds:

- **Explicit tooltip scope** — `eli_begin_tooltip` / `eli_end_tooltip` for
  arbitrary tooltip contents (text, widgets, images).
- **One-shot formatted tooltips** — `eli_set_tooltip` / `eli_set_tooltip_v` that
  begin a tooltip, print formatted text, and end it in one call.
- **Item-gated tooltips** — `eli_begin_item_tooltip`,
  `eli_set_item_tooltip` / `_v`, which show only once the *previous* item has been
  hovered past the configured hover delay.
- **Hover-delay accumulation** — the timer that gates the item variants, driven by
  `io.delta_time` and the `style.hover_delay_*` / `style.hover_stationary_delay`
  values.

## Public API

### `bool eli_begin_tooltip(void)`
Begin a tooltip window at the cursor. Currently always returns `true`. Emit the
body and balance with `eli_end_tooltip`.

### `void eli_end_tooltip(void)`
End the tooltip window opened by `eli_begin_tooltip` / `eli_begin_item_tooltip`.

### `void eli_set_tooltip(const char *fmt, ...)` / `eli_set_tooltip_v(fmt, args)`
Show a formatted one-shot tooltip at the cursor. Equivalent to
`begin_tooltip` + `eli_text(...)` + `end_tooltip`.

### `bool eli_begin_item_tooltip(void)`
Begin a tooltip only if the most-recent item has been hovered long enough
(uses `style.hover_flags_for_tooltip_mouse` — stationary + short delay +
allow-disabled). Balance with `eli_end_tooltip` **only when it returns `true`**.

### `void eli_set_item_tooltip(const char *fmt, ...)` / `eli_set_item_tooltip_v(fmt, args)`
Show a formatted tooltip for the most-recent item, only once it has been hovered
past the hover delay.

### `bool eli_is_item_hovered_for_tooltip(eli_hovered_flags flags)`
The gating query behind the item variants. Pass `ELI_HOVERED_FOR_TOOLTIP` to fold
in the style's shared mouse-tooltip flags; per-call `ELI_HOVERED_DELAY_*` bits
override the shared delay. Returns `true` once the base rect/window/active/disabled
gating passes **and** the hover-delay (and mouse-stationary) requirements are met.

## Hover-delay behavior

The delay accumulator mirrors Dear ImGui's `HoverItemDelay*` bookkeeping and lives
file-static in the header (the context has no slot for it):

- A per-item hover timer grows by `io.delta_time` each frame the same item stays
  hovered; it decays after a short grace period once the item is no longer hovered.
- A mouse-stationary timer grows while the mouse is still (resets on any motion);
  the `ELI_HOVERED_STATIONARY` flag requires the item to be "unlocked" by the mouse
  having been stationary for `style.hover_stationary_delay`.
- The effective delay comes from `style.hover_delay_short` (default 0.15s) or
  `style.hover_delay_normal` (0.40s) depending on the flags.

Because it is immediate-mode, the item variants must be called every frame after
the item for the timer to accumulate.

## Usage

```c
#include <eli/widgets/eli_widgets.h>
#include <eli/widgets/eli_tooltip.h>

if (eli_begin("Demo", NULL, 0)) {
    eli_button("Hover me");

    // Simple: formatted tooltip once the button is hovered past the delay.
    eli_set_item_tooltip("Clicks: %d", click_count);

    // Rich content, gated on hover delay:
    if (eli_begin_item_tooltip()) {
        eli_text("Detailed info");
        eli_separator();
        eli_text("Second line");
        eli_end_tooltip();
    }

    // Unconditional tooltip (e.g. while dragging):
    if (is_dragging) {
        if (eli_begin_tooltip()) {
            eli_text("Dragging...");
            eli_end_tooltip();
        }
    }

    eli_end();
}
```

## Gotchas

- **Warm-up frame.** Like any auto-resized window, a freshly-appearing tooltip
  needs one frame to measure its contents before it is sized and drawn; its text
  geometry appears from the second display frame. In steady state (a tooltip shown
  every frame) this is invisible.
- **Z-order.** Tooltip windows render in the normal window focus order, not a
  dedicated top layer. A window focused *after* the tooltip appears can overlap it.
  Since tooltips display while the user is hovering (not clicking), this is rarely
  an issue in practice.
- **Colors in tests / headless use.** The tooltip background (`ELI_COL_POPUP_BG`)
  and text (`ELI_COL_TEXT`) are skipped when their alpha is 0. Apply a theme
  (`eli_style_colors_dark(&ctx->style)`) so the tooltip actually produces geometry.
- **Balance rule.** Call `eli_end_tooltip` only when `eli_begin_tooltip` /
  `eli_begin_item_tooltip` returned `true`. The `eli_set_*` helpers manage the
  begin/end pair for you.
