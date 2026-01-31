# elimgui - Build Plan

This plan reflects **full feature parity** with Dear ImGui. Tasks are organized into phases that build on each other.

Each phase ends with a documentation task to create/update API docs in the `docs/` folder.

---

## Phase 1: Foundation & Core Types ✓

Core types, context management, and basic infrastructure.

### Core Types
- [x] Create `elimgui.h` with core types (eli_vec2, eli_vec4, eli_rect)
- [x] Define eli_col32 and color macros (ELI_COL32, ELI_COL32_WHITE, etc.)
- [x] Define eli_id type (uint32_t)
- [x] Define all flag enums (eli_window_flags, eli_child_flags, eli_item_flags)
- [x] Define eli_dir, eli_cond, eli_data_type enums
- [x] Define eli_key enum (full keyboard, mouse, gamepad)
- [x] Define eli_mouse_button and eli_mouse_cursor enums
- [x] Define eli_col enum (all 60 color indices)
- [x] Define eli_style_var enum (all 40 style variables)

### Context & IO
- [x] Define eli_io structure (full version with all fields)
- [x] Define eli_style structure (60+ properties)
- [x] Define eli_context structure
- [x] Implement eli_create_context()
- [x] Implement eli_destroy_context()
- [x] Implement eli_get_current_context()
- [x] Implement eli_set_current_context()

### Frame Lifecycle
- [x] Implement eli_new_frame()
- [x] Implement eli_end_frame()
- [x] Implement eli_render()
- [x] Implement eli_get_draw_data()

### Documentation
- [x] Create `docs/README.md` with table of contents structure
- [x] Create `docs/core-types.md` documenting core types, context, and frame lifecycle
- [x] Add core-types link to docs/README.md index

---

## Phase 2: Draw System ✓

Draw list, draw commands, and primitive rendering.

### Draw Types
- [x] Create `eli_draw.h`
- [x] Define eli_draw_cmd structure
- [x] Define eli_draw_vert structure
- [x] Define eli_draw_idx type
- [x] Define eli_draw_list structure (with path, clip, texture stacks)
- [x] Define eli_draw_data structure
- [x] Define eli_draw_list_flags and eli_draw_flags

### Draw List Management
- [x] Implement draw list initialization
- [x] Implement draw list cleanup
- [x] Implement draw command buffer growth
- [x] Implement vertex/index buffer growth
- [x] Implement draw command batching/merging

### Basic Primitives
- [x] Implement eli_draw_list_add_line()
- [x] Implement eli_draw_list_add_rect()
- [x] Implement eli_draw_list_add_rect_filled()
- [x] Implement eli_draw_list_add_rect_filled_multi_color()
- [x] Implement eli_draw_list_add_triangle()
- [x] Implement eli_draw_list_add_triangle_filled()
- [x] Implement eli_draw_list_add_quad()
- [x] Implement eli_draw_list_add_quad_filled()

### Circle/Ellipse Primitives
- [x] Implement eli_draw_list_add_circle()
- [x] Implement eli_draw_list_add_circle_filled()
- [x] Implement eli_draw_list_add_ngon()
- [x] Implement eli_draw_list_add_ngon_filled()
- [x] Implement eli_draw_list_add_ellipse()
- [x] Implement eli_draw_list_add_ellipse_filled()

### Bezier & Polyline
- [x] Implement eli_draw_list_add_polyline()
- [x] Implement eli_draw_list_add_convex_poly_filled()
- [x] Implement eli_draw_list_add_bezier_cubic()
- [x] Implement eli_draw_list_add_bezier_quadratic()

### Path API
- [x] Implement eli_draw_list_path_clear()
- [x] Implement eli_draw_list_path_line_to()
- [x] Implement eli_draw_list_path_line_to_merge_duplicate()
- [x] Implement eli_draw_list_path_fill_convex()
- [x] Implement eli_draw_list_path_stroke()
- [x] Implement eli_draw_list_path_arc_to()
- [x] Implement eli_draw_list_path_arc_to_fast()
- [x] Implement eli_draw_list_path_elliptical_arc_to()
- [x] Implement eli_draw_list_path_bezier_cubic_curve_to()
- [x] Implement eli_draw_list_path_bezier_quadratic_curve_to()
- [x] Implement eli_draw_list_path_rect()

### Clip & Texture Stacks
- [x] Implement eli_draw_list_push_clip_rect()
- [x] Implement eli_draw_list_push_clip_rect_full_screen()
- [x] Implement eli_draw_list_pop_clip_rect()
- [x] Implement eli_draw_list_get_clip_rect_min/max()
- [x] Implement eli_draw_list_push_texture_id()
- [x] Implement eli_draw_list_pop_texture_id()

