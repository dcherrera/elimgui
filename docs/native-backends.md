# Native Backends (Phase 35)

How to use elimgui outside the browser: build modes, consumer setup, and (as later
phases land) the SDL3 backend, the desktop example, idle redraw and platform notes.

---

## Build modes

### libc seam (`core/eli_platform.h`)

| Macro | Libc | Default for | Notes |
|---|---|---|---|
| `ELI_LIBC_SYSTEM` | host libc (`stdint` ... `float`, `stdio`) | every non-wasm32 target | Native apps and unit tests. `ELI_TEST_HOSTED` is an alias. |
| `ELI_LIBC_JACLIBC` | JAClibc (`vendor/jaclibc`) | wasm32 | On wasm32 it also turns on `ELI_JSIO` (JS interop) and the double-only `pow/log/exp` overrides. Natively it is opt-in and JS interop stays off. |

Defining both is a compile error. `ELI_ASSERT(x)` checks library invariants only when
`ELI_DEBUG` is defined (system libc); otherwise it compiles to nothing.

### Shared state across translation units (`core/eli_state.h`)

All library state that lives at file scope (current context, widget pools, clipboard
fallback, allocator hooks, popup stacks, ...) is declared with `ELI_GLOBAL` /
`ELI_GLOBAL_ARRAY` / `ELI_GLOBAL_ARRAY_INIT`.

- **Default:** `static` in every TU. One TU per program (the WASM model). Two TUs
  still link, but each gets its own context, clipboard and allocator.
- **`ELI_SHARED_STATE`:** `extern` in every TU and defined once, in the TU that
  defines `ELI_IMPLEMENTATION` before including elimgui. That TU also compiles the
  stb_truetype / stb_rect_pack bodies (external linkage); other TUs see declarations.

New file-scope state must use these macros (the check is
`grep -rn '^static [^(]*;' include/eli | grep -v 'static inline'`, which must print nothing).

### Host services (`util/eli_host_io.h`)

| Service | WASM | Native (`ELI_LIBC_SYSTEM`) |
|---|---|---|
| Settings file (`eli_load/save_ini_settings_*_disk`) | JS host import | `fopen`/`fread`/`fwrite` |
| Log to TTY / file | JS host import | `stdout` / append to the file |
| Open URL (`eli_platform_open_url`, `eli_text_link_open_url`) | `window.open` | `io.platform_open_url_fn(io.platform_user_data, url)`; NULL = no-op |
| Clipboard | fallback buffer + JS bridge | `io` clipboard callbacks set by the backend |

`io.ini_filename` defaults to NULL, so nothing is written unless the app asks. On iOS
the cwd is the read-only bundle: pass an absolute path (e.g. from `SDL_GetPrefPath`).

---

## Consumer setup

- Include paths: `elimgui/include`, `elimgui/vendor` (stb), and later `elimgui/backends`.
- Build flags in every TU: `-DELI_SHARED_STATE -DELI_DRAW_IDX_32` (recommended natively).
- Exactly one TU:

  ```c
  #define ELI_IMPLEMENTATION
  #include <eli/elimgui.h>
  ```

  Other TUs include `<eli/elimgui.h>` only.
- If the app also compiles stb_truetype or stb_rect_pack itself, those symbols clash
  with the copies in the `ELI_IMPLEMENTATION` TU under `ELI_SHARED_STATE`.

---

## JAClibc on native targets (probe)

`tests/probe/jaclibc_native_probe.sh` builds every unit suite with `-DELI_LIBC_JACLIBC`
against `vendor/jaclibc` and the system compiler. A target may use JAClibc natively only
if every suite builds and passes with `-Werror`. Results are recorded, not fixed here
(JAClibc is a separate project).

| Target | Date | Result | First errors |
|---|---|---|---|
| macOS arm64 (Apple clang 17) | 2026-10-27 | 0/70 build | JAClibc's `sys/time.h:286` calls undeclared `clock_settime`; `threads.h:370` calls undeclared `pthread_cond_timedwait` (both in every TU) |

Verdict: native builds use the system libc.
