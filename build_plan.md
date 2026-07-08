# elimgui - Build Plan

This plan reflects **full feature parity** with Dear ImGui. Tasks are organized into phases that build on each other.

Each phase ends with a documentation task to create/update API docs in the `docs/` folder.

---

## Phase 1: Foundation & Core Types

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

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/README.md` with table of contents structure
- [x] Create `docs/core-types.md` documenting core types, context, and frame lifecycle
- [x] Add core-types link to docs/README.md index

---

## Phase 2: Draw System

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

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/draw-system.md` documenting draw lists, primitives, paths, and channels
- [x] Add draw-system link to docs/README.md index

---

## Phase 3: Font System

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

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/font-system.md` documenting font atlas, glyph ranges, and text rendering
- [x] Add font-system link to docs/README.md index

---

## Phase 4: Input System

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

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/input-system.md` documenting mouse, keyboard, text input, and shortcuts
- [x] Add input-system link to docs/README.md index

---

## Phase 5: ID System & State

Widget identity, hashing, and state management.

### ID Hashing
- [x] Implement ID hashing algorithm (CRC32 or similar)
- [x] Implement `##` separator parsing for hidden IDs
- [x] Implement `###` separator for stable IDs

### ID Stack
- [x] Implement eli_push_id()
- [x] Implement eli_push_id_str()
- [x] Implement eli_push_id_ptr()
- [x] Implement eli_push_id_int()
- [x] Implement eli_pop_id()
- [x] Implement eli_get_id()
- [x] Implement eli_get_id_str()
- [x] Implement eli_get_id_ptr()
- [x] Implement eli_get_id_int()

### Active/Hot ID
- [x] Implement active_id tracking
- [x] Implement hot_id tracking
- [x] Implement eli_set_active_id() (internal)
- [x] Implement eli_clear_active_id() (internal)

### Storage
- [x] Define eli_storage structure
- [x] Implement eli_storage operations (get/set int/float/ptr)
- [x] Implement eli_set_state_storage()
- [x] Implement eli_get_state_storage()

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/id-system.md` documenting ID hashing, ID stack, and state storage
- [x] Add id-system link to docs/README.md index

---

## Phase 6: Style System

Colors, sizing, and theming.

### Style Structure
- [x] Create `eli_style.h`
- [x] Implement full eli_style structure (60+ properties)
- [x] Define all ELI_COL_* indices
- [x] Define all ELI_STYLE_VAR_* indices

### Style Functions
- [x] Implement eli_get_style()
- [x] Implement eli_style_colors_dark()
- [x] Implement eli_style_colors_light()
- [x] Implement eli_style_colors_classic()

### Style Stack
- [x] Implement eli_push_style_color()
- [x] Implement eli_push_style_color_vec4()
- [x] Implement eli_pop_style_color()
- [x] Implement eli_push_style_var()
- [x] Implement eli_push_style_var_vec2()
- [x] Implement eli_pop_style_var()

### Item Flags
- [x] Implement eli_push_item_flag()
- [x] Implement eli_pop_item_flag()

### Color Utilities
- [x] Implement eli_get_color_u32()
- [x] Implement eli_get_color_u32_vec4()
- [x] Implement eli_get_color_u32_col32()
- [x] Implement eli_get_style_color_vec4()
- [x] Implement eli_get_style_color_name()
- [x] Implement eli_color_convert_u32_to_float4()
- [x] Implement eli_color_convert_float4_to_u32()
- [x] Implement eli_color_convert_rgb_to_hsv()
- [x] Implement eli_color_convert_hsv_to_rgb()

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/style-system.md` documenting styles, colors, theming, and color utilities
- [x] Add style-system link to docs/README.md index

---

## Phase 7: Windows

Window management and rendering.

### Window Structure
- [x] Define eli_window structure
- [x] Implement window storage in context
- [x] Implement window lookup by name/ID

### Window Lifecycle
- [x] Implement eli_begin()
- [x] Implement eli_end()
- [x] Implement window creation/retrieval
- [x] Implement window title bar rendering
- [x] Implement window background
- [x] Implement window border

### Window Interaction
- [x] Implement window move (drag title bar)
- [x] Implement window resize (drag edges/corners)
- [x] Implement window focus/z-order
- [x] Implement window collapse
- [x] Implement all window flags

