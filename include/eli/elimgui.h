/**
 * @file elimgui.h
 * @brief Umbrella header for elimgui — a pure C11, header-only reimplementation
 *        of Dear ImGui targeting WebAssembly. Including this pulls in the whole
 *        library; each feature category is wired in below in dependency order.
 *
 * @status Phases 1-6 + 29 wired: core, draw, font, input, id, style, memory.
 * @issues None
 * @todo None
 */
#ifndef ELIMGUI_H
#define ELIMGUI_H

/* Library version (canonical). ELI_VERSION_NUM is MMmmpp (major/minor/patch). */
#define ELI_VERSION      "1.0.0"
#define ELI_VERSION_NUM  10000

/* Libc seam first: every header routes its runtime needs through here. */
#include "core/eli_platform.h"

/* ---------------------------------------------------------------------------
 * Forward declarations for types that are defined by later phases. Declaring
 * them here lets core headers (and user code) reference these opaque handles
 * before their owning category exists. C11 permits repeated identical typedefs,
 * so the owning phase re-declares/defines them without conflict.
 * ------------------------------------------------------------------------- */
typedef struct eli_draw_list eli_draw_list;
typedef struct eli_draw_data eli_draw_data;
typedef struct eli_font eli_font;
typedef struct eli_font_atlas eli_font_atlas;

/* ---------------------------------------------------------------------------
 * Core category — foundation types, enums, IO, style, context, and lifecycle.
 * ------------------------------------------------------------------------- */
#include "core/eli_core.h"

/* ---------------------------------------------------------------------------
 * Feature modules — wired in dependency order as each phase lands.
 * ------------------------------------------------------------------------- */
#include "draw/eli_draw.h"       /* Phase 2:  draw system      */
#include "font/eli_font.h"       /* Phase 3:  font system      */
#include "input/eli_input.h"     /* Phase 4:  input system     */
#include "id/eli_id.h"           /* Phase 5:  ID system/state  */
#include "style/eli_style.h"     /* Phase 6:  style system     */
#include "window/eli_window.h"   /* Phase 7:  windows          */
#include "layout/eli_layout.h"   /* Phase 8:  layout system    */
#include "widgets/eli_widgets.h" /* Phases 9-24: widgets + item status */
#include "interaction/eli_interaction.h" /* Phase 25: disabling/clipping/focus */
#include "util/eli_list_clipper.h"       /* Phase 26: list clipper     */
#include "util/eli_util.h"        /* Phase 27: misc utils + bg/fg lists */
#include "util/eli_settings.h"    /* Phase 28: INI settings     */
#include "util/eli_logging.h"     /* Phase 28: logging          */
#include "util/eli_mem.h"        /* Phase 29: memory allocator */
#include "docking/eli_dock.h"    /* Phase 34: docking system   */
#include "demo/eli_demo_all.h"   /* Phase 30: demo + debug windows */

/* Frame-lifecycle composition — MUST be last so every subsystem hook it calls
 * (input, window, popups, drag-drop, tables, tabs, docking) is already declared. */
#include "eli_frame.h"           /* eli_frame_begin / eli_frame_end            */

#endif /* ELIMGUI_H */
