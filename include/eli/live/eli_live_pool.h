/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_live_pool.h
 * @brief Live widget pool and lifecycle: the backend, the fixed pool, removal of
 *        widgets not submitted, occlusion, change delivery and the property
 *        setters.
 *
 * @status Live widgets core.
 * @issues One pool per program: live widgets assume a single elimgui context.
 * @todo None
 *
 * The pool is ELI_LIVE_MAX records in static storage; nothing here touches the
 * heap. eli_frame_end calls eli_live_end_frame right after eli_window_render (the
 * render order is needed for occlusion). Lifetime counts built frames only.
 */
#ifndef ELI_LIVE_POOL_H
#define ELI_LIVE_POOL_H

#include "eli_live_types.h"
#include "../core/eli_context.h"
#include "../core/eli_redraw.h"
#include "../draw/eli_draw_types.h"

ELI_GLOBAL_ARRAY(eli_live_widget, g_eli_live, ELI_LIVE_MAX);
ELI_GLOBAL(eli_live_backend, g_eli_live_backend, {0});
ELI_GLOBAL(eli_live_stats, g_eli_live_stats, {0});
ELI_GLOBAL(float, g_eli_live_fallback_interval, ELI_LIVE_FALLBACK_INTERVAL);

/** Every change bit (a new widget owes all of them). */
#define ELI_LIVE__CHANGED_ALL                                                       \
    (ELI_LIVE_CHANGED_GEOMETRY | ELI_LIVE_CHANGED_VISIBLE | ELI_LIVE_CHANGED_STYLE | \
     ELI_LIVE_CHANGED_VALUE | ELI_LIVE_CHANGED_TEXT)

/* ---------------------------------------------------------------------------
 * Lookup and removal
 * ------------------------------------------------------------------------- */

/**
 * @return the live widget with `id`, or NULL.
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline eli_live_widget *eli_live_find(eli_id id)
{
    for (int i = 0; i < ELI_LIVE_MAX; i++) {
        if (g_eli_live[i].used && g_eli_live[i].id == id)
            return &g_eli_live[i];
    }
    return NULL;
}

/** Destroy the widget's layer (if any) and free its slot. */
static inline void eli_live__remove(eli_live_widget *w)
{
    if (w->live && g_eli_live_backend.destroy) {
        g_eli_live_backend.destroy(g_eli_live_backend.user_data, w);
        g_eli_live_stats.destroys++;
    }
    memset(w, 0, sizeof(*w));
}

/**
 * Remove every live widget (their layers go). eli_destroy_context calls it
 * through the widget shutdown hook. Counters restart.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_live_shutdown(void)
{
    for (int i = 0; i < ELI_LIVE_MAX; i++) {
        if (g_eli_live[i].used)
            eli_live__remove(&g_eli_live[i]);
    }
    memset(&g_eli_live_stats, 0, sizeof(g_eli_live_stats));
}

/* ---------------------------------------------------------------------------
 * Setup
 * ------------------------------------------------------------------------- */

/**
 * Install the platform backend (copied), or NULL to draw every live widget with
 * the fallback. Existing widgets are removed (through the old backend) and come
 * back on their next submit.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_live_set_backend(const eli_live_backend *backend)
{
    for (int i = 0; i < ELI_LIVE_MAX; i++) {
        if (g_eli_live[i].used)
            eli_live__remove(&g_eli_live[i]);
    }
    if (backend)
        g_eli_live_backend = *backend;
    else
        memset(&g_eli_live_backend, 0, sizeof(g_eli_live_backend));
}

/** @return true when a backend that can draw `kind` is installed. */
static inline bool eli_live_backend_has(eli_live_kind kind)
{
    return g_eli_live_backend.create != NULL &&
           (g_eli_live_backend.kinds & ELI_LIVE_KIND_BIT(kind)) != 0;
}

/**
 * Slow-redraw interval of an animating fallback widget, in seconds (default
 * ELI_LIVE_FALLBACK_INTERVAL; 0 = draw it static, no timed redraw).
 */
