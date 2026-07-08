# Icons & Imagery

> Only about three icons on Earth are universally understood. Everything else needs a label, a tooltip, or both.

## Overview

Icons compress meaning into ~16–24px — when they work. NN/g's research is blunt: unlabeled icons routinely fail recognition, and the icons that don't (home, search/magnifier, print) are the rare exceptions ([NN/g icon usability](https://www.nngroup.com/articles/icon-usability/)). The craft is: established metaphors, one consistent family, exact grid sizes, generous hit targets, and labels wherever stakes are non-trivial.

---

## 1. Purpose & the Ambiguity Problem

- **Icon+label beats icon-only** for findability and confidence in essentially all testing; icon-only saves space at a real comprehension cost.
- **The 5-second rule**: if you can't think of an obvious icon for a concept in 5 seconds, an icon won't communicate it — use text ([NN/g](https://www.nngroup.com/articles/icon-usability/)).
- Icon-only is acceptable for: universally recognized symbols in conventional positions (search in a toolbar, × close, gear settings) — **and even then, always with a tooltip** and an accessible name.

## 2. Metaphors

Use the established vocabulary; never reinvent it:

| Icon | Meaning |
|------|---------|
| 💾 floppy | Save (anachronistic, still the most recognized) |
| ⚙ gear | Settings |
| 🔍 magnifier | Search |
| ▽ funnel | Filter |
| 🗑 trash | Delete |
| ⋮ / ⋯ kebab/meatball | More actions (item-level) |
| ☰ hamburger | Global nav menu — nothing else |
| + | Add/create |
| ✎ pencil | Edit |
| ▸/▾ chevrons | Expand/collapse |

Repurposing any of these (e.g. ⋮ to expand text) actively harms users ([NN/g contextual menus](https://www.nngroup.com/articles/contextual-menus-guidelines/)). Prefer concrete objects over abstract concepts; test anything novel.

## 3. Consistency — One Family

- **One style**: outline *or* filled as the base; mixing reads as broken. A useful convention: **outline = inactive, filled = active/selected** (tab bars).
- **Stroke width**: **2px at 24px**, ~1.5px at 16px — constant visual weight across the set ([Material system icons](https://m2.material.io/design/iconography/system-icons.html)).
- Consistent corner radius, consistent perspective (flat, front-facing), consistent optical density.
- **Pixel-snap**: align strokes to the pixel grid at target sizes or small icons blur ([pixel snapping](https://uxdesign.cc/pixel-snapping-in-icon-design-a-rendering-test-6ecd5b516522)).

## 4. Sizing

- **Canonical sizes: 16, 20, 24, 32 (48, 64)px** — design on a grid (Material: 24dp canvas, 20dp live area, 2dp padding). Don't scale to arbitrary sizes like 18 or 26px at 1x.
- **Icon ≠ hit target**: a 16–24px icon sits inside a **≥24px (AA) / 32–40px (comfortable desktop)** hit area; Material demands 48dp on touch ([WCAG target size](https://www.w3.org/WAI/WCAG21/Understanding/target-size.html)).
- Optical centering: visually center glyphs (triangles, arrows) rather than bounding-box centering.

## 5. Labels & Tooltips

- Toolbar/primary actions: prefer icon+label (or a user-switchable icon/label mode, the classic pro-tool option).
- Icon-only buttons: mandatory tooltip (name + shortcut) and accessible name.
- Label below (toolbars) or right (menus, lists); never hover-only labels for essential actions.

## 6. Color

- UI icons are **monochrome**, inheriting the text color of their context (primary/secondary/disabled) — this keeps them legible across themes and states for free.
- Semantic color sparingly: error ✕ red, warning ⚠ amber, success ✓ green — with text nearby.
- Multicolor icons hurt scannability in dense UIs; reserve for branding/illustration.
- Icon contrast vs background: ≥3:1 (meaningful graphics, WCAG 1.4.11).

## 7. Imagery & Illustration in Pro Tools

- Keep decoration near zero; the data is the interface.
- Legit uses: **empty states** (small, relevant illustration + text + CTA), onboarding, error pages.
- Never let illustration compete with content or add noise to working screens ([Carbon](https://carbondesignsystem.com/elements/icons/usage/)).

## 8. Rendering Strategy (Canvas/Vector)

| Approach | Verdict |
|----------|---------|
| **Baked glyph atlas** (rasterize at exact sizes at load) | ✅ Fast, crisp, matches font-atlas infrastructure |
| Icon font (bake into the existing text atlas) | ✅ Simple mono icons; ships with the font pipeline |
| SDF/MSDF atlas | For arbitrary scaling; slightly soft at tiny sizes |
| Runtime SVG parsing | ❌ Avoid per-frame; fine as an offline bake input |

Bake each icon at each canonical size (16/20/24) rather than scaling one raster — scaling is what makes icons blurry.

## 9. Testing Recognizability

- Show icons out of context: "what is this / what does it do?" — target ≥80% recognition for common actions; anything below gets a label.
- First-click tests in context; 5–10 users expose most problems ([NN/g icon testing](https://www.nngroup.com/articles/icon-testing/)).

---

## Do's & Don'ts

**Do**
- One family, one stroke weight, canonical grid sizes, pixel-snapped.
- Label actions; tooltip everything icon-only (name + shortcut).
- Monochrome icons driven by text color roles.
- Hit areas ≥24px regardless of glyph size.
- Use filled-vs-outline for selected states.
- Test recognition before shipping novel icons.

**Don't**
- Invent new metaphors for standard actions.
- Repurpose ☰ / ⋮ / established symbols.
- Mix outline and filled styles arbitrarily.
- Scale icons to off-grid sizes.
- Ship icon-only toolbars without tooltips.
- Add decorative imagery to working screens.

## Common Pitfalls

1. **Mystery-meat toolbars** — 12 unlabeled 16px glyphs.
2. **Blurry icons** — one 24px master scaled to 16px unsnapped.
3. **Frankenstein sets** — icons harvested from three libraries with different weights.
4. **16px icons with 16px hit areas** next to each other.
5. **Color-carried meaning** — a red dot with no shape/text distinction.
6. **Clever novel metaphors** failing the 5-second rule.

---

## Applying It in elimgui (immediate-mode / desktop GUI)

- **Bake icons into the font atlas** (`eli_font.h`): treat icons as glyphs — either merge an icon font (e.g. a Fluent/Material-style set) into the atlas at load, or rasterize a curated SVG set offline into the same texture at 16/20/24px. Expose codepoints as constants: `ELI_ICON_SAVE`, `ELI_ICON_GEAR`, `ELI_ICON_CHEVRON_RIGHT`…
- **Curate a small standard set (~40–80 icons)** covering the established vocabulary (file ops, edit ops, chevrons, close, search, settings, warning/error/info/success, dock/window controls). A consistent bundled set prevents every elimgui app from shipping Frankenstein icons.
- **API affordances**: `eli_button_icon(ELI_ICON_X, "Close")` — accessible label required even when rendering icon-only, auto-tooltip from the label + registered shortcut. `eli_icon(id, size)` for inline/decorative use inherits the current text color (so state/disabled tinting is automatic).
- **Hit-area enforcement**: icon buttons get `max(icon_size + 2*frame_padding, min_hit_size)` — a 16px glyph still yields a ≥24–28px button.
- **Snap icon origins to integer pixels** at 1x, same as text (typography.md).
- **State convention**: chevron rotation for expand state; outline→filled (or weight/color shift) for selected tabs and toggled tool buttons — pick one and apply library-wide.
- **Empty-state imagery**: keep it glyph-scale — a 32–48px icon in `TEXT_DISABLED` color above the message (see feedback-and-states.md), not a cartoon.
- **Semantic icon+color pairs** baked into helpers: `eli_text_error()` prefixes ✕, warnings ⚠, success ✓ — meaning never rides on hue alone.
