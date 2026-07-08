/**
 * @file elimgui.js
 * @description WASM loader + Canvas2D renderer + DOM input wiring for the
 *   elimgui browser demo. Loads demo.wasm, uploads the font atlas, runs a
 *   requestAnimationFrame loop that calls the module's `frame(dt)` and then
 *   rasterizes the resulting draw data (solid triangles + font-atlas glyph
 *   blits) onto a 2D canvas, and forwards mouse/keyboard events into the module.
 *
 * @status Working Canvas2D backend for the Phase 31 demo.
 * @issues None
 * @todo None
 */

'use strict';

/* Vertex/command/index binary layouts exported by examples/demo/main.c. */
const ELI_VTX_STRIDE = 20; // float x,y,u,v; uint32 col
const ELI_CMD_STRIDE = 40; // float clip[4]; uint32 tex,vtxOff,idxOff,elem; 2 ptrs
const ELI_IDX_SIZE = 2; // uint16

/* eli_key codes (see include/eli/core/eli_enums.h enum order). Only the subset
 * the demo needs is mapped; letters/digits are computed from these bases. */
const ELI_KEY = {
  Tab: 1, ArrowLeft: 2, ArrowRight: 3, ArrowUp: 4, ArrowDown: 5,
  PageUp: 6, PageDown: 7, Home: 8, End: 9, Insert: 10, Delete: 11,
  Backspace: 12, Space: 13, Enter: 14, Escape: 15,
  ControlLeft: 49, ShiftLeft: 50, AltLeft: 51, MetaLeft: 52,
  ControlRight: 53, ShiftRight: 54, AltRight: 55, MetaRight: 56
};
const ELI_KEY_A = 68; // ELI_KEY_A
const ELI_KEY_0 = 58; // ELI_KEY_0

/** Translate a KeyboardEvent.code to an eli_key code, or 0 if unmapped. */
function eliKeyFromCode(code) {
  if (ELI_KEY[code] !== undefined) return ELI_KEY[code];
  if (code.startsWith('Key') && code.length === 4) {
    const c = code.charCodeAt(3); // 'A'..'Z'
    if (c >= 65 && c <= 90) return ELI_KEY_A + (c - 65);
  }
  if (code.startsWith('Digit') && code.length === 6) {
    const d = code.charCodeAt(5) - 48; // '0'..'9'
    if (d >= 0 && d <= 9) return ELI_KEY_0 + d;
  }
  return 0;
}

/**
 * Build a font-atlas source canvas from the module's RGBA32 texture, plus a
 * small cache of color-tinted copies so glyphs can be drawn in any vertex color.
 */
class FontAtlas {
  constructor(exports) {
    const w = exports.eli_atlas_width();
    const h = exports.eli_atlas_height();
    const ptr = exports.eli_atlas_pixels();
    this.width = w;
    this.height = h;

    const base = document.createElement('canvas');
    base.width = w;
    base.height = h;
    const bctx = base.getContext('2d');
    const img = bctx.createImageData(w, h);
    // Atlas pixels are RGBA byte order — copy straight into the ImageData.
    const src = new Uint8Array(exports.memory.buffer, ptr, w * h * 4);
    img.data.set(src);
    bctx.putImageData(img, 0, 0);

    this.base = base;
    this.tints = new Map(); // "r,g,b" -> tinted canvas
  }

  /** Return an atlas canvas tinted to the given RGB (cached). */
  tinted(r, g, b) {
    if (r === 255 && g === 255 && b === 255) return this.base;
    const key = r + ',' + g + ',' + b;
    let c = this.tints.get(key);
    if (c) return c;
    c = document.createElement('canvas');
    c.width = this.width;
    c.height = this.height;
    const cx = c.getContext('2d');
    cx.drawImage(this.base, 0, 0);
    cx.globalCompositeOperation = 'source-in';
    cx.fillStyle = 'rgb(' + r + ',' + g + ',' + b + ')';
    cx.fillRect(0, 0, this.width, this.height);
    this.tints.set(key, c);
    return c;
  }
}

/** Canvas2D renderer that walks elimgui draw data and paints it. */
class Renderer {
  constructor(exports, ctx, atlas) {
    this.exports = exports;
    this.ctx = ctx;
    this.atlas = atlas;
  }

  /** Unpack a packed RGBA color (R in the low byte). */
  static unpack(col) {
    return {
      r: col & 0xff,
      g: (col >>> 8) & 0xff,
      b: (col >>> 16) & 0xff,
      a: (col >>> 24) & 0xff
    };
  }

