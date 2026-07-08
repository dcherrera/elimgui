/**
 * @file cheat_theme.h
 * @brief Small design system for the elimgui visual cheatsheet: named spacing,
 *        rounding, and dark-mode role/token colors, plus cheat_theme_apply()
 *        which programs the shared eli_style so the whole app is consistent.
 *
 * The palette and numbers here are lifted from established UI/UX best practices,
 * not guessed (topic -> what it informs):
 *   - layout-and-spacing.md   -> the 4/8pt spacing scale + containment order
 *                                 (space > background > border) and paddings.
 *   - color-and-theming.md    -> dark base #121212, elevation-by-lightening
 *                                 surfaces, white text at 87/60/38% tiers, a
 *                                 desaturated ~#82AAFF accent, 8/12% state
 *                                 layers, selection = accent @ ~25%.
 *   - typography.md           -> single 13px font; hierarchy comes from color
 *                                 (weight-by-color) + spacing, never font size.
 *   - feedback-and-states.md  -> hover 8% / active 12% overlays, selected fill.
 *
 * Everything is `static inline` so multiple translation units can include the
 * header without symbol collisions, matching the rest of the cheatsheet app.
 *
 * @status Cheatsheet app design system. Not part of the elimgui library API.
 * @issues None
 * @todo None
 */
#ifndef CHEAT_THEME_H
#define CHEAT_THEME_H

#include <eli/elimgui.h>

/* ---------------------------------------------------------------------------
 * Spacing scale (layout-and-spacing.md: "2 4 6 8 12 16 24 32 48")
 *
 * A single fixed, non-linear scale. Every gap/padding in the app is one of
 * these tokens; no ad-hoc 13/22/31px values. "4 inside a component, 8+ between
 * components, 24 between groups."
 * ------------------------------------------------------------------------- */

#define CHEAT_SPACE_2   2.0f
#define CHEAT_SPACE_4   4.0f
#define CHEAT_SPACE_6   6.0f
#define CHEAT_SPACE_8   8.0f
#define CHEAT_SPACE_12  12.0f
#define CHEAT_SPACE_16  16.0f
#define CHEAT_SPACE_24  24.0f
#define CHEAT_SPACE_32  32.0f

/* ---------------------------------------------------------------------------
 * Rounding & border tokens
 *
 * Cards get the larger radius (they are the biggest surface); frames/insets a
 * smaller one. Borders are 1px and used sparingly — the containment hierarchy
 * prefers background shifts and space over outlines.
 * ------------------------------------------------------------------------- */

#define CHEAT_ROUND_CARD   8.0f   /* card / code-box outer corners */
#define CHEAT_ROUND_FRAME  6.0f   /* inputs, buttons, selectables  */
#define CHEAT_ROUND_INSET  6.0f   /* live-preview inset panel       */
#define CHEAT_BORDER_SIZE  1.0f

/* ---------------------------------------------------------------------------
 * Dark-mode role/token palette (color-and-theming.md)
 *
 * Neutrals are near-black greys separated by lightness = elevation (shadows are
 * invisible on dark fields, so a raised surface is simply lighter). The accent
 * is a desaturated blue so it doesn't vibrate against the dark field. Text is
 * white at fixed opacity tiers, which doubles as our (font-size-less) hierarchy
 * mechanism: brighter == more important.
 * ------------------------------------------------------------------------- */

/* Neutral surfaces, darkest (recessed) -> lightest (most raised). */
static inline eli_vec4 cheat_bg_vec4(void)             { return eli_make_vec4(0.071f, 0.071f, 0.071f, 1.00f); } /* #121212 window base   */
static inline eli_vec4 cheat_surface_sidebar_vec4(void){ return eli_make_vec4(0.102f, 0.102f, 0.102f, 1.00f); } /* #1A1A1A nav region    */
static inline eli_vec4 cheat_surface_card_vec4(void)   { return eli_make_vec4(0.130f, 0.130f, 0.135f, 1.00f); } /* ~#212122 raised card  */
static inline eli_vec4 cheat_surface_inset_vec4(void)  { return eli_make_vec4(0.169f, 0.169f, 0.176f, 1.00f); } /* ~#2B2B2D preview inset */
static inline eli_vec4 cheat_code_vec4(void)           { return eli_make_vec4(0.043f, 0.043f, 0.051f, 1.00f); } /* ~#0B0B0D sunken code   */

