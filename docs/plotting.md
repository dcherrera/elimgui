# Data Plotting — Phase 23

`include/eli/widgets/eli_plot.h`

Immediate-mode data-plotting widgets for rendering float arrays as line graphs
or vertical bar histograms inside a framed widget region. Both line and histogram
modes support automatic scale detection, explicit scale bounds, a centred overlay
label, and hover highlighting with an inline value tooltip near the cursor.

---

## Concepts

### Frame region

Each plot occupies a rectangular framed region whose pixel size is set by the
`graph_size` argument. The frame is drawn with `ELI_COL_FRAME_BG`. A 1 px inset
on every side defines the **inner plot area** where geometry is clipped.

### Scale

Pass `ELI_PLOT_SCALE_AUTO` for either `scale_min` or `scale_max` to have the
widget compute that bound from the data on every frame. Pass explicit floats to
fix the range and clamp out-of-range values to the nearest edge.

If `scale_min == scale_max` after resolution, the range is widened by 1 so that
a zero-range divide never occurs.

### Circular buffer access

All four functions accept a `values_offset` ring-buffer index. Sample `i` is
read at logical index `(values_offset + i) % values_count`. Pass `0` when
reading a plain contiguous array from the start.

### Hover highlight

When the mouse cursor is inside the inner plot area the widget identifies the
nearest sample (for lines: the segment start; for histograms: the bar under the
pointer) and redraws it with the hovered colour (`ELI_COL_PLOT_LINES_HOVERED` /
`ELI_COL_PLOT_HISTOGRAM_HOVERED`). An inline text label with the raw value(s) is
rendered 8 px to the right of the cursor via `eli_draw_list_add_text`.

### Cursor advance

After the call the layout cursor advances by `graph_size.y + item_spacing.y`,
exactly like any other framed widget.

---

## Constants

```c
#define ELI_PLOT_SCALE_AUTO  FLT_MAX   /* auto-detect this bound from the data */
```

---

## Line graph

### Array variant

```c
void eli_plot_lines(
    const char *label,          /* widget label; "##foo" hides the text     */
    const float *values,        /* sample array (non-NULL)                   */
    int          values_count,  /* number of samples (>= 2 for any lines)   */
    int          values_offset, /* ring-buffer start index                   */
    const char  *overlay_text,  /* text centred over the graph, or NULL      */
    float        scale_min,     /* y lower bound, or ELI_PLOT_SCALE_AUTO     */
    float        scale_max,     /* y upper bound, or ELI_PLOT_SCALE_AUTO     */
    eli_vec2     graph_size,    /* framed region size in pixels (0 → default)*/
    int          stride         /* byte stride between values (0 → sizeof(float)) */
);
```

Draws a polyline connecting all `values_count` samples. The first sample maps
to the left edge of the inner area, the last to the right edge; y-position
follows the scale mapping (low value = bottom, high value = top).

### Callback variant

```c
void eli_plot_lines_fn(
    const char *label,
    float (*values_getter)(void *data, int idx),
    void       *data,
    int         values_count,
    int         values_offset,
    const char *overlay_text,
    float       scale_min,
    float       scale_max,
    eli_vec2    graph_size
);
```

Same as the array variant but reads each sample via `values_getter(data, idx)`.
Useful for ring buffers, computed series, or non-contiguous layouts.

### Example

```c
static float g_samples[128];
static int   g_head = 0;

/* push a new reading each frame */
g_samples[g_head % 128] = read_sensor();
g_head++;

eli_plot_lines("Sensor", g_samples, 128, g_head,
               "sensor", 0.0f, 100.0f,
               eli_make_vec2(300.0f, 80.0f), 0);
```

---

## Histogram

### Array variant

```c
void eli_plot_histogram(
    const char *label,
    const float *values,
    int          values_count,
    int          values_offset,
    const char  *overlay_text,
    float        scale_min,
    float        scale_max,
    eli_vec2     graph_size,
    int          stride
);
```

Draws one vertical bar per sample. Each bar occupies `inner_width / N` pixels
horizontally. Bar height is proportional to the normalised value
(`(v - scale_min) / (scale_max - scale_min)`). Bars with a normalised value of
0 or below (y_top >= inner_max.y) are skipped and produce no geometry.

### Callback variant

```c
void eli_plot_histogram_fn(
    const char *label,
    float (*values_getter)(void *data, int idx),
    void       *data,
    int         values_count,
    int         values_offset,
    const char *overlay_text,
    float       scale_min,
    float       scale_max,
    eli_vec2    graph_size
);
```

Same as the array variant via a getter callback.

### Example

```c
float buckets[16];
compute_histogram(buckets, 16);

eli_plot_histogram("Distribution", buckets, 16, 0,
                   NULL,
                   ELI_PLOT_SCALE_AUTO, ELI_PLOT_SCALE_AUTO,
                   eli_make_vec2(200.0f, 100.0f), 0);
```

---

## Geometry emitted

Vertex counts assume no hover and `frame_border_size = 0` (the default).

| Plot type  | N samples | Frame bg      | Data geometry              | Total        |
|------------|-----------|---------------|----------------------------|--------------|
| Lines      | N         | 4 vtx / 6 idx | (N−1)×4 vtx / (N−1)×6 idx | 4+(N−1)×4 vtx |
| Histogram  | N (all visible) | 4 vtx / 6 idx | N×4 vtx / N×6 idx     | 4+N×4 vtx    |

A hovered segment (lines) adds 4 vtx / 6 idx for the overdraw pass.
A hovered bar (histogram) uses the hovered colour in place of normal; no extra geometry.

---

## Style colours used

| Slot                           | Usage                           |
|--------------------------------|---------------------------------|
| `ELI_COL_FRAME_BG`             | Frame background fill           |
| `ELI_COL_PLOT_LINES`           | Line graph colour               |
| `ELI_COL_PLOT_LINES_HOVERED`   | Hovered segment overdraw        |
| `ELI_COL_PLOT_HISTOGRAM`       | Bar fill colour                 |
| `ELI_COL_PLOT_HISTOGRAM_HOVERED` | Hovered bar fill              |
| `ELI_COL_TEXT`                 | Overlay text and hover label    |

Colors must be non-zero (non-transparent) for geometry to be emitted. Call
`eli_style_colors_dark()` or `eli_style_colors_light()` after
`eli_create_context()` to initialise them.

---

## Implementation notes

- **Header-only**: everything is `static inline` in `eli_plot.h`.
- **No hidden allocations**: `eli_plot__emit_lines` makes one `malloc` call for
  the `eli_vec2` point array, sized to `values_count * sizeof(eli_vec2)`, and
  frees it before returning.
- **Auto-scale walk**: `eli_plot__compute_scale` performs a single O(N) walk
  over all samples using the getter. Auto-scale is only computed when at least
  one of the two bounds is `ELI_PLOT_SCALE_AUTO`.
- **Circular access**: all sample reads go through
  `getter(data, (values_offset + i) % values_count)` so the ring-buffer
  discipline is enforced uniformly.
- **Y mapping**: `value = scale_min` → `y = inner_max.y` (bottom of inner area);
  `value = scale_max` → `y = inner_min.y` (top). Out-of-range values are clamped
  by `eli_clamp_f` before the linear interpolation.
