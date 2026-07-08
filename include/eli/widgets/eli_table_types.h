/**
 * @file eli_table_types.h
 * @brief Table widget data model: the public table/column flag enums, sort-spec
 *        structures, the internal per-column and per-table state, and the
 *        module-private table pool + current-table stack (heap-allocated, freed
 *        via eli_table_shutdown).
 *
 * The table pool persists column widths / sort order / hidden state across frames
 * keyed by the table id. All state lives in file-static storage owned by this
 * header, so the whole table module is self-contained: nothing in the core context
 * knows about tables. Call eli_table_shutdown() once before destroying the context
 * to release the pool.
 *
 * @status Phase 19 table types in use.
 * @issues None
 * @todo Multi-column sort keeps a single primary spec (see eli_table.h).
 */
#ifndef ELI_WIDGETS_ELI_TABLE_TYPES_H
#define ELI_WIDGETS_ELI_TABLE_TYPES_H

#include "../core/eli_platform.h"
#include "../core/eli_types.h"
#include "../core/eli_enums.h"

/* ---------------------------------------------------------------------------
 * Capacities
 * ------------------------------------------------------------------------- */

/* Hard cap on columns in one table (mirrors Dear ImGui's 512-bit column budget,
 * scaled down for elimgui's fixed-size per-table arrays). */
#define ELI_TABLE_MAX_COLUMNS       64

/* Bytes reserved for a column's copied header label (name is optional). */
#define ELI_TABLE_COLUMN_NAME_MAX   32

/* Maximum table nesting depth on the current-table stack. */
#define ELI_TABLE_STACK_MAX          8

/* Thickness of drawn table borders, in pixels. */
#define ELI_TABLE_BORDER_SIZE        1.0f

/* Half-width of a column-border grab band used for resize hit-testing. */
#define ELI_TABLE_RESIZE_HALF_WIDTH  4.0f

/* Fallback content width for an auto/fixed column with no explicit width and no
 * header label to measure. */
#define ELI_TABLE_DEFAULT_COLUMN_WIDTH 60.0f

/* ---------------------------------------------------------------------------
 * Sort direction
 * ------------------------------------------------------------------------- */

typedef int eli_sort_direction;
enum eli_sort_direction_ {
    ELI_SORT_NONE       = 0,
    ELI_SORT_ASCENDING  = 1,
    ELI_SORT_DESCENDING = 2
};

/* ---------------------------------------------------------------------------
 * Table flags
 * ------------------------------------------------------------------------- */

