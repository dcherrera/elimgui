/**
 * @file eli_input_keyboard.h
 * @brief Keyboard-state queries for elimgui: key up/down/pressed/released with
 *        typematic repeat, modifier tracking, key-chord matching, per-frame
 *        press counts, key names, and keyboard capture-intent overrides.
 *
 * Per-key down durations are advanced each frame by eli_input_backend.h; a key
 * is "pressed" on the frame its duration is exactly zero, and (with repeat)
 * again at the typematic delay/rate. Modifier chords combine the ELI_MOD_* bit
 * flags with an eli_key.
 *
 * @status Phase 4 keyboard queries in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_INPUT_ELI_INPUT_KEYBOARD_H
#define ELI_INPUT_ELI_INPUT_KEYBOARD_H

#include "../core/eli_core.h"

/* ---------------------------------------------------------------------------
 * Key chords: an eli_key optionally OR'd with modifier bit flags. Named key
 * values are all well below 1<<12, so key and modifier bits never collide.
 * ------------------------------------------------------------------------- */

/** A key plus optional modifier flags (see ELI_MOD_*). */
typedef int eli_key_chord;

#define ELI_MOD_NONE  0
#define ELI_MOD_CTRL  (1 << 12)
#define ELI_MOD_SHIFT (1 << 13)
#define ELI_MOD_ALT   (1 << 14)
#define ELI_MOD_SUPER (1 << 15)
#define ELI_MOD_MASK  0xF000

/* ---------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

/** True if a key index addresses a real named-key slot. */
static inline bool eli_key_is_valid(int key)
{
    return key > ELI_KEY_NONE && key < ELI_KEY_COUNT;
}

/**
 * Number of typematic repeat "presses" that occurred in the interval (t0, t1].
 * Mirrors Dear ImGui's CalcTypematicRepeatAmount so repeat timing matches.
 *
 * @param t0            Down-duration at the start of the interval (seconds).
 * @param t1            Down-duration at the end of the interval (seconds).
 * @param repeat_delay  Delay before the first repeat (seconds).
 * @param repeat_rate   Interval between subsequent repeats (seconds).
 * @return              Count of repeat events within the interval.
 */
static inline int eli_calc_typematic_repeat_amount(float t0, float t1,
                                                   float repeat_delay, float repeat_rate)
{
    if (t1 == 0.0f)
        return 1;
    if (t0 >= t1)
        return 0;
    if (repeat_rate <= 0.0f)
        return (t0 < repeat_delay && t1 >= repeat_delay) ? 1 : 0;
    int count_t0 = (t0 < repeat_delay) ? -1 : (int)((t0 - repeat_delay) / repeat_rate);
    int count_t1 = (t1 < repeat_delay) ? -1 : (int)((t1 - repeat_delay) / repeat_rate);
    return count_t1 - count_t0;
}

/** Current modifier bit mask derived from IO modifier flags. */
static inline int eli_get_key_mods(const eli_io *io)
{
    int mods = ELI_MOD_NONE;
    if (io->key_ctrl)  mods |= ELI_MOD_CTRL;
    if (io->key_shift) mods |= ELI_MOD_SHIFT;
    if (io->key_alt)   mods |= ELI_MOD_ALT;
    if (io->key_super) mods |= ELI_MOD_SUPER;
    return mods;
}

/* ---------------------------------------------------------------------------
 * Key state
 * ------------------------------------------------------------------------- */

/**
 * @param key  Key index (see eli_key).
 * @return     true while the key is held down.
 *
 * Thread-safe: no (reads current-context IO)
 * Reentrant: yes
 */
static inline bool eli_is_key_down(eli_key key)
{
    const eli_io *io = eli_get_io();
    return io && eli_key_is_valid(key) && io->keys_down[key];
}

