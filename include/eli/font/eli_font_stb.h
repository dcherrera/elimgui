/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_font_stb.h
 * @brief stb_truetype + stb_rect_pack integration for the font system.
 *
 * This is the one place that defines STB_TRUETYPE_IMPLEMENTATION and
 * STB_RECT_PACK_IMPLEMENTATION; every STBTT_/STBRP_ hook is routed through the
 * libc seam (eli_platform.h) so the same code compiles for wasm32 (JAClibc) and
 * for native hosted unit tests.
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#ifndef ELI_FONT_ELI_FONT_STB_H
#define ELI_FONT_ELI_FONT_STB_H

#include "../core/eli_platform.h"
#include "../util/eli_mem.h"

/* Route stb's libc + math hooks through the seam headers (already included via
 * eli_platform.h). Defining these suppresses stb's own <stdlib.h>/<math.h>/etc. */
#define STBTT_malloc(x, u) ((void)(u), eli_mem_alloc(x))
#define STBTT_free(x, u)   ((void)(u), eli_mem_free(x))
#define STBTT_assert(x)    ((void)0)
#define STBTT_strlen(x)    strlen(x)
#define STBTT_memcpy       memcpy
#define STBTT_memset       memset
#define STBTT_ifloor(x)    ((int)floor(x))
#define STBTT_iceil(x)     ((int)ceil(x))
#define STBTT_sqrt(x)      sqrt(x)
#define STBTT_pow(x, y)    pow(x, y)
#define STBTT_fmod(x, y)   fmod(x, y)
#define STBTT_cos(x)       cos(x)
#define STBTT_acos(x)      acos(x)
#define STBTT_fabs(x)      fabs(x)

#define STBRP_SORT      qsort
#define STBRP_ASSERT(x) ((void)0)

/* stb_rect_pack must precede stb_truetype so the latter binds the real packer.
 * Linkage follows eli_state.h: under ELI_SHARED_STATE only the ELI_IMPLEMENTATION
 * TU compiles the stb bodies (external linkage) and other TUs see declarations;
 * by default every TU gets its own static copy, so static-mode TUs still link. */
#if !defined(ELI_SHARED_STATE) || defined(ELI_IMPLEMENTATION)
    #define STB_RECT_PACK_IMPLEMENTATION
    #define STB_TRUETYPE_IMPLEMENTATION
#endif
#if !defined(ELI_SHARED_STATE)
    #define STBRP_STATIC
    #define STBTT_STATIC
#endif
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include <stb/stb_rect_pack.h>
#include <stb/stb_truetype.h>
#pragma GCC diagnostic pop

#endif /* ELI_FONT_ELI_FONT_STB_H */
