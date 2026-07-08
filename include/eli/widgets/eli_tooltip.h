/**
 * @file eli_tooltip.h
 * @brief Phase 18 tooltips: the begin/end tooltip window scope
 *        (eli_begin_tooltip / eli_end_tooltip), the formatted one-shot tooltips
 *        (eli_set_tooltip / _v), the item-gated variants that appear only after the
 *        previous item has been hovered past the hover delay (eli_begin_item_tooltip,
 *        eli_set_item_tooltip / _v), and the hover-delay accumulator that drives them.
 *
 * A tooltip IS a window: an auto-sized, input-less, chrome-less window positioned
 * just off the cursor and reusing eli_begin / eli_end for layout and draw output.
 * The hover-delay state (timer, stationary unlock, per-frame id) lives file-static
 * here because the context has no slot for it; it mirrors Dear ImGui's
 * HoverItemDelay* bookkeeping, accumulating style.hover_delay_* / hover_stationary_delay
 * from io.delta_time. The tooltip color uses ELI_COL_POPUP_BG.
 *
 * @status Phase 18 tooltips in use.
 * @issues Tooltip windows render in window focus order rather than a dedicated top
 *         layer, so a window focused after the tooltip appears can overlap it. The
 *         hover-delay uses the mouse (style.hover_flags_for_tooltip_mouse) path; the
 *         keyboard/gamepad nav path is not yet wired.
 * @todo None
 */
#ifndef ELI_WIDGETS_ELI_TOOLTIP_H
#define ELI_WIDGETS_ELI_TOOLTIP_H

#include "eli_item_status.h"
#include "eli_text_widgets.h"

#include "../core/eli_platform.h"
#include "../window/eli_window.h"
#include "../input/eli_input.h"
#include "../style/eli_style_stack.h"

#include <stdarg.h>
#include <stdio.h>

/* ---------------------------------------------------------------------------
 * Tunables (guarded so a later phase may promote them without a clash)
 * ------------------------------------------------------------------------- */

/* Cursor-relative offset of a tooltip window's top-left, multiplied by
 * style.mouse_cursor_scale (mirrors Dear ImGui's TOOLTIP_DEFAULT_OFFSET_MOUSE). */
#ifndef ELI_TOOLTIP_OFFSET_X
#define ELI_TOOLTIP_OFFSET_X 16.0f
#endif
#ifndef ELI_TOOLTIP_OFFSET_Y
#define ELI_TOOLTIP_OFFSET_Y 10.0f
#endif

/* Standard flag composition for auto-sized, input-less, chrome-less tooltip
 * windows. NoInputs keeps the tooltip out of hover/nav so it never steals the
 * hover from the item it describes. */
#define ELI_TOOLTIP_WINDOW_FLAGS                                                       \
    (ELI_WINDOW_NO_TITLEBAR | ELI_WINDOW_NO_RESIZE | ELI_WINDOW_NO_MOVE |              \
     ELI_WINDOW_NO_SCROLLBAR | ELI_WINDOW_NO_COLLAPSE | ELI_WINDOW_NO_SAVED_SETTINGS | \
     ELI_WINDOW_AUTO_RESIZE | ELI_WINDOW_NO_INPUTS | ELI_WINDOW_NO_NAV |               \
     ELI_WINDOW_NO_FOCUS_ON_APPEARING)

/* ---------------------------------------------------------------------------
 * File-static state
 * ------------------------------------------------------------------------- */

/* Per-frame tooltip window naming: each tooltip begun within a frame gets a
 * distinct index, so a set_tooltip that follows a begin_tooltip uses a fresh
 * window (no stale content), while equal indices reuse the same persistent
 * windows across frames (their draw list is rebuilt each Begin). */
static int g_eli_tooltip_frame = -1;
static int g_eli_tooltip_count = 0;

/* Hover-delay accumulator (mirrors Dear ImGui's HoverItemDelay* fields). Lives
 * file-static because eli_context has no slot for it. */
