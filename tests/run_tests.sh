#!/bin/bash
#
# run_tests.sh — build and run elimgui unit tests natively.
#
# Each tests/unit/*.c is a self-contained test binary that uses tests/eli_test.h
# (host libc) and includes elimgui headers built with -DELI_TEST_HOSTED, so the
# library is exercised against the host system libc — no wasm runtime, no Docker,
# no dependency on JAClibc's per-OS native support.
#
# Usage: tests/run_tests.sh [name-filter]
#   name-filter  optional substring; only unit files whose name matches are run.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
UNIT_DIR="${SCRIPT_DIR}/unit"
BUILD_DIR="${SCRIPT_DIR}/build"
FILTER="${1:-}"

# Host compiler (Apple clang resolves the macOS SDK automatically). Override with CC.
CC="${CC:-cc}"
CFLAGS="-std=c11 -Wall -Wextra -Werror -g -DELI_TEST_HOSTED -I${ROOT_DIR}/include -I${ROOT_DIR}/vendor -I${SCRIPT_DIR}"

mkdir -p "${BUILD_DIR}"

total=0
passed=0
failed_files=()

shopt -s nullglob
for src in "${UNIT_DIR}"/*.c; do
    name="$(basename "${src}" .c)"
    [[ -n "${FILTER}" && "${name}" != *"${FILTER}"* ]] && continue

    total=$((total + 1))
    bin="${BUILD_DIR}/${name}"
    echo "=== ${name} ==="

    if ! ${CC} ${CFLAGS} "${src}" -o "${bin}"; then
        echo "  BUILD FAILED"
        failed_files+=("${name} (build)")
        continue
    fi

    if "${bin}"; then
        passed=$((passed + 1))
    else
        failed_files+=("${name} (run)")
    fi
    echo
done

echo "========================================"
echo "Suites: ${passed}/${total} passed"
if [[ ${#failed_files[@]} -gt 0 ]]; then
    echo "Failed:"
    for f in "${failed_files[@]}"; do echo "  - ${f}"; done
    exit 1
fi
[[ ${total} -eq 0 ]] && { echo "No test files matched."; exit 0; }
echo "ALL GREEN"
