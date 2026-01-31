# elimgui - Build Plan

## Phase 1: Foundation
Core types, context, and basic rendering infrastructure.

- [ ] Create `elimgui.h` with core types (eli_vec2, eli_vec4, eli_rect, eli_col32)
- [ ] Define eli_context structure
- [ ] Define eli_io structure
- [ ] Implement eli_init() and eli_shutdown()
- [ ] Implement eli_new_frame() shell
- [ ] Implement eli_render() shell
- [ ] Create `eli_draw.h` with eli_draw_list and eli_draw_cmd types
- [ ] Implement draw list initialization and cleanup
- [ ] Implement eli_draw_list_add_rect_filled()
- [ ] Implement eli_draw_list_add_rect()
- [ ] Implement eli_draw_list_add_line()
- [ ] Create basic eli_get_draw_data()

## Phase 2: Basic Drawing
Complete draw primitives.

- [ ] Implement eli_draw_list_add_circle()
- [ ] Implement eli_draw_list_add_circle_filled()
- [ ] Implement eli_draw_list_add_triangle()
- [ ] Implement eli_draw_list_add_triangle_filled()
- [ ] Implement eli_draw_list_add_polyline()
- [ ] Implement eli_draw_list_add_convex_poly_filled()
- [ ] Implement draw command batching (merge adjacent same-texture draws)
- [ ] Implement clip rect stack (push/pop)

## Phase 3: Text & Fonts
Font loading and text rendering.

- [ ] Create `eli_font.h`
- [ ] Integrate stb_truetype.h
- [ ] Implement eli_font_load() from TTF data
- [ ] Implement font atlas generation
- [ ] Implement eli_font_get_tex_data()
- [ ] Implement eli_draw_list_add_text()
- [ ] Implement eli_calc_text_size()
- [ ] Implement default embedded font (proggy or similar)
- [ ] Implement eli_push_font() / eli_pop_font()

## Phase 4: Input System
Mouse and keyboard handling.

- [ ] Create `eli_input.h`
- [ ] Implement mouse position tracking
- [ ] Implement mouse button state
- [ ] Implement mouse wheel/scroll
- [ ] Implement keyboard key state
- [ ] Implement modifier keys (ctrl, shift, alt)
- [ ] Implement text input buffer
- [ ] Implement eli_is_mouse_hovering_rect()
- [ ] Implement eli_is_mouse_clicked()
- [ ] Implement eli_is_mouse_down()
- [ ] Implement eli_is_key_pressed()
- [ ] Implement eli_is_key_down()

## Phase 5: ID System & State
Widget identity and state management.

- [ ] Implement eli_id type and hashing
- [ ] Implement eli_get_id() from string
- [ ] Implement eli_get_id_ptr() from pointer
- [ ] Implement ID stack (push/pop)
- [ ] Implement `##` separator parsing for hidden IDs
- [ ] Implement `###` separator for stable IDs
- [ ] Implement active_id / hot_id tracking
- [ ] Implement eli_set_active_id()
- [ ] Implement eli_clear_active_id()

## Phase 6: Windows
Window management.

- [ ] Define eli_window structure
- [ ] Implement window storage in context
- [ ] Implement eli_begin() - create/find window
- [ ] Implement eli_end()
- [ ] Implement window title bar drawing
- [ ] Implement window background
- [ ] Implement window border
- [ ] Implement window move (drag title bar)
- [ ] Implement window resize (drag edges/corners)
- [ ] Implement window focus / z-order
- [ ] Implement window flags (NO_TITLEBAR, NO_RESIZE, etc.)
- [ ] Implement eli_set_next_window_pos()
- [ ] Implement eli_set_next_window_size()

## Phase 7: Layout System
Positioning and sizing.

- [ ] Create `eli_layout.h`
- [ ] Implement cursor position tracking
- [ ] Implement eli_same_line()
- [ ] Implement eli_new_line()
- [ ] Implement eli_separator()
- [ ] Implement eli_spacing()
- [ ] Implement eli_indent() / eli_unindent()
- [ ] Implement item width stack
- [ ] Implement eli_set_next_item_width()
- [ ] Implement eli_push_item_width() / eli_pop_item_width()
- [ ] Implement eli_get_cursor_pos() / eli_set_cursor_pos()
- [ ] Implement eli_get_content_region_avail()

## Phase 8: Basic Widgets
Core widget implementations.

- [ ] Create `eli_widgets.h`
- [ ] Implement eli_text()
- [ ] Implement eli_text_colored()
- [ ] Implement eli_button()
- [ ] Implement eli_button_sized()
- [ ] Implement eli_small_button()
- [ ] Implement eli_invisible_button()
- [ ] Implement eli_checkbox()
- [ ] Implement eli_radio_button()
- [ ] Implement button interaction (hover, active states)

## Phase 9: Input Widgets
Text and number input.

- [ ] Implement eli_input_text() - basic
- [ ] Implement text cursor rendering
- [ ] Implement text selection
- [ ] Implement copy/paste (via JS interop)
- [ ] Implement eli_input_int()
- [ ] Implement eli_input_float()
- [ ] Implement eli_input_text_multiline()

## Phase 10: Sliders & Drags
Value adjustment widgets.

- [ ] Implement eli_slider_float()
- [ ] Implement eli_slider_int()
- [ ] Implement eli_slider_angle()
- [ ] Implement eli_drag_float()
- [ ] Implement eli_drag_int()
- [ ] Implement eli_drag_float_range()
- [ ] Implement vertical sliders

## Phase 11: Color Widgets
Color editing.

