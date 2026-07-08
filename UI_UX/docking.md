# Docking

> The hardest widget system you will build, and the one users judge hardest. The entire game is: **show the user exactly what will happen before they release the mouse button.**

## Overview

Docking lets users compose their own workspace from panels: tabbing them together, splitting them side-by-side, tearing them out into floating windows, and persisting the arrangement. Every serious tool (VS Code, Visual Studio, JetBrains, Blender, Dear ImGui) solves the same five problems: *drop targeting, tab-vs-split disambiguation, tear-out/re-dock, layout persistence, and recovery from mistakes.* This doc dissects how each tool does it and distills an opinionated spec.

---

## 1. How the Major Tools Do It

### VS Code — editor groups, spatial drops
([Custom Layout docs](https://code.visualstudio.com/docs/configure/custom-layout))
- Unit = **editor group**; drag a tab to an edge of the editor area and a **blue highlight fills exactly the region the new group will occupy**; drop on an existing group's tab strip to move the tab there.
- No diamond overlay — targeting is purely spatial (where you hover determines tab vs split), disambiguated by the live preview rectangle.
- Panels/side bars are separate docking regions; views drag between sidebar and panel.
- Tabs tear out into full floating windows (own grid); geometry persists.
- Everything drag-based also exists as commands (Split Left/Right/Up/Down, Command Palette) — the accessibility escape hatch.

### Visual Studio — the guide diamond
([Customize window layouts](https://learn.microsoft.com/en-us/visualstudio/ide/customizing-window-layouts-in-visual-studio?view=visualstudio), [guide diamond](https://learn.microsoft.com/en-us/archive/blogs/zainnab/using-the-new-guide-diamond), [dock & float](https://devblogs.microsoft.com/visualstudio/easily-dock-and-float-tool-windows/))
- Two window classes: **tool windows** (drag by title bar, dock anywhere incl. IDE edges, can auto-hide) and **document windows** (drag by tab, dock only inside the editor frame).
- During drag, a **diamond/cross overlay** appears over the hovered target: center = tab into it; arrows = split in that direction; extra edge targets pin to the IDE frame. Arrow icons contain miniature pictures of the resulting layout.
- Hovering an arrow shows a **shaded preview region** over the exact area the window will occupy.
- **Ctrl+drag = suppress docking entirely** (free move). **Ctrl+double-click title = toggle docked ⇄ floating.** These two affordances are the accidental-docking cure.
- **Auto-hide**: tool windows collapse to labeled edge tabs; hover/click slides them out; pin icon toggles.
- **Named layouts**: save up to 10, switch with Ctrl+Alt+1…0; plus Window > Reset Window Layout.

### JetBrains IDEs — tool window modes
([Tool windows](https://www.jetbrains.com/help/idea/manipulating-the-tool-windows.html), [view modes](https://www.jetbrains.com/help/idea/viewing-modes.html))
- Persistent **tool window bars** on left/right/bottom edges hold icons; each window has 5 view modes: dock pinned / dock unpinned (auto-hide) / undock / float / separate OS window.
- Everything drag-doable is also in a menu (right-click > Move To / View Mode) — drag is an accelerator, not the only path.
- Shift+F12 restores default layout. Layout auto-persists per project.
- Notable gap users complain about: tool windows can't be split freely — a reminder that too *little* docking freedom also frustrates.

### Blender — pure tiling
([Areas manual](https://docs.blender.org/manual/en/2.93/interface/window_system/areas.html))
- **No overlap, no floating**: the window is a tiling of areas. Drag a corner inward to split, outward to join (a dark overlay + arrow shows which area will be consumed). Workspace tabs = named layouts.
- Lesson: geometric constraints (everything stays a rectangle tiling) eliminate whole classes of invalid layouts — internally, model your dock tree the same way even if you allow floating windows on top.

### Dear ImGui docking branch — the cautionary reference
([Docking wiki](https://github.com/ocornut/imgui/wiki/Docking), issues [#2109](https://github.com/ocornut/imgui/issues/2109), [#7949](https://github.com/ocornut/imgui/issues/7949), [#2599](https://github.com/ocornut/imgui/issues/2599), [#4565](https://github.com/ocornut/imgui/issues/4565))
- Model: **dock nodes** form a binary split tree; each leaf node has a tab bar; `DockSpace()` hosts a tree; floating windows can themselves be dock nodes; layout persists in the `.ini`.
- Drag a tab = undock that window; drag a node's ≡ menu button = move the whole node; overlay arrows appear over targets (center = tab, edges = split).
- Known pain points to design around:
  - **Hidden dockspace undocks everything** (#2599) — dock tree lifetime must not depend on host visibility (`KeepAliveOnly` band-aid).
  - **Programmatic layout vs persisted layout fight** (#7949) — DockBuilder defaults get silently overridden by the .ini; new windows lose their intended home. Rule: apply defaults *only* when no persisted entry exists.
  - **Drag-source ambiguity** (#4565) — tab vs tab-bar-background vs title bar all drag different things; users can't predict which.
  - No keyboard path for any dock operation; API for building layouts is disliked by its own author.

---

## 2. Drop Zones & Indicators — The Spec

**Two-layer targeting (do both):**
1. **Overlay indicators** ("dock guides"): when a drag hovers a dock node, draw 5 targets — center + N/S/E/W — plus optional outer edge targets on the root dockspace. Visual size **~32–40px** per target, **hit area ≥ 40–48px**, 4–8px gaps.
2. **Live preview rectangle**: the instant a target is hovered, fill the *exact* resulting region with a translucent accent (primary color at ~25–35% alpha). This preview is the single most important element of docking UX — VS ships it, VS Code's blue highlight *is* it, and its absence is why lesser dock systems feel like gambling.

Rules:
- **Dock only via explicit indicator hover.** Raw edge-proximity docking causes accidents. (VS requires hovering an arrow; this is correct.)
- Distinguish center vs edge targets visually (center target drawn as a tab-strip pictogram or filled square; edges as directional arrows).
- Show indicators only after drag has begun and cursor is over a valid node — fade in ~100–150ms to avoid flicker.
- Invalid targets: hide (don't gray) the corresponding arrow.
- Split previews should reflect the actual resulting ratio (default 50%, or proportional).

## 3. Tab vs Split Semantics

| Drop on | Result | Meaning |
|---------|--------|---------|
| **Center** of a node | New **tab** in that node | *Alternatives* — one visible at a time, same context |
| **Edge** of a node | **Split** that node | *Simultaneous* — side-by-side reference |
| **Edge of root dockspace** | Split at top level | Major layout regions |
| **Nothing** (empty space) | **Floating window** | Independent |

Opinionated guidance on which to choose (and to encode in your demo/default layouts):
- **Tabs** for documents and interchangeable views (files, settings pages, multiple consoles).
- **Splits** for things consumed *while* working in another pane: code + output, canvas + inspector, diff halves.
- **Vertical stacking within a sidebar** for tool palettes: tree above, properties below.
- Practical ceilings: **2–3 editor splits** on a typical display; **≤ 2–3 levels of split nesting** before layouts become unmanageable. Enforce minimum pane sizes (~80–120px) so panes can't vanish.
- Don't split what could be a tab (wastes space); don't tab what must be watched simultaneously.

## 4. Drag-to-Dock Affordances

- **Drag sources — make them unambiguous and document them:**
  - Drag a **tab** → move/undock that one window.
  - Drag the **title bar** (or node grip) → move the whole node/window.
  - Avoid a third behavior for tab-bar empty space (ImGui's mistake); make it equivalent to the title bar or inert.
- **Drag threshold**: require **~4–6px** of movement before treating a press as a drag (below that it's a click/tab-switch). Tear-out from a tab strip can use a larger threshold (~10–20px, or leaving the tab-bar rect) so tab reordering doesn't accidentally undock.
- **Tear-out**: dragging beyond the threshold with no target hovered detaches the window under the cursor at ~70–85% opacity (ghost) following the mouse.
- **Modifier to suppress docking** while dragging (VS uses Ctrl; ImGui uses Shift): must exist. Also consider **Esc during drag = cancel and restore**.
- **Cursor**: normal arrow while dragging window previews; splitters get resize cursors (↔/↕) with a **4px visible / 8–12px hit** handle.
- **Re-dock shortcut**: double-click a floating window's title bar returns it to its last docked position (VS's Ctrl+dbl-click pattern, simplified).

## 5. Dock Spaces, Persistence, Layout Presets

- **Root dockspace** fills the app window (optionally with a central "document" node that other panels orbit; support a passthrough/empty central node for canvas-centric apps).
- **Floating dock nodes**: floating windows should accept docking too (they're just detached subtrees).
- **Persistence**: serialize the whole dock tree — split axes and *ratios* (fractions, not pixels), tab membership and order, active tab, floating geometry. Save on change, restore on start. In-browser: localStorage/IndexedDB via jsio.
- **Programmatic default layout**: applied **only when no saved state exists** for that window (learn from ImGui #7949). New windows added in a later version should have a declared default target ("dock into node 'right' else float").
- **Named layout presets**: even two ("Default", "Debug") transform usability; expose save/apply/reset APIs. **Reset Layout must always exist** — it is the universal recovery tool.
- **Panel recovery**: a View-menu-style registry listing every panel with its open/closed state; activating a closed panel reopens it at its default dock. Never let a panel become unreachable.

## 6. Auto-Hide (Pin/Unpin)

For rarely used panels (< ~20% of the time): collapse to a labeled edge tab; click (or hover with **300–500ms delay**) slides it over the content; focus loss slides it back; pin icon docks it permanently. This is the VS/JetBrains pattern and it's the right pressure valve between "always visible" and "closed".

## 7. Keyboard & Accessibility

- Every drag operation needs a non-drag path: window/context menu with "Move to → Left/Right/Bottom/Float", "Float", "Dock", tab context menu with "Split Left/Right/Up/Down" (VS Code model).
- Splitters follow the [W3C window-splitter pattern](https://www.w3.org/WAI/ARIA/apg/patterns/windowsplitter/): focusable, arrow keys move (small step), Home/End = min/max, Enter toggles collapse.
- Focus cycling between panes (F6-style "focus next panel") keeps keyboard users mobile in a docked layout.

---

## Do's & Don'ts

**Do**
- Always render the live result-preview rectangle before drop.
- Require explicit indicator hover to dock; support a suppress-docking modifier and Esc-cancel.
- Make tab-drag vs node-drag sources visually and behaviorally distinct.
- Persist ratios as fractions; clamp with min pane sizes.
- Ship Reset Layout + a panel registry menu from day one.
- Keep split nesting shallow (≤3) and enforce min sizes.
- Provide menu/keyboard equivalents for all dock operations.

**Don't**
- Dock on raw edge contact without an indicator hover.
- Make center and edge targets look alike.
- Let hidden/collapsed dockspaces destroy their subtree.
- Let programmatic layout fight persisted layout — persisted wins, defaults fill gaps.
- Allow a pane to be resized to zero or a panel to become unopenable.
- Rely on mouse-only interaction.
- Auto-reveal auto-hidden panels with zero delay.

## Common Pitfalls (with fixes)

| Pitfall | Fix |
|---------|-----|
| Accidental docking while repositioning | Indicator-hover requirement + Ctrl-to-suppress + Esc-cancel |
| Accidental tear-out while switching tabs | Larger tear-out threshold; tab switch on press, drag on move |
| User can't predict tab vs split | Distinct target glyphs + live preview |
| Lost panels | View/panel registry + Reset Layout |
| Layout destroyed on window hide | Dock tree lifetime independent of host visibility |
| .ini overrides new defaults | Apply defaults only where no saved entry exists |
| Ungrabbable splitters | 4px visible / 8–12px hit, hover highlight, resize cursor |
| Infinite nesting spaghetti | Depth soft-limit, min pane sizes, merge single-child nodes |

---

## Applying It in elimgui (immediate-mode / desktop GUI)

`eli_docking.h` design notes:

- **Data model**: a binary split tree per dockspace. Node = split (axis + ratio + two children) | leaf (ordered window list + active index). Floating windows are root nodes of their own trees. Keep the tree in the context (retained — docking is legitimately retained state in an IMGUI, like ImGui does), while windows still submit immediate-mode.
- **API sketch**:
  - `eli_dockspace(id, flags)` — host node, typically over the main viewport.
  - `eli_begin_window()` auto-docks if the window id exists in the tree; else uses a declared default: `eli_dock_default(window, "right")`.
  - `eli_dock_builder_*` for programmatic layouts — *only consulted when no persisted node exists for that id*.
  - `eli_dock_save(buf)/eli_dock_load(buf)` — serialize ratios/tabs/floating geometry; wire to localStorage via jsio.
- **Frame flow for drag-docking** (immediate mode friendly):
  1. On tab/title drag past threshold → set `ctx->drag_window`, render ghost at cursor (window rect at ~75% alpha via draw list, drawn last/topmost).
  2. Each frame, hit-test dock nodes under cursor → if hit, submit the 5-target overlay to the foreground draw list.
  3. If a target is hovered → compute the would-be rect and fill it with primary@30% (the preview).
  4. On release over target → mutate the tree (insert tab / split with 0.5 ratio). On release elsewhere → float at cursor. On Esc/modifier → cancel/suppress.
- **Metrics**: targets 36px visual / 44px hit; preview alpha 0.25–0.35; indicator fade-in ~120ms; tear-out threshold 12px or leaving the tab-bar rect; splitter 4px/10px; min pane 96px; auto-hide reveal delay 350ms.
- **Recovery**: implement `eli_dock_reset()` and expose a built-in "Windows" menu helper that lists all registered windows with checkmarks (open/closed) — the demo app should show it.
- **Fix ImGui's sore spots explicitly**: (a) node lifetime owned by the context, not by host-window visibility; (b) single unambiguous drag rule — tab drags window, title/grip drags node; (c) defaults never override saved state; (d) provide "Split ▸" / "Move to ▸" items in the tab context menu for keyboard/menu parity.
- **Phasing**: v1 = fixed splits + tabbed leaves + splitters (no drag-dock); v2 = drag-to-dock with overlay + preview; v3 = tear-out/floating + persistence + presets. Each phase is independently shippable and useful.
