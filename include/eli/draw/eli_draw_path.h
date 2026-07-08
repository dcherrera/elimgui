/**
 * @file eli_draw_path.h
 * @brief Stateful path-building API: accumulate points (lines, arcs, ellipses,
 *        bezier curves, rounded rects) into the draw list's path buffer, then
 *        emit them with path_stroke (outline) or path_fill_convex (fill).
 *
 * Path builders only touch the point buffer; path_stroke/path_fill_convex hand
 * the accumulated points to the polyline/convex-fill primitives (declared here,
 * defined in eli_draw_prim.h) and clear the path.
 *
 * @status Phase 2 path API in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_DRAW_ELI_DRAW_PATH_H
#define ELI_DRAW_ELI_DRAW_PATH_H

#include "eli_draw_types.h"
#include "eli_draw_list.h"

#include "../core/eli_platform.h"
#include "../core/eli_types.h"

/** Auto-tessellation tolerance for bezier curves (screen pixels). */
#define ELI_DRAW_CURVE_TESS_TOL 1.25f

/** Recursion cap for adaptive bezier subdivision. */
#define ELI_DRAW_BEZIER_MAX_LEVEL 10

/* Primitive emitters used by path_stroke/path_fill_convex (defined in prim.h). */
static inline void eli_draw_list_add_polyline(eli_draw_list *list, const eli_vec2 *points,
                                              int num_points, eli_col32 col,
                                              eli_draw_flags flags, float thickness);
static inline void eli_draw_list_add_convex_poly_filled(eli_draw_list *list,
                                                        const eli_vec2 *points, int num_points,
                                                        eli_col32 col);

/* ---------------------------------------------------------------------------
 * Auto segment count
 * ------------------------------------------------------------------------- */

/**
 * Compute the number of segments a full circle of the given radius needs so its
 * chord deviates from the true arc by at most `max_error`. Result is rounded up
 * to an even number and clamped to [4, 512].
 *
 * @param radius     Circle radius in pixels (assumed > 0).
 * @param max_error  Maximum tolerated chord error in pixels.
 * @return           Segment count in [4, 512].
 *
 * Thread-safe: yes (pure)
 * Reentrant: yes
 */
static inline int eli_draw_circle_auto_segments(float radius, float max_error)
{
    if (radius <= 0.0f)
        return 4;
    float clamped_error = eli_min_f(max_error, radius);
    int n = (int)ceilf(ELI_PI / acosf(1.0f - clamped_error / radius));
    n = ((n + 1) / 2) * 2;
    if (n < 4)
        n = 4;
    if (n > 512)
        n = 512;
    return n;
}

/* ---------------------------------------------------------------------------
 * Path point buffer
 * ------------------------------------------------------------------------- */

/** Append one point to the path buffer, growing it as needed. */
static inline void eli_draw_path_push(eli_draw_list *list, eli_vec2 p)
{
    list->path = (eli_vec2 *)eli_draw_buf_grow(list->path, &list->path_capacity,
                                               list->path_count + 1, sizeof(*list->path));
    list->path[list->path_count++] = p;
}

/**
 * Clear the current path without emitting anything.
 *
 * @param list  Target draw list.
 */
static inline void eli_draw_list_path_clear(eli_draw_list *list)
{
    list->path_count = 0;
}

/**
 * Append a point to the path.
 *
 * @param list  Target draw list.
 * @param pos   Point to append.
 */
static inline void eli_draw_list_path_line_to(eli_draw_list *list, eli_vec2 pos)
{
    eli_draw_path_push(list, pos);
}

/**
 * Append a point unless it duplicates the last one (avoids zero-length edges).
 *
 * @param list  Target draw list.
 * @param pos   Point to append.
 */
static inline void eli_draw_list_path_line_to_merge_duplicate(eli_draw_list *list, eli_vec2 pos)
{
    if (list->path_count == 0 || list->path[list->path_count - 1].x != pos.x ||
        list->path[list->path_count - 1].y != pos.y)
        eli_draw_path_push(list, pos);
}

