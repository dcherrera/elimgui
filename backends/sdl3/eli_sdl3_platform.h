/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_sdl3_platform.h
 * @brief SDL3 platform backend: SDL events -> eli_io input; eli_io outputs ->
 *        SDL (cursor, clipboard, open URL, mouse warp); display size, density and
 *        frame timing each frame.
 *
 * Header-only, `static inline`. Include after <eli/elimgui.h>. Requires SDL >= 3.2.
 * The app owns the window, the loop and SDL_Init; this layer never presents.
 *
 *     eli_sdl3_platform_init(window);
 *     while (running) {
 *         int t = eli_sdl3_wait_timeout_ms();          // -1, 0 or ms (idle loop)
 *         ... SDL_WaitEvent / SDL_WaitEventTimeout(t), then drain with SDL_PollEvent,
 *             feeding each event to eli_sdl3_platform_process_event(&e) ...
 *         eli_sdl3_platform_new_frame();
 *         eli_frame_begin(); ...widgets...; eli_frame_end();
 *         eli_sdl3_platform_end_frame();
 *         ... render with eli_sdl3_renderer.h, SDL_RenderPresent ...
 *     }
 *     eli_sdl3_platform_shutdown();
 *
 * Coordinates: io.display_size and mouse positions are in window points;
 * io.display_framebuffer_scale = SDL_GetWindowPixelDensity (2 on Retina).
 *
 * @status Desktop (macOS, Linux) in use. iOS lifecycle/safe area are not here yet.
 * @issues Text input is started once at init (desktop); want_text_input-driven
 *         start/stop, IME rect and on-screen keyboard are not implemented.
 * @todo Touch, IME rect, on-screen keyboard, iOS lifecycle (spec §10, §11, §14.3).
 */
#ifndef ELI_SDL3_PLATFORM_H
#define ELI_SDL3_PLATFORM_H

#include <SDL3/SDL.h>

#include <eli/elimgui.h>

#include "eli_sdl3_keymap.h"

/** Backend state (one window per app; shared across TUs like library state). */
typedef struct eli_sdl3_platform_state {
    SDL_Window *window;
    SDL_WindowID window_id;
    Uint64 last_counter;                       /* SDL_GetPerformanceCounter at last frame */
    char *clipboard_cache;                     /* last SDL_GetClipboardText, SDL_free'd */
    SDL_Cursor *cursors[ELI_MOUSE_CURSOR_COUNT];
    int last_cursor;                           /* last applied eli_mouse_cursor, -2 = none */
} eli_sdl3_platform_state;

ELI_GLOBAL(eli_sdl3_platform_state, g_eli_sdl3_platform, {0});

/* ---------------------------------------------------------------------------
 * io callbacks
 * ------------------------------------------------------------------------- */

/** io clipboard get: SDL's text, cached until the next get or shutdown. */
static inline const char *eli_sdl3__get_clipboard(void *user_data)
{
    (void)user_data;
    SDL_free(g_eli_sdl3_platform.clipboard_cache);
    g_eli_sdl3_platform.clipboard_cache = SDL_GetClipboardText(); /* "" on none/error */
    return g_eli_sdl3_platform.clipboard_cache ? g_eli_sdl3_platform.clipboard_cache : "";
}

/** io clipboard set. */
static inline void eli_sdl3__set_clipboard(void *user_data, const char *text)
{
    (void)user_data;
    if (!SDL_SetClipboardText(text ? text : ""))
        SDL_Log("eli_sdl3: SDL_SetClipboardText failed: %s", SDL_GetError());
}

/** io open URL. */
static inline bool eli_sdl3__open_url(void *user_data, const char *url)
{
    (void)user_data;
    if (SDL_OpenURL(url))
        return true;
    SDL_Log("eli_sdl3: SDL_OpenURL failed: %s", SDL_GetError());
    return false;
}

/* ---------------------------------------------------------------------------
 * Lifecycle
 * ------------------------------------------------------------------------- */

