/*
 * eli_input.h - Input System
 *
 * Mouse, keyboard, text input, and shortcut handling.
 */

#ifndef ELI_INPUT_H
#define ELI_INPUT_H

#include "elimgui.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * CONSTANTS
 *===========================================================================*/

/* Click detection timing (in seconds) */
#define ELI_MOUSE_DOUBLE_CLICK_TIME  0.30f
#define ELI_MOUSE_DOUBLE_CLICK_DIST  6.0f
#define ELI_MOUSE_DRAG_THRESHOLD     6.0f

/* Key repeat timing */
#define ELI_KEY_REPEAT_DELAY         0.275f
#define ELI_KEY_REPEAT_RATE          0.050f

/*============================================================================
 * INPUT STATE (stored in context)
 *===========================================================================*/

typedef struct eli_input_state {
    /* Mouse state */
    eli_vec2 mouse_pos_prev;
    bool mouse_clicked[ELI_MOUSE_BUTTON_COUNT];
    bool mouse_double_clicked[ELI_MOUSE_BUTTON_COUNT];
    bool mouse_released[ELI_MOUSE_BUTTON_COUNT];
    int mouse_clicked_count[ELI_MOUSE_BUTTON_COUNT];
    float mouse_clicked_time[ELI_MOUSE_BUTTON_COUNT];
    eli_vec2 mouse_clicked_pos[ELI_MOUSE_BUTTON_COUNT];
    float mouse_down_duration[ELI_MOUSE_BUTTON_COUNT];
    float mouse_down_duration_prev[ELI_MOUSE_BUTTON_COUNT];
    eli_vec2 mouse_drag_max_distance_abs[ELI_MOUSE_BUTTON_COUNT];
    float mouse_drag_max_distance_sqr[ELI_MOUSE_BUTTON_COUNT];

    /* Keyboard state */
    bool keys_down_prev[ELI_KEY_DATA_SIZE];

    /* Mouse cursor */
    eli_mouse_cursor mouse_cursor;
    eli_mouse_cursor mouse_cursor_prev;

    /* Popup mouse position */
    eli_vec2 mouse_pos_on_opening_popup;
} eli_input_state;

/*============================================================================
 * MOUSE QUERIES - POSITION
 *===========================================================================*/

/* Get current mouse position */
static inline eli_vec2 eli_get_mouse_pos(void) {
    eli_io* io = eli_get_io();
    return io ? io->mouse_pos : eli_make_vec2(-FLT_MAX, -FLT_MAX);
}

/* Check if mouse position is valid (not -FLT_MAX) */
static inline bool eli_is_mouse_pos_valid(const eli_vec2* pos) {
    const eli_vec2* p = pos ? pos : &eli_get_io()->mouse_pos;
    return p->x >= -FLT_MAX * 0.5f;
}

/* Get mouse position on opening current popup (for context menus) */
static inline eli_vec2 eli_get_mouse_pos_on_opening_current_popup(void) {
    eli_context* ctx = eli_get_current_context();
    /* For now, return current mouse pos - will be set when popups are implemented */
    return ctx ? ctx->io.mouse_pos : eli_make_vec2(-FLT_MAX, -FLT_MAX);
}

/*============================================================================
 * MOUSE QUERIES - BUTTONS
 *===========================================================================*/

/* Is mouse button held down? */
static inline bool eli_is_mouse_down(eli_mouse_button button) {
    eli_io* io = eli_get_io();
    if (!io || button < 0 || button >= ELI_MOUSE_BUTTON_COUNT) return false;
    return io->mouse_down[button];
}

