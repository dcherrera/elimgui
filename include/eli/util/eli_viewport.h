/**
 * @file eli_viewport.h
 * @brief The eli_viewport structure and eli_get_main_viewport, describing the
 *        area elimgui renders into (position, size, and usable work area).
 *
 * elimgui targets a single WASM/browser canvas, so exactly one viewport exists:
 * the main viewport, whose origin is (0,0) and whose size is io.display_size.
 * The work area (the region free of OS decoration such as menu bars) equals the
 * full viewport for now; a later multi-viewport phase may narrow it.
 *
 * @status Phase 27 single main viewport in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_UTIL_ELI_VIEWPORT_H
#define ELI_UTIL_ELI_VIEWPORT_H

#include "../core/eli_platform.h"

#include "../core/eli_types.h"
#include "../core/eli_context.h"

/* Stable identity of the single main viewport (mirrors Dear ImGui's convention
 * of a fixed non-zero id for the primary viewport). */
#define ELI_VIEWPORT_MAIN_ID 0x11111111u

/** Viewport capability/state flags. Only the "is primary" bit is used today. */
typedef int eli_viewport_flags;
enum eli_viewport_flags_ {
    ELI_VIEWPORT_FLAGS_NONE       = 0,
    ELI_VIEWPORT_FLAGS_IS_PRIMARY = 1 << 0
};

/**
 * A rendering surface elimgui draws into. `pos`/`size` describe the full surface;
 * `work_pos`/`work_size` describe the usable sub-region (equal to the full area
 * until OS-decoration reservation lands). Returned by eli_get_main_viewport.
 */
typedef struct eli_viewport {
    eli_id             id;         /* stable viewport identity */
    eli_viewport_flags flags;      /* capability/state flags */
    eli_vec2           pos;        /* top-left of the viewport, in screen space */
    eli_vec2           size;       /* full viewport size */
    eli_vec2           work_pos;   /* top-left of the usable work area */
    eli_vec2           work_size;  /* size of the usable work area */
} eli_viewport;

/**
 * Retrieve the single main viewport, refreshed from the current context's
 * io.display_size. Pos is (0,0); the work area equals the full viewport.
 *
 * @return  Pointer to the (file-static) main viewport, or NULL if no context is
 *          current. The pointer is stable across frames; its fields are updated
 *          on each call, so do not cache the values across a display resize.
 *
 * Thread-safe: no (reads the global current context)
 * Reentrant: no
 */
static inline eli_viewport *eli_get_main_viewport(void)
{
    static eli_viewport main_viewport;

    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return NULL;

    main_viewport.id = ELI_VIEWPORT_MAIN_ID;
    main_viewport.flags = ELI_VIEWPORT_FLAGS_IS_PRIMARY;
    main_viewport.pos = eli_make_vec2(0.0f, 0.0f);
    main_viewport.size = ctx->io.display_size;
    main_viewport.work_pos = main_viewport.pos;
    main_viewport.work_size = main_viewport.size;
    return &main_viewport;
}

#endif /* ELI_UTIL_ELI_VIEWPORT_H */
