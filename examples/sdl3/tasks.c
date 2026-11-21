/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file tasks.c
 * @brief SDL3 example, fourth TU: a worker-thread job that wakes the idle loop
 *        with a custom SDL event, an animated spinner that keeps frames coming
 *        only while enabled (eli_request_redraw_after), and live widgets that
 *        animate and update without frames (docs/live-widgets.md).
 *
 * Threads never touch elimgui: the worker stores its result atomically and pushes
 * g_example_wake_event; the main loop applies it. A finished job calls
 * eli_request_redraw(); job progress and the live clock's 1 s tick only call
 * eli_live_set_fraction / eli_live_set_text, which update the layers without a
 * frame (the fallback asks for one itself).
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#include "example_app.h"

#include <eli/elimgui.h>

#include <SDL3/SDL.h>

#include <math.h>
#include <stdio.h>

uint32_t g_example_wake_event;
bool g_example_live_dot = false;

/** g_example_wake_event codes (SDL_UserEvent.code). */
enum { TASKS_WAKE_DONE = 0, TASKS_WAKE_PROGRESS, TASKS_WAKE_CLOCK };

#define TASKS_JOB_STEPS 15                /* 15 x 100 ms of pretend I/O */

static SDL_AtomicInt g_tasks_result;      /* written by the worker */
static SDL_AtomicInt g_tasks_running;
static SDL_AtomicInt g_tasks_step;        /* job progress, 0..TASKS_JOB_STEPS */
static int g_tasks_shown_result = 0;      /* main-thread copy shown in the UI */
static bool g_tasks_spinner = false;

static bool g_tasks_live = false;         /* the live widgets section */
static float g_tasks_progress = 0.0f;     /* main-thread copy of the job progress */
static eli_id g_tasks_bar = 0, g_tasks_clock = 0;
static char g_tasks_clock_text[32] = "up 0 s";
static SDL_TimerID g_tasks_timer = 0;
static Uint64 g_tasks_live_start_ms = 0;

/** Push a wake event with `code` (any thread). */
static void tasks_wake(Sint32 code)
{
    SDL_Event ev;
    SDL_zero(ev);
    ev.type = g_example_wake_event;
    ev.user.code = code;
    if (!SDL_PushEvent(&ev))
        SDL_Log("tasks: SDL_PushEvent failed: %s", SDL_GetError());
}

/** Worker: pretend to do 1.5 s of I/O in steps, publish a result, wake the main loop. */
static int SDLCALL tasks_worker(void *user_data)
{
    (void)user_data;
    for (int i = 1; i <= TASKS_JOB_STEPS; i++) {
        SDL_Delay(100);
        SDL_SetAtomicInt(&g_tasks_step, i);
        tasks_wake(TASKS_WAKE_PROGRESS);
    }
    SDL_AddAtomicInt(&g_tasks_result, 1);
    SDL_SetAtomicInt(&g_tasks_running, 0);
    tasks_wake(TASKS_WAKE_DONE);
    return 0;
}

/** SDL timer thread: tick the live clock once a second. */
static Uint32 SDLCALL tasks_clock_timer(void *user_data, SDL_TimerID id, Uint32 interval)
{
    (void)user_data;
    (void)id;
    tasks_wake(TASKS_WAKE_CLOCK);
    return interval;
}

void example_tasks_init(void)
{
    g_example_wake_event = SDL_RegisterEvents(1);
    if (g_example_wake_event == 0)
        SDL_Log("tasks: SDL_RegisterEvents failed: %s", SDL_GetError());
}

void example_tasks_apply_wake(const SDL_Event *e)
{
    switch (e->user.code) {
    case TASKS_WAKE_PROGRESS:
        g_tasks_progress = (float)SDL_GetAtomicInt(&g_tasks_step) / (float)TASKS_JOB_STEPS;
        eli_live_set_fraction(g_tasks_bar, g_tasks_progress);   /* no frame with a layer */
        break;
    case TASKS_WAKE_CLOCK:
        snprintf(g_tasks_clock_text, sizeof(g_tasks_clock_text), "up %llu s",
                 (unsigned long long)((SDL_GetTicks() - g_tasks_live_start_ms) / 1000));
        eli_live_set_text(g_tasks_clock, g_tasks_clock_text);
        break;
    default:
        g_tasks_shown_result = SDL_GetAtomicInt(&g_tasks_result);
        eli_request_redraw();
        break;
    }
}

