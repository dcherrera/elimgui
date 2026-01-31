# Project Guidelines

## Project Structure

```
elimgui/
├── CLAUDE.md             # AI assistant guidelines
├── build_spec.md         # Full project specification
├── build_plan.md         # Phased task checklist
├── build.sh              # Build script
├── .gitignore
├── include/
│   └── eli/
│       ├── elimgui.h     # Master include, core types
│       ├── eli_draw.h    # Draw primitives
│       ├── eli_widgets.h # All widgets
│       ├── eli_layout.h  # Layout system
│       ├── eli_input.h   # Input handling
│       ├── eli_font.h    # Font system
│       ├── eli_style.h   # Styling
│       ├── eli_tables.h  # Table widget
│       └── eli_docking.h # Docking system (future)
├── docs/
│   ├── README.md         # Documentation index/table of contents
│   ├── core-types.md     # Phase 1: Foundation & Core Types
│   ├── draw-system.md    # Phase 2: Draw System
│   ├── font-system.md    # Phase 3: Font System
│   └── ...               # Additional docs per phase
├── examples/
│   └── demo/             # Full widget demo
├── tests/
├── web/
│   ├── elimgui.js        # WASM loader
│   └── index.html        # Demo shell
├── vendor/
│   └── jaclibc/          # JAClibc submodule (git submodule)
└── reference/            # Dear ImGui clone (gitignored)
    └── imgui/
```

## Terminology

- **"build spec"** or **"spec"** refers to `build_spec.md` - the full project description
- **"build plan"** or **"plan"** refers to `build_plan.md` - the phased task checklist
- **elimgui** - Pure C rewrite of Dear ImGui (NOT bindings)
- **Dear ImGui** - The original C++ library we're reimplementing

## Tech Stack

- **Language**: C11 (header-only)
- **Libc**: JAClibc (header-only, WASM-first)
- **Target**: WebAssembly (browser)
- **Compiler**: clang --target=wasm32 (NO Emscripten)
- **Fonts**: stb_truetype.h (vendored)

## JAClibc Dependency

JAClibc is the foundation for all C-to-JS interop via `jsio.h`. It is vendored as a git submodule.

**Location:** `vendor/jaclibc/` (git submodule from https://github.com/dcherrera/jaclibc.git)

**After cloning, initialize submodules:**
```bash
git submodule update --init --recursive
```

**Key files from JAClibc:**
- `include/jsio.h` - JS interop (JS_CODE, JS_EXPORT, JS_IMPORT)
- `include/core/jsio.h` - Implementation
- `web/loadWASM.js` - WASM loader base

**Include path for compilation:**
```bash
-Ivendor/jaclibc/include
```

## Build Script

```bash
./build.sh [options] [target]

# Examples:
./build.sh demo           # Build examples/demo
./build.sh clean          # Remove build artifacts
./build.sh serve          # Start dev server
```

### Compilation Command

```bash
clang --target=wasm32 \
    -nostdlib \
    -Ivendor/jaclibc/include \
    -Iinclude \
    -O2 \
    -Wl,--no-entry \
    -Wl,--export-dynamic \
    -o web/demo.wasm \
    examples/demo/main.c
```

**Flags explained:**
- `--target=wasm32` - Target WebAssembly
- `-nostdlib` - Don't link system libc (we use JAClibc)
- `-Wl,--no-entry` - No _start, we use js_start() from JAClibc
- `-Wl,--export-dynamic` - Export all JS_EXPORT functions

## Testing Changes

After code changes, rebuild and test in browser:

```bash
./build.sh demo
./build.sh serve    # Opens http://localhost:8080
```

Or manually open `web/index.html` in browser.

## Code Style

### Naming Conventions

- `eli_` prefix for all public functions and types
- Snake_case for functions: `eli_draw_rect()`, `eli_button()`
- Types: `eli_vec2`, `eli_context`, `eli_draw_list`
- Constants/macros: `ELI_WINDOW_NO_TITLEBAR`, `ELI_COL32`

### Header-Only Pattern

All code in headers with include guards:

```c
#ifndef ELI_DRAW_H
#define ELI_DRAW_H

#include <jaclibc.h>

// Declarations
void eli_draw_rect(float x, float y, float w, float h, uint32_t col);

// Implementation
static inline void eli_draw_rect(float x, float y, float w, float h, uint32_t col) {
    // ...
}

#endif // ELI_DRAW_H
```

### JS Interop via JAClibc

Use `JS_CODE` macro for JavaScript execution:

```c
// Execute JS and get result
js_t* result = JS_CODE(
    return document.getElementById('canvas').getContext('2d')
);
```

Use `JS_EXPORT` for functions callable from JS:

```c
JS_EXPORT(on_mouse_move)
void on_mouse_move(float x, float y) {
    eli_io.mouse_pos.x = x;
    eli_io.mouse_pos.y = y;
}
```

## Communication Rules

**Always explain before acting.** Before running any command or making significant changes:
- Give a short, concise explanation of WHAT you're about to do and WHY
- For complex changes, outline the approach first

## Commit Rules

- **Never mention Claude, AI, or "Generated with Claude Code" in commits**
- All commits are authored by the developer - no co-author tags
- Write commit messages in first person as the developer
- Keep commit messages concise and descriptive

## Development Approach

1. **Header-only** - All code in headers, implementation inline
2. **WASM-first** - Test in browser, not native
3. **Reference ImGui** - Check `reference/imgui/` for implementation details
4. **Minimal JS** - Use JS only for browser APIs, keep logic in C
5. **No Emscripten** - Direct clang to wasm32

## API Design Principles

1. **Match ImGui where sensible** - Familiar API for ImGui users
2. **C11 compatible** - No C++ features
3. **No hidden allocations** - User controls memory
4. **Immediate mode** - No retained state except what's explicit
5. **Renderer-agnostic** - Output draw lists, not pixels

## Reference Material

Dear ImGui source is cloned in `reference/imgui/` for implementation reference.

Key files to study:
- `imgui.h` - Public API
- `imgui.cpp` - Core implementation
- `imgui_draw.cpp` - Drawing and font atlas
- `imgui_widgets.cpp` - Widget implementations
- `imgui_tables.cpp` - Table widget

---

# Project-Specific Configuration

## Project Name

**elimgui** - Pure C Immediate Mode GUI Library

## Description

A ground-up reimplementation of Dear ImGui in pure C11, targeting WebAssembly. Designed for use with JAClibc and the Canvil framework, but usable standalone.

## Key Differences from Dear ImGui

| Aspect | Dear ImGui | elimgui |
|--------|-----------|---------|
| Language | C++ | C11 |
| Headers | imgui.h | elimgui.h (+ eli_*.h) |
| Prefix | ImGui:: | eli_ |
| Target | Multi-platform | WASM-first |
| Dependencies | None | JAClibc |

## External Dependencies

| Dependency | Location | Purpose |
|------------|----------|---------|
| JAClibc | `vendor/jaclibc/` (git submodule) | Header-only libc with JS interop |
| stb_truetype | vendored (future) | TTF font parsing |

## URLs

| Environment | URL |
|-------------|-----|
| Local dev | http://localhost:8080 |

## Data Flow

```
User code → eli_*() widgets → eli_draw_list → eli_draw_data → Your renderer → Canvas
```
