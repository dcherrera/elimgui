#!/bin/bash
# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 David (Corey) Herrera
#
# red_proof_live_sdl3.sh — the Core Animation live widget mutations, each through
# red_proof_sdl3.sh (example rebuilt, --live-selftest run, file restored). Every
# line must go red. macOS; opens the example window briefly per mutation.
#
# Usage: tests/probe/red_proof_live_sdl3.sh
# Exit status: 0 when every mutation went red.

set -uo pipefail
P="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/red_proof_sdl3.sh"
fails=0
run() {
    echo "== $1"
    "${P}" "$2" "$3" --live-selftest || fails=$((fails + 1))
}
B=backends/sdl3/eli_sdl3_live.h
run "the dot gets no pulse animation" "${B}" \
    's/if \(w->pulse\.period > 0\.0f\)\n            eli_sdl3__animate/if (0)\n            eli_sdl3__animate/'
run "the container is not flipped to y down" "${B}" \
    's/"contentsAreFlipped"\) \? NO : YES\)/"contentsAreFlipped") ? YES : NO)/'
run "visibility never reaches the layer" "${B}" \
    's/if \(changed & ELI_LIVE_CHANGED_VISIBLE\)/if (0)/'
run "a setter's value never reaches the layer" "${B}" \
    's/if \(changed & \(ELI_LIVE_CHANGED_GEOMETRY \| ELI_LIVE_CHANGED_VALUE\)\)/if (changed \& ELI_LIVE_CHANGED_GEOMETRY)/'
run "a layer widget still asks for the 12.5 fps redraw" include/eli/live/eli_live_widgets.h \
    's/(            eli_live__draw_main\(dl, w, false, 0\.0\);\n)/$1        eli_request_redraw_after(0.08f);\n/'
echo "red_proof_live_sdl3: ${fails} mutation(s) stayed green"
[[ ${fails} -eq 0 ]]
