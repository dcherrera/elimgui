/**
 * @file eli_window.h
 * @brief Window lifecycle and the window-system aggregator: eli_begin/eli_end,
 *        window creation/retrieval, decoration rendering (background, border,
 *        title bar), the per-frame window update (eli_window_new_frame), and the
 *        draw-data assembly (eli_window_render). Including this header pulls in
 *        the whole window category (settings, scroll, interaction, query, child).
 *
 * Frame flow the application drives each frame:
 *     eli_new_frame();                  (core: advance frame counter/time)
 *     eli_input_update_begin_frame();   (input: drain events, derive state)
 *     eli_window_new_frame();           (windows: hover/move/wheel, reset stack)
 *     ... eli_begin(...) / widgets / eli_end() ...
 *     eli_window_render();              (windows: assemble draw data)
 *     eli_render();                     (core: close the frame scope)
 *     eli_input_update_end_frame();     (input: roll per-frame accumulators)
 *
 * @status Phase 7 window lifecycle in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WINDOW_ELI_WINDOW_H
#define ELI_WINDOW_ELI_WINDOW_H

#include "eli_window_types.h"
#include "eli_window_internal.h"
#include "eli_window_interaction.h"
#include "eli_window_scroll.h"
#include "eli_window_settings.h"
#include "eli_window_query.h"

/* Side length (px) of the title-bar close button box. */
#define ELI_WINDOW_CLOSE_BUTTON_SIZE 14.0f

/* ---------------------------------------------------------------------------
 * Condition helper
 * ------------------------------------------------------------------------- */

/**
 * Decide whether a set-next-window setting should apply, given its condition,
 * whether the window is brand new, and whether it is appearing this frame.
 *
 * @param cond       The requested condition (0 == always).
 * @param first_use  True if the window was just created.
 * @param appearing  True if the window is appearing this frame.
 * @return           True if the setting should be applied.
 */
static inline bool eli_window_cond_applies(eli_cond cond, bool first_use, bool appearing)
{
    if (cond == ELI_COND_NONE || (cond & ELI_COND_ALWAYS))
        return true;
    if ((cond & (ELI_COND_ONCE | ELI_COND_FIRST_USE_EVER)) && first_use)
        return true;
    if ((cond & ELI_COND_APPEARING) && appearing)
        return true;
    return false;
}

/* ---------------------------------------------------------------------------
 * Decoration rendering
 * ------------------------------------------------------------------------- */

/** Compute the byte length of a window title up to a "##" hidden-id marker. */
static inline size_t eli_window_visible_label_len(const char *name)
{
    const char *p = name;
    while (p[0] != '\0') {
        if (p[0] == '#' && p[1] == '#')
            break;
        p++;
    }
    return (size_t)(p - name);
}

/**
 * Render a window's background, border, title bar, title text, and (when a
 * p_open pointer is supplied) a close button that clears *p_open when clicked.
 *
 * @param ctx      Context (non-NULL).
 * @param win      Window to decorate (non-NULL).
 * @param style    Active style (non-NULL).
 * @param p_open   Optional open flag; a close button is drawn when non-NULL.
 * @param bg_alpha Background alpha override (< 0 to use the theme alpha).
 */
