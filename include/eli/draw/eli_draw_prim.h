/**
 * @file eli_draw_prim.h
 * @brief Primitive shape helpers: lines, rectangles, triangles, quads, circles,
 *        ngons, ellipses, polylines, convex fills, and bezier curves.
 *
 * These build on the path API and the prim_* writers. Geometry is generated
 * without anti-aliasing so vertex/index counts are deterministic; the backend
 * (e.g. a Canvas2D or WebGL renderer) is expected to smooth if desired.
 *
 * @status Phase 2 primitives in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_DRAW_ELI_DRAW_PRIM_H
#define ELI_DRAW_ELI_DRAW_PRIM_H

#include "eli_draw_types.h"
#include "eli_draw_list.h"
#include "eli_draw_path.h"

#include "../core/eli_platform.h"
#include "../core/eli_types.h"

/** Maximum segment count for explicit circle/ngon tessellation. */
#define ELI_DRAW_CIRCLE_SEGMENT_MAX 512

/** Normalize a 2D vector in place; leaves a zero vector unchanged. */
static inline void eli_draw_normalize2f(float *x, float *y)
{
    float d2 = (*x) * (*x) + (*y) * (*y);
    if (d2 > 0.0f) {
        float inv_len = 1.0f / sqrtf(d2);
        *x *= inv_len;
        *y *= inv_len;
    }
}

/* ---------------------------------------------------------------------------
 * Polyline and convex fill (renderer-level, consumed by the path API)
 * ------------------------------------------------------------------------- */

/**
 * Stroke a polyline through `points`. Each segment becomes a quad (four
 * vertices, six indices); no vertices are shared between segments.
 *
 * @param list        Target draw list.
 * @param points      Point array.
 * @param num_points  Number of points (>= 2).
 * @param col         Line color (fully transparent is a no-op).
 * @param flags       ELI_DRAW_CLOSED to connect last point to first.
 * @param thickness   Line thickness in pixels.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_add_polyline(eli_draw_list *list, const eli_vec2 *points,
                                              int num_points, eli_col32 col,
                                              eli_draw_flags flags, float thickness)
{
    if (num_points < 2 || (col & ELI_COL32_A_MASK) == 0)
        return;

    bool closed = (flags & ELI_DRAW_CLOSED) != 0;
    int count = closed ? num_points : num_points - 1;
    eli_vec2 uv = list->tex_uv_white_pixel;
    eli_draw_list_prim_reserve(list, count * 6, count * 4);

    for (int i1 = 0; i1 < count; i1++) {
        int i2 = ((i1 + 1) == num_points) ? 0 : (i1 + 1);
        eli_vec2 p1 = points[i1];
        eli_vec2 p2 = points[i2];
        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;
        eli_draw_normalize2f(&dx, &dy);
        dx *= thickness * 0.5f;
        dy *= thickness * 0.5f;

        eli_draw_idx base = (eli_draw_idx)list->vtx_current_idx;
        eli_draw_list_prim_write_vtx(list, eli_make_vec2(p1.x + dy, p1.y - dx), uv, col);
        eli_draw_list_prim_write_vtx(list, eli_make_vec2(p2.x + dy, p2.y - dx), uv, col);
        eli_draw_list_prim_write_vtx(list, eli_make_vec2(p2.x - dy, p2.y + dx), uv, col);
        eli_draw_list_prim_write_vtx(list, eli_make_vec2(p1.x - dy, p1.y + dx), uv, col);
        eli_draw_list_prim_write_idx(list, base);
        eli_draw_list_prim_write_idx(list, (eli_draw_idx)(base + 1));
        eli_draw_list_prim_write_idx(list, (eli_draw_idx)(base + 2));
        eli_draw_list_prim_write_idx(list, base);
        eli_draw_list_prim_write_idx(list, (eli_draw_idx)(base + 2));
        eli_draw_list_prim_write_idx(list, (eli_draw_idx)(base + 3));
    }
}

/**
 * Fill a convex polygon as a triangle fan. Produces `num_points` vertices and
 * `(num_points - 2) * 3` indices.
 *
 * @param list        Target draw list.
 * @param points      Convex, clockwise-wound point array.
 * @param num_points  Number of points (>= 3).
 * @param col         Fill color (fully transparent is a no-op).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_add_convex_poly_filled(eli_draw_list *list,
                                                        const eli_vec2 *points, int num_points,
                                                        eli_col32 col)
{
    if (num_points < 3 || (col & ELI_COL32_A_MASK) == 0)
        return;

    eli_vec2 uv = list->tex_uv_white_pixel;
    eli_draw_list_prim_reserve(list, (num_points - 2) * 3, num_points);
    eli_draw_idx base = (eli_draw_idx)list->vtx_current_idx;
    for (int i = 0; i < num_points; i++)
        eli_draw_list_prim_write_vtx(list, points[i], uv, col);
    for (int i = 2; i < num_points; i++) {
        eli_draw_list_prim_write_idx(list, base);
        eli_draw_list_prim_write_idx(list, (eli_draw_idx)(base + i - 1));
        eli_draw_list_prim_write_idx(list, (eli_draw_idx)(base + i));
    }
}

/* ---------------------------------------------------------------------------
 * Lines and rectangles
 * ------------------------------------------------------------------------- */

