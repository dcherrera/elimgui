/*
 * elimgui.h - Pure C Immediate Mode GUI Library
 *
 * A ground-up reimplementation of Dear ImGui in C99.
 * Master include file - includes all eli_* headers.
 */

#ifndef ELIMGUI_H
#define ELIMGUI_H

#include <jaclibc.h>
#include <stdint.h>
#include <stdbool.h>

/* Version */
#define ELI_VERSION "0.1.0"

/* ============================================================================
 * Core Types
 * ============================================================================ */

typedef struct { float x, y; } eli_vec2;
typedef struct { float x, y, z, w; } eli_vec4;
typedef struct { float x, y, w, h; } eli_rect;

typedef uint32_t eli_col32;
typedef uint32_t eli_id;

/* Color macros */
#define ELI_COL32(r, g, b, a) \
    (((uint32_t)(a)<<24) | ((uint32_t)(b)<<16) | ((uint32_t)(g)<<8) | (uint32_t)(r))

#define ELI_COL32_WHITE     0xFFFFFFFF
#define ELI_COL32_BLACK     0x000000FF
#define ELI_COL32_TRANSPARENT 0x00000000

/* ============================================================================
 * Forward Declarations
 * ============================================================================ */

typedef struct eli_context eli_context;
typedef struct eli_io eli_io;
typedef struct eli_style eli_style;
typedef struct eli_draw_list eli_draw_list;
typedef struct eli_draw_data eli_draw_data;
typedef struct eli_window eli_window;
typedef struct eli_font eli_font;

/* ============================================================================
 * Window Flags
 * ============================================================================ */

typedef int eli_window_flags;

#define ELI_WINDOW_NONE             0
#define ELI_WINDOW_NO_TITLEBAR      (1 << 0)
#define ELI_WINDOW_NO_RESIZE        (1 << 1)
#define ELI_WINDOW_NO_MOVE          (1 << 2)
#define ELI_WINDOW_NO_SCROLLBAR     (1 << 3)
#define ELI_WINDOW_NO_COLLAPSE      (1 << 4)
#define ELI_WINDOW_AUTO_RESIZE      (1 << 5)
#define ELI_WINDOW_NO_BACKGROUND    (1 << 6)
#define ELI_WINDOW_NO_SAVED_SETTINGS (1 << 7)

/* ============================================================================
 * Context Management
 * ============================================================================ */

/* Initialize elimgui - call once at startup */
void eli_init(void);

/* Shutdown elimgui - call once at cleanup */
void eli_shutdown(void);

/* Get the current context */
eli_context* eli_get_context(void);

/* Get IO structure for input/output configuration */
eli_io* eli_get_io(void);

/* Get style structure for customization */
eli_style* eli_get_style(void);

/* ============================================================================
 * Frame Lifecycle
 * ============================================================================ */

/* Start a new frame - call once per frame before any widgets */
void eli_new_frame(void);

/* Finalize rendering - call once per frame after all widgets */
void eli_render(void);

/* Get render data for drawing - call after eli_render() */
eli_draw_data* eli_get_draw_data(void);

/* ============================================================================
 * Windows
 * ============================================================================ */

/* Begin a new window. Returns false if collapsed/clipped. */
bool eli_begin(const char* name, bool* p_open, eli_window_flags flags);

/* End the current window */
void eli_end(void);

/* ============================================================================
 * Include other headers
 * ============================================================================ */

/* Uncomment as implemented:
#include "eli_draw.h"
#include "eli_widgets.h"
#include "eli_layout.h"
#include "eli_input.h"
#include "eli_font.h"
#include "eli_style.h"
#include "eli_tables.h"
*/

#endif /* ELIMGUI_H */