### Window State Queries
- [x] Implement eli_is_window_appearing()
- [x] Implement eli_is_window_collapsed()
- [x] Implement eli_is_window_focused()
- [x] Implement eli_is_window_hovered()
- [x] Implement eli_get_window_draw_list()
- [x] Implement eli_get_window_pos()
- [x] Implement eli_get_window_size()
- [x] Implement eli_get_window_width()
- [x] Implement eli_get_window_height()

### Window Manipulation
- [x] Implement eli_set_next_window_pos()
- [x] Implement eli_set_next_window_size()
- [x] Implement eli_set_next_window_size_constraints()
- [x] Implement eli_set_next_window_content_size()
- [x] Implement eli_set_next_window_collapsed()
- [x] Implement eli_set_next_window_focus()
- [x] Implement eli_set_next_window_scroll()
- [x] Implement eli_set_next_window_bg_alpha()
- [x] Implement eli_set_window_pos() variants
- [x] Implement eli_set_window_size() variants
- [x] Implement eli_set_window_collapsed() variants
- [x] Implement eli_set_window_focus() variants
- [x] Implement eli_set_window_font_scale()

### Child Windows
- [x] Implement eli_begin_child()
- [x] Implement eli_begin_child_id()
- [x] Implement eli_end_child()

### Scrolling
- [x] Implement scroll state per window
- [x] Implement eli_get_scroll_x/y()
- [x] Implement eli_set_scroll_x/y()
- [x] Implement eli_get_scroll_max_x/y()
- [x] Implement eli_set_scroll_here_x/y()
- [x] Implement eli_set_scroll_from_pos_x/y()
- [x] Implement vertical scrollbar
- [x] Implement horizontal scrollbar
- [x] Implement mouse wheel scrolling

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/windows.md` documenting window lifecycle, flags, child windows, and scrolling
- [x] Add windows link to docs/README.md index

---

## Phase 8: Layout System

Positioning, sizing, and layout helpers.

### Cursor
- [x] Create `eli_layout.h`
- [x] Implement cursor position tracking
- [x] Implement eli_get_cursor_pos()
- [x] Implement eli_get_cursor_pos_x/y()
- [x] Implement eli_set_cursor_pos()
- [x] Implement eli_set_cursor_pos_x/y()
- [x] Implement eli_get_cursor_start_pos()
- [x] Implement eli_get_cursor_screen_pos()
- [x] Implement eli_set_cursor_screen_pos()

### Layout Helpers
- [x] Implement eli_separator()
- [x] Implement eli_same_line()
- [x] Implement eli_new_line()
- [x] Implement eli_spacing()
- [x] Implement eli_dummy()
- [x] Implement eli_indent()
- [x] Implement eli_unindent()
- [x] Implement eli_align_text_to_frame_padding()

### Groups
- [x] Implement eli_begin_group()
- [x] Implement eli_end_group()

### Content Region
- [x] Implement eli_get_content_region_avail()
- [x] Implement eli_get_content_region_max()
- [x] Implement eli_get_window_content_region_min()
- [x] Implement eli_get_window_content_region_max()

### Item Width
- [x] Implement item width stack
- [x] Implement eli_push_item_width()
- [x] Implement eli_pop_item_width()
- [x] Implement eli_set_next_item_width()
- [x] Implement eli_calc_item_width()

### Text Wrap
- [x] Implement eli_push_text_wrap_pos()
- [x] Implement eli_pop_text_wrap_pos()

### Sizing Helpers
- [x] Implement eli_get_text_line_height()
- [x] Implement eli_get_text_line_height_with_spacing()
- [x] Implement eli_get_frame_height()
- [x] Implement eli_get_frame_height_with_spacing()

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/layout-system.md` documenting cursor, layout helpers, groups, and sizing
- [x] Add layout-system link to docs/README.md index

---

## Phase 9: Basic Widgets

Text display and button widgets.

### Text Widgets
- [x] Create `eli_widgets.h`
- [x] Implement eli_text_unformatted()
- [x] Implement eli_text()
- [x] Implement eli_text_v()
- [x] Implement eli_text_colored()
- [x] Implement eli_text_colored_v()
- [x] Implement eli_text_disabled()
- [x] Implement eli_text_disabled_v()
- [x] Implement eli_text_wrapped()
- [x] Implement eli_text_wrapped_v()
- [x] Implement eli_label_text()
- [x] Implement eli_label_text_v()
- [x] Implement eli_bullet_text()
- [x] Implement eli_bullet_text_v()
- [x] Implement eli_separator_text()
- [x] Implement eli_bullet()