/**
 * Draw a line segment from p1 to p2.
 *
 * @param list       Target draw list.
 * @param p1         Start point.
 * @param p2         End point.
 * @param col        Line color.
 * @param thickness  Line thickness in pixels.
 */
static inline void eli_draw_list_add_line(eli_draw_list *list, eli_vec2 p1, eli_vec2 p2,
                                          eli_col32 col, float thickness)
{
    if ((col & ELI_COL32_A_MASK) == 0)
        return;
    eli_draw_list_path_line_to(list, eli_make_vec2(p1.x + 0.5f, p1.y + 0.5f));
    eli_draw_list_path_line_to(list, eli_make_vec2(p2.x + 0.5f, p2.y + 0.5f));
    eli_draw_list_path_stroke(list, col, ELI_DRAW_NONE, thickness);
}

/**
 * Draw a rectangle outline.
 *
 * @param list       Target draw list.
 * @param p_min      Top-left corner.
 * @param p_max      Bottom-right corner.
 * @param col        Line color.
 * @param rounding   Corner radius in pixels.
 * @param flags      Corner-selection flags (see eli_draw_flags).
 * @param thickness  Line thickness in pixels.
 */
static inline void eli_draw_list_add_rect(eli_draw_list *list, eli_vec2 p_min, eli_vec2 p_max,
                                          eli_col32 col, float rounding, eli_draw_flags flags,
                                          float thickness)
{
    if ((col & ELI_COL32_A_MASK) == 0)
        return;
    eli_draw_list_path_rect(list, eli_make_vec2(p_min.x + 0.5f, p_min.y + 0.5f),
                            eli_make_vec2(p_max.x - 0.49f, p_max.y - 0.49f), rounding, flags);
    eli_draw_list_path_stroke(list, col, ELI_DRAW_CLOSED, thickness);
}

/**
 * Draw a filled rectangle.
 *
 * @param list      Target draw list.
 * @param p_min     Top-left corner.
 * @param p_max     Bottom-right corner.
 * @param col       Fill color.
 * @param rounding  Corner radius in pixels.
 * @param flags     Corner-selection flags (see eli_draw_flags).
 */
static inline void eli_draw_list_add_rect_filled(eli_draw_list *list, eli_vec2 p_min,
                                                 eli_vec2 p_max, eli_col32 col, float rounding,
                                                 eli_draw_flags flags)
{
    if ((col & ELI_COL32_A_MASK) == 0)
        return;
    if (rounding < 0.5f || (flags & ELI_DRAW_ROUND_CORNERS_MASK) == ELI_DRAW_ROUND_CORNERS_NONE) {
        eli_draw_list_prim_reserve(list, 6, 4);
        eli_draw_list_prim_rect(list, p_min, p_max, col);
    } else {
        eli_draw_list_path_rect(list, p_min, p_max, rounding, flags);
        eli_draw_list_path_fill_convex(list, col);
    }
}

/**
 * Draw a filled rectangle with a per-corner color gradient.
 *
 * @param list           Target draw list.
 * @param p_min          Top-left corner.
 * @param p_max          Bottom-right corner.
 * @param col_upr_left   Top-left color.
 * @param col_upr_right  Top-right color.
 * @param col_bot_right  Bottom-right color.
 * @param col_bot_left   Bottom-left color.
 */
