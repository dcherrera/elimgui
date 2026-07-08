/**
 * @file eli_input_mouse.h
 * @brief Mouse-state queries for elimgui: button up/down/clicked/released/
 *        double-clicked tracking, click counts, hover testing, drag detection
 *        and drag delta, cursor shape get/set, and capture-intent overrides.
 *
 * Derived state (clicked/released/durations/drag distance) is produced each
 * frame by eli_input_backend.h; this header only reads it. Coordinates use the
 * top-left origin convention shared by the rest of elimgui.
 *
 * @status Phase 4 mouse queries in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_INPUT_ELI_INPUT_MOUSE_H
#define ELI_INPUT_ELI_INPUT_MOUSE_H

#include "../core/eli_core.h"

/* A mouse position with either component below this sentinel is treated as
 * invalid/offscreen (matches Dear ImGui's -256000 convention). The context
 * initializes mouse_pos to -FLT_MAX, which is below this. */
#define ELI_MOUSE_POS_INVALID (-256000.0f)

/* ---------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

/** True if a raw position value is a valid (on-screen) mouse position. */
static inline bool eli_mouse_pos_is_valid(eli_vec2 p)
{
    return p.x >= ELI_MOUSE_POS_INVALID && p.y >= ELI_MOUSE_POS_INVALID;
}

/** True if a button index addresses a real mouse button slot. */
static inline bool eli_mouse_button_is_valid(int button)
{
    return button >= 0 && button < ELI_MOUSE_BUTTON_COUNT;
}

/* ---------------------------------------------------------------------------
 * Button state
 * ------------------------------------------------------------------------- */

/**
 * @param button  Mouse button index (ELI_MOUSE_BUTTON_LEFT/RIGHT/MIDDLE, ...).
 * @return        true while the button is held down.
 *
 * Thread-safe: no (reads current-context IO)
 * Reentrant: yes
 */
static inline bool eli_is_mouse_down(eli_mouse_button button)
{
    const eli_io *io = eli_get_io();
    return io && eli_mouse_button_is_valid(button) && io->mouse_down[button];
}

/**
 * @param button  Mouse button index.
 * @return        true only on the frame the button transitions to down.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline bool eli_is_mouse_clicked(eli_mouse_button button)
{
    const eli_io *io = eli_get_io();
    return io && eli_mouse_button_is_valid(button) && io->mouse_clicked[button];
}

/**
 * @param button  Mouse button index.
 * @return        true only on the frame the button transitions to up.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline bool eli_is_mouse_released(eli_mouse_button button)
{
    const eli_io *io = eli_get_io();
    return io && eli_mouse_button_is_valid(button) && io->mouse_released[button];
}

/**
 * @param button  Mouse button index.
 * @return        true on the frame a click is recognized as the second of a
 *                double-click (also reports eli_is_mouse_clicked that frame).
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline bool eli_is_mouse_double_clicked(eli_mouse_button button)
{
    const eli_io *io = eli_get_io();
    return io && eli_mouse_button_is_valid(button) && io->mouse_double_clicked[button];
}

/**
 * @param button  Mouse button index.
 * @return        Number of successive clicks recognized on the frame a click
 *                happens (1 = single, 2 = double, ...); 0 otherwise.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline int eli_get_mouse_clicked_count(eli_mouse_button button)
{
    const eli_io *io = eli_get_io();
    if (!io || !eli_mouse_button_is_valid(button))
        return 0;
    return (int)io->mouse_clicked_count[button];
}

/**
 * @return  true if any mouse button is currently held down.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline bool eli_is_any_mouse_down(void)
{
    const eli_io *io = eli_get_io();
    if (!io)
        return false;
    for (int i = 0; i < ELI_MOUSE_BUTTON_COUNT; i++)
        if (io->mouse_down[i])
            return true;
    return false;
}

/* ---------------------------------------------------------------------------
 * Position and hover testing
 * ------------------------------------------------------------------------- */