### Primitives Reservation
- [x] Implement eli_draw_list_prim_reserve()
- [x] Implement eli_draw_list_prim_unreserve()
- [x] Implement eli_draw_list_prim_rect()
- [x] Implement eli_draw_list_prim_rect_uv()
- [x] Implement eli_draw_list_prim_quad_uv()
- [x] Implement eli_draw_list_prim_write_vtx()
- [x] Implement eli_draw_list_prim_write_idx()
- [x] Implement eli_draw_list_prim_vtx()

### Channels (Draw List Splitting)
- [x] Implement eli_draw_list_channels_split()
- [x] Implement eli_draw_list_channels_merge()
- [x] Implement eli_draw_list_channels_set_current()

### Advanced Draw List
- [x] Implement eli_draw_list_add_callback()
- [x] Implement eli_draw_list_add_draw_cmd()
- [x] Implement eli_draw_list_clone_output()

### Documentation
- [x] Create `docs/draw-system.md` documenting draw lists, primitives, paths, and channels
- [x] Add draw-system link to docs/README.md index

---

## Phase 3: Font System ✓

Font loading, atlas generation, and text rendering.

### Font Types
- [x] Create `eli_font.h`
- [x] Define eli_font structure
- [x] Define eli_font_atlas structure
- [x] Define eli_font_config structure
- [x] Define eli_font_glyph structure

### Font Atlas
- [x] Integrate stb_truetype.h (vendored in vendor/stb/)
- [x] Implement eli_font_atlas_add_font_default() (with embedded ProggyClean)
- [x] Implement eli_font_atlas_add_font_from_memory_ttf()
- [x] Implement eli_font_atlas_build()
- [x] Implement eli_font_atlas_get_tex_data_as_alpha8()
- [x] Implement eli_font_atlas_get_tex_data_as_rgba32()
- [x] Implement eli_font_atlas_is_built()
- [x] Implement eli_font_atlas_set_tex_id()
- [x] Implement eli_font_atlas_clear() variants

### Glyph Ranges
- [x] Implement eli_font_atlas_get_glyph_ranges_default()
- [x] Implement eli_font_atlas_get_glyph_ranges_greek()
- [x] Implement eli_font_atlas_get_glyph_ranges_korean()
- [x] Implement eli_font_atlas_get_glyph_ranges_japanese()
- [x] Implement eli_font_atlas_get_glyph_ranges_chinese_full()
- [x] Implement eli_font_atlas_get_glyph_ranges_chinese_simplified_common()
- [x] Implement eli_font_atlas_get_glyph_ranges_cyrillic()
- [x] Implement eli_font_atlas_get_glyph_ranges_thai()
- [x] Implement eli_font_atlas_get_glyph_ranges_vietnamese()

### Text Rendering
- [x] Implement eli_draw_list_add_text()
- [x] Implement eli_draw_list_add_text_ex()
- [x] Implement eli_calc_text_size()
- [x] Implement embedded default font (ProggyClean in eli_font_proggy.h)

### Font Stack
- [x] Implement eli_push_font()
- [x] Implement eli_pop_font()
- [x] Implement eli_get_font()
- [x] Implement eli_get_font_size()
- [x] Implement eli_get_font_tex_uv_white_pixel()

### Documentation
- [x] Create `docs/font-system.md` documenting font atlas, glyph ranges, and text rendering
- [x] Add font-system link to docs/README.md index

---

## Phase 4: Input System ✓

Mouse, keyboard, and input handling.

### Mouse Input
- [x] Create `eli_input.h`
- [x] Implement mouse position tracking in IO
- [x] Implement mouse button state tracking
- [x] Implement mouse wheel/scroll tracking
- [x] Implement eli_is_mouse_down()
- [x] Implement eli_is_mouse_clicked()
- [x] Implement eli_is_mouse_released()
- [x] Implement eli_is_mouse_double_clicked()
- [x] Implement eli_get_mouse_clicked_count()
- [x] Implement eli_is_mouse_hovering_rect()
- [x] Implement eli_is_mouse_pos_valid()
- [x] Implement eli_is_any_mouse_down()
- [x] Implement eli_get_mouse_pos()
- [x] Implement eli_get_mouse_pos_on_opening_current_popup()
- [x] Implement eli_is_mouse_dragging()
- [x] Implement eli_get_mouse_drag_delta()
- [x] Implement eli_reset_mouse_drag_delta()
- [x] Implement eli_get_mouse_cursor()
- [x] Implement eli_set_mouse_cursor()
- [x] Implement eli_set_next_frame_want_capture_mouse()

### Keyboard Input
- [x] Implement keyboard key state tracking
- [x] Implement modifier keys (ctrl, shift, alt, super)
- [x] Implement eli_is_key_down()
- [x] Implement eli_is_key_pressed()
- [x] Implement eli_is_key_released()
- [x] Implement eli_is_key_chord_pressed()
- [x] Implement eli_get_key_pressed_amount()
- [x] Implement eli_get_key_name()
- [x] Implement eli_set_next_frame_want_capture_keyboard()

