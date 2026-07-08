# Foundations of UI/UX

> The bedrock principles every other document in this reference builds on. If you internalize nothing else, internalize this file.

## Overview

Good UI is not taste — it is applied psychology. Users have limited working memory, predictable motor behavior, and expectations formed by every other app they use. The principles below (Nielsen's heuristics, Gestalt perception, Fitts's/Hick's Laws, cognitive load, response-time limits) are measurable, testable rules that explain *why* one design works and another fails.

---

## 1. Nielsen's 10 Usability Heuristics

The canonical evaluation checklist, from [Nielsen Norman Group](https://www.nngroup.com/articles/ten-usability-heuristics/):

| # | Heuristic | One-line meaning | Classic failure |
|---|-----------|------------------|-----------------|
| 1 | **Visibility of system status** | Always show what's happening, promptly | Silent operation > 1s with no feedback |
| 2 | **Match system & real world** | Speak the user's language; follow real-world conventions | Internal jargon, inverted mappings |
| 3 | **User control & freedom** | Clearly marked exits: undo, redo, cancel | Irreversible actions with no undo |
| 4 | **Consistency & standards** | Same thing looks/behaves the same everywhere | Two dropdowns that behave differently |
| 5 | **Error prevention** | Constraints and safe defaults beat error messages | Letting users type an invalid value at all |
| 6 | **Recognition over recall** | Make options visible; don't force memorization | Features reachable only by memorized shortcut |
| 7 | **Flexibility & efficiency** | Accelerators for experts, simplicity for novices | One slow path forced on everyone |
| 8 | **Aesthetic & minimalist design** | Every extra element competes with the relevant ones | "Just in case" options cluttering the primary view |
| 9 | **Help users recover from errors** | Plain-language, specific, constructive error messages | `Error 0x80004005` |
| 10 | **Help & documentation** | Contextual, task-focused, searchable | Burying help in a manual nobody opens |

**How to use them:** walk each screen of your app against this table. Any cell you can't defend is a defect.

---

## 2. Gestalt Principles — How Users Perceive Groups

Users group elements *before* they read them ([NN/g](https://www.nngroup.com/videos/the-gestalt-principles-intro/), [IxDF](https://www.interaction-design.org/literature/topics/gestalt-principles)):

| Principle | Rule | UI application |
|-----------|------|----------------|
| **Proximity** | Close = related | Tight spacing (4–8px) within a group; large gaps (24px+) between groups. Proximity beats color and shape as a grouping cue ([NN/g](https://www.nngroup.com/articles/gestalt-proximity/)) |
| **Similarity** | Same look = same function | All buttons styled alike; never style static text like a control |
| **Common region** | Same container = one group | Panels, cards, background shifts group content even at uniform spacing |
| **Continuity** | The eye follows lines | Align edges; a shared left edge creates a scannable column |
| **Closure** | We complete partial shapes | Partial borders / cut-off list rows imply "more content below" |
| **Figure-ground** | Foreground pops off background | Modals over scrims; contrast makes interactive elements "pop" |

**Practical rule:** spacing is your primary grouping tool, containers second, color last.

---

## 3. Visual Hierarchy

Organize elements so the eye lands on them in order of importance ([NN/g](https://www.nngroup.com/articles/visual-hierarchy-ux-definition/)).

- **Size** — max ~3 size levels per view; the most important element is the biggest ([NN/g](https://www.nngroup.com/articles/principles-visual-design/)).
- **Contrast/color** — importance comes from contrast against background, not the hue itself. Limit to ~2 primary + 2 secondary colors.
- **Spacing** — breathing room around an element emphasizes it.
- **Placement** — top-left and center get attention first (F/Z scan patterns). Primary actions go in prime positions.
- **Weight** — bold for key terms; never rely on many font families.

**Test:** the *squint test*. Blur the screen; if the most important thing doesn't still dominate, the hierarchy is broken.

---

## 4. Affordances vs Signifiers (Norman)

- **Affordance** = what an object actually allows (a button affords clicking).
- **Signifier** = the perceivable cue that communicates it (shadow, border, color). "Signifiers are of far more importance to designers than affordances" ([IxDF](https://ixdf.org/literature/topics/affordances)).

| Type | Example | Consequence |
|------|---------|-------------|
| Perceptible | Raised button with border/shadow | Instantly clickable |
| Hidden | Hover-only controls | Discovered by accident |
| False | Underlined non-link, box that looks like an input | Frustration, misclicks |

One NN/g-cited study saw **clicks increase 416%** after moving from fully flat to subtly dimensional buttons ([NN/g](https://www.nngroup.com/articles/clickable-elements/)). Flat design is fine — *invisible* design is not. Every interactive element needs at least one static signifier (not just a hover state).

---

## 5. Fitts's Law — Pointing Cost

**MT = a + b · log₂(2D/W)** — time to hit a target grows with distance (D) and shrinks with width (W) ([NN/g](https://www.nngroup.com/articles/fitts-law/), [Laws of UX](https://lawsofux.com/fittss-law/)).

Practical consequences:

- **Bigger targets are faster and less error-prone.** Desktop mouse minimum ≈ 24×24px hit area (WCAG AA), comfortable 28–40px; touch 44–48px.
- **The hit area can exceed the visual.** Pad clickable regions (icon 16px, hit area 28px+).
- **Screen edges/corners are "infinite" targets** for mouse users — the cursor can't overshoot. macOS menu bar and the Windows Start corner exploit this. (Does *not* apply to touch.)
- **Put things near where the user already is.** Context menus at the cursor; confirmation buttons near the triggering control.
- **Frequency ordering:** most-used menu items at the top of linear menus.

## 6. Hick's Law — Decision Cost

**RT = a + b · log₂(n)** — decision time grows with the number of choices ([Laws of UX](https://lawsofux.com/hicks-law/)).

- Keep top-level menus and toolbars to ~5–9 meaningful items; group the rest.
- Provide defaults and highlight the recommended choice.
- Use progressive disclosure instead of showing 20 equal options.
- Caveat: for *experts scanning a known list*, more visible options can be faster than nested menus — Hick's Law argues for *organized* choice, not hidden choice.

---

## 7. Cognitive Load & Chunking

Three kinds of load ([Laws of UX](https://lawsofux.com/cognitive-load/)):

- **Intrinsic** — task difficulty itself. Manage with progressive disclosure and good defaults.
- **Extraneous** — effort wasted parsing the UI (clutter, inconsistency, decoration). *Eliminate this.*
- **Germane** — productive effort understanding the content. *Protect this.*

Working memory holds **~4–7 chunks**, each fading in **20–30 seconds** ([Miller / Laws of UX](https://lawsofux.com/chunking/), [Working Memory](https://lawsofux.com/working-memory/)). Consequences:

- Don't require users to carry information across screens — repeat it (labels, summaries, breadcrumbs).
- **Chunk** everything: group form fields by category, split long lists into labeled sections, format numbers (`(555) 123-4567` is 1 chunk; `5551234567` is 10).
- Cap any one visual group at ~5–7 items.

---

## 8. Feedback & Response-Time Limits

The three canonical thresholds ([NN/g](https://www.nngroup.com/articles/response-times-3-important-limits/)):

| Threshold | Perception | Required feedback |
|-----------|-----------|-------------------|
| **≤ 0.1 s** | Instantaneous; direct manipulation | None beyond showing the result |
| **≤ 1 s** | Noticeable delay, flow intact | Minimal (cursor, subtle indicator) |
| **≤ 10 s** | Attention strained | Busy indicator / spinner |
| **> 10 s** | Attention lost | Percent-done progress + ETA + cancel |

Rules:

- Every click/keystroke gets visible feedback within **100 ms** — this is the definitive "feels responsive" line.
- Anything 1–10s: show activity immediately, or users assume a hang.
- Anything >10s: determinate progress, allow cancel, allow backgrounding, notify on completion.

---

## 9. Consistency & Jakob's Law

**Jakob's Law:** users spend most of their time in *other* apps; those apps set their expectations ([Laws of UX](https://lawsofux.com/jakobs-law/)).

- **Internal consistency** — same term, icon, position, and behavior for the same concept everywhere in your app.
- **External consistency** — follow platform/industry conventions: Esc cancels, Enter confirms, right-click opens context menu, Ctrl+Z undoes, gear = settings.
- Break a convention only when your alternative is *demonstrably* better — novelty has a real learning cost ([NN/g](https://www.nngroup.com/articles/consistency-and-standards/)).

## 10. Recognition over Recall

Recognition (pick from visible options) is far easier than recall (retrieve from memory) ([NN/g](https://www.nngroup.com/articles/recognition-and-recall/)).

- Show commands in menus; shortcuts are accelerators, never the only path.
- Preserve history: recent files, recent searches, last-used values.
- Label icons (or at minimum tooltip them).
- Show current state in place (breadcrumbs, selected-tab highlight, field values) instead of expecting users to remember it.

## 11. Progressive Disclosure

Show the essential by default; make the advanced reachable on request ([NN/g](https://www.nngroup.com/articles/progressive-disclosure/)).

- Get the **split** right (frequency data, not opinion): common options primary, rare options behind "Advanced…".
- Make the **path** obvious: a labeled, visible affordance ("More options ▸"), not a hidden gesture.
- **Max ~2 levels** of disclosure — beyond that, features become effectively invisible.

## 12. Aesthetic-Usability Effect

Users perceive attractive interfaces as more usable and forgive minor flaws in them (Kurosu & Kashimura's 1995 ATM study, 252 participants; [NN/g](https://www.nngroup.com/articles/aesthetic-usability-effect/)).

- Polish is not vanity — it buys real tolerance and trust.
- But it masks only *minor* issues, and it can hide problems during testing (watch what users *do*, not what they say).
- Form and function must ship together.

---

## Do's & Don'ts

**Do**
- Give feedback for every input within 100 ms.
- Use spacing and alignment as the primary grouping/hierarchy tools.
- Follow platform conventions (Esc/Enter, right-click, Ctrl+Z) exactly.
- Provide undo instead of confirmation wherever possible.
- Keep any single group of choices to ≤ 7 items.
- Make every interactive element statically distinguishable from static content.

**Don't**
- Rely on hover as the only signifier of interactivity.
- Hide primary features behind memorized shortcuts or deep nesting (>2 levels).
- Show 15+ ungrouped options anywhere.
- Let an operation run >1s in silence, or >10s without determinate progress.
- Mix two visual styles for the same control type.

## Common Pitfalls

1. **Feature creep** silently multiplying extraneous cognitive load.
2. **Hierarchy collapse** — everything bold, everything colored, nothing dominant.
3. **False affordances** — decorative elements that look interactive.
4. **1–10 second dead zone** — operations too long to feel instant, too short for someone to have added a spinner.
5. **Assuming users remember** anything from the previous screen (they hold 4–7 chunks for ~20s).
6. **Convention-breaking for style** — clever, novel interactions that fail Jakob's Law.

---

## Applying It in elimgui (immediate-mode / desktop GUI)

Immediate mode is actually an advantage for several of these principles — state is redrawn every frame, so *visibility of system status* is nearly free. Concrete guidance:

- **100 ms rule is trivially met** — hover/active states computed per frame must render the same frame the input arrives. Never defer visual feedback to the next event.
- **Bake the heuristics into widget defaults**: `eli_button()` should ship with visible default/hover/active/disabled signifiers so every app built on elimgui inherits correct affordances. A style system that *allows* removing all signifiers should not *default* to it.
- **Fitts's Law in the layout engine**: give widgets a `hit_rect` that can be larger than the draw rect (padded click targets for small icons, splitters, tree chevrons). Default interactive height ≥ 24px, comfortable 28px.
- **Hick's Law in the demo/app**: keep elimgui's own menus (window context menus, demo menu bar) ≤ 7 top-level items; document this as a recommendation.
- **Recognition over recall**: menu items should render their shortcut strings (`"Save\tCtrl+S"`); provide recent-values in combo boxes where cheap.
- **Consistency via the style struct**: one `eli_style` drives all widgets — a single source of truth for spacing, colors, and rounding is your internal-consistency enforcement mechanism. Resist per-widget one-off style pushes in app code.
- **Progressive disclosure primitives**: `eli_collapsing_header()` and tree nodes are your disclosure mechanism — default advanced sections to collapsed, and keep nesting ≤ 2 levels in your own demo layouts.
- **Long operations**: elimgui is single-threaded per frame; provide `eli_progress_bar()` (determinate + indeterminate) and document the 1 s / 10 s thresholds in its header comment so users pick the right variant.
- **Undo over confirm**: as a library, expose the pattern (toast widget with an action button) so apps can implement "Deleted — Undo" instead of modal confirmations.

### Checklist for every elimgui widget

- [ ] Visible signifier in default state (not hover-only)
- [ ] Distinct hover, active, focus, and disabled visuals
- [ ] Hit area ≥ 24×24 px regardless of visual size
- [ ] Feedback renders the same frame input is received
- [ ] Behavior matches desktop conventions (Esc, Enter, arrows, right-click)
- [ ] Works when the same code runs every frame (no hidden retained state surprises)