/* Was mouse button clicked this frame? (went from !down to down) */
static inline bool eli_is_mouse_clicked(eli_mouse_button button, bool repeat) {
    eli_io* io = eli_get_io();
    if (!io || button < 0 || button >= ELI_MOUSE_BUTTON_COUNT) return false;

    float t = io->keys_data[ELI_KEY_MOUSE_LEFT + button].down_duration;
    if (t == 0.0f) return true;

    if (repeat && t > ELI_KEY_REPEAT_DELAY) {
        /* Key repeat logic */
        float delay = ELI_KEY_REPEAT_DELAY;
        float rate = ELI_KEY_REPEAT_RATE;
        float prev_t = io->keys_data[ELI_KEY_MOUSE_LEFT + button].down_duration_prev;
        if (t > delay && prev_t <= delay) return true;
        if (t > delay) {
            float mod_prev = fmodf(prev_t - delay, rate);
            float mod_curr = fmodf(t - delay, rate);
            if (mod_curr < mod_prev) return true;
        }
    }
    return false;
}

/* Was mouse button released this frame? (went from down to !down) */
static inline bool eli_is_mouse_released(eli_mouse_button button) {
    eli_io* io = eli_get_io();
    if (!io || button < 0 || button >= ELI_MOUSE_BUTTON_COUNT) return false;

    eli_key_data* key = &io->keys_data[ELI_KEY_MOUSE_LEFT + button];
    return key->down_duration_prev >= 0.0f && key->down_duration < 0.0f;
}

/* Was mouse button double-clicked this frame? */
static inline bool eli_is_mouse_double_clicked(eli_mouse_button button) {
    eli_io* io = eli_get_io();
    if (!io || button < 0 || button >= ELI_MOUSE_BUTTON_COUNT) return false;

    /* Simplified: check if clicked twice quickly */
    /* Full implementation would track click times */
    return false;  /* TODO: Implement with click time tracking */
}

/* Get number of clicks (for detecting triple-click, etc.) */
static inline int eli_get_mouse_clicked_count(eli_mouse_button button) {
    eli_io* io = eli_get_io();
    if (!io || button < 0 || button >= ELI_MOUSE_BUTTON_COUNT) return 0;
    /* TODO: Implement with click tracking */
    return io->mouse_down[button] ? 1 : 0;
}

/* Is any mouse button down? */
static inline bool eli_is_any_mouse_down(void) {
    eli_io* io = eli_get_io();
    if (!io) return false;
    for (int i = 0; i < ELI_MOUSE_BUTTON_COUNT; i++) {
        if (io->mouse_down[i]) return true;
    }
    return false;
}

/*============================================================================
 * MOUSE QUERIES - HOVERING
 *===========================================================================*/

/* Is mouse hovering a given rect? (in screen coords) */
static inline bool eli_is_mouse_hovering_rect(eli_vec2 r_min, eli_vec2 r_max, bool clip) {
    eli_io* io = eli_get_io();
    if (!io) return false;

    eli_vec2 mp = io->mouse_pos;
    if (!eli_is_mouse_pos_valid(&mp)) return false;

    /* Basic rect test */
    if (mp.x < r_min.x || mp.x >= r_max.x) return false;
    if (mp.y < r_min.y || mp.y >= r_max.y) return false;

    /* TODO: If clip is true, also check against window clip rect */
    (void)clip;

    return true;
}

/*============================================================================
 * MOUSE QUERIES - DRAGGING
 *===========================================================================*/

/* Is mouse dragging with given button? */
static inline bool eli_is_mouse_dragging(eli_mouse_button button, float lock_threshold) {
    eli_io* io = eli_get_io();
    if (!io || button < 0 || button >= ELI_MOUSE_BUTTON_COUNT) return false;
    if (!io->mouse_down[button]) return false;

    if (lock_threshold < 0.0f) lock_threshold = ELI_MOUSE_DRAG_THRESHOLD;

    eli_key_data* key = &io->keys_data[ELI_KEY_MOUSE_LEFT + button];
    if (key->down_duration < 0.0f) return false;

    /* Check if mouse has moved beyond threshold since button was pressed */
    /* Simplified: just check current delta magnitude */
    float dist_sqr = io->mouse_delta.x * io->mouse_delta.x + io->mouse_delta.y * io->mouse_delta.y;
    return dist_sqr > 0.0f || key->down_duration > 0.0f;
}