### Text Input
- [x] Implement text input queue in IO
- [x] Implement character input handling (eli_io_add_input_character, eli_io_add_input_characters_utf8)

### Shortcuts
- [x] Implement eli_shortcut()
- [x] Implement predefined shortcuts (ELI_SHORTCUT_COPY, etc.)

### Clipboard (via JS interop)
- [x] Implement eli_get_clipboard_text()
- [x] Implement eli_set_clipboard_text()
- [x] Implement eli_set_clipboard_callbacks()

### Backend Integration
- [x] Implement eli_io_add_mouse_pos_event()
- [x] Implement eli_io_add_mouse_button_event()
- [x] Implement eli_io_add_mouse_wheel_event()
- [x] Implement eli_io_add_key_event()
- [x] Implement eli_io_add_key_analog_event()
- [x] Implement eli_input_update_begin_frame()
- [x] Implement eli_input_update_end_frame()

### Documentation
- [x] Create `docs/input-system.md` documenting mouse, keyboard, text input, and shortcuts
- [x] Add input-system link to docs/README.md index

---

## Phase 5: ID System & State

Widget identity, hashing, and state management.

### ID Hashing
- [ ] Implement ID hashing algorithm (CRC32 or similar)
- [ ] Implement `##` separator parsing for hidden IDs
- [ ] Implement `###` separator for stable IDs

### ID Stack
- [ ] Implement eli_push_id()
- [ ] Implement eli_push_id_str()
- [ ] Implement eli_push_id_ptr()
- [ ] Implement eli_push_id_int()
- [ ] Implement eli_pop_id()
- [ ] Implement eli_get_id()
- [ ] Implement eli_get_id_str()
- [ ] Implement eli_get_id_ptr()
- [ ] Implement eli_get_id_int()

### Active/Hot ID
- [ ] Implement active_id tracking
- [ ] Implement hot_id tracking
- [ ] Implement eli_set_active_id() (internal)
- [ ] Implement eli_clear_active_id() (internal)

### Storage
- [ ] Define eli_storage structure
- [ ] Implement eli_storage operations (get/set int/float/ptr)
- [ ] Implement eli_set_state_storage()
- [ ] Implement eli_get_state_storage()

### Documentation
- [ ] Create `docs/id-system.md` documenting ID hashing, ID stack, and state storage
- [ ] Add id-system link to docs/README.md index

---

## Phase 6: Style System

Colors, sizing, and theming.

### Style Structure
- [ ] Create `eli_style.h`
- [ ] Implement full eli_style structure (60+ properties)
- [ ] Define all ELI_COL_* indices
- [ ] Define all ELI_STYLE_VAR_* indices

### Style Functions
- [ ] Implement eli_get_style()
- [ ] Implement eli_style_colors_dark()
- [ ] Implement eli_style_colors_light()
- [ ] Implement eli_style_colors_classic()

### Style Stack
- [ ] Implement eli_push_style_color()
- [ ] Implement eli_push_style_color_vec4()
- [ ] Implement eli_pop_style_color()
- [ ] Implement eli_push_style_var()
- [ ] Implement eli_push_style_var_vec2()
- [ ] Implement eli_pop_style_var()

### Item Flags
- [ ] Implement eli_push_item_flag()
- [ ] Implement eli_pop_item_flag()

### Color Utilities
- [ ] Implement eli_get_color_u32()
- [ ] Implement eli_get_color_u32_vec4()
- [ ] Implement eli_get_color_u32_col32()
- [ ] Implement eli_get_style_color_vec4()
- [ ] Implement eli_get_style_color_name()
- [ ] Implement eli_color_convert_u32_to_float4()
- [ ] Implement eli_color_convert_float4_to_u32()
- [ ] Implement eli_color_convert_rgb_to_hsv()
- [ ] Implement eli_color_convert_hsv_to_rgb()

### Documentation
- [ ] Create `docs/style-system.md` documenting styles, colors, theming, and color utilities
- [ ] Add style-system link to docs/README.md index

---

## Phase 7: Windows

Window management and rendering.

### Window Structure
- [ ] Define eli_window structure
- [ ] Implement window storage in context
- [ ] Implement window lookup by name/ID

### Window Lifecycle
- [ ] Implement eli_begin()
- [ ] Implement eli_end()
- [ ] Implement window creation/retrieval
- [ ] Implement window title bar rendering
- [ ] Implement window background
- [ ] Implement window border

### Window Interaction
- [ ] Implement window move (drag title bar)
- [ ] Implement window resize (drag edges/corners)
- [ ] Implement window focus/z-order
- [ ] Implement window collapse
- [ ] Implement all window flags

