/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file unity.c
 * @brief Size probe, 1-TU variant: the same five files compiled as one TU.
 *
 * @status In use by tu_size_probe.sh.
 * @issues None
 * @todo None
 */
#include "probe_main.c"
#define PROBE_UI_FN probe_ui_1
#include "probe_ui.h"
#undef PROBE_UI_FN
#define PROBE_UI_FN probe_ui_2
#include "probe_ui.h"
#undef PROBE_UI_FN
#define PROBE_UI_FN probe_ui_3
#include "probe_ui.h"
#undef PROBE_UI_FN
#define PROBE_UI_FN probe_ui_4
#include "probe_ui.h"