/**
 * Fill the current path as a convex polygon and clear it.
 *
 * @param list  Target draw list.
 * @param col   Fill color.
 */
static inline void eli_draw_list_path_fill_convex(eli_draw_list *list, eli_col32 col)
{
    eli_draw_list_add_convex_poly_filled(list, list->path, list->path_count, col);
    list->path_count = 0;
}

/**
 * Stroke the current path as a polyline and clear it.
 *
 * @param list       Target draw list.
 * @param col        Line color.
 * @param flags      ELI_DRAW_CLOSED to connect the last point to the first.
 * @param thickness  Line thickness in pixels.
 */
static inline void eli_draw_list_path_stroke(eli_draw_list *list, eli_col32 col,
                                             eli_draw_flags flags, float thickness)
{
    eli_draw_list_add_polyline(list, list->path, list->path_count, col, flags, thickness);
    list->path_count = 0;
}

/* ---------------------------------------------------------------------------
 * Arcs and ellipses
 * ------------------------------------------------------------------------- */

/**
 * Append an arc centered at `center` from angle `a_min` to `a_max` (radians).
 * When `num_segments <= 0` a segment count is chosen automatically from the
 * radius and arc length. Emits num_segments+1 points.
 *
 * @param list          Target draw list.
 * @param center        Arc center.
 * @param radius        Arc radius.
 * @param a_min         Start angle in radians.
 * @param a_max         End angle in radians.
 * @param num_segments  Segment count, or <=0 for automatic.
 */
static inline void eli_draw_list_path_arc_to(eli_draw_list *list, eli_vec2 center, float radius,
                                             float a_min, float a_max, int num_segments)
{
    if (radius < 0.5f) {
        eli_draw_path_push(list, center);
        return;
    }
    if (num_segments <= 0) {
        int circle_segments = eli_draw_circle_auto_segments(radius, list->circle_segment_max_error);
        float arc_len = fabsf(a_max - a_min);
        num_segments = (int)ceilf((float)circle_segments * arc_len / (2.0f * ELI_PI));
        if (num_segments < 1)
            num_segments = 1;
    }
    for (int i = 0; i <= num_segments; i++) {
        float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        eli_draw_path_push(list, eli_make_vec2(center.x + cosf(a) * radius,
                                               center.y + sinf(a) * radius));
    }
}

/**
 * Append a quarter-resolution arc using the 12-o'clock division (0: east,
 * 3: south, 6: west, 9: north, 12: east). Points are emitted at each 1/12 step
 * from `a_min_of_12` to `a_max_of_12`, inclusive.
 *
 * @param list         Target draw list.
 * @param center       Arc center.
 * @param radius       Arc radius.
 * @param a_min_of_12  Start step in [0, 12].
 * @param a_max_of_12  End step in [0, 12].
 */
static inline void eli_draw_list_path_arc_to_fast(eli_draw_list *list, eli_vec2 center,
                                                  float radius, int a_min_of_12, int a_max_of_12)
{
    if (radius < 0.5f) {
        eli_draw_path_push(list, center);
        return;
    }
    int step = (a_max_of_12 >= a_min_of_12) ? 1 : -1;
    for (int a = a_min_of_12;; a += step) {
        float angle = ((float)a / 12.0f) * (2.0f * ELI_PI);
        eli_draw_path_push(list, eli_make_vec2(center.x + cosf(angle) * radius,
                                               center.y + sinf(angle) * radius));
        if (a == a_max_of_12)
            break;
    }
}