### Window State Queries
- [ ] Implement eli_is_window_appearing()
- [ ] Implement eli_is_window_collapsed()
- [ ] Implement eli_is_window_focused()
- [ ] Implement eli_is_window_hovered()
- [ ] Implement eli_get_window_draw_list()
- [ ] Implement eli_get_window_pos()
- [ ] Implement eli_get_window_size()
- [ ] Implement eli_get_window_width()
- [ ] Implement eli_get_window_height()

### Window Manipulation
- [ ] Implement eli_set_next_window_pos()
- [ ] Implement eli_set_next_window_size()
- [ ] Implement eli_set_next_window_size_constraints()
- [ ] Implement eli_set_next_window_content_size()
- [ ] Implement eli_set_next_window_collapsed()
- [ ] Implement eli_set_next_window_focus()
- [ ] Implement eli_set_next_window_scroll()
- [ ] Implement eli_set_next_window_bg_alpha()
- [ ] Implement eli_set_window_pos() variants
- [ ] Implement eli_set_window_size() variants
- [ ] Implement eli_set_window_collapsed() variants
- [ ] Implement eli_set_window_focus() variants
- [ ] Implement eli_set_window_font_scale()

### Child Windows
- [ ] Implement eli_begin_child()
- [ ] Implement eli_begin_child_id()
- [ ] Implement eli_end_child()

### Scrolling
- [ ] Implement scroll state per window
- [ ] Implement eli_get_scroll_x/y()
- [ ] Implement eli_set_scroll_x/y()
- [ ] Implement eli_get_scroll_max_x/y()
- [ ] Implement eli_set_scroll_here_x/y()
- [ ] Implement eli_set_scroll_from_pos_x/y()
- [ ] Implement vertical scrollbar
- [ ] Implement horizontal scrollbar
- [ ] Implement mouse wheel scrolling

### Documentation
- [ ] Create `docs/windows.md` documenting window lifecycle, flags, child windows, and scrolling
- [ ] Add windows link to docs/README.md index

---

## Phase 8: Layout System

Positioning, sizing, and layout helpers.

### Cursor
- [ ] Create `eli_layout.h`
- [ ] Implement cursor position tracking
- [ ] Implement eli_get_cursor_pos()
- [ ] Implement eli_get_cursor_pos_x/y()
- [ ] Implement eli_set_cursor_pos()
- [ ] Implement eli_set_cursor_pos_x/y()
- [ ] Implement eli_get_cursor_start_pos()
- [ ] Implement eli_get_cursor_screen_pos()
- [ ] Implement eli_set_cursor_screen_pos()

### Layout Helpers
- [ ] Implement eli_separator()
- [ ] Implement eli_same_line()
- [ ] Implement eli_new_line()
- [ ] Implement eli_spacing()
- [ ] Implement eli_dummy()
- [ ] Implement eli_indent()
- [ ] Implement eli_unindent()
- [ ] Implement eli_align_text_to_frame_padding()

### Groups
- [ ] Implement eli_begin_group()
- [ ] Implement eli_end_group()

### Content Region
- [ ] Implement eli_get_content_region_avail()
- [ ] Implement eli_get_content_region_max()
- [ ] Implement eli_get_window_content_region_min()
- [ ] Implement eli_get_window_content_region_max()

### Item Width
- [ ] Implement item width stack
- [ ] Implement eli_push_item_width()
- [ ] Implement eli_pop_item_width()
- [ ] Implement eli_set_next_item_width()
- [ ] Implement eli_calc_item_width()

### Text Wrap
- [ ] Implement eli_push_text_wrap_pos()
- [ ] Implement eli_pop_text_wrap_pos()

### Sizing Helpers
- [ ] Implement eli_get_text_line_height()
- [ ] Implement eli_get_text_line_height_with_spacing()
- [ ] Implement eli_get_frame_height()
- [ ] Implement eli_get_frame_height_with_spacing()

### Documentation
- [ ] Create `docs/layout-system.md` documenting cursor, layout helpers, groups, and sizing
- [ ] Add layout-system link to docs/README.md index

---

## Phase 9: Basic Widgets

Text display and button widgets.

### Text Widgets
- [ ] Create `eli_widgets.h`
- [ ] Implement eli_text_unformatted()
- [ ] Implement eli_text()
- [ ] Implement eli_text_v()
- [ ] Implement eli_text_colored()
- [ ] Implement eli_text_colored_v()
- [ ] Implement eli_text_disabled()
- [ ] Implement eli_text_disabled_v()
- [ ] Implement eli_text_wrapped()
- [ ] Implement eli_text_wrapped_v()
- [ ] Implement eli_label_text()
- [ ] Implement eli_label_text_v()
- [ ] Implement eli_bullet_text()
- [ ] Implement eli_bullet_text_v()
- [ ] Implement eli_separator_text()
- [ ] Implement eli_bullet()

