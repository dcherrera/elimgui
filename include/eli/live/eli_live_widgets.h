/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_live_widgets.h
 * @brief Live widget calls: dot, spinner, progress and text, laid out at the
 *        cursor or placed at a rect, plus the main-surface drawing used by the
 *        fallback and while a layer is occluded.
 *
 * @status Live widgets core.
 * @issues None
 * @todo None
 *
 * Each call submits the widget for this frame (eli_live_pool.h). With a backend
 * layer it draws nothing on the main surface and asks for no frame; the values
 * passed are authoritative (a setter's change lasts until the next submit). In
 * the fallback the widget draws itself and, when it animates, asks for a frame
 * every eli_live_get_fallback_interval() seconds.
 */
#ifndef ELI_LIVE_WIDGETS_H
#define ELI_LIVE_WIDGETS_H

#include "eli_live_types.h"
#include "eli_live_pool.h"
#include "eli_live_text.h"
#include "../core/eli_context.h"
#include "../draw/eli_draw.h"
#include "../font/eli_font.h"
#include "../style/eli_style.h"
#include "../window/eli_window.h"
#include "../layout/eli_layout.h"

/* ---------------------------------------------------------------------------
 * Animation curves (shared with backends that animate on the client)
 * ------------------------------------------------------------------------- */

/**
 * @return the pulse's opacity factor at `t` seconds (1 when the pulse is solid).
 *
 * Thread-safe: yes  Reentrant: yes
 */
static inline float eli_live_pulse_alpha(eli_live_pulse pulse, double t)
{
    if (pulse.period <= 0.0f)
        return 1.0f;
    double phase = fmod(t, (double)pulse.period) / (double)pulse.period;
    float c = 0.5f + 0.5f * cosf((float)(2.0 * ELI_PI * phase));
    return pulse.min_alpha + (1.0f - pulse.min_alpha) * c;
}

/** @return the spinner's rotation in radians at `t` seconds. */
static inline float eli_live_spinner_angle(double t)
{
    return (float)(2.0 * ELI_PI * fmod(t, (double)ELI_LIVE_SPINNER_PERIOD) /
                   (double)ELI_LIVE_SPINNER_PERIOD);
}

/* ---------------------------------------------------------------------------
 * Main-surface drawing (fallback / occluded)
 * ------------------------------------------------------------------------- */

/** @return `col` with its alpha multiplied by `k` (0..1). */
static inline eli_col32 eli_live__fade(eli_col32 col, float k)
{
    uint32_t a = (col & ELI_COL32_A_MASK) >> ELI_COL32_A_SHIFT;
    a = (uint32_t)((float)a * k + 0.5f);
    return (col & ~ELI_COL32_A_MASK) | (a << ELI_COL32_A_SHIFT);
}

/**
 * Draw `w` into `dl` as of time `t`; animate false draws the resting state (full
 * opacity, spinner at 0).
 */
static inline void eli_live__draw_main(eli_draw_list *dl, const eli_live_widget *w, bool animate,
                                       double t)
{
    eli_vec2 mn = eli_make_vec2(w->rect.x, w->rect.y);
    eli_vec2 mx = eli_make_vec2(w->rect.x + w->rect.w, w->rect.y + w->rect.h);
    eli_vec2 c = eli_make_vec2(w->rect.x + w->rect.w * 0.5f, w->rect.y + w->rect.h * 0.5f);
    float r = (w->rect.w < w->rect.h ? w->rect.w : w->rect.h) * 0.5f;
    switch (w->kind) {
    case ELI_LIVE_DOT:
        eli_draw_list_add_circle_filled(dl, c, r,
                                        eli_live__fade(w->col, animate
                                                           ? eli_live_pulse_alpha(w->pulse, t)
                                                           : 1.0f), 0);
        break;
    case ELI_LIVE_SPINNER: {
        float th = r * 0.25f;
        float a0 = animate ? eli_live_spinner_angle(t) : 0.0f;
        eli_draw_list_path_clear(dl);
        eli_draw_list_path_arc_to(dl, c, r - th * 0.5f, a0, a0 + 1.5f * ELI_PI, 24);
        eli_draw_list_path_stroke(dl, w->col, ELI_DRAW_NONE, th);
        break;
    }
    case ELI_LIVE_PROGRESS:
        eli_draw_list_add_rect_filled(dl, mn, mx, w->track_col, w->rounding,
                                      ELI_DRAW_ROUND_CORNERS_ALL);
        if (w->fraction > 0.0f)
            eli_draw_list_add_rect_filled(dl, mn,
                                          eli_make_vec2(mn.x + w->rect.w * w->fraction, mx.y),
                                          w->col, w->rounding, ELI_DRAW_ROUND_CORNERS_ALL);
        break;
    case ELI_LIVE_TEXT: {
        eli_vec4 clip = eli_make_vec4(mn.x, mn.y, mx.x, mx.y);
        if (w->font)
            eli_draw_list_add_text_ex(dl, w->font, w->font_size, mn, w->col, w->text, NULL, 0.0f,
                                      &clip);
        break;
    }
    default:
        break;
    }
}

/* ---------------------------------------------------------------------------
 * Submit
 * ------------------------------------------------------------------------- */

/** Copy `spec`'s properties into the slot, marking what changed. */
static inline void eli_live__apply(eli_live_widget *w, const eli_live_widget *spec)
{
    if (memcmp(&w->rect, &spec->rect, sizeof(w->rect)) != 0 ||
        memcmp(&w->clip, &spec->clip, sizeof(w->clip)) != 0) {
        w->rect = spec->rect;
        w->clip = spec->clip;
        w->pending |= ELI_LIVE_CHANGED_GEOMETRY;
    }
    if (w->col != spec->col || w->track_col != spec->track_col ||
        w->pulse.period != spec->pulse.period || w->pulse.min_alpha != spec->pulse.min_alpha ||
        w->rounding != spec->rounding) {
        w->col = spec->col;
        w->track_col = spec->track_col;
        w->pulse = spec->pulse;
        w->rounding = spec->rounding;
        w->pending |= ELI_LIVE_CHANGED_STYLE;
    }
    if (w->fraction != spec->fraction) {
        w->fraction = spec->fraction;
        w->pending |= ELI_LIVE_CHANGED_VALUE;
    }
    bool text_changed = eli_live__copy_text(w->text, spec->text);
    if (text_changed || w->font != spec->font || w->font_size != spec->font_size) {
        w->font = spec->font;
        w->font_size = spec->font_size;
        w->pending |= ELI_LIVE_CHANGED_TEXT;
    }
    w->list = spec->list;
}

/**
 * Submit a filled spec (kind, id, rect and properties) for this frame in the
 * current window: clip and draw list come from the window. Draws on the main
 * surface when the widget is not a shown layer.
 */
static inline void eli_live__submit(eli_live_widget *spec)
{
    eli_window *win = eli_get_current_window();
    if (!win || win->skip_items)
        return;
    eli_draw_list *dl = &win->draw_list;
    eli_vec2 cmn = eli_draw_list_get_clip_rect_min(dl);
    eli_vec2 cmx = eli_draw_list_get_clip_rect_max(dl);
    spec->clip = eli_make_rect(cmn.x, cmn.y, cmx.x - cmn.x, cmx.y - cmn.y);
    spec->list = dl;

    bool is_new = false;
    eli_live_widget *w = eli_live__acquire(spec->id, spec->kind, &is_new);
    if (w) {
        eli_live__apply(w, spec);
        eli_live__commit(w, is_new,
                         spec->kind != ELI_LIVE_TEXT || eli_live_text_can_raster(spec->font));
    }
    if (w && w->live) {
        if (w->main_static)
            eli_live__draw_main(dl, w, false, 0.0);
        return;
    }
    /* Fallback (no layer, or pool full): draw here; animate with a slow redraw. */
    bool animates = (spec->kind == ELI_LIVE_DOT && spec->pulse.period > 0.0f) ||
                    spec->kind == ELI_LIVE_SPINNER;
    float interval = eli_live_get_fallback_interval();
    bool animate = animates && interval > 0.0f;
    eli_context *ctx = eli_get_current_context();
    eli_live__draw_main(dl, w ? w : spec, animate, ctx ? ctx->time : 0.0);
    if (animate)
        eli_request_redraw_after(interval);
}

/** Default text color, font and size from the current style/font into `spec`. */
static inline void eli_live__text_spec(eli_live_widget *spec, eli_id id, eli_rect rect,
                                       const char *text)
{
    memset(spec, 0, sizeof(*spec));
    spec->kind = ELI_LIVE_TEXT;
    spec->id = id;
    spec->rect = rect;
    spec->col = eli_get_color_u32(ELI_COL_TEXT, 1.0f);
    spec->font = eli_get_font();
    spec->font_size = eli_get_font_size();
    eli_live__copy_text(spec->text, text);
}

/* ---------------------------------------------------------------------------
 * Placed widgets (a rect in screen points, current window)
 * ------------------------------------------------------------------------- */

/**
 * A live dot filling `rect` (a circle of min(w, h) / 2), optionally pulsing.
 *
 * @param id     Widget id (e.g. eli_get_id / eli_get_id_int); unique per program.
 * @param rect   Screen points inside the current window.
 * @param col    Color.
 * @param pulse  Opacity pulse, or NULL for a solid dot.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_live_dot_at(eli_id id, eli_rect rect, eli_col32 col,
                                   const eli_live_pulse *pulse)
{
    eli_live_widget spec;
    memset(&spec, 0, sizeof(spec));
    spec.kind = ELI_LIVE_DOT;
    spec.id = id;
    spec.rect = rect;
    spec.col = col;
    if (pulse)
        spec.pulse = *pulse;
    eli_live__submit(&spec);
}

/**
 * A live spinner (270-degree arc, one turn per ELI_LIVE_SPINNER_PERIOD) in `rect`.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_live_spinner_at(eli_id id, eli_rect rect, eli_col32 col)
{
    eli_live_widget spec;
    memset(&spec, 0, sizeof(spec));
    spec.kind = ELI_LIVE_SPINNER;
    spec.id = id;
    spec.rect = rect;
    spec.col = col;
    eli_live__submit(&spec);
}

/**
 * A live progress bar in `rect`: ELI_COL_FRAME_BG track, ELI_COL_PLOT_HISTOGRAM
 * fill of `fraction` (clamped to 0..1), style frame_rounding.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_live_progress_at(eli_id id, eli_rect rect, float fraction)
{
    eli_context *ctx = eli_get_current_context();
    eli_live_widget spec;
    memset(&spec, 0, sizeof(spec));
    spec.kind = ELI_LIVE_PROGRESS;
    spec.id = id;
    spec.rect = rect;
    spec.col = eli_get_color_u32(ELI_COL_PLOT_HISTOGRAM, 1.0f);
    spec.track_col = eli_get_color_u32(ELI_COL_FRAME_BG, 1.0f);
    spec.rounding = ctx ? ctx->style.frame_rounding : 0.0f;
    spec.fraction = fraction < 0.0f ? 0.0f : (fraction > 1.0f ? 1.0f : fraction);
    eli_live__submit(&spec);
}

/**
 * One line of live text in `rect` (ELI_COL_TEXT, current font and size), cut at
 * the rect and at ELI_LIVE_TEXT_MAX - 1 bytes.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_live_text_at(eli_id id, eli_rect rect, const char *text)
{
    eli_live_widget spec;
    eli_live__text_spec(&spec, id, rect, text);
    eli_live__submit(&spec);
}

/* ---------------------------------------------------------------------------
 * Laid-out widgets (at the cursor, like any widget)
 * ------------------------------------------------------------------------- */

/** Lay out an item of `size` at the cursor. @return false when it is clipped out. */
static inline bool eli_live__layout(eli_id id, eli_vec2 size, float baseline, eli_rect *bb)
{
    eli_window *win = eli_get_current_window();
    if (!win || win->skip_items)
        return false;
    *bb = eli_make_rect(win->cursor_pos.x, win->cursor_pos.y, size.x, size.y);
    eli_item_size(size, baseline);
    return eli_item_add(id, *bb, 0);
}

/**
 * A live dot of `radius` at the cursor.
 *
 * @return the widget id (for eli_live_set_color), 0 outside a window.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline eli_id eli_live_dot(const char *str_id, float radius, eli_col32 col,
                                  const eli_live_pulse *pulse)
{
    eli_id id = eli_get_id(str_id);
    eli_rect bb;
    if (eli_live__layout(id, eli_make_vec2(radius * 2.0f, radius * 2.0f), -1.0f, &bb))
        eli_live_dot_at(id, bb, col, pulse);
    return eli_get_current_window() ? id : 0u;
}

/**
 * A live spinner of `radius` at the cursor.
 *
 * @return the widget id, 0 outside a window.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline eli_id eli_live_spinner(const char *str_id, float radius, eli_col32 col)
{
    eli_id id = eli_get_id(str_id);
    eli_rect bb;
    if (eli_live__layout(id, eli_make_vec2(radius * 2.0f, radius * 2.0f), -1.0f, &bb))
        eli_live_spinner_at(id, bb, col);
    return eli_get_current_window() ? id : 0u;
}

/**
 * A live progress bar at the cursor. `size` 0 = item width x frame height.
 *
 * @return the widget id (for eli_live_set_fraction), 0 outside a window.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline eli_id eli_live_progress(const char *str_id, eli_vec2 size, float fraction)
{
    eli_context *ctx = eli_get_current_context();
    eli_id id = eli_get_id(str_id);
    eli_rect bb;
    eli_vec2 sz = eli_calc_item_size(size, eli_calc_item_width(), eli_get_frame_height());
    float baseline = ctx ? ctx->style.frame_padding.y : -1.0f;
    if (eli_live__layout(id, sz, baseline, &bb))
        eli_live_progress_at(id, bb, fraction);
    return eli_get_current_window() ? id : 0u;
}

/**
 * One line of live text at the cursor, `width` points wide (<= 0: the width of
 * `text` now; give a fixed width when the text changes length, e.g. a clock).
 *
 * @return the widget id (for eli_live_set_text), 0 outside a window.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline eli_id eli_live_text(const char *str_id, const char *text, float width)
{
    eli_id id = eli_get_id(str_id);
    eli_rect bb;
    if (width <= 0.0f)
        width = eli_calc_text_size(text ? text : "", NULL).x;
    if (eli_live__layout(id, eli_make_vec2(width, eli_get_font_size()), 0.0f, &bb))
        eli_live_text_at(id, bb, text);
    return eli_get_current_window() ? id : 0u;
}

#endif /* ELI_LIVE_WIDGETS_H */