typedef int eli_table_flags;
enum eli_table_flags_ {
    ELI_TABLE_NONE                       = 0,
    /* Features */
    ELI_TABLE_RESIZABLE                  = 1 << 0,
    ELI_TABLE_REORDERABLE                = 1 << 1,
    ELI_TABLE_HIDEABLE                   = 1 << 2,
    ELI_TABLE_SORTABLE                   = 1 << 3,
    ELI_TABLE_NO_SAVED_SETTINGS          = 1 << 4,
    ELI_TABLE_CONTEXT_MENU_IN_BODY       = 1 << 5,
    /* Decorations */
    ELI_TABLE_ROW_BG                     = 1 << 6,
    ELI_TABLE_BORDERS_INNER_H            = 1 << 7,
    ELI_TABLE_BORDERS_OUTER_H            = 1 << 8,
    ELI_TABLE_BORDERS_INNER_V            = 1 << 9,
    ELI_TABLE_BORDERS_OUTER_V            = 1 << 10,
    ELI_TABLE_BORDERS_H                  = ELI_TABLE_BORDERS_INNER_H | ELI_TABLE_BORDERS_OUTER_H,
    ELI_TABLE_BORDERS_V                  = ELI_TABLE_BORDERS_INNER_V | ELI_TABLE_BORDERS_OUTER_V,
    ELI_TABLE_BORDERS_INNER              = ELI_TABLE_BORDERS_INNER_H | ELI_TABLE_BORDERS_INNER_V,
    ELI_TABLE_BORDERS_OUTER              = ELI_TABLE_BORDERS_OUTER_H | ELI_TABLE_BORDERS_OUTER_V,
    ELI_TABLE_BORDERS                    = ELI_TABLE_BORDERS_INNER | ELI_TABLE_BORDERS_OUTER,
    ELI_TABLE_NO_BORDERS_IN_BODY         = 1 << 11,
    ELI_TABLE_NO_BORDERS_IN_BODY_UNTIL_RESIZE = 1 << 12,
    /* Sizing policy */
    ELI_TABLE_SIZING_FIXED_FIT           = 1 << 13,
    ELI_TABLE_SIZING_FIXED_SAME          = 1 << 14,
    ELI_TABLE_SIZING_STRETCH_PROP        = 1 << 15,
    ELI_TABLE_SIZING_STRETCH_SAME        = 1 << 16,
    /* Sizing extras */
    ELI_TABLE_NO_HOST_EXTEND_X           = 1 << 17,
    ELI_TABLE_NO_HOST_EXTEND_Y           = 1 << 18,
    ELI_TABLE_NO_KEEP_COLUMNS_VISIBLE    = 1 << 19,
    ELI_TABLE_PRECISE_WIDTHS             = 1 << 20,
    /* Clipping */
    ELI_TABLE_NO_CLIP                    = 1 << 21,
    /* Padding */
    ELI_TABLE_PAD_OUTER_X                = 1 << 22,
    ELI_TABLE_NO_PAD_OUTER_X             = 1 << 23,
    ELI_TABLE_NO_PAD_INNER_X             = 1 << 24,
    /* Scrolling */
    ELI_TABLE_SCROLL_X                   = 1 << 25,
    ELI_TABLE_SCROLL_Y                   = 1 << 26,
    /* Sorting */
    ELI_TABLE_SORT_MULTI                 = 1 << 27,
    ELI_TABLE_SORT_TRISTATE              = 1 << 28,
    ELI_TABLE_HIGHLIGHT_HOVERED_COLUMN   = 1 << 29,

    /* Combined masks (elimgui helpers, not part of the public spec grid). */
    ELI_TABLE_SIZING_MASK_ = ELI_TABLE_SIZING_FIXED_FIT | ELI_TABLE_SIZING_FIXED_SAME |
                             ELI_TABLE_SIZING_STRETCH_PROP | ELI_TABLE_SIZING_STRETCH_SAME
};

/* ---------------------------------------------------------------------------
 * Table column flags
 * ------------------------------------------------------------------------- */

typedef int eli_table_column_flags;
enum eli_table_column_flags_ {
    ELI_TABLE_COLUMN_NONE                = 0,
    ELI_TABLE_COLUMN_DISABLED            = 1 << 0,
    ELI_TABLE_COLUMN_DEFAULT_HIDE        = 1 << 1,
    ELI_TABLE_COLUMN_DEFAULT_SORT        = 1 << 2,
    ELI_TABLE_COLUMN_WIDTH_STRETCH       = 1 << 3,
    ELI_TABLE_COLUMN_WIDTH_FIXED         = 1 << 4,
    ELI_TABLE_COLUMN_NO_RESIZE           = 1 << 5,
    ELI_TABLE_COLUMN_NO_REORDER          = 1 << 6,
    ELI_TABLE_COLUMN_NO_HIDE             = 1 << 7,
    ELI_TABLE_COLUMN_NO_CLIP             = 1 << 8,
    ELI_TABLE_COLUMN_NO_SORT             = 1 << 9,
    ELI_TABLE_COLUMN_NO_SORT_ASCENDING   = 1 << 10,
    ELI_TABLE_COLUMN_NO_SORT_DESCENDING  = 1 << 11,
    ELI_TABLE_COLUMN_NO_HEADER_LABEL     = 1 << 12,
    ELI_TABLE_COLUMN_NO_HEADER_WIDTH     = 1 << 13,
    ELI_TABLE_COLUMN_PREFER_SORT_ASCENDING  = 1 << 14,
    ELI_TABLE_COLUMN_PREFER_SORT_DESCENDING = 1 << 15,
    ELI_TABLE_COLUMN_INDENT_ENABLE       = 1 << 16,
    ELI_TABLE_COLUMN_INDENT_DISABLE      = 1 << 17,
    ELI_TABLE_COLUMN_ANGLED_HEADER       = 1 << 18,
    /* Output flags (read back via eli_table_get_column_flags) */
    ELI_TABLE_COLUMN_IS_ENABLED          = 1 << 24,
    ELI_TABLE_COLUMN_IS_VISIBLE          = 1 << 25,
    ELI_TABLE_COLUMN_IS_SORTED           = 1 << 26,
    ELI_TABLE_COLUMN_IS_HOVERED          = 1 << 27,

