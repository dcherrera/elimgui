/**
 * @file eli_draw_list.h
 * @brief Draw-list lifecycle and the low-level command/geometry machinery:
 *        buffer growth, command batching, the clip-rect and texture-id stacks,
 *        and the primitive-reservation (prim_*) writing API.
 *
 * This is the foundation the shape/path helpers build on. Everything here is
 * renderer-agnostic and allocation-explicit: buffers grow geometrically via the
 * libc seam and are released by eli_draw_list_clear.
 *
 * @status Phase 2 draw-list core in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_DRAW_ELI_DRAW_LIST_H
#define ELI_DRAW_ELI_DRAW_LIST_H

#include "eli_draw_types.h"

#include "../core/eli_platform.h"
#include "../core/eli_types.h"

/** Half-extent of the default fullscreen clip rectangle, in pixels. */
#define ELI_DRAW_CLIP_FULLSCREEN_EXTENT 8192.0f

/** Default circle tessellation error used for automatic segment counts. */
#define ELI_DRAW_CIRCLE_SEG_MAX_ERROR 0.30f

/* ---------------------------------------------------------------------------
 * Buffer growth
 * ------------------------------------------------------------------------- */

/**
 * Grow a heap buffer so it can hold at least `need` elements, using ~1.5x
 * geometric growth. Returns the (possibly moved) buffer and writes the new
 * capacity through `capacity`. A no-op when the buffer is already large enough.
 *
 * @param data      Current buffer (may be NULL).
 * @param capacity  In/out element capacity.
 * @param need      Minimum required element count.
 * @param elem_size Size of one element in bytes.
 * @return          Buffer pointer with capacity >= need.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void *eli_draw_buf_grow(void *data, int *capacity, int need, size_t elem_size)
{
    if (*capacity >= need)
        return data;

    int cap = (*capacity > 0) ? *capacity : 8;
    while (cap < need)
        cap += cap / 2 + 1;

    void *grown = realloc(data, (size_t)cap * elem_size);
    if (grown != NULL)
        *capacity = cap;
    return (grown != NULL) ? grown : data;
}

/* ---------------------------------------------------------------------------
 * Clip rect helpers
 * ------------------------------------------------------------------------- */

/** Convert an internal (min.x,min.y,max.x,max.y) clip vec4 to an origin+size rect. */
static inline eli_rect eli_draw_clip_vec4_to_rect(eli_vec4 c)
{
    return eli_make_rect(c.x, c.y, c.z - c.x, c.w - c.y);
}

