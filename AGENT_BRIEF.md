# Agent Brief — elimgui Phase Builder

**You are implementing one phase of elimgui.** This brief is the standing contract for every
phase-building agent. The orchestrator will hand you a short *per-phase task* (which phase, its
task list, target directory, key APIs). This document covers everything that is the same every
time. Read it fully before writing code.

---

## 1. What elimgui is

A ground-up reimplementation of **Dear ImGui** in **pure C11**, **header-only**, targeting
**WebAssembly**. It builds with `clang --target=wasm32` against **JAClibc** (a header-only,
WASM-first libc). **No Emscripten.** No C++ features. It is *not* bindings — it is a rewrite.

- `eli_` prefix on all public functions/types. snake_case. `ELI_` prefix on macros/enums/guards.
- Immediate-mode: no retained state except what is explicit. No hidden allocations.
- Single global context (`g_eli_context`) via `eli_get_current_context()`.

## 2. Read before writing (mandatory)

1. `coding-best-practices.md` (repo root) — **the coding standard. It overrides defaults.**
2. `CLAUDE.md` (repo root) — project structure, tech stack, naming, JS-interop notes.
3. `build_spec.md` — the full spec; consult the section for your phase for type/API detail.
4. `build_plan.md` — find **your phase**; its checklist is your scope of work. **Read only —
   never edit it** (the orchestrator tracks the plan).
5. `reference/imgui/` if present (gitignored) — Dear ImGui source for exact behavior/values.

## 3. Coding conventions (from coding-best-practices.md — non-negotiable)

- **File-header block on EVERY file**, all four sections always present (even if `None`):
  ```c
  /**
   * @file eli_draw_list.h
   * @brief One-to-three line description of what this header provides.
   *
   * @status None
   * @issues None
   * @todo None
   */
  ```
- **Function docs on every public API** — purpose, `@param`, `@return`, and thread-safety
  (`Thread-safe:` / `Reentrant:` — use `TBD` if unknown). Internal helpers only need docs when
  the name+signature aren't self-explanatory.
- **K&R braces:** opening brace on the **next line for function definitions**, **same line** for
  control flow. Omit braces for single-statement `if`/`for`/`while` bodies (but if any branch of
  an if/else chain uses braces, all do).
- **4-space indent, no tabs. 100-column lines.** Pointer star binds to the variable: `char *p`.
- **`sizeof(*ptr)` not `sizeof(Type)`.** Zero-init: `calloc` or `= {0}`. `free(p); p = NULL;`.
- **const correctness:** every pointer param is `const` unless it must be mutated.
- **`static` internal linkage** for anything not part of the public API. Header-only public
  functions are `static inline`.
- **Descriptive names.** No magic numbers — name constants (`ELI_...` `#define` or enum).
- **Delete dead code.** No commented-out blocks. No unused symbols.
- **Header guards** `ELI_<PATH>_H`, with the closing comment: `#endif /* ELI_..._H */`.
- **Include ordering:** the module's own/related header first, then jaclibc, then project
  headers — each group blank-line separated.

## 4. Structure — categorical nesting, header-only

Code lives under `include/eli/<category>/`. `elimgui.h` is the umbrella that includes each
category so `#include <eli/elimgui.h>` pulls in the whole library. Category → phase map is in
`CLAUDE.md`. Within a category, split by concern.

