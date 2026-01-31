/*
 * eli_draw.h - Draw System
 *
 * Draw lists, draw commands, and primitive rendering.
 */

#ifndef ELI_DRAW_H
#define ELI_DRAW_H

#include "elimgui.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * DRAW TYPES
 *===========================================================================*/

/* Index type (16-bit for WebGL compatibility) */
typedef uint16_t eli_draw_idx;

/* Texture ID (opaque handle) */
typedef void* eli_texture_id;

/* Draw callback function pointer */
typedef void (*eli_draw_callback)(const struct eli_draw_list* parent_list,
                                   const struct eli_draw_cmd* cmd);

/* Special callback values */
#define ELI_DRAW_CALLBACK_RESET_RENDER_STATE ((eli_draw_callback)(intptr_t)-8)

/*============================================================================
 * DRAW VERTEX
 *===========================================================================*/

/* Vertex format: 20 bytes per vertex */
typedef struct eli_draw_vert {
    eli_vec2 pos;       /* Position (8 bytes) */
    eli_vec2 uv;        /* Texture coordinate (8 bytes) */
    uint32_t col;       /* Packed RGBA color (4 bytes) */
} eli_draw_vert;

/* Vertex layout offsets (for renderer setup) */
#define ELI_DRAW_VERT_POS_OFF  0
#define ELI_DRAW_VERT_UV_OFF   8
#define ELI_DRAW_VERT_COL_OFF  16
#define ELI_DRAW_VERT_SIZE     20

/*============================================================================
 * DRAW FLAGS
 *===========================================================================*/

typedef enum eli_draw_flags {
    ELI_DRAW_FLAGS_NONE                     = 0,
    ELI_DRAW_FLAGS_CLOSED                   = 1 << 0,  /* Close polyline/path */

    /* Round corner flags */
    ELI_DRAW_FLAGS_ROUND_CORNERS_TOP_LEFT     = 1 << 4,
    ELI_DRAW_FLAGS_ROUND_CORNERS_TOP_RIGHT    = 1 << 5,
    ELI_DRAW_FLAGS_ROUND_CORNERS_BOTTOM_LEFT  = 1 << 6,
    ELI_DRAW_FLAGS_ROUND_CORNERS_BOTTOM_RIGHT = 1 << 7,
    ELI_DRAW_FLAGS_ROUND_CORNERS_NONE         = 1 << 8,

    /* Convenience combinations */
    ELI_DRAW_FLAGS_ROUND_CORNERS_TOP = ELI_DRAW_FLAGS_ROUND_CORNERS_TOP_LEFT |
                                       ELI_DRAW_FLAGS_ROUND_CORNERS_TOP_RIGHT,
    ELI_DRAW_FLAGS_ROUND_CORNERS_BOTTOM = ELI_DRAW_FLAGS_ROUND_CORNERS_BOTTOM_LEFT |
                                          ELI_DRAW_FLAGS_ROUND_CORNERS_BOTTOM_RIGHT,
    ELI_DRAW_FLAGS_ROUND_CORNERS_LEFT = ELI_DRAW_FLAGS_ROUND_CORNERS_TOP_LEFT |
                                        ELI_DRAW_FLAGS_ROUND_CORNERS_BOTTOM_LEFT,
    ELI_DRAW_FLAGS_ROUND_CORNERS_RIGHT = ELI_DRAW_FLAGS_ROUND_CORNERS_TOP_RIGHT |
                                         ELI_DRAW_FLAGS_ROUND_CORNERS_BOTTOM_RIGHT,
    ELI_DRAW_FLAGS_ROUND_CORNERS_ALL = ELI_DRAW_FLAGS_ROUND_CORNERS_TOP_LEFT |
                                       ELI_DRAW_FLAGS_ROUND_CORNERS_TOP_RIGHT |
                                       ELI_DRAW_FLAGS_ROUND_CORNERS_BOTTOM_LEFT |
                                       ELI_DRAW_FLAGS_ROUND_CORNERS_BOTTOM_RIGHT,
    ELI_DRAW_FLAGS_ROUND_CORNERS_DEFAULT = ELI_DRAW_FLAGS_ROUND_CORNERS_ALL,
    ELI_DRAW_FLAGS_ROUND_CORNERS_MASK = ELI_DRAW_FLAGS_ROUND_CORNERS_ALL |
                                        ELI_DRAW_FLAGS_ROUND_CORNERS_NONE
} eli_draw_flags;

typedef enum eli_draw_list_flags {
    ELI_DRAW_LIST_FLAGS_NONE                    = 0,
    ELI_DRAW_LIST_FLAGS_ANTI_ALIASED_LINES      = 1 << 0,
    ELI_DRAW_LIST_FLAGS_ANTI_ALIASED_LINES_USE_TEX = 1 << 1,
    ELI_DRAW_LIST_FLAGS_ANTI_ALIASED_FILL       = 1 << 2,
    ELI_DRAW_LIST_FLAGS_ALLOW_VTX_OFFSET        = 1 << 3
} eli_draw_list_flags;

/*============================================================================
 * DRAW COMMAND
 *===========================================================================*/

typedef struct eli_draw_cmd {
    eli_vec4 clip_rect;         /* Clipping rectangle (x1, y1, x2, y2) */
    eli_texture_id texture_id;  /* Texture to bind */
    uint32_t vtx_offset;        /* Offset in vertex buffer */
    uint32_t idx_offset;        /* Offset in index buffer */
    uint32_t elem_count;        /* Number of indices (triangles * 3) */
    eli_draw_callback user_callback;      /* Optional callback */
    void* user_callback_data;   /* Callback user data */
} eli_draw_cmd;

/*============================================================================
 * DYNAMIC ARRAYS (simple C vector implementation)
 *===========================================================================*/

#define ELI_VECTOR_DEFINE(type, name) \
    typedef struct name { \
        type* data; \
        int size; \
        int capacity; \
    } name

ELI_VECTOR_DEFINE(eli_draw_cmd, eli_draw_cmd_array);
ELI_VECTOR_DEFINE(eli_draw_vert, eli_draw_vert_array);
ELI_VECTOR_DEFINE(eli_draw_idx, eli_draw_idx_array);
ELI_VECTOR_DEFINE(eli_vec2, eli_vec2_array);
ELI_VECTOR_DEFINE(eli_vec4, eli_vec4_array);
ELI_VECTOR_DEFINE(eli_texture_id, eli_texture_array);

/* Vector operations */
#define eli_vector_init(v) do { (v)->data = NULL; (v)->size = 0; (v)->capacity = 0; } while(0)

#define eli_vector_free(v) do { \
    if ((v)->data) { free((v)->data); (v)->data = NULL; } \
    (v)->size = 0; (v)->capacity = 0; \
} while(0)

#define eli_vector_clear(v) do { (v)->size = 0; } while(0)

#define eli_vector_reserve(v, new_cap, type) do { \
    if ((new_cap) > (v)->capacity) { \
        int _new_cap = (new_cap); \
        type* _new_data = (type*)realloc((v)->data, _new_cap * sizeof(type)); \
        if (_new_data) { (v)->data = _new_data; (v)->capacity = _new_cap; } \
    } \
} while(0)

#define eli_vector_grow(v, type) do { \
    int _new_cap = (v)->capacity ? (v)->capacity * 2 : 8; \
    eli_vector_reserve(v, _new_cap, type); \
} while(0)

#define eli_vector_push(v, item, type) do { \
    if ((v)->size >= (v)->capacity) { eli_vector_grow(v, type); } \
    (v)->data[(v)->size++] = (item); \
} while(0)

#define eli_vector_pop(v) ((v)->size > 0 ? (v)->size-- : 0)

#define eli_vector_resize(v, new_size, type) do { \
    if ((new_size) > (v)->capacity) { eli_vector_reserve(v, new_size, type); } \
    (v)->size = (new_size); \
} while(0)

#define eli_vector_back(v) ((v)->data[(v)->size - 1])

/*============================================================================
 * DRAW LIST
 *===========================================================================*/

typedef struct eli_draw_cmd_header {
    eli_vec4 clip_rect;
    eli_texture_id texture_id;
    uint32_t vtx_offset;
} eli_draw_cmd_header;

