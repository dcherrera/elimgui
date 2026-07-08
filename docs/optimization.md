# Optimization (Phase 33)

A measured performance pass over draw-list generation. Correctness was held
constant: the full unit suite stays green and `./build.sh demo` still builds
`web/demo.wasm`. Every change below is backed by the benchmark in
`tests/bench/bench_draw.c`.

## Benchmark method

`tests/bench/bench_draw.c` is a standalone native program (it lives outside
`tests/unit/`, so `./build.sh test` skips it). It exercises three representative
workloads and reports wall-clock time plus vertex/index totals:

1. **prim-fill** — 2000 filled rectangles into a bare draw list, repeated.
2. **text** — a 40-line text run rasterized to glyph quads, repeated.
3. **frame** — a full context frame: one window with 20 button/slider/checkbox
   rows and a 4×40 table, assembled through `eli_frame_begin`/`eli_frame_end`.

Each workload also proves the allocation-reuse invariant: after a short warm-up
it counts how many iterations still grow a buffer. At steady state this is `0`,
i.e. frames reuse their geometry buffers with zero reallocations.

Build and run:

```bash
cc -std=c11 -O2 -DELI_TEST_HOSTED -Iinclude -Ivendor \
   tests/bench/bench_draw.c -o /tmp/bench_draw -lm && /tmp/bench_draw
```

Timing uses `clock()`; treat absolute values as machine-relative and compare
before/after on the same host. Numbers below are from an Apple-silicon dev
machine, `-O2`, best-of-several.

## Measured results

### Draw-generation time and buffer reuse

| workload  | time / iter | geometry              | grow-events at steady state |
|-----------|-------------|-----------------------|-----------------------------|
| prim-fill | ~0.025 ms   | 8000 vtx / 12000 idx  | 0                           |
| text      | ~0.007 ms   | 7200 vtx / 10800 idx  | 0                           |
| frame     | ~0.020 ms   | 1688 vtx / 2568 idx   | 0 (stable from frame 0)     |

The frame workload assembles into a **single** draw command list with all
same-clip/same-texture geometry batched — command batching is working.

### Text path — before/after (this phase's change)

The text rasterizer previously called `prim_reserve(6, 4)` once per glyph. It now
makes a **single** capacity reservation for the whole run and charges the exact
emitted index count once at the end. Geometry is byte-for-byte identical.

| text rasterizer      | time / iter (40-line run) |
|----------------------|---------------------------|
| per-glyph reserve    | ~0.010 ms                 |
| batched reserve (new)| ~0.007 ms                 |

≈ 25–30% faster on the text path, which dominates real UI frames. Output vertex
and index counts are unchanged (7200 / 10800).

### Allocations per frame

Draw lists are **reset, not freed**, between frames (`eli_draw_list_reset` clears
counts but keeps buffers and capacity). Windows, the background/foreground util
lists, and the per-frame render-list array all follow this pattern. The benchmark
confirms **0 buffer reallocations per frame** once capacities stabilize (frame 0
for the demo-sized workload). There is no per-frame heap churn in the draw path.

### WASM binary size

`web/demo.wasm`, built with the Homebrew LLVM `clang --target=wasm32`, demo
`main.c`:

| opt level | size (bytes) | vs `-O2` |
|-----------|--------------|----------|
| `-O2`     | 196,536      | —        |
| `-O3`     | (larger)     | worse    |
| `-Os` (build.sh default) | 160,502 | −18%  |
| `-Oz`     | 113,400      | −42%     |

`build.sh` now defaults to **`-Os`** — a size-conscious level (~18% smaller than
`-O2`) with balanced runtime performance, a good fit for a browser-delivered,
header-only library. **Opt-in:** `-Oz` roughly halves the binary (−42%) at some
runtime cost — switch `build.sh`'s `CFLAGS` to `-Oz` when minimum download size
matters most. `-O3` is strictly worse here (bigger, no measured speedup for this
integer/geometry-bound workload). This phase records the finding
without altering `build.sh`.

## What changed and why

All changes are additive and preserve observable behavior (verified by the suite
and by byte-identical geometry assertions).

### 1. Split capacity-reservation from element-charging

`include/eli/draw/eli_draw_list.h` gained two helpers and `prim_reserve` was
refactored to compose them (its behavior is unchanged):

- `eli_draw_list_prim_reserve_capacity(list, idx, vtx)` — grows the vertex/index
  buffers geometrically (amortized O(1)) **without** charging any draw command.
- `eli_draw_list_prim_add_idx_to_cmd(list, idx)` — charges an index total to the
  current command without touching buffers.
- `eli_draw_list_prim_reserve` now = `reserve_capacity` + `add_idx_to_cmd`.

This enables a single up-front reservation for a run of primitives whose exact
index total is only known after emission (text), instead of one reserve call per
primitive.

### 2. Batched text reservation

`include/eli/font/eli_font_text.h` `eli_draw_list_add_text_ex` now reserves
capacity for the whole run once (upper bound: one quad per input byte, since a
UTF-8 sequence spans ≥ 1 byte and newlines/`\r`/invisible glyphs emit nothing),
emits glyph quads, then charges the exact emitted index count once. This removes
the per-glyph reserve overhead while leaving `vtx_count`, `idx_count`, and the
command's `elem_count` exactly where the per-glyph path left them.

## Already-optimal properties (verified, not changed)

- **Geometric buffer growth.** `eli_draw_buf_grow` grows ~1.5× (`cap += cap/2 + 1`,
  seeded at 8). Filling 16,000 vertices takes < 30 reallocations (a linear policy
  would need hundreds) and capacity overhead stays under 2× the live count.
- **Draw-call batching.** `eli_draw_list_on_changed_clip_rect` /
  `..._on_changed_texture` merge adjacent commands that share clip + texture and
  are index-contiguous, and drop empty commands. Same-state geometry collapses to
  one command; a distinct clip splits, and re-entering the prior clip while the
  interposed command is empty merges back.
- **Cross-frame buffer reuse.** `eli_draw_list_reset` retains all heap buffers, so
  steady-state frames allocate nothing in the draw path.

## Remaining opportunities

- **`-Oz` build** halves `web/demo.wasm` (189 KB → 109 KB) with no source change;
  adopt if binary size matters (kept out of `build.sh` this phase by scope).
- **Vertex offset for > 64K-vertex lists.** Indices are 16-bit and
  `ELI_DRAW_LIST_ALLOW_VTX_OFFSET` is reserved but unused. Large single lists
  currently cap at 65,536 vertices; a 32-bit-offset path would lift that.
- **Anti-aliased geometry** is deliberately not generated (the backend smooths),
  keeping vertex counts deterministic and low — a size/quality trade-off to
  revisit only if a backend needs CPU-side AA.
- **Text glyph lookup** (`eli_font_find_glyph`) is per-codepoint; a hot-ASCII
  fast path could shave the text loop further if profiling on WASM shows it hot.
```
