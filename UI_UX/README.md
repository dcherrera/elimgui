# UI/UX Best-Practices Reference

A study-oriented reference for designing and building **elimgui** — an immediate-mode, canvas-rendered desktop GUI library — compiled from Nielsen Norman Group, Apple HIG, Google Material Design, Microsoft Fluent/Windows guidelines, W3C WCAG/ARIA APG, IBM Carbon, Refactoring UI, Laws of UX, Atlassian Design, and primary tool documentation (VS Code, Visual Studio, JetBrains, Blender, Dear ImGui).

Every file follows the same shape: **Overview → Core principles → Concrete patterns → Do's & don'ts → Measurable numbers → Common pitfalls → "Applying it in elimgui"**.

## Reading Order

### Foundations (read first)
| File | Covers |
|------|--------|
| [00-foundations.md](00-foundations.md) | Nielsen's heuristics, Gestalt, visual hierarchy, affordances/signifiers, Fitts's & Hick's Laws, cognitive load, 0.1/1/10s response limits, consistency, recognition over recall, progressive disclosure, aesthetic-usability |

### Visual System
| File | Covers |
|------|--------|
| [layout-and-spacing.md](layout-and-spacing.md) | 4/8pt spacing scale, grids, alignment, density modes, whitespace, containment (space > background > shadow > border), resizable panels |
| [typography.md](typography.md) | Type scale & ratios, hierarchy via size/weight/color, line length/height, minimum sizes, tabular figures, monospace, all-caps tracking, truncation |
| [color-and-theming.md](color-and-theming.md) | Palette construction, semantic roles/tokens, 60-30-10, dark mode (#121212, 87/60/38% text), elevation, state layers (8/12/38%), contrast |
| [icons-and-imagery.md](icons-and-imagery.md) | Icon ambiguity & labels, metaphors, one-family consistency, grid sizes (16/20/24), hit targets, monochrome color, atlas rendering |

### Structure & Interaction
| File | Covers |
|------|--------|
| [windows-and-dialogs.md](windows-and-dialogs.md) | Window patterns, modal vs modeless, dialog anatomy & button order, destructive confirmation vs undo, focus traps & z-order, scrims |
| [docking.md](docking.md) | **Deep dive**: VS Code / Visual Studio / JetBrains / Blender / Dear ImGui docking dissected; drop guides & live previews, tab-vs-split semantics, tear-out, persistence, recovery — with an elimgui implementation spec |
| [drag-and-drop.md](drag-and-drop.md) | The five phases, thresholds & timings, handles, ghosts, drop targeting, insertion lines, auto-scroll, cancel/undo, WCAG 2.5.7 alternatives |
| [menus-and-navigation.md](menus-and-navigation.md) | Menu bars & item anatomy, context menus, dropdown thresholds, submenu safe-triangle, command palettes, sidebars/rails, tabs-as-nav, breadcrumbs |
| [controls-and-inputs.md](controls-and-inputs.md) | Button hierarchy & sizing, switch vs checkbox, radio/dropdown selection thresholds, sliders + drag-numbers, text/numeric inputs, forms & validation timing, keyboard behavior |

### Feedback & Data
| File | Covers |
|------|--------|
| [feedback-and-states.md](feedback-and-states.md) | Widget states (hover/active/focus/disabled/loading), spinners vs progress, skeletons, empty states, error messages, tooltips, toasts, undo-over-confirm |
| [data-display.md](data-display.md) | Tables (density, alignment, sorting, sticky headers, selection), lists, trees (indent, chevrons, arrow keys), tab strips, virtualization, chart basics |

### Quality Gates
| File | Covers |
|------|--------|
| [accessibility.md](accessibility.md) | WCAG contrast (4.5:1 / 3:1) & target sizes (24/44px), keyboard operability & roving focus, focus visibility (2px/3:1), canvas + screen readers (parallel-DOM strategy), reduced motion, color independence |
| [motion-and-microinteractions.md](motion-and-microinteractions.md) | Durations (100–200ms desktop), easing (ease-out in / ease-in out), what to animate vs keep instant, restraint in pro tools, delta-time lerp patterns for immediate mode |

## The Numbers That Matter Most (cheat sheet)

| Metric | Value |
|--------|-------|
| Feedback after input | ≤ 100ms (same frame) |
| Spinner / determinate progress thresholds | > 1s / > 10s |
| Spacing scale | 2 4 6 8 12 16 24 32 48 |
| Default UI text / minimum | 13–14px / 11px |
| Text / UI-component contrast | 4.5:1 / 3:1 |
| Hit target minimum / comfortable | 24×24px / 28–40px |
| Focus ring | ≥2px, 3:1 contrast |
| State overlays (hover/press/disabled) | 8% / 12% / 38% |
| Dark theme base / text | #121212 / white @ 87–60–38% |
| Animation range (desktop) | 100–250ms, ease-out in / ease-in out |
| Drag threshold / tooltip delay / toast duration | 4–5px / ~400ms / ~5s |
| Menu depth / tabs / radio options | ≤2 levels / ≤6 / 2–7 |
| Splitter visible/hit; min pane | 4px / 8–12px; ~96px |

## Key Sources

[Nielsen Norman Group](https://www.nngroup.com/) · [Apple Human Interface Guidelines](https://developer.apple.com/design/human-interface-guidelines/) · [Material Design 3](https://m3.material.io/) · [Microsoft Fluent 2 / Windows UX](https://fluent2.microsoft.design/) · [WCAG 2.2](https://www.w3.org/TR/WCAG22/) · [WAI-ARIA Authoring Practices](https://www.w3.org/WAI/ARIA/apg/) · [Refactoring UI](https://refactoringui.com/) · [Laws of UX](https://lawsofux.com/) · [IBM Carbon](https://carbondesignsystem.com/) · [Atlassian Design System](https://atlassian.design/) · [VS Code docs](https://code.visualstudio.com/docs/configure/custom-layout) · [Visual Studio layout docs](https://learn.microsoft.com/en-us/visualstudio/ide/customizing-window-layouts-in-visual-studio) · [JetBrains tool windows](https://www.jetbrains.com/help/idea/manipulating-the-tool-windows.html) · [Blender manual](https://docs.blender.org/manual/en/2.93/interface/window_system/areas.html) · [Dear ImGui docking wiki](https://github.com/ocornut/imgui/wiki/Docking)
