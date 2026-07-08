/**
 * @file eli_context.h
 * @brief The eli_context container plus the context lifecycle (create/destroy,
 *        current-context accessors) and the per-frame lifecycle
 *        (new_frame/end_frame/render/get_draw_data).
 *
 * A single global "current context" is addressed via eli_get/set_current_context.
 * eli_create_context populates the style with Dear ImGui default sizing values;
 * the color theme is left zeroed for the style phase to fill.
 *
 * @status Phase 1 lifecycle in use. Draw-data production is stubbed until the
 *         draw phase (eli_render currently only closes the frame scope).
 * @issues None
 * @todo None
 */
#ifndef ELI_CORE_ELI_CONTEXT_H
#define ELI_CORE_ELI_CONTEXT_H

#include "eli_types.h"
#include "eli_enums.h"
#include "eli_io.h"
#include "eli_style_types.h"

/* Draw data is produced by the draw phase; the context only holds a pointer.
 * eli_draw_list is likewise forward-declared: windows own one and the context
 * gathers them for a frame's draw data. */
typedef struct eli_draw_data eli_draw_data;
typedef struct eli_draw_list eli_draw_list;

/**
 * Top-level elimgui state container. Holds the IO bridge, the active style, and
 * frame bookkeeping. Created with eli_create_context and made current with
 * eli_set_current_context.
 */
/* Internal stack capacities. Owned by the orchestrator; used by later phases. */
#define ELI_ID_STACK_MAX          128
#define ELI_STYLE_COLOR_STACK_MAX 128
#define ELI_STYLE_VAR_STACK_MAX   128
#define ELI_ITEM_FLAG_STACK_MAX    64
#define ELI_FONT_STACK_MAX         32

/* Maximum window nesting depth (windows + child windows) on the current stack. */
#define ELI_WINDOW_STACK_MAX       64

/* Layout stacks (Phase 8): pushed/popped item-width and text-wrap positions, and
 * the group nesting used by eli_begin_group/eli_end_group. */
#define ELI_ITEM_WIDTH_STACK_MAX   64
#define ELI_TEXT_WRAP_POS_STACK_MAX 64
#define ELI_GROUP_STACK_MAX        32

/* Opaque handles defined by later phases; the context stores pointers to them.
 * C11 permits these identical typedefs to be repeated by the owning phase. */
typedef struct eli_font    eli_font;
typedef struct eli_storage eli_storage;

/* Window handles defined by the window phase (Phase 7). The context holds the
 * window registry, the current window stack, and the next-window settings; the
 * window module fills the layout/behavior. Forward-declared here so the context
 * can store pointers without depending on the window headers. */
typedef struct eli_window           eli_window;
typedef struct eli_next_window_data eli_next_window_data;

/* Dock node handle defined by the docking phase (Phase 34). The context owns the
 * node pool; the docking module fills the tree layout/behavior. Forward-declared
 * here so the context can store pointers without depending on the dock headers. */
typedef struct eli_dock_node eli_dock_node;

/* Backup record for the style color stack (Phase 6): a color index + old value. */
typedef struct eli_color_mod {
    int      col;             /* eli_col index */
    eli_vec4 backup_value;
} eli_color_mod;

/* Backup record for the style var stack (Phase 6): a var index + old value
 * (backup[0] for a float var, backup[0..1] for a vec2 var). */
typedef struct eli_style_mod {
    int   var;                /* eli_style_var index */
    float backup[2];
} eli_style_mod;

/* Backup record for the layout group stack (Phase 8). eli_begin_group snapshots
 * the window's layout cursor/indent state; eli_end_group restores it and emits the
 * enclosing bounding box as the last item. */
typedef struct eli_group_data {
    eli_vec2 backup_cursor_pos;
    eli_vec2 backup_cursor_max_pos;
    eli_vec2 backup_curr_line_size;
    float    backup_curr_line_text_baseline_offset;
    float    backup_indent;
    float    backup_group_offset;
} eli_group_data;

