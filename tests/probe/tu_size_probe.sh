#!/bin/bash
#
# tu_size_probe.sh — D4 measurement: binary size of the same app built as one
# translation unit vs five (ELI_SHARED_STATE, header-only static inline code is
# emitted once per TU that uses it). Output in tests/build/tu_size/ (gitignored).
#
# Usage: tests/probe/tu_size_probe.sh [opt-level, default -O2]

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
SRC="${SCRIPT_DIR}/tu_size"
OUT="${ROOT_DIR}/tests/build/tu_size"
OPT="${1:--O2}"
CC="${CC:-cc}"
FLAGS="-std=c11 ${OPT} -Wall -Wextra -Werror -DELI_SHARED_STATE -DELI_DRAW_IDX_32 -I${ROOT_DIR}/include -I${ROOT_DIR}/vendor"
mkdir -p "${OUT}"

# shellcheck disable=SC2086
${CC} ${FLAGS} "${SRC}/unity.c" -o "${OUT}/one_tu"
# shellcheck disable=SC2086
${CC} ${FLAGS} "${SRC}/probe_main.c" "${SRC}"/probe_ui_[1-4].c -o "${OUT}/five_tu"
"${OUT}/one_tu" && "${OUT}/five_tu"
strip -o "${OUT}/one_tu.stripped" "${OUT}/one_tu" 2>/dev/null || cp "${OUT}/one_tu" "${OUT}/one_tu.stripped"
strip -o "${OUT}/five_tu.stripped" "${OUT}/five_tu" 2>/dev/null || cp "${OUT}/five_tu" "${OUT}/five_tu.stripped"
size_of() { wc -c < "$1" | tr -d ' '; }
one=$(size_of "${OUT}/one_tu.stripped"); five=$(size_of "${OUT}/five_tu.stripped")
echo "$(uname -sm) ${CC} ${OPT}: 1 TU ${one} B, 5 TU ${five} B (+$(( (five - one) * 100 / one ))%)"
# Code+data section sizes (file sizes are page-aligned on Mach-O).
size "${OUT}/one_tu" "${OUT}/five_tu"