static inline void eli_window_render_decorations(eli_context *ctx, eli_window *win,
                                                 const eli_style *style, bool *p_open,
                                                 float bg_alpha)
{
    eli_draw_list *dl = &win->draw_list;
    float rounding = style->window_rounding;
    eli_vec2 outer_min = eli_rect_min(win->outer_rect);
    eli_vec2 outer_max = eli_rect_max(win->outer_rect);

    bool is_child = (win->flags & ELI_CHILD_FRAME_STYLE) != 0 || win->parent_window != NULL;
    if ((win->flags & ELI_WINDOW_NO_BACKGROUND) == 0) {
        eli_col32 bg = eli_get_color_u32(is_child ? ELI_COL_CHILD_BG : ELI_COL_WINDOW_BG, 1.0f);
        if (bg_alpha >= 0.0f) {
            eli_vec4 c = eli_color_u32_to_vec4(bg);
            c.w = bg_alpha;
            bg = eli_color_vec4_to_u32(c);
        }
        eli_draw_list_add_rect_filled(dl, outer_min, outer_max, bg, rounding,
                                      ELI_DRAW_ROUND_CORNERS_ALL);
    }

    if (eli_window_has_title_bar(win) && !win->collapsed) {
        bool focused = (ctx->nav_window == win ||
                        (ctx->nav_window && ctx->nav_window->root_window == win));
        eli_col title_slot = focused ? ELI_COL_TITLE_BG_ACTIVE : ELI_COL_TITLE_BG;
        eli_col32 tc = eli_get_color_u32(title_slot, 1.0f);
        eli_draw_list_add_rect_filled(dl, eli_rect_min(win->title_bar_rect),
                                      eli_rect_max(win->title_bar_rect), tc, rounding,
                                      ELI_DRAW_ROUND_CORNERS_TOP);
        eli_col32 text_col = eli_get_color_u32(ELI_COL_TEXT, 1.0f);
        eli_vec2 tp = eli_make_vec2(win->title_bar_rect.x + style->frame_padding.x,
                                    win->title_bar_rect.y + style->frame_padding.y);
        size_t vlen = eli_window_visible_label_len(win->name);
        eli_draw_list_add_text(dl, tp, text_col, win->name, win->name + vlen);
    } else if (eli_window_has_title_bar(win) && win->collapsed) {
        eli_col32 tc = eli_get_color_u32(ELI_COL_TITLE_BG_COLLAPSED, 1.0f);
        eli_draw_list_add_rect_filled(dl, eli_rect_min(win->title_bar_rect),
                                      eli_rect_max(win->title_bar_rect), tc, rounding,
                                      ELI_DRAW_ROUND_CORNERS_ALL);
    }

    if (win->title_bar_height > 0.0f && p_open != NULL) {
        float b = ELI_WINDOW_CLOSE_BUTTON_SIZE;
        eli_vec2 cmin = eli_make_vec2(win->title_bar_rect.x + win->title_bar_rect.w - b -
                                          style->frame_padding.x,
                                      win->title_bar_rect.y + style->frame_padding.y);
        eli_vec2 cmax = eli_make_vec2(cmin.x + b, cmin.y + b);
        bool hovered = ctx->hovered_window == win && eli_is_mouse_hovering_rect(cmin, cmax, false);
        eli_col32 xcol = eli_get_color_u32(hovered ? ELI_COL_BUTTON_HOVERED : ELI_COL_TEXT, 1.0f);
        eli_draw_list_add_line(dl, cmin, cmax, xcol, 1.0f);
        eli_draw_list_add_line(dl, eli_make_vec2(cmax.x, cmin.y), eli_make_vec2(cmin.x, cmax.y),
                               xcol, 1.0f);
        if (hovered && eli_is_mouse_clicked(ELI_MOUSE_BUTTON_LEFT))
            *p_open = false;
    }

    float border = style->window_border_size;
    if (win->parent_window != NULL)
        border = (win->child_flags & ELI_CHILD_BORDERS) ? style->child_border_size : 0.0f;
    if (border > 0.0f && (win->flags & ELI_WINDOW_NO_BACKGROUND) == 0) {
        eli_col32 bc = eli_get_color_u32(ELI_COL_BORDER, 1.0f);
        eli_draw_list_add_rect(dl, outer_min, outer_max, bc, rounding,
                               ELI_DRAW_ROUND_CORNERS_ALL, border);
    }
}

/* ---------------------------------------------------------------------------
 * Begin / End
 * ------------------------------------------------------------------------- */

/** Apply the pending next-window settings to a window during its first Begin. */
static inline void eli_window_apply_next_data(eli_context *ctx, eli_window *win, bool first_use,
                                              bool *out_pos_pending, eli_vec2 *out_pos,
                                              eli_vec2 *out_pivot, float *out_bg_alpha)
{
    *out_pos_pending = false;
    *out_bg_alpha = -1.0f;
    eli_next_window_data *d = ctx->next_window_data;
    if (d == NULL)
        return;

    if (d->has_size && eli_window_cond_applies(d->size_cond, first_use, win->appearing)) {
        if (d->size_val.x > 0.0f) win->size_full.x = d->size_val.x;
        if (d->size_val.y > 0.0f) win->size_full.y = d->size_val.y;
    }
    if (d->has_content_size) {
        win->content_size_explicit = d->content_size_val;
        win->content_size_explicit_valid = true;
    }
    if (d->has_collapsed && eli_window_cond_applies(d->collapsed_cond, first_use, win->appearing))
        win->collapsed = d->collapsed_val;
    if (d->has_pos && eli_window_cond_applies(d->pos_cond, first_use, win->appearing)) {
        *out_pos_pending = true;
        *out_pos = d->pos_val;
        *out_pivot = d->pos_pivot;
    }
    if (d->has_bg_alpha)
        *out_bg_alpha = d->bg_alpha_val;
}