struct eli_draw_list {
    /* Output buffers */
    eli_draw_cmd_array cmd_buffer;
    eli_draw_vert_array vtx_buffer;
    eli_draw_idx_array idx_buffer;

    /* Flags */
    eli_draw_list_flags flags;

    /* Internal state */
    uint32_t _vtx_current_idx;
    eli_draw_vert* _vtx_write_ptr;
    eli_draw_idx* _idx_write_ptr;

    /* Path building */
    eli_vec2_array _path;

    /* Command template */
    eli_draw_cmd_header _cmd_header;

    /* Stacks */
    eli_vec4_array _clip_rect_stack;
    eli_texture_array _texture_stack;

    /* Anti-aliasing */
    float _fringe_scale;

    /* Shared data */
    eli_vec2 _tex_uv_white_pixel;

    /* Owner name (for debugging) */
    const char* _owner_name;
};

/*============================================================================
 * DRAW DATA
 *===========================================================================*/

typedef struct eli_draw_list_ptr_array {
    eli_draw_list** data;
    int size;
    int capacity;
} eli_draw_list_ptr_array;

struct eli_draw_data {
    bool valid;
    int cmd_lists_count;
    int total_idx_count;
    int total_vtx_count;
    eli_draw_list_ptr_array cmd_lists;
    eli_vec2 display_pos;
    eli_vec2 display_size;
    eli_vec2 framebuffer_scale;
};

/*============================================================================
 * DRAW LIST FUNCTIONS
 *===========================================================================*/

/* Initialization */
static inline void eli_draw_list_init(eli_draw_list* list) {
    eli_vector_init(&list->cmd_buffer);
    eli_vector_init(&list->vtx_buffer);
    eli_vector_init(&list->idx_buffer);
    eli_vector_init(&list->_path);
    eli_vector_init(&list->_clip_rect_stack);
    eli_vector_init(&list->_texture_stack);

    list->flags = ELI_DRAW_LIST_FLAGS_ANTI_ALIASED_LINES |
                  ELI_DRAW_LIST_FLAGS_ANTI_ALIASED_FILL;
    list->_vtx_current_idx = 0;
    list->_vtx_write_ptr = NULL;
    list->_idx_write_ptr = NULL;
    list->_fringe_scale = 1.0f;
    list->_tex_uv_white_pixel = eli_make_vec2(0.0f, 0.0f);
    list->_owner_name = NULL;

    list->_cmd_header.clip_rect = eli_make_vec4(-8192.0f, -8192.0f, 8192.0f, 8192.0f);
    list->_cmd_header.texture_id = NULL;
    list->_cmd_header.vtx_offset = 0;
}

/* Cleanup */
static inline void eli_draw_list_destroy(eli_draw_list* list) {
    eli_vector_free(&list->cmd_buffer);
    eli_vector_free(&list->vtx_buffer);
    eli_vector_free(&list->idx_buffer);
    eli_vector_free(&list->_path);
    eli_vector_free(&list->_clip_rect_stack);
    eli_vector_free(&list->_texture_stack);
}

/* Clear for new frame */
static inline void eli_draw_list_clear(eli_draw_list* list) {
    eli_vector_clear(&list->cmd_buffer);
    eli_vector_clear(&list->vtx_buffer);
    eli_vector_clear(&list->idx_buffer);
    eli_vector_clear(&list->_path);
    eli_vector_clear(&list->_clip_rect_stack);
    eli_vector_clear(&list->_texture_stack);

    list->_vtx_current_idx = 0;
    list->_vtx_write_ptr = NULL;
    list->_idx_write_ptr = NULL;
    list->_cmd_header.vtx_offset = 0;
}

/* Add a new draw command */
static inline void eli_draw_list_add_draw_cmd(eli_draw_list* list) {
    eli_draw_cmd cmd;
    cmd.clip_rect = list->_cmd_header.clip_rect;
    cmd.texture_id = list->_cmd_header.texture_id;
    cmd.vtx_offset = list->_cmd_header.vtx_offset;
    cmd.idx_offset = list->idx_buffer.size;
    cmd.elem_count = 0;
    cmd.user_callback = NULL;
    cmd.user_callback_data = NULL;
    eli_vector_push(&list->cmd_buffer, cmd, eli_draw_cmd);
}

/* Reserve space for primitives */
static inline void eli_draw_list_prim_reserve(eli_draw_list* list, int idx_count, int vtx_count) {
    /* Handle 16-bit index overflow */
    if (sizeof(eli_draw_idx) == 2 &&
        (list->_vtx_current_idx + vtx_count >= (1 << 16)) &&
        (list->flags & ELI_DRAW_LIST_FLAGS_ALLOW_VTX_OFFSET)) {
        list->_cmd_header.vtx_offset = list->vtx_buffer.size;
        list->_vtx_current_idx = 0;
        eli_draw_list_add_draw_cmd(list);
    }

    /* Ensure we have a command */
    if (list->cmd_buffer.size == 0) {
        eli_draw_list_add_draw_cmd(list);
    }

    /* Grow vertex buffer */
    int vtx_old_size = list->vtx_buffer.size;
    eli_vector_resize(&list->vtx_buffer, vtx_old_size + vtx_count, eli_draw_vert);
    list->_vtx_write_ptr = list->vtx_buffer.data + vtx_old_size;

    /* Grow index buffer */
    int idx_old_size = list->idx_buffer.size;
    eli_vector_resize(&list->idx_buffer, idx_old_size + idx_count, eli_draw_idx);
    list->_idx_write_ptr = list->idx_buffer.data + idx_old_size;

    /* Update command element count */
    list->cmd_buffer.data[list->cmd_buffer.size - 1].elem_count += idx_count;
}

/* Unreserve (rollback) */
static inline void eli_draw_list_prim_unreserve(eli_draw_list* list, int idx_count, int vtx_count) {
    list->vtx_buffer.size -= vtx_count;
    list->idx_buffer.size -= idx_count;
    list->cmd_buffer.data[list->cmd_buffer.size - 1].elem_count -= idx_count;
}

/* Write a vertex directly */
static inline void eli_draw_list_prim_write_vtx(eli_draw_list* list, eli_vec2 pos, eli_vec2 uv, uint32_t col) {
    list->_vtx_write_ptr->pos = pos;
    list->_vtx_write_ptr->uv = uv;
    list->_vtx_write_ptr->col = col;
    list->_vtx_write_ptr++;
    list->_vtx_current_idx++;
}

/* Write an index directly */
static inline void eli_draw_list_prim_write_idx(eli_draw_list* list, eli_draw_idx idx) {
    *list->_idx_write_ptr = idx;
    list->_idx_write_ptr++;
}

/* Single vertex (position only, adds to current idx) */
static inline void eli_draw_list_prim_vtx(eli_draw_list* list, eli_vec2 pos, eli_vec2 uv, uint32_t col) {
    eli_draw_list_prim_write_idx(list, (eli_draw_idx)list->_vtx_current_idx);
    eli_draw_list_prim_write_vtx(list, pos, uv, col);
}

/*============================================================================
 * CLIP RECT STACK
 *===========================================================================*/

static inline void eli_draw_list_push_clip_rect(eli_draw_list* list, eli_vec2 min, eli_vec2 max, bool intersect) {
    eli_vec4 cr = eli_make_vec4(min.x, min.y, max.x, max.y);

    if (intersect && list->_clip_rect_stack.size > 0) {
        eli_vec4 cur = eli_vector_back(&list->_clip_rect_stack);
        if (cr.x < cur.x) cr.x = cur.x;
        if (cr.y < cur.y) cr.y = cur.y;
        if (cr.z > cur.z) cr.z = cur.z;
        if (cr.w > cur.w) cr.w = cur.w;
    }
    cr.z = (cr.z > cr.x) ? cr.z : cr.x;
    cr.w = (cr.w > cr.y) ? cr.w : cr.y;

    eli_vector_push(&list->_clip_rect_stack, cr, eli_vec4);
    list->_cmd_header.clip_rect = cr;
    eli_draw_list_add_draw_cmd(list);
}

