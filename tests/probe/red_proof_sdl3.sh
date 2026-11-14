#!/bin/bash
# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 David (Corey) Herrera
#
# red_proof_sdl3.sh — prove an SDL3 example selftest can fail: apply a perl
# substitution to a source file, rebuild the example from scratch (old binary
# deleted), run the selftest, then restore the file and rebuild. A useful mutation
# makes the selftest exit non-zero. Opens the example window briefly.
#
# Usage: tests/probe/red_proof_sdl3.sh <file> '<perl -0pe expression>' <selftest flag>
#   e.g. tests/probe/red_proof_sdl3.sh backends/sdl3/eli_sdl3_renderer.h \
#          's/return cfg->release_when_occluded/return false/' --swapchain-selftest
#
# Exit status: 0 if the mutation applied AND the selftest went red, 1 otherwise.

set -uo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
file="$1"; expr="$2"; flag="$3"
bin="${ROOT_DIR}/examples/sdl3/build/elimgui_sdl3"
backup="${ROOT_DIR}/examples/sdl3/build/red_proof.bak"
mkdir -p "${ROOT_DIR}/examples/sdl3/build"
cp "${ROOT_DIR}/${file}" "${backup}"
rebuild() { rm -f "${bin}" && "${ROOT_DIR}/build.sh" sdl3-example >/dev/null; }
trap 'cp "${backup}" "${ROOT_DIR}/${file}"; rebuild' EXIT
perl -0pi -e "${expr}" "${ROOT_DIR}/${file}"
if cmp -s "${backup}" "${ROOT_DIR}/${file}"; then
    echo "red_proof_sdl3: mutation did not change ${file}"
    exit 1
fi
rebuild || { echo "red_proof_sdl3: mutated build failed"; exit 1; }
"${bin}" "${flag}" 2>&1 | grep -E 'FAIL|failed$' | sed 's/.*\] //'
status=${PIPESTATUS[0]}
[[ ${status} -ne 0 ]] || { echo "red_proof_sdl3: still GREEN"; exit 1; }
exit 0
