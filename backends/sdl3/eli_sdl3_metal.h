/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_sdl3_metal.h
 * @brief CAMetalLayer controls SDL_Renderer does not expose: drawable count
 *        (swapchain depth) and drawable size, from plain C via the Objective-C
 *        runtime. Used by eli_sdl3_renderer.h; apps normally go through
 *        eli_sdl3_renderer_init_ex instead of calling these directly.
 *
 * Header-only, `static inline`. `layer` is SDL_GetRenderMetalLayer(renderer)
 * (NULL on every renderer except "metal"). Each drawable is one BGRA8 IOSurface
 * of the drawable size: 3420x1944 px = 26.6 MB, so 3 -> 2 drawables saves one.
 *
 * Linking: on Apple platforms the app links libobjc (any Cocoa/Foundation
 * framework, or -lobjc); only TUs that call these functions need it.
 *
 * @status In use (SDL3 renderer backend, macOS).
 * @issues Non-Apple builds compile to no-ops that return false / 0.
 * @todo None
 */
#ifndef ELI_SDL3_METAL_H
#define ELI_SDL3_METAL_H

#include <stdbool.h>

#if defined(__APPLE__)
#include <objc/message.h>
#include <objc/runtime.h>

/** CGSize without CoreGraphics (CGFloat is double on every 64-bit Apple target). */
typedef struct eli_sdl3_metal_size {
    double width, height;
} eli_sdl3_metal_size;

/** @return true if the layer object answers `selector_name`. */
static inline bool eli_sdl3__metal_responds(void *layer, const char *selector_name)
{
    return layer && class_respondsToSelector(object_getClass((id)layer), sel_registerName(selector_name));
}

/**
 * Set CAMetalLayer.maximumDrawableCount.
 *
 * @param layer  The renderer's CAMetalLayer (NULL: no-op).
 * @param count  2 or 3 (the only values Core Animation accepts; others are refused).
 * @return       true if applied.
 *
 * Thread-safe: no (main thread)  Reentrant: no
 */
static inline bool eli_sdl3_metal_set_drawable_count(void *layer, int count)
{
    if ((count != 2 && count != 3) || !eli_sdl3__metal_responds(layer, "setMaximumDrawableCount:"))
        return false;
    ((void (*)(id, SEL, unsigned long))objc_msgSend)((id)layer, sel_registerName("setMaximumDrawableCount:"),
                                                     (unsigned long)count);
    return true;
}

/** @return CAMetalLayer.maximumDrawableCount, or 0 when `layer` is not a Metal layer. */
static inline int eli_sdl3_metal_drawable_count(void *layer)
{
    if (!eli_sdl3__metal_responds(layer, "maximumDrawableCount"))
        return 0;
    return (int)((unsigned long (*)(id, SEL))objc_msgSend)((id)layer, sel_registerName("maximumDrawableCount"));
}

/**
 * Set CAMetalLayer.drawableSize in pixels. Core Animation drops the pooled
 * drawables of the old size; the one on screen stays until a later present
 * replaces it (measured: shrinking to 1x1 frees every drawable but that one).
 *
 * @return true if applied.
 *
 * Thread-safe: no (main thread)  Reentrant: no
 */
static inline bool eli_sdl3_metal_set_drawable_size(void *layer, int width, int height)
{
    if (width <= 0 || height <= 0 || !eli_sdl3__metal_responds(layer, "setDrawableSize:"))
        return false;
    eli_sdl3_metal_size size = { (double)width, (double)height };
    ((void (*)(id, SEL, eli_sdl3_metal_size))objc_msgSend)((id)layer, sel_registerName("setDrawableSize:"), size);
    return true;
}

/**
 * Read CAMetalLayer.drawableSize in pixels.
 *
 * @return false (and 0 x 0) when `layer` is not a Metal layer.
 */
static inline bool eli_sdl3_metal_drawable_size(void *layer, int *width, int *height)
{
    eli_sdl3_metal_size size = { 0.0, 0.0 };
    bool ok = eli_sdl3__metal_responds(layer, "drawableSize");
    if (ok)
        size = ((eli_sdl3_metal_size (*)(id, SEL))objc_msgSend)((id)layer, sel_registerName("drawableSize"));
    if (width)
        *width = (int)size.width;
    if (height)
        *height = (int)size.height;
    return ok;
}

#else /* !__APPLE__ */

static inline bool eli_sdl3_metal_set_drawable_count(void *layer, int count)
{
    (void)layer;
    (void)count;
    return false;
}

static inline int eli_sdl3_metal_drawable_count(void *layer)
{
    (void)layer;
    return 0;
}

static inline bool eli_sdl3_metal_set_drawable_size(void *layer, int width, int height)
{
    (void)layer;
    (void)width;
    (void)height;
    return false;
}

static inline bool eli_sdl3_metal_drawable_size(void *layer, int *width, int *height)
{
    (void)layer;
    if (width)
        *width = 0;
    if (height)
        *height = 0;
    return false;
}

#endif /* __APPLE__ */

#endif /* ELI_SDL3_METAL_H */