typedef struct eli_context {
    bool initialized;
    bool within_frame_scope;   /* true between eli_new_frame and end_frame/render */
    int frame_count;
    double time;
    eli_io io;
    eli_style style;
    eli_draw_data *draw_data;  /* NULL until the draw phase produces output */

    /* --- Font state + stack (Phase 3) --- */
    eli_font *font;            /* current font (NULL until a font is built) */
    float     font_size;       /* current font size in pixels */
    float     font_base_size;  /* unscaled base size */
    eli_font *font_stack[ELI_FONT_STACK_MAX];
    int       font_stack_size;

    /* --- ID system (Phase 5) --- */
    eli_id    active_id;
    eli_id    active_id_previous_frame;
    bool      active_id_is_just_activated;
    eli_id    hot_id;
    eli_id    hot_id_previous_frame;
    eli_id    id_stack[ELI_ID_STACK_MAX];  /* hashing seed stack */
    int       id_stack_size;
    eli_storage *state_storage;            /* current key/value storage */

    /* --- Style stacks (Phase 6) --- */
    eli_color_mod color_stack[ELI_STYLE_COLOR_STACK_MAX];
    int           color_stack_size;
    eli_style_mod style_var_stack[ELI_STYLE_VAR_STACK_MAX];
    int           style_var_stack_size;
    int           item_flags_stack[ELI_ITEM_FLAG_STACK_MAX];
    int           item_flags_stack_size;
    int           current_item_flags;

    /* --- Window / nav state (Phase 7) ---
     * All eli_window pointers below are owned by the window pool `windows`,
     * which the window module allocates and frees. `window_shutdown_fn` is the
     * hook the window module registers so eli_destroy_context can release the
     * window heap state (draw lists, names, arrays) it cannot see the layout of. */
    eli_window **windows;                 /* window pool (creation order) */
    int          windows_count;
    int          windows_capacity;
    eli_window **windows_focus_order;     /* root windows, back-to-front (last = top) */
    int          windows_focus_order_count;
    int          windows_focus_order_capacity;
    eli_window  *window_stack[ELI_WINDOW_STACK_MAX]; /* begin/end nesting stack */
    int          window_stack_size;
    eli_window  *current_window;          /* top of the window stack, or NULL */
    eli_window  *hovered_window;          /* window under the mouse this frame */
    eli_window  *moving_window;           /* window being dragged by its title bar */
    eli_window  *nav_window;              /* focused window */
    eli_next_window_data *next_window_data; /* set-next-window-* settings (lazily allocated) */

    eli_draw_list **render_draw_lists;    /* per-frame draw lists gathered for draw_data */
    int             render_draw_lists_count;
    int             render_draw_lists_capacity;

    /* --- Layout state (Phase 8) ---
     * Item-width and text-wrap-position stacks (push/pop balanced within a frame),
     * the group nesting stack, the one-shot next-item width, and the last-item
     * record written by eli_item_add and consumed by item-query APIs (Phase 10). */
    float item_width_stack[ELI_ITEM_WIDTH_STACK_MAX];
    int   item_width_stack_size;
    float text_wrap_pos_stack[ELI_TEXT_WRAP_POS_STACK_MAX];
    int   text_wrap_pos_stack_size;
    eli_group_data group_stack[ELI_GROUP_STACK_MAX];
    int            group_stack_size;

    float next_item_width;                /* one-shot width from eli_set_next_item_width */
    bool  has_next_item_width;            /* true when next_item_width is live */

    eli_id   last_item_id;                /* id of the most recent eli_item_add */
    eli_rect last_item_rect;             /* bounding rect of the most recent item */
    int      last_item_status_flags;      /* eli_item_status_flags for that item */

    /* --- Widget interaction state (Phase 9/10) ---
     * Edge/edit bookkeeping consumed by the item-status queries. active_id and
     * hot_id (mapped from ImGui's ActiveId/HoveredId) are reused from the ID
     * system above; these add the "edited" tracking across an item's active
     * lifetime and the nav-focus id. Rolled once per frame by eli_new_frame. */
    eli_id nav_id;                                 /* focused item id (nav; 0 until nav lands) */
    bool   active_id_has_been_edited_this_frame;   /* an edit happened while active this frame */
    bool   active_id_has_been_edited_before;       /* an edit happened during this active spell */
    bool   active_id_previous_frame_has_been_edited_before; /* rolled snapshot for deactivated-after-edit */

    void (*window_shutdown_fn)(struct eli_context *ctx); /* frees window state on destroy */
    void (*widget_shutdown_fn)(struct eli_context *ctx); /* frees widget-subsystem state on destroy */

    /* --- Docking (Phase 34) ---
     * The dock-node pool holds every node in the split trees (each node is heap
     * allocated individually, so pointers stay stable across pool growth). The
     * two begin hooks are installed by the docking module so eli_begin can defer
     * dock resolution / tab-strip rendering without a window->docking include
     * cycle; both are NULL when docking is not compiled in, leaving windows
     * behaving exactly as before. A single pending drag-to-dock request is queued
     * by the drag path and consumed by eli_dock_new_frame. dock_shutdown_fn frees
     * the pool on destroy. */
    eli_dock_node **dock_nodes;            /* node pool (heap; freed by dock_shutdown_fn) */
    int             dock_nodes_count;
    int             dock_nodes_capacity;
    bool            has_dock_request;      /* a drag-to-dock drop is pending */
    eli_id          dock_request_target;   /* target node id for the pending drop */
    eli_id          dock_request_window;   /* window id being dropped */
    int             dock_request_dir;      /* eli_dock_dir drop zone for the drop */
    eli_id          dock_drag_window_id;   /* window whose tab/title is mid drag-to-dock */
    void (*dock_begin_window_fn)(struct eli_context *ctx, struct eli_window *win);
    void (*dock_tab_bar_fn)(struct eli_context *ctx, struct eli_window *win);
    void (*dock_shutdown_fn)(struct eli_context *ctx); /* frees dock-node pool on destroy */

    /* --- Background / foreground draw lists (Phase 27) ---
     * Persistent per-frame draw lists rendered behind (background) and in front
     * of (foreground) all windows. Lazily allocated by the util phase, cleared
     * each frame, and assembled into draw_data (background first, foreground
     * last). Freed via util_shutdown_fn on destroy. */
    eli_draw_list *background_draw_list;
    eli_draw_list *foreground_draw_list;
    void (*util_shutdown_fn)(struct eli_context *ctx); /* frees util (bg/fg) state on destroy */
} eli_context;