    /* Masks (elimgui helpers). */
    ELI_TABLE_COLUMN_WIDTH_MASK_ = ELI_TABLE_COLUMN_WIDTH_STRETCH | ELI_TABLE_COLUMN_WIDTH_FIXED,
    ELI_TABLE_COLUMN_INDENT_MASK_ = ELI_TABLE_COLUMN_INDENT_ENABLE | ELI_TABLE_COLUMN_INDENT_DISABLE,
    ELI_TABLE_COLUMN_STATUS_MASK_ = ELI_TABLE_COLUMN_IS_ENABLED | ELI_TABLE_COLUMN_IS_VISIBLE |
                                    ELI_TABLE_COLUMN_IS_SORTED | ELI_TABLE_COLUMN_IS_HOVERED,
    ELI_TABLE_COLUMN_NO_DIRECTION_MASK_ = ELI_TABLE_COLUMN_NO_SORT_ASCENDING |
                                          ELI_TABLE_COLUMN_NO_SORT_DESCENDING
};

/* ---------------------------------------------------------------------------
 * Table row flags + background target
 * ------------------------------------------------------------------------- */

typedef int eli_table_row_flags;
enum eli_table_row_flags_ {
    ELI_TABLE_ROW_NONE    = 0,
    ELI_TABLE_ROW_HEADERS = 1 << 0
};

typedef int eli_table_bg_target;
enum eli_table_bg_target_ {
    ELI_TABLE_BG_TARGET_NONE    = 0,
    ELI_TABLE_BG_TARGET_ROW_BG0 = 1,
    ELI_TABLE_BG_TARGET_ROW_BG1 = 2,
    ELI_TABLE_BG_TARGET_CELL_BG = 3
};

/* ---------------------------------------------------------------------------
 * Sort specifications
 * ------------------------------------------------------------------------- */

/**
 * One sorted-column entry within an eli_table_sort_specs list. Read-only from the
 * caller's perspective: the table rebuilds it whenever the sort state changes.
 */
typedef struct eli_table_column_sort_specs {
    eli_id             column_user_id;   /* user id (eli_table_setup_column), or 0 */
    int                column_index;     /* index of the column */
    int                sort_order;       /* rank of this column (0 = primary) */
    eli_sort_direction sort_direction;   /* ascending / descending */
} eli_table_column_sort_specs;

/**
 * The full sort description handed to the caller by eli_table_get_sort_specs.
 * `specs_dirty` is set when the sort changed since last read; the caller should
 * re-sort its data while it is true and then it stays observable for that frame.
 */
typedef struct eli_table_sort_specs {
    eli_table_column_sort_specs *specs;        /* array (may be NULL when empty) */
    int                          specs_count;  /* number of sorted columns */
    bool                         specs_dirty;  /* sort changed since last query */
} eli_table_sort_specs;

/* ---------------------------------------------------------------------------
 * Internal per-column state
 * ------------------------------------------------------------------------- */

/**
 * Persistent + per-frame state for one table column. Width/hidden/sort fields
 * persist across frames; the resolved geometry (min_x..work_max_x) is recomputed
 * each frame in eli_table__update_layout.
 */