/**
 * Core window entry. Retrieves or creates the named window, applies pending
 * next-window settings, computes geometry/scroll, renders decorations, runs
 * title-bar interactions, and pushes the window as current.
 *
 * @param name    Window name (identity + visible title; non-NULL).
 * @param id      Window id (hash of the full name against the parent seed).
 * @param p_open  Optional open flag; when non-NULL a close button is shown.
 * @param flags   Window flags.
 * @param parent  Parent window for a child window, or NULL for a top-level one.
 * @return        true if the window is not collapsed/clipped (emit contents).
 */
static inline bool eli_begin_ex(const char *name, eli_id id, bool *p_open,
                                eli_window_flags flags, eli_window *parent)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return false;

    const eli_style *style = &ctx->style;
    eli_window *win = eli_window_get_or_create(ctx, name, id);
    if (win == NULL)
        return false;

    bool first_use = (win->last_frame_active < 0);
    bool begun_this_frame = (win->last_frame_active == ctx->frame_count);

    if (begun_this_frame) {
        /* Appended Begin: re-push as current without re-laying-out. Seed the id
         * stack with the window id (mirrors Dear ImGui's PushOverrideID) so
         * widgets in this window hash against the window, not the outer scope;
         * balanced by the matching eli_end. */
        win->begin_count++;
        ctx->window_stack[ctx->window_stack_size++] = win;
        ctx->current_window = win;
        eli_id_stack_push_raw(ctx, win->id);
        return !win->skip_items;
    }

    win->flags = flags;
    win->parent_window = parent;
    win->root_window = parent ? (parent->root_window ? parent->root_window : parent) : win;
    win->appearing = first_use || (win->last_frame_active < ctx->frame_count - 1);
    win->window_padding = style->window_padding;

    /* Docking (Phase 34): resolve this window's dock node via the hook installed
     * by the docking module (NULL when docking is unused). It fills win->dock_node
     * and, when docked, win->dock_body_rect / dock_is_visible. A docked window
     * loses its title bar (the shared tab strip replaces it). */
    if (ctx->dock_begin_window_fn != NULL)
        ctx->dock_begin_window_fn(ctx, win);
    bool win_docked = (win->dock_node != NULL);

    win->title_bar_height = (eli_window_has_title_bar(win) && !win_docked)
                                ? eli_window_effective_font_size(ctx) + style->frame_padding.y * 2.0f
                                : 0.0f;

    bool pos_pending = false;
    eli_vec2 pos_val = win->pos, pivot = eli_make_vec2(0.0f, 0.0f);
    float bg_alpha = -1.0f;
    eli_window_apply_next_data(ctx, win, first_use, &pos_pending, &pos_val, &pivot, &bg_alpha);

    /* Resolve content size (explicit wins, else last-frame measurement). */
    win->content_size = win->content_size_explicit_valid ? win->content_size_explicit
                                                         : win->content_size_measured;

    /* Resolve size_full: auto-resize or first-use default from content. */
    eli_vec2 pad = win->window_padding;
    if (flags & ELI_WINDOW_AUTO_RESIZE) {
        win->size_full.x = win->content_size.x + pad.x * 2.0f;
        win->size_full.y = win->content_size.y + pad.y * 2.0f + win->title_bar_height;
    } else {
        if (win->size_full.x <= 0.0f)
            win->size_full.x = eli_max_f(win->content_size.x + pad.x * 2.0f, style->window_min_size.x);
        if (win->size_full.y <= 0.0f)
            win->size_full.y = eli_max_f(win->content_size.y + pad.y * 2.0f + win->title_bar_height,
                                         style->window_min_size.y);
    }
    if (ctx->next_window_data && ctx->next_window_data->has_size_constraint) {
        eli_vec2 mn = ctx->next_window_data->size_constraint_min;
        eli_vec2 mx = ctx->next_window_data->size_constraint_max;
        win->size_full.x = eli_max_f(win->size_full.x, mn.x);
        win->size_full.y = eli_max_f(win->size_full.y, mn.y);
        if (mx.x >= 0.0f) win->size_full.x = eli_min_f(win->size_full.x, mx.x);
        if (mx.y >= 0.0f) win->size_full.y = eli_min_f(win->size_full.y, mx.y);
    }
    win->size_full.x = eli_max_f(win->size_full.x, style->window_min_size.x);
    win->size_full.y = eli_max_f(win->size_full.y, win->title_bar_height);

    /* Collapsed windows shrink to just the title bar and skip their body. */
    if (win->collapsed && eli_window_has_title_bar(win)) {
        win->size = eli_make_vec2(win->size_full.x, win->title_bar_height);
        win->skip_items = true;
    } else {
        win->size = win->size_full;
        win->skip_items = false;
    }

    if (pos_pending)
        win->pos = eli_vec2_sub(pos_val, eli_make_vec2(pivot.x * win->size.x, pivot.y * win->size.y));

    /* Docking geometry override: a docked window fills its node body and drops
     * its title bar. Non-selected tabs emit nothing, so return early after the
     * minimal bookkeeping needed to keep the window/id stacks balanced. */
    if (win_docked) {
        eli_rect body = win->dock_body_rect;
        win->size_full = eli_make_vec2(body.w, body.h);
        win->size = win->size_full;
        win->collapsed = false;
        win->skip_items = false;
        win->pos = eli_make_vec2(body.x, body.y);

        if (!win->dock_is_visible) {
            win->active = true;
            win->was_active = (win->last_frame_active == ctx->frame_count - 1);
            win->last_frame_active = ctx->frame_count;
            win->hidden = true;
            win->skip_items = true;
            win->begin_count = 1;
            eli_draw_list_reset(&win->draw_list);
            eli_clear_next_window_data(ctx);
            ctx->window_stack[ctx->window_stack_size++] = win;
            ctx->current_window = win;
            eli_id_stack_push_raw(ctx, win->id);
            return false;
        }
    }

    /* Focus + z-order registration (top-level windows only). */
    if (win->root_window == win && eli_window_focus_index(ctx, win) < 0)
        eli_window_add_to_focus_order(ctx, win);
    bool focus_request = ctx->next_window_data && ctx->next_window_data->has_focus;
    if (focus_request || (win->appearing && (flags & ELI_WINDOW_NO_FOCUS_ON_APPEARING) == 0))
        eli_window_focus(ctx, win);

    eli_window_update_layout(ctx, win, style);

    if (ctx->next_window_data && ctx->next_window_data->has_scroll) {
        eli_vec2 s = ctx->next_window_data->scroll_val;
        if (s.x >= 0.0f) win->scroll.x = s.x;
        if (s.y >= 0.0f) win->scroll.y = s.y;
        eli_window_clamp_scroll(win);
    }

    eli_clear_next_window_data(ctx);

    /* Seed the layout cursor at the (scrolled) work-area origin. */
    win->cursor_start_pos = eli_make_vec2(win->content_region_rect.x - win->scroll.x,
                                          win->content_region_rect.y - win->scroll.y);
    win->cursor_pos = win->cursor_start_pos;
    win->cursor_max_pos = win->cursor_start_pos;

    /* Reset per-frame layout advancement state (Phase 8). The indent carries the
     * padding/scroll base so a fresh line lands back on cursor_start_pos.x. */
    win->cursor_pos_prev_line = win->cursor_start_pos;
    win->curr_line_size = eli_make_vec2(0.0f, 0.0f);
    win->prev_line_size = eli_make_vec2(0.0f, 0.0f);
    win->curr_line_text_baseline_offset = 0.0f;
    win->prev_line_text_baseline_offset = 0.0f;
    win->indent = win->cursor_start_pos.x - win->pos.x;
    win->group_offset = 0.0f;
    win->item_width_default = (float)(long)(win->size.x * 0.65f);
    win->item_width = win->item_width_default;
    win->is_same_line = false;
    win->is_set_pos = false;

    win->active = true;
    win->was_active = (win->last_frame_active == ctx->frame_count - 1);
    win->last_frame_active = ctx->frame_count;
    win->hidden = false;
    win->begin_count = 1;

    /* Rebuild this frame's draw list and draw decorations. */
    eli_draw_list_reset(&win->draw_list);
    if (ctx->font && ctx->font->container_atlas) {
        win->draw_list.tex_uv_white_pixel = ctx->font->container_atlas->tex_uv_white_pixel;
        eli_draw_list_push_texture_id(&win->draw_list, ctx->font->container_atlas->tex_id);
    }
    eli_window_render_decorations(ctx, win, style, p_open, bg_alpha);
    eli_window_render_scrollbars(win, style);

    /* Docking: the visible docked window renders the node's shared tab strip and
     * handles tab switching / tear-out (hook installed by the docking module). */
    if (win_docked && ctx->dock_tab_bar_fn != NULL)
        ctx->dock_tab_bar_fn(ctx, win);

    /* Title-bar interactions (move begins the drag; double-click toggles collapse).
     * Skipped for docked windows, which have no title bar of their own. */
    if (!win_docked) {
        eli_window_handle_title_move(ctx, win);
        eli_window_handle_resize(ctx, win, style);
        eli_window_handle_title_collapse(ctx, win);
    }

    /* Clip subsequent contents to the work area. */
    eli_draw_list_push_clip_rect(&win->draw_list, eli_rect_min(win->content_region_rect),
                                 eli_rect_max(win->content_region_rect), true);

    ctx->window_stack[ctx->window_stack_size++] = win;
    ctx->current_window = win;
    /* Seed the id stack with the window id (Dear ImGui PushOverrideID) so that
     * identical labels in different windows derive distinct ids. Popped by the
     * matching eli_end, keeping the stack balanced with the window stack. */
    eli_id_stack_push_raw(ctx, win->id);
    return !win->skip_items;
}

