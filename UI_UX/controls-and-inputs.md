# Controls & Inputs

> Pick the right control for the data, size it for the pointer, label it for the eye, and validate it at the right moment.

## Overview

Widgets are where design systems earn their keep: buttons need a hierarchy, toggles and checkboxes mean different things, sliders trade precision for speed, and forms live or die on labeling and validation timing. This doc gives the selection rules and the numbers.

---

## 1. Buttons

### Hierarchy — one primary per view

| Level | Look | Use |
|-------|------|-----|
| **Primary** | Filled, accent color | The one main action of the view |
| **Secondary** | Outlined or tonal fill | Alternative actions |
| **Tertiary/Ghost** | Text-only, hover background | Low-emphasis, repeated, inline actions |
| **Danger** | Filled/outlined red | Destructive; never the default |

- **One primary button per view/dialog** ([button hierarchy](https://subux.pro/guides/article/button-hierarchy-primary-secondary-tertiary)); two "loud" buttons = no hierarchy.
- Differentiate levels by more than color (fill vs outline vs text) ([NN/g buttons](https://www.nngroup.com/topic/buttons/)).

### Sizing & labels

- Desktop heights: **28–32px dense**, 32–40px standard; ≥44px touch ([Material buttons](https://m3.material.io/components/buttons/guidelines), [Apple HIG](https://developer.apple.com/design/human-interface-guidelines/buttons)). Horizontal padding ~12–24px; min width so single-word buttons don't look square.
- **Verb-first labels**: "Save changes", "Delete file", "Create project" — not "OK/Submit/Click here" ([NN/g UI copy](https://www.nngroup.com/articles/ui-copy/)). 1–3 words.
- Icon+label is the most usable; icon-only buttons require tooltips and an accessible name.
- States: default / hover / active / focus / disabled / (loading) — see feedback-and-states.md. During async work: disable + spinner + "Saving…".

## 2. Toggles/Switches vs Checkboxes ([NN/g](https://www.nngroup.com/articles/toggle-switch-guidelines/))

| Control | Semantics |
|---------|-----------|
| **Switch** | Takes effect **immediately** (settings, live options) |
| **Checkbox** | Part of a form; applies on **submit/OK** |

- Never mix switches into a submit-button form — users can't tell if the change already happened.
- Switch labels: state the thing, not the state — "Autosave" + switch, not "Autosave: On/Off".

## 3. Checkboxes, Radios, and When to Use a List Instead

| Situation | Control |
|-----------|---------|
| 1 independent yes/no | Checkbox |
| N independent options | Checkboxes |
| Exactly one of **2–7** options | Radio group (all visible) |
| One of **7–15** | Dropdown |
| One of **>15** | Combo with search |
| Several of many | Multi-select list / checkboxes + filter |

([NN/g checkboxes vs radios](https://www.nngroup.com/articles/checkboxes-vs-radio-buttons/), [listbox vs dropdown](https://www.nngroup.com/articles/listbox-dropdown/))

- **The entire label is clickable**, not just the 14px box ([UX Movement](https://uxmovement.com/forms/ways-to-make-checkboxes-radio-buttons-easier-to-click/)); row hit height ≥24px.
- Radio groups: one option pre-selected where sensible; vertical layout scans best.
- Parent/child checkboxes use an **indeterminate** (mixed) state ([Material](https://m2.material.io/components/selection-controls/)).
- Keyboard: group = **one tab stop**; arrows move selection inside ([W3C radio pattern](https://www.w3.org/WAI/ARIA/apg/patterns/radio/)).

## 4. Sliders & Numeric Input ([NN/g sliders](https://www.nngroup.com/articles/gui-slider-controls/), [steppers](https://www.nngroup.com/articles/input-steppers/))

- Sliders suit **approximate, relative** values (volume, opacity) where the *effect* matters more than the number.
- **Always show the current value**, and for anything users might want exact, **pair slider + editable numeric field** (double-click / click the value to type — the ImGui convention is genuinely good UX).
- Click-on-track jumps to that value; arrows nudge by step; Shift/Ctrl modify step (fine/coarse); ticks for discrete steps.
- **Drag-number widgets** (ImGui `DragFloat`): excellent for pro tools but low affordance — show ↔ cursor on hover, underline or handle styling, and always allow type-in (click or double-click), which also satisfies accessibility (WCAG 2.5.7 needs a non-drag path).
- Numeric fields: steppers for small adjustments around a common value; show units inline ("12 px"); clamp and validate **on blur/commit, not per keystroke**.

## 5. Text Inputs

- **Visible label, above the field** (or left-aligned column in dense property panels); **placeholder is not a label** — it vanishes on focus, has weak contrast, and kills recall ([NN/g placeholders](https://www.nngroup.com/articles/form-design-placeholders/)).
- **Field width hints expected content** — 5-char field for a ZIP, wide field for a path ([match width](https://www.techstacker.com/match-input-field-width-with-input-length-usability/)).
- Standard editing behaviors users expect: click to place caret, double-click selects word, drag selects, Ctrl+A/C/V/X/Z, Home/End, Shift+arrows.
- Selection highlight = primary @ ~30%; caret blinks ~500ms on/off.
- Clear (×) affordance for search-style fields; password fields need show/hide.

## 6. Forms & Validation ([NN/g errors in forms](https://www.nngroup.com/articles/errors-forms-design-guidelines/))

- **Timing:** validate a field when the user *leaves* it (blur), and re-validate live *only after* it has erred once ("reward early, punish late"). Never scold mid-typing.
- **Placement:** error message directly **below the field**, red text + icon, field border turns error color. Keep the user's input for correction; never wipe the form.
- **Message:** specific + constructive: "Password must be at least 8 characters" — not "Invalid input" (see feedback-and-states.md).
- Mark required fields with `*` **plus** legend; better: mark the *optional* ones if most are required ([NN/g required fields](https://www.nngroup.com/articles/required-fields/)).
- Single-column layout; related fields grouped with whitespace; primary button bottom-right (with Cancel to its left), matching dialog conventions.
- Enter in a single-field form submits; Esc cancels/reverts the current edit.

## 7. Keyboard Behavior Summary

| Key | Behavior |
|-----|----------|
| Tab / Shift+Tab | Next/previous control (groups = one stop) |
| Arrows | Within groups: radios, lists, menus, sliders, steppers |
| Space | Toggle checkbox/button focus activation |
| Enter | Default/primary action; commit text edit |
| Esc | Cancel edit (revert value), close popup |
| Home/End | Extremes (lists, sliders, text) |

---

## Do's & Don'ts

**Do**
- One primary button per view; verb-first labels.
- Radios for ≤7 visible exclusive options; search past ~15.
- Pair every slider/drag control with type-in.
- Validate on blur; message below the field; keep input.
- Make whole rows/labels clickable.
- Show units, show current values, show which field is focused.

**Don't**
- Use placeholder text as the only label.
- Mix immediate-effect switches into submit forms.
- Validate on every keystroke before first blur.
- Use color alone for error/selected/disabled states.
- Make hit targets smaller than 24×24px.
- Wipe user input on error, ever.

## Common Pitfalls

1. **Two primaries** — competing filled buttons paralyze the choice.
2. **Slider-only precision** — forcing pixel-hunting for an exact number.
3. **Keystroke validation** — red errors while the user is mid-word.
4. **14px checkbox targets** — misses and rage-clicks.
5. **Mystery required fields** — discovered only on failed submit.
6. **Drag-number with no affordance** — powerful control nobody discovers (cursor + tooltip + double-click-to-edit fix it).

---

## Applying It in elimgui (immediate-mode / desktop GUI)

- **Button variants in the API**: `eli_button()` (secondary by default), plus flags/styles `ELI_BUTTON_PRIMARY`, `ELI_BUTTON_GHOST`, `ELI_BUTTON_DANGER`, `ELI_BUTTON_DEFAULT` (Enter-activated in dialogs). Heights from style: 28px dense / 32px default; auto width = text + 2×`frame_padding.x` with a min width.
- **Full-row hit areas**: `eli_checkbox("Label", &v)` and `eli_radio` must make label + box one hit rect, height ≥ frame height.
- **Slider trio**: `eli_slider_float` (track + grip + centered value text, click-track jump, Shift=fine/Ctrl=coarse), `eli_drag_float` (↔ cursor on hover, drag horizontal to adjust), and **Ctrl+click or double-click → inline text edit** on both. Keep ImGui's input-fallback pattern — it's the accessibility path too.
- **Text input** (`eli_input_text`): full caret/selection editing, 500ms caret blink, selection in primary@30%, Esc-revert / Enter-commit semantics, optional hint text drawn in `TEXT_DISABLED` *only* when empty — and encourage real labels in the docs.
- **Validation plumbing**: immediate mode makes "validate on blur" easy — `eli_input_text` returns an `edited`/`deactivated_after_edit` signal; document the pattern: run validation on `deactivated_after_edit`, store an error string, and render `eli_text_error(msg)` under the field (red, 12px, with icon) while pushing an error border color on the field.
- **Radio/checkbox groups as one nav stop**: within `eli_begin_group()`, arrow keys cycle members; Tab exits the group — implement in the nav layer so all composite widgets share it.
- **Property-panel layout helper**: `eli_label_column(width_frac)` for the label-left dense style with aligned field edges (see layout-and-spacing.md).
- **Steppers**: `eli_input_int` with −/+ buttons (24px min) and arrow-key support; hold-to-repeat with 400ms delay then 50ms interval.
