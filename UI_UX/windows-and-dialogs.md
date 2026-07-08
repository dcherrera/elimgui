# Windows & Dialogs

> Modality is a loan against the user's attention. Take it out rarely, and pay it back fast.

## Overview

Desktop apps juggle primary windows, tool panels, and dialogs. The core decisions: when to interrupt (modal) vs coexist (modeless), how to structure alerts so users act correctly in one glance, and how to manage focus and z-order so keyboard users never get lost. ([Apple HIG](https://developer.apple.com/design/human-interface-guidelines/), [Windows UX Guide](https://learn.microsoft.com/en-us/windows/win32/uxguide/win-dialog-box), [NN/g](https://www.nngroup.com/articles/modal-nonmodal-dialog/))

---

## 1. Window Management Patterns

| Pattern | Description | Modern use |
|---------|-------------|------------|
| SDI | One document per window | Simple editors |
| MDI | Child windows inside a parent | Legacy; avoid |
| **TDI (tabbed)** | Documents as tabs in one frame | Dominant (browsers, IDEs) |
| **Tiled/docked** | Non-overlapping panes | Pro tools, IDEs (see docking.md) |
| Floating panels | Tool windows above the main window | Palettes, inspectors |

Guidelines:
- **Persist geometry**: remember position, size, and state per window across sessions; clamp restored positions on-screen (monitor changes can strand windows off-screen) ([Microsoft](https://learn.microsoft.com/en-us/answers/questions/3871904/why-doesnt-windows-11-remember-the-position-of-my)).
- **Minimum sizes**: enforce sensible minimums so content never collapses; Windows guidance ranges ~192×48 minimum for small utility surfaces; content windows typically ≥ 320–500px ([Windows spacing](https://learn.microsoft.com/en-us/windows/apps/design/style/spacing)).
- **Default placement**: dialogs centered on their owner window; contextual property popups near the object they affect (offset so they don't cover it).

## 2. Modal vs Modeless ([NN/g](https://www.nngroup.com/articles/modal-nonmodal-dialog/))

**Use a modal only when:**
- The task must complete/cancel before anything else makes sense (unsaved changes on close).
- The action is destructive and irreversible.
- A critical error blocks continuation.

**Prefer modeless (panel, inspector, popup) when:**
- The user needs to see or interact with the content behind (find & replace, filters, properties).
- The task is frequent or repetitive.
- Changes can apply immediately (modeless = immediate-commit; modal = deferred OK/Cancel commit).

Costs of modality: broken flow, hidden context, trained dismissal ("alert fatigue"), eroded trust. High Esc-usage on a modal is a signal it shouldn't be modal.

| Aspect | Modal | Modeless |
|--------|-------|----------|
| Background | Blocked + scrim | Fully interactive |
| Commit model | Deferred (OK/Cancel) | Immediate |
| Dismissal | Explicit buttons, Esc | Close button, stays open |
| Good for | Confirmations, blocking errors, wizards | Tools, inspectors, search |

## 3. Dialog Anatomy

1. **Title** — state the point or question: "Delete 3 files?" not "Confirm". Max ~2 lines.
2. **Body** — cause, consequence, specifics ("This can't be undone. The files will be removed from disk.").
3. **Buttons** — 2–3 max ([Material dialogs](https://m3.material.io/components/dialogs)); **verb labels, never Yes/No/OK for consequential actions**: "Delete", "Save", "Keep Editing" ([Apple HIG alerts](https://developer.apple.com/design/human-interface-guidelines/alerts)).
4. Optional icon for warnings (data-loss risk only).

### Button order & defaults

| Platform | Order (left→right) |
|----------|--------------------|
| Windows | affirmative … Cancel (OK left of Cancel) |
| macOS | Cancel … affirmative (action on the far right) |

Pick one convention and apply it *everywhere* — consistency beats optimal placement ([NN/g OK-Cancel](https://www.nngroup.com/articles/ok-cancel-or-cancel-ok/)). For a web-delivered tool, the macOS/web convention (primary at bottom-right, Cancel to its left) is the safest default.

- **Default button** = visually emphasized + triggered by **Enter**. **Esc = Cancel**, always.
- **Never make the destructive action the default.** Default to the safe choice.

## 4. Destructive Actions & Confirmation ([NN/g](https://www.nngroup.com/articles/confirmation-dialog/))

- Confirm only what is serious *and* irreversible. Everything else: **do it + offer Undo** (toast with Undo button).
- Be specific: "Delete 'report.pdf'?" — restate the object and consequence.
- Destructive button styled with the error/danger color; safe button is default.
- High-friction confirmation (type the name to confirm) is reserved for catastrophic, unrecoverable operations.
- Repeated confirmation of routine actions trains reflexive clicking and makes real warnings invisible.

### Unsaved changes pattern

Trigger only when changes exist and the user closes/navigates. Standard 3 buttons: **Cancel** (return to editing), **Don't Save / Discard**, **Save** (default, Enter). Title names the document: "Save changes to 'main.c'?" ([Cloudscape](https://cloudscape.design/patterns/general/unsaved-changes/)).

## 5. Focus & Z-Order

Modal dialogs must ([UXPin focus traps](https://www.uxpin.com/studio/blog/how-to-build-accessible-modals-with-focus-traps/)):
1. **Move focus in** on open (first field, or the default/safe button).
2. **Trap Tab/Shift+Tab** inside the dialog (wrap around).
3. **Block** interaction with everything behind.
4. **Restore focus** to the triggering control on close.

Z-order stack (bottom→top): windows → focused window → modal + scrim → nested modal → menus/popups → tooltips → drag preview. Esc closes only the topmost layer. Avoid nested modals except double-confirmation of catastrophes.

**Click-outside-to-dismiss:** yes for popups, dropdowns, popovers, non-critical panels; **no** for modals with user input or required decisions.

## 6. Scrim / Dimming Overlay

- Black at **32%** opacity is the Material standard; 32–50% is the accepted band ([Material](https://github.com/material-components/material-components-android/issues/4295)).
- Too dark (>60%) hides context; too light (<20%) fails to signal modality.

## 7. Window Chrome

- Title bar: title text (document — app, or panel name), close/collapse controls in the platform corner. In a canvas GUI you define your own convention — pick one corner (right, Windows-style X is most familiar to most users) and keep it fixed.
- The title-bar close acts like Cancel, never like Save.
- Progress dialogs may disable close while an uncancelable operation completes (better: make it cancelable).
- Panels: close (x), collapse, and a drag area; see docking.md.

---

## Do's & Don'ts

**Do**
- Default to modeless; make modality the exception.
- Verb-label buttons; safe default; Enter/Esc wired everywhere.
- Center dialogs on their parent; clamp to screen.
- Trap and restore focus for modals.
- Use undo instead of confirmation for reversible actions.
- Persist window/panel geometry.

**Don't**
- Confirm routine actions.
- Make destructive buttons the default or place them where the primary usually sits.
- Use "OK / Yes / No" for consequential choices.
- Allow click-through or Tab-through behind a modal.
- Stack modals (except catastrophic double-confirm).
- Open modals the user didn't ask for.

## Common Pitfalls

1. **Alert fatigue** — reflexive "Yes" clicking from over-confirmation.
2. **Focus leaks** — Tab escaping the modal into the blocked background.
3. **Lost focus on close** — keyboard users dumped at the document start.
4. **Ambiguous titles** — "Warning!" tells the user nothing; ask the actual question.
5. **Close ≠ Cancel** — an X that silently saves (or a Cancel that loses typed input without warning).
6. **Off-screen restore** — remembered geometry pointing to an unplugged monitor.

---

## Applying It in elimgui (immediate-mode / desktop GUI)

- **Window flags map to these patterns**: `ELI_WINDOW_NO_TITLEBAR`, `ELI_WINDOW_MODAL`, etc. A modal flag must imply: scrim rendering (black 32–40% over everything below), input blocking of lower windows (both mouse *and* keyboard/nav), topmost z-order, and focus capture.
- **Provide `eli_begin_popup_modal()` / `eli_begin_popup()`** with the correct semantics split: popups close on click-outside and Esc; modals close only via buttons/Esc.
- **Focus behavior in immediate mode**: track a `nav focus` id per window; when a modal opens, push the previous focus id, set focus to the modal's first widget (or the default button), and pop/restore on close — this is cheap to implement with an id stack and is what makes keyboard use feel professional.
- **Enter/Esc plumbing**: within a modal, Enter activates the widget marked `ELI_BUTTON_DEFAULT` (drawn emphasized), Esc activates `ELI_BUTTON_CANCEL`. Make this part of the button API so apps get it for free.
- **A ready-made confirm helper**: `eli_dialog_confirm(title, body, verb, ELI_DIALOG_DANGER)` that renders correct layout — title (semibold), body (secondary color), Cancel + verb button bottom-right, danger styling, safe default. Apps will copy whatever your demo does; give them the right pattern.
- **Undo-toast primitive** (see feedback-and-states.md) so apps can replace confirmations.
- **Z-order manager**: maintain an explicit window stack; clicking a window raises it; modals pin to top; popups/tooltips above modals. Render order = input order reversed (topmost gets input first).
- **Geometry persistence**: elimgui owns window pos/size in its context — serialize to a settings blob (localStorage via jsio in the browser) and clamp to canvas bounds on restore.
- **Title bar spec**: fixed height (e.g. 28–32px), title left with 8px padding, close X right with a ≥24px hit area, hover state on the X, whole bar draggable.
