/**
 * @file eli_types.h
 * @brief Core value types for elimgui: vectors, rects, ids, packed colors,
 *        and the small inline math/color helpers built on top of them.
 *
 * @status Phase 1 foundation types in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_CORE_ELI_TYPES_H
#define ELI_CORE_ELI_TYPES_H

#include "eli_platform.h"

/* ---------------------------------------------------------------------------
 * Math constant
 * ------------------------------------------------------------------------- */

/** Pi as a 32-bit float literal (matches the precision used by Dear ImGui). */
#define ELI_PI 3.14159265358979323846f

/* ---------------------------------------------------------------------------
 * Vectors and rectangles
 * ------------------------------------------------------------------------- */

/** 2D vector used for positions, sizes, and spacing. */
typedef struct eli_vec2 {
    float x, y;
} eli_vec2;

/** 4D vector used for RGBA colors (components in the 0..1 range) and quads. */
typedef struct eli_vec4 {
    float x, y, z, w;
} eli_vec4;

/**
 * Axis-aligned rectangle stored as an origin (x, y) plus size (w, h).
 * The origin is the top-left (minimum) corner; width/height are non-negative.
 */
typedef struct eli_rect {
    float x, y, w, h;
} eli_rect;

/* ---------------------------------------------------------------------------
 * Identity and color scalar types
 * ------------------------------------------------------------------------- */

/** Widget/window identity hash. Distinct name from uint32_t for clarity. */
typedef uint32_t eli_id;

/** Packed 32-bit color in R,G,B,A byte order (see ELI_COL32 shift macros). */
typedef uint32_t eli_col32;

/* ---------------------------------------------------------------------------
 * Packed color layout
 *
 * Bytes are laid out little-endian friendly: R in the low byte, then G, B, A.
 * ELI_COL32(r, g, b, a) packs four 0..255 components into one eli_col32.
 * ------------------------------------------------------------------------- */

#define ELI_COL32_R_SHIFT 0
#define ELI_COL32_G_SHIFT 8
#define ELI_COL32_B_SHIFT 16
#define ELI_COL32_A_SHIFT 24
#define ELI_COL32_A_MASK  0xFF000000u

#define ELI_COL32(r, g, b, a)                                                  \
    (((uint32_t)(a) << ELI_COL32_A_SHIFT) |                                    \
     ((uint32_t)(b) << ELI_COL32_B_SHIFT) |                                    \
     ((uint32_t)(g) << ELI_COL32_G_SHIFT) |                                    \
     ((uint32_t)(r) << ELI_COL32_R_SHIFT))

/* Common color constants. */
#define ELI_COL32_WHITE       0xFFFFFFFFu
#define ELI_COL32_BLACK       0x000000FFu
#define ELI_COL32_BLACK_TRANS 0x00000000u
#define ELI_COL32_RED         ELI_COL32(255, 0,   0,   255)
#define ELI_COL32_GREEN       ELI_COL32(0,   255, 0,   255)
#define ELI_COL32_BLUE        ELI_COL32(0,   0,   255, 255)

/* ---------------------------------------------------------------------------
 * Small scalar helpers
 * ------------------------------------------------------------------------- */

/** Return the smaller of two floats. */
static inline float eli_min_f(float a, float b)
{
    return a < b ? a : b;
}

/** Return the larger of two floats. */
static inline float eli_max_f(float a, float b)
{
    return a > b ? a : b;
}

