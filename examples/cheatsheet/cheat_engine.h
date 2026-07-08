/**
 * @file cheat_engine.h
 * @brief Reusable "visual cheatsheet" engine for elimgui: renders a searchable,
 *        two-pane browser of widget entries where each entry shows a live widget
 *        alongside the exact eli_*() call and copyable source snippet.
 *
 * The engine is content-agnostic: it renders from a caller-supplied array of
 * `cheat_entry` records (see cheat_entries.h for the aggregated app list). Every
 * helper is `static` so multiple translation units can include this header
 * without symbol collisions; per-frame widget state lives in function-static
 * variables, matching elimgui's immediate-mode model.
 *
 * Presentation is driven entirely by the design tokens in cheat_theme.h (spacing
 * scale, dark-mode role palette, rounding), so the look stays consistent with
 * the global eli_style that cheat_theme_apply() programs. Hierarchy is built
 * from color + spacing + dividers (there is only one 13px font, so we never lean
 * on font size) per typography and layout best practices.
 *
 * @status Cheatsheet app engine (stage 2, UI/UX pass). Not part of the elimgui
 *         library API.
 * @issues None
 * @todo None
 */
#ifndef CHEAT_ENGINE_H
#define CHEAT_ENGINE_H

#include <eli/elimgui.h>

#include "cheat_theme.h"

/* ---------------------------------------------------------------------------
 * Entry model
 *
 * One `cheat_entry` describes a single widget showcase. Content agents populate
 * arrays of these (see cheat_entries.h). `render` draws the LIVE widget and owns
 * its own function-static state so it can be called every frame.
 * ------------------------------------------------------------------------- */

/** A single cheatsheet card: metadata plus a live-widget renderer. */
typedef struct cheat_entry {
    const char *category;    /* group name, e.g. "Sliders & Drags" */
    const char *title;       /* human name, e.g. "Slider (float)" */
    const char *api;         /* signature, e.g. "eli_slider_float(label, &v, ...)" */
    const char *description; /* one or two lines of prose */
    const char *snippet;     /* exact source the user would write (multi-line ok) */
    void (*render)(void);    /* draws the live widget; owns its function-static state */
} cheat_entry;

/* ---------------------------------------------------------------------------
 * Engine tunables (all lengths are scale tokens from cheat_theme.h)
 * ------------------------------------------------------------------------- */

#define CHEAT_LEFT_PANE_WIDTH   260.0f          /* sidebar/nav column width (240-320 band) */
#define CHEAT_SEARCH_CAP        128             /* search box buffer capacity (bytes) */
#define CHEAT_SNIPPET_CAP       2048            /* per-card snippet display buffer (bytes) */
#define CHEAT_MAX_CATEGORIES    64              /* distinct categories the left pane lists */
#define CHEAT_MAX_ENTRY_SLOTS   512             /* per-entry layout-cache slots (card heights) */

#define CHEAT_CARD_ROUNDING     CHEAT_ROUND_CARD  /* card / code-box corner radius (8px) */
#define CHEAT_CARD_PADDING      CHEAT_SPACE_16     /* inner padding inside each card (16px) */
#define CHEAT_CARD_SPACING      CHEAT_SPACE_24     /* gap between cards (24px; >= card padding) */
#define CHEAT_PANE_PADDING      CHEAT_SPACE_12     /* inner padding of each pane (12px) */
#define CHEAT_PANE_GUTTER       CHEAT_SPACE_16     /* gutter between the two panes (16px) */
#define CHEAT_CAT_ROW_HEIGHT    28.0f              /* category row height (comfortable-dense, >=24 hit) */
#define CHEAT_PREVIEW_PADDING   CHEAT_SPACE_8      /* inset padding around a live preview (8px) */

/* ---------------------------------------------------------------------------
 * Small helpers
 * ------------------------------------------------------------------------- */

/** Snap a (non-negative screen) coordinate to a whole pixel to avoid shimmer. */
static inline float cheat_snap(float v)
{
    return (float)(long)(v + 0.5f);
}

