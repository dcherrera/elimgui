# Layout & Spacing

> Space is the cheapest and most powerful design material you have. Almost every "this looks amateur" problem is a spacing problem.

## Overview

Professional UIs are built on a small, fixed spacing scale, strong alignment, and deliberate use of whitespace for grouping. Desktop tools additionally demand *density control* — pros want information, not air — without collapsing into clutter. This doc covers grids, the 4/8pt system, alignment, density, whitespace, containment, and resizable-panel behavior.

---

## 1. The Spacing Scale (4pt / 8pt System)

All major systems converge on multiples of a small base unit:

| System | Base | Scale |
|--------|------|-------|
| [Material 3](https://m3.material.io/foundations/layout/understanding-layout/spacing) | 8dp | 8, 16, 24, 32, 40, 48… (+ 4dp half-steps) |
| [Fluent 2](https://fluent2.microsoft.design/layout) | 4px | 2, 4, 6, 8, 10, 12, 16, 20, 24, 28, 32, 40, 48… |
| [IBM Carbon](https://carbondesignsystem.com/elements/spacing/overview/) | 8px "mini unit" | 2, 4, 8, 12, 16, 24, 32, 48, 64… |
| [Atlassian](https://atlassian.design/foundations/spacing) | 8px | space.100=8 … space.1000=80 |
| [Refactoring UI](https://refactoringui.com/) | non-linear | 4, 8, 12, 16, 20, 24, 32, 48, 64… |

**Why 8:** divisible by 2 and 4, avoids fractional pixels at 1x/1.5x/2x DPI scales, and a limited scale kills decision fatigue — you pick from ~10 values instead of infinite ones.

**Refactoring UI's key refinement:** make the scale non-linear so adjacent values differ by ≥ ~25% — 12 vs 13 is an agonizing choice; 12 vs 16 is obvious.

**When to use 4 vs 8:**
- **4px** — inside components: icon↔label gap, checkbox↔text, padding of compact controls.
- **8px+** — between components and groups: control rows, sections, panel padding.

### Recommended working scale for a desktop GUI

```
2  4  6  8  12  16  24  32  48  64
```

## 2. Grids & Composition

- A **12-column grid** is the web/app standard, but desktop tool UIs mostly use a **soft grid**: consistent spacing relationships rather than hard columns ([Fluent 2](https://fluent2.microsoft.design/layout)).
- Practical split: **hard grid for page/panel layout, soft grid (spacing scale) for component internals**.
- Gutters between major regions/panels: **12–16px** for dense pro tools, 16–24px standard ([Ontario DS](https://designsystem.ontario.ca/components/detail/grid.html), [Windows spacing](https://learn.microsoft.com/en-us/windows/apps/design/style/spacing)).
- Window/panel edge margins: 16px is the standard minimum between content and a surface edge.

## 3. Alignment

- **Edge alignment does the heavy lifting.** A shared left edge creates a scannable line (Gestalt continuity). Misalignment of even 1–2px registers as sloppy.
- **Optical alignment beats mathematical alignment** for icons and glyphs: a play triangle must be nudged right of mathematical center to *look* centered; circular icons need to slightly overhang a square's bounds ([optical alignment guide](https://blog.kleinpixelagency.com/perfect-your-designs-with-imperfection-guide-to-optical-alignment-f76999b5d62d)).
- **Form labels** ([SitePoint](https://www.sitepoint.com/definitive-guide-form-label-positioning/)):
  - *Top-aligned*: fastest scan, best for short forms — but vertically expensive.
  - *Left-aligned*: best for dense desktop property panels (ImGui-style label-left is fine); keep a consistent label column width.
  - Keep ≥ 16px between fields; ≤ 8px between a label and its own input (label must sit closer to its field than to the previous field — proximity!).
- **Right-align numbers**, left-align text (see data-display.md).

## 4. Information Density

Desktop pro tools live in the "compact-to-comfortable" band. Provide a density setting where feasible ([and.digital](https://www.and.digital/spotlight/designing-complex-desktop-interfaces), Gmail/Teams density modes):

| Density | List/table row height | Section gaps | Use |
|---------|----------------------|--------------|-----|
| **Compact** | 24–28px | 8–12px | IDEs, data grids, power users |
| **Comfortable (default)** | 32–40px | 16–24px | General desktop UI |
| **Spacious** | 44–56px | 24–32px | Reading-focused, touch |

- Even in compact mode keep interactive hit targets ≥ **24×24px** with ≥ 8px between adjacent targets ([WCAG 2.5.8](https://wcag.dock.codes/documentation/wcag258/)).
- Power users want density, but density ≠ crowding: reduce padding *uniformly via the scale*, don't remove grouping gaps.

## 5. Whitespace — Its Actual Jobs

1. **Grouping** (proximity): intra-group 4–12px, inter-group 24–32px.
2. **Hierarchy**: space *above* a heading should be ~1.5–2× the space *below* it, so the heading binds to its content ([Toptal typography](https://www.toptal.com/designers/typography/typographic-hierarchy)).
3. **Breathing room / perceived quality**: whitespace reads as polish and reduces cognitive load.

**Method ([Refactoring UI](https://refactoring-ui.nyc3.cdn.digitaloceanspaces.com/Refactoring%20UI%20-%20Start%20with%20too%20much%20white%20space.pdf)):** start with *too much* space and remove until it looks right. Starting cramped and adding never gets there.

## 6. Containment: Borders vs Space vs Background

Refactoring UI's ordering for separating regions ([source](https://medium.com/refactoring-ui/7-practical-tips-for-cheating-at-design-40c736799886)):

1. **Whitespace** — if distance alone separates it, you're done.
2. **Background shift** — a subtly different fill distinguishes a panel without a line.
3. **Subtle shadow** — lifts a surface (e.g. `0 1px 3px rgba(0,0,0,0.1)`).
4. **Border** — last resort; border-everything looks dated and claustrophobic.

**The inner ≤ outer rule ([EightShapes](https://medium.com/eightshapes-llc/space-in-design-systems-188bcbae0d62)):** a container's internal padding must be ≤ the margin around it. A card with 20px padding needs ≥ 20px (ideally 24px) between cards, otherwise the contents of adjacent cards read as closer to each other than to their own container.

- Standard panel/card padding: **12–16px** dense, 16–20px comfortable. Avoid uniform 32px+ padding — hollow and wasteful.

## 7. Resizable Panels (Desktop Specific)

- Use **proportional sizing** (fractions/weights) for split panes, not fixed pixels, so layouts survive window resizes.
- Enforce **minimum panel widths ~240–320px** for content panels (less for tool strips); never let a pane collapse to 0 accidentally.
- Splitter handles: **4–8px visible**, with a hover/hit zone of 8–12px.
- **Persist** user-set panel sizes across sessions.

---

## Do's & Don'ts

**Do**
- Pick one spacing scale and use *only* those values.
- Start with too much whitespace, then reduce.
- Keep labels closer to their own field than to neighbors.
- Use background shifts and spacing before borders.
- Offer/at least design for a density toggle in data-heavy views.
- Right-pad hit targets even when visuals are compact.

**Don't**
- Use ad-hoc values (13px, 22px, 31px) — no rhythm, looks unpolished.
- Give every card huge uniform padding.
- Separate everything with 1px borders.
- Let inner padding exceed outer margins.
- Compress *grouping* gaps when densifying — compress *within-group* padding first.

## Common Pitfalls

1. **Equal spacing everywhere** — if the gap between groups equals the gap within groups, grouping vanishes.
2. **Border soup** — outlines on every widget; modern UIs separate with space and fill.
3. **Mathematical centering of icons** that looks off-center (fix optically).
4. **Density by deletion** — removing whitespace uniformly instead of stepping down the scale.
5. **Splitters that are 1–2px wide** and ungrabbable.

---

## Applying It in elimgui (immediate-mode / desktop GUI)

- **Encode the scale in `eli_style`**: define `item_spacing` (8), `item_inner_spacing` (4), `window_padding` (12–16), `frame_padding` (8×4 to 8×6), `group_spacing` (24) — all from the `2/4/8/12/16/24/32/48` scale. Every widget consumes these; no widget hardcodes its own gaps.
- **Ship density presets**: `eli_style_compact()`, `eli_style_default()`, `eli_style_spacious()` that scale row heights (24 / 32 / 44) and paddings coherently — one call, whole app changes density. This is a differentiator versus Dear ImGui, which requires manual style surgery.
- **Layout cursor discipline**: the immediate-mode layout cursor (`same_line`, vertical advance) should advance by `item_spacing` by default; provide `eli_spacing()` / `eli_group_gap()` helpers that insert *scale* values, discouraging magic-number `eli_dummy(vec2(13,7))` hacks.
- **Group primitive**: an `eli_begin_group()/eli_end_group()` plus optional `eli_separator_text("Section")` gives users proximity-based grouping; document 24px+ as the between-groups default.
- **Panels over borders**: default child windows/panels to a background-shift fill (slightly lighter/darker than the window) with *no* border; border opt-in via flag. This alone modernizes the look versus classic ImGui border-everything.
- **Label-left forms**: for property-panel style widgets (`eli_slider_float("Speed", …)`), reserve a consistent label column (e.g. 40% of width, like ImGui) so all field edges align — alignment is what makes dense forms readable.
- **Splitters**: 4px visual, 8–12px hit zone, min-size clamps on both panes, cursor change on hover.
- **Optical fixes in the draw layer**: `eli_draw` icon helpers should center glyphs optically (triangles/plays nudged toward visual mass), and text baselines should align across mixed controls on a row (`eli_align_text_to_frame_padding` equivalent).

### Quick reference

| Token | Suggested default |
|-------|-------------------|
| frame padding (inside buttons/inputs) | 8px horizontal, 4–6px vertical |
| item spacing (between widgets) | 8px |
| inner spacing (icon↔label) | 4px |
| window/panel padding | 12–16px |
| group/section gap | 24px |
| splitter visible / hit | 4px / 8–12px |
| min row height (comfortable) | 28–32px |
| min hit target | 24×24px |
