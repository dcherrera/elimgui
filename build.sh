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
    echo "  clean     Remove build artifacts"
    echo "  serve     Start local dev server"
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

clean() {
    echo "Cleaning..."
    rm -f "${SCRIPT_DIR}/web/"*.wasm
    rm -f "${SCRIPT_DIR}/"*.o
    rm -rf "${SCRIPT_DIR}/tests/build"
    echo "Done."
}

serve() {
    echo "Starting server at http://localhost:8080"
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
    test)
        shift || true
        run_tests "$@"
        ;;
    clean)
        clean
        ;;
    serve)
        serve
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