/** True when two internal clip vec4s are bit-for-bit equal. */
static inline bool eli_draw_clip_equal(eli_vec4 a, eli_vec4 b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

/* ---------------------------------------------------------------------------
 * Command management
 * ------------------------------------------------------------------------- */

/**
 * Append a fresh, empty draw command that inherits the current clip rect and
 * texture id from the list header. New geometry accumulates into it.
 *
 * @param list  Target draw list.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_add_draw_cmd(eli_draw_list *list)
{
    int cap = (int)list->cmd_capacity;
    list->cmds = (eli_draw_cmd *)eli_draw_buf_grow(list->cmds, &cap,
                                                   (int)list->cmd_count + 1, sizeof(*list->cmds));
    list->cmd_capacity = (uint32_t)cap;
    eli_draw_cmd *cmd = &list->cmds[list->cmd_count++];
    cmd->clip_rect = eli_draw_clip_vec4_to_rect(list->cmd_clip_rect);
    cmd->texture_id = list->cmd_texture_id;
    cmd->vtx_offset = 0;
    cmd->idx_offset = list->idx_count;
    cmd->elem_count = 0;
    cmd->user_callback = NULL;
    cmd->user_callback_data = NULL;
}

/** Drop the trailing command if it is empty and carries no callback. */
static inline void eli_draw_list_pop_unused_draw_cmd(eli_draw_list *list)
{
    if (list->cmd_count == 0)
        return;
    eli_draw_cmd *cmd = &list->cmds[list->cmd_count - 1];
    if (cmd->elem_count == 0 && cmd->user_callback == NULL)
        list->cmd_count--;
}

/** React to a clip-rect change: split the current command or retag/merge it. */
static inline void eli_draw_list_on_changed_clip_rect(eli_draw_list *list)
{
    eli_draw_cmd *curr = &list->cmds[list->cmd_count - 1];
    eli_vec4 curr_clip = {curr->clip_rect.x, curr->clip_rect.y,
                          curr->clip_rect.x + curr->clip_rect.w,
                          curr->clip_rect.y + curr->clip_rect.h};
    if (curr->elem_count != 0 && !eli_draw_clip_equal(curr_clip, list->cmd_clip_rect)) {
        eli_draw_list_add_draw_cmd(list);
        return;
    }
    if (curr->elem_count == 0 && list->cmd_count > 1) {
        eli_draw_cmd *prev = &list->cmds[list->cmd_count - 2];
        eli_vec4 prev_clip = {prev->clip_rect.x, prev->clip_rect.y,
                              prev->clip_rect.x + prev->clip_rect.w,
                              prev->clip_rect.y + prev->clip_rect.h};
        if (eli_draw_clip_equal(prev_clip, list->cmd_clip_rect) &&
            prev->texture_id == list->cmd_texture_id && prev->user_callback == NULL &&
            prev->idx_offset + prev->elem_count == curr->idx_offset) {
            list->cmd_count--;
            return;
        }
    }
    curr->clip_rect = eli_draw_clip_vec4_to_rect(list->cmd_clip_rect);
}

/** React to a texture change: split the current command or retag/merge it. */
static inline void eli_draw_list_on_changed_texture(eli_draw_list *list)
{
    eli_draw_cmd *curr = &list->cmds[list->cmd_count - 1];
    if (curr->elem_count != 0 && curr->texture_id != list->cmd_texture_id) {
        eli_draw_list_add_draw_cmd(list);
        return;
    }
    if (curr->elem_count == 0 && list->cmd_count > 1) {
        eli_draw_cmd *prev = &list->cmds[list->cmd_count - 2];
        if (prev->texture_id == list->cmd_texture_id && prev->user_callback == NULL &&
            prev->idx_offset + prev->elem_count == curr->idx_offset) {
            list->cmd_count--;
            return;
        }
    }
    curr->texture_id = list->cmd_texture_id;
}

/* ---------------------------------------------------------------------------
 * Lifecycle
 * ------------------------------------------------------------------------- */

/**
 * Initialize a draw list in place: zero all buffers, set the fullscreen clip,
 * and open an initial empty command. Call eli_draw_list_clear to release it.
 *
 * @param list   Draw list storage to initialize (must be non-NULL).
 * @param flags  Construction flags (see eli_draw_list_flags).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_init(eli_draw_list *list, eli_draw_list_flags flags)
{
    memset(list, 0, sizeof(*list));
    list->flags = flags;
    list->fringe_scale = 1.0f;
    list->circle_segment_max_error = ELI_DRAW_CIRCLE_SEG_MAX_ERROR;
    list->tex_uv_white_pixel = eli_make_vec2(0.0f, 0.0f);
    const float e = ELI_DRAW_CLIP_FULLSCREEN_EXTENT;
    list->clip_rect_fullscreen = eli_make_vec4(-e, -e, e, e);
    list->cmd_clip_rect = list->clip_rect_fullscreen;
    list->cmd_texture_id = 0;
    eli_draw_list_add_draw_cmd(list);
}

/**
 * Release all heap buffers owned by a draw list (commands, geometry, path, clip
 * and texture stacks, channels) and zero it. Safe to call on a zeroed list.
 *
 * @param list  Draw list to clear.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_clear(eli_draw_list *list)
{
    free(list->cmds);
    free(list->vtx);
    free(list->idx);
    free(list->path);
    free(list->clip_rect_stack);
    free(list->texture_stack);
    /* The current channel's buffers alias the live cmds/idx freed above; every
     * other channel owns distinct buffers that must be released here. */
    for (int i = 0; i < list->channels_capacity; i++) {
        if (i == list->channels_current)
            continue;
        free(list->channels[i].cmds);
        free(list->channels[i].idx);
    }
    free(list->channels);
    memset(list, 0, sizeof(*list));
}