static inline void eli_live_set_fallback_interval(float seconds)
{
    g_eli_live_fallback_interval = seconds > 0.0f ? seconds : 0.0f;
}

/** @return the fallback slow-redraw interval in seconds. */
static inline float eli_live_get_fallback_interval(void)
{
    return g_eli_live_fallback_interval;
}

/** @return the pool counters (count and live are recomputed on each call). */
static inline const eli_live_stats *eli_live_get_stats(void)
{
    int count = 0, live = 0;
    for (int i = 0; i < ELI_LIVE_MAX; i++) {
        count += g_eli_live[i].used ? 1 : 0;
        live += g_eli_live[i].live ? 1 : 0;
    }
    g_eli_live_stats.count = count;
    g_eli_live_stats.live = live;
    return &g_eli_live_stats;
}

/* ---------------------------------------------------------------------------
 * Submit (used by eli_live_widgets.h)
 * ------------------------------------------------------------------------- */

/**
 * Find or allocate the slot for `id` this frame. A kind change replaces the old
 * widget.
 *
 * @param is_new  Set when the slot was allocated now (fill it, then eli_live__commit).
 * @return        the slot, or NULL when the pool is full (counted as overflow).
 */
static inline eli_live_widget *eli_live__acquire(eli_id id, eli_live_kind kind, bool *is_new)
{
    eli_context *ctx = eli_get_current_context();
    eli_live_widget *w = eli_live_find(id);
    *is_new = false;
    if (w && w->kind != kind) {
        eli_live__remove(w);
        w = NULL;
    }
    if (!w) {
        for (int i = 0; i < ELI_LIVE_MAX && !w; i++) {
            if (!g_eli_live[i].used)
                w = &g_eli_live[i];
        }
        if (!w) {
            g_eli_live_stats.overflow++;
            return NULL;
        }
        memset(w, 0, sizeof(*w));
        w->used = true;
        w->id = id;
        w->kind = kind;
        w->pending = ELI_LIVE__CHANGED_ALL;
        *is_new = true;
    }
    w->seen_frame = ctx ? ctx->frame_count : 0;
    return w;
}

/**
 * Hand a newly filled widget to the backend. `can_layer` is false when the kind
 * cannot be drawn by a layer here (e.g. text without CPU atlas pixels).
 */
static inline void eli_live__commit(eli_live_widget *w, bool is_new, bool can_layer)
{
    if (!is_new)
        return;
    if (can_layer && eli_live_backend_has(w->kind)) {
        w->live = g_eli_live_backend.create(g_eli_live_backend.user_data, w);
        g_eli_live_stats.creates++;
    }
    w->pending = 0;   /* create saw everything; visibility comes at frame end */
}

/**
 * Deliver change bits from a setter: inside a frame they wait for frame end;
 * between frames a layer is updated now. When the main surface draws the widget
 * (fallback, or static while occluded) a redraw is requested instead.
 */
static inline void eli_live__changed(eli_live_widget *w, unsigned bits)
{
    eli_context *ctx = eli_get_current_context();
    if (!w->live || w->main_static)
        eli_request_redraw();
    if (!w->live)
        return;
    if (ctx && ctx->within_frame_scope) {
        w->pending |= bits;
    } else if (g_eli_live_backend.update) {
        g_eli_live_backend.update(g_eli_live_backend.user_data, w, bits);
        g_eli_live_stats.updates++;
    }
}

/* ---------------------------------------------------------------------------
 * Frame end: removal, occlusion, delivery
 * ------------------------------------------------------------------------- */

/** @return a ∩ b (w or h <= 0 when they do not overlap). */
static inline eli_rect eli_live__intersect(eli_rect a, eli_rect b)
{
    float x0 = a.x > b.x ? a.x : b.x;
    float y0 = a.y > b.y ? a.y : b.y;
    float x1 = (a.x + a.w) < (b.x + b.w) ? (a.x + a.w) : (b.x + b.w);
    float y1 = (a.y + a.h) < (b.y + b.h) ? (a.y + a.h) : (b.y + b.h);
    return eli_make_rect(x0, y0, x1 - x0, y1 - y0);
}

