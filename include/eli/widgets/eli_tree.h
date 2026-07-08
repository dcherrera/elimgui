/**
 * @file eli_tree.h
 * @brief Phase 15 hierarchical widgets: tree nodes and collapsing headers. Provides
 *        the eli_tree_node* family (plain / ex / str / ptr / va_list forms), the
 *        eli_tree_push / eli_tree_pop indentation scope, eli_collapsing_header (and
 *        the p_visible close-button variant), plus eli_set_next_item_open /
 *        eli_set_next_item_storage_id and eli_get_tree_node_to_label_spacing.
 *
 * Open/closed state persists across frames in the id key/value storage
 * (eli_storage): the item's id (or a caller-supplied storage id) keys an int flag.
 * Clicking the arrow or label toggles it; an open node indents its children by
 * pushing onto the id + indent stacks (eli_tree_push) until eli_tree_pop. Mirrors
 * Dear ImGui's TreeNodeBehavior / TreePush / CollapsingHeader contracts.
 *
 * @status Phase 15 tree/collapsing widgets in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WIDGETS_ELI_TREE_H
#define ELI_WIDGETS_ELI_TREE_H

#include "eli_widget_behavior.h"
#include "eli_item_status.h"

#include "../id/eli_storage.h"
#include "../layout/eli_layout.h"

#include "../core/eli_platform.h"

#include <stdarg.h>
#include <stdio.h>

/* ---------------------------------------------------------------------------
 * Tree node flags
 *
 * Defined here (never in core/eli_enums.h) so this header is self-sufficient.
 * Guarded so a later central definition wins without a redefinition error.
 * Mirrors Dear ImGui's ImGuiTreeNodeFlags subset used by elimgui.
 * ------------------------------------------------------------------------- */

#ifndef ELI_TREE_NODE_NONE
typedef int eli_tree_node_flags;
enum eli_tree_node_flags_ {
    ELI_TREE_NODE_NONE                   = 0,
    ELI_TREE_NODE_SELECTED               = 1 << 0,
    ELI_TREE_NODE_FRAMED                 = 1 << 1,
    ELI_TREE_NODE_ALLOW_OVERLAP          = 1 << 2,
    ELI_TREE_NODE_NO_TREE_PUSH_ON_OPEN   = 1 << 3,
    ELI_TREE_NODE_NO_AUTO_OPEN_ON_LOG    = 1 << 4,
    ELI_TREE_NODE_DEFAULT_OPEN           = 1 << 5,
    ELI_TREE_NODE_OPEN_ON_DOUBLE_CLICK   = 1 << 6,
    ELI_TREE_NODE_OPEN_ON_ARROW          = 1 << 7,
    ELI_TREE_NODE_LEAF                   = 1 << 8,
    ELI_TREE_NODE_BULLET                 = 1 << 9,
    ELI_TREE_NODE_FRAME_PADDING          = 1 << 10,
    ELI_TREE_NODE_SPAN_AVAIL_WIDTH       = 1 << 11,
    ELI_TREE_NODE_SPAN_FULL_WIDTH        = 1 << 12,
    ELI_TREE_NODE_SPAN_TEXT_WIDTH        = 1 << 13,
    ELI_TREE_NODE_SPAN_ALL_COLUMNS       = 1 << 14,
    ELI_TREE_NODE_NAV_LEFT_JUMPS_BACK_HERE = 1 << 15,
    /* Combination: a collapsing header is a framed node that does not indent. */
    ELI_TREE_NODE_COLLAPSING_HEADER      = ELI_TREE_NODE_FRAMED |
                                           ELI_TREE_NODE_NO_TREE_PUSH_ON_OPEN |
                                           ELI_TREE_NODE_NO_AUTO_OPEN_ON_LOG,
    /* Mask of the flags that alter when a node toggles open. */
    ELI_TREE_NODE_OPEN_ON_MASK_          = ELI_TREE_NODE_OPEN_ON_ARROW |
                                           ELI_TREE_NODE_OPEN_ON_DOUBLE_CLICK
};
#endif /* ELI_TREE_NODE_NONE */

