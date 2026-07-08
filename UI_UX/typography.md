# Typography

> In a GUI, text *is* most of the interface. Typography quality is usability quality.

## Overview

UI typography differs from print/editorial typography: sizes are smaller, lines are shorter, text is functional (labels, values, commands), and rendering happens at low pixel budgets. The rules: one font family, a small type scale, hierarchy through size/weight/color, correct line heights, and tabular figures for numbers.

---

## 1. Type Scale

Pick a base size and a ratio; derive everything from it ([Cieden type scales](https://cieden.com/book/sub-atomic/typography/different-type-scale-types)):

| Ratio | Name | Feel | Suits |
|-------|------|------|-------|
| 1.125 | Major second | Subtle | Dense UI / desktop tools ✅ |
| 1.2 | Minor third | Moderate | General UI |
| 1.25 | Major third | Distinct | Marketing/content |
| 1.333+ | Fourth+ | Dramatic | Editorial only |

- **UI tools should use a tight ratio (1.125–1.2)** — you need many usable steps close together, not drama.
- [Material 3](https://m3.material.io/styles/typography/type-scale-tokens) defines 15 styles in 5 roles (display / headline / title / body / label), each Large/Medium/Small, using only weights 400 and 500.
- [Carbon](https://carbondesignsystem.com/elements/typography/type-sets/) splits "productive" (14px base, data-dense) from "expressive" (16px base) sets — desktop tools are the *productive* case.

### A practical desktop-tool scale

```
11px  caption / dense metadata
12px  secondary text, table meta, labels-small
13px  DEFAULT UI text (VS Code-class tools use 13-14px)
14px  body / primary content
16px  section titles
20px  panel/window titles
24px+ page headings (rare in tool UIs)
```

## 2. Hierarchy Without Extra Fonts

Three levers, in priority order ([Toptal/Refactoring UI](https://www.toptal.com/designers/typography/typographic-hierarchy)):

1. **Size** — the most instant signal.
2. **Weight** — 400 body, 600 (semibold) for titles/emphasis. Fluent's entire ramp uses only 400 and 600 ([Fluent typography](https://learn.microsoft.com/en-us/windows/apps/design/signature-experiences/typography)).
3. **Color** — primary text dark/high-contrast; secondary ~60–70% strength; tertiary/disabled lighter.

```
Primary:    16px / 600 / high-emphasis color
Secondary:  14px / 400 / medium-emphasis color
Tertiary:   12px / 400 / low-emphasis color
```

- **De-emphasize with lighter *color* or smaller *size*, never lighter *weight*** — weights <400 fail at small sizes ([font weight guide](https://madegooddesigns.com/font-weight-guide/)).
- Avoid italics in UI (legibility, dyslexia) — Fluent excludes italic from its ramp entirely.
- One font family (plus one monospace). Hierarchy from size/weight/color, not fonts.

## 3. Line Length & Line Height

- **Line length:** 45–75 characters optimal for reading; **50–60 CPL** for UI text ([Fluent](https://learn.microsoft.com/en-us/windows/apps/design/signature-experiences/typography), [UXPin](https://www.uxpin.com/studio/blog/optimal-line-length-for-readability/)). Below ~20 CPL forces choppy eye jumps; above ~90 causes line re-entry errors.
- **Line height by context:**

| Context | Line height |
|---------|-------------|
| Body/paragraph text | 1.4–1.6 |
| UI labels, buttons, single-line controls | 1.2–1.4 |
| Headings | 1.1–1.3 |
| Small text (<14px) | increase toward 1.5 |

Fluent's concrete ramp: 12px→16, 14px→20, 18px→24, 28px→36 ([Windows type ramp](https://learn.microsoft.com/en-us/windows/apps/design/signature-experiences/typography)).

## 4. Minimum Sizes & Platform Defaults

| Platform | Body default | Minimum | Weights |
|----------|-------------|---------|---------|
| macOS (SF Pro) | 13pt | 11pt | 400/600/700 |
| Windows (Segoe UI Variable) | 14px / 20 line | **12px regular, 14px semibold** | 400/600 |
| Material 3 (Roboto) | 14–16px | 12px | 400/500 |
| Carbon (IBM Plex) | 14px productive | 12px | 400/600 |
| VS Code | 13px editor / ~13px UI | — | — |

Rules of thumb:
- Never render essential UI text below **11px**.
- Never use weight <400 below ~16px.
- Quality degrades fast below 11px — reserve 10–11px for dense metadata only.

## 5. Numbers: Tabular Figures

- Use **tabular (fixed-width) lining figures** for any column of numbers or any live-updating value — proportional digits make columns ragged and labels "wiggle" as values change ([tabular figures](https://www.numberanalytics.com/blog/mastering-tabular-figures-in-typography), [proportional vs monospaced](https://azi.medium.com/proportional-vs-monospaced-numbers-when-to-use-which-one-in-order-to-avoid-wiggling-labels-e31b1c83e4d0)).
- Don't switch to a monospace *font* just to stabilize numbers — use the tabular variant of the UI font if available; digits only need uniform width.
- Right-align numeric columns (see data-display.md).

## 6. Monospace

Use for: code, terminals, hex values, IDs where character disambiguation (0/O, 1/l) matters. Don't use for general body text. Good monos (JetBrains Mono, Fira Code, IBM Plex Mono) are optimized for 12–16px.

## 7. All-Caps & Letter Spacing

- Lowercase body text needs **no tracking adjustment** — the font is already spaced.
- **ALL CAPS and small caps need +5–12% letter spacing** (`0.05–0.12em`); smaller caps text needs more ([Butterick](https://practicaltypography.com/letterspacing.html)).
- Use all-caps only for short labels (section headers, table headers), never sentences.

## 8. Truncation vs Wrapping

- Prefer **wrapping** when possible; truncation hides information and creates accessibility problems ([truncation pitfalls](https://www.jamesjacobs.me/blog/the-accessibility-pitfalls-of-truncating-text/)).
- When truncating: always show an ellipsis "…", always give access to the full text (tooltip on hover, expandable), and never truncate the interactive part of a control's label.
- Middle-truncate file paths (`/Users/…/project/file.c`) — the end is often the informative part.

## 9. Small-Size Rendering (Canvas-Relevant)

- Windows relies on hinting + subpixel AA; macOS ignores hinting and favors glyph shape; high-DPI displays make both moot ([macOS font rendering](https://skip.house/blog/macos-font-rendering)).
- For custom rasterizers: **snap baselines and glyph origins to integer pixels** at 1x, or text shimmers; at small sizes prefer slightly heavier rendering over thin/gray.

---

## Do's & Don'ts

**Do**
- One family + one monospace; hierarchy via size/weight/color.
- Base 13–14px, scale ratio ~1.125–1.2.
- Line height ≥1.4 for multi-line text, tighter for controls/headings.
- Tabular figures for all numeric columns and live values.
- +5–12% tracking on all-caps labels.
- Sentence case for UI text (labels, menu items, buttons).

**Don't**
- Use weights <400 at small sizes, or italic for de-emphasis.
- Render essential text below 11px.
- Let any text line exceed ~75 characters.
- Use placeholder-gray text colors that fail contrast (see accessibility.md).
- Truncate without ellipsis + a way to see the full string.
- Mix several font sizes 1px apart — steps must be visibly distinct.

## Common Pitfalls

1. **Wiggling numbers** — proportional digits in FPS counters, timers, tables.
2. **All-caps without tracking** — reads as cramped and cheap.
3. **Hierarchy by font-zoo** — three families where one family + two weights suffices.
4. **Uniform line height** — same 1.5 applied to buttons, headings, and body alike.
5. **Sub-pixel drift** — non-integer glyph positions causing blurry text in custom renderers.

---

## Applying It in elimgui (immediate-mode / desktop GUI)

- **Font system (`eli_font.h`) should bake a type ramp, not one size.** Load one TTF at 3–5 sizes (e.g. 12/13/16/20) into the atlas up front; expose `eli_push_font(ELI_FONT_BODY | ELI_FONT_TITLE | …)` semantic slots rather than raw pixel sizes so apps inherit a coherent scale.
- **Default UI size 13px** with a 20px default frame height (13px text + vertical frame padding) matches VS Code/Fluent-class density.
- **Bake tabular-width digits.** With stb_truetype, force all glyphs `0–9` to the same advance (max digit advance) in a "numeric" font variant, and use it in sliders, drag-ints, tables, and the FPS/demo overlays — this kills value-wiggle in immediate-mode widgets that re-render every frame.
- **Integer-snap text.** In `eli_draw` text emission, round the baseline origin to whole pixels at 1x DPI; only allow subpixel positioning when DPI scale ≥2.
- **Line-height token in the style**: `text_line_height = 1.4` for wrapped text; single-line widgets center text within the frame instead (vertical centering + `frame_padding`).
- **Weight support**: if only one weight is loaded, synthesize emphasis via color, not faux-bold; if two, use 400/600.
- **Truncation helper**: `eli_text_ellipsis(label, max_width)` clipping with "…" and automatic tooltip on hover for tab titles, tree items, and table cells — build it once in the library so every widget truncates correctly.
- **Letter-spacing param** for the (rare) all-caps section-header widget: +6–8% of em.

### Checklist

- [ ] Type ramp of ≤6 sizes, ratio ~1.125–1.2, base 13–14px
- [ ] No essential text <11px; no weight <400 at small sizes
- [ ] Tabular digits in every numeric widget
- [ ] Integer-pixel baselines at 1x
- [ ] Ellipsis + tooltip for every truncation site
- [ ] Line height 1.4+ for wrapped text; frame-centered for single-line
