# Drag & Drop

> Drag & drop has five phases, and users judge you on the feedback at every one of them: grab, lift, move, target, drop.

## Overview

DnD is powerful but low-discoverability and error-prone. The remedies are constant feedback (cursors, ghosts, highlights, insertion lines), forgiving thresholds, an always-available cancel, and non-drag alternatives for everything ([NN/g](https://www.nngroup.com/articles/drag-drop/), [Apple HIG](https://developer.apple.com/design/human-interface-guidelines/drag-and-drop), [Atlassian design guidelines](https://atlassian.design/components/pragmatic-drag-and-drop/design-guidelines/)).

---

## 1. The Interaction Phases

| Phase | Required feedback |
|-------|-------------------|
| **Target acquisition** | Draggable items signal it: grip handle, hover elevation/background, `grab` cursor |
| **Initiation** | Threshold passed → item "lifts": source dims to ~40%, ghost appears, cursor → `grabbing` |
| **Drag** | Ghost follows cursor; *all valid targets get a subtle highlight immediately*; hovered target gets strong highlight or insertion indicator |
| **Drop** | Item animates into place (~100–200ms); brief settle flash (Atlassian: selected-color flash ~700ms) |
| **Post-drop** | Result visible (scrolled into view if needed); **Undo available** |

## 2. Thresholds & Timings

| Parameter | Value | Source |
|-----------|-------|--------|
| Drag start threshold | **4–5px** movement (larger, 10–20px, where accidental drags are costly) | Windows default / NN/g |
| Press-and-hold vs context menu (touch) | disambiguate at ~500ms | [Microsoft](https://learn.microsoft.com/en-us/windows/apps/design/input/drag-and-drop) |
| Source item opacity during drag | **40%** | [Atlassian](https://atlassian.design/components/pragmatic-drag-and-drop/design-guidelines/) |
| Ghost/preview opacity | 40–75% (ghost must read as "in transit", not a duplicate) | consensus |
| Reorder displacement animation | **~100ms** | NN/g |
| Drop-target highlight transition | ~150–350ms ease-out | Atlassian |
| Auto-scroll activation zone | ~10–50px from scrollable edge | framework consensus |
| Spring-loaded open (hover over folder/tab) | **500–1000ms** dwell | Apple |
| Snap-back on cancel | ~200–300ms ease-out to origin | standard |

## 3. Drag Handles vs Whole-Item Drag

- **Whole-item drag** when the item has no other interactive content (canvas objects, file icons).
- **Explicit grip handle** (⋮⋮ 6-dot icon) when the row/card contains buttons, links, or editable text — the handle both *signals* draggability and *disambiguates* from clicking the content.
- Handle hit area ≥ 24px (44px touch); cursor `grab` on hover, `grabbing` while held.
- Discoverability is DnD's weakness — never rely on invisible draggability for important workflows.

## 4. Drop-Target Signaling

- **On drag start**, mark all valid targets subtly (tinted background or dashed outline) so the user knows where dropping is possible *before* hunting.
- **On hover**, strengthen: filled highlight for container targets; **insertion line for ordered lists** — Atlassian spec: **2px line** in the selection color with an **8px terminal dot**, drawn between items.
- **Invalid targets**: `not-allowed` cursor + no highlight (optionally a ⃠ badge on the ghost).
- Activate targets generously: trigger when the *dragged item's center* (not the cursor tip) overlaps the target edge — prevents "twitchy" reordering (NN/g).
- Copy vs move (where both exist): badge the ghost (+ for copy) and use modifiers per platform convention.

## 5. Reordering Lists & Trees

- Choose **insertion line** (precise, cheap) or **live displacement** (items animate apart, ~100ms) — displacement communicates the result better; insertion line scales better to dense lists. Either way, the result must be unambiguous before release.
- **Auto-scroll** the container when dragging near its top/bottom edge (accelerating with proximity).
- Trees: highlight *into-folder* drops (row highlight) distinctly from *between-siblings* drops (insertion line with indent showing target depth); spring-open collapsed folders after ~500–700ms hover.
- Collapse expanded subtrees while their parent is being dragged.
- Large items should shrink to a compact ghost (title-only card) while dragging.

## 6. Cancel & Recovery

- **Esc always cancels** an in-flight drag; item snaps back to origin.
- Release outside any target = cancel + snap-back (never silently delete/relocate).
- **Undo after drop** — DnD misfires are common; a toast with Undo (or Ctrl+Z support) makes the whole feature feel safe (NN/g).

## 7. Accessibility ([WCAG 2.5.7](https://www.w3.org/WAI/WCAG22/Understanding/dragging-movements.html))

- **Every drag operation needs a non-drag alternative** (Level AA): Move Up/Down buttons or context-menu "Move to…" for reordering; menu commands for dock/attach operations; click-track + numeric entry for sliders. Keyboard support alone doesn't satisfy 2.5.7 — a *single-pointer* (click-based) path is required.
- Keyboard model: focus item → Space to grab → arrows to move (announcing position) → Space/Enter to drop → Esc to cancel.
- Atlassian's pragmatic pattern: put the same outcomes in an **action menu** on the item ("Move to top / up / down / bottom") — often better than simulating drag with a keyboard.

---

## Do's & Don'ts

**Do**
- Show grab affordance at rest (handle or hover cue) — discoverability first.
- Dim source ~40%, ghost the payload, change the cursor.
- Highlight all valid targets on lift; strengthen on hover; insertion line for order.
- Animate displacement/settle ~100–200ms.
- Auto-scroll near edges; spring-open containers on dwell.
- Support Esc-cancel, snap-back, and post-drop Undo.
- Provide button/menu alternatives (WCAG 2.5.7).

**Don't**
- Start a drag on <4px of movement.
- Use a 100%-opaque drag image (reads as a duplicate).
- Reveal valid targets only after the user stumbles onto them.
- Make drop zones smaller than the things dragged onto them.
- Silently discard a drop on empty space.
- Make DnD the only way to do anything important.

## Common Pitfalls

1. **Invisible draggability** — no handle, no hover cue; the feature might as well not exist.
2. **Twitchy reorder** — flip-flopping insert position because targeting uses the cursor tip instead of item center.
3. **No escape** — Esc unhandled mid-drag.
4. **Un-scrollable drag** — can't drop below the fold because the list won't auto-scroll.
5. **Drop with no undo** — one slip reorganizes the user's world permanently.
6. **Click/drag conflation** — text selection or button clicks triggering drags (threshold + handle solve it).

---

## Applying It in elimgui (immediate-mode / desktop GUI)

- **Payload API (ImGui-proven, keep it):** `eli_begin_drag_drop_source()` / `eli_set_drag_drop_payload(type, data)` / `eli_begin_drag_drop_target()` / `eli_accept_drag_drop_payload(type)`. Typed payloads let targets self-select validity.
- **Bake the feedback into the framework, not the app:**
  - Source: once `drag_threshold` (default 5px, style-configurable) is exceeded, the library dims the source widget (multiply alpha to 0.4) and renders the ghost — default ghost = payload preview text/icon in a small rounded tooltip-like rect at ~80% alpha, offset ~12px from the cursor, always in the topmost draw layer.
  - Target: `eli_accept_drag_drop_payload()` automatically draws the hover highlight (primary @ ~30% fill or 2px inner border) on the target rect when a compatible payload hovers. Provide a flag to suppress if the app custom-draws.
- **Insertion-line helper for lists:** `eli_dnd_reorder_ctx` that, given item rects, computes the insertion index from the *dragged-center* rule and draws the 2px accent line with 8px end-dot. Reordering lists is the #1 app use case — solve it once.
- **Auto-scroll**: child windows/scroll regions auto-scroll when a drag hovers within 24px of their top/bottom edge (speed ramps with proximity).
- **Esc + snap-back**: context clears the payload on Esc; since IMGUI has no persistent ghost object, "snap-back" = simply not applying the move + a 150ms fade-out of the ghost at origin (cheap and sufficient).
- **Cursor plumbing**: expose `eli_set_cursor(ELI_CURSOR_GRAB/GRABBING/NOT_ALLOWED)` mapped to CSS cursors via jsio — cursor feedback is half of DnD's feel in a browser canvas.
- **Undo hook**: emit a `drop committed` event apps can pair with the undo-toast pattern (feedback-and-states.md).
- **Alternatives**: wherever the demo uses DnD (list reorder, dock), also demonstrate the context-menu "Move ▸" equivalent — set the standard for apps.