static inline void eli_draw_list_push_clip_rect_full_screen(eli_draw_list* list) {
    eli_draw_list_push_clip_rect(list,
        eli_make_vec2(-8192.0f, -8192.0f),
        eli_make_vec2(8192.0f, 8192.0f),
        false);
}

static inline void eli_draw_list_pop_clip_rect(eli_draw_list* list) {
    eli_vector_pop(&list->_clip_rect_stack);
    eli_vec4 cr = (list->_clip_rect_stack.size > 0)
        ? eli_vector_back(&list->_clip_rect_stack)
        : eli_make_vec4(-8192.0f, -8192.0f, 8192.0f, 8192.0f);
    list->_cmd_header.clip_rect = cr;
    eli_draw_list_add_draw_cmd(list);
}

static inline eli_vec2 eli_draw_list_get_clip_rect_min(eli_draw_list* list) {
    return eli_make_vec2(list->_cmd_header.clip_rect.x, list->_cmd_header.clip_rect.y);
}

static inline eli_vec2 eli_draw_list_get_clip_rect_max(eli_draw_list* list) {
    return eli_make_vec2(list->_cmd_header.clip_rect.z, list->_cmd_header.clip_rect.w);
}

/*============================================================================
 * TEXTURE STACK
 *===========================================================================*/

static inline void eli_draw_list_push_texture_id(eli_draw_list* list, eli_texture_id tex_id) {
    eli_vector_push(&list->_texture_stack, tex_id, eli_texture_id);
    list->_cmd_header.texture_id = tex_id;
    eli_draw_list_add_draw_cmd(list);
}

static inline void eli_draw_list_pop_texture_id(eli_draw_list* list) {
    eli_vector_pop(&list->_texture_stack);
    eli_texture_id tex_id = (list->_texture_stack.size > 0)
        ? eli_vector_back(&list->_texture_stack)
        : NULL;
    list->_cmd_header.texture_id = tex_id;
    eli_draw_list_add_draw_cmd(list);
}

/*============================================================================
 * PATH API
 *===========================================================================*/

static inline void eli_draw_list_path_clear(eli_draw_list* list) {
    eli_vector_clear(&list->_path);
}

static inline void eli_draw_list_path_line_to(eli_draw_list* list, eli_vec2 pos) {
    eli_vector_push(&list->_path, pos, eli_vec2);
}

static inline void eli_draw_list_path_line_to_merge_duplicate(eli_draw_list* list, eli_vec2 pos) {
    if (list->_path.size == 0 ||
        list->_path.data[list->_path.size - 1].x != pos.x ||
        list->_path.data[list->_path.size - 1].y != pos.y) {
        eli_vector_push(&list->_path, pos, eli_vec2);
    }
}

/* Forward declarations for path finish functions */
static void eli_draw_list_add_convex_poly_filled(eli_draw_list* list, const eli_vec2* points, int count, uint32_t col);
static void eli_draw_list_add_polyline(eli_draw_list* list, const eli_vec2* points, int count, uint32_t col, eli_draw_flags flags, float thickness);

static inline void eli_draw_list_path_fill_convex(eli_draw_list* list, uint32_t col) {
    eli_draw_list_add_convex_poly_filled(list, list->_path.data, list->_path.size, col);
    eli_vector_clear(&list->_path);
}

static inline void eli_draw_list_path_stroke(eli_draw_list* list, uint32_t col, eli_draw_flags flags, float thickness) {
    eli_draw_list_add_polyline(list, list->_path.data, list->_path.size, col, flags, thickness);
    eli_vector_clear(&list->_path);
}

/* Forward declaration for rounded path rect */
static inline void eli_draw_list_path_rect_rounded(eli_draw_list* list, eli_vec2 min, eli_vec2 max, float rounding, eli_draw_flags flags);

/* Path rectangle - delegates to rounded version */
static inline void eli_draw_list_path_rect(eli_draw_list* list, eli_vec2 min, eli_vec2 max, float rounding, eli_draw_flags flags) {
    eli_draw_list_path_rect_rounded(list, min, max, rounding, flags);
}

/*============================================================================
 * PRIMITIVE RENDERING
 *===========================================================================*/

/* Add a single filled rectangle (4 vertices, 6 indices) */
static inline void eli_draw_list_prim_rect(eli_draw_list* list, eli_vec2 a, eli_vec2 c, uint32_t col) {
    eli_vec2 b = eli_make_vec2(c.x, a.y);
    eli_vec2 d = eli_make_vec2(a.x, c.y);
    eli_vec2 uv = list->_tex_uv_white_pixel;
    eli_draw_idx idx = (eli_draw_idx)list->_vtx_current_idx;

    list->_idx_write_ptr[0] = idx;
    list->_idx_write_ptr[1] = (eli_draw_idx)(idx + 1);
    list->_idx_write_ptr[2] = (eli_draw_idx)(idx + 2);
    list->_idx_write_ptr[3] = idx;
    list->_idx_write_ptr[4] = (eli_draw_idx)(idx + 2);
    list->_idx_write_ptr[5] = (eli_draw_idx)(idx + 3);

    list->_vtx_write_ptr[0].pos = a; list->_vtx_write_ptr[0].uv = uv; list->_vtx_write_ptr[0].col = col;
    list->_vtx_write_ptr[1].pos = b; list->_vtx_write_ptr[1].uv = uv; list->_vtx_write_ptr[1].col = col;
    list->_vtx_write_ptr[2].pos = c; list->_vtx_write_ptr[2].uv = uv; list->_vtx_write_ptr[2].col = col;
    list->_vtx_write_ptr[3].pos = d; list->_vtx_write_ptr[3].uv = uv; list->_vtx_write_ptr[3].col = col;

    list->_vtx_write_ptr += 4;
    list->_vtx_current_idx += 4;
    list->_idx_write_ptr += 6;
}

/* Add a single textured rectangle */
static inline void eli_draw_list_prim_rect_uv(eli_draw_list* list, eli_vec2 a, eli_vec2 c, eli_vec2 uv_a, eli_vec2 uv_c, uint32_t col) {
    eli_vec2 b = eli_make_vec2(c.x, a.y);
    eli_vec2 d = eli_make_vec2(a.x, c.y);
    eli_vec2 uv_b = eli_make_vec2(uv_c.x, uv_a.y);
    eli_vec2 uv_d = eli_make_vec2(uv_a.x, uv_c.y);
    eli_draw_idx idx = (eli_draw_idx)list->_vtx_current_idx;

    list->_idx_write_ptr[0] = idx;
    list->_idx_write_ptr[1] = (eli_draw_idx)(idx + 1);
    list->_idx_write_ptr[2] = (eli_draw_idx)(idx + 2);
    list->_idx_write_ptr[3] = idx;
    list->_idx_write_ptr[4] = (eli_draw_idx)(idx + 2);
    list->_idx_write_ptr[5] = (eli_draw_idx)(idx + 3);

    list->_vtx_write_ptr[0].pos = a; list->_vtx_write_ptr[0].uv = uv_a; list->_vtx_write_ptr[0].col = col;
    list->_vtx_write_ptr[1].pos = b; list->_vtx_write_ptr[1].uv = uv_b; list->_vtx_write_ptr[1].col = col;
    list->_vtx_write_ptr[2].pos = c; list->_vtx_write_ptr[2].uv = uv_c; list->_vtx_write_ptr[2].col = col;
    list->_vtx_write_ptr[3].pos = d; list->_vtx_write_ptr[3].uv = uv_d; list->_vtx_write_ptr[3].col = col;

    list->_vtx_write_ptr += 4;
    list->_vtx_current_idx += 4;
    list->_idx_write_ptr += 6;
}