/**
 * Begin a top-level window.
 *
 * @param name    Window name and visible title (identity source; non-NULL).
 * @param p_open  Optional pointer to the window's open flag; when non-NULL a
 *                close button is drawn and cleared on click.
 * @param flags   Window flags (see eli_window_flags).
 * @return        true if the window body should be emitted (not collapsed).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline bool eli_begin(const char *name, bool *p_open, eli_window_flags flags)
{
    if (name == NULL)
        return false;
    eli_id id = eli_hash_str(name, 0);
    return eli_begin_ex(name, id, p_open, flags, NULL);
}

/**
 * Close the current window scope opened by eli_begin/eli_begin_child. Pops the
 * content clip rect, measures content, and restores the previous window.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_end(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->window_stack_size <= 0)
        return;

    eli_window *win = ctx->current_window;
    if (win != NULL && win->begin_count <= 1) {
        /* Measure content from the layout cursor for next frame's scroll range. */
        win->content_size_measured = eli_make_vec2(
            eli_max_f(0.0f, win->cursor_max_pos.x - win->cursor_start_pos.x),
            eli_max_f(0.0f, win->cursor_max_pos.y - win->cursor_start_pos.y));
        eli_draw_list_pop_clip_rect(&win->draw_list);
    }
    if (win != NULL)
        win->begin_count--;

    /* Pop the window-id seed pushed by the matching eli_begin_ex, keeping the id
     * stack balanced with the window stack across every begin/end pair. */
    eli_pop_id();

    ctx->window_stack_size--;
    ctx->current_window = (ctx->window_stack_size > 0)
                              ? ctx->window_stack[ctx->window_stack_size - 1]
                              : NULL;
}