/**
 * Test whether a key was pressed this frame, optionally including typematic
 * repeats while held.
 *
 * @param key     Key index.
 * @param repeat  If true, also report repeats at the IO delay/rate.
 * @return        true on the initial press frame (and repeat frames if enabled).
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline bool eli_is_key_pressed_ex(eli_key key, bool repeat)
{
    const eli_io *io = eli_get_io();
    if (!io || !eli_key_is_valid(key))
        return false;

    float t = io->key_down_duration[key];
    if (t < 0.0f)
        return false;
    if (t == 0.0f)
        return true;
    if (repeat && t > io->key_repeat_delay) {
        int amount = eli_calc_typematic_repeat_amount(t - io->delta_time, t,
                                                      io->key_repeat_delay, io->key_repeat_rate);
        return amount > 0;
    }
    return false;
}

/**
 * @param key  Key index.
 * @return     true on the initial press frame only (no repeat).
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline bool eli_is_key_pressed(eli_key key)
{
    return eli_is_key_pressed_ex(key, false);
}

/**
 * @param key  Key index.
 * @return     true on the frame the key transitions from down to up.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline bool eli_is_key_released(eli_key key)
{
    const eli_io *io = eli_get_io();
    if (!io || !eli_key_is_valid(key))
        return false;
    return !io->keys_down[key] && io->key_down_duration_prev[key] >= 0.0f;
}

/**
 * Test whether a modifier chord (mods + key) was pressed this frame. The current
 * modifier state must match the chord's modifiers exactly and the base key must
 * be pressed this frame.
 *
 * @param key_chord  ELI_MOD_* flags OR'd with an eli_key (e.g. ELI_SHORTCUT_COPY).
 * @return           true if the chord fired this frame.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline bool eli_is_key_chord_pressed(eli_key_chord key_chord)
{
    const eli_io *io = eli_get_io();
    if (!io)
        return false;

    int mods = key_chord & ELI_MOD_MASK;
    if (eli_get_key_mods(io) != mods)
        return false;

    int key = key_chord & ~ELI_MOD_MASK;
    if (key == ELI_KEY_NONE)
        return true;
    return eli_is_key_pressed_ex((eli_key)key, false);
}

/**
 * Number of key presses (including typematic repeats) accumulated this frame for
 * the given repeat timing.
 *
 * @param key           Key index.
 * @param repeat_delay  Delay before first repeat; negative uses the IO default.
 * @param repeat_rate   Interval between repeats; negative uses the IO default.
 * @return              Count of presses this frame (usually 0 or 1).
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline int eli_get_key_pressed_amount(eli_key key, float repeat_delay, float repeat_rate)
{
    const eli_io *io = eli_get_io();
    if (!io || !eli_key_is_valid(key) || !io->keys_down[key])
        return 0;
    if (repeat_delay < 0.0f)
        repeat_delay = io->key_repeat_delay;
    if (repeat_rate < 0.0f)
        repeat_rate = io->key_repeat_rate;
    float t = io->key_down_duration[key];
    return eli_calc_typematic_repeat_amount(t - io->delta_time, t, repeat_delay, repeat_rate);
}

/**
 * @param key  Key index.
 * @return     Human-readable, static name for the key ("None"/"Unknown" for the
 *             sentinel/out-of-range values). Never NULL; caller must not free.
 *
 * Thread-safe: yes (returns static strings)
 * Reentrant: yes
 */