/** @return true when `r` has area. */
static inline bool eli_live__has_area(eli_rect r)
{
    return r.w > 0.0f && r.h > 0.0f;
}

/**
 * What a draw list can cover: its vertex bounding box within the union of its
 * commands' clip rects.
 *
 * @return false when the list draws nothing.
 */
static inline bool eli_live__list_bounds(const eli_draw_list *l, eli_rect *out)
{
    if (l->vtx_count == 0 || l->idx_count == 0)
        return false;
    float x0 = l->vtx[0].x, y0 = l->vtx[0].y, x1 = x0, y1 = y0;
    for (uint32_t i = 1; i < l->vtx_count; i++) {
        const eli_draw_vert *v = &l->vtx[i];
        x0 = v->x < x0 ? v->x : x0;
        y0 = v->y < y0 ? v->y : y0;
        x1 = v->x > x1 ? v->x : x1;
        y1 = v->y > y1 ? v->y : y1;
    }
    bool any = false;
    float cx0 = 0.0f, cy0 = 0.0f, cx1 = 0.0f, cy1 = 0.0f;
    for (uint32_t i = 0; i < l->cmd_count; i++) {
        const eli_draw_cmd *c = &l->cmds[i];
        if (c->elem_count == 0 && c->user_callback == NULL)
            continue;
        float a = c->clip_rect.x, b = c->clip_rect.y;
        float e = a + c->clip_rect.w, f = b + c->clip_rect.h;
        cx0 = (!any || a < cx0) ? a : cx0;
        cy0 = (!any || b < cy0) ? b : cy0;
        cx1 = (!any || e > cx1) ? e : cx1;
        cy1 = (!any || f > cy1) ? f : cy1;
        any = true;
    }
    if (!any)
        return false;
    *out = eli_live__intersect(eli_make_rect(x0, y0, x1 - x0, y1 - y0),
                               eli_make_rect(cx0, cy0, cx1 - cx0, cy1 - cy0));
    return eli_live__has_area(*out);
}

/** @return the index of `list` in this frame's render order, or -1. */
static inline int eli_live__render_index(const eli_context *ctx, const eli_draw_list *list)
{
    for (int i = 0; i < ctx->render_draw_lists_count; i++) {
        if (ctx->render_draw_lists[i] == list)
            return i;
    }
    return -1;
}

/**
 * Per-frame live widget pass. eli_frame_end calls it after eli_window_render:
 *   1. widgets not submitted this frame are removed;
 *   2. each widget is visible when its clipped rect has area, its window was drawn,
 *      and no later draw list's bounds overlap it (occluded otherwise);
 *   3. an occluded layer is hidden and the main surface draws the widget static
 *      from the next frame; the layer shows again only after a frame drawn
 *      without the static copy (a change requests that frame);
 *   4. pending change bits go to the backend in one update per widget.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_live_end_frame(void)
{
    eli_context *ctx = eli_get_current_context();
    if (!ctx)
        return;
    bool vis[ELI_LIVE_MAX];
    bool occluded[ELI_LIVE_MAX];
    int order[ELI_LIVE_MAX];
    int first = ctx->render_draw_lists_count;

    for (int i = 0; i < ELI_LIVE_MAX; i++) {
        eli_live_widget *w = &g_eli_live[i];
        vis[i] = occluded[i] = false;
        order[i] = -1;
        if (!w->used)
            continue;
        if (w->seen_frame != ctx->frame_count) {
            eli_live__remove(w);
            continue;
        }
        order[i] = eli_live__render_index(ctx, w->list);
        vis[i] = order[i] >= 0 && eli_live__has_area(eli_live__intersect(w->rect, w->clip));
        if (vis[i] && order[i] < first)
            first = order[i];
    }

    for (int li = ctx->render_draw_lists_count - 1; li > first; li--) {
        eli_rect cover;
        if (!eli_live__list_bounds(ctx->render_draw_lists[li], &cover))
            continue;
        for (int i = 0; i < ELI_LIVE_MAX; i++) {
            if (vis[i] && order[i] < li &&
                eli_live__has_area(eli_live__intersect(
                    eli_live__intersect(g_eli_live[i].rect, g_eli_live[i].clip), cover))) {
                vis[i] = false;
                occluded[i] = true;
            }
        }
    }

    for (int i = 0; i < ELI_LIVE_MAX; i++) {
        eli_live_widget *w = &g_eli_live[i];
        if (!w->used)
            continue;
        bool drew_static = w->main_static;
        bool want_static = w->live && occluded[i];
        bool shown = w->live && vis[i] && !drew_static;
        if (want_static != drew_static) {
            w->main_static = want_static;
            eli_request_redraw();
        }
        if (shown != w->visible) {
            w->visible = shown;
            w->pending |= ELI_LIVE_CHANGED_VISIBLE;
        }
        if (w->live && w->pending && g_eli_live_backend.update) {
            g_eli_live_backend.update(g_eli_live_backend.user_data, w, w->pending);
            g_eli_live_stats.updates++;
        }
        w->pending = 0;
    }
}

/* ---------------------------------------------------------------------------
 * Property setters (inside or between frames)
 * ------------------------------------------------------------------------- */

