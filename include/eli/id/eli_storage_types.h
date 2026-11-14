/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_storage_types.h
 * @brief eli_storage data layout: the sorted key/value pair array.
 *
 * Split out of eli_storage.h so the context can embed a storage by value (the
 * window registry's id -> window map) without including the storage accessors,
 * which themselves depend on the context.
 *
 * @status In use by eli_storage.h and the context.
 * @issues None
 * @todo None
 */
#ifndef ELI_ID_ELI_STORAGE_TYPES_H
#define ELI_ID_ELI_STORAGE_TYPES_H

#include "../core/eli_types.h"

/** One key/value entry. The value union is selected by the accessor used. */
typedef struct eli_storage_pair {
    eli_id key;
    union {
        int   val_i;
        float val_f;
        void *val_p;
    };
} eli_storage_pair;

/**
 * Sorted key/value map. Zero-initialize (all fields 0/NULL) before first use;
 * the accessors grow it on demand. Release with eli_storage_clear.
 */
typedef struct eli_storage {
    eli_storage_pair *data;
    int size;
    int capacity;
} eli_storage;

#endif /* ELI_ID_ELI_STORAGE_TYPES_H */