### Buttons
- [ ] Implement eli_button()
- [ ] Implement eli_button_ex()
- [ ] Implement eli_small_button()
- [ ] Implement eli_invisible_button()
- [ ] Implement eli_arrow_button()
- [ ] Implement button interaction (hover, active states)

### Checkboxes & Radio
- [ ] Implement eli_checkbox()
- [ ] Implement eli_checkbox_flags_int()
- [ ] Implement eli_checkbox_flags_uint()
- [ ] Implement eli_radio_button()
- [ ] Implement eli_radio_button_int()

### Progress & Links
- [ ] Implement eli_progress_bar()
- [ ] Implement eli_text_link()
- [ ] Implement eli_text_link_open_url()

### Documentation
- [ ] Create `docs/basic-widgets.md` documenting text, buttons, checkboxes, radio, and progress
- [ ] Add basic-widgets link to docs/README.md index

---

## Phase 10: Item Status Queries

Widget state queries (critical for composition).

- [ ] Implement eli_is_item_hovered()
- [ ] Implement eli_is_item_active()
- [ ] Implement eli_is_item_focused()
- [ ] Implement eli_is_item_clicked()
- [ ] Implement eli_is_item_visible()
- [ ] Implement eli_is_item_edited()
- [ ] Implement eli_is_item_activated()
- [ ] Implement eli_is_item_deactivated()
- [ ] Implement eli_is_item_deactivated_after_edit()
- [ ] Implement eli_is_item_toggled_open()
- [ ] Implement eli_is_any_item_hovered()
- [ ] Implement eli_is_any_item_active()
- [ ] Implement eli_is_any_item_focused()
- [ ] Implement eli_get_item_id()
- [ ] Implement eli_get_item_rect_min()
- [ ] Implement eli_get_item_rect_max()
- [ ] Implement eli_get_item_rect_size()

### Documentation
- [ ] Create `docs/item-status.md` documenting item state queries and rect helpers
- [ ] Add item-status link to docs/README.md index

---

## Phase 11: Sliders & Drags

Value adjustment widgets.

### Slider Implementation
- [ ] Implement slider behavior (internal)
- [ ] Implement eli_slider_float()
- [ ] Implement eli_slider_float2/3/4()
- [ ] Implement eli_slider_angle()
- [ ] Implement eli_slider_int()
- [ ] Implement eli_slider_int2/3/4()
- [ ] Implement eli_slider_scalar()
- [ ] Implement eli_slider_scalar_n()

### Vertical Sliders
- [ ] Implement eli_v_slider_float()
- [ ] Implement eli_v_slider_int()
- [ ] Implement eli_v_slider_scalar()

### Drag Implementation
- [ ] Implement drag behavior (internal)
- [ ] Implement eli_drag_float()
- [ ] Implement eli_drag_float2/3/4()
- [ ] Implement eli_drag_float_range2()
- [ ] Implement eli_drag_int()
- [ ] Implement eli_drag_int2/3/4()
- [ ] Implement eli_drag_int_range2()
- [ ] Implement eli_drag_scalar()
- [ ] Implement eli_drag_scalar_n()

### Documentation
- [ ] Create `docs/sliders-drags.md` documenting slider and drag widgets
- [ ] Add sliders-drags link to docs/README.md index

---

## Phase 12: Input Widgets

Text and number input.

### Input Text Callback
- [ ] Define eli_input_text_callback_data structure
- [ ] Define eli_input_text_callback type

### Text Input
- [ ] Implement eli_input_text()
- [ ] Implement text cursor rendering
- [ ] Implement text selection
- [ ] Implement copy/paste (via JS interop)
- [ ] Implement eli_input_text_multiline()
- [ ] Implement eli_input_text_with_hint()

### Numeric Input
- [ ] Implement eli_input_float()
- [ ] Implement eli_input_float2/3/4()
- [ ] Implement eli_input_int()
- [ ] Implement eli_input_int2/3/4()
- [ ] Implement eli_input_double()
- [ ] Implement eli_input_scalar()
- [ ] Implement eli_input_scalar_n()

### Documentation
- [ ] Create `docs/input-widgets.md` documenting text and numeric input widgets
- [ ] Add input-widgets link to docs/README.md index

---

## Phase 13: Color Widgets

Color editing and picking.

- [ ] Implement eli_color_edit3()
- [ ] Implement eli_color_edit4()
- [ ] Implement eli_color_picker3()
- [ ] Implement eli_color_picker4()
- [ ] Implement eli_color_button()
- [ ] Implement eli_set_color_edit_options()
- [ ] Implement color preview square
- [ ] Implement hue bar
- [ ] Implement saturation/value square
- [ ] Implement alpha bar

### Documentation
- [ ] Create `docs/color-widgets.md` documenting color edit, picker, and button widgets
- [ ] Add color-widgets link to docs/README.md index

---

## Phase 14: Combo & Selectable