### Buttons
- [x] Implement eli_button()
- [x] Implement eli_button_ex()
- [x] Implement eli_small_button()
- [x] Implement eli_invisible_button()
- [x] Implement eli_arrow_button()
- [x] Implement button interaction (hover, active states)

### Checkboxes & Radio
- [x] Implement eli_checkbox()
- [x] Implement eli_checkbox_flags_int()
- [x] Implement eli_checkbox_flags_uint()
- [x] Implement eli_radio_button()
- [x] Implement eli_radio_button_int()

### Progress & Links
- [x] Implement eli_progress_bar()
- [x] Implement eli_text_link()
- [x] Implement eli_text_link_open_url()

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/basic-widgets.md` documenting text, buttons, checkboxes, radio, and progress
- [x] Add basic-widgets link to docs/README.md index

---

## Phase 10: Item Status Queries

Widget state queries (critical for composition).

- [x] Implement eli_is_item_hovered()
- [x] Implement eli_is_item_active()
- [x] Implement eli_is_item_focused()
- [x] Implement eli_is_item_clicked()
- [x] Implement eli_is_item_visible()
- [x] Implement eli_is_item_edited()
- [x] Implement eli_is_item_activated()
- [x] Implement eli_is_item_deactivated()
- [x] Implement eli_is_item_deactivated_after_edit()
- [x] Implement eli_is_item_toggled_open()
- [x] Implement eli_is_any_item_hovered()
- [x] Implement eli_is_any_item_active()
- [x] Implement eli_is_any_item_focused()
- [x] Implement eli_get_item_id()
- [x] Implement eli_get_item_rect_min()
- [x] Implement eli_get_item_rect_max()
- [x] Implement eli_get_item_rect_size()

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/item-status.md` documenting item state queries and rect helpers
- [x] Add item-status link to docs/README.md index

---

## Phase 11: Sliders & Drags

Value adjustment widgets.

### Slider Implementation
- [x] Implement slider behavior (internal)
- [x] Implement eli_slider_float()
- [x] Implement eli_slider_float2/3/4()
- [x] Implement eli_slider_angle()
- [x] Implement eli_slider_int()
- [x] Implement eli_slider_int2/3/4()
- [x] Implement eli_slider_scalar()
- [x] Implement eli_slider_scalar_n()

### Vertical Sliders
- [x] Implement eli_v_slider_float()
- [x] Implement eli_v_slider_int()
- [x] Implement eli_v_slider_scalar()

### Drag Implementation
- [x] Implement drag behavior (internal)
- [x] Implement eli_drag_float()
- [x] Implement eli_drag_float2/3/4()
- [x] Implement eli_drag_float_range2()
- [x] Implement eli_drag_int()
- [x] Implement eli_drag_int2/3/4()
- [x] Implement eli_drag_int_range2()
- [x] Implement eli_drag_scalar()
- [x] Implement eli_drag_scalar_n()

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/sliders-drags.md` documenting slider and drag widgets
- [x] Add sliders-drags link to docs/README.md index

---

## Phase 12: Input Widgets

Text and number input.

### Input Text Callback
- [x] Define eli_input_text_callback_data structure
- [x] Define eli_input_text_callback type

### Text Input
- [x] Implement eli_input_text()
- [x] Implement text cursor rendering
- [x] Implement text selection
- [x] Implement copy/paste (via JS interop)
- [x] Implement eli_input_text_multiline()
- [x] Implement eli_input_text_with_hint()

### Numeric Input
- [x] Implement eli_input_float()
- [x] Implement eli_input_float2/3/4()
- [x] Implement eli_input_int()
- [x] Implement eli_input_int2/3/4()
- [x] Implement eli_input_double()
- [x] Implement eli_input_scalar()
- [x] Implement eli_input_scalar_n()

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/input-widgets.md` documenting text and numeric input widgets
- [x] Add input-widgets link to docs/README.md index

---

## Phase 13: Color Widgets

Color editing and picking.

