#!/bin/bash
#
# jaclibc_native_probe.sh — spec §4.3 probe: can the unit tests build and pass
# natively against JAClibc (-DELI_LIBC_JACLIBC) instead of the system libc?
#
# Records results only; JAClibc is a separate project and is not fixed here.
# Output goes to tests/build/jaclibc_probe/ (gitignored). Prints one line per
# suite (BUILD-FAIL / RUN-FAIL / PASS), then a summary and the first errors.
#
# Usage: tests/probe/jaclibc_native_probe.sh [-Werror]

set -uo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
OUT="${ROOT_DIR}/tests/build/jaclibc_probe"
CC="${CC:-cc}"
EXTRA="${1:-}"
mkdir -p "${OUT}"
: > "${OUT}/errors.log"

pass=0; build_fail=0; run_fail=0
for src in "${ROOT_DIR}"/tests/unit/*.c; do
    name="$(basename "${src}" .c)"
    if ${CC} -std=c11 -g ${EXTRA} -DELI_LIBC_JACLIBC -I"${ROOT_DIR}/vendor/jaclibc/include" \
         -I"${ROOT_DIR}/include" -I"${ROOT_DIR}/vendor" -I"${ROOT_DIR}/tests" \
         "${src}" -lm -o "${OUT}/${name}" >"${OUT}/${name}.log" 2>&1; then
        if "${OUT}/${name}" >/dev/null 2>&1; then
            pass=$((pass + 1)); echo "PASS       ${name}"
        else
            run_fail=$((run_fail + 1)); echo "RUN-FAIL   ${name}"
        fi
    else
        build_fail=$((build_fail + 1)); echo "BUILD-FAIL ${name}"
        { echo "== ${name}"; grep -m3 'error' "${OUT}/${name}.log"; } >> "${OUT}/errors.log"
    fi
done
echo "summary: pass=${pass} build_fail=${build_fail} run_fail=${run_fail} (${CC} $(uname -sm))"
echo "first errors:"; grep -v '^==' "${OUT}/errors.log" | sed 's/^[^ ]*: //' | sort | uniq -c | sort -rn | head -8