Dropdown and selection widgets.

### Combo
- [ ] Implement eli_begin_combo()
- [ ] Implement eli_end_combo()
- [ ] Implement eli_combo()
- [ ] Implement eli_combo_str()
- [ ] Implement eli_combo_fn()

### Selectable
- [ ] Implement eli_selectable()
- [ ] Implement eli_selectable_bool()

### List Box
- [ ] Implement eli_begin_list_box()
- [ ] Implement eli_end_list_box()
- [ ] Implement eli_list_box()
- [ ] Implement eli_list_box_fn()

### Documentation
- [ ] Create `docs/combo-selectable.md` documenting combo, selectable, and list box widgets
- [ ] Add combo-selectable link to docs/README.md index

---

## Phase 15: Trees & Collapsing

Hierarchical widgets.

- [ ] Implement eli_tree_node()
- [ ] Implement eli_tree_node_str()
- [ ] Implement eli_tree_node_ptr()
- [ ] Implement eli_tree_node_v()
- [ ] Implement eli_tree_node_ex()
- [ ] Implement eli_tree_node_ex_str()
- [ ] Implement eli_tree_node_ex_ptr()
- [ ] Implement eli_tree_node_ex_v()
- [ ] Implement eli_tree_push()
- [ ] Implement eli_tree_push_ptr()
- [ ] Implement eli_tree_pop()
- [ ] Implement eli_get_tree_node_to_label_spacing()
- [ ] Implement eli_collapsing_header()
- [ ] Implement eli_collapsing_header_bool()
- [ ] Implement eli_set_next_item_open()
- [ ] Implement eli_set_next_item_storage_id()
- [ ] Implement tree indentation
- [ ] Implement tree arrow rendering

### Documentation
- [ ] Create `docs/trees-collapsing.md` documenting tree nodes and collapsing headers
- [ ] Add trees-collapsing link to docs/README.md index

---

## Phase 16: Menus

Menu bar and menu items.

### Menu Bar
- [ ] Implement eli_begin_menu_bar()
- [ ] Implement eli_end_menu_bar()
- [ ] Implement eli_begin_main_menu_bar()
- [ ] Implement eli_end_main_menu_bar()

### Menus
- [ ] Implement eli_begin_menu()
- [ ] Implement eli_end_menu()
- [ ] Implement eli_menu_item()
- [ ] Implement eli_menu_item_bool()
- [ ] Implement menu item shortcut display
- [ ] Implement submenus

### Documentation
- [ ] Create `docs/menus.md` documenting menu bar and menu items
- [ ] Add menus link to docs/README.md index

---

## Phase 17: Popups & Modals

Popup windows and modal dialogs.

### Popup Stack
- [ ] Implement popup stack management
- [ ] Implement eli_open_popup()
- [ ] Implement eli_open_popup_id()
- [ ] Implement eli_open_popup_on_item_click()
- [ ] Implement eli_close_current_popup()
- [ ] Implement eli_is_popup_open()

### Popup Windows
- [ ] Implement eli_begin_popup()
- [ ] Implement eli_end_popup()
- [ ] Implement eli_begin_popup_context_item()
- [ ] Implement eli_begin_popup_context_window()
- [ ] Implement eli_begin_popup_context_void()

### Modal Dialogs
- [ ] Implement eli_begin_popup_modal()
- [ ] Implement modal backdrop rendering

### Documentation
- [ ] Create `docs/popups-modals.md` documenting popups, context menus, and modals
- [ ] Add popups-modals link to docs/README.md index

---

## Phase 18: Tooltips

Hover tooltips.

- [ ] Implement eli_begin_tooltip()
- [ ] Implement eli_end_tooltip()
- [ ] Implement eli_set_tooltip()
- [ ] Implement eli_set_tooltip_v()
- [ ] Implement eli_begin_item_tooltip()
- [ ] Implement eli_set_item_tooltip()
- [ ] Implement eli_set_item_tooltip_v()
- [ ] Implement hover delay for tooltips

### Documentation
- [ ] Create `docs/tooltips.md` documenting tooltip widgets
- [ ] Add tooltips link to docs/README.md index

---

## Phase 19: Tables

Full table widget system.

### Table Structure
- [ ] Create `eli_tables.h`
- [ ] Define table internal structures
- [ ] Define eli_table_sort_specs structure
- [ ] Define eli_table_column_sort_specs structure

### Table Lifecycle
- [ ] Implement eli_begin_table()
- [ ] Implement eli_end_table()
- [ ] Implement eli_table_next_row()
- [ ] Implement eli_table_next_column()
- [ ] Implement eli_table_set_column_index()

### Table Setup
- [ ] Implement eli_table_setup_column()
- [ ] Implement eli_table_setup_scroll_freeze()
- [ ] Implement eli_table_header()
- [ ] Implement eli_table_headers_row()
- [ ] Implement eli_table_angled_headers_row()