/* Get mouse drag delta */
static inline eli_vec2 eli_get_mouse_drag_delta(eli_mouse_button button, float lock_threshold) {
    eli_io* io = eli_get_io();
    if (!io || button < 0 || button >= ELI_MOUSE_BUTTON_COUNT)
        return eli_make_vec2(0, 0);

    if (lock_threshold < 0.0f) lock_threshold = ELI_MOUSE_DRAG_THRESHOLD;

    if (io->mouse_down[button] || io->keys_data[ELI_KEY_MOUSE_LEFT + button].down_duration_prev >= 0.0f) {
        return io->mouse_delta;
    }
    return eli_make_vec2(0, 0);
}

/* Reset mouse drag delta */
static inline void eli_reset_mouse_drag_delta(eli_mouse_button button) {
    eli_io* io = eli_get_io();
    if (!io || button < 0 || button >= ELI_MOUSE_BUTTON_COUNT) return;
    /* Reset would clear accumulated drag delta - for now, just clear delta */
    io->mouse_delta = eli_make_vec2(0, 0);
}

/*============================================================================
 * MOUSE QUERIES - CURSOR
 *===========================================================================*/

/* Get current mouse cursor type */
static inline eli_mouse_cursor eli_get_mouse_cursor(void) {
    eli_io* io = eli_get_io();
    return io ? io->mouse_cursor : ELI_MOUSE_CURSOR_ARROW;
}

/* Set mouse cursor type (will be rendered by application) */
static inline void eli_set_mouse_cursor(eli_mouse_cursor cursor) {
    eli_io* io = eli_get_io();
    if (io) io->mouse_cursor = cursor;
}

/*============================================================================
 * MOUSE QUERIES - CAPTURE
 *===========================================================================*/

/* Request mouse capture for next frame */
static inline void eli_set_next_frame_want_capture_mouse(bool want_capture) {
    eli_io* io = eli_get_io();
    if (io) io->want_capture_mouse = want_capture;
}

/*============================================================================
 * KEYBOARD QUERIES - KEY STATE
 *===========================================================================*/

/* Convert eli_key to keys_data index */
static inline int eli_key_to_index(eli_key key) {
    if (key >= ELI_KEY_NAMED_KEY_BEGIN && key < ELI_KEY_NAMED_KEY_END)
        return key - ELI_KEY_NAMED_KEY_BEGIN;
    return -1;
}

/* Is key currently held down? */
static inline bool eli_is_key_down(eli_key key) {
    eli_io* io = eli_get_io();
    if (!io) return false;
    int idx = eli_key_to_index(key);
    if (idx < 0 || idx >= ELI_KEY_DATA_SIZE) return false;
    return io->keys_data[idx].down;
}

/* Was key pressed this frame? (went from !down to down) */
static inline bool eli_is_key_pressed(eli_key key, bool repeat) {
    eli_io* io = eli_get_io();
    if (!io) return false;
    int idx = eli_key_to_index(key);
    if (idx < 0 || idx >= ELI_KEY_DATA_SIZE) return false;

    eli_key_data* kd = &io->keys_data[idx];
    float t = kd->down_duration;

    if (t == 0.0f) return true;

    if (repeat && t > ELI_KEY_REPEAT_DELAY) {
        float prev_t = kd->down_duration_prev;
        if (t > ELI_KEY_REPEAT_DELAY && prev_t <= ELI_KEY_REPEAT_DELAY) return true;
        if (t > ELI_KEY_REPEAT_DELAY) {
            float mod_prev = fmodf(prev_t - ELI_KEY_REPEAT_DELAY, ELI_KEY_REPEAT_RATE);
            float mod_curr = fmodf(t - ELI_KEY_REPEAT_DELAY, ELI_KEY_REPEAT_RATE);
            if (mod_curr < mod_prev) return true;
        }
    }
    return false;
}

