/**
 * @file eli_sdl3_keymap.h
 * @brief SDL3 key event -> eli_key translation for the SDL3 platform backend.
 *
 * Keypad keys and punctuation map by scancode (layout-independent position);
 * named keys, digits and letters map by keycode (what the layout produces), as
 * Dear ImGui's SDL3 backend does, so Ctrl/Cmd+Z follows the user's layout.
 *
 * @status In use by eli_sdl3_platform.h.
 * @issues None
 * @todo None
 */
#ifndef ELI_SDL3_KEYMAP_H
#define ELI_SDL3_KEYMAP_H

#include <SDL3/SDL.h>

#include <eli/elimgui.h>

/** Map keypad scancodes (SDL3 has no distinct keypad keycodes). */
static inline eli_key eli_sdl3__keypad_key(SDL_Scancode sc)
{
    if (sc >= SDL_SCANCODE_KP_1 && sc <= SDL_SCANCODE_KP_9)
        return (eli_key)(ELI_KEY_KEYPAD_1 + (sc - SDL_SCANCODE_KP_1));
    switch (sc) {
    case SDL_SCANCODE_KP_0:        return ELI_KEY_KEYPAD_0;
    case SDL_SCANCODE_KP_PERIOD:   return ELI_KEY_KEYPAD_DECIMAL;
    case SDL_SCANCODE_KP_DIVIDE:   return ELI_KEY_KEYPAD_DIVIDE;
    case SDL_SCANCODE_KP_MULTIPLY: return ELI_KEY_KEYPAD_MULTIPLY;
    case SDL_SCANCODE_KP_MINUS:    return ELI_KEY_KEYPAD_SUBTRACT;
    case SDL_SCANCODE_KP_PLUS:     return ELI_KEY_KEYPAD_ADD;
    case SDL_SCANCODE_KP_ENTER:    return ELI_KEY_KEYPAD_ENTER;
    case SDL_SCANCODE_KP_EQUALS:   return ELI_KEY_KEYPAD_EQUAL;
    default:                       return ELI_KEY_NONE;
    }
}

/** Map named (non-printing) keycodes. */
static inline eli_key eli_sdl3__named_key(SDL_Keycode kc)
{
    switch (kc) {
    case SDLK_TAB:          return ELI_KEY_TAB;
    case SDLK_LEFT:         return ELI_KEY_LEFT_ARROW;
    case SDLK_RIGHT:        return ELI_KEY_RIGHT_ARROW;
    case SDLK_UP:           return ELI_KEY_UP_ARROW;
    case SDLK_DOWN:         return ELI_KEY_DOWN_ARROW;
    case SDLK_PAGEUP:       return ELI_KEY_PAGE_UP;
    case SDLK_PAGEDOWN:     return ELI_KEY_PAGE_DOWN;
    case SDLK_HOME:         return ELI_KEY_HOME;
    case SDLK_END:          return ELI_KEY_END;
    case SDLK_INSERT:       return ELI_KEY_INSERT;
    case SDLK_DELETE:       return ELI_KEY_DELETE;
    case SDLK_BACKSPACE:    return ELI_KEY_BACKSPACE;
    case SDLK_SPACE:        return ELI_KEY_SPACE;
    case SDLK_RETURN:       return ELI_KEY_ENTER;
    case SDLK_ESCAPE:       return ELI_KEY_ESCAPE;
    case SDLK_CAPSLOCK:     return ELI_KEY_CAPS_LOCK;
    case SDLK_SCROLLLOCK:   return ELI_KEY_SCROLL_LOCK;
    case SDLK_NUMLOCKCLEAR: return ELI_KEY_NUM_LOCK;
    case SDLK_PRINTSCREEN:  return ELI_KEY_PRINT_SCREEN;
    case SDLK_PAUSE:        return ELI_KEY_PAUSE;
    case SDLK_LCTRL:        return ELI_KEY_LEFT_CTRL;
    case SDLK_LSHIFT:       return ELI_KEY_LEFT_SHIFT;
    case SDLK_LALT:         return ELI_KEY_LEFT_ALT;
    case SDLK_LGUI:         return ELI_KEY_LEFT_SUPER;
    case SDLK_RCTRL:        return ELI_KEY_RIGHT_CTRL;
    case SDLK_RSHIFT:       return ELI_KEY_RIGHT_SHIFT;
    case SDLK_RALT:         return ELI_KEY_RIGHT_ALT;
    case SDLK_RGUI:         return ELI_KEY_RIGHT_SUPER;
    case SDLK_APPLICATION:  return ELI_KEY_MENU;
    default:                return ELI_KEY_NONE;
    }
}

/** Map punctuation by scancode (fallback when the keycode is not a letter/digit). */
static inline eli_key eli_sdl3__punct_key(SDL_Scancode sc)
{
    switch (sc) {
    case SDL_SCANCODE_APOSTROPHE:   return ELI_KEY_APOSTROPHE;
    case SDL_SCANCODE_COMMA:        return ELI_KEY_COMMA;
    case SDL_SCANCODE_MINUS:        return ELI_KEY_MINUS;
    case SDL_SCANCODE_PERIOD:       return ELI_KEY_PERIOD;
    case SDL_SCANCODE_SLASH:        return ELI_KEY_SLASH;
    case SDL_SCANCODE_SEMICOLON:    return ELI_KEY_SEMICOLON;
    case SDL_SCANCODE_EQUALS:       return ELI_KEY_EQUAL;
    case SDL_SCANCODE_LEFTBRACKET:  return ELI_KEY_LEFT_BRACKET;
    case SDL_SCANCODE_BACKSLASH:    return ELI_KEY_BACKSLASH;
    case SDL_SCANCODE_RIGHTBRACKET: return ELI_KEY_RIGHT_BRACKET;
    case SDL_SCANCODE_GRAVE:        return ELI_KEY_GRAVE_ACCENT;
    default:                        return ELI_KEY_NONE;
    }
}

/**
 * Translate an SDL3 key event's keycode + scancode to an eli_key.
 *
 * @param kc  SDL keycode (layout-dependent).
 * @param sc  SDL scancode (physical position).
 * @return    The eli_key, or ELI_KEY_NONE if unmapped.
 *
 * Thread-safe: yes  Reentrant: yes
 */
static inline eli_key eli_sdl3_key_from_event(SDL_Keycode kc, SDL_Scancode sc)
{
    eli_key k = eli_sdl3__keypad_key(sc);
    if (k != ELI_KEY_NONE)
        return k;
    k = eli_sdl3__named_key(kc);
    if (k != ELI_KEY_NONE)
        return k;
    if (kc >= SDLK_A && kc <= SDLK_Z)
        return (eli_key)(ELI_KEY_A + (int)(kc - SDLK_A));
    if (kc >= SDLK_0 && kc <= SDLK_9)
        return (eli_key)(ELI_KEY_0 + (int)(kc - SDLK_0));
    if (kc >= SDLK_F1 && kc <= SDLK_F12)
        return (eli_key)(ELI_KEY_F1 + (int)(kc - SDLK_F1));
    if (kc >= SDLK_F13 && kc <= SDLK_F24)
        return (eli_key)(ELI_KEY_F13 + (int)(kc - SDLK_F13));
    return eli_sdl3__punct_key(sc);
}

#endif /* ELI_SDL3_KEYMAP_H */
