# Motion & Microinteractions

> In a pro tool, animation is seasoning: a little clarifies, a lot nauseates. Every millisecond of motion must earn its keep.

## Overview

Motion in productivity UIs exists for four reasons only: **feedback** (your click registered), **spatial continuity** (where did that panel go), **attention** (look here), and **perceived performance** (smoothing a wait). Desktop tools run *faster and quieter* than mobile — think 100–200ms, ease-out, and "no animation" as a frequent correct answer. ([NN/g animation purpose](https://www.nngroup.com/articles/animation-purpose-ux/), [Material motion](https://m1.material.io/motion/duration-easing.html), [Carbon motion](https://carbondesignsystem.com/elements/motion/overview/))

---

## 1. Durations

| Context | Duration | Notes |
|---------|----------|-------|
| State feedback (hover, press, check) | **100–150ms** | Below ~80ms reads as broken/instant; that's fine for press-down |
| Standard transitions (tab indicator, small reveals) | **150–200ms** | The desktop sweet spot |
| Panel expand/collapse, popups, modals | **200–250ms** open / ~150ms close | Exits faster than entrances |
| Large transitions (rare in tools) | 300ms max | >400–500ms feels sluggish ([Val Head](https://valhead.com/2016/05/05/how-fast-should-your-ui-animations-be/), [NN/g durations](https://www.nngroup.com/articles/animation-duration/)) |
| Desktop vs mobile | ~30% shorter on desktop | Material guidance |

## 2. Easing

| Curve | cubic-bezier | Use |
|-------|--------------|-----|
| **Standard** | (0.4, 0, 0.2, 1) | Default for on-screen movement |
| **Decelerate (ease-out)** | (0, 0, 0.2, 1) | **Entering** elements — arrive fast, settle gently |
| **Accelerate (ease-in)** | (0.4, 0, 1, 1) | **Exiting** elements — leave without lingering |

- **Never linear** for spatial movement (mechanical); linear is fine for opacity-only fades and progress.
- Rule of thumb: **enter = ease-out, exit = ease-in, move = standard**.

## 3. What to Animate (and What Not To)

**Animate:**
- Hover/press color transitions (~100–150ms) — softens state flicker.
- Chevron rotation on expand (~150ms) — cheap spatial cue.
- Panel/tree expand-collapse (~200ms) — preserves "where things went".
- Popup/modal entrance: fade + slight scale (0.95→1) or 4–8px slide, ~150–200ms; exit faster (~100–150ms) or instant.
- Toast slide/fade in-out.
- Drag ghosts and drop-settle (~100–200ms), scroll momentum (physics).
- Progress and skeleton pulses.

**Don't animate (in a pro tool):**
- **Tab/document switching** — instant. Users switch hundreds of times; any delay compounds ([tabs UX](https://www.eleken.co/blog-posts/tabs-ux)). Animate the *indicator*, not the content.
- Menu opening beyond a ~100ms fade — menus are high-frequency.
- Typing/caret/selection — always immediate.
- Value changes from dragging — the value *is* the feedback.
- Anything the expert user repeats constantly — instant is a feature. IDE/pro-tool convention is minimal motion.

## 4. Microinteraction Anatomy (Saffer)

**Trigger → Rules → Feedback → Loops/Modes.** Design each deliberately: what starts it, what happens, how the user knows, and what changes on repeat. The classics: button press (down-state same frame + 100ms release transition), checkbox check (150ms mark draw/scale), collapse chevron rotate, drop settle.

## 5. Choreography

- Stagger list/panel entrances by **20–50ms per item**, first ~10–15 items only, total ≤400ms ([stagger guidance](https://motion.dev/docs/stagger)). Rarely needed in tool UIs — reserve for first-load or empty→populated moments.
- One thing animating at a time is usually right; two, tops.

## 6. Restraint, Reduced Motion, Performance

- Offer a global animation toggle/scale; experts will thank you.
- **`prefers-reduced-motion`**: replace movement with fades or instant switches; keep essential feedback (focus, progress) ([WCAG C39](https://www.w3.org/WAI/WCAG21/Techniques/css/C39)).
- **60fps or don't**: a stuttering animation is worse than none. Web rule "animate transform/opacity only" translates for a canvas renderer to: animate cheap uniforms (offsets, alpha, sizes), never per-frame layout thrash or reallocation; and drive everything from real delta-time so speed is framerate-independent.

---

## Do's & Don'ts

**Do**
- 100–200ms, ease-out in / ease-in out, as the whole vocabulary.
- Press feedback the same frame; transitions on release.
- Animate spatial changes users would otherwise lose track of.
- Respect reduced-motion; expose an animation-off switch.
- Drive by delta-time; interpolate toward targets.

**Don't**
- Animate tab switches, menus (beyond a whisper), or repeated expert actions.
- Exceed ~300ms for anything in a tool UI.
- Use linear easing for movement, or bounce/overshoot in productivity contexts.
- Stagger long lists.
- Let animation block input — the UI must accept the next action mid-transition.

## Common Pitfalls

1. **Sluggish-feeling app** — 300–400ms on high-frequency interactions.
2. **Ease-in entrances** — elements that "arrive late".
3. **Animation-gated input** — clicks ignored until the panel finishes sliding.
4. **Framerate-coupled motion** — speeds doubling at 120Hz.
5. **Decorative motion creep** — every release adds one more wiggle.
6. **No reduced-motion path.**

---

## Applying It in elimgui (immediate-mode / desktop GUI)

Immediate mode makes animation *state*, not timelines — embrace lerp-toward-target:

- **Core helper**: `eli_anim(id, target, speed)` — the context stores `current` per id and moves it toward `target` by `rate * dt` (exponential smoothing) or a fixed-duration ease. Widgets call it inline: `float t = eli_anim(id, hovered ? 1.0f : 0.0f, 0.15f);` then blend colors by `t`. No retained timeline system needed.
- **Style tokens**: `anim_fast = 0.10s` (state colors), `anim_normal = 0.15–0.20s` (chevrons, popups), `anim_slow = 0.25s` (panel collapse); plus a global `anim_scale` (0 disables everything — the reduced-motion/off switch, auto-set from `prefers-reduced-motion` via jsio).
- **Where elimgui should animate by default**: hover/active color blends (100–150ms), tree/collapsing-header arrow rotation + content height (150–200ms, clip during transition), popup/tooltip fade-in (100–150ms; close instant), toast slide+fade, drag-ghost fade-out on cancel, scrollbar-grab highlight, smooth programmatic scrolling (~200ms ease-out).
- **Where it must not**: tab switches (instant content, animate only the underline/indicator), menu item hover (instant fill), text editing, slider/drag value application, window focus changes.
- **Input never waits**: animations are cosmetic interpolations of already-committed state — the logical state flips instantly, only pixels catch up. This is the immediate-mode superpower; preserve it.
- **Delta time**: `eli_io.delta_time` feeds all lerps; clamp dt (e.g. ≤100ms) so background-tab hitches don't teleport animations.
- **Height animation cheaply**: for collapse/expand, animate a 0→1 factor, multiply the child region's clip height, and offset content — no per-frame relayout of hidden items (pairs with the clipper).
- **Easing library**: `eli_ease_out_cubic`, `eli_ease_in_cubic`, `eli_ease_standard` — three functions cover this entire document.
