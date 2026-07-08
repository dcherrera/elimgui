/**
 * @file render_combo_trees.h
 * @brief Live widget render functions for the "Combo & Lists" and "Trees"
 *        cheatsheet entries. Each function owns its own function-static state
 *        so it can be called once per frame in immediate-mode style.
 *
 * Include this file before rows_combo_trees.h so the render function
 * identifiers are in scope when the entry initializers are compiled.
 *
 * @status Cheatsheet content (Combo & Lists, Trees). Not part of the library.
 * @issues None
 * @todo None
 */
#ifndef CHEAT_RENDER_COMBO_TREES_H
#define CHEAT_RENDER_COMBO_TREES_H

#include <eli/elimgui.h>

/* ---------------------------------------------------------------------------
 * Combo & Lists
 * ------------------------------------------------------------------------- */

/** Selectable — single-select: one row highlighted at a time via an index. */
static void cheat_render_selectable(void)
{
    static int sel = 0;
    const char *items[] = {"Apple", "Banana", "Cherry"};
    for (int i = 0; i < 3; i++) {
        eli_push_id_int(i);
        if (eli_selectable(items[i], sel == i,
                           ELI_SELECTABLE_NONE, eli_make_vec2(0.0f, 0.0f)))
            sel = i;
        eli_pop_id();
    }
}

/** Selectable — bool toggle: each row independently toggles its own flag. */
static void cheat_render_selectable_bool(void)
{
    static bool checked[3] = {true, false, true};
    const char *labels[] = {"Option A", "Option B", "Option C"};
    for (int i = 0; i < 3; i++) {
        eli_push_id_int(i);
        eli_selectable_bool(labels[i], &checked[i],
                            ELI_SELECTABLE_NONE, eli_make_vec2(0.0f, 0.0f));
        eli_pop_id();
    }
}

/** Combo (array) — convenience form over a const char* array. */
static void cheat_render_combo(void)
{
    static const char *fruits[] = {"Apple", "Banana", "Cherry", "Date"};
    static int cur = 0;
    eli_combo("Fruit##combo", &cur, fruits, 4, -1);
}

/** Combo (begin/end) — caller fills the popup with custom selectables. */
static void cheat_render_begin_combo(void)
{
    static const char *options[] = {"Red", "Green", "Blue"};
    static int cur = 0;
    const char *preview = options[cur];
    if (eli_begin_combo("Color##bc", preview, ELI_COMBO_NONE)) {
        for (int i = 0; i < 3; i++) {
            eli_push_id_int(i);
            bool sel = (i == cur);
            if (eli_selectable(options[i], sel,
                               ELI_SELECTABLE_NONE, eli_make_vec2(0.0f, 0.0f)))
                cur = i;
            eli_pop_id();
        }
        eli_end_combo();
    }
}

/** List Box — always-visible framed list; selection changes via click. */
static void cheat_render_list_box(void)
{
    static const char *planets[] = {"Mercury", "Venus", "Earth", "Mars"};
    static int cur = 2;
    eli_list_box("Planet##lb", &cur, planets, 4, -1);
}

/* ---------------------------------------------------------------------------
 * Trees
 * ------------------------------------------------------------------------- */

/** Tree Node — nested collapsible hierarchy; close scope with eli_tree_pop. */
static void cheat_render_tree_node_children(void)
{
    if (eli_tree_node("Parent")) {
        eli_text("Child item 1");
        eli_text("Child item 2");
        if (eli_tree_node("Nested")) {
            eli_text("Leaf");
            eli_tree_pop();
        }
        eli_tree_pop();
    }
}

/** Tree Node Ex — DEFAULT_OPEN starts expanded; LEAF has no child scope. */
static void cheat_render_tree_node_ex(void)
{
    if (eli_tree_node_ex("Default Open##tnex", ELI_TREE_NODE_DEFAULT_OPEN)) {
        eli_text("Visible by default");
        eli_tree_pop();
    }
    /* LEAF: draws a bullet, never opens; no eli_tree_pop needed. */
    eli_tree_node_ex("Leaf Node##tnleaf", ELI_TREE_NODE_LEAF);
}

/** Collapsing Header — framed section; no indentation, no tree_pop. */
static void cheat_render_collapsing_header(void)
{
    if (eli_collapsing_header("Section A", ELI_TREE_NODE_NONE)) {
        eli_text("Content inside section A");
        eli_text("Another line of content");
    }
}

/** Collapsing Header (closable) — X button hides the section; button reopens. */
static void cheat_render_collapsing_header_bool(void)
{
    static bool visible = true;
    if (visible) {
        if (eli_collapsing_header_bool("Closable Section##chbool", &visible,
                                       ELI_TREE_NODE_NONE))
            eli_text("Click the X button to close this header.");
    }
    if (!visible) {
        if (eli_button("Reopen##chbool"))
            visible = true;
    }
}

#endif /* CHEAT_RENDER_COMBO_TREES_H */
