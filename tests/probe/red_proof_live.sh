#!/bin/bash
# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 David (Corey) Herrera
#
# red_proof_live.sh — the live widget mutations, each through red_proof.sh (binaries
# deleted, rebuilt, run, file restored). Every line must go red.
#
# Usage: tests/probe/red_proof_live.sh
# Exit status: 0 when every mutation went red.

set -uo pipefail
P="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/red_proof.sh"
fails=0
run() {
    echo "== $1"
    "${P}" "$2" "$3" "$4" || fails=$((fails + 1))
}
W=include/eli/live/eli_live_widgets.h
L=include/eli/live/eli_live_pool.h
run "a layer widget still asks for the 12.5 fps redraw" "${W}" \
    's/(            eli_live__draw_main\(dl, w, false, 0\.0\);\n)/$1        eli_request_redraw_after(0.08f);\n/' \
    live_frames
run "a layer widget is also drawn on the main surface" "${W}" \
    's/        if \(w->main_static\)\n            eli_live__draw_main\(dl, w, false, 0\.0\);/        eli_live__draw_main(dl, w, true, 0.0);/' \
    live_frames
run "the fallback asks for no slow redraw" "${W}" \
    's/    if \(animate\)\n        eli_request_redraw_after\(interval\);\n//' live_frames
run "a submit allocates" "${L}" \
    's/(    w->seen_frame = ctx \? ctx->frame_count : 0;\n)/$1    eli_mem_free(eli_mem_alloc(16));\n/' \
    live_allocs
run "the pool lives on the heap" "${L}" \
    's/(        memset\(w, 0, sizeof\(\*w\)\);\n        w->used = true;\n)/$1        w->handle = w->handle ? w->handle : eli_mem_alloc(1024);\n/' \
    live_allocs
run "widgets not submitted are kept" "${L}" \
    's/if \(w->seen_frame != ctx->frame_count\) \{/if (0) {/' live_lifetime
run "every frame resends the widget" "${L}" \
    's/if \(w->live && w->pending && g_eli_live_backend\.update\)/if (w->live \&\& g_eli_live_backend.update)/' \
    live_lifetime
run "no occlusion test" "${L}" \
    's/for \(int li = ctx->render_draw_lists_count - 1; li > first; li--\)/for (int li = -1; li > first; li--)/' \
    live_occlusion
run "an occluded widget is not drawn static" "${L}" \
    's/bool want_static = w->live && occluded\[i\];/bool want_static = false;/' live_occlusion
echo "red_proof_live: ${fails} mutation(s) stayed green"
[[ ${fails} -eq 0 ]]