### Table Queries
- [ ] Implement eli_table_get_sort_specs()
- [ ] Implement eli_table_get_column_count()
- [ ] Implement eli_table_get_column_index()
- [ ] Implement eli_table_get_row_index()
- [ ] Implement eli_table_get_column_name()
- [ ] Implement eli_table_get_column_flags()
- [ ] Implement eli_table_set_column_enabled()
- [ ] Implement eli_table_get_hovered_column()
- [ ] Implement eli_table_set_bg_color()

### Table Features
- [ ] Implement column resizing
- [ ] Implement column reordering
- [ ] Implement column hiding
- [ ] Implement column sorting
- [ ] Implement table scrolling
- [ ] Implement row backgrounds

### Documentation
- [ ] Create `docs/tables.md` documenting table widget, columns, sorting, and features
- [ ] Add tables link to docs/README.md index

---

## Phase 20: Tab Bars

Tab bar widget.

- [ ] Create `eli_tabs.h`
- [ ] Implement eli_begin_tab_bar()
- [ ] Implement eli_end_tab_bar()
- [ ] Implement eli_begin_tab_item()
- [ ] Implement eli_end_tab_item()
- [ ] Implement eli_tab_item_button()
- [ ] Implement eli_set_tab_item_closed()
- [ ] Implement tab reordering
- [ ] Implement tab scrolling
- [ ] Implement tab close button

### Documentation
- [ ] Create `docs/tab-bars.md` documenting tab bar and tab item widgets
- [ ] Add tab-bars link to docs/README.md index

---

## Phase 21: Drag & Drop

Drag and drop system.

### Payload
- [ ] Define eli_payload structure
- [ ] Implement payload storage

### Source
- [ ] Implement eli_begin_drag_drop_source()
- [ ] Implement eli_set_drag_drop_payload()
- [ ] Implement eli_end_drag_drop_source()

### Target
- [ ] Implement eli_begin_drag_drop_target()
- [ ] Implement eli_accept_drag_drop_payload()
- [ ] Implement eli_end_drag_drop_target()
- [ ] Implement eli_get_drag_drop_payload()

### Visual Feedback
- [ ] Implement drag preview
- [ ] Implement drop target highlight

### Documentation
- [ ] Create `docs/drag-drop.md` documenting drag and drop system
- [ ] Add drag-drop link to docs/README.md index

---

## Phase 22: Images

Image display widgets.

- [ ] Implement eli_image()
- [ ] Implement eli_image_button()
- [ ] Implement eli_draw_list_add_image()
- [ ] Implement eli_draw_list_add_image_quad()
- [ ] Implement eli_draw_list_add_image_rounded()

### Documentation
- [ ] Create `docs/images.md` documenting image display widgets
- [ ] Add images link to docs/README.md index

---

## Phase 23: Data Plotting

Simple plotting widgets.

- [ ] Implement eli_plot_lines()
- [ ] Implement eli_plot_lines_fn()
- [ ] Implement eli_plot_histogram()
- [ ] Implement eli_plot_histogram_fn()

### Documentation
- [ ] Create `docs/plotting.md` documenting plot lines and histogram widgets
- [ ] Add plotting link to docs/README.md index

---

## Phase 24: Value Display

Simple value display widgets.

- [ ] Implement eli_value_bool()
- [ ] Implement eli_value_int()
- [ ] Implement eli_value_uint()
- [ ] Implement eli_value_float()

### Documentation
- [ ] Create `docs/value-display.md` documenting value display widgets
- [ ] Add value-display link to docs/README.md index

---

## Phase 25: Disabling & Clipping

Widget disabling and clipping.

### Disabling
- [ ] Implement eli_begin_disabled()
- [ ] Implement eli_end_disabled()

### Clipping
- [ ] Implement eli_push_clip_rect()
- [ ] Implement eli_pop_clip_rect()

### Focus
- [ ] Implement eli_set_item_default_focus()
- [ ] Implement eli_set_keyboard_focus_here()

### Documentation
- [ ] Create `docs/disabling-clipping.md` documenting disabled state, clipping, and focus
- [ ] Add disabling-clipping link to docs/README.md index

---

## Phase 26: List Clipper

Efficient list rendering.

- [ ] Define eli_list_clipper structure
- [ ] Implement eli_list_clipper_begin()
- [ ] Implement eli_list_clipper_end()
- [ ] Implement eli_list_clipper_step()
- [ ] Implement eli_list_clipper_include_item_by_index()
- [ ] Implement eli_list_clipper_include_items_by_index()
- [ ] Implement eli_list_clipper_seek_cursor_for_item()

### Documentation
- [ ] Create `docs/list-clipper.md` documenting list clipper for efficient rendering
- [ ] Add list-clipper link to docs/README.md index

---

## Phase 27: Misc Utilities