static eli_id g_eli_tooltip_hover_id = 0;             /* item requesting delay this frame */
static eli_id g_eli_tooltip_hover_id_prev = 0;        /* id requested last frame */
static float  g_eli_tooltip_hover_timer = 0.0f;       /* time the current item has been hovered */
static float  g_eli_tooltip_hover_clear_timer = 0.0f; /* grace time before the timer resets */
static eli_id g_eli_tooltip_stationary_id = 0;        /* item unlocked by a stationary mouse */
static float  g_eli_tooltip_stationary_timer = 0.0f;  /* time the mouse has been stationary */
static int    g_eli_tooltip_tick_frame = -1;          /* last frame the accumulator advanced */

/* ---------------------------------------------------------------------------
 * Hover-delay logic
 * ------------------------------------------------------------------------- */

/**
 * Resolve the hover-delay id for the last-submitted item: its own id, or a hash
 * of its rect origin when the item has no id (e.g. a plain text item).
 *
 * @param ctx  Context (non-NULL, with a current window).
 * @return     Non-zero delay id.
 */
static inline eli_id eli_tooltip__hover_id(const eli_context *ctx)
{
    if (ctx->last_item_id != 0u)
        return ctx->last_item_id;
    eli_vec2 origin = eli_rect_min(ctx->last_item_rect);
    eli_id seed = ctx->current_window != NULL ? ctx->current_window->id : 0u;
    return eli_hash_data(&origin, sizeof origin, seed);
}

/**
 * Advance the hover-delay accumulator once per frame. Mirrors Dear ImGui's
 * end-of-frame HoverItemDelay update, executed lazily at the first hover-for-
 * tooltip query of the frame (equivalent for cross-frame accumulation): the id a
 * query requested last frame drives this frame's timer growth, then the request
 * slot is cleared for this frame's queries to re-assert.
 *
 * @param ctx  Context (non-NULL).
 */
static inline void eli_tooltip__tick_hover_delay(eli_context *ctx)
{
    if (g_eli_tooltip_tick_frame == ctx->frame_count)
        return;
    g_eli_tooltip_tick_frame = ctx->frame_count;

    float dt = ctx->io.delta_time;
    const eli_style *style = &ctx->style;

    /* Mouse-stationary timer: reset on any motion, accrue while the mouse is still. */
    if (ctx->io.mouse_delta.x != 0.0f || ctx->io.mouse_delta.y != 0.0f)
        g_eli_tooltip_stationary_timer = 0.0f;
    else
        g_eli_tooltip_stationary_timer += dt;

    g_eli_tooltip_hover_id_prev = g_eli_tooltip_hover_id;

    if (g_eli_tooltip_hover_id != 0u &&
        g_eli_tooltip_stationary_timer >= style->hover_stationary_delay)
        g_eli_tooltip_stationary_id = g_eli_tooltip_hover_id;
    else if (g_eli_tooltip_hover_id == 0u)
        g_eli_tooltip_stationary_id = 0u;

    if (g_eli_tooltip_hover_id != 0u) {
        g_eli_tooltip_hover_timer += dt;
        g_eli_tooltip_hover_clear_timer = 0.0f;
    } else if (g_eli_tooltip_hover_timer > 0.0f) {
        g_eli_tooltip_hover_clear_timer += dt;
        if (g_eli_tooltip_hover_clear_timer >= eli_max_f(0.25f, dt * 2.0f)) {
            g_eli_tooltip_hover_timer = 0.0f;
            g_eli_tooltip_hover_clear_timer = 0.0f;
        }
    }

    g_eli_tooltip_hover_id = 0u;
}

/**
 * Fold the shared tooltip hover flags into the caller's flags, letting an explicit
 * per-call delay flag override the shared delay bits (Dear ImGui parity).
 */
