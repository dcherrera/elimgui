# Color & Theming

> A UI needs surprisingly few hues and surprisingly many shades. Color's main jobs are hierarchy, state, and meaning — decoration comes last.

## Overview

Professional UI palettes are: a large family of neutrals (8–10 shades), one primary, a handful of semantic colors — each defined up-front as fixed shades and consumed through *semantic roles* (tokens), never as raw hex values in widget code. Dark mode is a first-class theme built on dark gray (not black), desaturated accents, and lightness-as-elevation.

---

## 1. Practical Color Theory for UI

- Think in **HSL/HSB**, not RGB: hold hue constant, vary saturation/lightness to create tints, shades and states predictably ([HSL vs RGB](https://madegooddesigns.com/hsl-vs-rgb/)).
- **HSL lightness is not perceptual** — yellow at L=50 looks far lighter than blue at L=50. Serious palette tools use OKLCH/perceptual spaces ([Accessible Palette](https://www.wildbit.com/blog/accessible-palette-stop-using-hsl-for-color-systems.html)). For hand-tuned palettes: trust your eyes over the numbers.
- Avoid pure black `#000` and pure white `#fff` for large areas — both are harsh; near-black/near-white read better ([UX Movement](https://uxmovement.com/content/why-you-should-never-use-pure-black-for-text-or-backgrounds/)).

## 2. Building the Palette ([Refactoring UI](https://refactoringui.com/previews/building-your-color-palette))

| Group | Count | Used for |
|-------|-------|----------|
| **Neutrals/grays** | 8–10 shades | Backgrounds, surfaces, borders, text — ~90% of the UI |
| **Primary** | 5–9 shades | Brand/action color: buttons, selection, focus, links |
| **Semantic** | 5–9 shades each | Red (error/destructive), amber (warning), green (success), blue (info) |

Rules:
- Define shades **up front**; don't compute them at runtime with naive `lighten()`/`darken()`.
- Pick the middle shade first (the one that works as a button background), then fill out lighter/darker steps.
- Tune by eye in real components (an alert background tells you if `red-100` is right).

## 3. Semantic Roles / Tokens

All major systems separate *what a color is* from *what it's for* ([Material 3 roles](https://m3.material.io/styles/color/roles), [Fluent alias tokens](https://fluent2.microsoft.design/color-tokens/), [Carbon](https://carbondesignsystem.com/elements/color/overview/)):

```
background      → app/window backdrop
surface         → panels, cards, popups (often layered: surface-1..3)
on-surface/text → primary, secondary, disabled text
primary         → accent actions, selection, active states
border/outline  → dividers, input outlines
error/warning/success/info → semantic states
```

Theming = remapping the role table. Widget code references roles only.

**Conventions:** red = error/destructive, amber = warning, green = success, blue = info/selection. Never repurpose these. And **never use color as the only carrier of meaning** — pair with icon/text ([NN/g](https://www.nngroup.com/articles/visual-treatments-accessibility/)); do the grayscale test.

**60-30-10:** roughly 60% neutral, 30% secondary/surface variation, 10% accent — accent color must stay scarce to keep pointing at what matters ([60-30-10](https://hype4.academy/articles/design/60-30-10-rule-in-ui)).

## 4. Dark Mode ([Material dark theme](https://m3.material.io/blog/android-dark-theme-tutorial))

| Element | Value |
|---------|-------|
| Base background | **#121212** (dark gray, not #000) |
| Elevated surfaces | lighter overlays: +5% white ≈ #1E1E1E (elev 1), +7% ≈ #242424 (elev 2), up to ~+16% |
| High-emphasis text | white at **87%** opacity |
| Medium-emphasis text | white at **60%** |
| Disabled text | white at **38%** |
| Accents | **desaturate** brand colors (use the lighter shades of the ramp, e.g. primary-200 instead of primary-600) |

Why: pure black kills shadows and depth; pure white text halates/vibrates; saturated accents scream against dark fields. In dark mode, **elevation = lighter surface** (shadows are nearly invisible on dark backgrounds).

## 5. Elevation & Shadows

- Light theme: shadow = elevation. Use a small shadow ramp (e.g. `0 1px 2px`, `0 2px 8px`, `0 8px 24px`, all rgba(0,0,0,0.10–0.25)); softer + larger = higher ([Material elevation](https://m1.material.io/material-design/elevation-shadows.html)).
- Dark theme: surface-lightening replaces (or supplements) shadows.
- Typical elevation order in a tool UI: window background < panel < window/card < menu/popup < tooltip < drag preview.

## 6. States via Color: State Layers

Material's model — overlay the *content color* on the component at fixed opacities ([M3 states](https://m3.material.io/foundations/interaction/states/state-layers)):

| State | Overlay opacity |
|-------|-----------------|
| Hover | **8%** |
| Focus | **12%** |
| Pressed | **12%** (or 10–16%) |
| Dragged | **16%** |
| Disabled content | **38%** of normal |

This gives every widget consistent, theme-independent state feedback from one rule.

## 7. Contrast (see accessibility.md for full detail)

- Normal text: **4.5:1** minimum (WCAG AA); large text: 3:1; UI component boundaries/icons: **3:1** ([WCAG 1.4.3/1.4.11](https://www.w3.org/WAI/WCAG21/Understanding/contrast-minimum.html)).
- Check the *secondary/disabled* grays especially — that's where most violations live.

---

## Do's & Don'ts

**Do**
- Define ~10 neutrals + shaded primary + shaded semantics, once.
- Route every widget through semantic roles/tokens.
- Use #121212-class dark gray and 87/60/38% text in dark mode.
- Use one state-layer rule (8/12/12/38) across all widgets.
- Desaturate accents in dark themes.
- Keep accent color to ~10% of pixels.

**Don't**
- Hardcode hex values inside widget drawing code.
- Use pure black/white surfaces or 100% white text on dark.
- Convey error/success by hue alone.
- Use more than 2–3 hues plus neutrals ([common mistakes](https://supercharge.design/blog/8-common-ui-color-mistakes)).
- Compute state colors ad hoc per widget.
- Let borders carry all separation (prefer surface shifts — see layout-and-spacing.md).

## Common Pitfalls

1. **Gray soup** — too few neutrals, so designers reuse the wrong shade and hierarchy flattens.
2. **Dark mode by inversion** — literally inverting the light palette instead of rebuilding on #121212 + overlays.
3. **Vibrating accents** — saturated brand color unchanged on dark surfaces.
4. **Meaningless color** — blue used both for interactive elements and decoration, destroying the "blue = clickable" signal.
5. **Contrast failure in "subtle" text** — placeholder/disabled/secondary grays below 4.5:1 on their backgrounds.

---

## Applying It in elimgui (immediate-mode / desktop GUI)

- **Model the theme as a role-indexed color table**: `eli_style.colors[ELI_COL_WINDOW_BG]`, `ELI_COL_SURFACE`, `ELI_COL_TEXT`, `ELI_COL_TEXT_SECONDARY`, `ELI_COL_TEXT_DISABLED`, `ELI_COL_PRIMARY`, `ELI_COL_BORDER`, `ELI_COL_ERROR`, etc. This is Dear ImGui's `ImGuiCol_` idea, but organize by *role* (surface/on-surface/primary) rather than per-widget entries where possible — fewer entries, more consistency.
- **Derive widget state colors, don't store them.** Instead of separate `ButtonHovered`/`ButtonActive` entries, compute `base + white/black overlay at 8%/12%` in one helper (`eli_state_color(col, state)`), matching the Material state-layer model. Fewer theme knobs, uniform feedback, and custom themes stay correct automatically.
- **Ship two built-in themes**: dark (default for a dev-tool library: #121212 bg, #1E1E1E panels, #2A2A2A popups, 87/60/38% white text, desaturated accent) and light (near-white bg #FAFAFA, white surfaces, near-black text #1F1F1F).
- **Alpha-composited overlays in `eli_draw`**: `ELI_COL32` colors with alpha + a draw helper `eli_draw_rect_overlay(rect, white, 0.08f)` makes state layers one line per widget.
- **Elevation ramp**: constants `ELI_SHADOW_SM/MD/LG` (draw as blurred/expanded rect layers or pre-baked 9-slice); in dark theme substitute surface lightening (popup bg lighter than window bg by ~7% white).
- **Selection color = primary at low alpha** (e.g. primary @ 25–35%) for text selection, selected rows, and selected tree nodes — one rule everywhere.
- **Contrast-check your default theme**: verify text roles hit 4.5:1 and border/focus roles hit 3:1 against their surfaces before shipping; document the ratios in the theme header.

### Suggested role table starter (dark)

| Role | Value |
|------|-------|
| window bg | #121212 |
| panel/surface | #1E1E1E |
| popup/menu | #2A2A2A |
| text | rgba(255,255,255,0.87) |
| text secondary | rgba(255,255,255,0.60) |
| text disabled | rgba(255,255,255,0.38) |
| border | rgba(255,255,255,0.12) |
| primary | desaturated blue ≈ #82AAFF |
| error / warning / success | #F28B82 / #FDD663 / #81C995 |
