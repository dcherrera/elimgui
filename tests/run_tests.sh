#!/bin/bash
#
# run_tests.sh — build and run elimgui unit tests natively.
#
# Each tests/unit/*.c is a self-contained test binary that uses tests/eli_test.h
# (host libc) and includes elimgui headers against the host system libc — no wasm
# runtime, no Docker, no dependency on JAClibc's per-OS native support.
#
# Two passes run by default:
#   hosted  -DELI_TEST_HOSTED (back-compat spelling; static per-TU state)
#   shared  -DELI_LIBC_SYSTEM -DELI_SHARED_STATE (the native-app build mode); the
#           test TU defines ELI_IMPLEMENTATION, so it owns the shared globals.
# A suite may have a second translation unit at tests/unit/companion/<name>_b.c; it is
# compiled without ELI_IMPLEMENTATION and linked in (proves multi-TU state, D4).
#
# Usage: tests/run_tests.sh [name-filter]
#   name-filter  optional substring; only unit files whose name matches are run.
#   ELI_TEST_PASSES="hosted shared" (env) picks the passes.
#   ELI_TEST_SANITIZE=1 (env) builds with -fsanitize=address,undefined.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
UNIT_DIR="${SCRIPT_DIR}/unit"
COMPANION_DIR="${UNIT_DIR}/companion"
BUILD_ROOT="${SCRIPT_DIR}/build"
FILTER="${1:-}"
PASSES="${ELI_TEST_PASSES:-hosted shared}"

# Host compiler (Apple clang resolves the macOS SDK automatically). Override with CC.
[[ -n "${CC:-}" ]] && CC_SET_BY_USER=1
CC="${CC:-cc}"
BASE_CFLAGS="-std=c11 -Wall -Wextra -Werror -g -I${ROOT_DIR}/include -I${ROOT_DIR}/vendor -I${SCRIPT_DIR}"
if [[ -n "${ELI_TEST_SANITIZE:-}" ]]; then
    # Apple clang 17's ASan runtime hangs at process start on macOS 26 (even for an
    # empty main); Homebrew LLVM's works. Prefer it on macOS unless CC is set.
    if [[ "$(uname -s)" == "Darwin" && -z "${CC_SET_BY_USER:-}" && -x /opt/homebrew/opt/llvm/bin/clang ]]; then
        CC="/opt/homebrew/opt/llvm/bin/clang"
        BASE_CFLAGS="${BASE_CFLAGS} -isysroot $(xcrun --show-sdk-path)"
    fi
    BASE_CFLAGS="${BASE_CFLAGS} -fsanitize=address,undefined -fno-omit-frame-pointer"
    BUILD_ROOT="${BUILD_ROOT}/sanitize"
fi

total=0
passed=0
failed_files=()

# run_suite <pass> <src> — build (main TU + optional companion) and run one suite.
run_suite() {
    local pass="$1" src="$2"
    local name build_dir mode_flags main_flags bin companion
    name="$(basename "${src}" .c)"
    case "${pass}" in
        hosted) build_dir="${BUILD_ROOT}"; mode_flags="-DELI_TEST_HOSTED"; main_flags="" ;;
        shared) build_dir="${BUILD_ROOT}/shared"
                mode_flags="-DELI_LIBC_SYSTEM -DELI_SHARED_STATE"
                main_flags="-DELI_IMPLEMENTATION" ;;
        *) echo "unknown pass ${pass}"; return 1 ;;
    esac
    mkdir -p "${build_dir}"
    mode_flags="${mode_flags} -DELI_TEST_OUT_DIR=\"${build_dir}\""
    bin="${build_dir}/${name}"
    companion="${COMPANION_DIR}/${name}_b.c"
    local objs=()
    if [[ -f "${companion}" ]]; then
        if ! ${CC} ${BASE_CFLAGS} ${mode_flags} -c "${companion}" -o "${bin}_b.o"; then
            return 2
        fi
        objs+=("${bin}_b.o")
    fi
    ${CC} ${BASE_CFLAGS} ${mode_flags} ${main_flags} "${src}" "${objs[@]+"${objs[@]}"}" -lm -o "${bin}" || return 2
    "${bin}" || return 1
}

shopt -s nullglob
for pass in ${PASSES}; do
    for src in "${UNIT_DIR}"/*.c; do
        name="$(basename "${src}" .c)"
        [[ -n "${FILTER}" && "${name}" != *"${FILTER}"* ]] && continue
        total=$((total + 1))
        echo "=== [${pass}] ${name} ==="
        run_suite "${pass}" "${src}"
        rc=$?
        if [[ ${rc} -eq 0 ]]; then
            passed=$((passed + 1))
        elif [[ ${rc} -eq 2 ]]; then
            echo "  BUILD FAILED"
            failed_files+=("[${pass}] ${name} (build)")
        else
            failed_files+=("[${pass}] ${name} (run)")
        fi
        echo
    done
done

echo "========================================"
echo "Suites: ${passed}/${total} passed (passes: ${PASSES})"
if [[ ${#failed_files[@]} -gt 0 ]]; then
    echo "Failed:"
    for f in "${failed_files[@]}"; do echo "  - ${f}"; done
    exit 1
fi
[[ ${total} -eq 0 ]] && { echo "No test files matched."; exit 0; }
echo "ALL GREEN"
