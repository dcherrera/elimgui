#!/bin/bash

# elimgui build script

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
JACLIBC_PATH="${SCRIPT_DIR}/vendor/jaclibc"

# Find a clang with WASM support
# Apple clang doesn't have WASM, so prefer Homebrew LLVM on macOS
if [[ -x "/opt/homebrew/opt/llvm/bin/clang" ]]; then
    CC="/opt/homebrew/opt/llvm/bin/clang"
elif [[ -x "/usr/local/opt/llvm/bin/clang" ]]; then
    CC="/usr/local/opt/llvm/bin/clang"
else
    CC="clang"
fi

# -Os: size-conscious default (~18% smaller wasm than -O2) with balanced runtime
# performance — a good fit for a browser-delivered, header-only library. For the
# smallest possible binary (~42% under -O2) at some runtime cost, use -Oz instead.
CFLAGS="--target=wasm32 -nostdlib -I${JACLIBC_PATH}/include -I${SCRIPT_DIR}/include -I${SCRIPT_DIR}/vendor -Os"
LDFLAGS="-Wl,--no-entry -Wl,--export-dynamic"

usage() {
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  demo      Build the demo example"
    echo "  cheatsheet  Build the visual cheatsheet app"
    echo "  test      Build and run native unit tests (tests/run_tests.sh)"
    echo "  wasm-smoke  Build demo + cheatsheet wasm and check their draw data headlessly (node)"
    echo "  sdl3-example        Build examples/sdl3 (-O2) -> examples/sdl3/build/elimgui_sdl3"
    echo "  sdl3-example-debug  Same at -O0 -g with ASan/UBSan -> elimgui_sdl3_debug"
    echo "                      ELI_SDL3_STATIC=1 links the static SDL3 from 'sdl3-deps'"
    echo "  sdl3-deps           Build SDL3 from source as a static lib into examples/sdl3/build/deps"
    echo "  clean     Remove build artifacts"
    echo "  serve     Build + serve the cheatsheet and open it in the browser"
    echo "            (use 'serve demo.html' for the widget demo)"
    echo "  help      Show this help"
    echo ""
}

run_tests() {
    exec "${SCRIPT_DIR}/tests/run_tests.sh" "$@"
}

# Build + run the headless cheatsheet dogfooding harness natively (host libc via
# the ELI_TEST_HOSTED seam) — diagnoses UI behavior without a browser.
build_dogfood() {
    local HOST_CC="${CC_HOST:-cc}"
    mkdir -p "${SCRIPT_DIR}/tests/build"
    "${HOST_CC}" -std=c11 -g -DELI_TEST_HOSTED \
        -I"${SCRIPT_DIR}/include" -I"${SCRIPT_DIR}/vendor" -I"${SCRIPT_DIR}/examples/cheatsheet" \
        "${SCRIPT_DIR}/examples/cheatsheet/dogfood.c" \
        -o "${SCRIPT_DIR}/tests/build/dogfood" || return 1
    "${SCRIPT_DIR}/tests/build/dogfood"
}

build_demo() {
    echo "Building demo..."
    mkdir -p "${SCRIPT_DIR}/web"
    ${CC} ${CFLAGS} ${LDFLAGS} \
        -o "${SCRIPT_DIR}/web/demo.wasm" \
        "${SCRIPT_DIR}/examples/demo/main.c"
    echo "Built: web/demo.wasm"
}

build_cheatsheet() {
    echo "Building cheatsheet..."
    mkdir -p "${SCRIPT_DIR}/web"
    ${CC} ${CFLAGS} ${LDFLAGS} \
        -o "${SCRIPT_DIR}/web/cheatsheet.wasm" \
        "${SCRIPT_DIR}/examples/cheatsheet/main.c"
    echo "Built: web/cheatsheet.wasm"
}

# Build both wasm apps and walk their draw data in Node the way web/elimgui.js
# does (layout + index-width check, no browser).
wasm_smoke() {
    build_demo && build_cheatsheet || return 1
    node "${SCRIPT_DIR}/tests/web/wasm_smoke.mjs" \
        "${SCRIPT_DIR}/web/demo.wasm" "${SCRIPT_DIR}/web/cheatsheet.wasm"
}

SDL3_EXAMPLE_DIR="${SCRIPT_DIR}/examples/sdl3"
SDL3_DEPS_DIR="${SDL3_EXAMPLE_DIR}/build/deps"
SDL3_VERSION="${SDL3_VERSION:-3.4.10}"

# sdl3_pkg_flags — cflags + libs for SDL3: the static build from sdl3-deps when
# ELI_SDL3_STATIC=1, else the system pkg-config sdl3 (shared).
sdl3_pkg_flags() {
    if [[ -n "${ELI_SDL3_STATIC:-}" ]]; then
        PKG_CONFIG_PATH="${SDL3_DEPS_DIR}/prefix/lib/pkgconfig:${SDL3_DEPS_DIR}/prefix/lib64/pkgconfig" \
            pkg-config --static --cflags --libs sdl3
    else
        pkg-config --cflags --libs sdl3
    fi
}