static inline void eli_draw_list_add_rect_filled_multi_color(eli_draw_list *list, eli_vec2 p_min,
                                                             eli_vec2 p_max, eli_col32 col_upr_left,
                                                             eli_col32 col_upr_right,
                                                             eli_col32 col_bot_right,
                                                             eli_col32 col_bot_left)
{
    if (((col_upr_left | col_upr_right | col_bot_right | col_bot_left) & ELI_COL32_A_MASK) == 0)
        return;
    eli_vec2 uv = list->tex_uv_white_pixel;
    eli_draw_list_prim_reserve(list, 6, 4);
    eli_draw_idx base = (eli_draw_idx)list->vtx_current_idx;
    eli_draw_list_prim_write_idx(list, base);
    eli_draw_list_prim_write_idx(list, (eli_draw_idx)(base + 1));
    eli_draw_list_prim_write_idx(list, (eli_draw_idx)(base + 2));
    eli_draw_list_prim_write_idx(list, base);
    eli_draw_list_prim_write_idx(list, (eli_draw_idx)(base + 2));
    eli_draw_list_prim_write_idx(list, (eli_draw_idx)(base + 3));
    eli_draw_list_prim_write_vtx(list, p_min, uv, col_upr_left);
    eli_draw_list_prim_write_vtx(list, eli_make_vec2(p_max.x, p_min.y), uv, col_upr_right);
    eli_draw_list_prim_write_vtx(list, p_max, uv, col_bot_right);
    eli_draw_list_prim_write_vtx(list, eli_make_vec2(p_min.x, p_max.y), uv, col_bot_left);
}

/* ---------------------------------------------------------------------------
 * Triangles and quads
 * ------------------------------------------------------------------------- */

/** Draw a triangle outline through p1, p2, p3. */
static inline void eli_draw_list_add_triangle(eli_draw_list *list, eli_vec2 p1, eli_vec2 p2,
                                              eli_vec2 p3, eli_col32 col, float thickness)
{
    if ((col & ELI_COL32_A_MASK) == 0)
        return;
    eli_draw_list_path_line_to(list, p1);
    eli_draw_list_path_line_to(list, p2);
    eli_draw_list_path_line_to(list, p3);
    eli_draw_list_path_stroke(list, col, ELI_DRAW_CLOSED, thickness);
}

/** Draw a filled triangle through p1, p2, p3. */
static inline void eli_draw_list_add_triangle_filled(eli_draw_list *list, eli_vec2 p1, eli_vec2 p2,
                                                     eli_vec2 p3, eli_col32 col)
{
    if ((col & ELI_COL32_A_MASK) == 0)
        return;
    eli_draw_list_path_line_to(list, p1);
    eli_draw_list_path_line_to(list, p2);
    eli_draw_list_path_line_to(list, p3);
    eli_draw_list_path_fill_convex(list, col);
}

/** Draw a quad outline through p1, p2, p3, p4. */
static inline void eli_draw_list_add_quad(eli_draw_list *list, eli_vec2 p1, eli_vec2 p2,
                                          eli_vec2 p3, eli_vec2 p4, eli_col32 col, float thickness)
{
    if ((col & ELI_COL32_A_MASK) == 0)
        return;
    eli_draw_list_path_line_to(list, p1);
    eli_draw_list_path_line_to(list, p2);
    eli_draw_list_path_line_to(list, p3);
    eli_draw_list_path_line_to(list, p4);
    eli_draw_list_path_stroke(list, col, ELI_DRAW_CLOSED, thickness);
}

/** Draw a filled quad through p1, p2, p3, p4. */
static inline void eli_draw_list_add_quad_filled(eli_draw_list *list, eli_vec2 p1, eli_vec2 p2,
                                                 eli_vec2 p3, eli_vec2 p4, eli_col32 col)
{
    if ((col & ELI_COL32_A_MASK) == 0)
        return;
    eli_draw_list_path_line_to(list, p1);
    eli_draw_list_path_line_to(list, p2);
    eli_draw_list_path_line_to(list, p3);
    eli_draw_list_path_line_to(list, p4);
    eli_draw_list_path_fill_convex(list, col);
}

/* ---------------------------------------------------------------------------
 * Circles, ngons, ellipses
 * ------------------------------------------------------------------------- */