/* White text tiers: high(87) / medium(60) / low(38) emphasis. */
static inline eli_vec4 cheat_text_hi_vec4(void)  { return eli_make_vec4(1.00f, 1.00f, 1.00f, 0.87f); }
static inline eli_vec4 cheat_text_med_vec4(void) { return eli_make_vec4(1.00f, 1.00f, 1.00f, 0.60f); }
static inline eli_vec4 cheat_text_lo_vec4(void)  { return eli_make_vec4(1.00f, 1.00f, 1.00f, 0.38f); }

/* Hairline separators / faint card outlines (white @ 12% / 8%). */
static inline eli_vec4 cheat_border_vec4(void)       { return eli_make_vec4(1.00f, 1.00f, 1.00f, 0.12f); }
static inline eli_vec4 cheat_border_faint_vec4(void) { return eli_make_vec4(1.00f, 1.00f, 1.00f, 0.07f); }

/* Desaturated accent (~#82AAFF) and its hover/active steps. */
static inline eli_vec4 cheat_accent_vec4(void)        { return eli_make_vec4(0.510f, 0.667f, 1.000f, 1.00f); }
static inline eli_vec4 cheat_accent_hover_vec4(void)  { return eli_make_vec4(0.616f, 0.741f, 1.000f, 1.00f); }
static inline eli_vec4 cheat_accent_active_vec4(void) { return eli_make_vec4(0.710f, 0.804f, 1.000f, 1.00f); }

/* State layers (feedback-and-states.md): neutral hover 8%, accent selection. */
static inline eli_vec4 cheat_overlay_hover_vec4(void)   { return eli_make_vec4(1.000f, 1.000f, 1.000f, 0.08f); }
static inline eli_vec4 cheat_overlay_active_vec4(void)  { return eli_make_vec4(1.000f, 1.000f, 1.000f, 0.12f); }
static inline eli_vec4 cheat_selected_vec4(void)        { return eli_make_vec4(0.510f, 0.667f, 1.000f, 0.24f); } /* accent @ 24% */
static inline eli_vec4 cheat_selected_active_vec4(void) { return eli_make_vec4(0.510f, 0.667f, 1.000f, 0.34f); } /* accent @ 34% */

/* ---------------------------------------------------------------------------
 * Packed-color convenience (resolve through style.alpha for uniform blending)
 * ------------------------------------------------------------------------- */

static inline eli_col32 cheat_bg_u32(void)            { return eli_get_color_u32_vec4(cheat_bg_vec4()); }
static inline eli_col32 cheat_surface_card_u32(void)  { return eli_get_color_u32_vec4(cheat_surface_card_vec4()); }
static inline eli_col32 cheat_surface_inset_u32(void) { return eli_get_color_u32_vec4(cheat_surface_inset_vec4()); }
static inline eli_col32 cheat_code_u32(void)          { return eli_get_color_u32_vec4(cheat_code_vec4()); }
static inline eli_col32 cheat_border_u32(void)        { return eli_get_color_u32_vec4(cheat_border_vec4()); }
static inline eli_col32 cheat_border_faint_u32(void)  { return eli_get_color_u32_vec4(cheat_border_faint_vec4()); }
static inline eli_col32 cheat_text_med_u32(void)      { return eli_get_color_u32_vec4(cheat_text_med_vec4()); }
static inline eli_col32 cheat_accent_u32(void)        { return eli_get_color_u32_vec4(cheat_accent_vec4()); }