**Size targets (soft, not hard limits — aim for them, exceed only when splitting would hurt):**
- **~1000 lines of code per header** (doc comments don't count). Prefer splitting a module into
  focused headers before it grows past this.
- **~100 lines of code per function body** (the executable logic; doc block above doesn't count).
  Extract logical chunks into helpers before a function sprawls past this.

These two are guidelines, not gates. If a unit genuinely can't be cleanly split — a big lookup
table, one cohesive algorithm, a generated font blob — it may go over, but keep it **within
reason** and don't pad. Everything else in §3 (file headers, function docs, braces, const,
guards) is firm.

- New category headers must `#include` their own dependencies so they compile standalone.
- If your phase adds a new category, add its aggregator include to `elimgui.h` in the
  clearly-commented "feature modules" section (in dependency order). Editing `elimgui.h` for
  include-wiring and forward-declares is allowed; do not gratuitously rewrite existing sections.
- **Do not redefine** anything an earlier phase already defined. Grep first.

## 5. Build & verify (REQUIRED — a phase is not done until this passes)

Apple's `/usr/bin/clang` has no wasm32 target. Use the Homebrew LLVM clang that `build.sh` selects:

```
/opt/homebrew/opt/llvm/bin/clang --target=wasm32 -nostdlib \
    -Ivendor/jaclibc/include -Iinclude -Ivendor -O2 -c -o /tmp/<probe>.o /tmp/<probe>.c
```

- Write a small probe `.c` that includes `<eli/elimgui.h>` (or just your new header) and
  exercises the functions you added. Compile it. **Zero errors.** **No warning may originate
  from your headers.** JAClibc emits harmless warnings (WCHAR redefinition, float.h precedence,
  unknown warning group) — those are expected and fine.
- Also compile **each new header standalone** (a probe that includes only that header) to prove
  its includes are self-sufficient.
- Fix until clean. Do not report done with a dirty compile.

## 6. Testing (MANDATORY every phase — proper testing is a first-class requirement)

Every phase ships unit tests for the logic it adds. A phase is not done until its tests exist
and pass green. Tests run **natively on the host** — no wasm runtime, no Docker.

**How it works:**
- The library never includes `<jaclibc.h>` directly — it includes `"eli/core/eli_platform.h"`,
  the libc seam. Under `-DELI_TEST_HOSTED` that seam pulls the **host system libc** instead of
  jaclibc, so tests compile and run as normal native binaries. **Every new header you create must
  route its libc through `eli/core/eli_platform.h`, never `<jaclibc.h>` directly.**
- Wrap any JS-interop-only code (`JS_EXPORT`, `JS_CODE`, `jsio.h`) in `#ifdef ELI_JSIO` so it
  compiles out of hosted test builds. That glue is browser-only and is covered by the demo, not
  unit tests — don't try to unit-test it.

**Write tests:**
- One file per unit under `tests/unit/test_<thing>.c`. Use the framework `tests/eli_test.h`
  (`#include "eli_test.h"`): `ELI_TEST(name){ ... }`, assertions `ELI_ASSERT_TRUE/FALSE/EQ/NE/
  GT/LT/GE/LE/NULL/NOT_NULL/STR_EQ/FLT_NEAR`, and `ELI_TEST_MAIN()` at the end. Tests self-register.
- Cover the real logic your phase adds: exact values (hashes, colors, geometry/vertex output,
  text metrics, layout math), edge cases, and invariants — not just "it compiles". Aim for
  meaningful assertions, not trivial ones.

**Run tests (must be green before you report done):**
```
./build.sh test               # builds & runs every tests/unit/*.c natively
./build.sh test <name-filter> # only matching files
```
The runner compiles with `-std=c11 -Wall -Wextra -Werror -DELI_TEST_HOSTED`. Warnings are errors
in tests — keep them clean. If your logic genuinely can't be hosted-tested (pure JS-interop),
say so in your report and explain how it's otherwise verified.

## 7. Documentation (the final step of every phase)

After code compiles clean, write the phase's docs into `docs/`:
- Create `docs/<phase-doc>.md` (name per the plan's Documentation task). Include: overview of
  what the phase adds, the public API (each function: purpose, params, returns), a short usage
  example that would actually compile, and any gotchas.
- Update `docs/README.md` (the index) — add/adjust the link to your phase doc. If `docs/README.md`
  does not exist yet (Phase 1), create it as the index with your entry.

## 8. Guardrails (do NOT)

- ❌ Edit `build_plan.md` — the orchestrator checks the boxes after verifying your work.
- ❌ Edit `build.sh`, `CLAUDE.md`, `coding-best-practices.md`, `AGENT_BRIEF.md`, or
  `examples/demo/main.c` unless your per-phase task explicitly says to.
- ❌ Add AI/assistant attribution anywhere (no "Generated with…", no co-author, no 🤖).
- ❌ Introduce non-C11 or Emscripten-specific code, or hidden global allocations per frame.
- ❌ Leave stubs silently — if you defer something, say so in your report.

## 9. Report back (fixed format, concise)

1. **Files** — tree of everything created/modified under `include/eli/` and `docs/`, with
   per-file **line counts** (confirm each code header <1000 LOC).
2. **Compile** — confirm the wasm32 probe(s) compiled clean with the command above; note any
   residual jaclibc-only warnings.
3. **Tests** — the test files added under `tests/unit/`, and the exact `./build.sh test` result
   (suite counts / ALL GREEN). Summarize what behavior they cover.
4. **Checklist** — the exact `build_plan.md` items from your phase you completed (quote them so
   the orchestrator can tick them), plus any Documentation items done.
5. **Deferred** — anything intentionally stubbed/left, and why.

Your final message is consumed by the orchestrator, not a human — return structured facts, not
prose padding.