/* Was key released this frame? (went from down to !down) */
static inline bool eli_is_key_released(eli_key key) {
    eli_io* io = eli_get_io();
    if (!io) return false;
    int idx = eli_key_to_index(key);
    if (idx < 0 || idx >= ELI_KEY_DATA_SIZE) return false;

    eli_key_data* kd = &io->keys_data[idx];
    return kd->down_duration_prev >= 0.0f && kd->down_duration < 0.0f;
}

/* Get how long the key has been held (in seconds), or -1 if not held */
static inline float eli_get_key_pressed_amount(eli_key key) {
    eli_io* io = eli_get_io();
    if (!io) return -1.0f;
    int idx = eli_key_to_index(key);
    if (idx < 0 || idx >= ELI_KEY_DATA_SIZE) return -1.0f;
    return io->keys_data[idx].down_duration;
}

/*============================================================================
 * KEYBOARD QUERIES - MODIFIERS
 *===========================================================================*/

/* Check if modifier key is held */
static inline bool eli_is_key_mod_down(int mod) {
    eli_io* io = eli_get_io();
    if (!io) return false;

    if (mod & ELI_MOD_CTRL)  if (!io->key_ctrl) return false;
    if (mod & ELI_MOD_SHIFT) if (!io->key_shift) return false;
    if (mod & ELI_MOD_ALT)   if (!io->key_alt) return false;
    if (mod & ELI_MOD_SUPER) if (!io->key_super) return false;
    return true;
}

/* Check for key chord (key + modifiers) */
static inline bool eli_is_key_chord_pressed(int key_chord) {
    eli_key key = (eli_key)(key_chord & ~ELI_MOD_MASK);
    int mods = key_chord & ELI_MOD_MASK;

    if (!eli_is_key_mod_down(mods)) return false;
    return eli_is_key_pressed(key, false);
}

/*============================================================================
 * KEYBOARD QUERIES - KEY NAME
 *===========================================================================*/

/* Get human-readable key name */
static inline const char* eli_get_key_name(eli_key key) {
    switch (key) {
        case ELI_KEY_TAB: return "Tab";
        case ELI_KEY_LEFT_ARROW: return "Left";
        case ELI_KEY_RIGHT_ARROW: return "Right";
        case ELI_KEY_UP_ARROW: return "Up";
        case ELI_KEY_DOWN_ARROW: return "Down";
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
        default: return "Unknown";
    }
}

/*============================================================================
 * KEYBOARD QUERIES - CAPTURE
 *===========================================================================*/

/* Request keyboard capture for next frame */
static inline void eli_set_next_frame_want_capture_keyboard(bool want_capture) {
    eli_io* io = eli_get_io();
    if (io) io->want_capture_keyboard = want_capture;
}

/*============================================================================
 * TEXT INPUT
 *===========================================================================*/

/* Add a character to the input queue (called by platform backend) */
static inline void eli_io_add_input_character(uint32_t c) {
    eli_io* io = eli_get_io();
    if (!io) return;
    if (io->input_queue_chars_count < 16) {
        io->input_queue_chars[io->input_queue_chars_count++] = c;
    }
}

