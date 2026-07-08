/**
 * @file eli_draw_types.h
 * @brief Draw-system data types: draw command, vertex, index, the draw list
 *        (with path/clip/texture/channel stacks), draw data, and the draw flags.
 *
 * These are the renderer-agnostic output structures. A draw list accumulates
 * vertices, indices, and clip/texture-scoped draw commands; draw data gathers
 * one or more finished lists for a backend to translate into pixels.
 *
 * @status Phase 2 draw types in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_DRAW_ELI_DRAW_TYPES_H
#define ELI_DRAW_ELI_DRAW_TYPES_H

#include "../core/eli_platform.h"
#include "../core/eli_types.h"

/* ---------------------------------------------------------------------------
 * Flags
 * ------------------------------------------------------------------------- */

/**
 * Per-draw flags shared by AddRect/AddRectFilled/PathRect/AddPolyline. Bit 0 is
 * ELI_DRAW_CLOSED for backward-compatible boolean use; the rounding-corner bits
 * mirror Dear ImGui's ImDrawFlags so rounded shapes can select corners.
 */
typedef int eli_draw_flags;
enum eli_draw_flags_ {
    ELI_DRAW_NONE                     = 0,
    ELI_DRAW_CLOSED                   = 1 << 0,
    ELI_DRAW_ROUND_CORNERS_TOP_LEFT   = 1 << 4,
    ELI_DRAW_ROUND_CORNERS_TOP_RIGHT  = 1 << 5,
    ELI_DRAW_ROUND_CORNERS_BOT_LEFT   = 1 << 6,
    ELI_DRAW_ROUND_CORNERS_BOT_RIGHT  = 1 << 7,
    ELI_DRAW_ROUND_CORNERS_NONE       = 1 << 8,
    ELI_DRAW_ROUND_CORNERS_TOP =
        ELI_DRAW_ROUND_CORNERS_TOP_LEFT | ELI_DRAW_ROUND_CORNERS_TOP_RIGHT,
    ELI_DRAW_ROUND_CORNERS_BOTTOM =
        ELI_DRAW_ROUND_CORNERS_BOT_LEFT | ELI_DRAW_ROUND_CORNERS_BOT_RIGHT,
    ELI_DRAW_ROUND_CORNERS_LEFT =
        ELI_DRAW_ROUND_CORNERS_TOP_LEFT | ELI_DRAW_ROUND_CORNERS_BOT_LEFT,
    ELI_DRAW_ROUND_CORNERS_RIGHT =
        ELI_DRAW_ROUND_CORNERS_TOP_RIGHT | ELI_DRAW_ROUND_CORNERS_BOT_RIGHT,
    ELI_DRAW_ROUND_CORNERS_ALL =
        ELI_DRAW_ROUND_CORNERS_TOP_LEFT | ELI_DRAW_ROUND_CORNERS_TOP_RIGHT |
        ELI_DRAW_ROUND_CORNERS_BOT_LEFT | ELI_DRAW_ROUND_CORNERS_BOT_RIGHT,
    ELI_DRAW_ROUND_CORNERS_MASK =
        ELI_DRAW_ROUND_CORNERS_ALL | ELI_DRAW_ROUND_CORNERS_NONE
};

/**
 * Draw-list construction flags. Anti-aliasing bits are recognized but the
 * current renderer path is non-anti-aliased (deterministic geometry, smoothing
 * left to the backend); ELI_DRAW_LIST_ALLOW_VTX_OFFSET is reserved for large
 * meshes and unused while indices are 16-bit.
 */
typedef int eli_draw_list_flags;
enum eli_draw_list_flags_ {
    ELI_DRAW_LIST_NONE                      = 0,
    ELI_DRAW_LIST_ANTI_ALIASED_LINES        = 1 << 0,
    ELI_DRAW_LIST_ANTI_ALIASED_LINES_USE_TEX = 1 << 1,
    ELI_DRAW_LIST_ANTI_ALIASED_FILL         = 1 << 2,
    ELI_DRAW_LIST_ALLOW_VTX_OFFSET          = 1 << 3
};

/* ---------------------------------------------------------------------------
 * Vertex and index
 * ------------------------------------------------------------------------- */

/** One index into a draw list's vertex buffer (16-bit, 64K vertices per list). */
typedef uint16_t eli_draw_idx;

/** A single renderable vertex: screen position, texture UV, and packed color. */
typedef struct eli_draw_vert {
    float x, y;
    float u, v;
    eli_col32 col;
} eli_draw_vert;