static inline eli_hovered_flags eli_tooltip__apply_flags(eli_hovered_flags user,
                                                         eli_hovered_flags shared)
{
    eli_hovered_flags delay_mask =
        ELI_HOVERED_DELAY_NONE | ELI_HOVERED_DELAY_SHORT | ELI_HOVERED_DELAY_NORMAL;
    if (user & delay_mask)
        shared &= ~delay_mask;
    return user | shared;
}

/**
 * Whether the most-recent item is hovered for tooltip purposes: the standard
 * rect/window/active/disabled gating plus the hover-delay and mouse-stationary
 * requirements resolved from the flags. Pass ELI_HOVERED_FOR_TOOLTIP to apply the
 * style's shared mouse-tooltip flags (stationary + short delay + allow-disabled).
 *
 * @param flags  eli_hovered_flags; ELI_HOVERED_FOR_TOOLTIP pulls in the shared set.
 * @return       true once the item has been hovered long enough to show a tooltip.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_is_item_hovered_for_tooltip(eli_hovered_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL)
        return false;

    if (flags & ELI_HOVERED_FOR_TOOLTIP)
        flags = eli_tooltip__apply_flags(flags, ctx->style.hover_flags_for_tooltip_mouse);

    eli_tooltip__tick_hover_delay(ctx);

    /* Base gating: rect/window/active/disabled. eli_is_item_hovered does not
     * understand the delay/stationary bits, so only forward the ones it honors. */
    eli_hovered_flags base = flags & (ELI_HOVERED_ALLOW_WHEN_DISABLED |
                                      ELI_HOVERED_ALLOW_WHEN_BLOCKED_BY_ACTIVE_ITEM |
                                      ELI_HOVERED_ALLOW_WHEN_OVERLAPPED_BY_WINDOW);
    if (!eli_is_item_hovered(base))
        return false;

    float delay = 0.0f;
    if (flags & ELI_HOVERED_DELAY_NORMAL)
        delay = ctx->style.hover_delay_normal;
    else if (flags & ELI_HOVERED_DELAY_SHORT)
        delay = ctx->style.hover_delay_short;

    bool stationary = (flags & ELI_HOVERED_STATIONARY) != 0;
    if (delay <= 0.0f && !stationary)
        return true;

    eli_id hover_id = eli_tooltip__hover_id(ctx);
    if ((flags & ELI_HOVERED_NO_SHARED_DELAY) && g_eli_tooltip_hover_id_prev != hover_id)
        g_eli_tooltip_hover_timer = 0.0f;
    g_eli_tooltip_hover_id = hover_id;

    if (stationary && g_eli_tooltip_stationary_id != hover_id)
        return false;
    if (g_eli_tooltip_hover_timer < delay)
        return false;
    return true;
}

/* ---------------------------------------------------------------------------
 * Tooltip window
 * ------------------------------------------------------------------------- */

/**
 * Position and begin the tooltip window for this call. Places the window just off
 * the cursor and, once its size is known from the previous display frame, clamps
 * it inside the display's safe area (flipping to the cursor's left when it would
 * overflow the right edge). Pushes ELI_COL_POPUP_BG as the window background.
 *
 * @return  true (the tooltip body should always be emitted).
 */