/* ---------------------------------------------------------------------------
 * Apply the design system to the shared eli_style
 *
 * Seeds every color slot with the built-in dark theme (so nothing is left
 * zeroed), then overrides the subset the cheatsheet cares about with our role
 * tokens, and programs the sizing/spacing/rounding vars from the scale.
 * ------------------------------------------------------------------------- */

/**
 * Program the current context's style with the cheatsheet design system.
 *
 * Call once at startup (replaces eli_style_colors_dark). Safe to call whenever a
 * current context exists; a no-op if none is set.
 *
 * Thread-safe: no (writes the current context's style)
 * Reentrant: yes
 */
static inline void cheat_theme_apply(void)
{
    eli_style *s = eli_get_style();
    if (s == NULL)
        return;

    /* Valid baseline for the ~50 slots we don't touch below. */
    eli_style_colors_dark(s);

    eli_vec4 *c = s->colors;

    /* Text tiers. TEXT_DISABLED is bumped to the *medium* (60%) tier because the
     * app leans on it for secondary text (api sigs, subtitle, counts) that must
     * stay readable; genuinely muted text is pushed to 38% locally. */
    c[ELI_COL_TEXT]          = cheat_text_hi_vec4();
    c[ELI_COL_TEXT_DISABLED] = cheat_text_med_vec4();

    /* Surfaces. Child bg is left transparent so the content pane reads as the
     * base window field and cards float on it (containment via background
     * shift); the sidebar paints its own raised surface locally. Transparent
     * child bg also keeps live-preview child widgets from covering their inset. */
    c[ELI_COL_WINDOW_BG] = cheat_bg_vec4();
    c[ELI_COL_CHILD_BG]  = eli_make_vec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ELI_COL_POPUP_BG]  = cheat_surface_inset_vec4();
    c[ELI_COL_MENU_BAR_BG] = cheat_surface_sidebar_vec4();

    /* Borders / separators (used sparingly). */
    c[ELI_COL_BORDER]        = cheat_border_vec4();
    c[ELI_COL_BORDER_SHADOW] = eli_make_vec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ELI_COL_SEPARATOR]     = cheat_border_vec4();

    /* Frames (inputs) as subtle white overlays so they lift off any surface;
     * hover/active step the state layer 5% -> 8% -> 12%. */
    c[ELI_COL_FRAME_BG]         = eli_make_vec4(1.0f, 1.0f, 1.0f, 0.05f);
    c[ELI_COL_FRAME_BG_HOVERED] = cheat_overlay_hover_vec4();
    c[ELI_COL_FRAME_BG_ACTIVE]  = cheat_overlay_active_vec4();

    /* Buttons: tonal-accent secondary (fill differentiates them from text; the
     * accent tint says "interactive"). Kept scarce (60-30-10). */
    c[ELI_COL_BUTTON]         = eli_make_vec4(0.510f, 0.667f, 1.000f, 0.16f);
    c[ELI_COL_BUTTON_HOVERED] = eli_make_vec4(0.510f, 0.667f, 1.000f, 0.28f);
    c[ELI_COL_BUTTON_ACTIVE]  = eli_make_vec4(0.510f, 0.667f, 1.000f, 0.40f);

    /* Selectable rows (sidebar categories): selected = persistent accent layer,
     * hover = neutral overlay — hover must never look like selected. */
    c[ELI_COL_HEADER]         = cheat_selected_vec4();
    c[ELI_COL_HEADER_HOVERED] = cheat_overlay_hover_vec4();
    c[ELI_COL_HEADER_ACTIVE]  = cheat_selected_active_vec4();

    /* Accent-driven controls. */
    c[ELI_COL_CHECK_MARK]        = cheat_accent_vec4();
    c[ELI_COL_SLIDER_GRAB]       = cheat_accent_vec4();
    c[ELI_COL_SLIDER_GRAB_ACTIVE]= cheat_accent_active_vec4();
    c[ELI_COL_TEXT_LINK]         = cheat_accent_vec4();
    c[ELI_COL_TEXT_SELECTED_BG]  = eli_make_vec4(0.510f, 0.667f, 1.000f, 0.30f);
    c[ELI_COL_NAV_CURSOR]        = cheat_accent_vec4();

    /* Scrollbars: quiet, neutral overlays. */
    c[ELI_COL_SCROLLBAR_BG]           = eli_make_vec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ELI_COL_SCROLLBAR_GRAB]         = eli_make_vec4(1.0f, 1.0f, 1.0f, 0.16f);
    c[ELI_COL_SCROLLBAR_GRAB_HOVERED] = eli_make_vec4(1.0f, 1.0f, 1.0f, 0.24f);
    c[ELI_COL_SCROLLBAR_GRAB_ACTIVE]  = eli_make_vec4(1.0f, 1.0f, 1.0f, 0.32f);

    /* Tabs consistent with the selection language. */
    c[ELI_COL_TAB]                   = eli_make_vec4(1.0f, 1.0f, 1.0f, 0.05f);
    c[ELI_COL_TAB_HOVERED]           = cheat_overlay_hover_vec4();
    c[ELI_COL_TAB_SELECTED]          = cheat_selected_vec4();
    c[ELI_COL_TAB_SELECTED_OVERLINE] = cheat_accent_vec4();

    /* Tables: hairline dividers + faint zebra, no heavy borders. */
    c[ELI_COL_TABLE_HEADER_BG]     = eli_make_vec4(1.0f, 1.0f, 1.0f, 0.05f);
    c[ELI_COL_TABLE_BORDER_STRONG] = cheat_border_vec4();
    c[ELI_COL_TABLE_BORDER_LIGHT]  = cheat_border_faint_vec4();
    c[ELI_COL_TABLE_ROW_BG_ALT]    = eli_make_vec4(1.0f, 1.0f, 1.0f, 0.03f);

    /* -----------------------------------------------------------------------
     * Sizing / spacing / rounding from the scale.
     * --------------------------------------------------------------------- */

    /* Window/panel padding 12 (dense-comfortable band). */
    s->window_padding = eli_make_vec2(CHEAT_SPACE_12, CHEAT_SPACE_12);

    /* Frame padding 12x7 -> ~27px control height (desktop dense 28-32 band),
     * ≥24px hit target. Horizontal 12 keeps single-word buttons from looking
     * square. */
    s->frame_padding = eli_make_vec2(CHEAT_SPACE_12, 7.0f);

    /* 8px between widgets (both axes) for an even vertical rhythm; 6px icon<->
     * label inner spacing. */
    s->item_spacing       = eli_make_vec2(CHEAT_SPACE_8, CHEAT_SPACE_8);
    s->item_inner_spacing = eli_make_vec2(CHEAT_SPACE_6, CHEAT_SPACE_6);
    s->indent_spacing     = CHEAT_SPACE_16;
    s->cell_padding       = eli_make_vec2(CHEAT_SPACE_8, CHEAT_SPACE_6);

    /* Rounding. */
    s->window_rounding = 0.0f;                 /* fullscreen host */
    s->child_rounding  = CHEAT_ROUND_CARD;
    s->popup_rounding  = CHEAT_ROUND_FRAME;
    s->frame_rounding  = CHEAT_ROUND_FRAME;
    s->grab_rounding   = CHEAT_ROUND_FRAME;
    s->tab_rounding    = CHEAT_ROUND_FRAME;
    s->scrollbar_rounding = CHEAT_ROUND_FRAME;

    /* Prefer background shifts + space to borders (containment order). */
    s->window_border_size = 0.0f;
    s->child_border_size  = 0.0f;
    s->frame_border_size  = 0.0f;

    /* Slimmer, rounded scrollbar. */
    s->scrollbar_size = CHEAT_SPACE_12;
}

#endif /* CHEAT_THEME_H */
