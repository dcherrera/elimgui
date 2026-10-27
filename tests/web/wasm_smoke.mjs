/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file wasm_smoke.mjs
 * @brief Headless smoke test for the WASM builds (web/demo.wasm, web/cheatsheet.wasm).
 *
 * Instantiates each module in Node with stub host imports, runs js_start and a
 * few frames, then walks the draw data exactly as web/elimgui.js does (vertex,
 * command and index layouts, eli_dd_idx_size) and checks every index lands inside
 * its list's vertex buffer and every command range inside its index buffer.
 * Catches layout drift between the C structs and the JS renderer without a browser.
 *
 * Usage: node tests/web/wasm_smoke.mjs [web/demo.wasm ...]
 *
 * @status In use (./build.sh wasm-smoke).
 * @issues Does not paint; Canvas2D output is still checked by eye in a browser.
 * @todo None
 */
import { readFileSync } from 'node:fs';

const VTX_STRIDE = 20;
const CMD_STRIDE = 40;

function stubImports(module) {
  const env = {};
  for (const imp of WebAssembly.Module.imports(module)) {
    if (imp.kind === 'function') env[imp.name] = () => 0;
  }
  return { env };
}

function checkFrame(ex) {
  const dv = new DataView(ex.memory.buffer);
  const idxSize = ex.eli_dd_idx_size ? ex.eli_dd_idx_size() : 2;
  const lists = ex.eli_dd_list_count();
  let vertices = 0, commands = 0;
  if (lists <= 0) throw new Error('no draw lists');
  for (let li = 0; li < lists; li++) {
    const vtxCount = ex.eli_dd_vtx_count(li);
    const idxCount = ex.eli_dd_idx_count(li);
    const idxBase = ex.eli_dd_idx_ptr(li);
    const cmdBase = ex.eli_dd_cmd_ptr(li);
    const cmdCount = ex.eli_dd_cmd_count(li);
    vertices += vtxCount;
    for (let ci = 0; ci < cmdCount; ci++) {
      const off = cmdBase + ci * CMD_STRIDE;
      const vtxOffset = dv.getUint32(off + 20, true);
      const idxOffset = dv.getUint32(off + 24, true);
      const elem = dv.getUint32(off + 28, true);
      if (idxOffset + elem > idxCount) throw new Error(`list ${li} cmd ${ci}: index range past end`);
      for (let k = 0; k < elem; k++) {
        const at = idxBase + (idxOffset + k) * idxSize;
        const idx = idxSize === 4 ? dv.getUint32(at, true) : dv.getUint16(at, true);
        if (vtxOffset + idx >= vtxCount) throw new Error(`list ${li} cmd ${ci}: index ${idx} >= ${vtxCount}`);
      }
      commands++;
    }
    const vtx0 = ex.eli_dd_vtx_ptr(li);
    if (vtxCount > 0 && !Number.isFinite(dv.getFloat32(vtx0 + (vtxCount - 1) * VTX_STRIDE, true)))
      throw new Error(`list ${li}: last vertex x is not finite`);
  }
  return { lists, vertices, commands, idxSize };
}

let failed = 0;
const files = process.argv.slice(2).length ? process.argv.slice(2) : ['web/demo.wasm', 'web/cheatsheet.wasm'];
for (const file of files) {
  try {
    const module = new WebAssembly.Module(readFileSync(file));
    const ex = new WebAssembly.Instance(module, stubImports(module)).exports;
    ex.js_start();
    ex.eli_set_display_size(1280, 800);
    let stats;
    for (let f = 0; f < 5; f++) {
      ex.frame(1 / 60);
      stats = checkFrame(ex);
    }
    console.log(`ok   ${file}: ${stats.lists} lists, ${stats.vertices} vtx, ${stats.commands} cmds, idx ${stats.idxSize} B`);
  } catch (err) {
    failed++;
    console.log(`FAIL ${file}: ${err.message}`);
  }
}
process.exit(failed ? 1 : 0);