/** Longest formatted tree-node label kept on the stack, including the NUL. */
#define ELI_TREE_LABEL_BUF 256

/* ---------------------------------------------------------------------------
 * Persistent open-state storage + next-item scratch
 * ------------------------------------------------------------------------- */

/**
 * @return the storage used for tree open/closed state: the current context storage
 *         if the caller set one, else a process-lifetime fallback so trees persist
 *         their state without any per-window setup.
 */
static inline eli_storage *eli_tree__storage(void)
{
    eli_storage *st = eli_get_state_storage();
    if (st != NULL)
        return st;
    static eli_storage fallback = {0};
    return &fallback;
}

/** One-shot "next item" overrides consumed by the following tree node. */
typedef struct eli_tree_next_item_state {
    bool     has_open;        /* eli_set_next_item_open was called */
    bool     open_val;        /* forced open value */
    eli_cond open_cond;       /* condition governing the forced value */
    bool     has_storage_id;  /* eli_set_next_item_storage_id was called */
    eli_id   storage_id;      /* override key for the open-state storage */
} eli_tree_next_item_state;

/** @return the shared one-shot next-item override record (file-static scratch). */
static inline eli_tree_next_item_state *eli_tree__next_item(void)
{
    static eli_tree_next_item_state s = {0};
    return &s;
}

/** Clear the one-shot next-item overrides once a tree node has consumed them. */
static inline void eli_tree__clear_next_item(void)
{
    eli_tree_next_item_state *ni = eli_tree__next_item();
    ni->has_open = false;
    ni->has_storage_id = false;
}

/* ---------------------------------------------------------------------------
 * Open-state accessors
 * ------------------------------------------------------------------------- */

/**
 * @param storage_id  Key identifying the node's open state.
 * @return            true if the node keyed by storage_id is currently open.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_tree_node_get_open(eli_id storage_id)
{
    return eli_storage_get_int(eli_tree__storage(), storage_id, 0) != 0;
}

/**
 * Persist a node's open state under storage_id.
 *
 * @param storage_id  Key identifying the node's open state.
 * @param open        New open value.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_tree_node_set_open(eli_id storage_id, bool open)
{
    eli_storage_set_int(eli_tree__storage(), storage_id, open ? 1 : 0);
}

/**
 * Resolve a node's open state for this frame, applying any pending
 * eli_set_next_item_open override and the default-open flag. Leaves are always
 * "open". Mirrors Dear ImGui's TreeNodeUpdateNextOpen.
 *
 * @param storage_id  Key identifying the node's open state.
 * @param flags       Node flags (LEAF / DEFAULT_OPEN honored).
 * @return            The resolved open state.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_tree_node_update_next_open(eli_id storage_id, eli_tree_node_flags flags)
{
    if (flags & ELI_TREE_NODE_LEAF)
        return true;

    eli_storage *st = eli_tree__storage();
    eli_tree_next_item_state *ni = eli_tree__next_item();
    bool is_open;
    if (ni->has_open) {
        if (ni->open_cond & ELI_COND_ALWAYS) {
            is_open = ni->open_val;
            eli_tree_node_set_open(storage_id, is_open);
        } else {
            /* _Once / _FirstUseEver: only apply when no value is stored yet. */
            int stored = eli_storage_get_int(st, storage_id, -1);
            if (stored == -1) {
                is_open = ni->open_val;
                eli_tree_node_set_open(storage_id, is_open);
            } else {
                is_open = stored != 0;
            }
        }
    } else {
        int def = (flags & ELI_TREE_NODE_DEFAULT_OPEN) ? 1 : 0;
        is_open = eli_storage_get_int(st, storage_id, def) != 0;
    }
    return is_open;
}

/* ---------------------------------------------------------------------------
 * Next-item state setters
 * ------------------------------------------------------------------------- */