/* Single global "current" context. Header-only builds are a single translation
 * unit (the app includes elimgui.h once), so file-static storage is correct. */
static eli_context *g_eli_context = NULL;

/* ---------------------------------------------------------------------------
 * Default style
 * ------------------------------------------------------------------------- */

/**
 * Populate a style with Dear ImGui's default sizing/spacing/rounding/behavior
 * values. The colors[] table is left untouched (zeroed by the caller) so the
 * style phase can apply a theme.
 *
 * @param style  Style to initialize (must be non-NULL, already zeroed).
 */
static inline void eli_style_set_defaults(eli_style *style)
{
    style->alpha = 1.0f;
    style->disabled_alpha = 0.60f;
    style->window_padding = eli_make_vec2(8.0f, 8.0f);
    style->window_rounding = 0.0f;
    style->window_border_size = 1.0f;
    style->window_min_size = eli_make_vec2(32.0f, 32.0f);
    style->window_title_align = eli_make_vec2(0.0f, 0.5f);
    style->window_menu_button_position = ELI_DIR_LEFT;
    style->child_rounding = 0.0f;
    style->child_border_size = 1.0f;
    style->popup_rounding = 0.0f;
    style->popup_border_size = 1.0f;
    style->frame_padding = eli_make_vec2(4.0f, 3.0f);
    style->frame_rounding = 0.0f;
    style->frame_border_size = 0.0f;
    style->item_spacing = eli_make_vec2(8.0f, 4.0f);
    style->item_inner_spacing = eli_make_vec2(4.0f, 4.0f);
    style->cell_padding = eli_make_vec2(4.0f, 2.0f);
    style->touch_extra_padding = eli_make_vec2(0.0f, 0.0f);
    style->indent_spacing = 21.0f;
    style->columns_min_spacing = 6.0f;
    style->scrollbar_size = 14.0f;
    style->scrollbar_rounding = 9.0f;
    style->grab_min_size = 12.0f;
    style->grab_rounding = 0.0f;
    style->log_slider_deadzone = 4.0f;
    style->tab_rounding = 5.0f;
    style->tab_border_size = 0.0f;
    style->tab_min_width_for_close_button = 0.0f;
    style->tab_bar_border_size = 1.0f;
    style->tab_bar_overline_size = 1.0f;
    style->table_angled_headers_angle = 35.0f * (ELI_PI / 180.0f);
    style->table_angled_headers_text_align = eli_make_vec2(0.5f, 0.0f);
    style->color_button_position = ELI_DIR_RIGHT;
    style->button_text_align = eli_make_vec2(0.5f, 0.5f);
    style->selectable_text_align = eli_make_vec2(0.0f, 0.0f);
    style->separator_text_border_size = 3.0f;
    style->separator_text_align = eli_make_vec2(0.0f, 0.5f);
    style->separator_text_padding = eli_make_vec2(20.0f, 3.0f);
    style->display_window_padding = eli_make_vec2(19.0f, 19.0f);
    style->display_safe_area_padding = eli_make_vec2(3.0f, 3.0f);
    style->docking_separator_size = 2.0f;
    style->mouse_cursor_scale = 1.0f;
    style->anti_aliased_lines = true;
    style->anti_aliased_lines_use_tex = true;
    style->anti_aliased_fill = true;
    style->curve_tessellation_tol = 1.25f;
    style->circle_tessellation_max_error = 0.30f;

    style->hover_stationary_delay = 0.15f;
    style->hover_delay_short = 0.15f;
    style->hover_delay_normal = 0.40f;
    style->hover_flags_for_tooltip_mouse =
        ELI_HOVERED_STATIONARY | ELI_HOVERED_DELAY_SHORT | ELI_HOVERED_ALLOW_WHEN_DISABLED;
    style->hover_flags_for_tooltip_nav =
        ELI_HOVERED_NO_SHARED_DELAY | ELI_HOVERED_DELAY_NORMAL | ELI_HOVERED_ALLOW_WHEN_DISABLED;
}