/* ---------------------------------------------------------------------------
 * Text helpers (freestanding, no libc <ctype.h>/<strings.h> dependency)
 * ------------------------------------------------------------------------- */

/** Lowercase a single ASCII byte. */
static inline char cheat_lower(char c)
{
    return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
}

/**
 * Case-insensitive substring test.
 *
 * @param haystack NUL-terminated text to search (NULL treated as empty).
 * @param needle   NUL-terminated pattern (empty needle matches everything).
 * @return         true when `needle` occurs in `haystack` ignoring ASCII case.
 */
static inline bool cheat_ci_contains(const char *haystack, const char *needle)
{
    if (needle == NULL || needle[0] == '\0')
        return true;
    if (haystack == NULL)
        return false;

    for (const char *h = haystack; *h; h++) {
        const char *hp = h;
        const char *np = needle;
        while (*np && cheat_lower(*hp) == cheat_lower(*np)) {
            hp++;
            np++;
        }
        if (*np == '\0')
            return true;
    }
    return false;
}

/* ---------------------------------------------------------------------------
 * Filter state and matching
 * ------------------------------------------------------------------------- */

/** Live filter: search text plus the selected category (NULL = "All"). */
typedef struct cheat_filter {
    const char *search;             /* current search box contents */
    const char *category;           /* selected category, or NULL for all */
} cheat_filter;

/** True when `e` matches the search term across its text fields. */
static inline bool cheat_entry_matches_search(const cheat_entry *e, const char *search)
{
    if (search == NULL || search[0] == '\0')
        return true;
    return cheat_ci_contains(e->title, search) ||
           cheat_ci_contains(e->api, search) ||
           cheat_ci_contains(e->category, search) ||
           cheat_ci_contains(e->description, search) ||
           cheat_ci_contains(e->snippet, search);
}

/** True when `e` passes both the category selection and the search term. */
static inline bool cheat_entry_matches(const cheat_entry *e, const cheat_filter *f)
{
    if (f->category != NULL && strcmp(e->category, f->category) != 0)
        return false;
    return cheat_entry_matches_search(e, f->search);
}

/* ---------------------------------------------------------------------------
 * Category collection
 * ------------------------------------------------------------------------- */

/** Distinct category names discovered in an entry array (order of appearance). */
typedef struct cheat_categories {
    const char *names[CHEAT_MAX_CATEGORIES];
    int count;
} cheat_categories;

/** Populate `out` with the distinct, first-seen category names from `entries`. */
static inline void cheat_collect_categories(const cheat_entry *entries, int count,
                                            cheat_categories *out)
{
    out->count = 0;
    for (int i = 0; i < count && out->count < CHEAT_MAX_CATEGORIES; i++) {
        const char *cat = entries[i].category;
        if (cat == NULL)
            continue;
        bool seen = false;
        for (int j = 0; j < out->count; j++) {
            if (strcmp(out->names[j], cat) == 0) {
                seen = true;
                break;
            }
        }
        if (!seen)
            out->names[out->count++] = cat;
    }
}

/** Count entries in one category (or all categories) that match the search term. */
static inline int cheat_count_matches(const cheat_entry *entries, int count,
                                      const char *category, const char *search)
{
    int n = 0;
    for (int i = 0; i < count; i++) {
        if (category != NULL && strcmp(entries[i].category, category) != 0)
            continue;
        if (cheat_entry_matches_search(&entries[i], search))
            n++;
    }
    return n;
}

/* ---------------------------------------------------------------------------
 * Shared building blocks
 * ------------------------------------------------------------------------- */

/**
 * A small all-caps group label in the lowest (38%) text tier. All-caps section
 * headers are a typographic hierarchy lever that costs no font size
 * (typography.md); the low tier keeps them quiet above their content.
 */
static inline void cheat_group_label(const char *text)
{
    eli_text_colored(cheat_text_lo_vec4(), "%s", text);
}