- [ ] Implement eli_color_edit3()
- [ ] Implement eli_color_edit4()
- [ ] Implement eli_color_picker3()
- [ ] Implement eli_color_picker4()
- [ ] Implement color preview square
- [ ] Implement hue bar
- [ ] Implement saturation/value square

## Phase 12: Combo & Selectable
Dropdown and selection.

- [ ] Implement eli_begin_combo()
- [ ] Implement eli_end_combo()
- [ ] Implement eli_selectable()
- [ ] Implement eli_combo() helper (string array)
- [ ] Implement eli_list_box()

## Phase 13: Trees & Collapsing
Hierarchical widgets.

- [ ] Implement eli_tree_node()
- [ ] Implement eli_tree_node_ex() with flags
- [ ] Implement eli_tree_pop()
- [ ] Implement eli_collapsing_header()
- [ ] Implement tree indentation
- [ ] Implement tree arrow rendering

## Phase 14: Menus
Menu bar and context menus.

- [ ] Implement eli_begin_main_menu_bar()
- [ ] Implement eli_end_main_menu_bar()
- [ ] Implement eli_begin_menu_bar() (window)
- [ ] Implement eli_end_menu_bar()
- [ ] Implement eli_begin_menu()
- [ ] Implement eli_end_menu()
- [ ] Implement eli_menu_item()
- [ ] Implement menu item shortcuts display
- [ ] Implement submenus

## Phase 15: Popups & Modals
Popup windows.

- [ ] Implement popup stack
- [ ] Implement eli_open_popup()
- [ ] Implement eli_begin_popup()
- [ ] Implement eli_end_popup()
- [ ] Implement eli_begin_popup_context_item()
- [ ] Implement eli_begin_popup_context_window()
- [ ] Implement eli_begin_popup_modal()
- [ ] Implement modal backdrop
- [ ] Implement eli_close_current_popup()

## Phase 16: Scrolling
Scrollable regions.

- [ ] Implement scroll state per window
- [ ] Implement eli_begin_child() scrollable region
- [ ] Implement eli_end_child()
- [ ] Implement vertical scrollbar
- [ ] Implement horizontal scrollbar
- [ ] Implement mouse wheel scrolling
- [ ] Implement eli_set_scroll_here()
- [ ] Implement eli_set_scroll_y()

## Phase 17: Tables
Table widget.

- [ ] Create `eli_tables.h`
- [ ] Define eli_table_flags
- [ ] Define eli_table_column_flags
- [ ] Implement eli_begin_table()
- [ ] Implement eli_end_table()
- [ ] Implement eli_table_setup_column()
- [ ] Implement eli_table_headers_row()
- [ ] Implement eli_table_next_row()
- [ ] Implement eli_table_next_column()
- [ ] Implement eli_table_set_column_index()
- [ ] Implement column resizing
- [ ] Implement column sorting
- [ ] Implement table scrolling
- [ ] Implement row selection

## Phase 18: Styling
Theming and customization.

- [ ] Create `eli_style.h`
- [ ] Define all ELI_COL_* color indices
- [ ] Implement eli_style structure
- [ ] Implement eli_style_colors_dark()
- [ ] Implement eli_style_colors_light()
- [ ] Implement eli_push_style_color() / eli_pop_style_color()
- [ ] Implement eli_push_style_var() / eli_pop_style_var()
- [ ] Implement eli_get_style_color_vec4()

## Phase 19: Tooltips & Overlays
Helper overlays.

- [ ] Implement eli_set_tooltip()
- [ ] Implement eli_begin_tooltip()
- [ ] Implement eli_end_tooltip()
- [ ] Implement hover delay for tooltips
- [ ] Implement eli_is_item_hovered()
- [ ] Implement eli_is_item_active()
- [ ] Implement eli_is_item_clicked()

## Phase 20: Groups & Clipping
Layout helpers.

- [ ] Implement eli_begin_group()
- [ ] Implement eli_end_group()
- [ ] Implement eli_get_item_rect_min/max()
- [ ] Implement eli_push_clip_rect()
- [ ] Implement eli_pop_clip_rect()
- [ ] Implement simple columns (eli_columns, eli_next_column)

## Phase 21: Demo & Examples
Example application.

- [ ] Create examples/demo/main.c
- [ ] Implement eli_show_demo_window()
- [ ] Demo: Basic widgets section
- [ ] Demo: Layout section
- [ ] Demo: Input widgets section
- [ ] Demo: Trees and collapsing section
- [ ] Demo: Tables section
- [ ] Demo: Popups section
- [ ] Demo: Style editor section
- [ ] Create web/index.html for demo
- [ ] Create web/elimgui.js loader

## Phase 22: Testing
Test coverage.

- [ ] Create test framework (simple assertions)
- [ ] Test: Core types and math
- [ ] Test: ID hashing
- [ ] Test: Draw list generation
- [ ] Test: Layout calculations
- [ ] Test: Input state
- [ ] Test: Window management

## Phase 23: Optimization
Performance improvements.

- [ ] Profile draw list generation
- [ ] Optimize vertex buffer growth
- [ ] Implement draw call batching
- [ ] Reduce allocations per frame
- [ ] Measure and optimize WASM size

## Phase 24: Documentation
API documentation.

- [ ] Document public API in headers
- [ ] Create examples for each widget category
- [ ] Document integration guide
- [ ] Document styling guide

## Future: Docking (Phase 25+)
Window docking system.

- [ ] Create `eli_docking.h`
- [ ] Implement dock node tree
- [ ] Implement window docking
- [ ] Implement dock splitting
- [ ] Implement tab bar for docked windows
- [ ] Implement dock space
