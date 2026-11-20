#!/bin/bash
# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 David (Corey) Herrera
#
# red_proof_memory.sh — the memory-pass mutations, each through red_proof.sh (binaries
# deleted, rebuilt, run, file restored). Every line must go red.
#
# Usage: tests/probe/red_proof_memory.sh
# Exit status: 0 when every mutation went red.

set -uo pipefail
P="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/red_proof.sh"
fails=0
run() {
    echo "== $1"
    "${P}" "$2" "$3" "$4" || fails=$((fails + 1))
}
run "text reserves a quad per byte up front" include/eli/font/eli_font_text.h \
    's/(    eli_font__text_run run = \{ list, 0, 0, false \};\n)/$1    if (!eli_draw_list_prim_reserve_capacity(list, (int)(text_end - text_begin) * 6, (int)(text_end - text_begin) * 4)) return;\n/' \
    draw_memory
run "draw lists never shrink" include/eli/draw/eli_draw_list.h \
    's/if \(ELI_DRAW_LIST_SHRINK_FRAMES > 0 &&/if (0 \&\&/' draw_memory
run "shrink at 1x the high water (a steady list reallocates)" include/eli/draw/eli_draw_list.h \
    's/\(uint64_t\)hw \* 4u/(uint64_t)hw * 1u/' draw_memory
run "unused windows keep their draw lists" include/eli/window/eli_window.h \
    's/    eli_window_compact_unused\(ctx\);\n//' draw_memory
run "polyline joins not mitered" include/eli/draw/eli_draw_prim.h \
    's/float inv = 1.0f \/ d2;/float inv = 1.0f;/' draw_memory
echo "red_proof_memory: ${fails} mutation(s) stayed green"
[[ ${fails} -eq 0 ]]