/* Add a textured quad with arbitrary UVs */
static inline void eli_draw_list_prim_quad_uv(eli_draw_list* list,
    eli_vec2 a, eli_vec2 b, eli_vec2 c, eli_vec2 d,
    eli_vec2 uv_a, eli_vec2 uv_b, eli_vec2 uv_c, eli_vec2 uv_d,
    uint32_t col)
{
    eli_draw_idx idx = (eli_draw_idx)list->_vtx_current_idx;

    list->_idx_write_ptr[0] = idx;
    list->_idx_write_ptr[1] = (eli_draw_idx)(idx + 1);
    list->_idx_write_ptr[2] = (eli_draw_idx)(idx + 2);
    list->_idx_write_ptr[3] = idx;
    list->_idx_write_ptr[4] = (eli_draw_idx)(idx + 2);
    list->_idx_write_ptr[5] = (eli_draw_idx)(idx + 3);

    list->_vtx_write_ptr[0].pos = a; list->_vtx_write_ptr[0].uv = uv_a; list->_vtx_write_ptr[0].col = col;
    list->_vtx_write_ptr[1].pos = b; list->_vtx_write_ptr[1].uv = uv_b; list->_vtx_write_ptr[1].col = col;
    list->_vtx_write_ptr[2].pos = c; list->_vtx_write_ptr[2].uv = uv_c; list->_vtx_write_ptr[2].col = col;
    list->_vtx_write_ptr[3].pos = d; list->_vtx_write_ptr[3].uv = uv_d; list->_vtx_write_ptr[3].col = col;

    list->_vtx_write_ptr += 4;
    list->_vtx_current_idx += 4;
    list->_idx_write_ptr += 6;
}

/*============================================================================
 * HIGH-LEVEL PRIMITIVES
 *===========================================================================*/

/* Draw a line */
static inline void eli_draw_list_add_line(eli_draw_list* list, eli_vec2 p1, eli_vec2 p2, uint32_t col, float thickness) {
    if ((col & ELI_COL32_A_MASK) == 0) return;
    eli_draw_list_path_line_to(list, eli_make_vec2(p1.x + 0.5f, p1.y + 0.5f));
    eli_draw_list_path_line_to(list, eli_make_vec2(p2.x + 0.5f, p2.y + 0.5f));
    eli_draw_list_path_stroke(list, col, ELI_DRAW_FLAGS_NONE, thickness);
}

/* Draw a rectangle outline */
static inline void eli_draw_list_add_rect(eli_draw_list* list, eli_vec2 min, eli_vec2 max, uint32_t col, float rounding, eli_draw_flags flags, float thickness) {
    if ((col & ELI_COL32_A_MASK) == 0) return;
    if (flags == 0) flags = ELI_DRAW_FLAGS_ROUND_CORNERS_DEFAULT;
    eli_draw_list_path_rect(list,
        eli_make_vec2(min.x + 0.5f, min.y + 0.5f),
        eli_make_vec2(max.x - 0.5f, max.y - 0.5f),
        rounding, flags);
    eli_draw_list_path_stroke(list, col, ELI_DRAW_FLAGS_CLOSED, thickness);
}

/* Draw a filled rectangle */
static inline void eli_draw_list_add_rect_filled(eli_draw_list* list, eli_vec2 min, eli_vec2 max, uint32_t col, float rounding, eli_draw_flags flags) {
    if ((col & ELI_COL32_A_MASK) == 0) return;

    if (rounding < 0.5f || (flags & ELI_DRAW_FLAGS_ROUND_CORNERS_MASK) == ELI_DRAW_FLAGS_ROUND_CORNERS_NONE) {
        eli_draw_list_prim_reserve(list, 6, 4);
        eli_draw_list_prim_rect(list, min, max, col);
    } else {
        if (flags == 0) flags = ELI_DRAW_FLAGS_ROUND_CORNERS_DEFAULT;
        eli_draw_list_path_rect(list, min, max, rounding, flags);
        eli_draw_list_path_fill_convex(list, col);
    }
}

/* Draw a filled rectangle with per-corner colors */
static inline void eli_draw_list_add_rect_filled_multi_color(eli_draw_list* list,
    eli_vec2 min, eli_vec2 max,
    uint32_t col_tl, uint32_t col_tr, uint32_t col_br, uint32_t col_bl)
{
    if (((col_tl | col_tr | col_br | col_bl) & ELI_COL32_A_MASK) == 0) return;

    eli_vec2 uv = list->_tex_uv_white_pixel;
    eli_draw_list_prim_reserve(list, 6, 4);

    eli_draw_idx idx = (eli_draw_idx)list->_vtx_current_idx;
    list->_idx_write_ptr[0] = idx;
    list->_idx_write_ptr[1] = (eli_draw_idx)(idx + 1);
    list->_idx_write_ptr[2] = (eli_draw_idx)(idx + 2);
    list->_idx_write_ptr[3] = idx;
    list->_idx_write_ptr[4] = (eli_draw_idx)(idx + 2);
    list->_idx_write_ptr[5] = (eli_draw_idx)(idx + 3);

    list->_vtx_write_ptr[0].pos = min;
    list->_vtx_write_ptr[0].uv = uv;
    list->_vtx_write_ptr[0].col = col_tl;

    list->_vtx_write_ptr[1].pos = eli_make_vec2(max.x, min.y);
    list->_vtx_write_ptr[1].uv = uv;
    list->_vtx_write_ptr[1].col = col_tr;

    list->_vtx_write_ptr[2].pos = max;
    list->_vtx_write_ptr[2].uv = uv;
    list->_vtx_write_ptr[2].col = col_br;

    list->_vtx_write_ptr[3].pos = eli_make_vec2(min.x, max.y);
    list->_vtx_write_ptr[3].uv = uv;
    list->_vtx_write_ptr[3].col = col_bl;

    list->_vtx_write_ptr += 4;
    list->_vtx_current_idx += 4;
    list->_idx_write_ptr += 6;
}

/* Draw a triangle outline */
static inline void eli_draw_list_add_triangle(eli_draw_list* list, eli_vec2 p1, eli_vec2 p2, eli_vec2 p3, uint32_t col, float thickness) {
    if ((col & ELI_COL32_A_MASK) == 0) return;
    eli_draw_list_path_line_to(list, p1);
    eli_draw_list_path_line_to(list, p2);
    eli_draw_list_path_line_to(list, p3);
    eli_draw_list_path_stroke(list, col, ELI_DRAW_FLAGS_CLOSED, thickness);
}

/* Draw a filled triangle */
static inline void eli_draw_list_add_triangle_filled(eli_draw_list* list, eli_vec2 p1, eli_vec2 p2, eli_vec2 p3, uint32_t col) {
    if ((col & ELI_COL32_A_MASK) == 0) return;
    eli_draw_list_path_line_to(list, p1);
    eli_draw_list_path_line_to(list, p2);
    eli_draw_list_path_line_to(list, p3);
    eli_draw_list_path_fill_convex(list, col);
}

/* Draw a quad outline */
static inline void eli_draw_list_add_quad(eli_draw_list* list, eli_vec2 p1, eli_vec2 p2, eli_vec2 p3, eli_vec2 p4, uint32_t col, float thickness) {
    if ((col & ELI_COL32_A_MASK) == 0) return;
    eli_draw_list_path_line_to(list, p1);
    eli_draw_list_path_line_to(list, p2);
    eli_draw_list_path_line_to(list, p3);
    eli_draw_list_path_line_to(list, p4);
    eli_draw_list_path_stroke(list, col, ELI_DRAW_FLAGS_CLOSED, thickness);
}

/* Draw a filled quad */
static inline void eli_draw_list_add_quad_filled(eli_draw_list* list, eli_vec2 p1, eli_vec2 p2, eli_vec2 p3, eli_vec2 p4, uint32_t col) {
    if ((col & ELI_COL32_A_MASK) == 0) return;
    eli_draw_list_path_line_to(list, p1);
    eli_draw_list_path_line_to(list, p2);
    eli_draw_list_path_line_to(list, p3);
    eli_draw_list_path_line_to(list, p4);
    eli_draw_list_path_fill_convex(list, col);
}

/*============================================================================
 * CIRCLE / ELLIPSE PRIMITIVES
 *===========================================================================*/

#ifndef ELI_PI
#define ELI_PI 3.14159265358979323846f
#endif

