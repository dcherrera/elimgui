/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file tasks.c
 * @brief SDL3 example, fourth TU: a worker-thread job that wakes the idle loop
 *        with a custom SDL event, and an animated spinner that keeps frames coming
 *        only while enabled (eli_request_redraw_after).
 *
 * Threads never touch elimgui: the worker stores its result atomically and pushes
 * g_example_wake_event; the main loop applies it and calls eli_request_redraw().
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

static SDL_AtomicInt g_tasks_result;      /* written by the worker */
static SDL_AtomicInt g_tasks_running;
static int g_tasks_shown_result = 0;      /* main-thread copy shown in the UI */
static bool g_tasks_spinner = false;

/** Worker: pretend to do 1.5 s of I/O, publish a result, wake the main loop. */
static int SDLCALL tasks_worker(void *user_data)
{
    (void)user_data;
    SDL_Delay(1500);
    SDL_AddAtomicInt(&g_tasks_result, 1);
    SDL_SetAtomicInt(&g_tasks_running, 0);
    SDL_Event ev;
    SDL_zero(ev);
    ev.type = g_example_wake_event;
    if (!SDL_PushEvent(&ev))
        SDL_Log("tasks: SDL_PushEvent failed: %s", SDL_GetError());
    return 0;
}

void example_tasks_init(void)
{
    g_example_wake_event = SDL_RegisterEvents(1);
    if (g_example_wake_event == 0)
        SDL_Log("tasks: SDL_RegisterEvents failed: %s", SDL_GetError());
}

void example_tasks_apply_wake(void)
{
    g_tasks_shown_result = SDL_GetAtomicInt(&g_tasks_result);
    eli_request_redraw();
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
}