/**
 * Force the open state of the next tree node / collapsing header.
 *
 * @param is_open  Desired open value.
 * @param cond     Condition: ELI_COND_ALWAYS applies every frame; ELI_COND_ONCE /
 *                 _FIRST_USE_EVER apply only until the state is first stored. 0 is
 *                 treated as ELI_COND_ALWAYS.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_set_next_item_open(bool is_open, eli_cond cond)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || (ctx->current_window != NULL && ctx->current_window->skip_items))
        return;
    eli_tree_next_item_state *ni = eli_tree__next_item();
    ni->has_open = true;
    ni->open_val = is_open;
    ni->open_cond = (cond != 0) ? cond : ELI_COND_ALWAYS;
}

/**
 * Override the storage key used for the next tree node's open state, letting
 * several nodes share (or relocate) their persisted open flag.
 *
 * @param storage_id  Key to use instead of the node's own id.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_set_next_item_storage_id(eli_id storage_id)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || (ctx->current_window != NULL && ctx->current_window->skip_items))
        return;
    eli_tree_next_item_state *ni = eli_tree__next_item();
    ni->has_storage_id = true;
    ni->storage_id = storage_id;
}

/**
 * @return the horizontal distance from a tree node's left edge to its label,
 *         i.e. the arrow column width (font size + horizontal frame padding).
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline float eli_get_tree_node_to_label_spacing(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return 0.0f;
    return ctx->font_size + ctx->style.frame_padding.x * 2.0f;
}

/* ---------------------------------------------------------------------------
 * Tree push / pop
 * ------------------------------------------------------------------------- */

/**
 * Indent and push a precomputed id as the id-stack seed (used internally after a
 * tree node opens). Mirrors Dear ImGui's TreePushOverrideID.
 *
 * @param id  Id to push as the new seed.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_tree_push_override(eli_id id)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    eli_indent(0.0f);
    eli_id_stack_push_raw(ctx, id);
}

/**
 * Open a tree indentation scope keyed by a string, indenting subsequent items and
 * pushing str_id onto the id stack. Pair with eli_tree_pop.
 *
 * @param str_id  Identity string for the scope.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_tree_push(const char *str_id)
{
    eli_indent(0.0f);
    eli_push_id(str_id);
}

/**
 * Open a tree indentation scope keyed by a pointer. Pair with eli_tree_pop.
 *
 * @param ptr_id  Pointer identity for the scope.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_tree_push_ptr(const void *ptr_id)
{
    eli_indent(0.0f);
    eli_push_id_ptr(ptr_id);
}

/**
 * Close the innermost tree scope opened by a tree node or eli_tree_push*: unindent
 * and pop the id stack.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_tree_pop(void)
{
    eli_unindent(0.0f);
    eli_pop_id();
}

/* ---------------------------------------------------------------------------
 * Geometry
 * ------------------------------------------------------------------------- */

/** Resolved placement of a tree node's frame, interaction rect, and label. */
typedef struct eli_tree_node_layout {
    eli_rect frame_bb;       /* visual frame / hover background rect */
    eli_rect interact_bb;    /* clickable rect (narrower for plain nodes) */
    eli_vec2 text_pos;       /* pen position of the label */
    eli_vec2 padding;        /* effective frame padding */
    float    text_offset_x;  /* arrow-column width preceding the label */
    bool     display_frame;  /* framed (collapsing-header) look */
} eli_tree_node_layout;