/**
 * Append an elliptical arc rotated by `rot`, from `a_min` to `a_max` (radians).
 * When `num_segments <= 0` a count is chosen from the larger radius.
 *
 * @param list          Target draw list.
 * @param center        Ellipse center.
 * @param radius        Per-axis radii (x, y).
 * @param rot           Rotation of the ellipse in radians.
 * @param a_min         Start angle in radians.
 * @param a_max         End angle in radians.
 * @param num_segments  Segment count, or <=0 for automatic.
 */
static inline void eli_draw_list_path_elliptical_arc_to(eli_draw_list *list, eli_vec2 center,
                                                        eli_vec2 radius, float rot, float a_min,
                                                        float a_max, int num_segments)
{
    if (num_segments <= 0)
        num_segments = eli_draw_circle_auto_segments(eli_max_f(radius.x, radius.y),
                                                     list->circle_segment_max_error);
    if (num_segments < 1)
        num_segments = 1;
    float cos_rot = cosf(rot);
    float sin_rot = sinf(rot);
    for (int i = 0; i <= num_segments; i++) {
        float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        float px = cosf(a) * radius.x;
        float py = sinf(a) * radius.y;
        eli_draw_path_push(list, eli_make_vec2(center.x + px * cos_rot - py * sin_rot,
                                               center.y + px * sin_rot + py * cos_rot));
    }
}

/* ---------------------------------------------------------------------------
 * Bezier curves
 * ------------------------------------------------------------------------- */

/** Cubic bezier position at parameter t in [0,1]. */
static inline eli_vec2 eli_draw_bezier_cubic_calc(eli_vec2 p1, eli_vec2 p2, eli_vec2 p3,
                                                  eli_vec2 p4, float t)
{
    float u = 1.0f - t;
    float w1 = u * u * u;
    float w2 = 3.0f * u * u * t;
    float w3 = 3.0f * u * t * t;
    float w4 = t * t * t;
    return eli_make_vec2(w1 * p1.x + w2 * p2.x + w3 * p3.x + w4 * p4.x,
                         w1 * p1.y + w2 * p2.y + w3 * p3.y + w4 * p4.y);
}

/** Quadratic bezier position at parameter t in [0,1]. */
static inline eli_vec2 eli_draw_bezier_quadratic_calc(eli_vec2 p1, eli_vec2 p2, eli_vec2 p3,
                                                      float t)
{
    float u = 1.0f - t;
    float w1 = u * u;
    float w2 = 2.0f * u * t;
    float w3 = t * t;
    return eli_make_vec2(w1 * p1.x + w2 * p2.x + w3 * p3.x,
                         w1 * p1.y + w2 * p2.y + w3 * p3.y);
}

/** Adaptive De Casteljau subdivision for a cubic bezier segment. */
static inline void eli_draw_bezier_cubic_casteljau(eli_draw_list *list, float x1, float y1,
                                                   float x2, float y2, float x3, float y3,
                                                   float x4, float y4, float tess_tol, int level)
{
    float dx = x4 - x1;
    float dy = y4 - y1;
    float d2 = (x2 - x4) * dy - (y2 - y4) * dx;
    float d3 = (x3 - x4) * dy - (y3 - y4) * dx;
    d2 = (d2 >= 0.0f) ? d2 : -d2;
    d3 = (d3 >= 0.0f) ? d3 : -d3;
    if ((d2 + d3) * (d2 + d3) < tess_tol * (dx * dx + dy * dy)) {
        eli_draw_path_push(list, eli_make_vec2(x4, y4));
    } else if (level < ELI_DRAW_BEZIER_MAX_LEVEL) {
        float x12 = (x1 + x2) * 0.5f, y12 = (y1 + y2) * 0.5f;
        float x23 = (x2 + x3) * 0.5f, y23 = (y2 + y3) * 0.5f;
        float x34 = (x3 + x4) * 0.5f, y34 = (y3 + y4) * 0.5f;
        float x123 = (x12 + x23) * 0.5f, y123 = (y12 + y23) * 0.5f;
        float x234 = (x23 + x34) * 0.5f, y234 = (y23 + y34) * 0.5f;
        float x1234 = (x123 + x234) * 0.5f, y1234 = (y123 + y234) * 0.5f;
        eli_draw_bezier_cubic_casteljau(list, x1, y1, x12, y12, x123, y123, x1234, y1234,
                                        tess_tol, level + 1);
        eli_draw_bezier_cubic_casteljau(list, x1234, y1234, x234, y234, x34, y34, x4, y4,
                                        tess_tol, level + 1);
    }
}

