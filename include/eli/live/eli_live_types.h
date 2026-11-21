/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_live_types.h
 * @brief Live widget types: kinds, pulse, the widget record backends read, the
 *        backend hook table, change bits and pool statistics.
 *
 * @status Live widgets core.
 * @issues None
 * @todo None
 *
 * A live widget is laid out by the immediate-mode pass but drawn by a platform
 * layer the OS compositor animates (docs/live-widgets.md). The core keeps a fixed
 * pool of eli_live_widget records in static storage and tells the backend only
 * what changed.
 */
#ifndef ELI_LIVE_TYPES_H
#define ELI_LIVE_TYPES_H

#include "../core/eli_platform.h"
#include "../core/eli_types.h"

/** Pool size: live widgets that can exist at once (override before including elimgui). */
#ifndef ELI_LIVE_MAX
#define ELI_LIVE_MAX 32
#endif

/** Inline text capacity of a live text widget, NUL included. */
#ifndef ELI_LIVE_TEXT_MAX
#define ELI_LIVE_TEXT_MAX 64
#endif

/** Seconds per spinner turn. */
#define ELI_LIVE_SPINNER_PERIOD 1.0f

/** Default slow-redraw interval of a fallback widget that animates (seconds). */
#define ELI_LIVE_FALLBACK_INTERVAL 0.25f

/** What a live widget draws. */
typedef enum eli_live_kind {
    ELI_LIVE_DOT = 0,     /* filled circle, optional opacity pulse */
    ELI_LIVE_SPINNER,     /* 270-degree arc turning once per ELI_LIVE_SPINNER_PERIOD */
    ELI_LIVE_PROGRESS,    /* track + fill of `fraction` */
    ELI_LIVE_TEXT,        /* one line of text */
    ELI_LIVE_KIND_COUNT
} eli_live_kind;

/** Bit for `kind` in eli_live_backend.kinds. */
#define ELI_LIVE_KIND_BIT(kind) (1u << (unsigned)(kind))

/**
 * Opacity pulse: 1 -> min_alpha -> 1 on a cosine over `period` seconds,
 * alpha = min + (1 - min) * (0.5 + 0.5 * cos(2 pi t / period)). period 0 = solid.
 */
typedef struct eli_live_pulse {
    float period;
    float min_alpha;
} eli_live_pulse;

/** Argus's working-dot pulse: 1.4 s, 0.3 .. 1.0. */
#define ELI_LIVE_PULSE_DEFAULT ((eli_live_pulse){ 1.4f, 0.3f })

/** eli_live_backend.update `changed` bits. */
enum {
    ELI_LIVE_CHANGED_GEOMETRY = 1u << 0,   /* rect or clip */
    ELI_LIVE_CHANGED_VISIBLE  = 1u << 1,   /* visible */
    ELI_LIVE_CHANGED_STYLE    = 1u << 2,   /* col, track_col, pulse, rounding */
    ELI_LIVE_CHANGED_VALUE    = 1u << 3,   /* fraction */
    ELI_LIVE_CHANGED_TEXT     = 1u << 4    /* text, font, font_size */
};

/**
 * One live widget. Backends read every field and own `handle`; the rest belongs
 * to the core. Coordinates are screen points (eli_draw_data space).
 */
typedef struct eli_live_widget {
    eli_live_kind kind;
    eli_id id;
    eli_rect rect;                  /* the widget's box */
    eli_rect clip;                  /* what of it may show (window + draw-list clip) */
    bool visible;                   /* false: occluded, clipped out, or drawn static by
                                       the main surface this frame */
    eli_col32 col;                  /* dot / arc / fill / text color */
    eli_col32 track_col;            /* progress track */
    eli_live_pulse pulse;           /* dot only; period 0 = solid */
    float rounding;                 /* progress corner radius */
    float fraction;                 /* progress, 0..1 */
    const struct eli_font *font;    /* text */
    float font_size;                /* text */
    char text[ELI_LIVE_TEXT_MAX];   /* text, NUL-terminated */
    void *handle;                   /* the backend's platform object */

    /* Core bookkeeping (backends: read-only, never rely on it). */
    bool used;                      /* slot in use */
    bool live;                      /* the backend created a layer for it */
    bool main_static;               /* the main surface draws it static this frame */
    unsigned pending;               /* change bits not yet sent to the backend */
    int seen_frame;                 /* frame_count of the last submit */
    const struct eli_draw_list *list;  /* owning draw list (z order) */
} eli_live_widget;

/**
 * Platform hooks. All run on the main thread, from eli_frame_end or a setter.
 * `create` gets a fully filled widget (visible false until the first frame end
 * says otherwise) and returns false when it cannot make a layer (the widget then
 * uses the fallback for its whole life); `update` gets the bits that changed;
 * `destroy` frees the layer and must clear `handle`.
 */
typedef struct eli_live_backend {
    void *user_data;
    unsigned kinds;   /* ELI_LIVE_KIND_BIT of each kind the backend draws */
    bool (*create)(void *user_data, eli_live_widget *w);
    void (*update)(void *user_data, eli_live_widget *w, unsigned changed);
    void (*destroy)(void *user_data, eli_live_widget *w);
} eli_live_backend;

/** Pool counters (eli_live_get_stats). Totals count since the last eli_live_shutdown. */
typedef struct eli_live_stats {
    int count;            /* widgets in the pool */
    int live;             /* of those, drawn by a backend layer */
    uint64_t creates;     /* backend create calls */
    uint64_t updates;     /* backend update calls */
    uint64_t destroys;    /* backend destroy calls */
    uint64_t overflow;    /* submits drawn by the fallback because the pool was full */
} eli_live_stats;

#endif /* ELI_LIVE_TYPES_H */