/* Add UTF-8 string to input queue */
static inline void eli_io_add_input_characters_utf8(const char* str) {
    if (!str) return;
    while (*str) {
        uint32_t c = (unsigned char)*str;
        if (c < 0x80) {
            eli_io_add_input_character(c);
            str++;
        } else if ((c & 0xE0) == 0xC0) {
            /* 2-byte UTF-8 */
            c = ((str[0] & 0x1F) << 6) | (str[1] & 0x3F);
            eli_io_add_input_character(c);
            str += 2;
        } else if ((c & 0xF0) == 0xE0) {
            /* 3-byte UTF-8 */
            c = ((str[0] & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F);
            eli_io_add_input_character(c);
            str += 3;
        } else if ((c & 0xF8) == 0xF0) {
            /* 4-byte UTF-8 */
            c = ((str[0] & 0x07) << 18) | ((str[1] & 0x3F) << 12) | ((str[2] & 0x3F) << 6) | (str[3] & 0x3F);
            eli_io_add_input_character(c);
            str += 4;
        } else {
            str++;  /* Invalid UTF-8, skip */
        }
    }
}

/* Clear input queue (called at end of frame) */
static inline void eli_io_clear_input_chars(void) {
    eli_io* io = eli_get_io();
    if (io) io->input_queue_chars_count = 0;
}

/*============================================================================
 * INPUT STATE UPDATE (called by platform backend)
 *===========================================================================*/

/* Add mouse position event */
static inline void eli_io_add_mouse_pos_event(float x, float y) {
    eli_io* io = eli_get_io();
    if (!io) return;

    /* Calculate delta before updating position */
    if (io->mouse_pos.x >= -FLT_MAX * 0.5f) {
        io->mouse_delta.x = x - io->mouse_pos.x;
        io->mouse_delta.y = y - io->mouse_pos.y;
    }

    io->mouse_pos.x = x;
    io->mouse_pos.y = y;
}

/* Add mouse button event */
static inline void eli_io_add_mouse_button_event(eli_mouse_button button, bool down) {
    eli_io* io = eli_get_io();
    if (!io || button < 0 || button >= ELI_MOUSE_BUTTON_COUNT) return;

    io->mouse_down[button] = down;

    /* Update key data for mouse buttons */
    int key_idx = ELI_KEY_MOUSE_LEFT + button - ELI_KEY_NAMED_KEY_BEGIN;
    if (key_idx >= 0 && key_idx < ELI_KEY_DATA_SIZE) {
        io->keys_data[key_idx].down = down;
    }
}

/* Add mouse wheel event */
static inline void eli_io_add_mouse_wheel_event(float wheel_x, float wheel_y) {
    eli_io* io = eli_get_io();
    if (!io) return;
    io->mouse_wheel_h += wheel_x;
    io->mouse_wheel += wheel_y;
}

/* Add key event */
static inline void eli_io_add_key_event(eli_key key, bool down) {
    eli_io* io = eli_get_io();
    if (!io) return;

    int idx = eli_key_to_index(key);
    if (idx >= 0 && idx < ELI_KEY_DATA_SIZE) {
        io->keys_data[idx].down = down;
    }

    /* Update modifier flags */
    if (key == ELI_KEY_LEFT_CTRL || key == ELI_KEY_RIGHT_CTRL)
        io->key_ctrl = down || eli_is_key_down(key == ELI_KEY_LEFT_CTRL ? ELI_KEY_RIGHT_CTRL : ELI_KEY_LEFT_CTRL);
    if (key == ELI_KEY_LEFT_SHIFT || key == ELI_KEY_RIGHT_SHIFT)
        io->key_shift = down || eli_is_key_down(key == ELI_KEY_LEFT_SHIFT ? ELI_KEY_RIGHT_SHIFT : ELI_KEY_LEFT_SHIFT);
    if (key == ELI_KEY_LEFT_ALT || key == ELI_KEY_RIGHT_ALT)
        io->key_alt = down || eli_is_key_down(key == ELI_KEY_LEFT_ALT ? ELI_KEY_RIGHT_ALT : ELI_KEY_LEFT_ALT);
    if (key == ELI_KEY_LEFT_SUPER || key == ELI_KEY_RIGHT_SUPER)
        io->key_super = down || eli_is_key_down(key == ELI_KEY_LEFT_SUPER ? ELI_KEY_RIGHT_SUPER : ELI_KEY_LEFT_SUPER);
}

/* Add key with analog value (for gamepad) */
static inline void eli_io_add_key_analog_event(eli_key key, bool down, float analog_value) {
    eli_io* io = eli_get_io();
    if (!io) return;

    int idx = eli_key_to_index(key);
    if (idx >= 0 && idx < ELI_KEY_DATA_SIZE) {
        io->keys_data[idx].down = down;
        io->keys_data[idx].analog_value = analog_value;
    }
}

/*============================================================================
 * INPUT UPDATE (called each frame internally)
 *===========================================================================*/

/* Update input state (called at start of frame) */
static inline void eli_input_update_begin_frame(void) {
    eli_io* io = eli_get_io();
    if (!io) return;

    float dt = io->delta_time;

    /* Update key down durations */
    for (int i = 0; i < ELI_KEY_DATA_SIZE; i++) {
        eli_key_data* kd = &io->keys_data[i];
        kd->down_duration_prev = kd->down_duration;
        kd->down_duration = kd->down ? (kd->down_duration < 0.0f ? 0.0f : kd->down_duration + dt) : -1.0f;
    }
}

/* End frame cleanup */
static inline void eli_input_update_end_frame(void) {
    eli_io* io = eli_get_io();
    if (!io) return;

    /* Clear mouse wheel */
    io->mouse_wheel = 0.0f;
    io->mouse_wheel_h = 0.0f;

    /* Clear input characters */
    eli_io_clear_input_chars();
}

/*============================================================================
 * SHORTCUTS
 *===========================================================================*/

/* Check if a shortcut was pressed (key + modifiers) */
static inline bool eli_shortcut(int key_chord, bool repeat) {
    eli_key key = (eli_key)(key_chord & ~ELI_MOD_MASK);
    int mods = key_chord & ELI_MOD_MASK;

    if (!eli_is_key_mod_down(mods)) return false;
    return eli_is_key_pressed(key, repeat);
}

/* Common shortcuts */
#define ELI_SHORTCUT_UNDO   (ELI_KEY_Z | ELI_MOD_CTRL)
#define ELI_SHORTCUT_REDO   (ELI_KEY_Y | ELI_MOD_CTRL)
#define ELI_SHORTCUT_CUT    (ELI_KEY_X | ELI_MOD_CTRL)
#define ELI_SHORTCUT_COPY   (ELI_KEY_C | ELI_MOD_CTRL)
#define ELI_SHORTCUT_PASTE  (ELI_KEY_V | ELI_MOD_CTRL)
#define ELI_SHORTCUT_SELECT_ALL (ELI_KEY_A | ELI_MOD_CTRL)

/*============================================================================
 * CLIPBOARD (stub - requires JS interop)
 *===========================================================================*/

/* Clipboard function pointers (set by platform backend) */
typedef const char* (*eli_get_clipboard_fn)(void* user_data);
typedef void (*eli_set_clipboard_fn)(void* user_data, const char* text);

static eli_get_clipboard_fn g_get_clipboard_fn = NULL;
static eli_set_clipboard_fn g_set_clipboard_fn = NULL;
static void* g_clipboard_user_data = NULL;

/* Set clipboard callbacks */
static inline void eli_set_clipboard_callbacks(
    eli_get_clipboard_fn get_fn,
    eli_set_clipboard_fn set_fn,
    void* user_data)
{
    g_get_clipboard_fn = get_fn;
    g_set_clipboard_fn = set_fn;
    g_clipboard_user_data = user_data;
}

/* Get clipboard text */
static inline const char* eli_get_clipboard_text(void) {
    if (g_get_clipboard_fn) {
        return g_get_clipboard_fn(g_clipboard_user_data);
    }
    return "";
}

/* Set clipboard text */
static inline void eli_set_clipboard_text(const char* text) {
    if (g_set_clipboard_fn) {
        g_set_clipboard_fn(g_clipboard_user_data, text);
    }
}

#ifdef __cplusplus
}
#endif

#endif /* ELI_INPUT_H */