- [x] Implement eli_color_edit3()
- [x] Implement eli_color_edit4()
- [x] Implement eli_color_picker3()
- [x] Implement eli_color_picker4()
- [x] Implement eli_color_button()
- [x] Implement eli_set_color_edit_options()
- [x] Implement color preview square
- [x] Implement hue bar
- [x] Implement saturation/value square
- [x] Implement alpha bar

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/color-widgets.md` documenting color edit, picker, and button widgets
- [x] Add color-widgets link to docs/README.md index

---

## Phase 14: Combo & Selectable

Dropdown and selection widgets.

### Combo
- [x] Implement eli_begin_combo()
- [x] Implement eli_end_combo()
- [x] Implement eli_combo()
- [x] Implement eli_combo_str()
- [x] Implement eli_combo_fn()

### Selectable
- [x] Implement eli_selectable()
- [x] Implement eli_selectable_bool()

### List Box
- [x] Implement eli_begin_list_box()
- [x] Implement eli_end_list_box()
- [x] Implement eli_list_box()
- [x] Implement eli_list_box_fn()

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/combo-selectable.md` documenting combo, selectable, and list box widgets
- [x] Add combo-selectable link to docs/README.md index

---

## Phase 15: Trees & Collapsing

Hierarchical widgets.

- [x] Implement eli_tree_node()
- [x] Implement eli_tree_node_str()
- [x] Implement eli_tree_node_ptr()
- [x] Implement eli_tree_node_v()
- [x] Implement eli_tree_node_ex()
- [x] Implement eli_tree_node_ex_str()
- [x] Implement eli_tree_node_ex_ptr()
- [x] Implement eli_tree_node_ex_v()
- [x] Implement eli_tree_push()
- [x] Implement eli_tree_push_ptr()
- [x] Implement eli_tree_pop()
- [x] Implement eli_get_tree_node_to_label_spacing()
- [x] Implement eli_collapsing_header()
- [x] Implement eli_collapsing_header_bool()
- [x] Implement eli_set_next_item_open()
- [x] Implement eli_set_next_item_storage_id()
- [x] Implement tree indentation
- [x] Implement tree arrow rendering

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/trees-collapsing.md` documenting tree nodes and collapsing headers
- [x] Add trees-collapsing link to docs/README.md index

---

## Phase 16: Menus

Menu bar and menu items.

### Menu Bar
- [x] Implement eli_begin_menu_bar()
- [x] Implement eli_end_menu_bar()
- [x] Implement eli_begin_main_menu_bar()
- [x] Implement eli_end_main_menu_bar()

### Menus
- [x] Implement eli_begin_menu()
- [x] Implement eli_end_menu()
- [x] Implement eli_menu_item()
- [x] Implement eli_menu_item_bool()
- [x] Implement menu item shortcut display
- [x] Implement submenus

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/menus.md` documenting menu bar and menu items
- [x] Add menus link to docs/README.md index

---

## Phase 17: Popups & Modals

Popup windows and modal dialogs.

### Popup Stack
- [x] Implement popup stack management
- [x] Implement eli_open_popup()
- [x] Implement eli_open_popup_id()
- [x] Implement eli_open_popup_on_item_click()
- [x] Implement eli_close_current_popup()
- [x] Implement eli_is_popup_open()

### Popup Windows
- [x] Implement eli_begin_popup()
- [x] Implement eli_end_popup()
- [x] Implement eli_begin_popup_context_item()
- [x] Implement eli_begin_popup_context_window()
- [x] Implement eli_begin_popup_context_void()

### Modal Dialogs
- [x] Implement eli_begin_popup_modal()
- [x] Implement modal backdrop rendering

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/popups-modals.md` documenting popups, context menus, and modals
- [x] Add popups-modals link to docs/README.md index

---

## Phase 18: Tooltips

Hover tooltips.

- [x] Implement eli_begin_tooltip()
- [x] Implement eli_end_tooltip()
- [x] Implement eli_set_tooltip()
- [x] Implement eli_set_tooltip_v()
- [x] Implement eli_begin_item_tooltip()
- [x] Implement eli_set_item_tooltip()
- [x] Implement eli_set_item_tooltip_v()
- [x] Implement hover delay for tooltips

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/tooltips.md` documenting tooltip widgets
- [x] Add tooltips link to docs/README.md index

---

## Phase 19: Tables

Full table widget system.

### Table Structure
- [x] Create `eli_tables.h`
- [x] Define table internal structures
- [x] Define eli_table_sort_specs structure
- [x] Define eli_table_column_sort_specs structure

### Table Lifecycle
- [x] Implement eli_begin_table()
- [x] Implement eli_end_table()
- [x] Implement eli_table_next_row()
- [x] Implement eli_table_next_column()
- [x] Implement eli_table_set_column_index()

### Table Setup
- [x] Implement eli_table_setup_column()
- [x] Implement eli_table_setup_scroll_freeze()
- [x] Implement eli_table_header()
- [x] Implement eli_table_headers_row()
- [x] Implement eli_table_angled_headers_row()

