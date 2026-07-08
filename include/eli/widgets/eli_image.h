/**
 * @file eli_image.h
 * @brief Phase 22 image display: draw-list image helpers (add_image,
 *        add_image_quad, add_image_rounded) and the image widget and image-button
 *        widget (eli_image, eli_image_button).
 *
 * All three draw-list helpers push/pop the texture-id stack so a dedicated draw
 * command is emitted whenever the texture changes. The widget functions integrate
 * with the layout cursor (item_size/item_add) and, for eli_image_button, with
 * button_behavior for hover/active/press semantics.
 *
 * @status Phase 22 images complete.
 * @issues None
 * @todo None
 */
#ifndef ELI_WIDGETS_ELI_IMAGE_H
#define ELI_WIDGETS_ELI_IMAGE_H

#include "eli_widget_behavior.h"

#include "../core/eli_platform.h"

/* ---------------------------------------------------------------------------
 * Alpha mask constant (if not already defined by draw types)
 * ------------------------------------------------------------------------- */

#ifndef ELI_IMAGE_COL32_A_MASK
#define ELI_IMAGE_COL32_A_MASK 0xFF000000u
#endif

/* ---------------------------------------------------------------------------
 * Internal UV shading helper
 * ------------------------------------------------------------------------- */

/**
 * Remap the UV coordinates of vertices [vert_start, list->vtx_count) so they
 * map linearly from uv_min at p_min to uv_max at p_max. Used by
 * eli_draw_list_add_image_rounded to UV-shade a path-built rounded rect.
 * UV values are clamped to [uv_min, uv_max].
 *
 * @param list        Target draw list (vertices are modified in place).
 * @param vert_start  First vertex index to shade.
 * @param p_min       Screen-space top-left corner of the image.
 * @param p_max       Screen-space bottom-right corner of the image.
 * @param uv_min      UV coordinate at p_min.
 * @param uv_max      UV coordinate at p_max.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_image__shade_verts_linear_uv(eli_draw_list *list, uint32_t vert_start,
                                                     eli_vec2 p_min, eli_vec2 p_max,
                                                     eli_vec2 uv_min, eli_vec2 uv_max)
{
    float rng_x = p_max.x - p_min.x;
    float rng_y = p_max.y - p_min.y;
    float scale_x = (rng_x != 0.0f) ? ((uv_max.x - uv_min.x) / rng_x) : 0.0f;
    float scale_y = (rng_y != 0.0f) ? ((uv_max.y - uv_min.y) / rng_y) : 0.0f;

    float uv_lo_x = (uv_min.x < uv_max.x) ? uv_min.x : uv_max.x;
    float uv_hi_x = (uv_min.x < uv_max.x) ? uv_max.x : uv_min.x;
    float uv_lo_y = (uv_min.y < uv_max.y) ? uv_min.y : uv_max.y;
    float uv_hi_y = (uv_min.y < uv_max.y) ? uv_max.y : uv_min.y;

    for (uint32_t i = vert_start; i < list->vtx_count; i++) {
        float u = uv_min.x + (list->vtx[i].x - p_min.x) * scale_x;
        float v = uv_min.y + (list->vtx[i].y - p_min.y) * scale_y;
        if (u < uv_lo_x) u = uv_lo_x;
        if (u > uv_hi_x) u = uv_hi_x;
        if (v < uv_lo_y) v = uv_lo_y;
        if (v > uv_hi_y) v = uv_hi_y;
        list->vtx[i].u = u;
        list->vtx[i].v = v;
    }
}

/* ---------------------------------------------------------------------------
 * Draw-list image helpers
 * ------------------------------------------------------------------------- */