/* ---------------------------------------------------------------------------
 * Sidebar: search field + category navigation
 *
 * Nav best-practices (menus-and-navigation.md, data-display.md): a labelled
 * search with a visible Clear affordance, a "CATEGORIES" group label, rows with
 * a clear selected state (accent state layer), a neutral hover, comfortable
 * density, and right-aligned counts so magnitudes line up.
 * ------------------------------------------------------------------------- */

/** One category row: full-width selectable name + right-aligned match count. */
static inline bool cheat_category_row(const char *name, int match_count, bool selected)
{
    eli_draw_list *dl = eli_get_window_draw_list();
    bool clicked = eli_selectable(name, selected, ELI_SELECTABLE_NONE,
                                  eli_make_vec2(0.0f, CHEAT_CAT_ROW_HEIGHT));

    eli_vec2 rmin = eli_get_item_rect_min();
    eli_vec2 rmax = eli_get_item_rect_max();

    char cnt[16];
    snprintf(cnt, sizeof(cnt), "%d", match_count);
    eli_vec2 ts = eli_calc_text_size(cnt, NULL);
    eli_col32 col = selected ? cheat_accent_u32() : cheat_text_med_u32();
    eli_vec2 tp = eli_make_vec2(cheat_snap(rmax.x - ts.x - CHEAT_SPACE_8),
                                cheat_snap(rmin.y + (rmax.y - rmin.y - ts.y) * 0.5f));
    eli_draw_list_add_text(dl, tp, col, cnt, NULL);
    return clicked;
}

/**
 * Render the left column. Reads/writes the persistent `search` buffer and the
 * selected-category pointer through the supplied pointers so the caller's static
 * state survives across frames.
 *
 * @param entries      Full entry array (for per-category match counts).
 * @param count        Number of entries.
 * @param cats         Precomputed distinct categories.
 * @param search       Mutable search buffer (edited in place by the input box).
 * @param selected_cat In/out selected category pointer (NULL = "All").
 */
static inline void cheat_render_left_pane(const cheat_entry *entries, int count,
                                          const cheat_categories *cats, char *search,
                                          const char **selected_cat)
{
    eli_vec2 size = eli_make_vec2(CHEAT_LEFT_PANE_WIDTH, 0.0f);

    /* The sidebar is a raised neutral surface (background shift = containment),
     * with its own comfortable padding and no border. */
    eli_push_style_color_vec4(ELI_COL_CHILD_BG, cheat_surface_sidebar_vec4());
    eli_push_style_var_vec2(ELI_STYLE_VAR_WINDOW_PADDING,
                            eli_make_vec2(CHEAT_PANE_PADDING, CHEAT_PANE_PADDING));
    bool open = eli_begin_child("cheat_left", size, ELI_CHILD_NONE, ELI_WINDOW_NONE);
    eli_pop_style_var(1);
    eli_pop_style_color(1);
    if (!open) {
        eli_end_child();
        return;
    }

    /* Search group: label, full-width field with a hint + Clear affordance. */
    cheat_group_label("SEARCH");
    eli_set_next_item_width(-1.0f);
    eli_input_text_with_hint("##search", "Filter widgets\xe2\x80\xa6", search, CHEAT_SEARCH_CAP,
                             ELI_INPUT_TEXT_NONE, NULL, NULL);
    if (eli_button("Clear"))
        search[0] = '\0';

    /* Category group: a full-width divider header, then the nav rows. */
    eli_dummy(eli_make_vec2(0.0f, CHEAT_SPACE_8));
    cheat_group_label("CATEGORIES");
    eli_dummy(eli_make_vec2(0.0f, CHEAT_SPACE_2));

    /* Tighten the row rhythm so nav reads as one dense group. */
    eli_push_style_var_vec2(ELI_STYLE_VAR_ITEM_SPACING, eli_make_vec2(CHEAT_SPACE_8, CHEAT_SPACE_2));

    int all_n = cheat_count_matches(entries, count, NULL, search);
    if (cheat_category_row("All widgets", all_n, *selected_cat == NULL))
        *selected_cat = NULL;

    for (int i = 0; i < cats->count; i++) {
        const char *cat = cats->names[i];
        int n = cheat_count_matches(entries, count, cat, search);
        bool selected = (*selected_cat != NULL && strcmp(*selected_cat, cat) == 0);
        eli_push_id_int(i);
        if (cheat_category_row(cat, n, selected))
            *selected_cat = cat;
        eli_pop_id();
    }

    eli_pop_style_var(1);
    eli_end_child();
}

