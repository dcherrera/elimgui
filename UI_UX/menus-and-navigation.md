# Menus & Navigation

> Menus are the app's table of contents: they must be exhaustive (recognition over recall) even when toolbars and shortcuts duplicate them.

## Overview

Desktop navigation is built from a small set of battle-tested structures: the menu bar, context menus, dropdowns/combos, cascading submenus, sidebars/drawers, tabs, breadcrumbs, and (in modern tools) the command palette. The conventions here are old, strong, and cheap to follow — and expensive to violate (Jakob's Law).

---

## 1. Menu Bars

- **Canonical order**: `File  Edit  View  [app-specific…]  Window  Help` ([Apple HIG menu bar](https://developer.apple.com/design/human-interface-guidelines/the-menu-bar)). One-word titles, title case.
- **Group with separators** by function; order groups by frequency/workflow.
- **Everything belongs in the menu bar** even if it's also on a toolbar/context menu — the menu is where users *discover* commands and learn their shortcuts.
- **Ellipsis rule**: `Save As…` — append "…" when the item opens a dialog needing more input; omit for immediate actions.
- **Shortcuts right-aligned** in a consistent column (`Save        Ctrl+S`).
- **Disabled vs hidden** ([Smashing](https://www.smashingmagazine.com/2024/05/hidden-vs-disabled-ux/)): disable (gray out) when temporarily unavailable — it teaches the UI and avoids layout shift; hide only what's genuinely irrelevant. Ideally a disabled item's tooltip explains why.
- Keep each menu scannable: prefer ≤ ~12 items with separators; push overflow into submenus (max depth 2).

### Menu item anatomy

```
[check/radio] [icon] Label………………… Shortcut  [▸ submenu chevron]
```
- Reserve a consistent left column for check/radio/icon; label left-aligned; shortcut right-aligned in secondary color; chevron for submenus.
- Row height ~24–28px desktop-dense; full-row hover highlight; full-row hit area.

## 2. Context Menus ([NN/g](https://www.nngroup.com/articles/contextual-menus-guidelines/))

- Contain the **most relevant commands for the clicked object**, most likely first; keep short (~≤10 items).
- **Redundancy rule: never the only way to do something** — right-click is invisible to many users.
- Open adjacent to the cursor (Fitts's Law); flip to stay on-screen.
- Don't mix object-specific and global commands in one menu.
- Also openable without a mouse: keyboard Menu key / Shift+F10 convention, and visible ⋮ (kebab) buttons for the same actions on hoverable items.
- Dismiss on Esc, on click outside, and after executing an item.

## 3. Dropdowns & Combo Boxes ([NN/g](https://www.nngroup.com/articles/drop-down-menus/), [USWDS](https://designsystem.digital.gov/components/combo-box/))

- **<7 options** → radio buttons / segmented control (all visible beats hidden).
- **7–15 options** → dropdown.
- **>15 options** → combo with type-to-filter (autocomplete); never a giant scroll list.
- Keep the trigger label visible while open; show a scrollbar when the list scrolls (communicates extent); reflect the current value in the closed state.
- Don't use a dropdown where typing is faster (dates, quantities).

## 4. Submenus & the Hover-Intent Problem

- **Max 2 levels** of cascade ([NN/g](https://www.nngroup.com/articles/menu-design/)).
- Chevron ▸ marks items with submenus.
- **Open delay ~200–300ms; close delay ~300–500ms** so the menu doesn't flicker on pass-through.
- **Safe triangle**: while the cursor moves inside the triangle formed by its position and the submenu's near corners, keep the submenu open even though the cursor crosses sibling items ([Smashing — safe triangles](https://www.smashingmagazine.com/2023/08/better-context-menus-safe-triangles/)). This single trick is the difference between "polished" and "infuriating" cascading menus.
- Arrow keys: Right opens submenu/enters, Left closes/returns, Up/Down navigate, Enter activates, Esc closes one level.

## 5. Command Palette ([UX Patterns](https://uxpatterns.dev/patterns/advanced/command-palette))

The pro-tool accelerator: `Ctrl/Cmd+Shift+P` (or Ctrl+K) opens a centered search over *all* commands.
- **Fuzzy match** against command names; rank recents/frequency first; show each command's shortcut inline (this is how users *learn* shortcuts).
- Up/Down + Enter; Esc closes. Include panel-toggling and navigation commands, not just actions.
- A palette complements — never replaces — visible menus (recognition over recall).

## 6. Sidebars, Drawers, Rails

| Pattern | Width | Use |
|---------|-------|-----|
| Full sidebar/drawer | **240–320px** | Primary nav, file trees |
| Collapsed icon rail | **48–72px** | Space-saving persistent nav ([Material rail](https://m3.material.io/components/navigation-rail/guidelines)) |
| Temporary drawer (overlay) | ≤ ~400px | Rare on desktop; mobile pattern |

- Desktop tools favor **persistent, collapsible, resizable** sidebars on the leading edge; remember collapsed state and width.
- Icon-only collapsed state requires tooltips on every icon.
- 3–7 top-level destinations is the comfortable range for a rail.

## 7. Tabs as Navigation, Breadcrumbs

- Tabs for top-level section switching: **≤ ~6 tabs** ([Apple HIG]) with concise labels; selected tab must be unmistakable (fill or underline + weight); don't mix "navigation tabs" and "document tabs" in one strip ([NN/g tabs](https://www.nngroup.com/articles/tabs-used-right/)).
- Breadcrumbs for hierarchies ≥3 levels: `Root ▸ Parent ▸ Current`, ancestors clickable, current not a link ([NN/g breadcrumbs](https://www.nngroup.com/articles/breadcrumbs/)). Useful in settings trees and file browsers; never in linear wizards.

---

## Do's & Don'ts

**Do**
- Follow File/Edit/View/…/Help ordering and the "…" convention.
- Show shortcuts in menus; right-aligned, consistent column.
- Keep context menus short, relevant, redundant with visible UI.
- Implement safe-triangle + delays for submenus.
- Use type-to-filter above ~15 options.
- Give disabled items a "why" (tooltip).

**Don't**
- Nest menus >2 deep.
- Make right-click the only path to any command.
- Repurpose ☰/⋮/… icons for unrelated behaviors.
- Auto-hide menu items based on usage (layout instability, mystery-meat).
- Use ALL CAPS tab labels or >6 top-level tabs.
- Put interactive content in a menu that closes on first click.

## Common Pitfalls

1. **Flickering submenus** — no close-delay/safe-triangle; users physically can't reach the submenu.
2. **Menu-only features** — commands that exist nowhere visible; discovery is zero.
3. **Ambiguous checked state** — check/radio items with no reserved indicator column, so labels shift when toggled.
4. **Context menu grab-bag** — 20 mixed global+local items.
5. **Dropdown abuse** — 3 options behind a click, or 200 options without search.
6. **Menus that outrun the screen** — no flip/clamp logic at viewport edges.

---

## Applying It in elimgui (immediate-mode / desktop GUI)

- **API family** (mirrors ImGui, keep the shape): `eli_begin_menu_bar()`, `eli_begin_menu("File")`, `eli_menu_item("Save", "Ctrl+S", selected, enabled)`, `eli_separator()`, `eli_begin_popup_context_item()`. `eli_menu_item` should render the full anatomy: check column, label, right-aligned shortcut in `TEXT_SECONDARY`, chevron when it's a submenu.
- **Implement submenu hover-intent in the library**: per-menu open timer (~250ms) + close grace (~350ms) + safe-triangle test between the cursor and the open submenu's rect. App code should get correct cascades for free — this is exactly the kind of fiddly interaction a framework must own.
- **Positioning/clamping**: popups flip vertically/horizontally at canvas edges; submenus open to the right, flipping left near the edge with slight vertical overlap (~4px) so the chevron row stays aligned.
- **Keyboard**: menu bar activation (Alt), arrow navigation across menus and within them, Enter/Esc, and type-ahead (jump to item by first letter). Route through the same nav-focus system as dialogs.
- **Context menu helper**: `eli_begin_popup_context_item()` bound to the last widget's right-click, opening at the cursor. Encourage ≤10 items in docs.
- **Sidebar recipe**: a child panel with width persisted in settings, a splitter (4/10px), and a collapse toggle that animates (~150ms) to a 48px icon rail; icons get automatic tooltips from their labels.
- **Command palette as a widget**: `eli_command_palette(commands[], n)` — centered modal, text input with fuzzy filter, scored list showing name + shortcut, recents on empty query. It advertises every registered command; pairs naturally with a shortcut registry (`eli_shortcut("Ctrl+S", cmd_save)`), which also feeds the menu shortcut column from one source of truth.
- **Ellipsis/label conventions in the demo**: the demo app is documentation-by-example — follow File/Edit/View, "…", and separator grouping exactly.