  /** Read one vertex into {x,y,u,v,col} from a fresh DataView. */
  static vertex(dv, base, index) {
    const o = base + index * ELI_VTX_STRIDE;
    return {
      x: dv.getFloat32(o, true),
      y: dv.getFloat32(o + 4, true),
      u: dv.getFloat32(o + 8, true),
      v: dv.getFloat32(o + 12, true),
      col: dv.getUint32(o + 16, true)
    };
  }

  /** Paint the whole current frame's draw data. */
  render() {
    const ex = this.exports;
    const listCount = ex.eli_dd_list_count();
    // Re-acquire the view every frame: linear memory may have grown.
    const dv = new DataView(ex.memory.buffer);

    for (let li = 0; li < listCount; li++) {
      const vtxBase = ex.eli_dd_vtx_ptr(li);
      const idxBase = ex.eli_dd_idx_ptr(li);
      const cmdBase = ex.eli_dd_cmd_ptr(li);
      const cmdCount = ex.eli_dd_cmd_count(li);
      if (!vtxBase || !idxBase || !cmdBase) continue;

      for (let ci = 0; ci < cmdCount; ci++) {
        this.renderCommand(dv, vtxBase, idxBase, cmdBase + ci * ELI_CMD_STRIDE);
      }
    }
  }

  /** Paint one draw command (clip + triangle/glyph run). */
  renderCommand(dv, vtxBase, idxBase, cmdOff) {
    const clipX = dv.getFloat32(cmdOff, true);
    const clipY = dv.getFloat32(cmdOff + 4, true);
    const clipW = dv.getFloat32(cmdOff + 8, true);
    const clipH = dv.getFloat32(cmdOff + 12, true);
    const vtxOffset = dv.getUint32(cmdOff + 20, true);
    const idxOffset = dv.getUint32(cmdOff + 24, true);
    const elemCount = dv.getUint32(cmdOff + 28, true);
    if (elemCount === 0 || clipW <= 0 || clipH <= 0) return;

    const ctx = this.ctx;
    ctx.save();
    ctx.beginPath();
    ctx.rect(clipX, clipY, clipW, clipH);
    ctx.clip();

    const idxAt = (k) => dv.getUint16(idxBase + (idxOffset + k) * ELI_IDX_SIZE, true);
    const vtx = (k) => Renderer.vertex(dv, vtxBase, vtxOffset + idxAt(k));

    let e = 0;
    while (e + 3 <= elemCount) {
      const a = vtx(e), b = vtx(e + 1), c = vtx(e + 2);
      const solid = a.u === b.u && b.u === c.u && a.v === b.v && b.v === c.v;

      if (solid) {
        this.fillTriangle(a, b, c);
        e += 3;
        continue;
      }

      // Textured: two consecutive triangles form one axis-aligned glyph quad.
      if (e + 6 <= elemCount) {
        const d = vtx(e + 3), f = vtx(e + 4), g = vtx(e + 5);
        this.blitGlyph([a, b, c, d, f, g]);
        e += 6;
        continue;
      }
      // Lone textured triangle (shouldn't happen for text) — skip gracefully.
      e += 3;
    }

    ctx.restore();
  }

  /** Fill a solid triangle with its first vertex color. */
  fillTriangle(a, b, c) {
    const { r, g, b: bl, a: al } = Renderer.unpack(a.col);
    const ctx = this.ctx;
    ctx.beginPath();
    ctx.moveTo(a.x, a.y);
    ctx.lineTo(b.x, b.y);
    ctx.lineTo(c.x, c.y);
    ctx.closePath();
    ctx.fillStyle = 'rgba(' + r + ',' + g + ',' + bl + ',' + (al / 255) + ')';
    ctx.fill();
  }

  /** Blit a glyph quad: map its atlas sub-rect onto its screen rect. */
  blitGlyph(verts) {
    let xmin = Infinity, ymin = Infinity, xmax = -Infinity, ymax = -Infinity;
    let umin = Infinity, vmin = Infinity, umax = -Infinity, vmax = -Infinity;
    for (const p of verts) {
      if (p.x < xmin) xmin = p.x;
      if (p.x > xmax) xmax = p.x;
      if (p.y < ymin) ymin = p.y;
      if (p.y > ymax) ymax = p.y;
      if (p.u < umin) umin = p.u;
      if (p.u > umax) umax = p.u;
      if (p.v < vmin) vmin = p.v;
      if (p.v > vmax) vmax = p.v;
    }
    const dw = xmax - xmin, dh = ymax - ymin;
    if (dw <= 0 || dh <= 0) return;

    const { r, g, b, a } = Renderer.unpack(verts[0].col);
    const img = this.atlas.tinted(r, g, b);
    const sx = umin * this.atlas.width;
    const sy = vmin * this.atlas.height;
    const sw = (umax - umin) * this.atlas.width;
    const sh = (vmax - vmin) * this.atlas.height;

    const ctx = this.ctx;
    const prevAlpha = ctx.globalAlpha;
    ctx.globalAlpha = a / 255;
    ctx.drawImage(img, sx, sy, sw, sh, xmin, ymin, dw, dh);
    ctx.globalAlpha = prevAlpha;
  }
}