### Table Queries
- [x] Implement eli_table_get_sort_specs()
- [x] Implement eli_table_get_column_count()
- [x] Implement eli_table_get_column_index()
- [x] Implement eli_table_get_row_index()
- [x] Implement eli_table_get_column_name()
- [x] Implement eli_table_get_column_flags()
- [x] Implement eli_table_set_column_enabled()
- [x] Implement eli_table_get_hovered_column()
- [x] Implement eli_table_set_bg_color()

### Table Features
- [x] Implement column resizing
- [x] Implement column reordering
- [x] Implement column hiding
- [x] Implement column sorting
- [x] Implement table scrolling
- [x] Implement row backgrounds

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/tables.md` documenting table widget, columns, sorting, and features
- [x] Add tables link to docs/README.md index

---

## Phase 20: Tab Bars

Tab bar widget.

- [x] Create `eli_tabs.h`
- [x] Implement eli_begin_tab_bar()
- [x] Implement eli_end_tab_bar()
- [x] Implement eli_begin_tab_item()
- [x] Implement eli_end_tab_item()
- [x] Implement eli_tab_item_button()
- [x] Implement eli_set_tab_item_closed()
- [x] Implement tab reordering
- [x] Implement tab scrolling
- [x] Implement tab close button

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/tab-bars.md` documenting tab bar and tab item widgets
- [x] Add tab-bars link to docs/README.md index

---

## Phase 21: Drag & Drop

Drag and drop system.

### Payload
- [x] Define eli_payload structure
- [x] Implement payload storage

### Source
- [x] Implement eli_begin_drag_drop_source()
- [x] Implement eli_set_drag_drop_payload()
- [x] Implement eli_end_drag_drop_source()

### Target
- [x] Implement eli_begin_drag_drop_target()
- [x] Implement eli_accept_drag_drop_payload()
- [x] Implement eli_end_drag_drop_target()
- [x] Implement eli_get_drag_drop_payload()

### Visual Feedback
- [x] Implement drag preview
- [x] Implement drop target highlight

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/drag-drop.md` documenting drag and drop system
- [x] Add drag-drop link to docs/README.md index

---

## Phase 22: Images

Image display widgets.

- [x] Implement eli_image()
- [x] Implement eli_image_button()
- [x] Implement eli_draw_list_add_image()
- [x] Implement eli_draw_list_add_image_quad()
- [x] Implement eli_draw_list_add_image_rounded()

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/images.md` documenting image display widgets
- [x] Add images link to docs/README.md index

---

## Phase 23: Data Plotting

Simple plotting widgets.

- [x] Implement eli_plot_lines()
- [x] Implement eli_plot_lines_fn()
- [x] Implement eli_plot_histogram()
- [x] Implement eli_plot_histogram_fn()

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/plotting.md` documenting plot lines and histogram widgets
- [x] Add plotting link to docs/README.md index

---

## Phase 24: Value Display

Simple value display widgets.

- [x] Implement eli_value_bool()
- [x] Implement eli_value_int()
- [x] Implement eli_value_uint()
- [x] Implement eli_value_float()

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/value-display.md` documenting value display widgets
- [x] Add value-display link to docs/README.md index

---

## Phase 25: Disabling & Clipping

Widget disabling and clipping.

### Disabling
- [x] Implement eli_begin_disabled()
- [x] Implement eli_end_disabled()

### Clipping
- [x] Implement eli_push_clip_rect()
- [x] Implement eli_pop_clip_rect()

### Focus
- [x] Implement eli_set_item_default_focus()
- [x] Implement eli_set_keyboard_focus_here()

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/disabling-clipping.md` documenting disabled state, clipping, and focus
- [x] Add disabling-clipping link to docs/README.md index

---

## Phase 26: List Clipper

Efficient list rendering.

- [x] Define eli_list_clipper structure
- [x] Implement eli_list_clipper_begin()
- [x] Implement eli_list_clipper_end()
- [x] Implement eli_list_clipper_step()
- [x] Implement eli_list_clipper_include_item_by_index()
- [x] Implement eli_list_clipper_include_items_by_index()
- [x] Implement eli_list_clipper_seek_cursor_for_item()

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/list-clipper.md` documenting list clipper for efficient rendering
- [x] Add list-clipper link to docs/README.md index

---

## Phase 27: Misc Utilities

Remaining utility functions.