/* Get auto-calculated circle segment count */
static inline int eli_draw_list_calc_circle_auto_segment_count(float radius, float max_error) {
    if (radius <= 0.0f) return 4;
    int count = (int)((ELI_PI * 2.0f) / acosf((radius - max_error) / radius));
    if (count < 4) count = 4;
    if (count > 512) count = 512;
    return count;
}

/* Draw a circle outline */
static inline void eli_draw_list_add_circle(eli_draw_list* list, eli_vec2 center, float radius, uint32_t col, int num_segments, float thickness) {
    if ((col & ELI_COL32_A_MASK) == 0 || radius < 0.5f) return;

    if (num_segments <= 0) {
        num_segments = eli_draw_list_calc_circle_auto_segment_count(radius, 0.3f);
    }

    float a_step = (ELI_PI * 2.0f) / (float)num_segments;
    for (int i = 0; i < num_segments; i++) {
        float a = (float)i * a_step;
        eli_draw_list_path_line_to(list, eli_make_vec2(
            center.x + cosf(a) * radius,
            center.y + sinf(a) * radius));
    }
    eli_draw_list_path_stroke(list, col, ELI_DRAW_FLAGS_CLOSED, thickness);
}

/* Draw a filled circle */
static inline void eli_draw_list_add_circle_filled(eli_draw_list* list, eli_vec2 center, float radius, uint32_t col, int num_segments) {
    if ((col & ELI_COL32_A_MASK) == 0 || radius < 0.5f) return;

    if (num_segments <= 0) {
        num_segments = eli_draw_list_calc_circle_auto_segment_count(radius, 0.3f);
    }

    float a_step = (ELI_PI * 2.0f) / (float)num_segments;
    for (int i = 0; i < num_segments; i++) {
        float a = (float)i * a_step;
        eli_draw_list_path_line_to(list, eli_make_vec2(
            center.x + cosf(a) * radius,
            center.y + sinf(a) * radius));
    }
    eli_draw_list_path_fill_convex(list, col);
}

/* Draw an ngon outline */
static inline void eli_draw_list_add_ngon(eli_draw_list* list, eli_vec2 center, float radius, uint32_t col, int num_segments, float thickness) {
    if ((col & ELI_COL32_A_MASK) == 0 || num_segments < 3) return;
    eli_draw_list_add_circle(list, center, radius, col, num_segments, thickness);
}

/* Draw a filled ngon */
static inline void eli_draw_list_add_ngon_filled(eli_draw_list* list, eli_vec2 center, float radius, uint32_t col, int num_segments) {
    if ((col & ELI_COL32_A_MASK) == 0 || num_segments < 3) return;
    eli_draw_list_add_circle_filled(list, center, radius, col, num_segments);
}

/* Draw an ellipse outline */
static inline void eli_draw_list_add_ellipse(eli_draw_list* list, eli_vec2 center, eli_vec2 radius, uint32_t col, float rot, int num_segments, float thickness) {
    if ((col & ELI_COL32_A_MASK) == 0) return;

    if (num_segments <= 0) {
        float r = (radius.x > radius.y) ? radius.x : radius.y;
        num_segments = eli_draw_list_calc_circle_auto_segment_count(r, 0.3f);
    }

    float cos_rot = cosf(rot);
    float sin_rot = sinf(rot);
    float a_step = (ELI_PI * 2.0f) / (float)num_segments;

    for (int i = 0; i < num_segments; i++) {
        float a = (float)i * a_step;
        float x = cosf(a) * radius.x;
        float y = sinf(a) * radius.y;
        float rx = x * cos_rot - y * sin_rot;
        float ry = x * sin_rot + y * cos_rot;
        eli_draw_list_path_line_to(list, eli_make_vec2(center.x + rx, center.y + ry));
    }
    eli_draw_list_path_stroke(list, col, ELI_DRAW_FLAGS_CLOSED, thickness);
}

/* Draw a filled ellipse */
static inline void eli_draw_list_add_ellipse_filled(eli_draw_list* list, eli_vec2 center, eli_vec2 radius, uint32_t col, float rot, int num_segments) {
    if ((col & ELI_COL32_A_MASK) == 0) return;

    if (num_segments <= 0) {
        float r = (radius.x > radius.y) ? radius.x : radius.y;
        num_segments = eli_draw_list_calc_circle_auto_segment_count(r, 0.3f);
    }

    float cos_rot = cosf(rot);
    float sin_rot = sinf(rot);
    float a_step = (ELI_PI * 2.0f) / (float)num_segments;

    for (int i = 0; i < num_segments; i++) {
        float a = (float)i * a_step;
        float x = cosf(a) * radius.x;
        float y = sinf(a) * radius.y;
        float rx = x * cos_rot - y * sin_rot;
        float ry = x * sin_rot + y * cos_rot;
        eli_draw_list_path_line_to(list, eli_make_vec2(center.x + rx, center.y + ry));
    }
    eli_draw_list_path_fill_convex(list, col);
}

/*============================================================================
 * POLYLINE AND CONVEX POLY FILL
 *===========================================================================*/

/* Add a polyline (stroked path) - simplified non-AA version */
static void eli_draw_list_add_polyline(eli_draw_list* list, const eli_vec2* points, int count, uint32_t col, eli_draw_flags flags, float thickness) {
    if (count < 2 || (col & ELI_COL32_A_MASK) == 0) return;

    bool closed = (flags & ELI_DRAW_FLAGS_CLOSED) != 0;
    int segments = closed ? count : count - 1;

    /* For simplicity, using thick line quads (non-AA) */
    float half_thickness = thickness * 0.5f;
    eli_vec2 uv = list->_tex_uv_white_pixel;

    eli_draw_list_prim_reserve(list, segments * 6, segments * 4);

    for (int i = 0; i < segments; i++) {
        int i1 = i;
        int i2 = (i + 1) % count;
        eli_vec2 p1 = points[i1];
        eli_vec2 p2 = points[i2];

        /* Direction and normal */
        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;
        float len = sqrtf(dx * dx + dy * dy);
        if (len < 0.0001f) continue;

        float inv_len = 1.0f / len;
        dx *= inv_len;
        dy *= inv_len;

        float nx = -dy * half_thickness;
        float ny = dx * half_thickness;

        /* Quad corners */
        eli_vec2 a = eli_make_vec2(p1.x + nx, p1.y + ny);
        eli_vec2 b = eli_make_vec2(p2.x + nx, p2.y + ny);
        eli_vec2 c = eli_make_vec2(p2.x - nx, p2.y - ny);
        eli_vec2 d = eli_make_vec2(p1.x - nx, p1.y - ny);

        eli_draw_idx idx = (eli_draw_idx)list->_vtx_current_idx;

        list->_idx_write_ptr[0] = idx;
        list->_idx_write_ptr[1] = (eli_draw_idx)(idx + 1);
        list->_idx_write_ptr[2] = (eli_draw_idx)(idx + 2);
        list->_idx_write_ptr[3] = idx;
        list->_idx_write_ptr[4] = (eli_draw_idx)(idx + 2);
        list->_idx_write_ptr[5] = (eli_draw_idx)(idx + 3);

        list->_vtx_write_ptr[0].pos = a; list->_vtx_write_ptr[0].uv = uv; list->_vtx_write_ptr[0].col = col;
        list->_vtx_write_ptr[1].pos = b; list->_vtx_write_ptr[1].uv = uv; list->_vtx_write_ptr[1].col = col;
        list->_vtx_write_ptr[2].pos = c; list->_vtx_write_ptr[2].uv = uv; list->_vtx_write_ptr[2].col = col;
        list->_vtx_write_ptr[3].pos = d; list->_vtx_write_ptr[3].uv = uv; list->_vtx_write_ptr[3].col = col;

        list->_vtx_write_ptr += 4;
        list->_vtx_current_idx += 4;
        list->_idx_write_ptr += 6;
    }
}