/* ---------------------------------------------------------------------------
 * Cards: one per matching entry (containment + data-display)
 * ------------------------------------------------------------------------- */

/** Estimate the pixel height needed to show `text` as `snippet` lines. */
static inline float cheat_snippet_height(const char *text)
{
    int lines = 1;
    for (const char *p = text; *p; p++) {
        if (*p == '\n')
            lines++;
    }
    float line_h = eli_get_text_line_height_with_spacing();
    return (float)lines * line_h + eli_get_text_line_height();
}

/** Draw the live widget inside a raised, rounded inset panel of width `w`. */
static inline void cheat_render_preview(const cheat_entry *e, int index, float w)
{
    static float s_prev_h[CHEAT_MAX_ENTRY_SLOTS];
    const eli_vec2 pad = eli_make_vec2(CHEAT_PREVIEW_PADDING, CHEAT_PREVIEW_PADDING);
    bool track = (index >= 0 && index < CHEAT_MAX_ENTRY_SLOTS);

    eli_draw_list *dl = eli_get_window_draw_list();
    eli_vec2 q0 = eli_get_cursor_screen_pos();
    q0 = eli_make_vec2(cheat_snap(q0.x), cheat_snap(q0.y));
    float body_h = (track && s_prev_h[index] > 0.0f) ? s_prev_h[index]
                                                     : eli_get_frame_height();
    eli_vec2 q1 = eli_make_vec2(cheat_snap(q0.x + w), cheat_snap(q0.y + body_h + pad.y * 2.0f));

    /* Raised inset via a lighter surface (elevation = lightness); no border
     * needed — the background shift alone signifies the live area. */
    eli_draw_list_add_rect_filled(dl, q0, q1, cheat_surface_inset_u32(), CHEAT_ROUND_INSET,
                                  ELI_DRAW_ROUND_CORNERS_ALL);

    eli_set_cursor_screen_pos(eli_make_vec2(q0.x + pad.x, q0.y + pad.y));
    eli_push_item_width(w - pad.x * 2.0f);
    eli_begin_group();
    if (e->render)
        e->render();
    eli_end_group();
    eli_pop_item_width();

    if (track)
        s_prev_h[index] = eli_get_item_rect_max().y - (q0.y + pad.y);
    eli_set_cursor_screen_pos(eli_make_vec2(q0.x, q1.y));
}

/** Render the card interior (title, api, description, preview, snippet) at width `w`. */
static inline void cheat_render_card_body(const cheat_entry *e, int index, float w)
{
    /* Reads top-down: title (accent, primary) -> API (secondary) -> description
     * (muted) -> live preview -> code snippet. Hierarchy from color, not size. */
    eli_text_colored(cheat_accent_vec4(), "%s", e->title ? e->title : "(untitled)");

    if (e->api)
        eli_text_colored(cheat_text_med_vec4(), "%s", e->api);

    if (e->description) {
        eli_dummy(eli_make_vec2(0.0f, CHEAT_SPACE_2));
        eli_push_style_color_vec4(ELI_COL_TEXT, cheat_text_med_vec4());
        eli_text_wrapped("%s", e->description);
        eli_pop_style_color(1);
    }

    eli_dummy(eli_make_vec2(0.0f, CHEAT_SPACE_4));
    cheat_render_preview(e, index, w);

    if (e->snippet) {
        eli_dummy(eli_make_vec2(0.0f, CHEAT_SPACE_8));
        cheat_group_label("SNIPPET");
        eli_dummy(eli_make_vec2(0.0f, CHEAT_SPACE_2));

        char buf[CHEAT_SNIPPET_CAP];
        size_t n = strlen(e->snippet);
        if (n >= sizeof(buf))
            n = sizeof(buf) - 1;
        memcpy(buf, e->snippet, n);
        buf[n] = '\0';

        /* Distinct sunken code surface (near-black), read-only. */
        eli_push_style_color_vec4(ELI_COL_FRAME_BG, cheat_code_vec4());
        eli_vec2 box = eli_make_vec2(w, cheat_snippet_height(buf));
        eli_input_text_multiline("##snippet", buf, sizeof(buf), box,
                                 ELI_INPUT_TEXT_READ_ONLY, NULL, NULL);
        eli_pop_style_color(1);

        if (eli_button("Copy"))
            eli_set_clipboard_text(e->snippet);
    }
}

