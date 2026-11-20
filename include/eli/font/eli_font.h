/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_font.h
 * @brief Aggregator for the elimgui font category: font types, the embedded
 *        default font, glyph-range tables, the stb-backed atlas builder, and
 *        text measurement/rendering plus the context font stack, faux bold and
 *        single-line cuts with an ellipsis.
 *
 * Include this to pull in the whole font system:
 *     #include <eli/font/eli_font.h>
 *
 * Text rendering emits geometry into an eli_draw_list, so the draw category is
 * included transitively.
 *
 * @status Phase 3 font aggregator in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_FONT_ELI_FONT_H
#define ELI_FONT_ELI_FONT_H

#include "../draw/eli_draw.h"

#include "eli_font_types.h"
#include "eli_font_proggy.h"
#include "eli_font_glyph_ranges.h"
#include "eli_font_stb.h"
#include "eli_font_glyph_table.h"
#include "eli_font_atlas_pages.h"
#include "eli_font_atlas_on_demand.h"
#include "eli_font_atlas.h"
#include "eli_font_text.h"
#include "eli_font_text_style.h"

#endif /* ELI_FONT_ELI_FONT_H */