/* Add a filled convex polygon */
static void eli_draw_list_add_convex_poly_filled(eli_draw_list* list, const eli_vec2* points, int count, uint32_t col) {
    if (count < 3 || (col & ELI_COL32_A_MASK) == 0) return;

    eli_vec2 uv = list->_tex_uv_white_pixel;
    int idx_count = (count - 2) * 3;
    int vtx_count = count;

    eli_draw_list_prim_reserve(list, idx_count, vtx_count);

    /* Write vertices */
    for (int i = 0; i < count; i++) {
        list->_vtx_write_ptr[i].pos = points[i];
        list->_vtx_write_ptr[i].uv = uv;
        list->_vtx_write_ptr[i].col = col;
    }

    /* Write indices (triangle fan) */
    eli_draw_idx idx0 = (eli_draw_idx)list->_vtx_current_idx;
    for (int i = 2; i < count; i++) {
        list->_idx_write_ptr[0] = idx0;
        list->_idx_write_ptr[1] = (eli_draw_idx)(idx0 + i - 1);
        list->_idx_write_ptr[2] = (eli_draw_idx)(idx0 + i);
        list->_idx_write_ptr += 3;
    }

    list->_vtx_write_ptr += vtx_count;
    list->_vtx_current_idx += vtx_count;
}

/*============================================================================
 * BEZIER CURVES
 *===========================================================================*/

/* Compute a point on a cubic bezier curve */
static inline eli_vec2 eli_bezier_cubic_calc(eli_vec2 p1, eli_vec2 p2, eli_vec2 p3, eli_vec2 p4, float t) {
    float u = 1.0f - t;
    float w1 = u * u * u;
    float w2 = 3.0f * u * u * t;
    float w3 = 3.0f * u * t * t;
    float w4 = t * t * t;
    return eli_make_vec2(
        w1 * p1.x + w2 * p2.x + w3 * p3.x + w4 * p4.x,
        w1 * p1.y + w2 * p2.y + w3 * p3.y + w4 * p4.y);
}

/* Compute a point on a quadratic bezier curve */
static inline eli_vec2 eli_bezier_quadratic_calc(eli_vec2 p1, eli_vec2 p2, eli_vec2 p3, float t) {
    float u = 1.0f - t;
    float w1 = u * u;
    float w2 = 2.0f * u * t;
    float w3 = t * t;
    return eli_make_vec2(
        w1 * p1.x + w2 * p2.x + w3 * p3.x,
        w1 * p1.y + w2 * p2.y + w3 * p3.y);
}

/* Path: Add cubic bezier curve */
static inline void eli_draw_list_path_bezier_cubic_curve_to(eli_draw_list* list,
    eli_vec2 p2, eli_vec2 p3, eli_vec2 p4, int num_segments)
{
    eli_vec2 p1 = (list->_path.size > 0) ? list->_path.data[list->_path.size - 1] : eli_make_vec2(0, 0);

    if (num_segments == 0) {
        /* Auto-calculate segments based on curve length approximation */
        float dx = p4.x - p1.x;
        float dy = p4.y - p1.y;
        float d = sqrtf(dx * dx + dy * dy);
        num_segments = (int)(d * 0.1f);
        if (num_segments < 4) num_segments = 4;
        if (num_segments > 100) num_segments = 100;
    }

    float t_step = 1.0f / (float)num_segments;
    for (int i = 1; i <= num_segments; i++) {
        float t = t_step * i;
        eli_vec2 p = eli_bezier_cubic_calc(p1, p2, p3, p4, t);
        eli_vector_push(&list->_path, p, eli_vec2);
    }
}

/* Path: Add quadratic bezier curve */
static inline void eli_draw_list_path_bezier_quadratic_curve_to(eli_draw_list* list,
    eli_vec2 p2, eli_vec2 p3, int num_segments)
{
    eli_vec2 p1 = (list->_path.size > 0) ? list->_path.data[list->_path.size - 1] : eli_make_vec2(0, 0);

    if (num_segments == 0) {
        float dx = p3.x - p1.x;
        float dy = p3.y - p1.y;
        float d = sqrtf(dx * dx + dy * dy);
        num_segments = (int)(d * 0.1f);
        if (num_segments < 4) num_segments = 4;
        if (num_segments > 100) num_segments = 100;
    }

    float t_step = 1.0f / (float)num_segments;
    for (int i = 1; i <= num_segments; i++) {
        float t = t_step * i;
        eli_vec2 p = eli_bezier_quadratic_calc(p1, p2, p3, t);
        eli_vector_push(&list->_path, p, eli_vec2);
    }
}

/* Draw a cubic bezier curve */
static inline void eli_draw_list_add_bezier_cubic(eli_draw_list* list,
    eli_vec2 p1, eli_vec2 p2, eli_vec2 p3, eli_vec2 p4,
    uint32_t col, float thickness, int num_segments)
{
    if ((col & ELI_COL32_A_MASK) == 0) return;

    eli_draw_list_path_line_to(list, p1);
    eli_draw_list_path_bezier_cubic_curve_to(list, p2, p3, p4, num_segments);
    eli_draw_list_path_stroke(list, col, ELI_DRAW_FLAGS_NONE, thickness);
}

/* Draw a quadratic bezier curve */
static inline void eli_draw_list_add_bezier_quadratic(eli_draw_list* list,
    eli_vec2 p1, eli_vec2 p2, eli_vec2 p3,
    uint32_t col, float thickness, int num_segments)
{
    if ((col & ELI_COL32_A_MASK) == 0) return;

    eli_draw_list_path_line_to(list, p1);
    eli_draw_list_path_bezier_quadratic_curve_to(list, p2, p3, num_segments);
    eli_draw_list_path_stroke(list, col, ELI_DRAW_FLAGS_NONE, thickness);
}

/*============================================================================
 * PATH ARC FUNCTIONS
 *===========================================================================*/

/* Path: Add arc (with manual segment count) */
static inline void eli_draw_list_path_arc_to(eli_draw_list* list,
    eli_vec2 center, float radius, float a_min, float a_max, int num_segments)
{
    if (radius < 0.5f) {
        eli_vector_push(&list->_path, center, eli_vec2);
        return;
    }

    if (num_segments <= 0) {
        num_segments = eli_draw_list_calc_circle_auto_segment_count(radius, 0.3f);
        /* Scale by arc angle */
        float arc_angle = fabsf(a_max - a_min);
        num_segments = (int)((float)num_segments * arc_angle / (ELI_PI * 2.0f)) + 1;
        if (num_segments < 2) num_segments = 2;
    }

    for (int i = 0; i <= num_segments; i++) {
        float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        eli_vec2 p = eli_make_vec2(
            center.x + cosf(a) * radius,
            center.y + sinf(a) * radius);
        eli_vector_push(&list->_path, p, eli_vec2);
    }
}

/* Precomputed lookup table for fast arc (12 points per quadrant) */
static const eli_vec2 _eli_arc_fast_vtx[48] = {
    { 1.000000f, 0.000000f }, { 0.965926f, 0.258819f }, { 0.866025f, 0.500000f }, { 0.707107f, 0.707107f },
    { 0.500000f, 0.866025f }, { 0.258819f, 0.965926f }, { 0.000000f, 1.000000f }, { -0.258819f, 0.965926f },
    { -0.500000f, 0.866025f }, { -0.707107f, 0.707107f }, { -0.866025f, 0.500000f }, { -0.965926f, 0.258819f },
    { -1.000000f, 0.000000f }, { -0.965926f, -0.258819f }, { -0.866025f, -0.500000f }, { -0.707107f, -0.707107f },
    { -0.500000f, -0.866025f }, { -0.258819f, -0.965926f }, { 0.000000f, -1.000000f }, { 0.258819f, -0.965926f },
    { 0.500000f, -0.866025f }, { 0.707107f, -0.707107f }, { 0.866025f, -0.500000f }, { 0.965926f, -0.258819f },
    { 1.000000f, 0.000000f }, { 0.965926f, 0.258819f }, { 0.866025f, 0.500000f }, { 0.707107f, 0.707107f },
    { 0.500000f, 0.866025f }, { 0.258819f, 0.965926f }, { 0.000000f, 1.000000f }, { -0.258819f, 0.965926f },
    { -0.500000f, 0.866025f }, { -0.707107f, 0.707107f }, { -0.866025f, 0.500000f }, { -0.965926f, 0.258819f },
    { -1.000000f, 0.000000f }, { -0.965926f, -0.258819f }, { -0.866025f, -0.500000f }, { -0.707107f, -0.707107f },
    { -0.500000f, -0.866025f }, { -0.258819f, -0.965926f }, { 0.000000f, -1.000000f }, { 0.258819f, -0.965926f },
    { 0.500000f, -0.866025f }, { 0.707107f, -0.707107f }, { 0.866025f, -0.500000f }, { 0.965926f, -0.258819f }
};
#define ELI_ARC_FAST_LOOKUP_SIZE 48