static inline bool eli_tooltip__begin(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return false;

    if (ctx->frame_count != g_eli_tooltip_frame) {
        g_eli_tooltip_frame = ctx->frame_count;
        g_eli_tooltip_count = 0;
    }
    int index = g_eli_tooltip_count++;
    if (index > 99)
        index = 99;

    char name[32];
    snprintf(name, sizeof name, "##Tooltip_%02d", index);

    const eli_style *style = &ctx->style;
    float scale = style->mouse_cursor_scale;
    eli_vec2 mouse = eli_get_mouse_pos();
    if (!eli_mouse_pos_is_valid(mouse))
        mouse = eli_make_vec2(0.0f, 0.0f);
    eli_vec2 pos = eli_make_vec2(mouse.x + ELI_TOOLTIP_OFFSET_X * scale,
                                 mouse.y + ELI_TOOLTIP_OFFSET_Y * scale);

    /* Keep on-screen using the previous frame's auto-resized size (known after the
     * first display frame). */
    eli_window *existing = eli_find_window_by_name(ctx, name);
    eli_vec2 disp = ctx->io.display_size;
    if (existing != NULL && disp.x > 0.0f && disp.y > 0.0f &&
        existing->size.x > 0.0f && existing->size.y > 0.0f) {
        eli_vec2 pad = style->display_safe_area_padding;
        float max_x = disp.x - existing->size.x - pad.x;
        float max_y = disp.y - existing->size.y - pad.y;
        if (pos.x > max_x)
            pos.x = mouse.x - ELI_TOOLTIP_OFFSET_X * scale - existing->size.x;
        if (pos.y > max_y)
            pos.y = max_y;
        pos.x = eli_max_f(pos.x, pad.x);
        pos.y = eli_max_f(pos.y, pad.y);
    }
    eli_set_next_window_pos(pos, ELI_COND_ALWAYS, eli_make_vec2(0.0f, 0.0f));

    eli_push_style_color(ELI_COL_WINDOW_BG, eli_get_color_u32(ELI_COL_POPUP_BG, 1.0f));
    eli_begin(name, NULL, ELI_TOOLTIP_WINDOW_FLAGS);
    return true;
}

/**
 * Begin a tooltip window at the mouse cursor. Emit contents (text, widgets) and
 * balance with eli_end_tooltip. Currently always returns true.
 *
 * @return  true (emit the tooltip body and call eli_end_tooltip).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_begin_tooltip(void)
{
    return eli_tooltip__begin();
}

/**
 * End the tooltip window opened by eli_begin_tooltip / eli_begin_item_tooltip.
 * Pops the tooltip window scope and its background color.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_end_tooltip(void)
{
    if (eli_get_current_context() == NULL)
        return;
    eli_end();
    eli_pop_style_color(1);
}

/**
 * Begin a tooltip only if the most-recent item has been hovered long enough
 * (style.hover_flags_for_tooltip_mouse). Balance with eli_end_tooltip only when
 * this returns true.
 *
 * @return  true if the tooltip is shown (emit its body and call eli_end_tooltip).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_begin_item_tooltip(void)
{
    if (!eli_is_item_hovered_for_tooltip(ELI_HOVERED_FOR_TOOLTIP))
        return false;
    return eli_tooltip__begin();
}

/* ---------------------------------------------------------------------------
 * One-shot formatted tooltips
 * ------------------------------------------------------------------------- */

/**
 * Show a formatted one-shot tooltip at the cursor (va_list form): begins the
 * tooltip window, emits the formatted text, and ends it.
 *
 * @param fmt   printf-style format string.
 * @param args  Variadic argument list.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_set_tooltip_v(const char *fmt, va_list args)
{
    if (!eli_tooltip__begin())
        return;
    eli_text_v(fmt, args);
    eli_end_tooltip();
}

/**
 * Show a formatted one-shot tooltip at the cursor.
 *
 * @param fmt  printf-style format string.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_set_tooltip(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    eli_set_tooltip_v(fmt, args);
    va_end(args);
}

/**
 * Show a formatted tooltip for the most-recent item, only once it has been hovered
 * past the hover delay (va_list form).
 *
 * @param fmt   printf-style format string.
 * @param args  Variadic argument list.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_set_item_tooltip_v(const char *fmt, va_list args)
{
    if (eli_is_item_hovered_for_tooltip(ELI_HOVERED_FOR_TOOLTIP))
        eli_set_tooltip_v(fmt, args);
}

/**
 * Show a formatted tooltip for the most-recent item, only once it has been hovered
 * past the hover delay.
 *
 * @param fmt  printf-style format string.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_set_item_tooltip(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    eli_set_item_tooltip_v(fmt, args);
    va_end(args);
}

#endif /* ELI_WIDGETS_ELI_TOOLTIP_H */
