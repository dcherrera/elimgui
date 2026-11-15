/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file bench_hotpath.mjs
 * @brief WASM driver for tests/bench/bench_hotpath.c.
 *
 * Builds bench_hotpath.c with clang --target=wasm32 (-nostdlib, JAClibc, -Os like
 * build.sh) into tests/build/bench_hotpath.wasm, instantiates it in Node with stub
 * imports, and times each exported bench_* workload with performance.now() (best of 5).
 * This is the path where trig is JAClibc's software cosf/sinf.
 *
 * Usage: node tests/bench/bench_hotpath.mjs [include-dir]   (from the repo root)
 *   include-dir  elimgui headers to build against (default: include); point it at
 *                an extracted older tree for before/after comparisons.
 *
 * @status In use (numbers in docs/optimization.md).
 * @issues None
 * @todo None
 */
import { execFileSync } from 'node:child_process';
import { mkdirSync, readFileSync } from 'node:fs';
import { performance } from 'node:perf_hooks';

const includeDir = process.argv[2] || 'include';
const out = 'tests/build/bench_hotpath.wasm';
mkdirSync('tests/build', { recursive: true });
execFileSync('clang', [
  '--target=wasm32', '-nostdlib', '-Ivendor/jaclibc/include', `-I${includeDir}`, '-Ivendor',
  '-Os', '-w',
  '-Wl,--no-entry', '-Wl,--export-dynamic', '-o', out, 'tests/bench/bench_hotpath.c',
], { stdio: 'inherit' });

const module = new WebAssembly.Module(readFileSync(out));
const env = {};
for (const imp of WebAssembly.Module.imports(module))
  if (imp.kind === 'function') env[imp.name] = () => 0;
const ex = new WebAssembly.Instance(module, { env }).exports;
ex.bench_setup();

const workloads = [
  ['wrap-frame', ex.bench_wrap_frame, 500],
  ['wrap-long', ex.bench_wrap_long, 500],
  ['wrap-drawlist', ex.bench_wrap_drawlist, 500],
  ['round-rect', ex.bench_round_rect, 500],
  ['circles', ex.bench_circles, 500],
];
for (const [name, fn, iterations] of workloads) {
  fn(Math.floor(iterations / 10) + 1);
  let best = Infinity, checksum = 0;
  for (let rep = 0; rep < 5; rep++) {
    const t0 = performance.now();
    checksum = fn(iterations);
    best = Math.min(best, performance.now() - t0);
  }
  const us = (best * 1000) / iterations;
  const line = `${name.padEnd(14)} ${us.toFixed(2).padStart(8)} us/iter`;
  console.log(`${line}  (${iterations} iters, checksum ${checksum})`);
}