/* ---------------------------------------------------------------------------
 * Per-frame update
 * ------------------------------------------------------------------------- */

/** Find the top-most window under the mouse from last frame's geometry. */
static inline eli_window *eli_window_find_hovered(eli_context *ctx)
{
    const eli_io *io = &ctx->io;
    if (!eli_mouse_pos_is_valid(io->mouse_pos))
        return NULL;

    for (int i = ctx->windows_focus_order_count - 1; i >= 0; i--) {
        eli_window *win = ctx->windows_focus_order[i];
        if (win->last_frame_active < ctx->frame_count - 1)
            continue;
        if (win->flags & ELI_WINDOW_NO_MOUSE_INPUTS)
            continue;
        eli_rect r = win->collapsed ? win->title_bar_rect : win->outer_rect;
        if (eli_rect_contains(r, io->mouse_pos))
            return win;
    }
    return NULL;
}

/**
 * Begin-of-frame window update: reset the window stack, continue/finish a title
 * drag, determine the hovered window from last frame's geometry, and apply
 * mouse-wheel scrolling. Call after eli_new_frame and eli_input_update_begin_frame.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_window_new_frame(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;

    ctx->window_stack_size = 0;
    ctx->current_window = NULL;
    ctx->render_draw_lists_count = 0;

    eli_window_update_moving(ctx);
    ctx->hovered_window = eli_window_find_hovered(ctx);
    eli_window_update_wheel(ctx);
}

/* ---------------------------------------------------------------------------
 * Draw-data assembly
 * ------------------------------------------------------------------------- */

