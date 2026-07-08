/* rows_combo_trees.h — NO include guard; included as an initializer fragment.
 *
 * Field order: { category, title, api, description, snippet, render_fn }
 * Every entry is comma-terminated so the enclosing array needs no trailing
 * comma adjustment. Render functions are defined in render_combo_trees.h,
 * which must be included before this file.
 */

/* ---- Combo & Lists ---------------------------------------------------- */

{
    "Combo & Lists",
    "Selectable (single-select)",
    "bool eli_selectable(label, selected, flags, size)",
    "A full-width clickable row. Pass the current selected state and return "
    "true when the row is pressed. Use a shared index to enforce "
    "single-selection across sibling rows.",
    "static int sel = 0;\n"
    "const char *items[] = {\"Apple\", \"Banana\", \"Cherry\"};\n"
    "for (int i = 0; i < 3; i++) {\n"
    "    eli_push_id_int(i);\n"
    "    if (eli_selectable(items[i], sel == i,\n"
    "                       ELI_SELECTABLE_NONE,\n"
    "                       eli_make_vec2(0.0f, 0.0f)))\n"
    "        sel = i;\n"
    "    eli_pop_id();\n"
    "}",
    cheat_render_selectable
},

{
    "Combo & Lists",
    "Selectable (bool toggle)",
    "bool eli_selectable_bool(label, &selected, flags, size)",
    "Convenience variant: pressing the row toggles the bool pointed to by "
    "p_selected. Well-suited for multi-select lists where each row has its "
    "own independent flag.",
    "static bool checked[3] = {true, false, true};\n"
    "const char *labels[] = {\"Option A\", \"Option B\", \"Option C\"};\n"
    "for (int i = 0; i < 3; i++) {\n"
    "    eli_push_id_int(i);\n"
    "    eli_selectable_bool(labels[i], &checked[i],\n"
    "                        ELI_SELECTABLE_NONE,\n"
    "                        eli_make_vec2(0.0f, 0.0f));\n"
    "    eli_pop_id();\n"
    "}",
    cheat_render_selectable_bool
},

{
    "Combo & Lists",
    "Combo (array)",
    "bool eli_combo(label, &current, items[], count, max_height)",
    "One-call combo over a const char* array. Displays the current item as "
    "the preview; clicking opens the dropdown. Returns true on the frame the "
    "selection changes. Pass -1 for max_height to use the default (8 rows).",
    "static const char *fruits[] =\n"
    "    {\"Apple\", \"Banana\", \"Cherry\", \"Date\"};\n"
    "static int cur = 0;\n"
    "eli_combo(\"Fruit\", &cur, fruits, 4, -1);",
    cheat_render_combo
},

{
    "Combo & Lists",
    "Combo (begin/end custom)",
    "bool eli_begin_combo(label, preview, flags) / eli_end_combo()",
    "Low-level combo: eli_begin_combo opens the dropdown popup when clicked. "
    "Fill the body with any widgets — typically eli_selectable rows — and "
    "call eli_end_combo to close. Only emit the body when it returns true.",
    "static const char *opts[] = {\"Red\", \"Green\", \"Blue\"};\n"
    "static int cur = 0;\n"
    "if (eli_begin_combo(\"Color\", opts[cur], ELI_COMBO_NONE)) {\n"
    "    for (int i = 0; i < 3; i++) {\n"
    "        eli_push_id_int(i);\n"
    "        bool sel = (i == cur);\n"
    "        if (eli_selectable(opts[i], sel,\n"
    "                           ELI_SELECTABLE_NONE,\n"
    "                           eli_make_vec2(0.0f, 0.0f)))\n"
    "            cur = i;\n"
    "        eli_pop_id();\n"
    "    }\n"
    "    eli_end_combo();\n"
    "}",
    cheat_render_begin_combo
},

{
    "Combo & Lists",
    "List Box",
    "bool eli_list_box(label, &current, items[], count, height)",
    "A framed, scrollable list that always shows rows (unlike a combo "
    "dropdown). Returns true on the frame the selection changes. Pass -1 "
    "for height_in_items to auto-size to min(count, 7) visible rows.",
    "static const char *planets[] =\n"
    "    {\"Mercury\", \"Venus\", \"Earth\", \"Mars\"};\n"
    "static int cur = 2;\n"
    "eli_list_box(\"Planet\", &cur, planets, 4, -1);",
    cheat_render_list_box
},

/* ---- Trees ------------------------------------------------------------ */

{
    "Trees",
    "Tree Node",
    "bool eli_tree_node(label) / eli_tree_pop()",
    "A collapsible node whose label also serves as its id. Emit children "
    "while it returns true, then call eli_tree_pop to close the indentation "
    "and id scope. Nodes can nest arbitrarily.",
    "if (eli_tree_node(\"Parent\")) {\n"
    "    eli_text(\"Child item 1\");\n"
    "    eli_text(\"Child item 2\");\n"
    "    if (eli_tree_node(\"Nested\")) {\n"
    "        eli_text(\"Leaf\");\n"
    "        eli_tree_pop();\n"
    "    }\n"
    "    eli_tree_pop();\n"
    "}",
    cheat_render_tree_node_children
},

{
    "Trees",
    "Tree Node Ex (flags)",
    "bool eli_tree_node_ex(label, flags)",
    "Tree node with explicit eli_tree_node_flags. DEFAULT_OPEN starts "
    "expanded on first display. LEAF draws a bullet instead of an arrow and "
    "never pushes a child scope, so no eli_tree_pop is needed for leaf nodes.",
    "/* Default-open branch */\n"
    "if (eli_tree_node_ex(\"Settings\",\n"
    "                      ELI_TREE_NODE_DEFAULT_OPEN)) {\n"
    "    eli_text(\"Visible by default\");\n"
    "    eli_tree_pop();\n"
    "}\n"
    "/* Leaf: no arrow, no tree_pop */\n"
    "eli_tree_node_ex(\"Item (leaf)\", ELI_TREE_NODE_LEAF);",
    cheat_render_tree_node_ex
},

{
    "Trees",
    "Collapsing Header",
    "bool eli_collapsing_header(label, flags)",
    "A framed section header that collapses/expands its body. Unlike a tree "
    "node it does not indent children or push an id scope, so no "
    "eli_tree_pop is required. Emit section contents while it returns true.",
    "if (eli_collapsing_header(\"Section A\", ELI_TREE_NODE_NONE)) {\n"
    "    eli_text(\"Content inside section A\");\n"
    "    eli_text(\"Another line of content\");\n"
    "}",
    cheat_render_collapsing_header
},

{
    "Trees",
    "Collapsing Header (closable)",
    "bool eli_collapsing_header_bool(label, &visible, flags)",
    "Collapsing header with an X close button. When *p_visible is true a "
    "small X is drawn on the header; clicking it sets *p_visible = false and "
    "hides the header completely. Pass NULL to omit the button. No "
    "eli_tree_pop needed.",
    "static bool visible = true;\n"
    "if (visible) {\n"
    "    if (eli_collapsing_header_bool(\"Section\", &visible,\n"
    "                                   ELI_TREE_NODE_NONE))\n"
    "        eli_text(\"Click X to close.\");\n"
    "}\n"
    "if (!visible && eli_button(\"Reopen\"))\n"
    "    visible = true;",
    cheat_render_collapsing_header_bool
},
