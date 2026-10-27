/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_input_queue.h
 * @brief The buffered input-event queue primitive: append (growing on the heap)
 *        and the mouse-position coalescing rule. Shared by the event adders in
 *        eli_input_backend.h and the text adders in eli_input_text.h.
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#ifndef ELI_INPUT_ELI_INPUT_QUEUE_H
#define ELI_INPUT_ELI_INPUT_QUEUE_H

#include "../core/eli_core.h"
#include "../util/eli_mem.h"

/**
 * Make room for one more event, doubling the heap queue as needed.
 *
 * @return false if the queue could not grow (out of memory).
 */
static inline bool eli_input__queue_reserve(eli_io *io)
{
    if (io->input_events_count < io->input_events_capacity)
        return true;
    int cap = io->input_events_capacity > 0 ? io->input_events_capacity * 2
                                            : ELI_INPUT_EVENT_QUEUE_SIZE;
    eli_input_event *grown = (eli_input_event *)eli_mem_realloc(
        io->input_events, (size_t)io->input_events_capacity * sizeof(*grown),
        (size_t)cap * sizeof(*grown));
    if (!grown)
        return false;
    io->input_events = grown;
    io->input_events_capacity = cap;
    return true;
}

/**
 * Append one event and return its slot (type set, data to be filled by the
 * caller). A MOUSE_POS directly after another MOUSE_POS reuses that slot
 * (coalescing): only the latest position matters when nothing happened between.
 *
 * @param io    Target IO.
 * @param type  Event type.
 * @return      The slot, or NULL if the queue could not grow (event dropped).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline eli_input_event *eli_input_push_event(eli_io *io, eli_input_event_type type)
{
    if (type == ELI_INPUT_EVENT_TYPE_MOUSE_POS && io->input_events_count > 0 &&
        io->input_events[io->input_events_count - 1].type == ELI_INPUT_EVENT_TYPE_MOUSE_POS)
        return &io->input_events[io->input_events_count - 1];
    if (!eli_input__queue_reserve(io))
        return NULL;
    eli_input_event *e = &io->input_events[io->input_events_count++];
    memset(e, 0, sizeof(*e));
    e->type = type;
    return e;
}

#endif /* ELI_INPUT_ELI_INPUT_QUEUE_H */
