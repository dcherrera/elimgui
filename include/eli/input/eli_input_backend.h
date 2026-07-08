/**
 * @file eli_input_backend.h
 * @brief Backend-facing input integration for elimgui: the event adders a host
 *        calls (mouse pos/button/wheel, key, analog key) and the per-frame
 *        update that drains the queue and derives click/press/drag/duration
 *        state (eli_input_update_begin_frame / eli_input_update_end_frame).
 *
 * Backends push raw events during their platform callbacks; elimgui applies them
 * in order at begin-frame, then computes derived state widgets read. End-frame
 * rolls per-frame accumulators (previous mouse position, wheel, text queue).
 *
 * @status Phase 4 backend integration in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_INPUT_ELI_INPUT_BACKEND_H
#define ELI_INPUT_ELI_INPUT_BACKEND_H

#include "eli_input_mouse.h"
#include "eli_input_keyboard.h"
#include "eli_input_text.h"

/* Default input tunables (mirror Dear ImGui's ImGuiIO defaults). */
#define ELI_INPUT_DEFAULT_DOUBLE_CLICK_TIME     0.30f
#define ELI_INPUT_DEFAULT_DOUBLE_CLICK_MAX_DIST 6.0f
#define ELI_INPUT_DEFAULT_DRAG_THRESHOLD        6.0f
#define ELI_INPUT_DEFAULT_KEY_REPEAT_DELAY      0.275f
#define ELI_INPUT_DEFAULT_KEY_REPEAT_RATE       0.050f

/* ---------------------------------------------------------------------------
 * Configuration bootstrap
 * ------------------------------------------------------------------------- */

/**
 * Apply input tunable defaults and initialize down-durations to the "not down"
 * sentinel (-1). Idempotent: runs once per IO (guarded by input_config_initialized).
 * Backends may call it explicitly; begin-frame calls it lazily.
 *
 * @param io  IO block to initialize (non-NULL).
 */
static inline void eli_input_init_io(eli_io *io)
{
    if (io->input_config_initialized)
        return;

    io->mouse_double_click_time = ELI_INPUT_DEFAULT_DOUBLE_CLICK_TIME;
    io->mouse_double_click_max_dist = ELI_INPUT_DEFAULT_DOUBLE_CLICK_MAX_DIST;
    io->mouse_drag_threshold = ELI_INPUT_DEFAULT_DRAG_THRESHOLD;
    io->key_repeat_delay = ELI_INPUT_DEFAULT_KEY_REPEAT_DELAY;
    io->key_repeat_rate = ELI_INPUT_DEFAULT_KEY_REPEAT_RATE;
    io->mouse_cursor = ELI_MOUSE_CURSOR_ARROW;

    for (int i = 0; i < ELI_MOUSE_BUTTON_COUNT; i++) {
        io->mouse_down_duration[i] = -1.0f;
        io->mouse_down_duration_prev[i] = -1.0f;
    }
    for (int k = 0; k < ELI_KEY_COUNT; k++) {
        io->key_down_duration[k] = -1.0f;
        io->key_down_duration_prev[k] = -1.0f;
    }

    io->input_config_initialized = true;
}

/* ---------------------------------------------------------------------------
 * Event adders (called by the host backend)
 * ------------------------------------------------------------------------- */

/** Append one event to the queue if capacity remains; returns the slot or NULL. */
static inline eli_input_event *eli_input_push_event(eli_io *io, eli_input_event_type type)
{
    if (io->input_events_count >= ELI_INPUT_EVENT_QUEUE_SIZE)
        return NULL;
    eli_input_event *e = &io->input_events[io->input_events_count++];
    e->type = type;
    return e;
}