/* Path: Add arc using precomputed lookup (a_min_of_12 and a_max_of_12 are multiples of 12) */
static inline void eli_draw_list_path_arc_to_fast(eli_draw_list* list,
    eli_vec2 center, float radius, int a_min_of_12, int a_max_of_12)
{
    if (radius < 0.5f) {
        eli_vector_push(&list->_path, center, eli_vec2);
        return;
    }

    /* Normalize indices */
    if (a_min_of_12 > a_max_of_12) return;

    for (int a = a_min_of_12; a <= a_max_of_12; a++) {
        int idx = a % ELI_ARC_FAST_LOOKUP_SIZE;
        if (idx < 0) idx += ELI_ARC_FAST_LOOKUP_SIZE;
        eli_vec2 p = eli_make_vec2(
            center.x + _eli_arc_fast_vtx[idx].x * radius,
            center.y + _eli_arc_fast_vtx[idx].y * radius);
        eli_vector_push(&list->_path, p, eli_vec2);
    }
}

/* Path: Add elliptical arc */
static inline void eli_draw_list_path_elliptical_arc_to(eli_draw_list* list,
    eli_vec2 center, eli_vec2 radius, float rot, float a_min, float a_max, int num_segments)
{
    if (radius.x < 0.5f && radius.y < 0.5f) {
        eli_vector_push(&list->_path, center, eli_vec2);
        return;
    }

    if (num_segments <= 0) {
        float r = (radius.x > radius.y) ? radius.x : radius.y;
        num_segments = eli_draw_list_calc_circle_auto_segment_count(r, 0.3f);
        float arc_angle = fabsf(a_max - a_min);
        num_segments = (int)((float)num_segments * arc_angle / (ELI_PI * 2.0f)) + 1;
        if (num_segments < 2) num_segments = 2;
    }

    float cos_rot = cosf(rot);
    float sin_rot = sinf(rot);

    for (int i = 0; i <= num_segments; i++) {
        float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        float x = cosf(a) * radius.x;
        float y = sinf(a) * radius.y;
        /* Rotate */
        float rx = x * cos_rot - y * sin_rot;
        float ry = x * sin_rot + y * cos_rot;
        eli_vec2 p = eli_make_vec2(center.x + rx, center.y + ry);
        eli_vector_push(&list->_path, p, eli_vec2);
    }
}

/*============================================================================
 * PATH RECT WITH ROUNDED CORNERS (using arcs)
 *===========================================================================*/

/* Internal: Add arc for rounded corner */
static inline void _eli_path_arc_to_n(eli_draw_list* list, eli_vec2 center, float radius, float a_min, float a_max, int num_segments) {
    if (num_segments <= 0) num_segments = 3;
    for (int i = 0; i <= num_segments; i++) {
        float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        eli_vec2 p = eli_make_vec2(
            center.x + cosf(a) * radius,
            center.y + sinf(a) * radius);
        eli_draw_list_path_line_to_merge_duplicate(list, p);
    }
}

/* Updated path_rect with proper rounded corners */
static inline void eli_draw_list_path_rect_rounded(eli_draw_list* list,
    eli_vec2 min, eli_vec2 max, float rounding, eli_draw_flags flags)
{
    /* Clamp rounding */
    float w = max.x - min.x;
    float h = max.y - min.y;
    float rounding_max = ((w < h) ? w : h) * 0.5f - 1.0f;
    if (rounding > rounding_max) rounding = rounding_max;

    if (rounding < 0.5f || (flags & ELI_DRAW_FLAGS_ROUND_CORNERS_MASK) == ELI_DRAW_FLAGS_ROUND_CORNERS_NONE) {
        eli_draw_list_path_line_to(list, min);
        eli_draw_list_path_line_to(list, eli_make_vec2(max.x, min.y));
        eli_draw_list_path_line_to(list, max);
        eli_draw_list_path_line_to(list, eli_make_vec2(min.x, max.y));
        return;
    }

    /* Determine which corners to round */
    bool round_tl = (flags & ELI_DRAW_FLAGS_ROUND_CORNERS_TOP_LEFT) != 0;
    bool round_tr = (flags & ELI_DRAW_FLAGS_ROUND_CORNERS_TOP_RIGHT) != 0;
    bool round_br = (flags & ELI_DRAW_FLAGS_ROUND_CORNERS_BOTTOM_RIGHT) != 0;
    bool round_bl = (flags & ELI_DRAW_FLAGS_ROUND_CORNERS_BOTTOM_LEFT) != 0;

    float r = rounding;
    int arc_segments = 4;

    /* Top-left corner */
    if (round_tl) {
        _eli_path_arc_to_n(list, eli_make_vec2(min.x + r, min.y + r), r, ELI_PI, ELI_PI * 1.5f, arc_segments);
    } else {
        eli_draw_list_path_line_to(list, min);
    }

    /* Top-right corner */
    if (round_tr) {
        _eli_path_arc_to_n(list, eli_make_vec2(max.x - r, min.y + r), r, ELI_PI * 1.5f, ELI_PI * 2.0f, arc_segments);
    } else {
        eli_draw_list_path_line_to(list, eli_make_vec2(max.x, min.y));
    }

    /* Bottom-right corner */
    if (round_br) {
        _eli_path_arc_to_n(list, eli_make_vec2(max.x - r, max.y - r), r, 0.0f, ELI_PI * 0.5f, arc_segments);
    } else {
        eli_draw_list_path_line_to(list, max);
    }

    /* Bottom-left corner */
    if (round_bl) {
        _eli_path_arc_to_n(list, eli_make_vec2(min.x + r, max.y - r), r, ELI_PI * 0.5f, ELI_PI, arc_segments);
    } else {
        eli_draw_list_path_line_to(list, eli_make_vec2(min.x, max.y));
    }
}

/*============================================================================
 * DRAW LIST CHANNELS
 *===========================================================================*/

/* Channel for draw list splitting */
typedef struct eli_draw_channel {
    eli_draw_cmd_array _cmd_buffer;
    eli_draw_idx_array _idx_buffer;
} eli_draw_channel;

ELI_VECTOR_DEFINE(eli_draw_channel, eli_draw_channel_array);

/* Draw list splitter for channel-based rendering */
typedef struct eli_draw_list_splitter {
    int _current;           /* Current channel index */
    int _count;             /* Number of channels */
    eli_draw_channel_array _channels;
} eli_draw_list_splitter;

/* Initialize splitter */
static inline void eli_draw_list_splitter_init(eli_draw_list_splitter* splitter) {
    splitter->_current = 0;
    splitter->_count = 0;
    eli_vector_init(&splitter->_channels);
}

/* Clear splitter */
static inline void eli_draw_list_splitter_clear(eli_draw_list_splitter* splitter) {
    splitter->_current = 0;
    splitter->_count = 0;
    for (int i = 0; i < splitter->_channels.size; i++) {
        eli_vector_free(&splitter->_channels.data[i]._cmd_buffer);
        eli_vector_free(&splitter->_channels.data[i]._idx_buffer);
    }
    eli_vector_clear(&splitter->_channels);
}

/* Destroy splitter */
static inline void eli_draw_list_splitter_destroy(eli_draw_list_splitter* splitter) {
    eli_draw_list_splitter_clear(splitter);
    eli_vector_free(&splitter->_channels);
}