/** Adaptive De Casteljau subdivision for a quadratic bezier segment. */
static inline void eli_draw_bezier_quadratic_casteljau(eli_draw_list *list, float x1, float y1,
                                                       float x2, float y2, float x3, float y3,
                                                       float tess_tol, int level)
{
    float dx = x3 - x1, dy = y3 - y1;
    float det = (x2 - x3) * dy - (y2 - y3) * dx;
    if (det * det * 4.0f < tess_tol * (dx * dx + dy * dy)) {
        eli_draw_path_push(list, eli_make_vec2(x3, y3));
    } else if (level < ELI_DRAW_BEZIER_MAX_LEVEL) {
        float x12 = (x1 + x2) * 0.5f, y12 = (y1 + y2) * 0.5f;
        float x23 = (x2 + x3) * 0.5f, y23 = (y2 + y3) * 0.5f;
        float x123 = (x12 + x23) * 0.5f, y123 = (y12 + y23) * 0.5f;
        eli_draw_bezier_quadratic_casteljau(list, x1, y1, x12, y12, x123, y123, tess_tol,
                                            level + 1);
        eli_draw_bezier_quadratic_casteljau(list, x123, y123, x23, y23, x3, y3, tess_tol,
                                            level + 1);
    }
}

/**
 * Append a cubic bezier from the last path point through control points p2, p3
 * to p4. `num_segments == 0` auto-tessellates; otherwise emits `num_segments`
 * evenly-parameterized points.
 *
 * @param list          Target draw list (path must be non-empty).
 * @param p2            First control point.
 * @param p3            Second control point.
 * @param p4            End point.
 * @param num_segments  Fixed segment count, or 0 for adaptive.
 */
static inline void eli_draw_list_path_bezier_cubic_curve_to(eli_draw_list *list, eli_vec2 p2,
                                                            eli_vec2 p3, eli_vec2 p4,
                                                            int num_segments)
{
    if (list->path_count == 0)
        return;
    eli_vec2 p1 = list->path[list->path_count - 1];
    if (num_segments == 0) {
        eli_draw_bezier_cubic_casteljau(list, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y,
                                        ELI_DRAW_CURVE_TESS_TOL, 0);
    } else {
        float t_step = 1.0f / (float)num_segments;
        for (int i = 1; i <= num_segments; i++)
            eli_draw_path_push(list, eli_draw_bezier_cubic_calc(p1, p2, p3, p4, t_step * (float)i));
    }
}

/**
 * Append a quadratic bezier from the last path point through control point p2
 * to p3. `num_segments == 0` auto-tessellates.
 *
 * @param list          Target draw list (path must be non-empty).
 * @param p2            Control point.
 * @param p3            End point.
 * @param num_segments  Fixed segment count, or 0 for adaptive.
 */
static inline void eli_draw_list_path_bezier_quadratic_curve_to(eli_draw_list *list, eli_vec2 p2,
                                                                eli_vec2 p3, int num_segments)
{
    if (list->path_count == 0)
        return;
    eli_vec2 p1 = list->path[list->path_count - 1];
    if (num_segments == 0) {
        eli_draw_bezier_quadratic_casteljau(list, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y,
                                            ELI_DRAW_CURVE_TESS_TOL, 0);
    } else {
        float t_step = 1.0f / (float)num_segments;
        for (int i = 1; i <= num_segments; i++)
            eli_draw_path_push(list, eli_draw_bezier_quadratic_calc(p1, p2, p3, t_step * (float)i));
    }
}