typedef struct eli_table_column {
    eli_table_column_flags flags;          /* effective flags (incl. resolved WIDTH_*) */
    eli_table_column_flags flags_in;       /* flags passed to setup_column */

    float width_requested;    /* fixed content width the user requested (persisted) */
    float stretch_weight;     /* stretch weight (persisted) */
    float width_given;        /* resolved full slot width this frame (incl. padding) */
    float width_auto;         /* auto content width derived from the header/content */

    float min_x, max_x;       /* screen-space column slot edges */
    float work_min_x;         /* content start x (min_x + inner padding) */
    float work_max_x;         /* content end x (max_x - inner padding) */
    float clip_min_x, clip_max_x; /* cell clip x extents */
    float content_max_x;      /* measured content right edge this frame */

    bool  is_enabled;         /* user-visible this frame (not hidden) */
    bool  is_visible;         /* enabled AND overlapping the clip rect */
    bool  is_setup;           /* eli_table_setup_column filled this slot this frame */
    bool  init_done;          /* one-time persisted init applied (hide/width/sort) */

    eli_sort_direction sort_direction; /* current sort direction for this column */
    int   sort_order;         /* rank within the sort (-1 = not sorted) */

    int   display_order;      /* position of this column in display order */

    eli_id user_id;           /* user id from setup_column */
    bool   has_name;
    char   name[ELI_TABLE_COLUMN_NAME_MAX];
} eli_table_column;

/* ---------------------------------------------------------------------------
 * Internal table state
 * ------------------------------------------------------------------------- */

/* Forward decl: tables draw into and advance the current window's layout. */
typedef struct eli_window eli_window;

/**
 * Persistent + per-frame state for one table instance, keyed by id in the pool.
 */
typedef struct eli_table {
    eli_id          id;
    eli_table_flags flags;

    int columns_count;
    int current_column;     /* column being filled (-1 before the first cell) */
    int current_row;        /* row index (-1 before the first row) */
    int setup_column_next;  /* next slot eli_table_setup_column will fill */

    bool is_layout_locked;  /* columns resolved (no more setup_column) */
    bool is_inside_row;     /* between next_row and end-of-row */
    bool is_cell_open;      /* a cell clip rect is currently pushed */
    bool is_sortable;       /* SORTABLE and at least one sortable column */
    bool channels_split;    /* draw list is split into bg/content channels */

    eli_window *outer_window;   /* host window */
    eli_window *inner_window;   /* working window (== outer unless scrolling) */

    /* Geometry (screen space). */
    eli_rect outer_rect;        /* full table rect */
    float    last_outer_height; /* finalized table height from the previous frame */
    float    work_min_x, work_max_x; /* content x span available to columns */
    float    work_min_y;        /* content top y */
    float    cell_padding_x, cell_padding_y;
    float    columns_right_x;   /* right edge of the last enabled column */

    /* Row bookkeeping. */
    eli_table_row_flags row_flags;
    float row_pos_y1, row_pos_y2;  /* current row top / bottom (screen) */
    float row_min_height;
    float cell_top_y;              /* top of the current cell's content */

    /* Row/cell background overrides (reset each row). */
    eli_col32 row_bg_color[2];                     /* BG0 / BG1 targets */
    eli_col32 cell_bg_color[ELI_TABLE_MAX_COLUMNS];/* per-column CELL_BG target */

    /* Hover / resize interaction. */
    int   hovered_column_body;    /* column under the mouse, or -1 */
    int   resized_column;         /* column border being dragged, or -1 */
    int   left_most_enabled;
    int   right_most_enabled;

    /* Scroll-freeze request (setup_scroll_freeze). */
    int freeze_columns;
    int freeze_rows;

    /* Backup of host-window layout state restored at end_table. */
    eli_vec2 host_cursor_backup;
    eli_vec2 host_cursor_max_backup;
    float    host_indent_backup;

    eli_table_column columns[ELI_TABLE_MAX_COLUMNS];
    int              display_order[ELI_TABLE_MAX_COLUMNS]; /* column indices L->R */

    /* Sort specs handed to the caller (heap-owned `specs`). */
    eli_table_sort_specs sort_specs;
    bool                 sort_dirty;   /* sort changed; specs need rebuild */
} eli_table;