/**
 * Render one entry as a rounded, filled card: a raised panel background sits
 * behind the padded content (title, API signature, description, a framed
 * live-widget preview, and a code-styled snippet box with a Copy button).
 *
 * The card height is measured each frame and reused on the next so the
 * background rectangle — which must be drawn before the content to sit behind it
 * — can be sized without a second layout pass. This avoids splitting the draw
 * list, which some live widgets (tables, tab bars) already do internally.
 *
 * @param e     Entry to render.
 * @param index Stable index used to scope widget ids and cache the card height.
 */
static inline void cheat_render_card(const cheat_entry *e, int index)
{
    static float s_card_h[CHEAT_MAX_ENTRY_SLOTS];
    bool track = (index >= 0 && index < CHEAT_MAX_ENTRY_SLOTS);

    eli_push_id_int(index);

    eli_draw_list *dl = eli_get_window_draw_list();
    eli_vec2 p0 = eli_get_cursor_screen_pos();
    p0 = eli_make_vec2(cheat_snap(p0.x), cheat_snap(p0.y));
    float avail_w = eli_get_content_region_avail().x;
    float est = eli_get_text_line_height_with_spacing() * 9.0f;
    float card_h = (track && s_card_h[index] > 0.0f) ? s_card_h[index] : est;
    eli_vec2 p1 = eli_make_vec2(cheat_snap(p0.x + avail_w), cheat_snap(p0.y + card_h));

    /* Raised card surface + a faint hairline for figure-ground crispness. */
    eli_draw_list_add_rect_filled(dl, p0, p1, cheat_surface_card_u32(), CHEAT_CARD_ROUNDING,
                                  ELI_DRAW_ROUND_CORNERS_ALL);
    eli_draw_list_add_rect(dl, p0, p1, cheat_border_faint_u32(), CHEAT_CARD_ROUNDING,
                           ELI_DRAW_ROUND_CORNERS_ALL, CHEAT_BORDER_SIZE);

    /* Inset the content by the padding and wrap text to the inner width. */
    float pad = CHEAT_CARD_PADDING;
    float content_w = avail_w - pad * 2.0f;
    eli_set_cursor_screen_pos(eli_make_vec2(p0.x + pad, p0.y + pad));
    eli_push_text_wrap_pos(eli_get_cursor_pos_x() + content_w);
    eli_begin_group();
    cheat_render_card_body(e, index, content_w);
    eli_end_group();
    eli_pop_text_wrap_pos();

    /* Record the measured height and drop the cursor below the card. */
    float bottom = eli_get_item_rect_max().y + pad;
    if (track)
        s_card_h[index] = bottom - p0.y;
    eli_set_cursor_screen_pos(eli_make_vec2(p0.x, bottom + CHEAT_CARD_SPACING));

    eli_pop_id();
}

/** Empty state: name the miss and point at the recovery (feedback-and-states.md). */
static inline void cheat_render_empty_state(const cheat_filter *f)
{
    eli_dummy(eli_make_vec2(0.0f, CHEAT_SPACE_16));
    eli_text_colored(cheat_text_hi_vec4(), "No widgets match your filter.");
    if (f->search != NULL && f->search[0] != '\0')
        eli_text_colored(cheat_text_med_vec4(), "No results for \"%s\".", f->search);
    eli_text_colored(cheat_text_med_vec4(),
                     "Try a different term, or press Clear in the sidebar.");
}

/**
 * Render the scrollable main pane, one card per matching entry.
 *
 * @param entries Full entry array.
 * @param count   Number of entries.
 * @param f       Active filter (category + search).
 */
