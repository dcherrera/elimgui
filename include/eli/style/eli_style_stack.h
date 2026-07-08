/**
 * @file eli_style_stack.h
 * @brief Push/pop stacks for style colors and style variables. Mirrors Dear
 *        ImGui's PushStyleColor/PushStyleVar: each push records the previous
 *        value on the context stack so the matching pop restores it exactly.
 *
 * @status Phase 6 style stacks in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_STYLE_ELI_STYLE_STACK_H
#define ELI_STYLE_ELI_STYLE_STACK_H

#include "../core/eli_platform.h"
#include "../core/eli_types.h"
#include "../core/eli_enums.h"
#include "../core/eli_style_types.h"
#include "../core/eli_context.h"
#include "eli_style_color_utils.h"

/* ---------------------------------------------------------------------------
 * Style variable info
 *
 * Maps each eli_style_var to its backing field in eli_style plus a component
 * count (1 = float, 2 = eli_vec2). eli_vec2 is two contiguous floats, so a
 * float pointer into the field addresses both components. This mirrors ImGui's
 * GStyleVarInfo/GetVarPtr but resolves via a switch rather than offsetof.
 * ------------------------------------------------------------------------- */

/**
 * Resolve a style variable to a mutable pointer into the style and its
 * component count.
 *
 * @param style      Style to address into (must be non-NULL).
 * @param idx        Style variable (see eli_style_var).
 * @param out_count  Receives 1 (float) or 2 (vec2); untouched on failure.
 * @return           Pointer to the first float of the field, or NULL if idx is
 *                   out of range.
 */
static inline float *eli_style_var_get_ptr(eli_style *style, eli_style_var idx, int *out_count)
{
    switch (idx) {
    case ELI_STYLE_VAR_ALPHA:                          *out_count = 1; return &style->alpha;
    case ELI_STYLE_VAR_DISABLED_ALPHA:                 *out_count = 1; return &style->disabled_alpha;
    case ELI_STYLE_VAR_WINDOW_PADDING:                 *out_count = 2; return &style->window_padding.x;
    case ELI_STYLE_VAR_WINDOW_ROUNDING:                *out_count = 1; return &style->window_rounding;
    case ELI_STYLE_VAR_WINDOW_BORDER_SIZE:             *out_count = 1; return &style->window_border_size;
    case ELI_STYLE_VAR_WINDOW_MIN_SIZE:                *out_count = 2; return &style->window_min_size.x;
    case ELI_STYLE_VAR_WINDOW_TITLE_ALIGN:             *out_count = 2; return &style->window_title_align.x;
    case ELI_STYLE_VAR_CHILD_ROUNDING:                 *out_count = 1; return &style->child_rounding;
    case ELI_STYLE_VAR_CHILD_BORDER_SIZE:              *out_count = 1; return &style->child_border_size;
    case ELI_STYLE_VAR_POPUP_ROUNDING:                 *out_count = 1; return &style->popup_rounding;
    case ELI_STYLE_VAR_POPUP_BORDER_SIZE:              *out_count = 1; return &style->popup_border_size;
    case ELI_STYLE_VAR_FRAME_PADDING:                  *out_count = 2; return &style->frame_padding.x;
    case ELI_STYLE_VAR_FRAME_ROUNDING:                 *out_count = 1; return &style->frame_rounding;
    case ELI_STYLE_VAR_FRAME_BORDER_SIZE:              *out_count = 1; return &style->frame_border_size;
    case ELI_STYLE_VAR_ITEM_SPACING:                   *out_count = 2; return &style->item_spacing.x;
    case ELI_STYLE_VAR_ITEM_INNER_SPACING:             *out_count = 2; return &style->item_inner_spacing.x;
    case ELI_STYLE_VAR_INDENT_SPACING:                 *out_count = 1; return &style->indent_spacing;
    case ELI_STYLE_VAR_CELL_PADDING:                   *out_count = 2; return &style->cell_padding.x;
    case ELI_STYLE_VAR_SCROLLBAR_SIZE:                 *out_count = 1; return &style->scrollbar_size;
    case ELI_STYLE_VAR_SCROLLBAR_ROUNDING:             *out_count = 1; return &style->scrollbar_rounding;
    case ELI_STYLE_VAR_GRAB_MIN_SIZE:                  *out_count = 1; return &style->grab_min_size;
    case ELI_STYLE_VAR_GRAB_ROUNDING:                  *out_count = 1; return &style->grab_rounding;
    case ELI_STYLE_VAR_TAB_ROUNDING:                   *out_count = 1; return &style->tab_rounding;
    case ELI_STYLE_VAR_TAB_BORDER_SIZE:                *out_count = 1; return &style->tab_border_size;
    case ELI_STYLE_VAR_TAB_BAR_BORDER_SIZE:            *out_count = 1; return &style->tab_bar_border_size;
    case ELI_STYLE_VAR_TAB_BAR_OVERLINE_SIZE:          *out_count = 1; return &style->tab_bar_overline_size;
    case ELI_STYLE_VAR_TABLE_ANGLED_HEADERS_ANGLE:     *out_count = 1; return &style->table_angled_headers_angle;
    case ELI_STYLE_VAR_TABLE_ANGLED_HEADERS_TEXT_ALIGN: *out_count = 2; return &style->table_angled_headers_text_align.x;
    case ELI_STYLE_VAR_BUTTON_TEXT_ALIGN:              *out_count = 2; return &style->button_text_align.x;
    case ELI_STYLE_VAR_SELECTABLE_TEXT_ALIGN:          *out_count = 2; return &style->selectable_text_align.x;
    case ELI_STYLE_VAR_SEPARATOR_TEXT_BORDER_SIZE:     *out_count = 1; return &style->separator_text_border_size;
    case ELI_STYLE_VAR_SEPARATOR_TEXT_ALIGN:           *out_count = 2; return &style->separator_text_align.x;
    case ELI_STYLE_VAR_SEPARATOR_TEXT_PADDING:         *out_count = 2; return &style->separator_text_padding.x;
    default:                                           return NULL;
    }
}