/**
 * Draw a textured axis-aligned rectangle into a draw list. Pushes the texture
 * id onto the stack (so a new draw command is emitted for a texture change) and
 * pops it after reserving the geometry.
 *
 * @param list             Target draw list.
 * @param user_texture_id  Backend texture handle.
 * @param p_min            Top-left screen position.
 * @param p_max            Bottom-right screen position.
 * @param uv_min           UV at p_min.
 * @param uv_max           UV at p_max.
 * @param col              Tint color (ELI_COL32). Pass ELI_COL32_WHITE for no tint.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_add_image(eli_draw_list *list, uint32_t user_texture_id,
                                           eli_vec2 p_min, eli_vec2 p_max,
                                           eli_vec2 uv_min, eli_vec2 uv_max, eli_col32 col)
{
    if ((col & ELI_IMAGE_COL32_A_MASK) == 0)
        return;

    bool push_tex = (list->cmd_texture_id != user_texture_id);
    if (push_tex)
        eli_draw_list_push_texture_id(list, user_texture_id);

    eli_draw_list_prim_reserve(list, 6, 4);
    eli_draw_list_prim_rect_uv(list, p_min, p_max, uv_min, uv_max, col);

    if (push_tex)
        eli_draw_list_pop_texture_id(list);
}

/**
 * Draw a textured quad with explicit per-corner positions and UV coordinates.
 * The quad is split into two triangles: (p1,p2,p3) and (p1,p3,p4).
 *
 * @param list             Target draw list.
 * @param user_texture_id  Backend texture handle.
 * @param p1               Top-left corner.
 * @param p2               Top-right corner.
 * @param p3               Bottom-right corner.
 * @param p4               Bottom-left corner.
 * @param uv1              UV at p1.
 * @param uv2              UV at p2.
 * @param uv3              UV at p3.
 * @param uv4              UV at p4.
 * @param col              Tint color (ELI_COL32).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_add_image_quad(eli_draw_list *list, uint32_t user_texture_id,
                                                eli_vec2 p1, eli_vec2 p2,
                                                eli_vec2 p3, eli_vec2 p4,
                                                eli_vec2 uv1, eli_vec2 uv2,
                                                eli_vec2 uv3, eli_vec2 uv4,
                                                eli_col32 col)
{
    if ((col & ELI_IMAGE_COL32_A_MASK) == 0)
        return;

    bool push_tex = (list->cmd_texture_id != user_texture_id);
    if (push_tex)
        eli_draw_list_push_texture_id(list, user_texture_id);

    eli_draw_list_prim_reserve(list, 6, 4);
    eli_draw_list_prim_quad_uv(list, p1, p2, p3, p4, uv1, uv2, uv3, uv4, col);

    if (push_tex)
        eli_draw_list_pop_texture_id(list);
}

/**
 * Draw a textured rectangle with rounded corners. The shape is built via the
 * path API and the UV coordinates of each generated vertex are remapped
 * bilinearly from the bounding rect corners. Falls back to a plain rect when
 * the rounding is below 0.5 or no corners are selected for rounding.
 *
 * @param list             Target draw list.
 * @param user_texture_id  Backend texture handle.
 * @param p_min            Top-left screen position.
 * @param p_max            Bottom-right screen position.
 * @param uv_min           UV at p_min.
 * @param uv_max           UV at p_max.
 * @param col              Tint color (ELI_COL32).
 * @param rounding         Corner radius in pixels.
 * @param flags            Corner-selection flags (eli_draw_flags round-corner bits).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_add_image_rounded(eli_draw_list *list, uint32_t user_texture_id,
                                                   eli_vec2 p_min, eli_vec2 p_max,
                                                   eli_vec2 uv_min, eli_vec2 uv_max,
                                                   eli_col32 col, float rounding,
                                                   eli_draw_flags flags)
{
    if ((col & ELI_IMAGE_COL32_A_MASK) == 0)
        return;

    if (rounding < 0.5f || (flags & ELI_DRAW_ROUND_CORNERS_MASK) == ELI_DRAW_ROUND_CORNERS_NONE) {
        eli_draw_list_add_image(list, user_texture_id, p_min, p_max, uv_min, uv_max, col);
        return;
    }

    bool push_tex = (list->cmd_texture_id != user_texture_id);
    if (push_tex)
        eli_draw_list_push_texture_id(list, user_texture_id);

    /* Record the vertex cursor before generating the rounded fill so we can
     * shade only the new vertices afterward. */
    uint32_t vert_start = list->vtx_count;
    eli_draw_list_path_rect(list, p_min, p_max, rounding, flags);
    eli_draw_list_path_fill_convex(list, col);

    /* Remap the UVs of the newly generated vertices from their screen positions. */
    eli_image__shade_verts_linear_uv(list, vert_start, p_min, p_max, uv_min, uv_max);

    if (push_tex)
        eli_draw_list_pop_texture_id(list);
}

/* ---------------------------------------------------------------------------
 * Image widget
 * ------------------------------------------------------------------------- */

