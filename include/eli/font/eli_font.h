/**
 * @file eli_font.h
 * @brief Aggregator for the elimgui font category: font types, the embedded
 *        default font, glyph-range tables, the stb-backed atlas builder, and
 *        text measurement/rendering plus the context font stack.
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
#include "eli_font_atlas.h"
#include "eli_font_text.h"

#endif /* ELI_FONT_ELI_FONT_H */