/* ---------------------------------------------------------------------------
 * Context lifecycle
 * ------------------------------------------------------------------------- */

/**
 * Create a new elimgui context with default IO and style values. If no context
 * is current yet, the new context becomes current (mirrors Dear ImGui).
 *
 * @return  Newly allocated context, or NULL on allocation failure. Caller owns
 *          it and must release it with eli_destroy_context.
 *
 * Thread-safe: no (mutates the global current-context pointer)
 * Reentrant: no
 */
static inline eli_context *eli_create_context(void)
{
    eli_context *ctx = (eli_context *)calloc(1, sizeof(*ctx));
    if (!ctx)
        return NULL;

    ctx->io.font_global_scale = 1.0f;
    ctx->io.display_framebuffer_scale = eli_make_vec2(1.0f, 1.0f);
    ctx->io.mouse_pos = eli_make_vec2(-FLT_MAX, -FLT_MAX);
    ctx->io.mouse_pos_prev = eli_make_vec2(-FLT_MAX, -FLT_MAX);
    ctx->io.mouse_draw_cursor = ELI_MOUSE_CURSOR_ARROW;

    eli_style_set_defaults(&ctx->style);

    ctx->initialized = true;

    if (g_eli_context == NULL)
        g_eli_context = ctx;

    return ctx;
}