# build_sdl3_example <mode> — mode is "release" or "debug".
build_sdl3_example() {
    local mode="$1" out flags sdl_flags host_cc="${CC_HOST:-cc}"
    sdl_flags="$(sdl3_pkg_flags)" || { echo "SDL3 not found (pkg-config sdl3)"; return 1; }
    mkdir -p "${SDL3_EXAMPLE_DIR}/build"
    if [[ "${mode}" == "debug" ]]; then
        out="${SDL3_EXAMPLE_DIR}/build/elimgui_sdl3_debug"
        flags="-O0 -g -DELI_DEBUG -fsanitize=address,undefined -fno-omit-frame-pointer"
        # Apple clang's ASan hangs at startup on macOS 26; use Homebrew LLVM there.
        if [[ "$(uname -s)" == "Darwin" && -z "${CC_HOST:-}" && -x /opt/homebrew/opt/llvm/bin/clang ]]; then
            host_cc="/opt/homebrew/opt/llvm/bin/clang"
            flags="${flags} -isysroot $(xcrun --show-sdk-path)"
        fi
    else
        out="${SDL3_EXAMPLE_DIR}/build/elimgui_sdl3"
        flags="-O2"
    fi
    # shellcheck disable=SC2086
    "${host_cc}" -std=c11 ${flags} -Wall -Wextra -Werror \
        -DELI_SHARED_STATE -DELI_DRAW_IDX_32 \
        -I"${SCRIPT_DIR}/include" -I"${SCRIPT_DIR}/vendor" -I"${SCRIPT_DIR}/backends" \
        "${SDL3_EXAMPLE_DIR}/main.c" "${SDL3_EXAMPLE_DIR}/shell.c" "${SDL3_EXAMPLE_DIR}/perf_fixture.c" \
        "${SDL3_EXAMPLE_DIR}/tasks.c" \
        ${sdl_flags} -o "${out}" || return 1
    echo "Built: ${out#"${SCRIPT_DIR}/"}"
}

# Build SDL3 from the release tarball as a static library (no shared lib) into
# examples/sdl3/build/deps/prefix. Never installs system-wide.
build_sdl3_deps() {
    local src="${SDL3_DEPS_DIR}/SDL3-${SDL3_VERSION}"
    mkdir -p "${SDL3_DEPS_DIR}"
    if [[ ! -d "${src}" ]]; then
        curl -fL -o "${SDL3_DEPS_DIR}/SDL3-${SDL3_VERSION}.tar.gz" \
            "https://github.com/libsdl-org/SDL/releases/download/release-${SDL3_VERSION}/SDL3-${SDL3_VERSION}.tar.gz" \
            && tar -xzf "${SDL3_DEPS_DIR}/SDL3-${SDL3_VERSION}.tar.gz" -C "${SDL3_DEPS_DIR}" || return 1
    fi
    cmake -S "${src}" -B "${SDL3_DEPS_DIR}/build" -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${SDL3_DEPS_DIR}/prefix" -DSDL_SHARED=OFF -DSDL_STATIC=ON \
        -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF ${SDL3_CMAKE_ARGS:-} || return 1
    cmake --build "${SDL3_DEPS_DIR}/build" -j "${SDL3_JOBS:-4}" && cmake --install "${SDL3_DEPS_DIR}/build"
}

clean() {
    echo "Cleaning..."
    rm -f "${SCRIPT_DIR}/web/"*.wasm
    rm -f "${SCRIPT_DIR}/"*.o
    rm -rf "${SCRIPT_DIR}/tests/build"
    echo "Done."
}

# serve [page] — build the app, start a local server, and open it in the browser.
# Defaults to the visual cheatsheet; pass `demo.html` for the widget demo.
serve() {
    local page="${1:-cheatsheet.html}"
    case "$page" in
        demo.html) build_demo ;;
        *)         build_cheatsheet ;;
    esac
    local url="http://localhost:8080/${page}"
    echo "Serving web/ at ${url}  (Ctrl-C to stop)"
    # Open the browser once the server is up (macOS `open`, Linux `xdg-open`).
    ( sleep 1
      if command -v open >/dev/null 2>&1; then open "${url}"
      elif command -v xdg-open >/dev/null 2>&1; then xdg-open "${url}"; fi ) >/dev/null 2>&1 &
    cd "${SCRIPT_DIR}/web"
    python3 -m http.server 8080
}

case "${1:-help}" in
    demo)
        build_demo
        ;;
    cheatsheet)
        build_cheatsheet
        ;;
    dogfood)
        build_dogfood
        ;;
    wasm-smoke)
        wasm_smoke
        ;;
    sdl3-example)
        build_sdl3_example release
        ;;
    sdl3-example-debug)
        build_sdl3_example debug
        ;;
    sdl3-deps)
        build_sdl3_deps
        ;;
    test)
        shift || true
        run_tests "$@"
        ;;
    clean)
        clean
        ;;
    serve)
        shift || true
        serve "$@"
        ;;
    help|--help|-h)
        usage
        ;;
    *)
        echo "Unknown command: $1"
        usage
        exit 1
        ;;
esac