/* Split draw list into channels */
static inline void eli_draw_list_splitter_split(eli_draw_list_splitter* splitter, eli_draw_list* list, int count) {
    if (splitter->_count > 0) {
        eli_draw_list_splitter_clear(splitter);
    }

    splitter->_count = count;
    splitter->_current = 0;

    /* Allocate channels */
    eli_vector_resize(&splitter->_channels, count, eli_draw_channel);
    for (int i = 0; i < count; i++) {
        eli_vector_init(&splitter->_channels.data[i]._cmd_buffer);
        eli_vector_init(&splitter->_channels.data[i]._idx_buffer);
    }

    /* Move current command to channel 0 */
    if (list->cmd_buffer.size > 0) {
        eli_draw_cmd cmd = eli_vector_back(&list->cmd_buffer);
        eli_vector_push(&splitter->_channels.data[0]._cmd_buffer, cmd, eli_draw_cmd);
        list->cmd_buffer.data[list->cmd_buffer.size - 1].elem_count = 0;
    }
}

/* Set current channel */
static inline void eli_draw_list_splitter_set_current_channel(eli_draw_list_splitter* splitter, eli_draw_list* list, int channel_idx) {
    if (channel_idx < 0 || channel_idx >= splitter->_count) return;
    if (splitter->_current == channel_idx) return;

    /* Save current channel state */
    eli_draw_channel* cur_ch = &splitter->_channels.data[splitter->_current];

    /* Save command */
    if (list->cmd_buffer.size > 0 && cur_ch->_cmd_buffer.size > 0) {
        cur_ch->_cmd_buffer.data[cur_ch->_cmd_buffer.size - 1] = list->cmd_buffer.data[list->cmd_buffer.size - 1];
    }

    /* Switch to new channel */
    splitter->_current = channel_idx;
    eli_draw_channel* new_ch = &splitter->_channels.data[channel_idx];

    /* Restore from new channel */
    list->_idx_write_ptr = list->idx_buffer.data + list->idx_buffer.size;
    if (new_ch->_cmd_buffer.size > 0 && list->cmd_buffer.size > 0) {
        list->cmd_buffer.data[list->cmd_buffer.size - 1] = new_ch->_cmd_buffer.data[new_ch->_cmd_buffer.size - 1];
    }
}

/* Merge all channels back into the draw list */
static inline void eli_draw_list_splitter_merge(eli_draw_list_splitter* splitter, eli_draw_list* list) {
    if (splitter->_count <= 0) return;

    /* Set back to channel 0 */
    eli_draw_list_splitter_set_current_channel(splitter, list, 0);

    /* Calculate total sizes */
    int total_cmd_count = 0;
    int total_idx_count = 0;
    for (int i = 0; i < splitter->_count; i++) {
        total_cmd_count += splitter->_channels.data[i]._cmd_buffer.size;
        total_idx_count += splitter->_channels.data[i]._idx_buffer.size;
    }

    /* Reserve space */
    int cmd_start = list->cmd_buffer.size;
    int idx_start = list->idx_buffer.size;

    eli_vector_reserve(&list->cmd_buffer, cmd_start + total_cmd_count, eli_draw_cmd);
    eli_vector_reserve(&list->idx_buffer, idx_start + total_idx_count, eli_draw_idx);

    /* Merge channels in order */
    for (int i = 0; i < splitter->_count; i++) {
        eli_draw_channel* ch = &splitter->_channels.data[i];

        if (i == 0 && ch->_cmd_buffer.size > 0) {
            /* Channel 0: restore last command to draw list */
            list->cmd_buffer.data[list->cmd_buffer.size - 1] = ch->_cmd_buffer.data[ch->_cmd_buffer.size - 1];
        }

        /* Copy commands and indices */
        for (int cmd_i = (i == 0 ? 0 : 0); cmd_i < ch->_cmd_buffer.size; cmd_i++) {
            eli_draw_cmd* cmd = &ch->_cmd_buffer.data[cmd_i];
            cmd->idx_offset += idx_start;
            if (i > 0 || cmd_i > 0) {
                eli_vector_push(&list->cmd_buffer, *cmd, eli_draw_cmd);
            }
        }

        for (int idx_i = 0; idx_i < ch->_idx_buffer.size; idx_i++) {
            eli_vector_push(&list->idx_buffer, ch->_idx_buffer.data[idx_i], eli_draw_idx);
        }

        idx_start += ch->_idx_buffer.size;
    }

    eli_draw_list_splitter_clear(splitter);
}

/* Convenience functions on draw_list (simplified, requires external splitter storage) */
static inline void eli_draw_list_channels_split(eli_draw_list* list, eli_draw_list_splitter* splitter, int count) {
    eli_draw_list_splitter_split(splitter, list, count);
}

static inline void eli_draw_list_channels_merge(eli_draw_list* list, eli_draw_list_splitter* splitter) {
    eli_draw_list_splitter_merge(splitter, list);
}

static inline void eli_draw_list_channels_set_current(eli_draw_list* list, eli_draw_list_splitter* splitter, int n) {
    eli_draw_list_splitter_set_current_channel(splitter, list, n);
}

/*============================================================================
 * ADVANCED DRAW LIST FUNCTIONS
 *===========================================================================*/

/* Add a user callback command */
static inline void eli_draw_list_add_callback(eli_draw_list* list, eli_draw_callback callback, void* callback_data) {
    /* Close current command if it has elements */
    if (list->cmd_buffer.size > 0 && list->cmd_buffer.data[list->cmd_buffer.size - 1].elem_count > 0) {
        eli_draw_list_add_draw_cmd(list);
    }

    eli_draw_cmd cmd;
    cmd.clip_rect = list->_cmd_header.clip_rect;
    cmd.texture_id = list->_cmd_header.texture_id;
    cmd.vtx_offset = list->_cmd_header.vtx_offset;
    cmd.idx_offset = list->idx_buffer.size;
    cmd.elem_count = 0;
    cmd.user_callback = callback;
    cmd.user_callback_data = callback_data;
    eli_vector_push(&list->cmd_buffer, cmd, eli_draw_cmd);

    /* Add another command for subsequent drawing */
    eli_draw_list_add_draw_cmd(list);
}

/* Clone the output of a draw list */
static inline void eli_draw_list_clone_output(eli_draw_list* src, eli_draw_list* dst) {
    /* Clone command buffer */
    eli_vector_resize(&dst->cmd_buffer, src->cmd_buffer.size, eli_draw_cmd);
    for (int i = 0; i < src->cmd_buffer.size; i++) {
        dst->cmd_buffer.data[i] = src->cmd_buffer.data[i];
    }

    /* Clone vertex buffer */
    eli_vector_resize(&dst->vtx_buffer, src->vtx_buffer.size, eli_draw_vert);
    for (int i = 0; i < src->vtx_buffer.size; i++) {
        dst->vtx_buffer.data[i] = src->vtx_buffer.data[i];
    }

    /* Clone index buffer */
    eli_vector_resize(&dst->idx_buffer, src->idx_buffer.size, eli_draw_idx);
    for (int i = 0; i < src->idx_buffer.size; i++) {
        dst->idx_buffer.data[i] = src->idx_buffer.data[i];
    }

    /* Copy state */
    dst->flags = src->flags;
    dst->_cmd_header = src->_cmd_header;
    dst->_vtx_current_idx = src->_vtx_current_idx;
    dst->_fringe_scale = src->_fringe_scale;
    dst->_tex_uv_white_pixel = src->_tex_uv_white_pixel;
    dst->_owner_name = src->_owner_name;
}

/*============================================================================
 * DRAW DATA FUNCTIONS
 *===========================================================================*/

static inline void eli_draw_data_init(eli_draw_data* data) {
    data->valid = false;
    data->cmd_lists_count = 0;
    data->total_idx_count = 0;
    data->total_vtx_count = 0;
    data->cmd_lists.data = NULL;
    data->cmd_lists.size = 0;
    data->cmd_lists.capacity = 0;
    data->display_pos = eli_make_vec2(0, 0);
    data->display_size = eli_make_vec2(0, 0);
    data->framebuffer_scale = eli_make_vec2(1, 1);
}

static inline void eli_draw_data_clear(eli_draw_data* data) {
    data->valid = false;
    data->cmd_lists_count = 0;
    data->total_idx_count = 0;
    data->total_vtx_count = 0;
    data->cmd_lists.size = 0;
}

#ifdef __cplusplus
}
#endif

#endif /* ELI_DRAW_H */