/* ---------------------------------------------------------------------------
 * Module-private pool + current-table stack (file-static, heap-backed)
 * ------------------------------------------------------------------------- */

static eli_table **g_eli_table_pool = NULL;
static int         g_eli_table_pool_count = 0;
static int         g_eli_table_pool_capacity = 0;

static eli_table  *g_eli_table_stack[ELI_TABLE_STACK_MAX];
static int         g_eli_table_stack_size = 0;

/** @return the table currently being built (top of the stack), or NULL. */
static inline eli_table *eli_table_get_current(void)
{
    return (g_eli_table_stack_size > 0) ? g_eli_table_stack[g_eli_table_stack_size - 1] : NULL;
}

/**
 * Find the pooled table for `id`, allocating and appending a fresh one if none
 * exists yet. Table state persists across frames so widths/sort/hidden survive.
 *
 * @param id  Table id (hashed from the begin_table label).
 * @return    The pooled table, or NULL on allocation failure.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline eli_table *eli_table__find_or_create(eli_id id)
{
    for (int i = 0; i < g_eli_table_pool_count; i++)
        if (g_eli_table_pool[i]->id == id)
            return g_eli_table_pool[i];

    if (g_eli_table_pool_count >= g_eli_table_pool_capacity) {
        int new_cap = (g_eli_table_pool_capacity > 0) ? g_eli_table_pool_capacity * 2 : 8;
        eli_table **grown = (eli_table **)realloc(g_eli_table_pool,
                                                  (size_t)new_cap * sizeof(*grown));
        if (grown == NULL)
            return NULL;
        g_eli_table_pool = grown;
        g_eli_table_pool_capacity = new_cap;
    }

    eli_table *table = (eli_table *)calloc(1, sizeof(*table));
    if (table == NULL)
        return NULL;
    table->id = id;
    g_eli_table_pool[g_eli_table_pool_count++] = table;
    return table;
}

/* ---------------------------------------------------------------------------
 * Per-frame / shutdown hooks
 * ------------------------------------------------------------------------- */

/**
 * Reset transient per-frame table state at the top of a UI frame. Currently only
 * clears the current-table stack defensively (a well-formed frame ends all tables
 * it begins). Safe to call every frame; optional.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_table_new_frame(void)
{
    g_eli_table_stack_size = 0;
}

/**
 * Close out table bookkeeping at the end of a UI frame. No-op today; provided as a
 * symmetric hook alongside eli_table_new_frame.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_table_end_frame(void)
{
    /* Nothing to flush yet; table state persists in the pool across frames. */
}

/**
 * Release the entire table pool and all per-table heap state (sort-spec arrays).
 * Call once before eli_destroy_context; after this the module is back to its
 * initial empty state and may be reused.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_table_shutdown(void)
{
    for (int i = 0; i < g_eli_table_pool_count; i++) {
        if (g_eli_table_pool[i] != NULL) {
            free(g_eli_table_pool[i]->sort_specs.specs);
            free(g_eli_table_pool[i]);
        }
    }
    free(g_eli_table_pool);
    g_eli_table_pool = NULL;
    g_eli_table_pool_count = 0;
    g_eli_table_pool_capacity = 0;
    g_eli_table_stack_size = 0;
}

/* ---------------------------------------------------------------------------
 * Small shared helpers
 * ------------------------------------------------------------------------- */

/** Copy a NUL-terminated label into a fixed column-name buffer (always NUL-terminated). */
static inline void eli_table__copy_name(char *dst, const char *src)
{
    int i = 0;
    if (src != NULL)
        for (; src[i] != '\0' && i < ELI_TABLE_COLUMN_NAME_MAX - 1; i++)
            dst[i] = src[i];
    dst[i] = '\0';
}

#endif /* ELI_WIDGETS_ELI_TABLE_TYPES_H */