/**
 * @return  The current mouse position (top-left origin). May be an invalid
 *          (offscreen) sentinel; test with eli_is_mouse_pos_valid.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline eli_vec2 eli_get_mouse_pos(void)
{
    const eli_io *io = eli_get_io();
    return io ? io->mouse_pos : eli_make_vec2(ELI_MOUSE_POS_INVALID, ELI_MOUSE_POS_INVALID);
}

/**
 * @return  The mouse position; popups are not implemented yet, so this returns
 *          the live mouse position (added for API parity with Dear ImGui).
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline eli_vec2 eli_get_mouse_pos_on_opening_current_popup(void)
{
    return eli_get_mouse_pos();
}

/**
 * @param mouse_pos  Position to test, or NULL to test the current mouse position.
 * @return           true if the position is a valid (on-screen) coordinate.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline bool eli_is_mouse_pos_valid(const eli_vec2 *mouse_pos)
{
    eli_vec2 p = mouse_pos ? *mouse_pos : eli_get_mouse_pos();
    return eli_mouse_pos_is_valid(p);
}

/**
 * Hit-test the mouse against a rectangle, expanded by the style's touch-extra
 * padding for imprecise input devices. Minimum edges are inclusive, maximum
 * edges exclusive.
 *
 * @param r_min  Rectangle top-left corner.
 * @param r_max  Rectangle bottom-right corner.
 * @param clip   Reserved for window-clip integration (later phase); currently
 *               the test is unclipped.
 * @return       true if the mouse is inside the padded rectangle.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline bool eli_is_mouse_hovering_rect(eli_vec2 r_min, eli_vec2 r_max, bool clip)
{
    (void)clip;
    const eli_io *io = eli_get_io();
    const eli_style *style = eli_get_style();
    if (!io)
        return false;

    eli_vec2 pad = style ? style->touch_extra_padding : eli_make_vec2(0.0f, 0.0f);
    eli_vec2 p = io->mouse_pos;
    return p.x >= r_min.x - pad.x && p.y >= r_min.y - pad.y &&
           p.x <  r_max.x + pad.x && p.y <  r_max.y + pad.y;
}

/* ---------------------------------------------------------------------------
 * Dragging
 * ------------------------------------------------------------------------- */

/**
 * @param button          Mouse button index.
 * @param lock_threshold  Distance in pixels the mouse must travel from the click
 *                        origin before a drag is recognized; pass a negative
 *                        value to use the style/IO default.
 * @return                true while the button is held and has dragged past the
 *                        threshold at least once.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline bool eli_is_mouse_dragging(eli_mouse_button button, float lock_threshold)
{
    const eli_io *io = eli_get_io();
    if (!io || !eli_mouse_button_is_valid(button) || !io->mouse_down[button])
        return false;
    if (lock_threshold < 0.0f)
        lock_threshold = io->mouse_drag_threshold;
    return io->mouse_drag_max_distance_sqr[button] >= lock_threshold * lock_threshold;
}

/**
 * Delta from the click origin while a button is held (or on its release frame),
 * locked to (0,0) until the drag passes the threshold at least once.
 *
 * @param button          Mouse button index.
 * @param lock_threshold  Drag threshold in pixels; negative uses the default.
 * @return                Movement since the click origin, or (0,0) if not yet
 *                        dragging or positions are invalid.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline eli_vec2 eli_get_mouse_drag_delta(eli_mouse_button button, float lock_threshold)
{
    const eli_io *io = eli_get_io();
    if (!io || !eli_mouse_button_is_valid(button))
        return eli_make_vec2(0.0f, 0.0f);
    if (lock_threshold < 0.0f)
        lock_threshold = io->mouse_drag_threshold;

    if ((io->mouse_down[button] || io->mouse_released[button]) &&
        io->mouse_drag_max_distance_sqr[button] >= lock_threshold * lock_threshold &&
        eli_mouse_pos_is_valid(io->mouse_pos) && eli_mouse_pos_is_valid(io->mouse_clicked_pos[button]))
        return eli_vec2_sub(io->mouse_pos, io->mouse_clicked_pos[button]);
    return eli_make_vec2(0.0f, 0.0f);
}

/**
 * Reset the drag origin for a button to the current mouse position, so a fresh
 * drag delta accumulates from here.
 *
 * @param button  Mouse button index.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_reset_mouse_drag_delta(eli_mouse_button button)
{
    eli_io *io = eli_get_io();
    if (!io || !eli_mouse_button_is_valid(button))
        return;
    io->mouse_clicked_pos[button] = io->mouse_pos;
    io->mouse_drag_max_distance_sqr[button] = 0.0f;
}

/* ---------------------------------------------------------------------------
 * Cursor and capture intent
 * ------------------------------------------------------------------------- */

/**
 * @return  The mouse-cursor shape requested for this frame (defaults to
 *          ELI_MOUSE_CURSOR_ARROW; reset each frame at begin-frame).
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline eli_mouse_cursor eli_get_mouse_cursor(void)
{
    const eli_io *io = eli_get_io();
    return io ? io->mouse_cursor : ELI_MOUSE_CURSOR_ARROW;
}

/**
 * Request a mouse-cursor shape for the current frame; the backend reads it after
 * the frame to update the OS/browser cursor.
 *
 * @param cursor  Desired cursor shape.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_set_mouse_cursor(eli_mouse_cursor cursor)
{
    eli_io *io = eli_get_io();
    if (io)
        io->mouse_cursor = cursor;
}

/**
 * Override whether elimgui wants to capture the mouse on the next frame. Applied
 * at the next begin-frame. Useful for backends to force-release capture.
 *
 * @param want_capture  Desired capture intent for the next frame.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_set_next_frame_want_capture_mouse(bool want_capture)
{
    eli_io *io = eli_get_io();
    if (io)
        io->want_capture_mouse_next_frame =
            want_capture ? ELI_CAPTURE_OVERRIDE_TRUE : ELI_CAPTURE_OVERRIDE_FALSE;
}

#endif /* ELI_INPUT_ELI_INPUT_MOUSE_H */