/** Compute a tree node's geometry from the current cursor and style. */
static inline eli_tree_node_layout eli_tree__layout(eli_context *ctx, eli_window *win,
                                                    eli_tree_node_flags flags,
                                                    eli_vec2 label_size)
{
    const eli_style *style = &ctx->style;
    eli_tree_node_layout layout;
    layout.display_frame = (flags & ELI_TREE_NODE_FRAMED) != 0;
    bool use_frame_padding = layout.display_frame || (flags & ELI_TREE_NODE_FRAME_PADDING) != 0;
    layout.padding = use_frame_padding
        ? style->frame_padding
        : eli_make_vec2(style->frame_padding.x,
                        eli_min_f(win->curr_line_text_baseline_offset, style->frame_padding.y));

    float font_size = ctx->font_size;
    layout.text_offset_x =
        font_size + (layout.display_frame ? layout.padding.x * 3.0f : layout.padding.x * 2.0f);
    float text_offset_y = use_frame_padding
        ? eli_max_f(style->frame_padding.y, win->curr_line_text_baseline_offset)
        : win->curr_line_text_baseline_offset;
    float text_width = font_size + label_size.x + layout.padding.x * 2.0f;
    float frame_height = label_size.y + layout.padding.y * 2.0f;

    float work_min_x = win->content_region_rect.x;
    float work_max_x = win->content_region_rect.x + win->content_region_rect.w;
    eli_vec2 cursor = win->cursor_pos;

    float fx_min = (flags & ELI_TREE_NODE_SPAN_FULL_WIDTH) ? work_min_x : cursor.x;
    float fy_min = cursor.y + (text_offset_y - layout.padding.y);
    float fx_max = work_max_x;
    float fy_max = fy_min + frame_height;
    if (layout.display_frame) {
        float outer_extend = eli_layout_trunc(win->window_padding.x * 0.5f);
        fx_min -= outer_extend;
        fx_max += outer_extend;
    }
    layout.frame_bb = eli_make_rect(fx_min, fy_min, fx_max - fx_min, fy_max - fy_min);
    layout.text_pos = eli_make_vec2(cursor.x + layout.text_offset_x, cursor.y + text_offset_y);

    layout.interact_bb = layout.frame_bb;
    int span_mask = ELI_TREE_NODE_FRAMED | ELI_TREE_NODE_SPAN_AVAIL_WIDTH |
                    ELI_TREE_NODE_SPAN_FULL_WIDTH;
    if ((flags & span_mask) == 0) {
        float extra = (label_size.x > 0.0f) ? style->item_spacing.x * 2.0f : 0.0f;
        float new_max_x = fx_min + text_width + extra;
        layout.interact_bb = eli_make_rect(fx_min, fy_min, new_max_x - fx_min, fy_max - fy_min);
    }
    return layout;
}

/* ---------------------------------------------------------------------------
 * Rendering
 * ------------------------------------------------------------------------- */

/** Draw a tree node's background frame, arrow/bullet, and label. */
static inline void eli_tree__render(eli_context *ctx, const eli_tree_node_layout *layout,
                                    eli_tree_node_flags flags, bool is_open, bool is_leaf,
                                    bool hovered, bool held, const char *label,
                                    const char *label_end)
{
    eli_draw_list *dl = eli_get_window_draw_list();
    if (dl == NULL)
        return;
    const eli_style *style = &ctx->style;
    eli_col32 text_col = eli_get_color_u32(ELI_COL_TEXT, 1.0f);
    bool selected = (flags & ELI_TREE_NODE_SELECTED) != 0;
    eli_vec2 fmin = eli_rect_min(layout->frame_bb);
    eli_vec2 fmax = eli_rect_max(layout->frame_bb);
    eli_vec2 text_pos = layout->text_pos;
    float font_size = ctx->font_size;
    eli_dir dir = is_open ? ELI_DIR_DOWN : ELI_DIR_RIGHT;

    if (layout->display_frame) {
        eli_col32 bg = eli_get_color_u32(
            (held && hovered) ? ELI_COL_HEADER_ACTIVE : hovered ? ELI_COL_HEADER_HOVERED
                                                                : ELI_COL_HEADER, 1.0f);
        eli_render_frame(fmin, fmax, bg, true, style->frame_rounding);
        eli_render_nav_highlight(layout->frame_bb, ctx->last_item_id);
        if (flags & ELI_TREE_NODE_BULLET)
            eli_render_bullet(dl, eli_make_vec2(text_pos.x - layout->text_offset_x * 0.60f,
                                                text_pos.y), text_col);
        else if (!is_leaf)
            eli_render_arrow(dl, eli_make_vec2(text_pos.x - layout->text_offset_x + layout->padding.x,
                                               text_pos.y), text_col, dir, 1.0f);
        else
            text_pos.x -= layout->text_offset_x - layout->padding.x;
    } else {
        if (hovered || held || selected) {
            eli_col32 bg = eli_get_color_u32(
                (held && hovered) ? ELI_COL_HEADER_ACTIVE : hovered ? ELI_COL_HEADER_HOVERED
                                                                    : ELI_COL_HEADER, 1.0f);
            eli_render_frame(fmin, fmax, bg, false, 0.0f);
        }
        eli_render_nav_highlight(layout->frame_bb, ctx->last_item_id);
        if (flags & ELI_TREE_NODE_BULLET)
            eli_render_bullet(dl, eli_make_vec2(text_pos.x - layout->text_offset_x * 0.5f,
                                                text_pos.y), text_col);
        else if (!is_leaf)
            eli_render_arrow(dl, eli_make_vec2(text_pos.x - layout->text_offset_x + layout->padding.x,
                                               text_pos.y + font_size * 0.15f), text_col, dir, 0.70f);
    }
    eli_render_text(text_pos, text_col, label, label_end, false);
}

