#!/bin/bash
# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 David (Corey) Herrera
#
# sdl3_swapchain_probe.sh — swapchain memory of the SDL3 example (macOS): the
# IOSurface bytes (Metal drawables) while visible, then after the window hides.
#
#   ./build.sh sdl3-example
#   tests/probe/sdl3_swapchain_probe.sh [example args...]
#   e.g. tests/probe/sdl3_swapchain_probe.sh --drawables 2 --release-hidden
#
# Runs elimgui_sdl3 --idle-report 16 --warm-frames 120 --hide-after 8 <args>,
# samples `footprint` at 6 s (visible, the drawable pool filled by 120 back-to-back
# frames) and 14 s (hidden), and prints IOSurface and total footprint in MB (MiB).
# The window shows for ~8 s. Only the process it starts is signalled.

set -uo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
bin="${ROOT_DIR}/examples/sdl3/build/elimgui_sdl3"
out="${ROOT_DIR}/examples/sdl3/build/swapchain_probe"
[[ -x "${bin}" ]] || { echo "no ${bin}: run ./build.sh sdl3-example"; exit 2; }
command -v footprint >/dev/null || { echo "footprint not found (macOS only)"; exit 2; }
mkdir -p "${out}"

"${bin}" --idle-report 16 --warm-frames 120 --hide-after 8 "$@" >"${out}/example.log" 2>&1 &
app=$!
trap 'kill "${app}" 2>/dev/null' EXIT

# sample <label>: footprint snapshot to ${out}/<label>.txt
sample() { footprint -f bytes "${app}" >"${out}/$1.txt" 2>&1; }
# iosurf_mb <label> / total_mb <label>
iosurf_mb() { awk '$NF == "IOSurface" && $2 == "B" { printf "%.1f", $1 / 1048576; f = 1 }
                   END { if (!f) printf "0.0" }' "${out}/$1.txt"; }
total_mb() { sed -n 's/.*Footprint: \([0-9]*\) B.*/\1/p' "${out}/$1.txt" | awk '{ printf "%.1f", $1 / 1048576 }'; }

sleep 6 && sample visible
sleep 8 && sample hidden
wait "${app}"
trap - EXIT
printf 'args: %s\n' "${*:-(none)}"
printf '%-8s %10s %10s\n' state iosurf_MB foot_MB
printf '%-8s %10s %10s\n' visible "$(iosurf_mb visible)" "$(total_mb visible)"
printf '%-8s %10s %10s\n' hidden "$(iosurf_mb hidden)" "$(total_mb hidden)"
grep -E 'example:' "${out}/example.log" | sed 's/.*example: /example: /'