/**
 * Reset a draw list for reuse without freeing its buffers: clears geometry and
 * commands, resets the path/clip/texture stacks, and opens a fresh command.
 *
 * @param list  Draw list to reset (must have been initialized).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_reset(eli_draw_list *list)
{
    list->cmd_count = 0;
    list->vtx_count = 0;
    list->idx_count = 0;
    list->path_count = 0;
    list->clip_rect_stack_count = 0;
    list->texture_stack_count = 0;
    list->channels_count = 0;
    list->channels_current = 0;
    list->vtx_current_idx = 0;
    list->cmd_clip_rect = list->clip_rect_fullscreen;
    list->cmd_texture_id = 0;
    eli_draw_list_add_draw_cmd(list);
}

/* ---------------------------------------------------------------------------
 * Clip rect stack
 * ------------------------------------------------------------------------- */

/**
 * Push a clip rectangle. When `intersect_with_current` is true the new rect is
 * intersected with the active one. Scissors subsequent geometry.
 *
 * @param list                    Target draw list.
 * @param clip_min                Top-left corner of the clip rect.
 * @param clip_max                Bottom-right corner of the clip rect.
 * @param intersect_with_current  Intersect with the current clip when true.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_push_clip_rect(eli_draw_list *list, eli_vec2 clip_min,
                                                eli_vec2 clip_max, bool intersect_with_current)
{
    eli_vec4 cr = {clip_min.x, clip_min.y, clip_max.x, clip_max.y};
    if (intersect_with_current) {
        eli_vec4 cur = list->cmd_clip_rect;
        if (cr.x < cur.x) cr.x = cur.x;
        if (cr.y < cur.y) cr.y = cur.y;
        if (cr.z > cur.z) cr.z = cur.z;
        if (cr.w > cur.w) cr.w = cur.w;
    }
    cr.z = eli_max_f(cr.x, cr.z);
    cr.w = eli_max_f(cr.y, cr.w);

    list->clip_rect_stack = (eli_vec4 *)eli_draw_buf_grow(
        list->clip_rect_stack, &list->clip_rect_stack_capacity,
        list->clip_rect_stack_count + 1, sizeof(*list->clip_rect_stack));
    list->clip_rect_stack[list->clip_rect_stack_count++] = cr;
    list->cmd_clip_rect = cr;
    eli_draw_list_on_changed_clip_rect(list);
}

/**
 * Push the list's fullscreen clip rectangle (disables scissoring).
 *
 * @param list  Target draw list.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_push_clip_rect_full_screen(eli_draw_list *list)
{
    eli_vec4 f = list->clip_rect_fullscreen;
    eli_draw_list_push_clip_rect(list, eli_make_vec2(f.x, f.y), eli_make_vec2(f.z, f.w), false);
}

/**
 * Pop the top clip rectangle, restoring the previous one (or fullscreen).
 *
 * @param list  Target draw list.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_pop_clip_rect(eli_draw_list *list)
{
    if (list->clip_rect_stack_count > 0)
        list->clip_rect_stack_count--;
    list->cmd_clip_rect = (list->clip_rect_stack_count == 0)
                              ? list->clip_rect_fullscreen
                              : list->clip_rect_stack[list->clip_rect_stack_count - 1];
    eli_draw_list_on_changed_clip_rect(list);
}

/** @return Top-left corner of the current clip rectangle. */
static inline eli_vec2 eli_draw_list_get_clip_rect_min(const eli_draw_list *list)
{
    return eli_make_vec2(list->cmd_clip_rect.x, list->cmd_clip_rect.y);
}

/** @return Bottom-right corner of the current clip rectangle. */
static inline eli_vec2 eli_draw_list_get_clip_rect_max(const eli_draw_list *list)
{
    return eli_make_vec2(list->cmd_clip_rect.z, list->cmd_clip_rect.w);
}

/* ---------------------------------------------------------------------------
 * Texture id stack
 * ------------------------------------------------------------------------- */

