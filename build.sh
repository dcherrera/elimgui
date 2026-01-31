#!/bin/bash

# elimgui build script

set -e

JACLIBC_PATH="${SCRIPT_DIR}/vendor/jaclibc"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

CC="clang"
CFLAGS="--target=wasm32 -nostdlib -I${JACLIBC_PATH}/include -I${SCRIPT_DIR}/include -O2"
LDFLAGS="-Wl,--no-entry -Wl,--export-dynamic"

usage() {
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  demo      Build the demo example"
    echo "  clean     Remove build artifacts"
    echo "  serve     Start local dev server"
    echo "  help      Show this help"
    echo ""
}

build_demo() {
    echo "Building demo..."
    mkdir -p "${SCRIPT_DIR}/web"
    ${CC} ${CFLAGS} ${LDFLAGS} \
        -o "${SCRIPT_DIR}/web/demo.wasm" \
        "${SCRIPT_DIR}/examples/demo/main.c"
    echo "Built: web/demo.wasm"
}

clean() {
    echo "Cleaning..."
    rm -f "${SCRIPT_DIR}/web/"*.wasm
    rm -f "${SCRIPT_DIR}/"*.o
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
