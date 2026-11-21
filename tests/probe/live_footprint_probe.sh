#!/bin/bash
# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 David (Corey) Herrera
#
# live_footprint_probe.sh — graphics footprint of the SDL3 example (macOS) in three
# runs, all with the idle swapchain release at 1 s and the window visible:
#   idle      nothing animates
#   layer     a pulsing live dot drawn by a Core Animation layer (--live-dot)
#   redraw    the same dot drawn by the main surface at 12.5 fps (--live-dot
#             --live-fallback --live-fallback-interval 0.08: Argus's redraw pulse)
#
#   ./build.sh sdl3-example
#   tests/probe/live_footprint_probe.sh [extra example args...]
#
# Each run is elimgui_sdl3 --idle-report 22 --release-idle 1000 <run args> <extra>;
# `footprint` is sampled at 8, 12, 16 and 20 s and the median of each row printed in
# MiB: IOSurface (Metal drawables), graphics-owned (unmapped, dirty and reclaimable),
# IOAccelerator (graphics), and the process footprint, plus the frames built over the
# idle report's 20 s window. The window shows ~22 s per run. Only the processes it
# starts are signalled.

set -uo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
bin="${ROOT_DIR}/examples/sdl3/build/elimgui_sdl3"
out="${ROOT_DIR}/examples/sdl3/build/live_probe"
[[ -x "${bin}" ]] || { echo "no ${bin}: run ./build.sh sdl3-example"; exit 2; }
command -v footprint >/dev/null || { echo "footprint not found (macOS only)"; exit 2; }
mkdir -p "${out}"

# row <file> <category> <column>: bytes in that column (1 dirty, 3 reclaimable) of the
# row whose category is exactly <category>
row() { awk -v cat="$2" -v col="$3" '
        { line = $0; sub(/^ +/, "", line); n = length(line) - length(cat) }
        n > 0 && substr(line, n + 1) == cat && substr(line, n, 1) == " " {
            split(line, f, / +B? */); v = f[col] + 0; found = 1 }
        END { print (found ? v : 0) }' "$1"; }
foot() { sed -n 's/.*Footprint: \([0-9]*\) B.*/\1/p' "$1"; }
median() { sort -n | awk 'NF { a[++n] = $1 }
                          END { m = (n % 2) ? a[(n + 1) / 2] : (a[n / 2] + a[n / 2 + 1]) / 2;
                                printf "%.1f", m / 1048576 }'; }
# rows <label> <category> <column>: that row in every sample, one per line
rows() { for at in 8 12 16 20; do row "${out}/$1_${at}.txt" "$2" "$3"; done; }

measure() {
    local label="$1"; shift
    "${bin}" --idle-report 22 --release-idle 1000 "$@" >"${out}/${label}.log" 2>&1 &
    local app=$!
    trap 'kill "${app}" 2>/dev/null' EXIT
    local t=0
    for at in 8 12 16 20; do
        sleep $((at - t)); t=${at}
        footprint -f bytes "${app}" >"${out}/${label}_${at}.txt" 2>&1
    done
    wait "${app}"
    trap - EXIT
    local ios gd gr acc fp
    ios=$(rows "${label}" "IOSurface" 1 | median)
    gd=$(rows "${label}" "Owned physical footprint (unmapped) (graphics)" 1 | median)
    gr=$(rows "${label}" "Owned physical footprint (unmapped) (graphics)" 3 | median)
    acc=$(rows "${label}" "IOAccelerator (graphics)" 1 | median)
    fp=$(for at in 8 12 16 20; do foot "${out}/${label}_${at}.txt"; done | median)
    local frames
    frames=$(sed -n 's/.*idle-report: [0-9.]* s window: \([0-9]*\) frames.*/\1/p' "${out}/${label}.log")
    printf '%-8s %8s %10s %10s %8s %9s %7s\n' "${label}" "${ios}" "${gd}" "${gr}" "${acc}" "${fp}" "${frames:-?}"
}

printf 'args: %s\n' "${*:-(none)}"
printf '%-8s %8s %10s %10s %8s %9s %7s\n' run iosurf gfx_dirty gfx_reclm ioaccel footprint frames
measure idle "$@"
measure layer --live-dot "$@"
measure redraw --live-dot --live-fallback --live-fallback-interval 0.08 "$@"