/* ---------------------------------------------------------------------------
 * Rectangle path
 * ------------------------------------------------------------------------- */

/** Resolve corner-rounding flags, defaulting to all corners when none set. */
static inline eli_draw_flags eli_draw_fix_rect_corner_flags(eli_draw_flags flags)
{
    if ((flags & ELI_DRAW_ROUND_CORNERS_MASK) == 0)
        flags |= ELI_DRAW_ROUND_CORNERS_ALL;
    return flags;
}

/**
 * Append a (optionally rounded) rectangle outline path from corner `a` (top-
 * left) to `b` (bottom-right). Rounding is clamped to fit the rectangle.
 *
 * @param list      Target draw list.
 * @param a         Top-left corner.
 * @param b         Bottom-right corner.
 * @param rounding  Corner radius in pixels (<0.5 disables rounding).
 * @param flags     Corner-selection flags (see eli_draw_flags).
 */
static inline void eli_draw_list_path_rect(eli_draw_list *list, eli_vec2 a, eli_vec2 b,
                                           float rounding, eli_draw_flags flags)
{
    if (rounding >= 0.5f) {
        flags = eli_draw_fix_rect_corner_flags(flags);
        float wf = ((flags & ELI_DRAW_ROUND_CORNERS_TOP) == ELI_DRAW_ROUND_CORNERS_TOP ||
                    (flags & ELI_DRAW_ROUND_CORNERS_BOTTOM) == ELI_DRAW_ROUND_CORNERS_BOTTOM)
                       ? 0.5f
                       : 1.0f;
        float hf = ((flags & ELI_DRAW_ROUND_CORNERS_LEFT) == ELI_DRAW_ROUND_CORNERS_LEFT ||
                    (flags & ELI_DRAW_ROUND_CORNERS_RIGHT) == ELI_DRAW_ROUND_CORNERS_RIGHT)
                       ? 0.5f
                       : 1.0f;
        rounding = eli_min_f(rounding, fabsf(b.x - a.x) * wf - 1.0f);
        rounding = eli_min_f(rounding, fabsf(b.y - a.y) * hf - 1.0f);
    }
    if (rounding < 0.5f || (flags & ELI_DRAW_ROUND_CORNERS_MASK) == ELI_DRAW_ROUND_CORNERS_NONE) {
        eli_draw_list_path_line_to(list, a);
        eli_draw_list_path_line_to(list, eli_make_vec2(b.x, a.y));
        eli_draw_list_path_line_to(list, b);
        eli_draw_list_path_line_to(list, eli_make_vec2(a.x, b.y));
        return;
    }
    float r_tl = (flags & ELI_DRAW_ROUND_CORNERS_TOP_LEFT) ? rounding : 0.0f;
    float r_tr = (flags & ELI_DRAW_ROUND_CORNERS_TOP_RIGHT) ? rounding : 0.0f;
    float r_br = (flags & ELI_DRAW_ROUND_CORNERS_BOT_RIGHT) ? rounding : 0.0f;
    float r_bl = (flags & ELI_DRAW_ROUND_CORNERS_BOT_LEFT) ? rounding : 0.0f;
    eli_draw_list_path_arc_to_fast(list, eli_make_vec2(a.x + r_tl, a.y + r_tl), r_tl, 6, 9);
    eli_draw_list_path_arc_to_fast(list, eli_make_vec2(b.x - r_tr, a.y + r_tr), r_tr, 9, 12);
    eli_draw_list_path_arc_to_fast(list, eli_make_vec2(b.x - r_br, b.y - r_br), r_br, 0, 3);
    eli_draw_list_path_arc_to_fast(list, eli_make_vec2(a.x + r_bl, b.y - r_bl), r_bl, 3, 6);
}

#endif /* ELI_DRAW_ELI_DRAW_PATH_H */