/** Install DOM event listeners that forward input into the WASM module. */
function installInput(exports, canvas) {
  const pos = (ev) => {
    const rect = canvas.getBoundingClientRect();
    exports.eli_on_mouse_pos(ev.clientX - rect.left, ev.clientY - rect.top);
  };

  canvas.addEventListener('mousemove', pos);
  canvas.addEventListener('mousedown', (ev) => {
    pos(ev);
    exports.eli_on_mouse_button(ev.button === 1 ? 2 : ev.button === 2 ? 1 : 0, 1);
  });
  window.addEventListener('mouseup', (ev) => {
    exports.eli_on_mouse_button(ev.button === 1 ? 2 : ev.button === 2 ? 1 : 0, 0);
  });
  canvas.addEventListener('contextmenu', (ev) => ev.preventDefault());
  canvas.addEventListener('wheel', (ev) => {
    ev.preventDefault();
    const scale = ev.deltaMode === 1 ? 1 : 1 / 40; // lines vs pixels
    exports.eli_on_mouse_wheel(-ev.deltaX * scale, -ev.deltaY * scale);
  }, { passive: false });

  window.addEventListener('keydown', (ev) => {
    const key = eliKeyFromCode(ev.code);
    if (key) exports.eli_on_key(key, 1);
    // Keep browser shortcuts (F5, devtools) but stop the page from scrolling.
    if (['Tab', 'Space', 'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight'].includes(ev.code)) {
      ev.preventDefault();
    }
  });
  window.addEventListener('keyup', (ev) => {
    const key = eliKeyFromCode(ev.code);
    if (key) exports.eli_on_key(key, 0);
  });
  window.addEventListener('keypress', (ev) => {
    if (ev.charCode) exports.eli_on_char(ev.charCode);
  });
}

/** Size the canvas to the viewport (DPR-aware) and report display size to WASM. */
function resize(canvas, ctx, exports) {
  const dpr = window.devicePixelRatio || 1;
  const cssW = window.innerWidth;
  const cssH = window.innerHeight;
  canvas.width = Math.floor(cssW * dpr);
  canvas.height = Math.floor(cssH * dpr);
  canvas.style.width = cssW + 'px';
  canvas.style.height = cssH + 'px';
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  exports.eli_set_display_size(cssW, cssH);
  return { cssW, cssH, dpr };
}

/**
 * Load demo.wasm, wire up rendering + input, and start the frame loop.
 *
 * @param {string} wasmPath URL of the wasm module.
 * @returns {Promise<WebAssembly.Instance>}
 */
async function loadElimgui(wasmPath) {
  const canvas = document.getElementById('canvas');
  const ctx = canvas.getContext('2d');

  let instance = null;
  const decodeCStr = (ptr) => {
    const mem = new Uint8Array(instance.exports.memory.buffer);
    let end = ptr;
    while (mem[end]) end++;
    return new TextDecoder().decode(mem.subarray(ptr, end));
  };

  const imports = {
    env: {
      // Library clipboard bridge (JS_IMPORT eli_host_set_clipboard).
      eli_host_set_clipboard: (ptr) => {
        try {
          const text = decodeCStr(ptr);
          if (navigator.clipboard) navigator.clipboard.writeText(text);
        } catch (e) { /* clipboard is best-effort */ }
      }
    }
  };

  const response = await fetch(wasmPath);
  const bytes = await response.arrayBuffer();
  const module = await WebAssembly.instantiate(bytes, imports);
  instance = module.instance;
  const exports = instance.exports;

  exports.js_start();

  const atlas = new FontAtlas(exports);
  const renderer = new Renderer(exports, ctx, atlas);

  let dims = resize(canvas, ctx, exports);
  window.addEventListener('resize', () => { dims = resize(canvas, ctx, exports); });
  installInput(exports, canvas);

  let last = performance.now();
  function loop(now) {
    const dt = (now - last) / 1000;
    last = now;

    exports.frame(dt);

    ctx.clearRect(0, 0, dims.cssW, dims.cssH);
    renderer.render();

    requestAnimationFrame(loop);
  }
  requestAnimationFrame(loop);

  return instance;
}

if (typeof module !== 'undefined' && module.exports) {
  module.exports = { loadElimgui, eliKeyFromCode };
}
