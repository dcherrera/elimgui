/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file selftest.c
 * @brief SDL3 example, fifth TU: `--input-selftest`, the input checklist run
 *        through the real backend (synthetic SDL events into
 *        eli_sdl3_platform_process_event) and the real system clipboard.
 *
 * Checks: emoji typed via SDL_EVENT_TEXT_INPUT lands as UTF-8; Shortcut+A/C/V
 * (Cmd on macOS, Ctrl elsewhere) copy/paste through SDL's clipboard; a 2 KB paste
 * into the field; 64 KB set/get both ways between elimgui and SDL; the app's
 * GLOBAL Shortcut+C does not fire while the field is active, and does after.
 * The clipboard's previous text is restored at the end.
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#include "example_app.h"

#include <eli/elimgui.h>
#include <sdl3/eli_sdl3_platform.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SDL_WindowID g_selftest_window_id;
static int g_selftest_failed;

/** Record one check. */
static void selftest_check(bool ok, const char *what)
{
    SDL_Log("selftest: %s %s", ok ? "PASS" : "FAIL", what);
    if (!ok)
        g_selftest_failed++;
}

/** Feed one synthetic key event (down/up) to the backend. */
static void selftest_key(SDL_Keycode key, SDL_Scancode sc, bool down)
{
    SDL_Event e;
    SDL_zero(e);
    e.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    e.key.windowID = g_selftest_window_id;
    e.key.key = key;
    e.key.scancode = sc;
    e.key.down = down;
    eli_sdl3_platform_process_event(&e);
}

/** Press Shortcut+key (Cmd on macOS, Ctrl elsewhere) over two frames. */
static void selftest_chord(SDL_Keycode key, SDL_Scancode sc, void (*frame)(void))
{
    bool mac = eli_get_io()->config_mac_os_behaviors;
    SDL_Keycode mod = mac ? SDLK_LGUI : SDLK_LCTRL;
    SDL_Scancode mod_sc = mac ? SDL_SCANCODE_LGUI : SDL_SCANCODE_LCTRL;
    selftest_key(mod, mod_sc, true);
    selftest_key(key, sc, true);
    frame();
    selftest_key(key, sc, false);
    selftest_key(mod, mod_sc, false);
    frame();
}

/** Click at a point in window coordinates. */
static void selftest_click(float x, float y, void (*frame)(void))
{
    SDL_Event e;
    SDL_zero(e);
    e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    e.button.windowID = g_selftest_window_id;
    e.button.button = SDL_BUTTON_LEFT;
    e.button.down = true;
    e.button.x = x;
    e.button.y = y;
    eli_sdl3_platform_process_event(&e);
    frame();
    e.type = SDL_EVENT_MOUSE_BUTTON_UP;
    e.button.down = false;
    eli_sdl3_platform_process_event(&e);
    frame();
}

/** Type UTF-8 text as one SDL_EVENT_TEXT_INPUT. */
static void selftest_type(const char *utf8, void (*frame)(void))
{
    SDL_Event e;
    SDL_zero(e);
    e.type = SDL_EVENT_TEXT_INPUT;
    e.text.windowID = g_selftest_window_id;
    e.text.text = utf8;
    eli_sdl3_platform_process_event(&e);
    frame();
    frame();
}

/** @return a heap string of n bytes of 'a'..'z' (caller frees). */
static char *selftest_text(size_t n, char first)
{
    char *s = (char *)malloc(n + 1);
    for (size_t i = 0; s && i < n; i++)
        s[i] = (char)(first + (int)(i % 26));
    if (s)
        s[n] = '\0';
    return s;
}

/** Clipboard checks that do not need the field: 64 KB both ways. */
static void selftest_clipboard_64k(void)
{
    char *big = selftest_text(64 * 1024, 'a');
    eli_set_clipboard_text(big);
    char *got = SDL_GetClipboardText();
    selftest_check(got && strcmp(got, big) == 0, "64 KB elimgui -> SDL clipboard");
    SDL_free(got);
    big[0] = 'Z';
    SDL_SetClipboardText(big);
    const char *mine = eli_get_clipboard_text();
    selftest_check(strcmp(mine, big) == 0, "64 KB SDL clipboard -> elimgui");
    free(big);
}

int example_input_selftest(SDL_Window *window, void (*frame)(void))
{
    g_selftest_window_id = SDL_GetWindowID(window);
    g_selftest_failed = 0;
    char *saved = SDL_GetClipboardText();
    float x = 0.0f, y = 0.0f;
    for (int i = 0; i < 4; i++)
        frame();
    selftest_check(example_shell_name_field_center(&x, &y), "Name field laid out");
    selftest_click(x, y, frame);
    selftest_check(eli_get_io()->want_text_input, "field active -> want_text_input");

    selftest_chord(SDLK_A, SDL_SCANCODE_A, frame);                      /* select all */
    selftest_type("x\xF0\x9F\x98\x80", frame);                          /* x + U+1F600 */
    selftest_check(strcmp(example_shell_name_text(), "x\xF0\x9F\x98\x80") == 0,
                   "emoji typed via SDL_EVENT_TEXT_INPUT stored as UTF-8");

    int app_copies = example_shell_app_copy_count();
    selftest_chord(SDLK_A, SDL_SCANCODE_A, frame);
    selftest_chord(SDLK_C, SDL_SCANCODE_C, frame);
    char *got = SDL_GetClipboardText();
    selftest_check(got && strcmp(got, "x\xF0\x9F\x98\x80") == 0, "Shortcut+C copies to SDL");
    SDL_free(got);
    selftest_check(example_shell_app_copy_count() == app_copies,
                   "app's GLOBAL Shortcut+C does not fire while typing");

    char *two_kb = selftest_text(2048, 'a');
    SDL_SetClipboardText(two_kb);
    selftest_chord(SDLK_A, SDL_SCANCODE_A, frame);
    selftest_chord(SDLK_V, SDL_SCANCODE_V, frame);
    selftest_check(strcmp(example_shell_name_text(), two_kb) == 0, "2 KB paste from SDL");
    free(two_kb);

    selftest_key(SDLK_ESCAPE, SDL_SCANCODE_ESCAPE, true);               /* leave the field */
    frame();
    selftest_key(SDLK_ESCAPE, SDL_SCANCODE_ESCAPE, false);
    frame();
    frame();
    selftest_chord(SDLK_C, SDL_SCANCODE_C, frame);
    selftest_check(example_shell_app_copy_count() == app_copies + 1,
                   "app's Shortcut+C fires once the field is inactive");

    selftest_clipboard_64k();
    SDL_SetClipboardText(saved ? saved : "");
    SDL_free(saved);
    SDL_Log("selftest: %d failed", g_selftest_failed);
    return g_selftest_failed;
}
