# Accessibility

> A canvas-rendered GUI starts at zero accessibility — the pixels mean nothing to assistive tech. Everything must be designed in.

## Overview

Accessibility for elimgui splits into two layers: (1) **visual/motor guidance** you can satisfy entirely inside the library (contrast, target sizes, keyboard navigation, focus visibility, reduced motion, color independence), and (2) **assistive-technology integration** (screen readers), which for canvas UIs requires deliberate architecture. The numbers below are WCAG 2.1/2.2 plus platform guidance.

---

## 1. Contrast (WCAG 1.4.3, 1.4.11, 1.4.6)

| Element | Minimum ratio |
|---------|---------------|
| Normal text (<18pt / <14pt bold) | **4.5:1** (AA) |
| Large text (≥18pt or ≥14pt bold) | **3:1** (AA) |
| **UI component boundaries & meaningful graphics** (input borders, icons, focus rings, chart series) | **3:1** (AA, 1.4.11) |
| AAA text | 7:1 / 4.5:1 |

([Understanding 1.4.3](https://www.w3.org/WAI/WCAG22/Understanding/contrast-minimum), [1.4.11](https://www.w3.org/WAI/WCAG21/Understanding/non-text-contrast.html), [WebAIM](https://webaim.org/articles/contrast/))

Danger zones: secondary/disabled text, placeholder text, hairline borders, hover-only affordances. Check with a contrast tool; disabled *content* is exempt but shouldn't be illegible.

## 2. Target Sizes (WCAG 2.5.5 / 2.5.8)

| Guideline | Size |
|-----------|------|
| WCAG 2.5.8 (AA, minimum) | **24×24 px** |
| WCAG 2.5.5 (AAA) / Apple HIG | **44×44** |
| Material | 48×48dp, ≥8dp between targets |
| Fluent | 40epx |

Desktop mouse UIs can run compact visuals, but the **hit area** should still be ≥24px with ≥8px between adjacent targets. Inline text links are exempt.

## 3. Keyboard Operability (2.1.1, 2.1.2, 2.4.3)

- **Everything** must be operable by keyboard — every button, slider, menu, dock action, drag operation needs a key/menu path.
- **No keyboard traps**: focus can always leave (Tab/Shift+Tab/Esc).
- **Logical focus order** matching visual/reading order.
- The **composite-widget pattern** ([APG keyboard interface](https://www.w3.org/WAI/ARIA/apg/practices/keyboard-interface/)): a group (toolbar, radio group, tree, table, tab bar) is **one Tab stop**; **arrow keys** move within it ("roving focus"). Tab from anywhere inside exits the group. This keeps Tab-through fast in dense tool UIs.

Standard key map:

| Key | Meaning |
|-----|---------|
| Tab / Shift+Tab | Next/previous control or group |
| Arrows | Move within groups, lists, trees, menus, sliders |
| Space | Toggle/activate focused control |
| Enter | Activate / default action / commit |
| Esc | Cancel, close topmost popup/modal |
| Home/End | First/last |
| F6-style | Cycle panels/panes (recommended for docked layouts) |

## 4. Focus Visibility (2.4.7, 2.4.11, 2.4.13)

- A visible focus indicator is mandatory (AA). Spec for a solid one ([2.4.13](https://www.w3.org/WAI/WCAG22/Understanding/focus-appearance.html)): **≥2px** ring around the control's perimeter with **≥3:1** contrast against the unfocused state.
- Focused elements must not be fully obscured by other layers (2.4.11) — auto-scroll focused widgets into view.
- Dialogs: focus in on open, trapped inside while modal, **restored to the trigger on close**.
- Distinguish keyboard-focus (ring) from mouse-hover (fill) — the `:focus-visible` model: show the ring during keyboard navigation, keep it quiet during pure mouse use.

## 5. Screen Readers & Canvas UIs

A canvas is a bitmap: **no accessibility tree, no text, no roles** ([WebAIM on canvas](https://webaim.org/blog/future-web-accessibility-html-canvas/)). Options, in increasing fidelity:

1. **Canvas fallback content / `aria-label`** — only viable for static graphics, not a GUI.
2. **Parallel semantic DOM** (the Flutter-web / Google-Docs approach): maintain a hidden DOM tree mirroring the widget tree — one element per widget with `role`, name, state, and position — synchronized as the UI changes; input events from those elements route back into the GUI ([Flutter web a11y](https://docs.flutter.dev/ui/accessibility/web-accessibility)). Flutter gates this behind an "enable accessibility" toggle to avoid constant sync cost.
3. Native accessibility APIs (out of scope for pure-WASM/browser; the DOM *is* the native API here).

Realistic path for a canvas GUI library: design the widget API so every widget has a **label** (you already require this for rendering), keep a per-frame widget list with id/role/rect/state, and expose an optional semantics-DOM backend later. What you must not do is design an API where widgets are anonymous rectangles — that forecloses accessibility permanently.

## 6. Reduced Motion (2.3.3, `prefers-reduced-motion`)

- Vestibular disorders make large movement (parallax, zooming panels, sliding surfaces) physically unpleasant.
- Honor the OS setting via the `prefers-reduced-motion` media query ([MDN](https://developer.mozilla.org/en-US/docs/Web/CSS/Reference/At-rules/@media/prefers-reduced-motion)): replace movement with **fades or instant changes**; keep tiny feedback transitions and spinners.
- Never rely on animation as the only signal for anything.

## 7. Color Independence & Color Blindness (1.4.1)

- ~8% of men have some color-vision deficiency. **Never encode meaning in hue alone**: error = red **+ icon + message**; selected = fill **+ check/weight**; chart series = color **+ direct labels/markers**.
- Avoid red-vs-green as the only differentiator anywhere; test the UI in grayscale.

## 8. Text Resize (1.4.4) & High Contrast

- Support **200% text/UI scaling without loss** — a global scale factor applied to font sizes and layout metrics; don't hardcode pixel layouts that clip at larger scales.
- Windows High Contrast / `forced-colors` will not reach into a canvas — offer your own **high-contrast theme** (pure role-based theming makes this a palette swap).

## 9. Cognitive Basics (3.2, 3.3)

Plain language, consistent naming and placement (same command = same label everywhere), predictable behavior (no surprise context changes on focus), clear error identification with suggestions, confirmation for destructive/irreversible acts. These fall out of the foundations doc if followed.

---

## Do's & Don'ts

**Do**
- Meet 4.5:1 text / 3:1 component contrast in every built-in theme.
- Keep hit areas ≥24px with ≥8px gaps.
- Make all functionality keyboard-reachable; one Tab stop per composite widget + arrows within.
- Draw a 2px, 3:1 focus ring during keyboard nav; restore focus after modals.
- Honor reduced-motion; pair color with icon/text everywhere.
- Support a global UI scale.

**Don't**
- Encode meaning in hue alone or hide affordances behind hover only.
- Trap focus (except modals, which Esc exits).
- Suppress the focus indicator "because it's ugly".
- Animate large surfaces without a reduced-motion fallback.
- Design anonymous widgets with no accessible name.
- Assume mouse; every drag needs a click/keyboard path (WCAG 2.5.7).

## Common Pitfalls

1. **Gray-on-gray secondary text** failing 4.5:1 in dark themes.
2. **Tab-stop explosion** — every tree row a Tab stop, making keyboard traversal useless (use roving focus).
3. **Focus lost after dialog close** — dumped to nowhere.
4. **Invisible canvas** — shipping v1 with an API that can't ever grow semantics.
5. **1px splitters and 16px icons with 16px hit areas.**
6. **Red/green-only status dots.**

---

## Applying It in elimgui (immediate-mode / desktop GUI)

- **Nav system first-class** (`eli_input.h`): a focus id + roving-focus groups. `eli_begin_group()`-scoped widgets share one Tab stop; arrows move the focus id within; Tab/Shift+Tab moves between groups/widgets. Trees, radio groups, tab bars, toolbars, and tables all reuse this one mechanism.
- **Focus ring** rendered by the nav layer (2px, primary color, ≥3:1 vs surface, outside the widget rect) only when `nav_active` (keyboard drove the last focus change) — the `:focus-visible` behavior.
- **Hit-rect padding**: every widget registers a hit rect ≥24×24 even when its visual is smaller (16px icons, 4px splitters, tree chevrons). Style-level `min_hit_size` token.
- **Scale factor**: `eli_style_scale(float s)` multiplying font size and all spacing/size tokens — this is both HiDPI support and WCAG 1.4.4 in one feature.
- **Reduced motion**: read `matchMedia('(prefers-reduced-motion: reduce)')` via jsio into `eli_io.reduced_motion`; the animation helpers (motion doc) then swap movement for fades/instants automatically.
- **High-contrast theme**: a third built-in theme (true black/white, 7:1+, thick borders) — cheap because theming is role-based.
- **Semantics-ready core**: widgets already take labels; keep a per-frame record `(id, role, label, rect, state)` behind a flag. A future `eli_a11y.h` backend can mirror that list into hidden DOM nodes via jsio (Flutter's model), including routing DOM focus/click events back into `eli_io`. Even before that ships, the recorded list enables an in-canvas screen-reader-lite and automated UI testing.
- **Contrast tests in CI**: a tiny test that computes contrast ratios of the shipped themes' role pairs (text/surface, border/surface, focus/surface) and fails below 4.5/3.0 — accessibility as a build gate.