### Visibility
- [x] Implement eli_is_rect_visible()
- [x] Implement eli_is_rect_visible_vec2()

### Time & Frame
- [x] Implement eli_get_time()
- [x] Implement eli_get_frame_count()

### Viewports
- [x] Define eli_viewport structure
- [x] Implement eli_get_main_viewport()

### Draw Lists
- [x] Implement eli_get_background_draw_list()
- [x] Implement eli_get_foreground_draw_list()

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/utilities.md` documenting visibility, time, viewports, and draw lists
- [x] Add utilities link to docs/README.md index

---

## Phase 28: Settings & Logging

Configuration persistence and logging.

### Settings
- [x] Implement eli_load_ini_settings_from_disk()
- [x] Implement eli_load_ini_settings_from_memory()
- [x] Implement eli_save_ini_settings_to_disk()
- [x] Implement eli_save_ini_settings_to_memory()

### Logging
- [x] Implement eli_log_to_tty()
- [x] Implement eli_log_to_file()
- [x] Implement eli_log_to_clipboard()
- [x] Implement eli_log_finish()
- [x] Implement eli_log_buttons()
- [x] Implement eli_log_text()
- [x] Implement eli_log_text_v()

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/settings-logging.md` documenting INI settings and logging
- [x] Add settings-logging link to docs/README.md index

---

## Phase 29: Memory Management

Custom allocators.

- [x] Implement eli_set_allocator_functions()
- [x] Implement eli_get_allocator_functions()
- [x] Implement eli_mem_alloc()
- [x] Implement eli_mem_free()

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/memory.md` documenting custom allocators and memory management
- [x] Add memory link to docs/README.md index

---

## Phase 30: Demo & Debug Windows

Demo application and debug tools.

### Demo Window
- [x] Implement eli_show_demo_window()
- [x] Demo: Basic widgets section
- [x] Demo: Layout section
- [x] Demo: Input widgets section
- [x] Demo: Sliders & drags section
- [x] Demo: Color widgets section
- [x] Demo: Trees and collapsing section
- [x] Demo: Tables section
- [x] Demo: Tabs section
- [x] Demo: Popups section
- [x] Demo: Drag & drop section
- [x] Demo: Style editor section

### Debug Windows
- [x] Implement eli_show_metrics_window()
- [x] Implement eli_show_debug_log_window()
- [x] Implement eli_show_id_stack_tool_window()
- [x] Implement eli_show_about_window()
- [x] Implement eli_show_style_editor()
- [x] Implement eli_show_style_selector()
- [x] Implement eli_show_font_selector()
- [x] Implement eli_show_user_guide()
- [x] Implement eli_get_version()

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/demo-debug.md` documenting demo window and debug tools
- [x] Add demo-debug link to docs/README.md index

---

## Phase 31: Web Integration

WASM loader and browser integration.

- [x] Create web/elimgui.js WASM loader
- [x] Create web/index.html demo shell
- [x] Implement JS event handlers (mouse, keyboard)
- [x] Implement Canvas2D renderer backend
- [x] Implement WebGL renderer backend (optional)

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/web-integration.md` documenting WASM loader, JS events, and renderers
- [x] Add web-integration link to docs/README.md index

---

## Phase 32: Testing

Test coverage.

- [x] Create test framework
- [x] Test: Core types and math
- [x] Test: ID hashing
- [x] Test: Draw list generation
- [x] Test: Layout calculations
- [x] Test: Input state
- [x] Test: Window management
- [x] Test: Widget behavior
- [x] Test: Table functionality

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/testing.md` documenting test framework and coverage
- [x] Add testing link to docs/README.md index

---

## Phase 33: Optimization

Performance improvements.

- [x] Profile draw list generation
- [x] Optimize vertex buffer growth
- [x] Implement draw call batching
- [x] Reduce allocations per frame
- [x] Measure and optimize WASM size

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/optimization.md` documenting performance tips and benchmarks
- [x] Add optimization link to docs/README.md index

---

## Future: Docking (Phase 34+)

Window docking system.

- [x] Create `eli_docking.h`
- [x] Implement dock node tree
- [x] Implement window docking
- [x] Implement dock splitting
- [x] Implement tab bar for docked windows
- [x] Implement dock space

### Tests
- [x] Write native unit tests in `tests/unit/` covering this phase's logic (use `tests/eli_test.h`)
- [x] `./build.sh test` passes green

### Documentation
- [x] Create `docs/docking.md` documenting docking system
- [x] Add docking link to docs/README.md index
