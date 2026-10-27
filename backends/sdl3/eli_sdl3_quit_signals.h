/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_sdl3_quit_signals.h
 * @brief Make SIGINT/SIGTERM quit an idle SDL3 app (POSIX: macOS, Linux).
 *
 * An event-driven loop sleeps in SDL_WaitEvent. SDL's own signal handler only
 * marks a quit as pending, which is not delivered until some other event wakes
 * the loop: measured on macOS 26 / SDL 3.4.10, an idle app ignored SIGTERM and
 * SIGINT indefinitely. This helper blocks both signals in every thread and runs a
 * small thread that sigwait()s for them and pushes SDL_EVENT_QUIT, which does wake
 * SDL_WaitEvent (SDL_PushEvent is thread-safe).
 *
 * Call eli_sdl3_quit_signals_install() before SDL_Init, so every thread SDL starts
 * inherits the blocked mask. On glibc with -std=c11 the including TU must define
 * _POSIX_C_SOURCE 200809L (or _DEFAULT_SOURCE) before any #include.
 *
 * @status In use (desktop example).
 * @issues Windows: compiles to a no-op returning false.
 * @todo None
 */
#ifndef ELI_SDL3_QUIT_SIGNALS_H
#define ELI_SDL3_QUIT_SIGNALS_H

#include <SDL3/SDL.h>

#if !defined(_WIN32)
#include <pthread.h>
#include <signal.h>

/** Signal set handled by the helper: SIGINT and SIGTERM. */
static inline void eli_sdl3__quit_signal_set(sigset_t *set)
{
    sigemptyset(set);
    sigaddset(set, SIGINT);
    sigaddset(set, SIGTERM);
}

/** Thread body: turn each SIGINT/SIGTERM into SDL_EVENT_QUIT. */
static inline int SDLCALL eli_sdl3__quit_signal_thread(void *user_data)
{
    (void)user_data;
    sigset_t set;
    eli_sdl3__quit_signal_set(&set);
    for (;;) {
        int sig = 0;
        if (sigwait(&set, &sig) != 0)
            continue;
        SDL_Event e;
        SDL_zero(e);
        e.type = SDL_EVENT_QUIT;
        if (!SDL_PushEvent(&e)) {
            /* No event queue (SDL not up yet): fall back to the default action. */
            signal(sig, SIG_DFL);
            pthread_sigmask(SIG_UNBLOCK, &set, NULL);
            raise(sig);
        }
    }
    return 0;
}
#endif /* !_WIN32 */

/**
 * Block SIGINT/SIGTERM in this and all later threads, disable SDL's own handlers,
 * and start the sigwait thread. Call once, before SDL_Init.
 *
 * @return true if installed (false on Windows or if the thread could not start).
 *
 * Thread-safe: no (call from main before other threads exist)  Reentrant: no
 */
static inline bool eli_sdl3_quit_signals_install(void)
{
#if !defined(_WIN32)
    sigset_t set;
    eli_sdl3__quit_signal_set(&set);
    if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0)
        return false;
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_Thread *t = SDL_CreateThread(eli_sdl3__quit_signal_thread, "eli-quit-signals", NULL);
    if (!t) {
        SDL_Log("eli_sdl3: signal thread failed: %s", SDL_GetError());
        pthread_sigmask(SIG_UNBLOCK, &set, NULL);
        return false;
    }
    SDL_DetachThread(t);
    return true;
#else
    return false;
#endif
}

#endif /* ELI_SDL3_QUIT_SIGNALS_H */
