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
 * @status Cheatsheet app engine (stage 1). Not part of the elimgui library API.
 * @issues None
 * @todo None
 */
#ifndef CHEAT_ENGINE_H
#define CHEAT_ENGINE_H

#include <eli/elimgui.h>

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
 * Engine tunables
 * ------------------------------------------------------------------------- */

#define CHEAT_LEFT_PANE_WIDTH   240.0f   /* fixed width of the category/search column */
#define CHEAT_SEARCH_CAP        128      /* search box buffer capacity (bytes) */
#define CHEAT_SNIPPET_CAP       2048     /* per-card snippet display buffer (bytes) */
#define CHEAT_MAX_CATEGORIES    64       /* distinct categories the left pane lists */

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
 * Left pane: search box, clear button, category selectors
 * ------------------------------------------------------------------------- */

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
    if (!eli_begin_child("cheat_left", size, ELI_CHILD_BORDERS, ELI_WINDOW_NONE)) {
        eli_end_child();
        return;
    }

    eli_text("Search");
    eli_set_next_item_width(-1.0f);
    eli_input_text("##search", search, CHEAT_SEARCH_CAP, ELI_INPUT_TEXT_NONE, NULL, NULL);
    if (eli_button("Clear"))
        search[0] = '\0';

    eli_separator();
    eli_text_disabled("Categories");

    int all_n = cheat_count_matches(entries, count, NULL, search);
    char label[160];
    snprintf(label, sizeof(label), "All (%d)", all_n);
    if (eli_selectable(label, *selected_cat == NULL, ELI_SELECTABLE_NONE, eli_make_vec2(0, 0)))
        *selected_cat = NULL;

    for (int i = 0; i < cats->count; i++) {
        const char *cat = cats->names[i];
        int n = cheat_count_matches(entries, count, cat, search);
        snprintf(label, sizeof(label), "%s (%d)", cat, n);
        bool selected = (*selected_cat != NULL && strcmp(*selected_cat, cat) == 0);
        eli_push_id_int(i);
        if (eli_selectable(label, selected, ELI_SELECTABLE_NONE, eli_make_vec2(0, 0)))
            *selected_cat = cat;
        eli_pop_id();
    }

    eli_end_child();
}

/* ---------------------------------------------------------------------------
 * Main pane: one card per matching entry
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

/**
 * Render one entry as a framed card: title, API signature, description, the live
 * widget, and a read-only snippet box with a Copy button.
 *
 * @param e     Entry to render.
 * @param index Stable index used to scope widget ids across cards.
 */
static inline void cheat_render_card(const cheat_entry *e, int index)
{
    eli_push_id_int(index);
    eli_begin_group();

    /* Title — accent color for emphasis. */
    eli_vec4 accent = eli_get_style_color_vec4(ELI_COL_HEADER);
    accent.x = accent.x * 0.4f + 0.6f;
    accent.y = accent.y * 0.4f + 0.7f;
    accent.z = accent.z * 0.2f + 0.8f;
    accent.w = 1.0f;
    eli_text_colored(accent, "%s", e->title ? e->title : "(untitled)");

    /* API signature in the disabled/dim color. */
    if (e->api)
        eli_text_disabled("%s", e->api);

    /* Description, wrapped to the pane width. */
    if (e->description)
        eli_text_wrapped("%s", e->description);

    eli_spacing();

    /* Live widget — grouped so it lays out as one block. */
    eli_begin_group();
    if (e->render)
        e->render();
    eli_end_group();

    eli_spacing();

    /* Snippet in a read-only monospace-style code box. */
    if (e->snippet) {
        char buf[CHEAT_SNIPPET_CAP];
        size_t n = strlen(e->snippet);
        if (n >= sizeof(buf))
            n = sizeof(buf) - 1;
        memcpy(buf, e->snippet, n);
        buf[n] = '\0';

        eli_vec2 box = eli_make_vec2(-1.0f, cheat_snippet_height(buf));
        eli_input_text_multiline("##snippet", buf, sizeof(buf), box,
                                 ELI_INPUT_TEXT_READ_ONLY, NULL, NULL);
        if (eli_button("Copy"))
            eli_set_clipboard_text(e->snippet);
    }

    eli_end_group();
    eli_pop_id();
}

/**
 * Render the scrollable main pane, one card per matching entry, with separators
 * between cards.
 *
 * @param entries Full entry array.
 * @param count   Number of entries.
 * @param f       Active filter (category + search).
 */
static inline void cheat_render_main_pane(const cheat_entry *entries, int count,
                                          const cheat_filter *f)
{
    eli_same_line(0.0f, -1.0f);
    if (!eli_begin_child("cheat_main", eli_make_vec2(0.0f, 0.0f), ELI_CHILD_BORDERS,
                         ELI_WINDOW_NONE)) {
        eli_end_child();
        return;
    }

    int shown = 0;
    for (int i = 0; i < count; i++) {
        if (!cheat_entry_matches(&entries[i], f))
            continue;
        if (shown > 0)
            eli_separator();
        cheat_render_card(&entries[i], i);
        shown++;
    }

    if (shown == 0)
        eli_text_disabled("No widgets match your search.");

    eli_end_child();
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

    cheat_render_left_pane(entries, count, &cats, s_search, &s_selected_cat);

    cheat_filter f = {s_search, s_selected_cat};
    cheat_render_main_pane(entries, count, &f);
}

#endif /* CHEAT_ENGINE_H */