/**
 * Queue a mouse-position update.
 *
 * @param x  New mouse X (top-left origin), or an invalid sentinel when offscreen.
 * @param y  New mouse Y.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_io_add_mouse_pos_event(float x, float y)
{
    eli_io *io = eli_get_io();
    if (!io)
        return;
    eli_input_event *e = eli_input_push_event(io, ELI_INPUT_EVENT_TYPE_MOUSE_POS);
    if (e) {
        e->data.mouse_pos.x = x;
        e->data.mouse_pos.y = y;
    }
}

/**
 * Queue a mouse-button transition.
 *
 * @param button  Mouse button index (0..ELI_MOUSE_BUTTON_COUNT-1).
 * @param down    true on press, false on release.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_io_add_mouse_button_event(int button, bool down)
{
    eli_io *io = eli_get_io();
    if (!io || !eli_mouse_button_is_valid(button))
        return;
    eli_input_event *e = eli_input_push_event(io, ELI_INPUT_EVENT_TYPE_MOUSE_BUTTON);
    if (e) {
        e->data.mouse_button.button = button;
        e->data.mouse_button.down = down;
    }
}

/**
 * Queue a mouse-wheel delta (accumulated into this frame's wheel totals).
 *
 * @param wheel_x  Horizontal wheel delta.
 * @param wheel_y  Vertical wheel delta.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_io_add_mouse_wheel_event(float wheel_x, float wheel_y)
{
    eli_io *io = eli_get_io();
    if (!io)
        return;
    eli_input_event *e = eli_input_push_event(io, ELI_INPUT_EVENT_TYPE_MOUSE_WHEEL);
    if (e) {
        e->data.mouse_wheel.x = wheel_x;
        e->data.mouse_wheel.y = wheel_y;
    }
}

/**
 * Queue a keyboard key transition.
 *
 * @param key   Key index (see eli_key).
 * @param down  true on press, false on release.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_io_add_key_event(eli_key key, bool down)
{
    eli_io *io = eli_get_io();
    if (!io || !eli_key_is_valid(key))
        return;
    eli_input_event *e = eli_input_push_event(io, ELI_INPUT_EVENT_TYPE_KEY);
    if (e) {
        e->data.key.key = key;
        e->data.key.down = down;
        e->data.key.analog_value = down ? 1.0f : 0.0f;
    }
}

/**
 * Queue a keyboard/gamepad key transition carrying an analog value.
 *
 * @param key    Key index.
 * @param down   true if the key is considered pressed.
 * @param value  Analog value in [0,1] (e.g. trigger pressure).
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_io_add_key_analog_event(eli_key key, bool down, float value)
{
    eli_io *io = eli_get_io();
    if (!io || !eli_key_is_valid(key))
        return;
    eli_input_event *e = eli_input_push_event(io, ELI_INPUT_EVENT_TYPE_KEY);
    if (e) {
        e->data.key.key = key;
        e->data.key.down = down;
        e->data.key.analog_value = value;
    }
}

/* ---------------------------------------------------------------------------
 * Per-frame update (internal helpers)
 * ------------------------------------------------------------------------- */

/** Apply queued events to the live IO state, then clear the queue. */
static inline void eli_input_process_events(eli_io *io)
{
    for (int i = 0; i < io->input_events_count; i++) {
        eli_input_event *e = &io->input_events[i];
        switch (e->type) {
        case ELI_INPUT_EVENT_TYPE_MOUSE_POS:
            io->mouse_pos = eli_make_vec2(e->data.mouse_pos.x, e->data.mouse_pos.y);
            break;
        case ELI_INPUT_EVENT_TYPE_MOUSE_BUTTON:
            if (eli_mouse_button_is_valid(e->data.mouse_button.button))
                io->mouse_down[e->data.mouse_button.button] = e->data.mouse_button.down;
            break;
        case ELI_INPUT_EVENT_TYPE_MOUSE_WHEEL:
            io->mouse_wheel_h += e->data.mouse_wheel.x;
            io->mouse_wheel += e->data.mouse_wheel.y;
            break;
        case ELI_INPUT_EVENT_TYPE_KEY:
            if (eli_key_is_valid(e->data.key.key)) {
                io->keys_down[e->data.key.key] = e->data.key.down;
                io->keys_analog[e->data.key.key] = e->data.key.analog_value;
            }
            break;
        case ELI_INPUT_EVENT_TYPE_TEXT:
            eli_io_push_input_char_u16(io, e->data.text.c);
            break;
        case ELI_INPUT_EVENT_TYPE_NONE:
        default:
            break;
        }
    }
    io->input_events_count = 0;
}

/** Derive per-frame mouse state (delta, clicked/released, counts, durations). */
static inline void eli_input_update_mouse(eli_io *io)
{
    float dt = io->delta_time;

    if (eli_mouse_pos_is_valid(io->mouse_pos) && eli_mouse_pos_is_valid(io->mouse_pos_prev))
        io->mouse_delta = eli_vec2_sub(io->mouse_pos, io->mouse_pos_prev);
    else
        io->mouse_delta = eli_make_vec2(0.0f, 0.0f);

    for (int i = 0; i < ELI_MOUSE_BUTTON_COUNT; i++) {
        io->mouse_clicked[i] = io->mouse_down[i] && io->mouse_down_duration[i] < 0.0f;
        io->mouse_clicked_count[i] = 0;
        io->mouse_released[i] = !io->mouse_down[i] && io->mouse_down_duration[i] >= 0.0f;
        if (io->mouse_released[i])
            io->mouse_released_time[i] = io->input_time;
        io->mouse_down_duration_prev[i] = io->mouse_down_duration[i];
        io->mouse_down_duration[i] = io->mouse_down[i]
            ? (io->mouse_down_duration[i] < 0.0f ? 0.0f : io->mouse_down_duration[i] + dt)
            : -1.0f;

        if (io->mouse_clicked[i]) {
            bool is_repeated_click = false;
            if ((float)(io->input_time - io->mouse_clicked_time[i]) < io->mouse_double_click_time) {
                eli_vec2 d = eli_mouse_pos_is_valid(io->mouse_pos)
                    ? eli_vec2_sub(io->mouse_pos, io->mouse_clicked_pos[i])
                    : eli_make_vec2(0.0f, 0.0f);
                if (d.x * d.x + d.y * d.y <
                    io->mouse_double_click_max_dist * io->mouse_double_click_max_dist)
                    is_repeated_click = true;
            }
            io->mouse_clicked_last_count[i] =
                is_repeated_click ? (uint16_t)(io->mouse_clicked_last_count[i] + 1) : (uint16_t)1;
            io->mouse_clicked_time[i] = io->input_time;
            io->mouse_clicked_pos[i] = io->mouse_pos;
            io->mouse_clicked_count[i] = io->mouse_clicked_last_count[i];
            io->mouse_drag_max_distance_sqr[i] = 0.0f;
        } else if (io->mouse_down[i]) {
            eli_vec2 d = eli_mouse_pos_is_valid(io->mouse_pos)
                ? eli_vec2_sub(io->mouse_pos, io->mouse_clicked_pos[i])
                : eli_make_vec2(0.0f, 0.0f);
            float d2 = d.x * d.x + d.y * d.y;
            io->mouse_drag_max_distance_sqr[i] = eli_max_f(io->mouse_drag_max_distance_sqr[i], d2);
        }

        io->mouse_double_clicked[i] = (io->mouse_clicked_count[i] == 2);
    }
}