Remaining utility functions.

### Visibility
- [ ] Implement eli_is_rect_visible()
- [ ] Implement eli_is_rect_visible_vec2()

### Time & Frame
- [ ] Implement eli_get_time()
- [ ] Implement eli_get_frame_count()

### Viewports
- [ ] Define eli_viewport structure
- [ ] Implement eli_get_main_viewport()

### Draw Lists
- [ ] Implement eli_get_background_draw_list()
- [ ] Implement eli_get_foreground_draw_list()

### Documentation
- [ ] Create `docs/utilities.md` documenting visibility, time, viewports, and draw lists
- [ ] Add utilities link to docs/README.md index

---

## Phase 28: Settings & Logging

Configuration persistence and logging.

### Settings
- [ ] Implement eli_load_ini_settings_from_disk()
- [ ] Implement eli_load_ini_settings_from_memory()
- [ ] Implement eli_save_ini_settings_to_disk()
- [ ] Implement eli_save_ini_settings_to_memory()

### Logging
- [ ] Implement eli_log_to_tty()
- [ ] Implement eli_log_to_file()
- [ ] Implement eli_log_to_clipboard()
- [ ] Implement eli_log_finish()
- [ ] Implement eli_log_buttons()
- [ ] Implement eli_log_text()
- [ ] Implement eli_log_text_v()

### Documentation
- [ ] Create `docs/settings-logging.md` documenting INI settings and logging
- [ ] Add settings-logging link to docs/README.md index

---

## Phase 29: Memory Management

Custom allocators.

- [ ] Implement eli_set_allocator_functions()
- [ ] Implement eli_get_allocator_functions()
- [ ] Implement eli_mem_alloc()
- [ ] Implement eli_mem_free()

### Documentation
- [ ] Create `docs/memory.md` documenting custom allocators and memory management
- [ ] Add memory link to docs/README.md index

---

## Phase 30: Demo & Debug Windows

Demo application and debug tools.

### Demo Window
- [ ] Implement eli_show_demo_window()
- [ ] Demo: Basic widgets section
- [ ] Demo: Layout section
- [ ] Demo: Input widgets section
- [ ] Demo: Sliders & drags section
- [ ] Demo: Color widgets section
- [ ] Demo: Trees and collapsing section
- [ ] Demo: Tables section
- [ ] Demo: Tabs section
- [ ] Demo: Popups section
- [ ] Demo: Drag & drop section
- [ ] Demo: Style editor section

### Debug Windows
- [ ] Implement eli_show_metrics_window()
- [ ] Implement eli_show_debug_log_window()
- [ ] Implement eli_show_id_stack_tool_window()
- [ ] Implement eli_show_about_window()
- [ ] Implement eli_show_style_editor()
- [ ] Implement eli_show_style_selector()
- [ ] Implement eli_show_font_selector()
- [ ] Implement eli_show_user_guide()
- [ ] Implement eli_get_version()

### Documentation
- [ ] Create `docs/demo-debug.md` documenting demo window and debug tools
- [ ] Add demo-debug link to docs/README.md index

---

## Phase 31: Web Integration

WASM loader and browser integration.

- [ ] Create web/elimgui.js WASM loader
- [ ] Create web/index.html demo shell
- [ ] Implement JS event handlers (mouse, keyboard)
- [ ] Implement Canvas2D renderer backend
- [ ] Implement WebGL renderer backend (optional)

### Documentation
- [ ] Create `docs/web-integration.md` documenting WASM loader, JS events, and renderers
- [ ] Add web-integration link to docs/README.md index

---

## Phase 32: Testing

Test coverage.

- [ ] Create test framework
- [ ] Test: Core types and math
- [ ] Test: ID hashing
- [ ] Test: Draw list generation
- [ ] Test: Layout calculations
- [ ] Test: Input state
- [ ] Test: Window management
- [ ] Test: Widget behavior
- [ ] Test: Table functionality

### Documentation
- [ ] Create `docs/testing.md` documenting test framework and coverage
- [ ] Add testing link to docs/README.md index

---

## Phase 33: Optimization

Performance improvements.

- [ ] Profile draw list generation
- [ ] Optimize vertex buffer growth
- [ ] Implement draw call batching
- [ ] Reduce allocations per frame
- [ ] Measure and optimize WASM size

### Documentation
- [ ] Create `docs/optimization.md` documenting performance tips and benchmarks
- [ ] Add optimization link to docs/README.md index

---

## Future: Docking (Phase 34+)

Window docking system.

- [ ] Create `eli_docking.h`
- [ ] Implement dock node tree
- [ ] Implement window docking
- [ ] Implement dock splitting
- [ ] Implement tab bar for docked windows
- [ ] Implement dock space

### Documentation
- [ ] Create `docs/docking.md` documenting docking system
- [ ] Add docking link to docs/README.md index