/**
 * Attach the backend to an SDL window and the current elimgui context: installs
 * clipboard and open-URL callbacks, sets backend flags and starts SDL text input.
 *
 * @param window  The app's SDL window (must stay alive until shutdown).
 * @return        false if window is NULL or there is no current context.
 *
 * Thread-safe: no (main thread only)  Reentrant: no
 */
static inline bool eli_sdl3_platform_init(SDL_Window *window)
{
    eli_io *io = eli_get_io();
    if (!window || !io)
        return false;
    memset(&g_eli_sdl3_platform, 0, sizeof(g_eli_sdl3_platform));
    g_eli_sdl3_platform.window = window;
    g_eli_sdl3_platform.window_id = SDL_GetWindowID(window);
    g_eli_sdl3_platform.last_cursor = -2;
    io->get_clipboard_text_fn = eli_sdl3__get_clipboard;
    io->set_clipboard_text_fn = eli_sdl3__set_clipboard;
    io->clipboard_user_data = NULL;
    io->platform_open_url_fn = eli_sdl3__open_url;
    io->backend_flags |= ELI_BACKEND_FLAGS_HAS_MOUSE_CURSORS | ELI_BACKEND_FLAGS_HAS_SET_MOUSE_POS;
#if defined(SDL_PLATFORM_APPLE)
    io->config_mac_os_behaviors = true;       /* Cmd = shortcut modifier */
#else
    io->config_mac_os_behaviors = false;
#endif
    if (!SDL_StartTextInput(window))
        SDL_Log("eli_sdl3: SDL_StartTextInput failed: %s", SDL_GetError());
    return true;
}

/**
 * Detach from the window: stops text input, frees cursors and the cached
 * clipboard string, and clears the io callbacks this backend installed.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_sdl3_platform_shutdown(void)
{
    eli_sdl3_platform_state *st = &g_eli_sdl3_platform;
    if (st->window)
        SDL_StopTextInput(st->window);
    for (int i = 0; i < ELI_MOUSE_CURSOR_COUNT; i++)
        if (st->cursors[i])
            SDL_DestroyCursor(st->cursors[i]);
    SDL_free(st->clipboard_cache);
    eli_io *io = eli_get_io();
    if (io && io->get_clipboard_text_fn == eli_sdl3__get_clipboard) {
        io->get_clipboard_text_fn = NULL;
        io->set_clipboard_text_fn = NULL;
        io->platform_open_url_fn = NULL;
    }
    memset(st, 0, sizeof(*st));
}

/* ---------------------------------------------------------------------------
 * Events
 * ------------------------------------------------------------------------- */

/** Release every held key and mouse button (focus lost: no stuck keys). */
static inline void eli_sdl3__release_all_input(void)
{
    eli_io *io = eli_get_io();
    for (int k = ELI_KEY_NONE + 1; k < ELI_KEY_MOD_CTRL; k++)
        if (io->keys_down[k])
            eli_io_add_key_event((eli_key)k, false);
    for (int b = 0; b < ELI_MOUSE_BUTTON_COUNT; b++)
        if (io->mouse_down[b])
            eli_io_add_mouse_button_event(b, false);
}

/** Map an SDL mouse button to an elimgui button index, or -1. */
static inline int eli_sdl3__mouse_button(Uint8 sdl_button)
{
    switch (sdl_button) {
    case SDL_BUTTON_LEFT:   return 0;
    case SDL_BUTTON_RIGHT:  return 1;
    case SDL_BUTTON_MIDDLE: return 2;
    case SDL_BUTTON_X1:     return 3;
    case SDL_BUTTON_X2:     return 4;
    default:                return -1;
    }
}