/* ---------------------------------------------------------------------------
 * Core behavior
 * ------------------------------------------------------------------------- */

/**
 * The shared tree-node engine: measure, register, resolve/toggle open state, render,
 * and (when open and not a collapsing header) push the child scope. Mirrors Dear
 * ImGui's TreeNodeBehavior.
 *
 * @param id         Node id (already derived from the label/str/ptr).
 * @param flags      eli_tree_node_flags controlling look and open behavior.
 * @param label      Visible label (its rendered part stops at "##").
 * @param label_end  End of the label, or NULL to scan to NUL.
 * @return           true when the node is open (children should be emitted).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_tree_node_behavior(eli_id id, eli_tree_node_flags flags, const char *label,
                                          const char *label_end)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return false;
    eli_window *win = ctx->current_window;

    if (label_end == NULL)
        label_end = eli_find_rendered_text_end(label, NULL);
    eli_vec2 label_size = eli_calc_text_size(label, label_end);
    eli_tree_node_layout layout = eli_tree__layout(ctx, win, flags, label_size);

    /* Resolve open state + storage id before ItemSize/ItemAdd (they clear scratch). */
    eli_tree_next_item_state *ni = eli_tree__next_item();
    eli_id storage_id = ni->has_storage_id ? ni->storage_id : id;
    bool is_open = eli_tree_node_update_next_open(storage_id, flags);
    bool is_leaf = (flags & ELI_TREE_NODE_LEAF) != 0;

    eli_item_size_rect(layout.interact_bb, layout.padding.y);
    bool is_visible = eli_item_add(id, layout.interact_bb, 0);
    eli_tree__clear_next_item();

    /* A leaf has no children, so it never pushes an indentation/id scope. */
    bool push_on_open = is_open && !is_leaf && !(flags & ELI_TREE_NODE_NO_TREE_PUSH_ON_OPEN);

    if (!is_visible) {
        if (push_on_open)
            eli_tree_push_override(id);
        return is_open;
    }

    /* The arrow occupies the leading font-size + padding column of the frame. */
    float arrow_x1 = layout.text_pos.x - layout.text_offset_x;
    float arrow_x2 = arrow_x1 + ctx->font_size + layout.padding.x * 2.0f;
    float mouse_x = ctx->io.mouse_pos.x;
    bool over_arrow = (mouse_x >= arrow_x1 && mouse_x < arrow_x2);

    eli_button_flags button_flags;
    if (over_arrow)
        button_flags = ELI_BUTTON_PRESSED_ON_CLICK;
    else
        button_flags = ELI_BUTTON_PRESSED_ON_CLICK_RELEASE;

    bool hovered = false, held = false;
    bool pressed = eli_button_behavior(layout.interact_bb, id, &hovered, &held, button_flags);

    if (!is_leaf && pressed) {
        bool toggled = false;
        if ((flags & ELI_TREE_NODE_OPEN_ON_MASK_) == 0)
            toggled = true;
        if (flags & ELI_TREE_NODE_OPEN_ON_ARROW)
            toggled = toggled || over_arrow;
        if ((flags & ELI_TREE_NODE_OPEN_ON_DOUBLE_CLICK) &&
            eli_get_mouse_clicked_count(ELI_MOUSE_BUTTON_LEFT) == 2)
            toggled = true;
        if (toggled) {
            is_open = !is_open;
            eli_tree_node_set_open(storage_id, is_open);
            ctx->last_item_status_flags |= ELI_ITEM_STATUS_TOGGLED_OPEN;
        }
    }

    eli_tree__render(ctx, &layout, flags, is_open, is_leaf, hovered, held, label, label_end);

    if (push_on_open)
        eli_tree_push_override(id);
    return is_open;
}