/** The live widgets section: a pulsing dot, a spinner, the job's progress, a clock. */
static void tasks_draw_live(void)
{
    static const eli_live_pulse pulse = { 1.4f, 0.3f };
    if (!g_tasks_timer) {
        g_tasks_live_start_ms = SDL_GetTicks();
        g_tasks_timer = SDL_AddTimer(1000, tasks_clock_timer, NULL);
    }
    eli_live_dot("live-dot", 5.0f, ELI_COL32(90, 210, 130, 255), &pulse);
    eli_same_line(0.0f, -1.0f);
    eli_live_spinner("live-spin", 8.0f, ELI_COL32(220, 220, 230, 255));
    eli_same_line(0.0f, -1.0f);
    g_tasks_bar = eli_live_progress("live-bar", eli_make_vec2(160.0f, 0.0f), g_tasks_progress);
    eli_same_line(0.0f, -1.0f);
    g_tasks_clock = eli_live_text("live-clock", g_tasks_clock_text,
                                  eli_calc_text_size("up 00000 s", NULL).x);
}

/** Draw a 12-spoke spinner; requests the next animation frame while visible. */
static void tasks_draw_spinner(void)
{
    eli_draw_list *dl = eli_get_window_draw_list();
    eli_vec2 c = eli_get_cursor_screen_pos();
    c.x += 12.0f;
    c.y += 12.0f;
    double t = eli_get_time();
    int head = (int)(t * 12.0) % 12;
    for (int i = 0; i < 12; i++) {
        float a = (float)i * 6.2831853f / 12.0f;
        int age = (head - i + 12) % 12;
        eli_col32 col = ELI_COL32(255, 255, 255, (unsigned)(255 - age * 18));
        eli_draw_list_add_line(dl, eli_make_vec2(c.x + cosf(a) * 5.0f, c.y + sinf(a) * 5.0f),
                               eli_make_vec2(c.x + cosf(a) * 10.0f, c.y + sinf(a) * 10.0f), col, 2.0f);
    }
    eli_dummy(eli_make_vec2(24.0f, 24.0f));
    eli_request_redraw_after(1.0f / 30.0f);   /* 30 fps while spinning, idle otherwise */
}

void example_tasks_draw(void)
{
    eli_separator();
    if (eli_button("Start background job") && SDL_GetAtomicInt(&g_tasks_running) == 0) {
        SDL_SetAtomicInt(&g_tasks_running, 1);
        SDL_Thread *th = SDL_CreateThread(tasks_worker, "example-job", NULL);
        if (th)
            SDL_DetachThread(th);
        else
            SDL_SetAtomicInt(&g_tasks_running, 0);
    }
    eli_same_line(0.0f, -1.0f);
    eli_text("%s, jobs finished: %d", SDL_GetAtomicInt(&g_tasks_running) ? "running" : "idle",
             g_tasks_shown_result);
    eli_checkbox("Spinner (animates via eli_request_redraw_after)", &g_tasks_spinner);
    if (g_tasks_spinner)
        tasks_draw_spinner();
    eli_checkbox("Live widgets (compositor layers: no frames while they animate)", &g_tasks_live);
    if (g_tasks_live) {
        tasks_draw_live();
    } else if (g_tasks_timer) {
        SDL_RemoveTimer(g_tasks_timer);
        g_tasks_timer = 0;
    }
}

void example_tasks_draw_probe(void)
{
    static const eli_live_pulse pulse = { 1.4f, 0.3f };
    if (!g_example_live_dot)
        return;
    eli_live_dot("probe-dot", 5.0f, ELI_COL32(90, 210, 130, 255), &pulse);
    eli_same_line(0.0f, -1.0f);
    eli_text("pulsing live dot (--live-dot)");
}