/** Handle mouse/keyboard/text events; returns true if consumed. */
static inline bool eli_sdl3__process_input_event(const SDL_Event *e)
{
    switch (e->type) {
    case SDL_EVENT_MOUSE_MOTION:
        eli_io_add_mouse_pos_event(e->motion.x, e->motion.y);
        return true;
    case SDL_EVENT_MOUSE_WHEEL:
        eli_io_add_mouse_wheel_event(-e->wheel.x, e->wheel.y);
        return true;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
        int b = eli_sdl3__mouse_button(e->button.button);
        if (b < 0 || b >= ELI_MOUSE_BUTTON_COUNT)
            return false;
        eli_io_add_mouse_pos_event(e->button.x, e->button.y);
        eli_io_add_mouse_button_event(b, e->type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        return true;
    }
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
        eli_key k = eli_sdl3_key_from_event(e->key.key, e->key.scancode);
        if (k == ELI_KEY_NONE)
            return false;
        eli_io_add_key_event(k, e->type == SDL_EVENT_KEY_DOWN);
        return true;
    }
    case SDL_EVENT_TEXT_INPUT:
        eli_io_add_input_characters_utf8(e->text.text);
        return true;
    default:
        return false;
    }
}

/** Handle window events for our window; returns true if consumed. */
static inline bool eli_sdl3__process_window_event(const SDL_Event *e)
{
    switch (e->type) {
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        eli_io_add_mouse_pos_event(-FLT_MAX, -FLT_MAX);
        return true;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        eli_sdl3__release_all_input();
        return true;
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
    case SDL_EVENT_WINDOW_EXPOSED:
        return true;
    default:
        return false;
    }
}

/** @return the window id an event targets, or 0 for window-less events. */
static inline SDL_WindowID eli_sdl3__event_window_id(const SDL_Event *e)
{
    switch (e->type) {
    case SDL_EVENT_MOUSE_MOTION:      return e->motion.windowID;
    case SDL_EVENT_MOUSE_WHEEL:       return e->wheel.windowID;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:   return e->button.windowID;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:            return e->key.windowID;
    case SDL_EVENT_TEXT_INPUT:        return e->text.windowID;
    default:
        if (e->type >= SDL_EVENT_WINDOW_FIRST && e->type <= SDL_EVENT_WINDOW_LAST)
            return e->window.windowID;
        return 0;
    }
}

/**
 * Feed one SDL event to elimgui. Events for other windows are ignored.
 *
 * @param e  The event (from SDL_PollEvent/SDL_WaitEvent).
 * @return   true if elimgui consumed it (it changes elimgui state; the app should
 *           build a frame).
 *
 * Thread-safe: no (main thread only)  Reentrant: no
 */
static inline bool eli_sdl3_platform_process_event(const SDL_Event *e)
{
    if (!e || !g_eli_sdl3_platform.window || !eli_get_io())
        return false;
    SDL_WindowID id = eli_sdl3__event_window_id(e);
    if (id != 0 && id != g_eli_sdl3_platform.window_id)
        return false;
    bool consumed = eli_sdl3__process_input_event(e) || eli_sdl3__process_window_event(e);
    if (consumed)
        eli_request_redraw(); /* also covers expose/resize, which queue no input */
    return consumed;
}

/**
 * How long the app's loop may sleep, from eli_get_redraw_timeout().
 *
 * @return -1 = wait for the next event (SDL_WaitEvent), 0 = poll and build a frame
 *         now, > 0 = SDL_WaitEventTimeout milliseconds (rounded up).
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline int eli_sdl3_wait_timeout_ms(void)
{
    float t = eli_get_redraw_timeout();
    if (t < 0.0f)
        return -1;
    float ms = ceilf(t * 1000.0f);
    return ms > 2147483647.0f ? 2147483647 : (int)ms;
}

/* ---------------------------------------------------------------------------
 * Per frame
 * ------------------------------------------------------------------------- */