static inline const char *eli_get_key_name(eli_key key)
{
    switch (key) {
    case ELI_KEY_NONE: return "None";
    case ELI_KEY_TAB: return "Tab";
    case ELI_KEY_LEFT_ARROW: return "LeftArrow";
    case ELI_KEY_RIGHT_ARROW: return "RightArrow";
    case ELI_KEY_UP_ARROW: return "UpArrow";
    case ELI_KEY_DOWN_ARROW: return "DownArrow";
    case ELI_KEY_PAGE_UP: return "PageUp";
    case ELI_KEY_PAGE_DOWN: return "PageDown";
    case ELI_KEY_HOME: return "Home";
    case ELI_KEY_END: return "End";
    case ELI_KEY_INSERT: return "Insert";
    case ELI_KEY_DELETE: return "Delete";
    case ELI_KEY_BACKSPACE: return "Backspace";
    case ELI_KEY_SPACE: return "Space";
    case ELI_KEY_ENTER: return "Enter";
    case ELI_KEY_ESCAPE: return "Escape";
    case ELI_KEY_APOSTROPHE: return "'";
    case ELI_KEY_COMMA: return ",";
    case ELI_KEY_MINUS: return "-";
    case ELI_KEY_PERIOD: return ".";
    case ELI_KEY_SLASH: return "/";
    case ELI_KEY_SEMICOLON: return ";";
    case ELI_KEY_EQUAL: return "=";
    case ELI_KEY_LEFT_BRACKET: return "[";
    case ELI_KEY_BACKSLASH: return "\\";
    case ELI_KEY_RIGHT_BRACKET: return "]";
    case ELI_KEY_GRAVE_ACCENT: return "`";
    case ELI_KEY_CAPS_LOCK: return "CapsLock";
    case ELI_KEY_SCROLL_LOCK: return "ScrollLock";
    case ELI_KEY_NUM_LOCK: return "NumLock";
    case ELI_KEY_PRINT_SCREEN: return "PrintScreen";
    case ELI_KEY_PAUSE: return "Pause";
    case ELI_KEY_KEYPAD_0: return "Keypad0";
    case ELI_KEY_KEYPAD_1: return "Keypad1";
    case ELI_KEY_KEYPAD_2: return "Keypad2";
    case ELI_KEY_KEYPAD_3: return "Keypad3";
    case ELI_KEY_KEYPAD_4: return "Keypad4";
    case ELI_KEY_KEYPAD_5: return "Keypad5";
    case ELI_KEY_KEYPAD_6: return "Keypad6";
    case ELI_KEY_KEYPAD_7: return "Keypad7";
    case ELI_KEY_KEYPAD_8: return "Keypad8";
    case ELI_KEY_KEYPAD_9: return "Keypad9";
    case ELI_KEY_KEYPAD_DECIMAL: return "KeypadDecimal";
    case ELI_KEY_KEYPAD_DIVIDE: return "KeypadDivide";
    case ELI_KEY_KEYPAD_MULTIPLY: return "KeypadMultiply";
    case ELI_KEY_KEYPAD_SUBTRACT: return "KeypadSubtract";
    case ELI_KEY_KEYPAD_ADD: return "KeypadAdd";
    case ELI_KEY_KEYPAD_ENTER: return "KeypadEnter";
    case ELI_KEY_KEYPAD_EQUAL: return "KeypadEqual";
    case ELI_KEY_LEFT_CTRL: return "LeftCtrl";
    case ELI_KEY_LEFT_SHIFT: return "LeftShift";
    case ELI_KEY_LEFT_ALT: return "LeftAlt";
    case ELI_KEY_LEFT_SUPER: return "LeftSuper";
    case ELI_KEY_RIGHT_CTRL: return "RightCtrl";
    case ELI_KEY_RIGHT_SHIFT: return "RightShift";
    case ELI_KEY_RIGHT_ALT: return "RightAlt";
    case ELI_KEY_RIGHT_SUPER: return "RightSuper";
    case ELI_KEY_MENU: return "Menu";
    case ELI_KEY_0: return "0";
    case ELI_KEY_1: return "1";
    case ELI_KEY_2: return "2";
    case ELI_KEY_3: return "3";
    case ELI_KEY_4: return "4";
    case ELI_KEY_5: return "5";
    case ELI_KEY_6: return "6";
    case ELI_KEY_7: return "7";
    case ELI_KEY_8: return "8";
    case ELI_KEY_9: return "9";
    case ELI_KEY_A: return "A";
    case ELI_KEY_B: return "B";
    case ELI_KEY_C: return "C";
    case ELI_KEY_D: return "D";
    case ELI_KEY_E: return "E";
    case ELI_KEY_F: return "F";
    case ELI_KEY_G: return "G";
    case ELI_KEY_H: return "H";
    case ELI_KEY_I: return "I";
    case ELI_KEY_J: return "J";
    case ELI_KEY_K: return "K";
    case ELI_KEY_L: return "L";
    case ELI_KEY_M: return "M";
    case ELI_KEY_N: return "N";
    case ELI_KEY_O: return "O";
    case ELI_KEY_P: return "P";
    case ELI_KEY_Q: return "Q";
    case ELI_KEY_R: return "R";
    case ELI_KEY_S: return "S";
    case ELI_KEY_T: return "T";
    case ELI_KEY_U: return "U";
    case ELI_KEY_V: return "V";
    case ELI_KEY_W: return "W";
    case ELI_KEY_X: return "X";
    case ELI_KEY_Y: return "Y";
    case ELI_KEY_Z: return "Z";
    case ELI_KEY_F1: return "F1";
    case ELI_KEY_F2: return "F2";
    case ELI_KEY_F3: return "F3";
    case ELI_KEY_F4: return "F4";
    case ELI_KEY_F5: return "F5";
    case ELI_KEY_F6: return "F6";
    case ELI_KEY_F7: return "F7";
    case ELI_KEY_F8: return "F8";
    case ELI_KEY_F9: return "F9";
    case ELI_KEY_F10: return "F10";
    case ELI_KEY_F11: return "F11";
    case ELI_KEY_F12: return "F12";
    case ELI_KEY_F13: return "F13";
    case ELI_KEY_F14: return "F14";
    case ELI_KEY_F15: return "F15";
    case ELI_KEY_F16: return "F16";
    case ELI_KEY_F17: return "F17";
    case ELI_KEY_F18: return "F18";
    case ELI_KEY_F19: return "F19";
    case ELI_KEY_F20: return "F20";
    case ELI_KEY_F21: return "F21";
    case ELI_KEY_F22: return "F22";
    case ELI_KEY_F23: return "F23";
    case ELI_KEY_F24: return "F24";
    case ELI_KEY_MOD_CTRL: return "ModCtrl";
    case ELI_KEY_MOD_SHIFT: return "ModShift";
    case ELI_KEY_MOD_ALT: return "ModAlt";
    case ELI_KEY_MOD_SUPER: return "ModSuper";
    default: return "Unknown";
    }
}

/**
 * Override whether elimgui wants to capture the keyboard next frame. Applied at
 * the next begin-frame.
 *
 * @param want_capture  Desired capture intent for the next frame.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_set_next_frame_want_capture_keyboard(bool want_capture)
{
    eli_io *io = eli_get_io();
    if (io)
        io->want_capture_keyboard_next_frame =
            want_capture ? ELI_CAPTURE_OVERRIDE_TRUE : ELI_CAPTURE_OVERRIDE_FALSE;
}

#endif /* ELI_INPUT_ELI_INPUT_KEYBOARD_H */