/** Clamp an explicit segment count into the valid range [3, max]. */
static inline int eli_draw_clamp_circle_segments(int num_segments)
{
    if (num_segments < 3)
        return 3;
    if (num_segments > ELI_DRAW_CIRCLE_SEGMENT_MAX)
        return ELI_DRAW_CIRCLE_SEGMENT_MAX;
    return num_segments;
}

/**
 * Draw a circle outline.
 *
 * @param list          Target draw list.
 * @param center        Circle center.
 * @param radius        Circle radius.
 * @param col           Line color.
 * @param num_segments  Segment count, or <=0 for automatic.
 * @param thickness     Line thickness in pixels.
 */
static inline void eli_draw_list_add_circle(eli_draw_list *list, eli_vec2 center, float radius,
                                            eli_col32 col, int num_segments, float thickness)
{
    if ((col & ELI_COL32_A_MASK) == 0 || radius < 0.5f)
        return;
    if (num_segments <= 0)
        num_segments = eli_draw_circle_auto_segments(radius, list->circle_segment_max_error);
    num_segments = eli_draw_clamp_circle_segments(num_segments);
    float a_max = (2.0f * ELI_PI) * ((float)num_segments - 1.0f) / (float)num_segments;
    eli_draw_list_path_arc_to(list, center, radius - 0.5f, 0.0f, a_max, num_segments - 1);
    eli_draw_list_path_stroke(list, col, ELI_DRAW_CLOSED, thickness);
}

/**
 * Draw a filled circle.
 *
 * @param list          Target draw list.
 * @param center        Circle center.
 * @param radius        Circle radius.
 * @param col           Fill color.
 * @param num_segments  Segment count, or <=0 for automatic.
 */
static inline void eli_draw_list_add_circle_filled(eli_draw_list *list, eli_vec2 center,
                                                   float radius, eli_col32 col, int num_segments)
{
    if ((col & ELI_COL32_A_MASK) == 0 || radius < 0.5f)
        return;
    if (num_segments <= 0)
        num_segments = eli_draw_circle_auto_segments(radius, list->circle_segment_max_error);
    num_segments = eli_draw_clamp_circle_segments(num_segments);
    float a_max = (2.0f * ELI_PI) * ((float)num_segments - 1.0f) / (float)num_segments;
    eli_draw_list_path_arc_to(list, center, radius, 0.0f, a_max, num_segments - 1);
    eli_draw_list_path_fill_convex(list, col);
}

/**
 * Draw an n-gon outline (honors `num_segments` exactly).
 *
 * @param list          Target draw list.
 * @param center        Polygon center.
 * @param radius        Circumradius.
 * @param col           Line color.
 * @param num_segments  Number of sides (> 2).
 * @param thickness     Line thickness in pixels.
 */
static inline void eli_draw_list_add_ngon(eli_draw_list *list, eli_vec2 center, float radius,
                                          eli_col32 col, int num_segments, float thickness)
{
    if ((col & ELI_COL32_A_MASK) == 0 || num_segments <= 2)
        return;
    float a_max = (2.0f * ELI_PI) * ((float)num_segments - 1.0f) / (float)num_segments;
    eli_draw_list_path_arc_to(list, center, radius - 0.5f, 0.0f, a_max, num_segments - 1);
    eli_draw_list_path_stroke(list, col, ELI_DRAW_CLOSED, thickness);
}

/**
 * Draw a filled n-gon (honors `num_segments` exactly).
 *
 * @param list          Target draw list.
 * @param center        Polygon center.
 * @param radius        Circumradius.
 * @param col           Fill color.
 * @param num_segments  Number of sides (> 2).
 */
static inline void eli_draw_list_add_ngon_filled(eli_draw_list *list, eli_vec2 center,
                                                 float radius, eli_col32 col, int num_segments)
{
    if ((col & ELI_COL32_A_MASK) == 0 || num_segments <= 2)
        return;
    float a_max = (2.0f * ELI_PI) * ((float)num_segments - 1.0f) / (float)num_segments;
    eli_draw_list_path_arc_to(list, center, radius, 0.0f, a_max, num_segments - 1);
    eli_draw_list_path_fill_convex(list, col);
}

