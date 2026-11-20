/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_draw_dash.h
 * @brief Dashed strokes: stroke the current path as dashes, and dashed rect /
 *        circle outlines built on it.
 *
 * The dash pattern runs along the path's arc length, so its phase carries on
 * around corners: a dash that reaches a corner bends round it and keeps its
 * length. Each dash is emitted as one polyline (four vertices per segment, as
 * eli_draw_list_add_polyline). A closed path stretches the pattern so a whole
 * number of periods fits its perimeter, which leaves no partial dash at the
 * seam. Dash points are staged in the draw list's own path buffer, after the
 * path being stroked: no allocation beyond that buffer's growth.
 *
 * @status In use (Argus Table: ghost cards, the hill suggestion ring).
 * @issues None
 * @todo None
 */
#ifndef ELI_DRAW_ELI_DRAW_DASH_H
#define ELI_DRAW_ELI_DRAW_DASH_H

#include "eli_draw_types.h"
#include "eli_draw_list.h"
#include "eli_draw_path.h"
#include "eli_draw_prim.h"

#include "../core/eli_platform.h"
#include "../core/eli_types.h"

/** Length of segment i of the path (i+1 wraps to 0 on the closing segment). */
static inline float eli_draw__dash_seg_len(const eli_draw_list *list, int n, int i)
{
    eli_vec2 a = list->path[i];
    eli_vec2 b = list->path[(i + 1) % n];
    float dx = b.x - a.x, dy = b.y - a.y;
    return sqrtf(dx * dx + dy * dy);
}

/** Emit the staged dash (path points [base, path_count)) and drop it from the buffer. */
static inline void eli_draw__dash_flush(eli_draw_list *list, int base, eli_col32 col,
                                        float thickness)
{
    if (list->path_count - base >= 2)
        eli_draw_list_add_polyline(list, list->path + base, list->path_count - base, col, 0,
                                   thickness);
    list->path_count = base;
}

/**
 * Stroke the current path as dashes and clear it. The pattern starts with a
 * dash at the first point and runs along the arc length, continuous across
 * corners. With ELI_DRAW_CLOSED the closing segment is dashed too and the
 * pattern is stretched (by under half a period overall) so the perimeter holds a
 * whole number of periods. `dash <= 0` or `gap <= 0` strokes the path solid.
 *
 * @param list       Target draw list.
 * @param col        Line color (fully transparent clears the path, draws nothing).
 * @param flags      ELI_DRAW_CLOSED to connect the last point to the first.
 * @param thickness  Line thickness in pixels.
 * @param dash       Dash length in pixels.
 * @param gap        Gap length in pixels.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_path_stroke_dashed(eli_draw_list *list, eli_col32 col,
                                                    eli_draw_flags flags, float thickness,
                                                    float dash, float gap)
{
    int n = list->path_count;
    if (n < 2 || (col & ELI_COL32_A_MASK) == 0) {
        list->path_count = 0;
        return;
    }
    if (dash <= 0.0f || gap <= 0.0f) {
        eli_draw_list_path_stroke(list, col, flags, thickness);
        return;
    }
    bool closed = (flags & ELI_DRAW_CLOSED) != 0;
    int segs = closed ? n : n - 1;
    if (closed) {
        float perimeter = 0.0f;
        for (int i = 0; i < segs; i++)
            perimeter += eli_draw__dash_seg_len(list, n, i);
        float periods = floorf(perimeter / (dash + gap) + 0.5f);
        if (periods < 1.0f)
            periods = 1.0f;
        float stretch = perimeter / (periods * (dash + gap));
        dash *= stretch;
        gap *= stretch;
    }
    float eps = (dash + gap) * 1e-4f;

    /* Dash points go after the n path points; the buffer may move as it grows, so
     * the path is read by index on every step. */
    int base = n;
    bool on = true;
    float done = 0.0f;   /* length already covered in the current dash or gap */
    for (int i = 0; i < segs; i++) {
        float len = eli_draw__dash_seg_len(list, n, i);
        if (len <= 0.0f)
            continue;
        float t = 0.0f;
        while (len - t > eps) {
            float span = on ? dash : gap;
            float step = eli_min_f(span - done, len - t);
            if (on) {
                eli_vec2 a = list->path[i];
                eli_vec2 b = list->path[(i + 1) % n];
                float dx = (b.x - a.x) / len, dy = (b.y - a.y) / len;
                if (list->path_count == base)
                    eli_draw_path_push(list, eli_make_vec2(a.x + dx * t, a.y + dy * t));
                eli_draw_path_push(list, eli_make_vec2(a.x + dx * (t + step),
                                                       a.y + dy * (t + step)));
            }
            t += step;
            done += step;
            if (span - done <= eps) {
                if (on)
                    eli_draw__dash_flush(list, base, col, thickness);
                on = !on;
                done = 0.0f;
            }
        }
    }
    if (on)
        eli_draw__dash_flush(list, base, col, thickness);
    list->path_count = 0;
}

/**
 * Draw a dashed (optionally rounded) rectangle outline. Same geometry as
 * eli_draw_list_add_rect, stroked with eli_draw_list_path_stroke_dashed.
 *
 * @param list       Target draw list.
 * @param p_min      Top-left corner.
 * @param p_max      Bottom-right corner.
 * @param col        Line color.
 * @param rounding   Corner radius in pixels.
 * @param flags      Corner-selection flags (see eli_draw_flags).
 * @param thickness  Line thickness in pixels.
 * @param dash       Dash length in pixels.
 * @param gap        Gap length in pixels.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_add_rect_dashed(eli_draw_list *list, eli_vec2 p_min,
                                                 eli_vec2 p_max, eli_col32 col, float rounding,
                                                 eli_draw_flags flags, float thickness,
                                                 float dash, float gap)
{
    if ((col & ELI_COL32_A_MASK) == 0)
        return;
    eli_draw_list_path_rect(list, eli_make_vec2(p_min.x + 0.5f, p_min.y + 0.5f),
                            eli_make_vec2(p_max.x - 0.49f, p_max.y - 0.49f), rounding, flags);
    eli_draw_list_path_stroke_dashed(list, col, ELI_DRAW_CLOSED, thickness, dash, gap);
}

/**
 * Draw a dashed circle outline. Same geometry as eli_draw_list_add_circle,
 * stroked with eli_draw_list_path_stroke_dashed.
 *
 * @param list          Target draw list.
 * @param center        Circle center.
 * @param radius        Circle radius.
 * @param col           Line color.
 * @param num_segments  Segment count, or <=0 for automatic.
 * @param thickness     Line thickness in pixels.
 * @param dash          Dash length in pixels.
 * @param gap           Gap length in pixels.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_add_circle_dashed(eli_draw_list *list, eli_vec2 center,
                                                   float radius, eli_col32 col, int num_segments,
                                                   float thickness, float dash, float gap)
{
    if ((col & ELI_COL32_A_MASK) == 0 || radius < 0.5f)
        return;
    if (num_segments <= 0)
        num_segments = eli_draw_circle_auto_segments(radius, list->circle_segment_max_error);
    num_segments = eli_draw_clamp_circle_segments(num_segments);
    float a_max = (2.0f * ELI_PI) * ((float)num_segments - 1.0f) / (float)num_segments;
    eli_draw_list_path_arc_to(list, center, radius - 0.5f, 0.0f, a_max, num_segments - 1);
    eli_draw_list_path_stroke_dashed(list, col, ELI_DRAW_CLOSED, thickness, dash, gap);
}

#endif /* ELI_DRAW_ELI_DRAW_DASH_H */