/** Clamp v into the inclusive range [lo, hi]. */
static inline float eli_clamp_f(float v, float lo, float hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

/* ---------------------------------------------------------------------------
 * Vector constructors and arithmetic
 * ------------------------------------------------------------------------- */

/** Build an eli_vec2 from its components. */
static inline eli_vec2 eli_make_vec2(float x, float y)
{
    eli_vec2 v = {x, y};
    return v;
}

/** Build an eli_vec4 from its components. */
static inline eli_vec4 eli_make_vec4(float x, float y, float z, float w)
{
    eli_vec4 v = {x, y, z, w};
    return v;
}

/** Component-wise sum of two 2D vectors. */
static inline eli_vec2 eli_vec2_add(eli_vec2 a, eli_vec2 b)
{
    return eli_make_vec2(a.x + b.x, a.y + b.y);
}

/** Component-wise difference (a - b) of two 2D vectors. */
static inline eli_vec2 eli_vec2_sub(eli_vec2 a, eli_vec2 b)
{
    return eli_make_vec2(a.x - b.x, a.y - b.y);
}

/** Scale a 2D vector by a scalar. */
static inline eli_vec2 eli_vec2_scale(eli_vec2 a, float s)
{
    return eli_make_vec2(a.x * s, a.y * s);
}

/* ---------------------------------------------------------------------------
 * Rectangle helpers
 * ------------------------------------------------------------------------- */

/** Build an eli_rect from an origin (x, y) and a size (w, h). */
static inline eli_rect eli_make_rect(float x, float y, float w, float h)
{
    eli_rect r = {x, y, w, h};
    return r;
}

/** Rectangle width. */
static inline float eli_rect_width(eli_rect r)
{
    return r.w;
}

/** Rectangle height. */
static inline float eli_rect_height(eli_rect r)
{
    return r.h;
}

/** Rectangle size as a 2D vector (width, height). */
static inline eli_vec2 eli_rect_size(eli_rect r)
{
    return eli_make_vec2(r.w, r.h);
}

/** Top-left (minimum) corner of the rectangle. */
static inline eli_vec2 eli_rect_min(eli_rect r)
{
    return eli_make_vec2(r.x, r.y);
}

/** Bottom-right (maximum) corner of the rectangle. */
static inline eli_vec2 eli_rect_max(eli_rect r)
{
    return eli_make_vec2(r.x + r.w, r.y + r.h);
}

/** Geometric center of the rectangle. */
static inline eli_vec2 eli_rect_center(eli_rect r)
{
    return eli_make_vec2(r.x + r.w * 0.5f, r.y + r.h * 0.5f);
}

/**
 * Test whether a point lies inside a rectangle.
 * The minimum edges are inclusive and the maximum edges are exclusive
 * (matching Dear ImGui's ImRect::Contains), so adjacent rects never both
 * claim a shared border pixel.
 *
 * @param r  Rectangle to test against.
 * @param p  Point to test.
 * @return   true if p is within [x, x+w) x [y, y+h), false otherwise.
 *
 * Thread-safe: yes (pure function)
 * Reentrant: yes
 */
static inline bool eli_rect_contains(eli_rect r, eli_vec2 p)
{
    return p.x >= r.x && p.y >= r.y && p.x < r.x + r.w && p.y < r.y + r.h;
}

/* ---------------------------------------------------------------------------
 * Color pack / unpack
 * ------------------------------------------------------------------------- */

/** Convert a float in [0,1] to an 8-bit channel value with saturation + rounding. */
static inline uint32_t eli_f32_to_u8_sat(float x)
{
    if (x <= 0.0f)
        return 0u;
    if (x >= 1.0f)
        return 255u;
    return (uint32_t)(x * 255.0f + 0.5f);
}

/**
 * Unpack a packed color into a float RGBA vector (each component in [0,1]).
 *
 * @param c  Packed color.
 * @return   eli_vec4 { r, g, b, a } normalized to 0..1.
 *
 * Thread-safe: yes (pure function)
 * Reentrant: yes
 */
static inline eli_vec4 eli_color_u32_to_vec4(eli_col32 c)
{
    const float s = 1.0f / 255.0f;
    eli_vec4 v;
    v.x = (float)((c >> ELI_COL32_R_SHIFT) & 0xFFu) * s;
    v.y = (float)((c >> ELI_COL32_G_SHIFT) & 0xFFu) * s;
    v.z = (float)((c >> ELI_COL32_B_SHIFT) & 0xFFu) * s;
    v.w = (float)((c >> ELI_COL32_A_SHIFT) & 0xFFu) * s;
    return v;
}

/**
 * Pack a float RGBA vector (each component in [0,1]) into a packed color.
 * Components are saturated to [0,1] before conversion.
 *
 * @param v  Float RGBA color.
 * @return   Packed eli_col32 in R,G,B,A byte order.
 *
 * Thread-safe: yes (pure function)
 * Reentrant: yes
 */
static inline eli_col32 eli_color_vec4_to_u32(eli_vec4 v)
{
    uint32_t r = eli_f32_to_u8_sat(v.x);
    uint32_t g = eli_f32_to_u8_sat(v.y);
    uint32_t b = eli_f32_to_u8_sat(v.z);
    uint32_t a = eli_f32_to_u8_sat(v.w);
    return ELI_COL32(r, g, b, a);
}

#endif /* ELI_CORE_ELI_TYPES_H */
