/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_test_live.h
 * @brief Live widget test harness: a recording fake backend and a simulated
 *        event-driven loop that counts the main-surface frames the library asks
 *        for (the SDL3 example's loop without SDL).
 *
 * @status In use by the test_live_* suites.
 * @issues None
 * @todo None
 */
#ifndef ELI_TEST_LIVE_H
#define ELI_TEST_LIVE_H

#include "eli_test_ui.h"

/** What the fake backend saw. */
typedef struct test_live_log {
    int creates, updates, destroys;
    unsigned changed_or;          /* OR of every update's bits */
    unsigned last_changed;
    eli_id last_id;
    bool last_visible;
    bool refuse;                  /* create returns false */
    int handles;                  /* layers alive */
    unsigned char raster[64 * 24];
    bool raster_ok;
} test_live_log;

static test_live_log g_test_live;

static bool test_live_create(void *user, eli_live_widget *w)
{
    (void)user;
    g_test_live.creates++;
    if (g_test_live.refuse)
        return false;
    w->handle = &g_test_live;
    g_test_live.handles++;
    if (w->kind == ELI_LIVE_TEXT)
        g_test_live.raster_ok = eli_live_text_raster(w, g_test_live.raster, 64, 24, 64, 1.0f);
    return true;
}

static void test_live_update(void *user, eli_live_widget *w, unsigned changed)
{
    (void)user;
    g_test_live.updates++;
    g_test_live.changed_or |= changed;
    g_test_live.last_changed = changed;
    g_test_live.last_id = w->id;
    g_test_live.last_visible = w->visible;
}

static void test_live_destroy(void *user, eli_live_widget *w)
{
    (void)user;
    g_test_live.destroys++;
    g_test_live.handles--;
    w->handle = NULL;
}

/** Install the fake backend (all kinds) and clear its log. */
static inline void test_live_install(void)
{
    memset(&g_test_live, 0, sizeof(g_test_live));
    eli_live_backend b = { NULL, 0xFFu, test_live_create, test_live_update, test_live_destroy };
    eli_live_set_backend(&b);
}

/** Clear the fake backend's counters (keeps `refuse` and `handles`). */
static inline void test_live_reset_counts(void)
{
    g_test_live.creates = g_test_live.updates = g_test_live.destroys = 0;
    g_test_live.changed_or = g_test_live.last_changed = 0;
}

/** One frame of `dt` seconds around `ui`. */
static inline void test_live_frame_dt(void (*ui)(void), float dt)
{
    eli_get_io()->delta_time = dt;
    eli_frame_begin();
    if (ui)
        ui();
    eli_frame_end();
}

/**
 * Run an event-driven loop with no input for `seconds` of simulated time: a frame
 * is built only when the library's redraw timeout asks for one (0 = next vsync at
 * 60 Hz, > 0 = after that long, -1 = sleep until input, which ends the run).
 *
 * @return the number of frames built.
 */
static inline int test_live_loop_frames(void (*ui)(void), double seconds)
{
    double t = 0.0;
    int frames = 0;
    for (;;) {
        float timeout = eli_get_redraw_timeout();
        if (timeout < 0.0f)
            break;
        float dt = timeout > 0.0f ? timeout : 1.0f / 60.0f;
        t += dt;
        if (t > seconds)
            break;
        test_live_frame_dt(ui, dt);
        frames++;
    }
    return frames;
}

/** Window `name` at `pos` (always), `size`. */
static inline bool test_live_begin(const char *name, eli_vec2 pos, eli_vec2 size)
{
    eli_set_next_window_pos(pos, ELI_COND_ALWAYS, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(size, ELI_COND_ALWAYS);
    return eli_begin(name, NULL, ELI_WINDOW_NO_SAVED_SETTINGS);
}

#endif /* ELI_TEST_LIVE_H */
