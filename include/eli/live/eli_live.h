/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_live.h
 * @brief Live widgets aggregator: widgets laid out by the immediate pass but drawn
 *        by platform layers the OS compositor animates, so the main surface stays
 *        idle while they animate (docs/live-widgets.md).
 *
 * @status Live widgets core.
 * @issues None
 * @todo None
 *
 *     eli_live_set_backend(&backend);          // or nothing: fallback drawing
 *     ... in a window ...
 *     eli_live_dot("busy", 4.0f, col, &(eli_live_pulse)ELI_LIVE_PULSE_DEFAULT);
 *     eli_id bar = eli_live_progress("job", eli_make_vec2(0, 0), done);
 *     ... between frames ...
 *     eli_live_set_fraction(bar, done);        // no frame with a backend
 */
#ifndef ELI_LIVE_H
#define ELI_LIVE_H

#include "eli_live_types.h"
#include "eli_live_pool.h"
#include "eli_live_text.h"
#include "eli_live_widgets.h"

#endif /* ELI_LIVE_H */
