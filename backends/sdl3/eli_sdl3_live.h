/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_sdl3_live.h
 * @brief Live widget backend for the SDL3 Metal renderer on macOS: each live
 *        widget is a small Core Animation layer tree on top of the renderer's
 *        CAMetalLayer, animated by the compositor (docs/live-widgets.md).
 *
 * @status In use (SDL3 example, macOS).
 * @issues Non-Apple builds compile to no-ops: init returns false and live widgets
 *         use the fallback. During a layout change a layer can lead the main
 *         surface by one presented frame (both are committed independently).
 * @todo Wayland subsurfaces and DirectComposition (docs/live-widgets.md).
 *
 * Header-only, `static inline`, plain C through the Objective-C runtime (like
 * eli_sdl3_metal.h); no .m file. Include after <eli/elimgui.h>.
 *
 *     eli_sdl3_live_init(renderer);            // after eli_sdl3_renderer_init*
 *     ... each SDL event: eli_sdl3_live_process_event(&e); ...
 *     eli_sdl3_live_shutdown();                // before eli_sdl3_renderer_shutdown
 *
 * Layers: one container (points, y down) added on top of the CAMetalLayer; per
 * widget a clip layer (masksToBounds, the widget's rect cut by its clip) holding
 *   - dot: a rounded CALayer with an `opacity` CABasicAnimation (autoreverse,
 *     sine ease, repeat forever) for the pulse;
 *   - spinner: a CAShapeLayer arc with a `transform.rotation.z` animation;
 *   - progress: a track layer and a fill sublayer;
 *   - text: a layer of the text color masked by an alpha8 image rastered from
 *     the atlas (eli_live_text_raster).
 * Changes run in a CATransaction with actions disabled. The CAMetalLayer itself
 * is never redrawn for an animation, so the idle swapchain release keeps working.
 * Layer records live in a static array (ELI_LIVE_MAX); nothing uses the elimgui
 * heap. Text bitmaps are made by CoreGraphics on a text change only.
 *
 * Linking (Apple): -lobjc -framework QuartzCore -framework CoreGraphics.
 */
#ifndef ELI_SDL3_LIVE_H
#define ELI_SDL3_LIVE_H

#include <SDL3/SDL.h>

#include <eli/elimgui.h>

#if defined(__APPLE__)
#include <CoreGraphics/CoreGraphics.h>
#include <objc/message.h>
#include <objc/runtime.h>

/* Exported by libobjc; not in a public header. */
void *objc_autoreleasePoolPush(void);
void objc_autoreleasePoolPop(void *pool);

/** Core Animation objects of one live widget. */
typedef struct eli_sdl3_live_layers {
    bool used;
    id clip;          /* masksToBounds; frame = rect cut by clip */
    id content;       /* dot, arc, track, or the colored text layer */
    id part;          /* progress fill or text mask */
    float raster_w;   /* text: size the mask was rastered at (points) */
    float raster_h;
} eli_sdl3_live_layers;

/** Backend state. */
typedef struct eli_sdl3_live_state {
    id metal;         /* the renderer's CAMetalLayer (not owned) */
    id container;     /* owned */
    float scale;      /* window pixel density */
    SDL_Window *window;
} eli_sdl3_live_state;

ELI_GLOBAL(eli_sdl3_live_state, g_eli_sdl3_live, {0});
ELI_GLOBAL_ARRAY(eli_sdl3_live_layers, g_eli_sdl3_live_layers, ELI_LIVE_MAX);

/* ---------------------------------------------------------------------------
 * Objective-C runtime helpers
 * ------------------------------------------------------------------------- */

static inline SEL eli_sdl3__sel(const char *name)
{
    return sel_registerName(name);
}

static inline id eli_sdl3__msg(id o, const char *sel)
{
    return ((id (*)(id, SEL))objc_msgSend)(o, eli_sdl3__sel(sel));
}

static inline void eli_sdl3__msg_p(id o, const char *sel, const void *arg)
{
    ((void (*)(id, SEL, const void *))objc_msgSend)(o, eli_sdl3__sel(sel), arg);
}

static inline void eli_sdl3__msg_pp(id o, const char *sel, const void *a, const void *b)
{
    ((void (*)(id, SEL, const void *, const void *))objc_msgSend)(o, eli_sdl3__sel(sel), a, b);
}

static inline void eli_sdl3__msg_b(id o, const char *sel, BOOL v)
{
    ((void (*)(id, SEL, BOOL))objc_msgSend)(o, eli_sdl3__sel(sel), v);
}

static inline void eli_sdl3__msg_f(id o, const char *sel, float v)
{
    ((void (*)(id, SEL, float))objc_msgSend)(o, eli_sdl3__sel(sel), v);
}

static inline void eli_sdl3__msg_d(id o, const char *sel, double v)
{
    ((void (*)(id, SEL, double))objc_msgSend)(o, eli_sdl3__sel(sel), v);
}

static inline void eli_sdl3__msg_u(id o, const char *sel, unsigned int v)
{
    ((void (*)(id, SEL, unsigned int))objc_msgSend)(o, eli_sdl3__sel(sel), v);
}

static inline void eli_sdl3__msg_rect(id o, const char *sel, CGRect r)
{
    ((void (*)(id, SEL, CGRect))objc_msgSend)(o, eli_sdl3__sel(sel), r);
}

static inline void eli_sdl3__msg_point(id o, const char *sel, CGPoint p)
{
    ((void (*)(id, SEL, CGPoint))objc_msgSend)(o, eli_sdl3__sel(sel), p);
}

static inline BOOL eli_sdl3__msg_ret_b(id o, const char *sel)
{
    return ((BOOL (*)(id, SEL))objc_msgSend)(o, eli_sdl3__sel(sel));
}

/** A CGRect-returning message (x86-64 returns large structs through _stret). */
static inline CGRect eli_sdl3__msg_ret_rect(id o, const char *sel)
{
#if defined(__x86_64__)
    return ((CGRect (*)(id, SEL))objc_msgSend_stret)(o, eli_sdl3__sel(sel));
#else
    return ((CGRect (*)(id, SEL))objc_msgSend)(o, eli_sdl3__sel(sel));
#endif
}

/** [[cls alloc] init] (+1, no autorelease). */
static inline id eli_sdl3__new(const char *cls)
{
    return eli_sdl3__msg(eli_sdl3__msg((id)objc_getClass(cls), "alloc"), "init");
}

/** [[NSString alloc] initWithUTF8String:] (+1). */
static inline id eli_sdl3__string(const char *s)
{
    id o = eli_sdl3__msg((id)objc_getClass("NSString"), "alloc");
    return ((id (*)(id, SEL, const char *))objc_msgSend)(o, eli_sdl3__sel("initWithUTF8String:"), s);
}

/** [[NSNumber alloc] initWithDouble:] (+1). */
static inline id eli_sdl3__number(double v)
{
    id o = eli_sdl3__msg((id)objc_getClass("NSNumber"), "alloc");
    return ((id (*)(id, SEL, double))objc_msgSend)(o, eli_sdl3__sel("initWithDouble:"), v);
}

static inline void eli_sdl3__release(id o)
{
    if (o)
        eli_sdl3__msg(o, "release");
}

/** Begin a transaction with implicit animations off. */
static inline void eli_sdl3__tx_begin(void)
{
    id tx = (id)objc_getClass("CATransaction");
    eli_sdl3__msg(tx, "begin");
    eli_sdl3__msg_b(tx, "setDisableActions:", YES);
}

static inline void eli_sdl3__tx_commit(void)
{
    eli_sdl3__msg((id)objc_getClass("CATransaction"), "commit");
}

/** Set a CGColor property from an eli_col32 (sRGB, like the renderer's layer). */
static inline void eli_sdl3__set_color(id layer, const char *sel, eli_col32 col)
{
    eli_vec4 c = eli_color_u32_to_vec4(col);
    CGColorRef cg = CGColorCreateSRGB(c.x, c.y, c.z, c.w);
    eli_sdl3__msg_p(layer, sel, cg);
    CGColorRelease(cg);
}

/** Set a string property ("lineCap", ...). */
static inline void eli_sdl3__set_string(id o, const char *sel, const char *value)
{
    id s = eli_sdl3__string(value);
    eli_sdl3__msg_p(o, sel, s);
    eli_sdl3__release(s);
}

/**
 * Add a repeat-forever animation of `key_path` from `from` to `to` over `duration`
 * seconds, under `key`. `autoreverse` plays it back; `sine` eases in and out on a
 * sine curve (else linear).
 */
static inline void eli_sdl3__animate(id layer, const char *key, const char *key_path, double from,
                                     double to, double duration, bool autoreverse, bool sine)
{
    id anim = eli_sdl3__new("CABasicAnimation");
    id path = eli_sdl3__string(key_path);
    id from_n = eli_sdl3__number(from);
    id to_n = eli_sdl3__number(to);
    eli_sdl3__msg_p(anim, "setKeyPath:", path);
    eli_sdl3__msg_p(anim, "setFromValue:", from_n);
    eli_sdl3__msg_p(anim, "setToValue:", to_n);
    eli_sdl3__msg_d(anim, "setDuration:", duration);
    eli_sdl3__msg_b(anim, "setAutoreverses:", autoreverse ? YES : NO);
    eli_sdl3__msg_f(anim, "setRepeatCount:", HUGE_VALF);
    if (sine) {
        id fn = eli_sdl3__msg((id)objc_getClass("CAMediaTimingFunction"), "alloc");
        fn = ((id (*)(id, SEL, float, float, float, float))objc_msgSend)(
            fn, eli_sdl3__sel("initWithControlPoints::::"), 0.37f, 0.0f, 0.63f, 1.0f);
        eli_sdl3__msg_p(anim, "setTimingFunction:", fn);
        eli_sdl3__release(fn);
    }
    id key_s = eli_sdl3__string(key);
    eli_sdl3__msg_pp(layer, "addAnimation:forKey:", anim, key_s);
    eli_sdl3__release(key_s);
    eli_sdl3__release(to_n);
    eli_sdl3__release(from_n);
    eli_sdl3__release(path);
    eli_sdl3__release(anim);
}

static inline void eli_sdl3__remove_animation(id layer, const char *key)
{
    id key_s = eli_sdl3__string(key);
    eli_sdl3__msg_p(layer, "removeAnimationForKey:", key_s);
    eli_sdl3__release(key_s);
}

/** @return a new CALayer at the backend's contents scale (+1). */
static inline id eli_sdl3__layer(const char *cls)
{
    id l = eli_sdl3__new(cls);
    eli_sdl3__msg_d(l, "setContentsScale:", (double)g_eli_sdl3_live.scale);
    return l;
}

/* ---------------------------------------------------------------------------
 * Per-kind layer updates
 * ------------------------------------------------------------------------- */

/** Rasterize the text into the mask at the current scale. */
static inline void eli_sdl3__live_raster(const eli_live_widget *w, eli_sdl3_live_layers *L)
{
    float s = g_eli_sdl3_live.scale;
    int pw = (int)ceilf(w->rect.w * s), ph = (int)ceilf(w->rect.h * s);
    L->raster_w = w->rect.w;
    L->raster_h = w->rect.h;
    if (pw <= 0 || ph <= 0) {
        eli_sdl3__msg_p(L->part, "setContents:", NULL);
        return;
    }
    CGContextRef cg = CGBitmapContextCreate(NULL, (size_t)pw, (size_t)ph, 8, 0, NULL,
                                            (CGBitmapInfo)kCGImageAlphaOnly);
    if (!cg)
        return;
    eli_live_text_raster(w, (unsigned char *)CGBitmapContextGetData(cg), pw, ph,
                         (int)CGBitmapContextGetBytesPerRow(cg), s);
    CGImageRef img = CGBitmapContextCreateImage(cg);
    eli_sdl3__msg_p(L->part, "setContents:", img);
    CGImageRelease(img);
    CGContextRelease(cg);
}

/** Spinner arc path for a d x d layer. */
static inline void eli_sdl3__live_arc(id shape, double d)
{
    double th = d * 0.125;   /* thickness = radius / 4 */
    CGMutablePathRef p = CGPathCreateMutable();
    CGPathAddArc(p, NULL, d * 0.5, d * 0.5, d * 0.5 - th * 0.5, 0.0, 1.5 * ELI_PI, false);
    eli_sdl3__msg_p(shape, "setPath:", p);
    eli_sdl3__msg_d(shape, "setLineWidth:", th);
    CGPathRelease(p);
}

static inline void eli_sdl3__live_geometry(const eli_live_widget *w, eli_sdl3_live_layers *L)
{
    eli_rect v = eli_live__intersect(w->rect, w->clip);
    v.w = v.w > 0.0f ? v.w : 0.0f;
    v.h = v.h > 0.0f ? v.h : 0.0f;
    eli_sdl3__msg_rect(L->clip, "setFrame:", CGRectMake(v.x, v.y, v.w, v.h));
    double ox = w->rect.x - v.x, oy = w->rect.y - v.y, rw = w->rect.w, rh = w->rect.h;
    double d = rw < rh ? rw : rh;
    switch (w->kind) {
    case ELI_LIVE_DOT:
        eli_sdl3__msg_rect(L->content, "setFrame:",
                           CGRectMake(ox + (rw - d) * 0.5, oy + (rh - d) * 0.5, d, d));
        eli_sdl3__msg_d(L->content, "setCornerRadius:", d * 0.5);
        break;
    case ELI_LIVE_SPINNER:   /* rotated: bounds + position, never frame */
        eli_sdl3__msg_rect(L->content, "setBounds:", CGRectMake(0.0, 0.0, d, d));
        eli_sdl3__msg_point(L->content, "setPosition:", CGPointMake(ox + rw * 0.5, oy + rh * 0.5));
        eli_sdl3__live_arc(L->content, d);
        break;
    case ELI_LIVE_PROGRESS:
        eli_sdl3__msg_rect(L->content, "setFrame:", CGRectMake(ox, oy, rw, rh));
        eli_sdl3__msg_rect(L->part, "setFrame:", CGRectMake(0.0, 0.0, rw * w->fraction, rh));
        break;
    case ELI_LIVE_TEXT:
        eli_sdl3__msg_rect(L->content, "setFrame:", CGRectMake(ox, oy, rw, rh));
        eli_sdl3__msg_rect(L->part, "setFrame:", CGRectMake(0.0, 0.0, rw, rh));
        if (L->raster_w != w->rect.w || L->raster_h != w->rect.h)
            eli_sdl3__live_raster(w, L);
        break;
    default:
        break;
    }
}

static inline void eli_sdl3__live_style(const eli_live_widget *w, eli_sdl3_live_layers *L)
{
    switch (w->kind) {
    case ELI_LIVE_DOT:
        eli_sdl3__set_color(L->content, "setBackgroundColor:", w->col);
        eli_sdl3__remove_animation(L->content, "eli.pulse");
        if (w->pulse.period > 0.0f)
            eli_sdl3__animate(L->content, "eli.pulse", "opacity", 1.0, w->pulse.min_alpha,
                              w->pulse.period * 0.5, true, true);
        break;
    case ELI_LIVE_SPINNER:
        eli_sdl3__set_color(L->content, "setStrokeColor:", w->col);
        break;
    case ELI_LIVE_PROGRESS:
        eli_sdl3__set_color(L->content, "setBackgroundColor:", w->track_col);
        eli_sdl3__set_color(L->part, "setBackgroundColor:", w->col);
        eli_sdl3__msg_d(L->content, "setCornerRadius:", w->rounding);
        eli_sdl3__msg_d(L->part, "setCornerRadius:", w->rounding);
        break;
    case ELI_LIVE_TEXT:
        eli_sdl3__set_color(L->content, "setBackgroundColor:", w->col);
        break;
    default:
        break;
    }
}

/* ---------------------------------------------------------------------------
 * Backend hooks
 * ------------------------------------------------------------------------- */

static inline void eli_sdl3__live_update(void *user, eli_live_widget *w, unsigned changed)
{
    (void)user;
    eli_sdl3_live_layers *L = (eli_sdl3_live_layers *)w->handle;
    if (!L)
        return;
    void *pool = objc_autoreleasePoolPush();
    eli_sdl3__tx_begin();
    if (changed & (ELI_LIVE_CHANGED_GEOMETRY | ELI_LIVE_CHANGED_VALUE))
        eli_sdl3__live_geometry(w, L);
    if (changed & ELI_LIVE_CHANGED_STYLE)
        eli_sdl3__live_style(w, L);
    if ((changed & ELI_LIVE_CHANGED_TEXT) && w->kind == ELI_LIVE_TEXT)
        eli_sdl3__live_raster(w, L);
    if (changed & ELI_LIVE_CHANGED_VISIBLE)
        eli_sdl3__msg_b(L->clip, "setHidden:", w->visible ? NO : YES);
    eli_sdl3__tx_commit();
    objc_autoreleasePoolPop(pool);
}

static inline bool eli_sdl3__live_create(void *user, eli_live_widget *w)
{
    (void)user;
    eli_sdl3_live_layers *L = NULL;
    for (int i = 0; i < ELI_LIVE_MAX && !L; i++) {
        if (!g_eli_sdl3_live_layers[i].used)
            L = &g_eli_sdl3_live_layers[i];
    }
    if (!L || !g_eli_sdl3_live.container)
        return false;
    memset(L, 0, sizeof(*L));
    L->used = true;
    void *pool = objc_autoreleasePoolPush();
    eli_sdl3__tx_begin();
    L->clip = eli_sdl3__layer("CALayer");
    eli_sdl3__msg_b(L->clip, "setMasksToBounds:", YES);
    eli_sdl3__msg_b(L->clip, "setHidden:", YES);   /* shown at frame end when visible */
    switch (w->kind) {
    case ELI_LIVE_SPINNER:
        L->content = eli_sdl3__layer("CAShapeLayer");
        eli_sdl3__msg_p(L->content, "setFillColor:", NULL);
        eli_sdl3__set_string(L->content, "setLineCap:", "round");
        eli_sdl3__animate(L->content, "eli.spin", "transform.rotation.z", 0.0, 2.0 * ELI_PI,
                          ELI_LIVE_SPINNER_PERIOD, false, false);
        break;
    case ELI_LIVE_PROGRESS:
        L->content = eli_sdl3__layer("CALayer");
        L->part = eli_sdl3__layer("CALayer");
        eli_sdl3__msg_b(L->content, "setMasksToBounds:", YES);
        eli_sdl3__msg_p(L->content, "addSublayer:", L->part);
        break;
    case ELI_LIVE_TEXT:
        L->content = eli_sdl3__layer("CALayer");
        L->part = eli_sdl3__layer("CALayer");
        eli_sdl3__msg_p(L->content, "setMask:", L->part);
        L->raster_w = L->raster_h = -1.0f;
        break;
    default:   /* dot */
        L->content = eli_sdl3__layer("CALayer");
        break;
    }
    eli_sdl3__msg_p(L->clip, "addSublayer:", L->content);
    eli_sdl3__msg_p(g_eli_sdl3_live.container, "addSublayer:", L->clip);
    w->handle = L;
    eli_sdl3__live_geometry(w, L);
    eli_sdl3__live_style(w, L);
    eli_sdl3__tx_commit();
    objc_autoreleasePoolPop(pool);
    return true;
}

static inline void eli_sdl3__live_destroy(void *user, eli_live_widget *w)
{
    (void)user;
    eli_sdl3_live_layers *L = (eli_sdl3_live_layers *)w->handle;
    w->handle = NULL;
    if (!L)
        return;
    void *pool = objc_autoreleasePoolPush();
    eli_sdl3__tx_begin();
    eli_sdl3__msg(L->clip, "removeFromSuperlayer");
    eli_sdl3__tx_commit();
    eli_sdl3__release(L->part);
    eli_sdl3__release(L->content);
    eli_sdl3__release(L->clip);
    objc_autoreleasePoolPop(pool);
    memset(L, 0, sizeof(*L));
}

/** Size the container to the CAMetalLayer (y-down points). */
static inline void eli_sdl3__live_fit_container(void)
{
    eli_sdl3__tx_begin();
    eli_sdl3__msg_rect(g_eli_sdl3_live.container, "setFrame:",
                       eli_sdl3__msg_ret_rect(g_eli_sdl3_live.metal, "bounds"));
    eli_sdl3__tx_commit();
}

/* ---------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

/**
 * Attach live widget layers to the renderer's CAMetalLayer and install the
 * backend (eli_live_set_backend). Call after eli_sdl3_renderer_init*.
 *
 * @param renderer  An SDL renderer; only "metal" has a CAMetalLayer.
 * @return          false (live widgets keep the fallback) when there is no
 *                  CAMetalLayer.
 *
 * Thread-safe: no (main thread)  Reentrant: no
 */
static inline bool eli_sdl3_live_init(SDL_Renderer *renderer)
{
    id metal = renderer ? (id)SDL_GetRenderMetalLayer(renderer) : NULL;
    if (!metal || g_eli_sdl3_live.container)
        return false;
    g_eli_sdl3_live.metal = metal;
    g_eli_sdl3_live.window = SDL_GetRenderWindow(renderer);
    g_eli_sdl3_live.scale = g_eli_sdl3_live.window ? SDL_GetWindowPixelDensity(g_eli_sdl3_live.window)
                                                   : 1.0f;
    void *pool = objc_autoreleasePoolPush();
    eli_sdl3__tx_begin();
    id c = eli_sdl3__layer("CALayer");
    /* Points with y down, whatever the metal layer's own flip. */
    eli_sdl3__msg_b(c, "setGeometryFlipped:", eli_sdl3__msg_ret_b(metal, "contentsAreFlipped") ? NO : YES);
    eli_sdl3__msg_u(c, "setAutoresizingMask:", 2u | 16u);   /* width + height sizable */
    eli_sdl3__msg_p(metal, "addSublayer:", c);
    g_eli_sdl3_live.container = c;
    eli_sdl3__tx_commit();
    eli_sdl3__live_fit_container();
    objc_autoreleasePoolPop(pool);
    eli_live_backend b = { NULL,
                           ELI_LIVE_KIND_BIT(ELI_LIVE_DOT) | ELI_LIVE_KIND_BIT(ELI_LIVE_SPINNER) |
                               ELI_LIVE_KIND_BIT(ELI_LIVE_PROGRESS) | ELI_LIVE_KIND_BIT(ELI_LIVE_TEXT),
                           eli_sdl3__live_create, eli_sdl3__live_update, eli_sdl3__live_destroy };
    eli_live_set_backend(&b);
    return true;
}

/**
 * Keep the layers in step with the window: resize (container size) and display
 * scale change (contents scale, text re-rastered). Feed every SDL event.
 *
 * Thread-safe: no (main thread)  Reentrant: no
 */
static inline void eli_sdl3_live_process_event(const SDL_Event *e)
{
    if (!e || !g_eli_sdl3_live.container)
        return;
    if (e->type == SDL_EVENT_WINDOW_RESIZED || e->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
        void *pool = objc_autoreleasePoolPush();
        eli_sdl3__live_fit_container();
        objc_autoreleasePoolPop(pool);
    }
    if (e->type != SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED || !g_eli_sdl3_live.window)
        return;
    g_eli_sdl3_live.scale = SDL_GetWindowPixelDensity(g_eli_sdl3_live.window);
    void *pool = objc_autoreleasePoolPush();
    eli_sdl3__tx_begin();
    eli_sdl3__msg_d(g_eli_sdl3_live.container, "setContentsScale:", (double)g_eli_sdl3_live.scale);
    for (int i = 0; i < ELI_LIVE_MAX; i++) {
        eli_live_widget *w = &g_eli_live[i];
        eli_sdl3_live_layers *L = (eli_sdl3_live_layers *)w->handle;
        if (!w->used || !L)
            continue;
        id layers[3] = { L->clip, L->content, L->part };
        for (int k = 0; k < 3; k++) {
            if (layers[k])
                eli_sdl3__msg_d(layers[k], "setContentsScale:", (double)g_eli_sdl3_live.scale);
        }
        if (w->kind == ELI_LIVE_TEXT)
            eli_sdl3__live_raster(w, L);
    }
    eli_sdl3__tx_commit();
    objc_autoreleasePoolPop(pool);
}

/**
 * Remove every live widget layer and the container, and uninstall the backend
 * (live widgets fall back). Call before eli_sdl3_renderer_shutdown.
 *
 * Thread-safe: no (main thread)  Reentrant: no
 */
static inline void eli_sdl3_live_shutdown(void)
{
    if (!g_eli_sdl3_live.container)
        return;
    eli_live_set_backend(NULL);
    void *pool = objc_autoreleasePoolPush();
    eli_sdl3__tx_begin();
    eli_sdl3__msg(g_eli_sdl3_live.container, "removeFromSuperlayer");
    eli_sdl3__tx_commit();
    eli_sdl3__release(g_eli_sdl3_live.container);
    objc_autoreleasePoolPop(pool);
    memset(&g_eli_sdl3_live, 0, sizeof(g_eli_sdl3_live));
}

/** @return the container layer (CALayer *), or NULL when not attached. */
static inline void *eli_sdl3_live_container(void)
{
    return (void *)g_eli_sdl3_live.container;
}

/**
 * @return a live widget's layer (CALayer *): 0 = clip, 1 = content, 2 = part
 *         (progress fill / text mask); NULL when it has none.
 */
static inline void *eli_sdl3_live_layer(eli_id id, int which)
{
    eli_live_widget *w = eli_live_find(id);
    eli_sdl3_live_layers *L = w ? (eli_sdl3_live_layers *)w->handle : NULL;
    if (!L)
        return NULL;
    return (void *)(which == 0 ? L->clip : (which == 1 ? L->content : L->part));
}

#else /* !__APPLE__ */

static inline bool eli_sdl3_live_init(SDL_Renderer *renderer)
{
    (void)renderer;
    return false;
}

static inline void eli_sdl3_live_process_event(const SDL_Event *e)
{
    (void)e;
}

static inline void eli_sdl3_live_shutdown(void)
{
}

static inline void *eli_sdl3_live_container(void)
{
    return NULL;
}

static inline void *eli_sdl3_live_layer(eli_id id, int which)
{
    (void)id;
    (void)which;
    return NULL;
}

#endif /* __APPLE__ */

#endif /* ELI_SDL3_LIVE_H */