/* Forward declarations so the callback typedef can reference these types. */
typedef struct eli_draw_list eli_draw_list;
struct eli_draw_cmd;

/* ---------------------------------------------------------------------------
 * Draw command
 * ------------------------------------------------------------------------- */

/**
 * A user callback invoked by the backend in place of drawing a command's
 * geometry. Receives the parent list and the command that carried it.
 */
typedef void (*eli_draw_callback)(const eli_draw_list *parent_list,
                                  const struct eli_draw_cmd *cmd);

/**
 * One draw call: a run of `elem_count` indices starting at `idx_offset`, all
 * sharing the same clip rectangle and texture. `clip_rect` is a scissor rect in
 * origin+size form (x, y = top-left; w, h = size). When `user_callback` is set
 * the backend calls it instead of drawing geometry.
 */
typedef struct eli_draw_cmd {
    eli_rect clip_rect;
    uint32_t texture_id;
    uint32_t vtx_offset;
    uint32_t idx_offset;
    uint32_t elem_count;
    eli_draw_callback user_callback;
    void *user_callback_data;
} eli_draw_cmd;

/* ---------------------------------------------------------------------------
 * Channel (draw-list splitting)
 * ------------------------------------------------------------------------- */

/**
 * A saved command+index buffer used while a draw list is split into channels.
 * Vertices remain shared in the parent list; only commands and indices split.
 */
typedef struct eli_draw_channel {
    eli_draw_cmd *cmds;
    int cmd_count;
    int cmd_capacity;
    eli_draw_idx *idx;
    int idx_count;
    int idx_capacity;
} eli_draw_channel;

/* ---------------------------------------------------------------------------
 * Draw list
 * ------------------------------------------------------------------------- */

/**
 * Accumulates geometry and draw commands for one logical surface (e.g. a
 * window). Public buffers (cmds/vtx/idx) are consumed by the backend; the
 * remaining fields are working state for the path, clip, texture, and channel
 * APIs. Clip rectangles are held internally as vec4 (min.x, min.y, max.x,
 * max.y) for easy intersection; each emitted command stores its clip as an
 * origin+size eli_rect.
 */
struct eli_draw_list {
    eli_draw_cmd *cmds;
    uint32_t cmd_count;
    uint32_t cmd_capacity;

    eli_draw_vert *vtx;
    uint32_t vtx_count;
    uint32_t vtx_capacity;

    eli_draw_idx *idx;
    uint32_t idx_count;
    uint32_t idx_capacity;

    eli_draw_list_flags flags;

    /* Path building. */
    eli_vec2 *path;
    int path_count;
    int path_capacity;

    /* Clip rect stack (each entry: min.x, min.y, max.x, max.y). */
    eli_vec4 *clip_rect_stack;
    int clip_rect_stack_count;
    int clip_rect_stack_capacity;

    /* Texture id stack. */
    uint32_t *texture_stack;
    int texture_stack_count;
    int texture_stack_capacity;

    /* Channels (draw-list splitting). */
    eli_draw_channel *channels;
    int channels_count;
    int channels_current;
    int channels_capacity;

    /* Working state / shared config. */
    uint32_t vtx_current_idx;      /* base index for the next vertex written */
    eli_vec4 cmd_clip_rect;        /* current header clip (min.x,min.y,max.x,max.y) */
    uint32_t cmd_texture_id;       /* current header texture id */
    eli_vec4 clip_rect_fullscreen; /* default/fullscreen clip bounds */
    eli_vec2 tex_uv_white_pixel;   /* UV of an opaque white texel */
    float fringe_scale;            /* 1.0 at 1x scale */
    float circle_segment_max_error;/* tessellation error for auto segment counts */
};

/* ---------------------------------------------------------------------------
 * Draw data
 * ------------------------------------------------------------------------- */

/**
 * The finished output of a frame: an array of draw lists plus totals and the
 * display transform. Handed to a renderer backend by eli_get_draw_data.
 */
typedef struct eli_draw_data {
    bool valid;
    int cmd_lists_count;
    eli_draw_list **cmd_lists;
    int total_idx_count;
    int total_vtx_count;
    eli_vec2 display_pos;
    eli_vec2 display_size;
    eli_vec2 framebuffer_scale;
} eli_draw_data;

#endif /* ELI_DRAW_ELI_DRAW_TYPES_H */