static inline void cheat_render_main_pane(const cheat_entry *entries, int count,
                                          const cheat_filter *f)
{
    eli_same_line(0.0f, CHEAT_PANE_GUTTER);

    /* Content pane is transparent (reads as the base window field); cards float
     * on it. No border — space + surface shift do the separating. */
    eli_push_style_var_vec2(ELI_STYLE_VAR_WINDOW_PADDING,
                            eli_make_vec2(CHEAT_PANE_PADDING, CHEAT_PANE_PADDING));
    bool open = eli_begin_child("cheat_main", eli_make_vec2(0.0f, 0.0f), ELI_CHILD_NONE,
                                ELI_WINDOW_NONE);
    eli_pop_style_var(1);
    if (!open) {
        eli_end_child();
        return;
    }

    int shown = 0;
    for (int i = 0; i < count; i++) {
        if (!cheat_entry_matches(&entries[i], f))
            continue;
        cheat_render_card(&entries[i], i);
        shown++;
    }

    if (shown == 0)
        cheat_render_empty_state(f);

    eli_end_child();
}

/* ---------------------------------------------------------------------------
 * Header
 * ------------------------------------------------------------------------- */

/**
 * Render the accent title, a right-aligned live count, a one-line subtitle, and
 * a divider above the two panes. Strong hierarchy from color + spacing.
 *
 * @param total Total number of registered entries.
 * @param shown Number of entries matching the active filter.
 */
static inline void cheat_render_header(int total, int shown)
{
    eli_draw_list *dl = eli_get_window_draw_list();
    eli_vec2 hp = eli_get_cursor_screen_pos();
    float avail = eli_get_content_region_avail().x;

    /* Title (primary, accent) on the left; live "N of M" count right-aligned on
     * the same baseline in the secondary tier. */
    eli_text_colored(cheat_accent_vec4(), "elimgui Visual Cheatsheet");

    char cnt[64];
    snprintf(cnt, sizeof(cnt), "%d of %d widgets", shown, total);
    eli_vec2 ts = eli_calc_text_size(cnt, NULL);
    eli_vec2 tp = eli_make_vec2(cheat_snap(hp.x + avail - ts.x), cheat_snap(hp.y));
    eli_draw_list_add_text(dl, tp, cheat_text_med_u32(), cnt, NULL);

    /* Subtitle (secondary), then a full-width hairline divider. */
    eli_text_colored(cheat_text_med_vec4(),
                     "Live widgets paired with the exact eli_*() call and a copyable snippet.");
    eli_dummy(eli_make_vec2(0.0f, CHEAT_SPACE_4));
    eli_separator();
    eli_dummy(eli_make_vec2(0.0f, CHEAT_SPACE_8));
}

/* ---------------------------------------------------------------------------
 * Public entry point
 * ------------------------------------------------------------------------- */

/**
 * Render the whole cheatsheet UI inside the current elimgui window/frame. Owns
 * its own persistent search/selection state via function-static storage, so the
 * caller only needs to pass the (stable) entry registry each frame.
 *
 * @param entries Aggregated entry array (see cheat_entries.h).
 * @param count   Number of entries in `entries`.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void cheat_show(const cheat_entry *entries, int count)
{
    static char s_search[CHEAT_SEARCH_CAP] = {0};
    static const char *s_selected_cat = NULL; /* NULL = "All" */

    if (entries == NULL || count <= 0) {
        eli_text_disabled("No cheatsheet entries registered.");
        return;
    }

    cheat_categories cats;
    cheat_collect_categories(entries, count, &cats);

    cheat_filter f = {s_search, s_selected_cat};
    int shown = 0;
    for (int i = 0; i < count; i++)
        if (cheat_entry_matches(&entries[i], &f))
            shown++;

    cheat_render_header(count, shown);
    cheat_render_left_pane(entries, count, &cats, s_search, &s_selected_cat);
    cheat_render_main_pane(entries, count, &f);
}

#endif /* CHEAT_ENGINE_H */
