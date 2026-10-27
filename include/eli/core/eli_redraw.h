/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_redraw.h
 * @brief Idle / redraw API (spec §7, D6): tells the app's loop how long it may
 *        sleep after a frame instead of repainting forever.
 *
 * After eli_frame_end (or eli_redraw_end_frame when driving the hooks by hand):
 *
 *     float t = eli_get_redraw_timeout();  // -1 sleep until input, 0 frame now,
 *                                          // > 0 seconds until a timed change
 *
 * Rules applied at frame end:
 *   1. Input processed this frame, or eli_request_redraw() since the last frame
 *      end, owes io.config_redraw_settle_frames more frames (default 2).
 *   2. While frames are owed the timeout is 0.
 *   3. Otherwise it is the earliest eli_request_redraw_after() of this frame, or -1.
 * Library sources: caret blink, tooltip hover delay, key/mouse repeat, appearing
 * windows. Apps call eli_request_redraw() when non-input data changes what is
 * visible and eli_request_redraw_after() for their own animations.
 *
 * @status In use (native loops; the web loop adopts it in the web-parity phase).
 * @issues None
 * @todo None
 */
#ifndef ELI_CORE_REDRAW_H
#define ELI_CORE_REDRAW_H

#include "eli_platform.h"
#include "eli_context.h"

/**
 * Something visible changed: build at least one more frame (plus settle frames).
 * Outside a frame it also zeroes the current timeout so a waiting loop wakes.
 *
 * Thread-safe: no (main thread; worker threads wake the loop with an OS/SDL event)
 * Reentrant: yes
 */
static inline void eli_request_redraw(void)
{
    eli_context *ctx = eli_get_current_context();
    if (!ctx)
        return;
    ctx->redraw_requested = true;
    if (!ctx->redraw_collecting)
        ctx->redraw_timeout = 0.0f;
}

/**
 * Request a frame `seconds` from now (timed state: blink, delay, animation). The
 * earliest request of a frame wins; requests are per frame, so re-request while
 * the timed state lasts. Outside a frame it only shortens the pending timeout.
 *
 * @param seconds  Delay (< 0 is treated as 0).
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline void eli_request_redraw_after(float seconds)
{
    eli_context *ctx = eli_get_current_context();
    if (!ctx)
        return;
    if (seconds < 0.0f)
        seconds = 0.0f;
    if (!ctx->redraw_collecting) {
        /* Between frames: only the pending wait changes (the frame it wakes for
         * resolves its own deadlines). */
        if (ctx->redraw_timeout < 0.0f || seconds < ctx->redraw_timeout)
            ctx->redraw_timeout = seconds;
        return;
    }
    if (ctx->redraw_after_min < 0.0f || seconds < ctx->redraw_after_min)
        ctx->redraw_after_min = seconds;
}

/**
 * Resolve this frame's redraw timeout (rules in the file header) and reset the
 * per-frame request state. eli_frame_end calls it last; call it yourself only
 * when driving the per-subsystem hooks by hand.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_redraw_end_frame(void)
{
    eli_context *ctx = eli_get_current_context();
    if (!ctx)
        return;
    if (ctx->redraw_input_seen || ctx->redraw_requested)
        ctx->redraw_settle_left = ctx->io.config_redraw_settle_frames > 0
                                      ? ctx->io.config_redraw_settle_frames : 1;
    if (ctx->redraw_settle_left > 0) {
        ctx->redraw_timeout = 0.0f;
        ctx->redraw_settle_left--;
    } else {
        ctx->redraw_timeout = ctx->redraw_after_min;
    }
    ctx->redraw_requested = false;
    ctx->redraw_input_seen = false;
    ctx->redraw_after_min = -1.0f;
    ctx->redraw_collecting = false;
}

/**
 * @return the redraw timeout computed at the last frame end, in seconds:
 *         -1 = nothing pending (sleep until input), 0 = build a frame now,
 *         > 0 = build one after that long.
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline float eli_get_redraw_timeout(void)
{
    eli_context *ctx = eli_get_current_context();
    return ctx ? ctx->redraw_timeout : -1.0f;
}

/**
 * Next typematic-repeat edge for a control held `held` seconds.
 *
 * @return seconds until the next repeat (first one at `delay`, then every `rate`).
 */
static inline float eli_redraw_next_repeat_in(float held, float delay, float rate)
{
    if (held < delay)
        return delay - held;
    if (rate <= 0.0f)
        return -1.0f;
    return rate - fmodf(held - delay, rate);
}

#endif /* ELI_CORE_REDRAW_H */
