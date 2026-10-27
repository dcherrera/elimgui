/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file example_app.h
 * @brief Shared declarations for the SDL3 desktop example's translation units.
 *
 * main.c (the ELI_IMPLEMENTATION TU: SDL, loop, fonts), shell.c (dockspace and
 * panes), perf_fixture.c (text-heavy window + frame overlay) and tasks.c (worker
 * thread + spinner) are separate TUs built with ELI_SHARED_STATE, proving one
 * elimgui context across files (D4).
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#ifndef ELI_EXAMPLE_APP_H
#define ELI_EXAMPLE_APP_H

#include <stdbool.h>
#include <stdint.h>

#include <SDL3/SDL.h>

/** Monospace fixture size (spec §20.3: 120 columns x 48 lines). */
#define EXAMPLE_FIXTURE_COLS 120
#define EXAMPLE_FIXTURE_LINES 48

/** Per-run statistics shown by the overlay and printed at exit. */
typedef struct example_stats {
    uint64_t frames_built;      /* frames built since start */
    double last_build_ms;       /* CPU time of the last eli_frame_begin..end */
    double last_submit_ms;      /* CPU time to submit draw data to SDL (excl. present) */
    float density;              /* window pixel density */
    const char *renderer_name;  /* SDL_GetRendererName */
} example_stats;

/** Global stats, owned by main.c. */
extern example_stats g_example_stats;

/** Build the dockspace and its panes (shell.c). */
void example_shell_draw(void);

/** @return the dock node id for the perf fixture (0 until the layout exists) (shell.c). */
unsigned int example_shell_fixture_dock_id(void);

/** Build the text-heavy perf fixture window (perf_fixture.c). */
void example_perf_fixture_draw(void);

/** Draw the frame counter / timing overlay on the foreground list (perf_fixture.c). */
void example_overlay_draw(void);

/** @return true and the Name field's center in points once it has been laid out (shell.c). */
bool example_shell_name_field_center(float *x, float *y);

/** @return the Name field's text (shell.c). */
const char *example_shell_name_text(void);

/** @return how often the app-level Shortcut+C (GLOBAL route) fired (shell.c). */
int example_shell_app_copy_count(void);

/**
 * Drive the real SDL3 backend with synthetic SDL events and the system clipboard:
 * emoji typing, Cmd/Ctrl copy/paste, > 1 KB and 64 KB clipboard both ways, and app
 * shortcuts yielding while typing. Restores the clipboard. (selftest.c)
 *
 * @param window  The example window (event window id).
 * @param frame   Builds and renders one frame.
 * @return        Number of failed checks (0 = all passed).
 */
int example_input_selftest(SDL_Window *window, void (*frame)(void));

/** SDL user event type that worker threads push to wake the loop (tasks.c). */
extern uint32_t g_example_wake_event;

/** Register the wake event (tasks.c). Call after SDL_Init. */
void example_tasks_init(void);

/** Apply worker results on the main thread and request a redraw (tasks.c). */
void example_tasks_apply_wake(void);

/** Worker-job button and spinner toggle, drawn inside the Editor pane (tasks.c). */
void example_tasks_draw(void);

#endif /* ELI_EXAMPLE_APP_H */