/**
 * Draw an ellipse outline.
 *
 * @param list          Target draw list.
 * @param center        Ellipse center.
 * @param radius        Per-axis radii (x, y).
 * @param col           Line color.
 * @param rot           Rotation in radians.
 * @param num_segments  Segment count, or <=0 for automatic.
 * @param thickness     Line thickness in pixels.
 */
static inline void eli_draw_list_add_ellipse(eli_draw_list *list, eli_vec2 center, eli_vec2 radius,
                                             eli_col32 col, float rot, int num_segments,
                                             float thickness)
{
    if ((col & ELI_COL32_A_MASK) == 0)
        return;
    if (num_segments <= 0)
        num_segments = eli_draw_circle_auto_segments(eli_max_f(radius.x, radius.y),
                                                     list->circle_segment_max_error);
    num_segments = eli_draw_clamp_circle_segments(num_segments);
    float a_max = (2.0f * ELI_PI) * ((float)num_segments - 1.0f) / (float)num_segments;
    eli_draw_list_path_elliptical_arc_to(list, center, radius, rot, 0.0f, a_max, num_segments - 1);
    eli_draw_list_path_stroke(list, col, ELI_DRAW_CLOSED, thickness);
}

/**
 * Draw a filled ellipse.
 *
 * @param list          Target draw list.
 * @param center        Ellipse center.
 * @param radius        Per-axis radii (x, y).
 * @param col           Fill color.
 * @param rot           Rotation in radians.
 * @param num_segments  Segment count, or <=0 for automatic.
 */
static inline void eli_draw_list_add_ellipse_filled(eli_draw_list *list, eli_vec2 center,
                                                    eli_vec2 radius, eli_col32 col, float rot,
                                                    int num_segments)
{
    if ((col & ELI_COL32_A_MASK) == 0)
        return;
    if (num_segments <= 0)
        num_segments = eli_draw_circle_auto_segments(eli_max_f(radius.x, radius.y),
                                                     list->circle_segment_max_error);
    num_segments = eli_draw_clamp_circle_segments(num_segments);
    float a_max = (2.0f * ELI_PI) * ((float)num_segments - 1.0f) / (float)num_segments;
    eli_draw_list_path_elliptical_arc_to(list, center, radius, rot, 0.0f, a_max, num_segments - 1);
    eli_draw_list_path_fill_convex(list, col);
}

/* ---------------------------------------------------------------------------
 * Bezier curves
 * ------------------------------------------------------------------------- */

/**
 * Draw a cubic bezier curve through control points p1..p4.
 *
 * @param list          Target draw list.
 * @param p1            Start point.
 * @param p2            First control point.
 * @param p3            Second control point.
 * @param p4            End point.
 * @param col           Line color.
 * @param thickness     Line thickness in pixels.
 * @param num_segments  Fixed segment count, or 0 for adaptive.
 */
static inline void eli_draw_list_add_bezier_cubic(eli_draw_list *list, eli_vec2 p1, eli_vec2 p2,
                                                  eli_vec2 p3, eli_vec2 p4, eli_col32 col,
                                                  float thickness, int num_segments)
{
    if ((col & ELI_COL32_A_MASK) == 0)
        return;
    eli_draw_list_path_line_to(list, p1);
    eli_draw_list_path_bezier_cubic_curve_to(list, p2, p3, p4, num_segments);
    eli_draw_list_path_stroke(list, col, ELI_DRAW_NONE, thickness);
}

/**
 * Draw a quadratic bezier curve through control points p1, p2, p3.
 *
 * @param list          Target draw list.
 * @param p1            Start point.
 * @param p2            Control point.
 * @param p3            End point.
 * @param col           Line color.
 * @param thickness     Line thickness in pixels.
 * @param num_segments  Fixed segment count, or 0 for adaptive.
 */
static inline void eli_draw_list_add_bezier_quadratic(eli_draw_list *list, eli_vec2 p1,
                                                      eli_vec2 p2, eli_vec2 p3, eli_col32 col,
                                                      float thickness, int num_segments)
{
    if ((col & ELI_COL32_A_MASK) == 0)
        return;
    eli_draw_list_path_line_to(list, p1);
    eli_draw_list_path_bezier_quadratic_curve_to(list, p2, p3, num_segments);
    eli_draw_list_path_stroke(list, col, ELI_DRAW_NONE, thickness);
}

#endif /* ELI_DRAW_ELI_DRAW_PRIM_H */
