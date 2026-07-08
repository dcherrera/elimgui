# Feedback & States

> Silence reads as breakage. Every input, wait, emptiness, and error needs a designed response.

## Overview

This doc covers the two feedback layers: **micro** (widget states — hover, active, focus, disabled) and **macro** (loading, progress, empty states, errors, tooltips, toasts, undo). The governing numbers are Nielsen's 0.1s / 1s / 10s and Material's 8%/12%/38% state opacities.

---

## 1. Interactive Widget States

Every interactive widget needs distinct visuals for:

| State | Treatment ([Material state layers](https://m3.material.io/foundations/interaction/states/state-layers), [NN/g button states](https://www.nngroup.com/articles/button-states-communicate-interaction/)) |
|-------|-----------|
| **Default** | Resting signifier (fill/border) — visibly interactive without hover |
| **Hover** | +**8%** overlay of the content color; cursor change where applicable |
| **Active/pressed** | +**12%** overlay (or slight darken/translate); must appear the same frame as the press |
| **Focus (keyboard)** | **≥2px** ring, **3:1** contrast vs unfocused state ([WCAG 2.4.13](https://www.w3.org/WAI/WCAG22/Understanding/focus-appearance.html)); distinct from hover |
| **Selected** | Persistent fill (primary @ ~25–35%) — different from hover |
| **Disabled** | Content at **38%** opacity, no hover response, `not-allowed` where fitting |
| **Loading** | Spinner replaces/joins label; input blocked; label like "Saving…" |

**On disabled controls** ([NN/g](https://www.nngroup.com/videos/why-disabled-buttons-hurt-ux-and-how-to-fix-them/)): disabled-with-no-explanation is a UX dead end. Prefer (a) keep enabled and show a helpful error on click, or (b) disabled + tooltip explaining how to enable. Reserve plain disabled for obvious cases.

## 2. Response Times & Loading

Nielsen's thresholds ([NN/g](https://www.nngroup.com/articles/response-times-3-important-limits/)):

| Wait | Show |
|------|------|
| <0.1s | Nothing — just the result |
| 0.1–1s | Subtle busy cue at most |
| **1–10s** | **Spinner** (indeterminate) + verb label ("Loading…") |
| **>10s** (or known-long) | **Determinate progress bar** + % / steps / ETA + Cancel |

- Delay showing any spinner ~100–300ms so instant operations don't flash.
- **Skeleton screens** for 1–10s content loads that have a known layout (better perceived speed than a spinner); don't bother under ~300ms ([NN/g skeletons](https://www.nngroup.com/articles/skeleton-screens/)).
- Progress bars: move honestly. **Never stall at 99%** — use step labels ("Step 3 of 5: Indexing…") when the tail is unpredictable ([NN/g progress](https://www.nngroup.com/articles/progress-indicators/)). Users tolerate ~3× longer waits with a good indicator.

## 3. Empty States ([Carbon pattern](https://carbondesignsystem.com/patterns/empty-states-pattern/), [UXPin](https://www.uxpin.com/studio/blog/ux-best-practices-designing-the-overlooked-empty-states/))

Four kinds, all need *explanation + action*:

| Kind | Copy | CTA |
|------|------|-----|
| First use | What goes here + why it's useful | "Create your first project" |
| User-cleared | Positive confirmation ("All caught up") | Optional next step |
| No search results | "No results for 'kiwi'" + suggestions | Clear filters / edit search |
| Error | What failed, plainly | Retry / go back |

Never render a bare void or a lone "No data". Small icon/illustration optional; in pro tools keep it restrained.

## 4. Error Messages ([NN/g](https://www.nngroup.com/articles/error-message-guidelines/))

Rules: **human language** (no bare codes), **specific** (what exactly is wrong), **constructive** (how to fix), **polite** (no blame — avoid "invalid", "illegal", "you failed").

Severity routing:

| Severity | Surface |
|----------|---------|
| Field-level | Inline below the field (persistent until fixed) |
| Operation failed, recoverable | Inline alert/banner in context |
| Needs immediate decision / data loss | Modal dialog |
| FYI only | Toast — **never for errors requiring action** |

## 5. Tooltips

- **Show delay ~300–500ms** on hover ([consensus](https://www.setproduct.com/blog/tooltip-ui-design)); once one tooltip is open, siblings open instantly (shared-timer pattern); hide immediately on leave.
- Show on **keyboard focus** too (no delay).
- Position above the target by default, 4–8px gap, flip/clamp at screen edges; never under the cursor.
- Content: supplementary only — name + shortcut ("Save — Ctrl+S"), or explanation for disabled state. **Essential info must never live only in a tooltip.**
- Rich tooltips (title + body) fine; interactive content belongs in a popover instead.

## 6. Toasts / Notifications

- Duration **~5s default, up to 10s** for longer text (~30ms/word + padding); pause the timer on hover ([Carbon](https://carbondesignsystem.com/patterns/notification-pattern/)).
- One consistent position (bottom-right is the desktop-tool norm; bottom-center also common); stack newest-in, cap ~3 visible.
- Use for: confirmations, non-critical info, **undo offers**. Don't use for: errors needing action, anything the user must not miss, anything with a deadline ("expires in 5s" pressure is hostile).

## 7. Undo — the Confirmation Killer ([confirm vs undo](https://joshwayne.com/posts/confirm-or-undo/))

- Prefer **act + toast with Undo** over "Are you sure?" for reversible operations — dialogs get dismissed by reflex; undo preserves flow and encourages exploration.
- Undo window 5–8s in the toast (plus Ctrl+Z where an undo stack exists); soft-delete under the hood so undo actually works.
- Reserve confirmation dialogs for the truly irreversible (see windows-and-dialogs.md).

## 8. Status Indicators

- Dot badges for boolean status; numeric badges cap at "99+"; pair color with icon/text (never hue alone); position top-right of the parent.

---

## Do's & Don'ts

**Do**
- Render hover/active feedback the same frame as input.
- Draw a real keyboard focus ring (≥2px, 3:1) distinct from hover.
- One overlay rule for all widgets (8/12/38).
- Spinner after ~1s, determinate progress after ~10s, always with a label.
- Give every empty state a sentence and a button.
- Prefer undo-toast to confirm-dialog.

**Don't**
- Use color alone for any state.
- Show a spinner for <300ms operations, or an indeterminate spinner for minutes.
- Freeze a progress bar at 99%.
- Put required information only in tooltips.
- Toast an error that needs action.
- Disable buttons with no path to understanding why.

## Common Pitfalls

1. **Hover == focus** — keyboard users can't tell where they are.
2. **Flash of spinner** on fast operations (add show-delay).
3. **Optimistic progress** that lies, then stalls.
4. **Empty void screens** on first run — the single worst first impression.
5. **Tooltip-dependent UI** — icon bars unusable on touch/keyboard.
6. **Toast graveyard** — critical failures scrolling away unseen.

---

## Applying It in elimgui (immediate-mode / desktop GUI)

- **Central state resolver**: each widget computes `hovered / held / focused / disabled` from the context every frame; a single `eli_state_color(base, state)` applies the 8%/12% overlay and 38% disabled fade (see color-and-theming.md). All widgets route through it — uniform feedback with zero per-widget theme entries.
- **Focus ring in the nav layer**: when navigation is keyboard-driven (`ctx->nav_active`), draw a 2px primary-colored ring *outside* the focused widget's rect (rounded to match). Suppress it during pure mouse use, exactly like browsers' `:focus-visible`.
- **Spinner + progress widgets**: `eli_spinner(radius)` (rotating arc; needs per-frame time — trivial in immediate mode) and `eli_progress_bar(fraction, overlay_text)` with `fraction < 0` meaning indeterminate (animated sweep). Header comments should cite the 1s/10s rule so users pick correctly.
- **Toast system in the library**: `eli_toast(ELI_TOAST_INFO, "Saved", 5.0f)` and `eli_toast_action("Deleted 3 items", "Undo", &clicked, 6.0f)`. Context keeps a small queue; renders bottom-right above all windows; hover pauses the countdown. This unlocks the undo pattern for every app.
- **Tooltip timing owned by the context**: `eli_set_tooltip("Save — Ctrl+S")` displays only after the shared 400ms hover timer; timer stays "warm" for ~300ms after a tooltip closes so adjacent icons show instantly. Also trigger on nav focus.
- **Skeleton helper**: `eli_skeleton(rect)` drawing a pulsing neutral rounded rect (1.5s ease cycle) for async-loading panels — cheap, and it nudges apps away from blank panels.
- **Empty-state helper**: `eli_empty_state(icon, "No results for 'kiwi'", "Clear filters", &clicked)` — centered, secondary-color text, one button. Make the good pattern the easy pattern.
- **Caret/blink, hold-repeat, double-click** timings live in one `eli_io` config block (caret 500ms, tooltip 400ms, key-repeat 400/50ms) so apps can honor OS/user preferences later.