/**
 * Push a texture id; subsequent geometry is tagged for that texture.
 *
 * @param list        Target draw list.
 * @param texture_id  Backend texture identifier.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_push_texture_id(eli_draw_list *list, uint32_t texture_id)
{
    list->texture_stack = (uint32_t *)eli_draw_buf_grow(
        list->texture_stack, &list->texture_stack_capacity,
        list->texture_stack_count + 1, sizeof(*list->texture_stack));
    list->texture_stack[list->texture_stack_count++] = texture_id;
    list->cmd_texture_id = texture_id;
    eli_draw_list_on_changed_texture(list);
}

/**
 * Pop the top texture id, restoring the previous one (or 0).
 *
 * @param list  Target draw list.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_pop_texture_id(eli_draw_list *list)
{
    if (list->texture_stack_count > 0)
        list->texture_stack_count--;
    list->cmd_texture_id = (list->texture_stack_count == 0)
                               ? 0u
                               : list->texture_stack[list->texture_stack_count - 1];
    eli_draw_list_on_changed_texture(list);
}

/* ---------------------------------------------------------------------------
 * Primitive reservation and writing
 * ------------------------------------------------------------------------- */

/**
 * Grow the vertex and index buffers so they can hold `vtx_count` more vertices
 * and `idx_count` more indices beyond the current counts, WITHOUT charging the
 * indices to any command. Use this to batch a single up-front reservation for a
 * run of primitives (e.g. a whole text string) whose exact index total is only
 * known after emission; charge the actual total afterward with
 * eli_draw_list_prim_add_idx_to_cmd. Growth is geometric (amortized O(1)).
 *
 * @param list       Target draw list.
 * @param idx_count  Number of additional indices to make room for.
 * @param vtx_count  Number of additional vertices to make room for.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_prim_reserve_capacity(eli_draw_list *list, int idx_count,
                                                       int vtx_count)
{
    int vtx_cap = (int)list->vtx_capacity;
    list->vtx = (eli_draw_vert *)eli_draw_buf_grow(list->vtx, &vtx_cap,
                                                   (int)list->vtx_count + vtx_count,
                                                   sizeof(*list->vtx));
    list->vtx_capacity = (uint32_t)vtx_cap;

    int idx_cap = (int)list->idx_capacity;
    list->idx = (eli_draw_idx *)eli_draw_buf_grow(list->idx, &idx_cap,
                                                  (int)list->idx_count + idx_count,
                                                  sizeof(*list->idx));
    list->idx_capacity = (uint32_t)idx_cap;
}

/**
 * Charge `idx_count` indices to the current command's element count without
 * touching the buffers. Pairs with eli_draw_list_prim_reserve_capacity when the
 * index total is tallied after emission.
 *
 * @param list       Target draw list.
 * @param idx_count  Number of indices to add to the current command.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_prim_add_idx_to_cmd(eli_draw_list *list, int idx_count)
{
    list->cmds[list->cmd_count - 1].elem_count += (uint32_t)idx_count;
}

/**
 * Reserve capacity for `vtx_count` vertices and `idx_count` indices and charge
 * the indices to the current command. Fill the reserved slots with the prim_*
 * writers before reserving again.
 *
 * @param list       Target draw list.
 * @param idx_count  Number of indices to reserve.
 * @param vtx_count  Number of vertices to reserve.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_prim_reserve(eli_draw_list *list, int idx_count, int vtx_count)
{
    eli_draw_list_prim_reserve_capacity(list, idx_count, vtx_count);
    eli_draw_list_prim_add_idx_to_cmd(list, idx_count);
}

/**
 * Release the last-reserved vertices/indices from the end of the buffers and
 * uncharge them from the current command.
 *
 * @param list       Target draw list.
 * @param idx_count  Number of indices to release.
 * @param vtx_count  Number of vertices to release.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_prim_unreserve(eli_draw_list *list, int idx_count, int vtx_count)
{
    list->cmds[list->cmd_count - 1].elem_count -= (uint32_t)idx_count;
    list->vtx_count -= (uint32_t)vtx_count;
    list->idx_count -= (uint32_t)idx_count;
}

/** Write one vertex into the reserved region and advance the running index. */
static inline void eli_draw_list_prim_write_vtx(eli_draw_list *list, eli_vec2 pos, eli_vec2 uv,
                                                eli_col32 col)
{
    eli_draw_vert *v = &list->vtx[list->vtx_count++];
    v->x = pos.x;
    v->y = pos.y;
    v->u = uv.x;
    v->v = uv.y;
    v->col = col;
    list->vtx_current_idx++;
}