/**
 * Update io for a new frame: display size (points), framebuffer scale (pixel
 * density) and delta time. Call after processing events, before eli_frame_begin.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_sdl3_platform_new_frame(void)
{
    eli_sdl3_platform_state *st = &g_eli_sdl3_platform;
    eli_io *io = eli_get_io();
    if (!st->window || !io)
        return;
    int w = 0, h = 0;
    SDL_GetWindowSize(st->window, &w, &h);
    if (SDL_GetWindowFlags(st->window) & SDL_WINDOW_MINIMIZED)
        w = h = 0;
    io->display_size = eli_make_vec2((float)w, (float)h);
    float density = SDL_GetWindowPixelDensity(st->window);
    if (density <= 0.0f)
        density = 1.0f;
    io->display_framebuffer_scale = eli_make_vec2(density, density);

    Uint64 now = SDL_GetPerformanceCounter();
    double dt = st->last_counter ? (double)(now - st->last_counter) / (double)SDL_GetPerformanceFrequency()
                                 : 1.0 / 60.0;
    io->delta_time = (dt > 0.0) ? (float)dt : 1.0f / 1000.0f;
    st->last_counter = now;
}

/** SDL system cursor for an eli_mouse_cursor (ARROW for unknown values). */
static inline SDL_SystemCursor eli_sdl3__system_cursor(int cursor)
{
    switch (cursor) {
    case ELI_MOUSE_CURSOR_TEXT_INPUT:  return SDL_SYSTEM_CURSOR_TEXT;
    case ELI_MOUSE_CURSOR_RESIZE_ALL:  return SDL_SYSTEM_CURSOR_MOVE;
    case ELI_MOUSE_CURSOR_RESIZE_NS:   return SDL_SYSTEM_CURSOR_NS_RESIZE;
    case ELI_MOUSE_CURSOR_RESIZE_EW:   return SDL_SYSTEM_CURSOR_EW_RESIZE;
    case ELI_MOUSE_CURSOR_RESIZE_NESW: return SDL_SYSTEM_CURSOR_NESW_RESIZE;
    case ELI_MOUSE_CURSOR_RESIZE_NWSE: return SDL_SYSTEM_CURSOR_NWSE_RESIZE;
    case ELI_MOUSE_CURSOR_HAND:        return SDL_SYSTEM_CURSOR_POINTER;
    case ELI_MOUSE_CURSOR_NOT_ALLOWED: return SDL_SYSTEM_CURSOR_NOT_ALLOWED;
    default:                           return SDL_SYSTEM_CURSOR_DEFAULT;
    }
}

/** Apply io.mouse_cursor (cached system cursors; NONE hides). */
static inline void eli_sdl3__apply_cursor(const eli_io *io)
{
    eli_sdl3_platform_state *st = &g_eli_sdl3_platform;
    int cursor = (int)io->mouse_cursor;
    if (cursor == st->last_cursor)
        return;
    st->last_cursor = cursor;
    if (cursor == ELI_MOUSE_CURSOR_NONE) {
        SDL_HideCursor();
        return;
    }
    if (cursor < 0 || cursor >= ELI_MOUSE_CURSOR_COUNT)
        cursor = ELI_MOUSE_CURSOR_ARROW;
    if (!st->cursors[cursor])
        st->cursors[cursor] = SDL_CreateSystemCursor(eli_sdl3__system_cursor(cursor));
    if (st->cursors[cursor])
        SDL_SetCursor(st->cursors[cursor]);
    SDL_ShowCursor();
}

/**
 * Apply this frame's elimgui outputs to SDL: mouse cursor shape and mouse warp
 * (io.want_set_mouse_pos). Call after eli_frame_end.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_sdl3_platform_end_frame(void)
{
    eli_sdl3_platform_state *st = &g_eli_sdl3_platform;
    eli_io *io = eli_get_io();
    if (!st->window || !io)
        return;
    eli_sdl3__apply_cursor(io);
    if (io->want_set_mouse_pos) {
        SDL_WarpMouseInWindow(st->window, io->mouse_pos.x, io->mouse_pos.y);
        io->want_set_mouse_pos = false;
    }
}

#endif /* ELI_SDL3_PLATFORM_H */