/**
 * Display a textured image at the current cursor position. Advances the layout
 * cursor by image_size (plus a 1-pixel border inset when border_col is visible).
 * The texture is drawn with uv0/uv1 mapping and a per-pixel tint_col multiply.
 *
 * @param user_texture_id  Backend texture handle.
 * @param image_size       Display size in pixels.
 * @param uv0              UV at the top-left corner of the image.
 * @param uv1              UV at the bottom-right corner of the image.
 * @param tint_col         RGBA float tint applied to the texture.
 * @param border_col       RGBA float border color; pass zero-alpha to suppress.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_image(uint32_t user_texture_id, eli_vec2 image_size,
                              eli_vec2 uv0, eli_vec2 uv1,
                              eli_vec4 tint_col, eli_vec4 border_col)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return;

    eli_draw_list *dl = eli_get_window_draw_list();
    if (dl == NULL)
        return;

    float border_size = (border_col.w > 0.0f) ? 1.0f : 0.0f;
    eli_vec2 pos = ctx->current_window->cursor_pos;
    eli_vec2 total_size = eli_make_vec2(image_size.x + border_size * 2.0f,
                                        image_size.y + border_size * 2.0f);
    eli_rect bb = eli_make_rect(pos.x, pos.y, total_size.x, total_size.y);

    eli_item_size(total_size, 0.0f);
    if (!eli_item_add(0u, bb, 0))
        return;

    if (border_col.w > 0.0f) {
        eli_col32 bc = eli_color_convert_float4_to_u32(border_col);
        eli_draw_list_add_rect(dl, pos, eli_rect_max(bb), bc, 0.0f, ELI_DRAW_NONE, 1.0f);
        eli_vec2 img_min = eli_make_vec2(pos.x + border_size, pos.y + border_size);
        eli_vec2 img_max = eli_make_vec2(img_min.x + image_size.x, img_min.y + image_size.y);
        eli_col32 tc = eli_color_convert_float4_to_u32(tint_col);
        eli_draw_list_add_image(dl, user_texture_id, img_min, img_max, uv0, uv1, tc);
    } else {
        eli_col32 tc = eli_color_convert_float4_to_u32(tint_col);
        eli_draw_list_add_image(dl, user_texture_id, pos, eli_rect_max(bb), uv0, uv1, tc);
    }
}

/* ---------------------------------------------------------------------------
 * Image button widget
 * ------------------------------------------------------------------------- */

/**
 * A clickable image button: the image is padded by style.frame_padding and
 * surrounded by the standard button frame (hover and active color states). The
 * optional bg_col fills the padded area behind the image.
 *
 * @param str_id           Widget string id (must be unique within the window scope).
 * @param user_texture_id  Backend texture handle.
 * @param image_size       Image display size (not including padding).
 * @param uv0              UV at the top-left corner.
 * @param uv1              UV at the bottom-right corner.
 * @param bg_col           RGBA float background fill; zero-alpha to suppress.
 * @param tint_col         RGBA float tint applied to the texture.
 * @return                 true on the frame the button is pressed (left mouse).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline bool eli_image_button(const char *str_id, uint32_t user_texture_id,
                                    eli_vec2 image_size, eli_vec2 uv0, eli_vec2 uv1,
                                    eli_vec4 bg_col, eli_vec4 tint_col)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return false;

    eli_draw_list *dl = eli_get_window_draw_list();
    if (dl == NULL)
        return false;

    const eli_style *style = &ctx->style;
    eli_id id = eli_get_id(str_id);

    eli_vec2 padding = style->frame_padding;
    eli_vec2 pos = ctx->current_window->cursor_pos;
    eli_vec2 total_size = eli_make_vec2(image_size.x + padding.x * 2.0f,
                                        image_size.y + padding.y * 2.0f);
    eli_rect bb = eli_make_rect(pos.x, pos.y, total_size.x, total_size.y);

    eli_item_size(total_size, 0.0f);
    if (!eli_item_add(id, bb, 0))
        return false;

    bool hovered = false, held = false;
    bool pressed = eli_button_behavior(bb, id, &hovered, &held, ELI_BUTTON_NONE);

    /* Button frame (hover / active color). */
    eli_col32 frame_col = eli_get_color_u32(
        (held && hovered) ? ELI_COL_BUTTON_ACTIVE : hovered ? ELI_COL_BUTTON_HOVERED : ELI_COL_BUTTON,
        1.0f);
    float rounding = eli_max_f(padding.x < padding.y ? padding.x : padding.y, 0.0f);
    if (rounding > style->frame_rounding)
        rounding = style->frame_rounding;

    eli_render_nav_highlight(bb, id);
    eli_render_frame(eli_rect_min(bb), eli_rect_max(bb), frame_col, true, rounding);

    /* Optional background fill behind the image. */
    eli_vec2 img_min = eli_make_vec2(pos.x + padding.x, pos.y + padding.y);
    eli_vec2 img_max = eli_make_vec2(img_min.x + image_size.x, img_min.y + image_size.y);

    if (bg_col.w > 0.0f) {
        eli_col32 bg = eli_color_convert_float4_to_u32(bg_col);
        eli_draw_list_add_rect_filled(dl, img_min, img_max, bg, 0.0f, ELI_DRAW_NONE);
    }

    /* The image itself with tint. */
    eli_col32 tc = eli_color_convert_float4_to_u32(tint_col);
    eli_draw_list_add_image(dl, user_texture_id, img_min, img_max, uv0, uv1, tc);

    return pressed;
}

#endif /* ELI_WIDGETS_ELI_IMAGE_H */