/** Write one index into the reserved region. */
static inline void eli_draw_list_prim_write_idx(eli_draw_list *list, eli_draw_idx idx)
{
    list->idx[list->idx_count++] = idx;
}

/** Emit an index referencing the next vertex, then write that vertex. */
static inline void eli_draw_list_prim_vtx(eli_draw_list *list, eli_vec2 pos, eli_vec2 uv,
                                          eli_col32 col)
{
    eli_draw_list_prim_write_idx(list, (eli_draw_idx)list->vtx_current_idx);
    eli_draw_list_prim_write_vtx(list, pos, uv, col);
}

/**
 * Write an axis-aligned filled rectangle (two triangles) into reserved space.
 * `a` is the top-left corner and `c` the bottom-right corner.
 */
static inline void eli_draw_list_prim_rect(eli_draw_list *list, eli_vec2 a, eli_vec2 c,
                                           eli_col32 col)
{
    eli_vec2 b = eli_make_vec2(c.x, a.y);
    eli_vec2 d = eli_make_vec2(a.x, c.y);
    eli_vec2 uv = list->tex_uv_white_pixel;
    eli_draw_idx idx = (eli_draw_idx)list->vtx_current_idx;
    list->idx[list->idx_count + 0] = idx;
    list->idx[list->idx_count + 1] = (eli_draw_idx)(idx + 1);
    list->idx[list->idx_count + 2] = (eli_draw_idx)(idx + 2);
    list->idx[list->idx_count + 3] = idx;
    list->idx[list->idx_count + 4] = (eli_draw_idx)(idx + 2);
    list->idx[list->idx_count + 5] = (eli_draw_idx)(idx + 3);
    list->idx_count += 6;
    eli_draw_list_prim_write_vtx(list, a, uv, col);
    eli_draw_list_prim_write_vtx(list, b, uv, col);
    eli_draw_list_prim_write_vtx(list, c, uv, col);
    eli_draw_list_prim_write_vtx(list, d, uv, col);
}

/**
 * Write a filled rectangle with explicit UVs. `a`/`c` are opposite corners and
 * `uv_a`/`uv_c` their texture coordinates.
 */
static inline void eli_draw_list_prim_rect_uv(eli_draw_list *list, eli_vec2 a, eli_vec2 c,
                                              eli_vec2 uv_a, eli_vec2 uv_c, eli_col32 col)
{
    eli_vec2 b = eli_make_vec2(c.x, a.y);
    eli_vec2 d = eli_make_vec2(a.x, c.y);
    eli_vec2 uv_b = eli_make_vec2(uv_c.x, uv_a.y);
    eli_vec2 uv_d = eli_make_vec2(uv_a.x, uv_c.y);
    eli_draw_idx idx = (eli_draw_idx)list->vtx_current_idx;
    list->idx[list->idx_count + 0] = idx;
    list->idx[list->idx_count + 1] = (eli_draw_idx)(idx + 1);
    list->idx[list->idx_count + 2] = (eli_draw_idx)(idx + 2);
    list->idx[list->idx_count + 3] = idx;
    list->idx[list->idx_count + 4] = (eli_draw_idx)(idx + 2);
    list->idx[list->idx_count + 5] = (eli_draw_idx)(idx + 3);
    list->idx_count += 6;
    eli_draw_list_prim_write_vtx(list, a, uv_a, col);
    eli_draw_list_prim_write_vtx(list, b, uv_b, col);
    eli_draw_list_prim_write_vtx(list, c, uv_c, col);
    eli_draw_list_prim_write_vtx(list, d, uv_d, col);
}

/**
 * Write a filled quad with explicit per-corner UVs. Corners a,b,c,d wind in
 * order; two triangles (a,b,c) and (a,c,d) are emitted.
 */