/**
 * Destroy a context created by eli_create_context. Passing NULL destroys the
 * current context. If the destroyed context was current, the current pointer is
 * cleared.
 *
 * @param ctx  Context to destroy, or NULL to destroy the current context.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_destroy_context(eli_context *ctx)
{
    if (ctx == NULL)
        ctx = g_eli_context;
    if (ctx == NULL)
        return;

    if (g_eli_context == ctx)
        g_eli_context = NULL;

    /* Let the window module release its heap state (window pool, draw lists,
     * names, focus/render arrays, next-window data) before the context is gone.
     * The hook is registered lazily by the window module when the first window
     * is created; it is NULL for contexts that never used windows. */
    if (ctx->window_shutdown_fn)
        ctx->window_shutdown_fn(ctx);

    /* Release widget-subsystem heap state (table/tab pools, drag-drop payload).
     * The hook is registered lazily by the frame-lifecycle layer (eli_frame.h)
     * when eli_frame_begin runs; it is NULL for contexts that never used it, so
     * a bare core+widgets user should call the widget shutdowns directly. */
    if (ctx->widget_shutdown_fn)
        ctx->widget_shutdown_fn(ctx);

    /* Release util (background/foreground) draw-list heap state. Registered
     * lazily by the util phase when a bg/fg draw list is first requested. */
    if (ctx->util_shutdown_fn)
        ctx->util_shutdown_fn(ctx);

    /* Release the dock-node pool (Phase 34). Registered lazily by the docking
     * module when the first node is created; NULL for contexts that never
     * docked. */
    if (ctx->dock_shutdown_fn)
        ctx->dock_shutdown_fn(ctx);

    free(ctx);
}

/**
 * @return  The current context, or NULL if none is set.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline eli_context *eli_get_current_context(void)
{
    return g_eli_context;
}

/**
 * Set the current context.
 *
 * @param ctx  Context to make current (may be NULL to clear).
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_set_current_context(eli_context *ctx)
{
    g_eli_context = ctx;
}

/**
 * @return  Pointer to the current context's IO block, or NULL if no context.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline eli_io *eli_get_io(void)
{
    return g_eli_context ? &g_eli_context->io : NULL;
}

/**
 * @return  Pointer to the current context's style, or NULL if no context.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline eli_style *eli_get_style(void)
{
    return g_eli_context ? &g_eli_context->style : NULL;
}

/* ---------------------------------------------------------------------------
 * Frame lifecycle
 * ------------------------------------------------------------------------- */

/**
 * Begin a new UI frame on the current context: advances the frame counter,
 * opens the frame scope, and derives framerate from io.delta_time.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_new_frame(void)
{
    eli_context *ctx = g_eli_context;
    if (ctx == NULL)
        return;

    ctx->frame_count++;
    ctx->within_frame_scope = true;
    ctx->time += (double)ctx->io.delta_time;
    ctx->io.framerate = (ctx->io.delta_time > 0.0f) ? (1.0f / ctx->io.delta_time) : 0.0f;

    /* Roll the per-frame widget-interaction edges (mirrors Dear ImGui's NewFrame):
     * snapshot the active/hot ids into their previous-frame fields, clear the hot
     * id so this frame's widgets re-establish hover, and reset the just-activated /
     * edited-this-frame flags used by the item-status queries. */
    ctx->active_id_previous_frame = ctx->active_id;
    ctx->active_id_previous_frame_has_been_edited_before = ctx->active_id_has_been_edited_before;
    ctx->hot_id_previous_frame = ctx->hot_id;
    ctx->hot_id = 0u;
    ctx->active_id_is_just_activated = false;
    ctx->active_id_has_been_edited_this_frame = false;
}

/**
 * Close the current frame scope. Safe to call once per frame; a no-op if the
 * frame scope is not open.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_end_frame(void)
{
    eli_context *ctx = g_eli_context;
    if (ctx == NULL || !ctx->within_frame_scope)
        return;

    ctx->within_frame_scope = false;
}

/**
 * Finalize the frame for rendering. Ensures the frame scope is closed (calling
 * eli_end_frame if the caller did not) so draw data can be produced. Draw-data
 * assembly is added by the draw phase.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_render(void)
{
    eli_context *ctx = g_eli_context;
    if (ctx == NULL)
        return;

    if (ctx->within_frame_scope)
        eli_end_frame();
}

/**
 * @return  Draw data assembled by the last eli_render, or NULL if none/not yet
 *          produced (draw output arrives in the draw phase).
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline eli_draw_data *eli_get_draw_data(void)
{
    return g_eli_context ? g_eli_context->draw_data : NULL;
}

#endif /* ELI_CORE_ELI_CONTEXT_H */
