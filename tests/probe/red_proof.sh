#!/bin/bash
#
# red_proof.sh — prove a test can fail: apply a perl substitution to a source
# file, rebuild + run the matching suites (hosted pass) from scratch, then restore
# the file. A useful mutation makes the suite go red.
#
# Usage: tests/probe/red_proof.sh <file> '<perl -0pe expression>' <test-filter>
#   e.g. tests/probe/red_proof.sh include/eli/core/eli_redraw.h \
#          's/if \(ctx->redraw_input_seen/if (0 \&\& ctx->redraw_input_seen/' redraw
#
# Exit status: 0 if the mutation applied AND the suites went red, 1 otherwise.

set -uo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
file="$1"; expr="$2"; filter="$3"
backup="${ROOT_DIR}/tests/build/red_proof.bak"
mkdir -p "${ROOT_DIR}/tests/build"
cp "${ROOT_DIR}/${file}" "${backup}"
trap 'cp "${backup}" "${ROOT_DIR}/${file}"' EXIT
perl -0pi -e "${expr}" "${ROOT_DIR}/${file}"
if cmp -s "${backup}" "${ROOT_DIR}/${file}"; then
    echo "red_proof: mutation did not change ${file}"
    exit 1
fi
rm -rf "${ROOT_DIR}"/tests/build/test_*"${filter}"*
out="$(ELI_TEST_PASSES=hosted "${ROOT_DIR}/tests/run_tests.sh" "${filter}" 2>&1)"
echo "${out}" | grep -E '\[FAIL\]|BUILD FAILED|^Suites'
echo "${out}" | grep -q '^ALL GREEN' && { echo "red_proof: still GREEN"; exit 1; }
exit 0