/** Derive per-frame keyboard state (modifiers, per-key down durations). */
static inline void eli_input_update_keyboard(eli_io *io)
{
    float dt = io->delta_time;

    io->key_ctrl  = io->keys_down[ELI_KEY_LEFT_CTRL]  || io->keys_down[ELI_KEY_RIGHT_CTRL];
    io->key_shift = io->keys_down[ELI_KEY_LEFT_SHIFT] || io->keys_down[ELI_KEY_RIGHT_SHIFT];
    io->key_alt   = io->keys_down[ELI_KEY_LEFT_ALT]   || io->keys_down[ELI_KEY_RIGHT_ALT];
    io->key_super = io->keys_down[ELI_KEY_LEFT_SUPER] || io->keys_down[ELI_KEY_RIGHT_SUPER];
    io->keys_down[ELI_KEY_MOD_CTRL]  = io->key_ctrl;
    io->keys_down[ELI_KEY_MOD_SHIFT] = io->key_shift;
    io->keys_down[ELI_KEY_MOD_ALT]   = io->key_alt;
    io->keys_down[ELI_KEY_MOD_SUPER] = io->key_super;

    for (int k = 0; k < ELI_KEY_COUNT; k++) {
        io->key_down_duration_prev[k] = io->key_down_duration[k];
        io->key_down_duration[k] = io->keys_down[k]
            ? (io->key_down_duration[k] < 0.0f ? 0.0f : io->key_down_duration[k] + dt)
            : -1.0f;
    }
}

/* ---------------------------------------------------------------------------
 * Per-frame update (public entry points)
 * ------------------------------------------------------------------------- */

/**
 * Begin-of-frame input update: lazily applies config defaults, advances the
 * input clock by io.delta_time, drains the event queue into live IO state, and
 * derives mouse/keyboard per-frame state. Also resets the requested cursor to
 * ELI_MOUSE_CURSOR_ARROW and applies any next-frame capture overrides.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_input_update_begin_frame(void)
{
    eli_io *io = eli_get_io();
    if (!io)
        return;

    eli_input_init_io(io);
    io->input_time += (double)io->delta_time;

    eli_input_process_events(io);
    io->mouse_cursor = ELI_MOUSE_CURSOR_ARROW;

    eli_input_update_mouse(io);
    eli_input_update_keyboard(io);

    if (io->want_capture_mouse_next_frame != ELI_CAPTURE_OVERRIDE_NONE) {
        io->want_capture_mouse = (io->want_capture_mouse_next_frame == ELI_CAPTURE_OVERRIDE_TRUE);
        io->want_capture_mouse_next_frame = ELI_CAPTURE_OVERRIDE_NONE;
    }
    if (io->want_capture_keyboard_next_frame != ELI_CAPTURE_OVERRIDE_NONE) {
        io->want_capture_keyboard = (io->want_capture_keyboard_next_frame == ELI_CAPTURE_OVERRIDE_TRUE);
        io->want_capture_keyboard_next_frame = ELI_CAPTURE_OVERRIDE_NONE;
    }
}

/**
 * End-of-frame input update: rolls the previous mouse position, clears the
 * per-frame wheel accumulators, and empties the text-input queue for next frame.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_input_update_end_frame(void)
{
    eli_io *io = eli_get_io();
    if (!io)
        return;

    io->mouse_pos_prev = io->mouse_pos;
    io->mouse_wheel = 0.0f;
    io->mouse_wheel_h = 0.0f;
    io->input_queue_characters_count = 0;
}

#endif /* ELI_INPUT_ELI_INPUT_BACKEND_H */