/**
 * Set a live progress bar's fraction (clamped to 0..1). With a layer this
 * updates the layer and needs no frame.
 *
 * @return false when no live progress bar has `id`.
 *
 * Thread-safe: no (main thread)  Reentrant: no
 */
static inline bool eli_live_set_fraction(eli_id id, float fraction)
{
    eli_live_widget *w = eli_live_find(id);
    if (!w || w->kind != ELI_LIVE_PROGRESS)
        return false;
    fraction = fraction < 0.0f ? 0.0f : (fraction > 1.0f ? 1.0f : fraction);
    if (fraction != w->fraction) {
        w->fraction = fraction;
        eli_live__changed(w, ELI_LIVE_CHANGED_VALUE);
    }
    return true;
}

/**
 * Copy `text` (at most ELI_LIVE_TEXT_MAX - 1 bytes, cut at a UTF-8 boundary)
 * into `dst`.
 *
 * @return true when `dst` changed.
 */
static inline bool eli_live__copy_text(char *dst, const char *text)
{
    size_t n = 0;
    if (text) {
        while (n < ELI_LIVE_TEXT_MAX - 1 && text[n] != '\0')
            n++;
        if (text[n] != '\0') {   /* cut: back off a partial UTF-8 sequence */
            while (n > 0 && ((unsigned char)text[n] & 0xC0u) == 0x80u)
                n--;
        }
    }
    if (strncmp(dst, text ? text : "", n) == 0 && dst[n] == '\0')
        return false;
    if (n)
        memcpy(dst, text, n);
    dst[n] = '\0';
    return true;
}

/**
 * Set a live text widget's text. With a layer this re-rasterizes the layer only.
 *
 * @return false when no live text widget has `id`.
 *
 * Thread-safe: no (main thread)  Reentrant: no
 */
static inline bool eli_live_set_text(eli_id id, const char *text)
{
    eli_live_widget *w = eli_live_find(id);
    if (!w || w->kind != ELI_LIVE_TEXT)
        return false;
    if (eli_live__copy_text(w->text, text))
        eli_live__changed(w, ELI_LIVE_CHANGED_TEXT);
    return true;
}

/**
 * Set a live widget's main color (dot, arc, fill or text).
 *
 * @return false when no live widget has `id`.
 *
 * Thread-safe: no (main thread)  Reentrant: no
 */
static inline bool eli_live_set_color(eli_id id, eli_col32 col)
{
    eli_live_widget *w = eli_live_find(id);
    if (!w)
        return false;
    if (col != w->col) {
        w->col = col;
        eli_live__changed(w, ELI_LIVE_CHANGED_STYLE);
    }
    return true;
}

#endif /* ELI_LIVE_POOL_H */