/* ---------------------------------------------------------------------------
 * Public tree-node API
 * ------------------------------------------------------------------------- */

/**
 * A plain tree node whose label is also its id.
 *
 * @param label  Node label (the visible part stops at "##").
 * @return       true when the node is open (emit children, then eli_tree_pop).
 */
static inline bool eli_tree_node(const char *label)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return false;
    return eli_tree_node_behavior(eli_get_id(label), ELI_TREE_NODE_NONE, label, NULL);
}

/**
 * A tree node with explicit flags whose label is also its id.
 *
 * @param label  Node label.
 * @param flags  eli_tree_node_flags.
 * @return       true when the node is open.
 */
static inline bool eli_tree_node_ex(const char *label, eli_tree_node_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return false;
    return eli_tree_node_behavior(eli_get_id(label), flags, label, NULL);
}

/**
 * A tree node with a stable string id and a printf-formatted label (va_list form).
 *
 * @param str_id  Identity string (label may repeat between nodes).
 * @param flags   eli_tree_node_flags.
 * @param fmt     printf-style format for the label.
 * @param args    Variadic arguments.
 * @return        true when the node is open.
 */
static inline bool eli_tree_node_ex_v(const char *str_id, eli_tree_node_flags flags,
                                      const char *fmt, va_list args)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return false;
    char buf[ELI_TREE_LABEL_BUF];
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    const char *end = buf + (n < 0 ? 0 : ((size_t)n < sizeof(buf) ? (size_t)n : sizeof(buf) - 1));
    return eli_tree_node_behavior(eli_get_id(str_id), flags, buf, end);
}

/**
 * A tree node with a pointer id and a printf-formatted label (va_list form).
 *
 * @param ptr_id  Identity pointer.
 * @param flags   eli_tree_node_flags.
 * @param fmt     printf-style format for the label.
 * @param args    Variadic arguments.
 * @return        true when the node is open.
 */
static inline bool eli_tree_node_ex_ptr_v(const void *ptr_id, eli_tree_node_flags flags,
                                          const char *fmt, va_list args)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return false;
    char buf[ELI_TREE_LABEL_BUF];
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    const char *end = buf + (n < 0 ? 0 : ((size_t)n < sizeof(buf) ? (size_t)n : sizeof(buf) - 1));
    return eli_tree_node_behavior(eli_get_id_ptr(ptr_id), flags, buf, end);
}

/** A tree node with a string id + formatted label (va_list form, default flags). */
static inline bool eli_tree_node_v(const char *str_id, const char *fmt, va_list args)
{
    return eli_tree_node_ex_v(str_id, ELI_TREE_NODE_NONE, fmt, args);
}

/** A tree node with a string id + printf-formatted label. */
static inline bool eli_tree_node_str(const char *str_id, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool is_open = eli_tree_node_ex_v(str_id, ELI_TREE_NODE_NONE, fmt, args);
    va_end(args);
    return is_open;
}

/** A tree node with a pointer id + printf-formatted label. */
static inline bool eli_tree_node_ptr(const void *ptr_id, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool is_open = eli_tree_node_ex_ptr_v(ptr_id, ELI_TREE_NODE_NONE, fmt, args);
    va_end(args);
    return is_open;
}

/** A tree node with a string id, explicit flags, and a printf-formatted label. */
static inline bool eli_tree_node_ex_str(const char *str_id, eli_tree_node_flags flags,
                                        const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool is_open = eli_tree_node_ex_v(str_id, flags, fmt, args);
    va_end(args);
    return is_open;
}