static inline void eli_draw_list_prim_quad_uv(eli_draw_list *list, eli_vec2 a, eli_vec2 b,
                                              eli_vec2 c, eli_vec2 d, eli_vec2 uv_a, eli_vec2 uv_b,
                                              eli_vec2 uv_c, eli_vec2 uv_d, eli_col32 col)
{
    eli_draw_idx idx = (eli_draw_idx)list->vtx_current_idx;
    list->idx[list->idx_count + 0] = idx;
    list->idx[list->idx_count + 1] = (eli_draw_idx)(idx + 1);
    list->idx[list->idx_count + 2] = (eli_draw_idx)(idx + 2);
    list->idx[list->idx_count + 3] = idx;
    list->idx[list->idx_count + 4] = (eli_draw_idx)(idx + 2);
    list->idx[list->idx_count + 5] = (eli_draw_idx)(idx + 3);
    list->idx_count += 6;
    eli_draw_list_prim_write_vtx(list, a, uv_a, col);
    eli_draw_list_prim_write_vtx(list, b, uv_b, col);
    eli_draw_list_prim_write_vtx(list, c, uv_c, col);
    eli_draw_list_prim_write_vtx(list, d, uv_d, col);
}

/* ---------------------------------------------------------------------------
 * Advanced
 * ------------------------------------------------------------------------- */

/**
 * Attach a user callback to be invoked by the backend in place of drawing, then
 * open a fresh command so following geometry is unaffected.
 *
 * @param list           Target draw list.
 * @param callback       Callback to invoke (must be non-NULL).
 * @param callback_data  Opaque pointer passed through to the callback.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_add_callback(eli_draw_list *list, eli_draw_callback callback,
                                              void *callback_data)
{
    eli_draw_cmd *curr = &list->cmds[list->cmd_count - 1];
    if (curr->elem_count != 0 || curr->user_callback != NULL) {
        eli_draw_list_add_draw_cmd(list);
        curr = &list->cmds[list->cmd_count - 1];
    }
    curr->user_callback = callback;
    curr->user_callback_data = callback_data;
    eli_draw_list_add_draw_cmd(list);
}

/**
 * Deep-copy a draw list's rendered output (commands, vertices, indices) into a
 * freshly allocated draw list. Path, clip, texture, and channel working state
 * are not copied.
 *
 * @param list  Source draw list.
 * @return      New heap-allocated clone (release with eli_draw_list_clear then
 *              free), or NULL on allocation failure.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline eli_draw_list *eli_draw_list_clone_output(const eli_draw_list *list)
{
    eli_draw_list *clone = (eli_draw_list *)calloc(1, sizeof(*clone));
    if (clone == NULL)
        return NULL;

    clone->flags = list->flags;
    clone->fringe_scale = list->fringe_scale;
    clone->circle_segment_max_error = list->circle_segment_max_error;
    clone->tex_uv_white_pixel = list->tex_uv_white_pixel;
    clone->clip_rect_fullscreen = list->clip_rect_fullscreen;
    clone->cmd_clip_rect = list->cmd_clip_rect;
    clone->cmd_texture_id = list->cmd_texture_id;
    clone->vtx_current_idx = list->vtx_current_idx;

    if (list->cmd_count > 0) {
        clone->cmds = (eli_draw_cmd *)malloc((size_t)list->cmd_count * sizeof(*clone->cmds));
        if (clone->cmds != NULL) {
            memcpy(clone->cmds, list->cmds, (size_t)list->cmd_count * sizeof(*clone->cmds));
            clone->cmd_count = list->cmd_count;
            clone->cmd_capacity = (int)list->cmd_count;
        }
    }
    if (list->vtx_count > 0) {
        clone->vtx = (eli_draw_vert *)malloc((size_t)list->vtx_count * sizeof(*clone->vtx));
        if (clone->vtx != NULL) {
            memcpy(clone->vtx, list->vtx, (size_t)list->vtx_count * sizeof(*clone->vtx));
            clone->vtx_count = list->vtx_count;
            clone->vtx_capacity = (int)list->vtx_count;
        }
    }
    if (list->idx_count > 0) {
        clone->idx = (eli_draw_idx *)malloc((size_t)list->idx_count * sizeof(*clone->idx));
        if (clone->idx != NULL) {
            memcpy(clone->idx, list->idx, (size_t)list->idx_count * sizeof(*clone->idx));
            clone->idx_count = list->idx_count;
            clone->idx_capacity = (int)list->idx_count;
        }
    }
    return clone;
}

#endif /* ELI_DRAW_ELI_DRAW_LIST_H */
