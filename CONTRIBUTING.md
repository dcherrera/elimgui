# Contributing to elimgui

Thanks for your interest in elimgui. This document covers the basics.

## Licensing of contributions — read this first

elimgui ships under **AGPL-3.0-or-later** (see [`LICENSE`](./LICENSE)).

> **Contributions are made under the Contributor License Agreement in
> [`CLA.md`](./CLA.md)**, which grants me a broad copyright and patent license,
> including the right to sublicense your contribution under any terms and to
> transfer that right along with the project.

**You keep your copyright.** The CLA is a license, not an assignment — nothing
is transferred away from you. Your contribution stays yours to use, publish,
sell, or license to anyone else, anywhere, forever.

Signing takes one commit: add your entry to
[`.cla/signatures.md`](./.cla/signatures.md) in your first pull request. Nothing
to print, scan, or email. Organizations whose employees contribute on company
time sign [`CLA-CORPORATE.md`](./CLA-CORPORATE.md) by email instead.

If you would rather not sign, open an issue describing the change instead of a
pull request. An idea costs nothing to accept.

### Sign your commits (DCO)

Every commit must carry a `Signed-off-by` line certifying the
[Developer Certificate of Origin](https://developercertificate.org/) — that you
wrote the code, or otherwise have the right to submit it:

```bash
git commit -s -m "your message"
```

which appends:

```
Signed-off-by: Your Name <your.email@example.com>
```

Use your real name and a working email address — the same address as your CLA
roster entry, since that is what CI matches on. If you are contributing work your
employer owns, confirm you actually have the right to do so before you sign off.

These checks run in CI on every pull request, and you can run them yourself first:

```bash
./scripts/check-dco.sh     # every commit signed off, matching its author
./scripts/check-cla.sh     # every author on the CLA roster
./scripts/check-headers.sh # every source file you ADD has SPDX + the @file block
```

### License header on new files

Every source file starts with:

```c
/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
```

followed by the `@file` / `@brief` / `@status` / `@issues` / `@todo` block. Add
both to new files you create. Do not edit the copyright line on existing files
— you keep your copyright in what you contribute without needing a line in the
header. `git log` and `.cla/signatures.md` are the authoritative record of who
wrote what.

Third-party code keeps its own license header and gets an entry in
[`NOTICE`](./NOTICE).

## Getting Started

```bash
git submodule update --init --recursive   # JAClibc, for the wasm builds
./build.sh test                           # native unit tests
./build.sh demo                           # build the widget demo (wasm)
./build.sh cheatsheet                     # build the visual cheatsheet (wasm)
```

See the README for the other `build.sh` targets.

## Code Style

- **C11**, header-only, organized by category under `include/eli/`.
- **`eli_` prefix** on all public symbols.
- No hidden allocations; no POSIX assumptions in the library.
- Files stay small (about 1,000 lines of code at most), functions short.

## Pull Requests

**One logical change per PR.** Title is `scope: imperative summary`, ≤72
characters; the body says what changed and why in a few lines. A test you call
green should be shown able to go red. No tool, assistant, or generator
attribution anywhere — title, body, or comments.

**Merge bar:** builds clean with zero warnings, `./build.sh test` passes, DCO
and CLA checks green.

## Reporting Bugs

Open an issue with what you expected, what happened, minimal reproduction steps,
and your platform (browser or native backend, OS, compiler version).
