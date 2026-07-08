/**
 * @file eli_font_glyph_ranges.h
 * @brief Predefined glyph-range tables (codepoint [lo, hi] pairs, zero
 *        terminated) selecting which characters an atlas bakes for a font.
 *
 * Each getter returns a pointer to a static, immutable range table. The atlas
 * argument is accepted for API symmetry with Dear ImGui but is unused. Ranges
 * for large scripts (CJK) cover the relevant Unicode blocks rather than a
 * frequency-ranked subset, so they favor completeness over atlas size.
 *
 * @status Phase 3 glyph ranges in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_FONT_ELI_FONT_GLYPH_RANGES_H
#define ELI_FONT_ELI_FONT_GLYPH_RANGES_H

#include "../core/eli_platform.h"
#include "eli_font_types.h"

/**
 * @return Basic Latin + Latin-1 Supplement (0x0020..0x00FF).
 * Thread-safe: yes  Reentrant: yes
 */
static inline const uint16_t *eli_font_atlas_get_glyph_ranges_default(eli_font_atlas *atlas)
{
    static const uint16_t ranges[] = {0x0020, 0x00FF, 0};
    (void)atlas;
    return ranges;
}

/** @return Default range plus the Greek and Coptic block. */
static inline const uint16_t *eli_font_atlas_get_glyph_ranges_greek(eli_font_atlas *atlas)
{
    static const uint16_t ranges[] = {
        0x0020, 0x00FF, /* Basic Latin + Latin Supplement */
        0x0370, 0x03FF, /* Greek and Coptic */
        0x1F00, 0x1FFF, /* Greek Extended */
        0};
    (void)atlas;
    return ranges;
}

/** @return Default range plus Hangul Jamo, compatibility Jamo, and syllables. */
static inline const uint16_t *eli_font_atlas_get_glyph_ranges_korean(eli_font_atlas *atlas)
{
    static const uint16_t ranges[] = {
        0x0020, 0x00FF, /* Basic Latin + Latin Supplement */
        0x3131, 0x3163, /* Hangul compatibility Jamo */
        0xAC00, 0xD7A3, /* Hangul syllables */
        0};
    (void)atlas;
    return ranges;
}

/** @return Default range plus Kana, CJK punctuation, and unified ideographs. */
static inline const uint16_t *eli_font_atlas_get_glyph_ranges_japanese(eli_font_atlas *atlas)
{
    static const uint16_t ranges[] = {
        0x0020, 0x00FF, /* Basic Latin + Latin Supplement */
        0x3000, 0x30FF, /* CJK symbols, Hiragana, Katakana */
        0x31F0, 0x31FF, /* Katakana phonetic extensions */
        0xFF00, 0xFFEF, /* Half/fullwidth forms */
        0x4E00, 0x9FAF, /* CJK unified ideographs (common) */
        0};
    (void)atlas;
    return ranges;
}

/** @return Default range plus the full common CJK unified-ideograph blocks. */
static inline const uint16_t *eli_font_atlas_get_glyph_ranges_chinese_full(eli_font_atlas *atlas)
{
    static const uint16_t ranges[] = {
        0x0020, 0x00FF, /* Basic Latin + Latin Supplement */
        0x2000, 0x206F, /* General punctuation */
        0x3000, 0x30FF, /* CJK symbols, Hiragana, Katakana */
        0x31F0, 0x31FF, /* Katakana phonetic extensions */
        0xFF00, 0xFFEF, /* Half/fullwidth forms */
        0x4E00, 0x9FAF, /* CJK unified ideographs */
        0};
    (void)atlas;
    return ranges;
}

/**
 * @return Common simplified-Chinese coverage. Approximated by the shared CJK
 *         blocks (not a frequency-ranked subset), so it is broader but larger.
 */
static inline const uint16_t *
eli_font_atlas_get_glyph_ranges_chinese_simplified_common(eli_font_atlas *atlas)
{
    static const uint16_t ranges[] = {
        0x0020, 0x00FF, /* Basic Latin + Latin Supplement */
        0x2000, 0x206F, /* General punctuation */
        0x3000, 0x30FF, /* CJK symbols, Hiragana, Katakana */
        0x31F0, 0x31FF, /* Katakana phonetic extensions */
        0xFF00, 0xFFEF, /* Half/fullwidth forms */
        0x4E00, 0x9FAF, /* CJK unified ideographs */
        0};
    (void)atlas;
    return ranges;
}

/** @return Default range plus Cyrillic and Cyrillic Supplement blocks. */
static inline const uint16_t *eli_font_atlas_get_glyph_ranges_cyrillic(eli_font_atlas *atlas)
{
    static const uint16_t ranges[] = {
        0x0020, 0x00FF, /* Basic Latin + Latin Supplement */
        0x0400, 0x052F, /* Cyrillic + Cyrillic Supplement */
        0x2DE0, 0x2DFF, /* Cyrillic Extended-A */
        0xA640, 0xA69F, /* Cyrillic Extended-B */
        0};
    (void)atlas;
    return ranges;
}

/** @return Default range plus the Thai block and some general punctuation. */
static inline const uint16_t *eli_font_atlas_get_glyph_ranges_thai(eli_font_atlas *atlas)
{
    static const uint16_t ranges[] = {
        0x0020, 0x00FF, /* Basic Latin + Latin Supplement */
        0x2010, 0x205E, /* General punctuation */
        0x0E00, 0x0E7F, /* Thai */
        0};
    (void)atlas;
    return ranges;
}

/** @return Default range plus the Latin Extended blocks used by Vietnamese. */
static inline const uint16_t *eli_font_atlas_get_glyph_ranges_vietnamese(eli_font_atlas *atlas)
{
    static const uint16_t ranges[] = {
        0x0020, 0x00FF, /* Basic Latin + Latin Supplement */
        0x0102, 0x0103, /* A/a with breve */
        0x0110, 0x0111, /* D/d with stroke */
        0x0128, 0x0129, /* I/i with tilde */
        0x0168, 0x0169, /* U/u with tilde */
        0x01A0, 0x01A1, /* O/o with horn */
        0x01AF, 0x01B0, /* U/u with horn */
        0x1EA0, 0x1EF9, /* Latin Extended Additional (Vietnamese) */
        0};
    (void)atlas;
    return ranges;
}

#endif /* ELI_FONT_ELI_FONT_GLYPH_RANGES_H */