/* ---------------------------------------------------------------------------
 * Style color stack
 * ------------------------------------------------------------------------- */

/**
 * Push a color slot override from a packed color, saving the previous value.
 *
 * @param idx  Color slot (see eli_col).
 * @param col  Packed color in R,G,B,A byte order.
 *
 * Thread-safe: no (mutates the current context)
 * Reentrant: no
 */
static inline void eli_push_style_color(eli_col idx, eli_col32 col)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || idx < 0 || idx >= ELI_COL_COUNT)
        return;
    if (ctx->color_stack_size >= ELI_STYLE_COLOR_STACK_MAX)
        return;

    eli_color_mod *mod = &ctx->color_stack[ctx->color_stack_size++];
    mod->col = idx;
    mod->backup_value = ctx->style.colors[idx];
    ctx->style.colors[idx] = eli_color_convert_u32_to_float4(col);
}

/**
 * Push a color slot override from a float color, saving the previous value.
 *
 * @param idx  Color slot (see eli_col).
 * @param col  Float RGBA color (each component in 0..1).
 *
 * Thread-safe: no (mutates the current context)
 * Reentrant: no
 */
static inline void eli_push_style_color_vec4(eli_col idx, eli_vec4 col)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || idx < 0 || idx >= ELI_COL_COUNT)
        return;
    if (ctx->color_stack_size >= ELI_STYLE_COLOR_STACK_MAX)
        return;

    eli_color_mod *mod = &ctx->color_stack[ctx->color_stack_size++];
    mod->col = idx;
    mod->backup_value = ctx->style.colors[idx];
    ctx->style.colors[idx] = col;
}

/**
 * Pop the last `count` pushed color overrides, restoring their prior values.
 * A count larger than the stack depth is clamped.
 *
 * @param count  Number of overrides to undo.
 *
 * Thread-safe: no (mutates the current context)
 * Reentrant: no
 */
static inline void eli_pop_style_color(int count)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    if (count > ctx->color_stack_size)
        count = ctx->color_stack_size;

    while (count > 0) {
        eli_color_mod *mod = &ctx->color_stack[--ctx->color_stack_size];
        ctx->style.colors[mod->col] = mod->backup_value;
        count--;
    }
}

/* ---------------------------------------------------------------------------
 * Style variable stack
 * ------------------------------------------------------------------------- */

/**
 * Push a float style variable override, saving the previous value. Ignored if
 * the variable is not a single-float variable (wrong-kind push).
 *
 * @param idx  Style variable (see eli_style_var).
 * @param val  New value.
 *
 * Thread-safe: no (mutates the current context)
 * Reentrant: no
 */
static inline void eli_push_style_var(eli_style_var idx, float val)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->style_var_stack_size >= ELI_STYLE_VAR_STACK_MAX)
        return;

    int count = 0;
    float *pvar = eli_style_var_get_ptr(&ctx->style, idx, &count);
    if (pvar == NULL || count != 1)
        return;

    eli_style_mod *mod = &ctx->style_var_stack[ctx->style_var_stack_size++];
    mod->var = idx;
    mod->backup[0] = pvar[0];
    pvar[0] = val;
}

/**
 * Push a vec2 style variable override, saving the previous value. Ignored if
 * the variable is not a vec2 variable (wrong-kind push).
 *
 * @param idx  Style variable (see eli_style_var).
 * @param val  New value.
 *
 * Thread-safe: no (mutates the current context)
 * Reentrant: no
 */
static inline void eli_push_style_var_vec2(eli_style_var idx, eli_vec2 val)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->style_var_stack_size >= ELI_STYLE_VAR_STACK_MAX)
        return;

    int count = 0;
    float *pvar = eli_style_var_get_ptr(&ctx->style, idx, &count);
    if (pvar == NULL || count != 2)
        return;

    eli_style_mod *mod = &ctx->style_var_stack[ctx->style_var_stack_size++];
    mod->var = idx;
    mod->backup[0] = pvar[0];
    mod->backup[1] = pvar[1];
    pvar[0] = val.x;
    pvar[1] = val.y;
}

/**
 * Pop the last `count` pushed style-variable overrides, restoring prior values.
 * A count larger than the stack depth is clamped.
 *
 * @param count  Number of overrides to undo.
 *
 * Thread-safe: no (mutates the current context)
 * Reentrant: no
 */
static inline void eli_pop_style_var(int count)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    if (count > ctx->style_var_stack_size)
        count = ctx->style_var_stack_size;

    while (count > 0) {
        eli_style_mod *mod = &ctx->style_var_stack[--ctx->style_var_stack_size];
        int var_count = 0;
        float *pvar = eli_style_var_get_ptr(&ctx->style, mod->var, &var_count);
        if (pvar != NULL) {
            pvar[0] = mod->backup[0];
            if (var_count == 2)
                pvar[1] = mod->backup[1];
        }
        count--;
    }
}

#endif /* ELI_STYLE_ELI_STYLE_STACK_H */