/** Append a draw list to the frame's render list. */
static inline void eli_window_push_render_draw_list(eli_context *ctx, eli_draw_list *dl)
{
    ctx->render_draw_lists = (eli_draw_list **)eli_window_grow_ptr_array(
        (void **)ctx->render_draw_lists, &ctx->render_draw_lists_capacity,
        ctx->render_draw_lists_count + 1);
    ctx->render_draw_lists[ctx->render_draw_lists_count++] = dl;
}

/** Append one window's draw list to the frame's render list. */
static inline void eli_window_push_render_list(eli_context *ctx, eli_window *win)
{
    eli_window_push_render_draw_list(ctx, &win->draw_list);
}

/**
 * Assemble the frame's draw data from every active window's draw list, ordered
 * back-to-front by focus order (with each root's child windows following it), and
 * publish it on the context so eli_get_draw_data returns it. Call after all
 * eli_begin/eli_end pairs and before eli_render.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_window_render(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;

    ctx->render_draw_lists_count = 0;

    /* Background draw list (Phase 27 util) renders behind every window. Included
     * only when it holds geometry; owned by the util phase via the context. */
    if (ctx->background_draw_list != NULL && ctx->background_draw_list->idx_count > 0)
        eli_window_push_render_draw_list(ctx, ctx->background_draw_list);

    for (int i = 0; i < ctx->windows_focus_order_count; i++) {
        eli_window *root = ctx->windows_focus_order[i];
        if (root->last_frame_active != ctx->frame_count)
            continue;
        eli_window_push_render_list(ctx, root);
        for (int j = 0; j < ctx->windows_count; j++) {
            eli_window *w = ctx->windows[j];
            if (w != root && w->last_frame_active == ctx->frame_count &&
                (w->root_window ? w->root_window : w) == root)
                eli_window_push_render_list(ctx, w);
        }
    }

    /* Foreground draw list (Phase 27 util) renders in front of every window. */
    if (ctx->foreground_draw_list != NULL && ctx->foreground_draw_list->idx_count > 0)
        eli_window_push_render_draw_list(ctx, ctx->foreground_draw_list);

    if (ctx->draw_data == NULL)
        ctx->draw_data = (eli_draw_data *)calloc(1, sizeof(*ctx->draw_data));
    eli_draw_data *dd = ctx->draw_data;
    if (dd == NULL)
        return;

    int total_vtx = 0, total_idx = 0;
    for (int i = 0; i < ctx->render_draw_lists_count; i++) {
        total_vtx += (int)ctx->render_draw_lists[i]->vtx_count;
        total_idx += (int)ctx->render_draw_lists[i]->idx_count;
    }

    dd->valid = true;
    dd->cmd_lists = ctx->render_draw_lists;
    dd->cmd_lists_count = ctx->render_draw_lists_count;
    dd->total_vtx_count = total_vtx;
    dd->total_idx_count = total_idx;
    dd->display_pos = eli_make_vec2(0.0f, 0.0f);
    dd->display_size = ctx->io.display_size;
    dd->framebuffer_scale = ctx->io.display_framebuffer_scale;

    ctx->io.metrics_render_vertices = total_vtx;
    ctx->io.metrics_render_indices = total_idx;
    ctx->io.metrics_render_windows = ctx->render_draw_lists_count;
}

#include "eli_window_child.h"

#endif /* ELI_WINDOW_ELI_WINDOW_H */
