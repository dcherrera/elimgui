# Draw System

This document covers the draw list architecture, primitive rendering, and how to work with elimgui's rendering output.

## Table of Contents

- [Overview](#overview)
- [Draw Types](#draw-types)
  - [eli_draw_vert](#eli_draw_vert)
  - [eli_draw_idx](#eli_draw_idx)
  - [eli_draw_cmd](#eli_draw_cmd)
- [Draw List](#draw-list)
  - [Initialization](#initialization)
  - [Buffer Management](#buffer-management)
- [Draw Data](#draw-data)
- [Clip Rect Stack](#clip-rect-stack)
- [Texture Stack](#texture-stack)
- [Path API](#path-api)
  - [Arc Functions](#arc-functions)
  - [Bezier Curves](#bezier-curves-path)
- [Primitives](#primitives)
  - [Lines](#lines)
  - [Rectangles](#rectangles)
  - [Triangles and Quads](#triangles-and-quads)
  - [Circles and Ellipses](#circles-and-ellipses)
  - [Bezier Curves](#bezier-curves)
  - [Polylines](#polylines)
  - [Convex Polygons](#convex-polygons)
- [Draw List Channels](#draw-list-channels)
- [Callbacks](#callbacks)
- [Draw Flags](#draw-flags)
- [Rendering the Output](#rendering-the-output)

---

## Overview

The draw system is the core rendering backend of elimgui. It produces batched draw commands that your renderer consumes.

**Key concepts:**
- **Draw List** - Collects vertices, indices, and commands for a single layer
- **Draw Data** - Contains all draw lists to render for a frame
- **Draw Command** - A single batched draw call with clip rect, texture, and index range

**Data flow:**
```
Widget calls → Path/Primitive API → Draw List buffers → Draw Data → Your renderer
```

---

## Draw Types

### eli_draw_vert

The vertex format used by all primitives. 20 bytes per vertex.

```c
typedef struct eli_draw_vert {
    eli_vec2 pos;   // Position (8 bytes)
    eli_vec2 uv;    // Texture coordinate (8 bytes)
    uint32_t col;   // Packed RGBA color (4 bytes)
} eli_draw_vert;
```

**Layout offsets (for renderer setup):**
```c
ELI_DRAW_VERT_POS_OFF  // 0
ELI_DRAW_VERT_UV_OFF   // 8
ELI_DRAW_VERT_COL_OFF  // 16
ELI_DRAW_VERT_SIZE     // 20
```

### eli_draw_idx

Index type for draw calls. 16-bit for WebGL compatibility.

```c
typedef uint16_t eli_draw_idx;
```

**Note:** With 16-bit indices, each draw command can reference up to 65535 vertices. The draw list automatically handles overflow by creating new commands with vertex offsets.

### eli_draw_cmd

A single draw command representing a batched draw call.

```c
typedef struct eli_draw_cmd {
    eli_vec4 clip_rect;       // Clipping rectangle (x1, y1, x2, y2)
    eli_texture_id texture_id; // Texture to bind
    uint32_t vtx_offset;      // Offset in vertex buffer
    uint32_t idx_offset;      // Offset in index buffer
    uint32_t elem_count;      // Number of indices (triangles * 3)
    eli_draw_callback user_callback;
    void* user_callback_data;
} eli_draw_cmd;
```

---

## Draw List

The `eli_draw_list` structure holds all the buffers and state for rendering.

```c
struct eli_draw_list {
    // Output buffers
    eli_draw_cmd_array cmd_buffer;
    eli_draw_vert_array vtx_buffer;
    eli_draw_idx_array idx_buffer;

    // Flags
    eli_draw_list_flags flags;

    // Path building
    eli_vec2_array _path;

    // Stacks
    eli_vec4_array _clip_rect_stack;
    eli_texture_array _texture_stack;

    // ... internal state
};
```

### Initialization

```c
eli_draw_list list;
eli_draw_list_init(&list);

// ... use the list ...

eli_draw_list_destroy(&list);
```

### Buffer Management

```c
// Clear for new frame (keeps allocated memory)
eli_draw_list_clear(&list);

// Reserve space for primitives
eli_draw_list_prim_reserve(&list, index_count, vertex_count);

// Unreserve (rollback)
eli_draw_list_prim_unreserve(&list, index_count, vertex_count);
```

---

## Draw Data

The `eli_draw_data` structure contains all draw lists for a frame.

```c
struct eli_draw_data {
    bool valid;
    int cmd_lists_count;
    int total_idx_count;
    int total_vtx_count;
    eli_draw_list_ptr_array cmd_lists;
    eli_vec2 display_pos;
    eli_vec2 display_size;
    eli_vec2 framebuffer_scale;
};
```

**Usage:**
```c
eli_render();
eli_draw_data* draw_data = eli_get_draw_data();

if (draw_data->valid) {
    for (int n = 0; n < draw_data->cmd_lists_count; n++) {
        eli_draw_list* list = draw_data->cmd_lists.data[n];
        // Render list...
    }
}
```

---

## Clip Rect Stack

Push and pop clipping rectangles to constrain rendering.

```c
// Push a clip rect (intersects with current by default)
eli_draw_list_push_clip_rect(&list,
    eli_make_vec2(100, 100),  // min
    eli_make_vec2(400, 300),  // max
    true);                     // intersect with current

// Push full screen (no clipping)
eli_draw_list_push_clip_rect_full_screen(&list);

// Pop back to previous
eli_draw_list_pop_clip_rect(&list);

// Get current clip rect bounds
eli_vec2 min = eli_draw_list_get_clip_rect_min(&list);
eli_vec2 max = eli_draw_list_get_clip_rect_max(&list);
```

---

## Texture Stack

Push and pop textures for textured rendering.

```c
// Push a texture
eli_draw_list_push_texture_id(&list, my_texture);

// ... draw textured primitives ...

// Pop back to previous
eli_draw_list_pop_texture_id(&list);
```

---

## Path API

Build complex shapes by defining paths, then stroke or fill them.

```c
// Clear path
eli_draw_list_path_clear(&list);

// Add points
eli_draw_list_path_line_to(&list, eli_make_vec2(100, 100));
eli_draw_list_path_line_to(&list, eli_make_vec2(200, 100));
eli_draw_list_path_line_to(&list, eli_make_vec2(150, 200));

// Stroke (outline)
eli_draw_list_path_stroke(&list, ELI_COL32_WHITE, ELI_DRAW_FLAGS_CLOSED, 2.0f);

// Or fill (solid)
eli_draw_list_path_fill_convex(&list, ELI_COL32_WHITE);
```

**Path functions:**
```c
void eli_draw_list_path_clear(eli_draw_list* list);
void eli_draw_list_path_line_to(eli_draw_list* list, eli_vec2 pos);
void eli_draw_list_path_line_to_merge_duplicate(eli_draw_list* list, eli_vec2 pos);
void eli_draw_list_path_rect(eli_draw_list* list, eli_vec2 min, eli_vec2 max,
                              float rounding, eli_draw_flags flags);
void eli_draw_list_path_stroke(eli_draw_list* list, uint32_t col,
                                eli_draw_flags flags, float thickness);
void eli_draw_list_path_fill_convex(eli_draw_list* list, uint32_t col);
```

### Arc Functions

Add arc segments to the current path.

```c
// Arc with angle range (radians)
eli_draw_list_path_arc_to(&list,
    eli_make_vec2(100, 100),  // center
    50.0f,                     // radius
    0.0f,                      // a_min (start angle)
    ELI_PI * 0.5f,            // a_max (end angle)
    0);                        // segments (0 = auto)

// Fast arc using precomputed lookup (indices are multiples of 12)
// 0 = 0°, 3 = 90°, 6 = 180°, 9 = 270°, 12 = 360°
eli_draw_list_path_arc_to_fast(&list,
    eli_make_vec2(100, 100),
    50.0f,
    0,   // a_min_of_12 (0 = East)
    3);  // a_max_of_12 (3 = South, quarter circle)

// Elliptical arc with rotation
eli_draw_list_path_elliptical_arc_to(&list,
    eli_make_vec2(100, 100),       // center
    eli_make_vec2(80, 40),         // radius (x, y)
    0.5f,                          // rotation (radians)
    0.0f,                          // a_min
    ELI_PI,                        // a_max
    0);                            // segments (0 = auto)
```

### Bezier Curves (Path)

Add bezier curve segments to the current path.

```c
// Cubic bezier (4 control points: current + p2, p3, p4)
eli_draw_list_path_line_to(&list, eli_make_vec2(10, 100));  // Start point
eli_draw_list_path_bezier_cubic_curve_to(&list,
    eli_make_vec2(40, 10),    // p2 (control point 1)
    eli_make_vec2(160, 10),   // p3 (control point 2)
    eli_make_vec2(190, 100),  // p4 (end point)
    0);                        // segments (0 = auto)
eli_draw_list_path_stroke(&list, ELI_COL32_WHITE, ELI_DRAW_FLAGS_NONE, 2.0f);

// Quadratic bezier (3 control points: current + p2, p3)
eli_draw_list_path_line_to(&list, eli_make_vec2(10, 100));
eli_draw_list_path_bezier_quadratic_curve_to(&list,
    eli_make_vec2(100, 10),   // p2 (control point)
    eli_make_vec2(190, 100),  // p3 (end point)
    0);                        // segments (0 = auto)
eli_draw_list_path_stroke(&list, ELI_COL32_WHITE, ELI_DRAW_FLAGS_NONE, 2.0f);
```

---

## Primitives

### Lines

```c
eli_draw_list_add_line(&list,
    eli_make_vec2(10, 10),   // p1
    eli_make_vec2(100, 50),  // p2
    ELI_COL32(255, 0, 0, 255), // color
    2.0f);                    // thickness
```

### Rectangles

```c
// Outline
eli_draw_list_add_rect(&list,
    eli_make_vec2(10, 10),    // min
    eli_make_vec2(110, 60),   // max
    ELI_COL32_WHITE,          // color
    5.0f,                     // rounding
    ELI_DRAW_FLAGS_NONE,      // flags
    1.0f);                    // thickness

// Filled
eli_draw_list_add_rect_filled(&list,
    eli_make_vec2(10, 10),
    eli_make_vec2(110, 60),
    ELI_COL32(100, 150, 200, 255),
    5.0f,                     // rounding
    ELI_DRAW_FLAGS_NONE);

// Gradient (per-corner colors)
eli_draw_list_add_rect_filled_multi_color(&list,
    eli_make_vec2(10, 10),
    eli_make_vec2(110, 60),
    ELI_COL32(255, 0, 0, 255),   // top-left
    ELI_COL32(0, 255, 0, 255),   // top-right
    ELI_COL32(0, 0, 255, 255),   // bottom-right
    ELI_COL32(255, 255, 0, 255)); // bottom-left
```

### Triangles and Quads

```c
// Triangle outline
eli_draw_list_add_triangle(&list,
    eli_make_vec2(50, 10),
    eli_make_vec2(90, 80),
    eli_make_vec2(10, 80),
    ELI_COL32_WHITE, 1.0f);

// Triangle filled
eli_draw_list_add_triangle_filled(&list,
    eli_make_vec2(50, 10),
    eli_make_vec2(90, 80),
    eli_make_vec2(10, 80),
    ELI_COL32(255, 128, 0, 255));

// Quad outline
eli_draw_list_add_quad(&list, p1, p2, p3, p4, ELI_COL32_WHITE, 1.0f);

// Quad filled
eli_draw_list_add_quad_filled(&list, p1, p2, p3, p4, ELI_COL32_WHITE);
```

### Circles and Ellipses

```c
// Circle outline
eli_draw_list_add_circle(&list,
    eli_make_vec2(100, 100),  // center
    50.0f,                     // radius
    ELI_COL32_WHITE,          // color
    0,                         // segments (0 = auto)
    2.0f);                    // thickness

// Circle filled
eli_draw_list_add_circle_filled(&list,
    eli_make_vec2(100, 100),
    50.0f,
    ELI_COL32(255, 200, 100, 255),
    0);                        // segments (0 = auto)

// Ngon (explicit segment count)
eli_draw_list_add_ngon(&list, center, radius, color, 6, thickness);  // hexagon
eli_draw_list_add_ngon_filled(&list, center, radius, color, 8);      // octagon

// Ellipse outline
eli_draw_list_add_ellipse(&list,
    eli_make_vec2(100, 100),       // center
    eli_make_vec2(80, 40),         // radius (x, y)
    ELI_COL32_WHITE,
    0.0f,                          // rotation (radians)
    0,                             // segments (0 = auto)
    2.0f);                         // thickness

// Ellipse filled
eli_draw_list_add_ellipse_filled(&list,
    eli_make_vec2(100, 100),
    eli_make_vec2(80, 40),
    ELI_COL32(100, 200, 255, 255),
    0.5f,                          // rotation (radians)
    0);
```

### Bezier Curves

Draw bezier curves directly (without using path API).

```c
// Cubic bezier curve
eli_draw_list_add_bezier_cubic(&list,
    eli_make_vec2(10, 100),   // p1 (start)
    eli_make_vec2(40, 10),    // p2 (control 1)
    eli_make_vec2(160, 10),   // p3 (control 2)
    eli_make_vec2(190, 100),  // p4 (end)
    ELI_COL32_WHITE,          // color
    2.0f,                     // thickness
    0);                       // segments (0 = auto)

// Quadratic bezier curve
eli_draw_list_add_bezier_quadratic(&list,
    eli_make_vec2(10, 100),   // p1 (start)
    eli_make_vec2(100, 10),   // p2 (control)
    eli_make_vec2(190, 100),  // p3 (end)
    ELI_COL32_WHITE,
    2.0f,
    0);
```

**Bezier helper functions:**
```c
// Calculate point on cubic bezier at t (0.0 to 1.0)
eli_vec2 p = eli_bezier_cubic_calc(p1, p2, p3, p4, t);

// Calculate point on quadratic bezier at t
eli_vec2 p = eli_bezier_quadratic_calc(p1, p2, p3, t);
```

### Polylines

```c
eli_vec2 points[] = {
    {10, 10}, {50, 30}, {80, 10}, {100, 50}
};
int count = sizeof(points) / sizeof(points[0]);

// Open polyline
eli_draw_list_add_polyline(&list, points, count,
    ELI_COL32_WHITE,
    ELI_DRAW_FLAGS_NONE,
    2.0f);

// Closed polyline
eli_draw_list_add_polyline(&list, points, count,
    ELI_COL32_WHITE,
    ELI_DRAW_FLAGS_CLOSED,
    2.0f);
```

### Convex Polygons

```c
eli_vec2 points[] = {
    {50, 10}, {90, 30}, {80, 80}, {20, 80}, {10, 30}
};
int count = sizeof(points) / sizeof(points[0]);

eli_draw_list_add_convex_poly_filled(&list, points, count, ELI_COL32_WHITE);
```

---

## Draw List Channels

Channels allow you to split a draw list into multiple independent streams, then merge them back in a different order. This is useful for rendering items in layers (e.g., background, content, foreground).

### Splitter Structure

```c
typedef struct eli_draw_list_splitter {
    int _current;
    int _count;
    eli_draw_channel_array _channels;
} eli_draw_list_splitter;
```

### Usage

```c
eli_draw_list_splitter splitter;
eli_draw_list_splitter_init(&splitter);

// Split into 3 channels
eli_draw_list_splitter_split(&splitter, &list, 3);

// Draw to channel 0 (background)
eli_draw_list_splitter_set_current_channel(&splitter, &list, 0);
eli_draw_list_add_rect_filled(&list, ...);  // Background

// Draw to channel 2 (foreground)
eli_draw_list_splitter_set_current_channel(&splitter, &list, 2);
eli_draw_list_add_rect_filled(&list, ...);  // Foreground (on top)

// Draw to channel 1 (middle)
eli_draw_list_splitter_set_current_channel(&splitter, &list, 1);
eli_draw_list_add_rect_filled(&list, ...);  // Content

// Merge all channels (0, 1, 2 order)
eli_draw_list_splitter_merge(&splitter, &list);

eli_draw_list_splitter_destroy(&splitter);
```

### Convenience Functions

```c
// These wrap the splitter functions
eli_draw_list_channels_split(&list, &splitter, count);
eli_draw_list_channels_merge(&list, &splitter);
eli_draw_list_channels_set_current(&list, &splitter, channel_idx);
```

---

## Callbacks

Add custom callbacks into the command stream for special rendering operations.

```c
void my_render_callback(const eli_draw_list* list, const eli_draw_cmd* cmd) {
    // Custom rendering code
    // cmd->user_callback_data contains your data
}

// Add callback to command stream
eli_draw_list_add_callback(&list, my_render_callback, my_data);

// Special callback to reset render state
eli_draw_list_add_callback(&list, ELI_DRAW_CALLBACK_RESET_RENDER_STATE, NULL);
```

**Clone a draw list:**
```c
eli_draw_list src, dst;
eli_draw_list_init(&src);
eli_draw_list_init(&dst);

// ... draw to src ...

// Clone src to dst
eli_draw_list_clone_output(&src, &dst);
```

---

## Draw Flags

### eli_draw_flags

Used for line and rectangle rendering options.

```c
ELI_DRAW_FLAGS_NONE                        // Default
ELI_DRAW_FLAGS_CLOSED                      // Close polyline/path

// Round corner flags
ELI_DRAW_FLAGS_ROUND_CORNERS_TOP_LEFT
ELI_DRAW_FLAGS_ROUND_CORNERS_TOP_RIGHT
ELI_DRAW_FLAGS_ROUND_CORNERS_BOTTOM_LEFT
ELI_DRAW_FLAGS_ROUND_CORNERS_BOTTOM_RIGHT
ELI_DRAW_FLAGS_ROUND_CORNERS_NONE

// Convenience combinations
ELI_DRAW_FLAGS_ROUND_CORNERS_TOP           // Top-left + Top-right
ELI_DRAW_FLAGS_ROUND_CORNERS_BOTTOM        // Bottom-left + Bottom-right
ELI_DRAW_FLAGS_ROUND_CORNERS_LEFT          // Top-left + Bottom-left
ELI_DRAW_FLAGS_ROUND_CORNERS_RIGHT         // Top-right + Bottom-right
ELI_DRAW_FLAGS_ROUND_CORNERS_ALL           // All corners
ELI_DRAW_FLAGS_ROUND_CORNERS_DEFAULT       // Same as ALL
```

### eli_draw_list_flags

Control anti-aliasing and vertex offset behavior.

```c
ELI_DRAW_LIST_FLAGS_NONE
ELI_DRAW_LIST_FLAGS_ANTI_ALIASED_LINES
ELI_DRAW_LIST_FLAGS_ANTI_ALIASED_LINES_USE_TEX
ELI_DRAW_LIST_FLAGS_ANTI_ALIASED_FILL
ELI_DRAW_LIST_FLAGS_ALLOW_VTX_OFFSET
```

---

## Rendering the Output

After calling `eli_render()`, retrieve the draw data and render it.

**Example WebGL renderer loop:**
```c
eli_render();
eli_draw_data* draw_data = eli_get_draw_data();

if (!draw_data->valid) return;

// Set up projection matrix from display_pos/display_size
// ...

for (int n = 0; n < draw_data->cmd_lists_count; n++) {
    eli_draw_list* list = draw_data->cmd_lists.data[n];

    // Upload vertex/index buffers
    // glBufferData(GL_ARRAY_BUFFER, list->vtx_buffer.size * sizeof(eli_draw_vert), ...);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, list->idx_buffer.size * sizeof(eli_draw_idx), ...);

    for (int cmd_i = 0; cmd_i < list->cmd_buffer.size; cmd_i++) {
        eli_draw_cmd* cmd = &list->cmd_buffer.data[cmd_i];

        if (cmd->user_callback) {
            cmd->user_callback(list, cmd);
        } else {
            // Set clip rect (scissor test)
            // Bind texture (cmd->texture_id)
            // Draw elements
            // glDrawElements(GL_TRIANGLES, cmd->elem_count, GL_UNSIGNED_SHORT,
            //                (void*)(cmd->idx_offset * sizeof(eli_draw_idx)));
        }
    }
}
```

---

## See Also

- [Core Types & Context](core-types.md)
- [API Quick Reference](README.md#api-quick-reference)