/** A tree node with a pointer id, explicit flags, and a printf-formatted label. */
static inline bool eli_tree_node_ex_ptr(const void *ptr_id, eli_tree_node_flags flags,
                                        const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool is_open = eli_tree_node_ex_ptr_v(ptr_id, flags, fmt, args);
    va_end(args);
    return is_open;
}

/* ---------------------------------------------------------------------------
 * Collapsing headers
 * ------------------------------------------------------------------------- */

/**
 * A framed, collapsing section header. Unlike a tree node it does not indent or
 * push an id scope, so no matching eli_tree_pop is needed.
 *
 * @param label  Header label.
 * @param flags  eli_tree_node_flags (FRAMED / NO_TREE_PUSH_ON_OPEN forced on).
 * @return       true when the section is open (emit its contents).
 */
static inline bool eli_collapsing_header(const char *label, eli_tree_node_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return false;
    return eli_tree_node_behavior(eli_get_id(label), flags | ELI_TREE_NODE_COLLAPSING_HEADER,
                                  label, NULL);
}

/** Draw a small close button (a hoverable "X") and return true when clicked. */
static inline bool eli_tree__close_button(eli_id id, eli_vec2 pos)
{
    eli_context *ctx = eli_get_current_context();
    eli_draw_list *dl = eli_get_window_draw_list();
    if (ctx == NULL || dl == NULL)
        return false;
    float sz = ctx->font_size;
    eli_rect bb = eli_make_rect(pos.x, pos.y, sz, sz);

    bool hovered = false, held = false;
    bool pressed = eli_button_behavior(bb, id, &hovered, &held, ELI_BUTTON_NONE);

    if (hovered) {
        eli_col32 bg = eli_get_color_u32(held ? ELI_COL_BUTTON_ACTIVE : ELI_COL_BUTTON_HOVERED, 1.0f);
        eli_vec2 center = eli_make_vec2(pos.x + sz * 0.5f, pos.y + sz * 0.5f);
        eli_draw_list_add_circle_filled(dl, center, sz * 0.5f, bg, 12);
    }
    eli_col32 cross = eli_get_color_u32(ELI_COL_TEXT, 1.0f);
    float pad = sz * 0.30f;
    eli_draw_list_add_line(dl, eli_make_vec2(pos.x + pad, pos.y + pad),
                           eli_make_vec2(pos.x + sz - pad, pos.y + sz - pad), cross, 1.0f);
    eli_draw_list_add_line(dl, eli_make_vec2(pos.x + sz - pad, pos.y + pad),
                           eli_make_vec2(pos.x + pad, pos.y + sz - pad), cross, 1.0f);
    return pressed;
}

/**
 * A collapsing header that also shows a close button when p_visible is provided.
 *
 * @param label      Header label.
 * @param p_visible  If non-NULL and *p_visible is true, a close button is drawn and
 *                   clicking it sets *p_visible = false. If *p_visible is false the
 *                   header is not shown at all and false is returned.
 * @param flags      eli_tree_node_flags.
 * @return           true when the section is open.
 */
static inline bool eli_collapsing_header_bool(const char *label, bool *p_visible,
                                              eli_tree_node_flags flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->current_window->skip_items)
        return false;
    if (p_visible != NULL && !*p_visible)
        return false;

    eli_id id = eli_get_id(label);
    flags |= ELI_TREE_NODE_COLLAPSING_HEADER;
    if (p_visible != NULL)
        flags |= ELI_TREE_NODE_ALLOW_OVERLAP;
    bool is_open = eli_tree_node_behavior(id, flags, label, NULL);

    if (p_visible != NULL) {
        /* Overlay a close button on the header's top-right corner. */
        eli_rect rect = ctx->last_item_rect;
        float button_size = ctx->font_size;
        float button_x = eli_max_f(rect.x, rect.x + rect.w - ctx->style.frame_padding.x - button_size);
        float button_y = rect.y + ctx->style.frame_padding.y;
        eli_id close_id = eli_hash_str("#CLOSE", id);
        if (eli_tree__close_button(close_id, eli_make_vec2(button_x, button_y)))
            *p_visible = false;
    }
    return is_open;
}

#endif /* ELI_WIDGETS_ELI_TREE_H */
