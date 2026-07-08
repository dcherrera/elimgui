/**
 * @file eli_io.h
 * @brief The eli_io structure: the input/output bridge between the host backend
 *        and elimgui (display metrics, timing, mouse/keyboard state, text queue,
 *        capture-intent outputs, and clipboard callbacks).
 *
 * @status Phase 1 structure definition in use. Input-state fields and the
 *         buffered event queue are populated by the input phase.
 * @issues None
 * @todo None
 */
#ifndef ELI_CORE_ELI_IO_H
#define ELI_CORE_ELI_IO_H

#include "eli_types.h"
#include "eli_enums.h"

/* Opaque font types are owned by the font phase; IO only stores pointers. */
typedef struct eli_font eli_font;
typedef struct eli_font_atlas eli_font_atlas;

/* Capacity of the per-frame buffered input-event queue. Backends append events
 * with the eli_io_add_*_event helpers; they are drained at begin-frame. */
#ifndef ELI_INPUT_EVENT_QUEUE_SIZE
#define ELI_INPUT_EVENT_QUEUE_SIZE 256
#endif

/* Capacity of the per-frame text-input character queue (UTF-16 code units). */
#ifndef ELI_INPUT_QUEUE_CHAR_SIZE
#define ELI_INPUT_QUEUE_CHAR_SIZE 64
#endif

/* Encoding for the next-frame capture-intent overrides. Chosen so a zeroed IO
 * (calloc) naturally means "no override", independent of input initialization. */
#define ELI_CAPTURE_OVERRIDE_NONE  0
#define ELI_CAPTURE_OVERRIDE_FALSE 1
#define ELI_CAPTURE_OVERRIDE_TRUE  2

/** Discriminator for a buffered input event. */
typedef enum eli_input_event_type {
    ELI_INPUT_EVENT_TYPE_NONE = 0,
    ELI_INPUT_EVENT_TYPE_MOUSE_POS,
    ELI_INPUT_EVENT_TYPE_MOUSE_BUTTON,
    ELI_INPUT_EVENT_TYPE_MOUSE_WHEEL,
    ELI_INPUT_EVENT_TYPE_KEY,
    ELI_INPUT_EVENT_TYPE_TEXT
} eli_input_event_type;

/**
 * A single buffered input event. Backends push these via the eli_io_add_*_event
 * helpers; eli_input_update_begin_frame applies them to the live IO state in the
 * order they were received (matching Dear ImGui's queued-input model).
 */
typedef struct eli_input_event {
    eli_input_event_type type;
    union {
        struct { float x, y; } mouse_pos;
        struct { int button; bool down; } mouse_button;
        struct { float x, y; } mouse_wheel;
        struct { int key; bool down; float analog_value; } key;
        struct { uint16_t c; } text;
    } data;
} eli_input_event;

/**
 * Input/output state shared with the host backend.
 *
 * The backend writes the configuration and input fields at the top (display
 * size, delta time, mouse/keyboard events); elimgui writes the output fields
 * (want_capture_*, framerate, metrics) during the frame. Per-frame derived
 * state (mouse deltas, click counts, key durations) is maintained by the input
 * phase and consumed by widgets.
 */
typedef struct eli_io {
    /* --- Configuration (set by the application/backend) --- */
    eli_vec2 display_size;
    eli_vec2 display_framebuffer_scale;
    float delta_time;
    float ini_saving_rate;
    const char *ini_filename;
    const char *log_filename;
    void *user_data;
    eli_font_atlas *fonts;
    float font_global_scale;
    bool font_allow_user_scaling;
    eli_font *font_default;

    /* --- Capability / behavior flags --- */
    eli_config_flags config_flags;
    eli_backend_flags backend_flags;

    /* --- Mouse input (current frame, set by backend) --- */
    eli_vec2 mouse_pos;
    bool mouse_down[ELI_MOUSE_BUTTON_COUNT];
    float mouse_wheel;
    float mouse_wheel_h;
    eli_mouse_cursor mouse_draw_cursor;

    /* --- Keyboard input (current frame, set by backend) --- */
    bool key_ctrl;
    bool key_shift;
    bool key_alt;
    bool key_super;
    bool keys_down[ELI_KEY_COUNT];

    /* --- Text input queue (UTF-16 code units) --- */
    uint16_t input_queue_characters[ELI_INPUT_QUEUE_CHAR_SIZE];
    int input_queue_characters_count;

    /* --- Output (written by elimgui each frame) --- */
    bool want_capture_mouse;
    bool want_capture_keyboard;
    bool want_text_input;
    bool want_set_mouse_pos;
    bool want_save_ini_settings;
    bool nav_active;
    bool nav_visible;
    float framerate;
    int metrics_render_vertices;
    int metrics_render_indices;
    int metrics_render_windows;
    int metrics_active_windows;

    /* --- Derived mouse state (maintained by the input phase) --- */
    eli_vec2 mouse_pos_prev;
    eli_vec2 mouse_delta;
    bool mouse_clicked[ELI_MOUSE_BUTTON_COUNT];
    bool mouse_double_clicked[ELI_MOUSE_BUTTON_COUNT];
    uint16_t mouse_clicked_count[ELI_MOUSE_BUTTON_COUNT];
    bool mouse_released[ELI_MOUSE_BUTTON_COUNT];
    bool mouse_down_owned[ELI_MOUSE_BUTTON_COUNT];
    float mouse_down_duration[ELI_MOUSE_BUTTON_COUNT];
    float mouse_down_duration_prev[ELI_MOUSE_BUTTON_COUNT];
    float mouse_drag_max_distance_sqr[ELI_MOUSE_BUTTON_COUNT];

    /* --- Additional derived mouse-click tracking (input phase) --- */
    double mouse_clicked_time[ELI_MOUSE_BUTTON_COUNT];
    eli_vec2 mouse_clicked_pos[ELI_MOUSE_BUTTON_COUNT];
    uint16_t mouse_clicked_last_count[ELI_MOUSE_BUTTON_COUNT];
    double mouse_released_time[ELI_MOUSE_BUTTON_COUNT];

    /* --- Derived keyboard state (maintained by the input phase) --- */
    float key_down_duration[ELI_KEY_COUNT];
    float key_down_duration_prev[ELI_KEY_COUNT];
    float keys_analog[ELI_KEY_COUNT];

    /* --- Requested mouse-cursor shape for the current frame --- */
    eli_mouse_cursor mouse_cursor;

    /* --- Next-frame capture overrides (ELI_CAPTURE_OVERRIDE_*) --- */
    int want_capture_mouse_next_frame;
    int want_capture_keyboard_next_frame;

    /* --- Input timing / tunables (defaults applied by the input phase) --- */
    bool input_config_initialized;
    double input_time;
    float mouse_double_click_time;
    float mouse_double_click_max_dist;
    float mouse_drag_threshold;
    float key_repeat_delay;
    float key_repeat_rate;

    /* --- Buffered input-event queue (drained at begin-frame) --- */
    eli_input_event input_events[ELI_INPUT_EVENT_QUEUE_SIZE];
    int input_events_count;

    /* --- Clipboard platform callbacks --- */
    const char *(*get_clipboard_text_fn)(void *user_data);
    void (*set_clipboard_text_fn)(void *user_data, const char *text);
    void *clipboard_user_data;
} eli_io;

#endif /* ELI_CORE_ELI_IO_H */
