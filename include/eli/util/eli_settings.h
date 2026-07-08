/**
 * @file eli_settings.h
 * @brief INI settings persistence for elimgui: parse/serialize per-window
 *        geometry ([Window][Name] Pos/Size/Collapsed) into a file-static store,
 *        with memory and (JS-hooked) disk variants and a lookup accessor.
 *
 * The store is a flat, growable array of window entries kept in file-scope
 * statics (single-TU header-only build model, matching eli_mem.h). Saving reads
 * the live window registry (ctx->windows, read-only); loading fills the store so
 * a caller can re-apply an entry via eli_set_next_window_pos/_size/_collapsed
 * before eli_begin. Automatic apply-in-Begin is intentionally deferred.
 *
 * Disk I/O is browser-only and guarded by ELI_JSIO: *_from_disk/_to_disk call a
 * JS host hook. Under hosted (test) builds those hooks compile out and the disk
 * entry points become clearly-documented no-ops that report failure.
 *
 * @status Memory round-trip + lookup implemented; disk paths are JS-hooked.
 * @issues None
 * @todo Apply-in-Begin (auto-restore) is deferred to the window/frame phase.
 */
#ifndef ELI_UTIL_SETTINGS_H
#define ELI_UTIL_SETTINGS_H

#include "eli_mem.h"

#include "../core/eli_platform.h"

#include "../core/eli_context.h"
#include "../window/eli_window_types.h"

#include <stdio.h>
#include <stdlib.h>

/* Maximum stored window-name length (including the NUL terminator). Names longer
 * than this are truncated in the store; INI section headers are keyed on it. */
#ifndef ELI_SETTINGS_MAX_NAME
#define ELI_SETTINGS_MAX_NAME 128
#endif

/* Bytes reserved per serialized window section. A section is three short lines
 * plus the header, so this is comfortably above the worst case. */
#define ELI_SETTINGS_SECTION_BUFFER 256

/* ---------------------------------------------------------------------------
 * Store types + file-static state
 * ------------------------------------------------------------------------- */

/**
 * One persisted window entry: identity plus the geometry the INI captures.
 */
typedef struct eli_window_settings {
    char     name[ELI_SETTINGS_MAX_NAME];
    eli_vec2 pos;
    eli_vec2 size;
    bool     collapsed;
} eli_window_settings;

/* Growable settings store (single-TU header-only build). */
static eli_window_settings *eli_settings_store = NULL;
static size_t               eli_settings_store_count = 0;
static size_t               eli_settings_store_capacity = 0;

/* ---------------------------------------------------------------------------
 * Store management (internal)
 * ------------------------------------------------------------------------- */

/**
 * Locate a stored entry by exact name.
 *
 * @param name  Window name to match (NUL-terminated).
 * @return      Pointer to the entry, or NULL if absent.
 */
static inline eli_window_settings *eli_settings_find(const char *name)
{
    if (!name)
        return NULL;
    for (size_t i = 0; i < eli_settings_store_count; i++) {
        if (strcmp(eli_settings_store[i].name, name) == 0)
            return &eli_settings_store[i];
    }
    return NULL;
}

/**
 * Find an entry by name, creating a zero-initialized one if it does not exist.
 * The store array grows geometrically; on allocation failure returns NULL.
 *
 * @param name  Window name (truncated to ELI_SETTINGS_MAX_NAME - 1 chars).
 * @return      Pointer to the (existing or new) entry, or NULL on OOM.
 */
static inline eli_window_settings *eli_settings_get_or_add(const char *name)
{
    eli_window_settings *found = eli_settings_find(name);
    if (found)
        return found;

    if (eli_settings_store_count == eli_settings_store_capacity) {
        size_t new_cap = eli_settings_store_capacity ? eli_settings_store_capacity * 2 : 8;
        eli_window_settings *grown = (eli_window_settings *)
            eli_mem_alloc(new_cap * sizeof(*grown));
        if (!grown)
            return NULL;
        if (eli_settings_store) {
            memcpy(grown, eli_settings_store, eli_settings_store_count * sizeof(*grown));
            eli_mem_free(eli_settings_store);
        }
        eli_settings_store = grown;
        eli_settings_store_capacity = new_cap;
    }

    eli_window_settings *entry = &eli_settings_store[eli_settings_store_count++];
    memset(entry, 0, sizeof(*entry));
    size_t n = strlen(name);
    if (n >= ELI_SETTINGS_MAX_NAME)
        n = ELI_SETTINGS_MAX_NAME - 1;
    memcpy(entry->name, name, n);
    entry->name[n] = '\0';
    return entry;
}

/**
 * Clear the settings store and release its backing memory.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_settings_clear(void)
{
    if (eli_settings_store) {
        eli_mem_free(eli_settings_store);
        eli_settings_store = NULL;
    }
    eli_settings_store_count = 0;
    eli_settings_store_capacity = 0;
}

/* ---------------------------------------------------------------------------
 * Parsing helpers (internal)
 * ------------------------------------------------------------------------- */

/** Parse an "x,y" pair (NUL/whitespace-terminated) into a vec2. */
static inline bool eli_settings_parse_vec2(const char *text, eli_vec2 *out)
{
    char *end = NULL;
    float x = strtof(text, &end);
    if (end == text)
        return false;
    while (*end == ' ' || *end == '\t')
        end++;
    if (*end != ',')
        return false;
    const char *second = end + 1;
    char *end2 = NULL;
    float y = strtof(second, &end2);
    if (end2 == second)
        return false;
    out->x = x;
    out->y = y;
    return true;
}

/** Extract the window name from a "[Window][Name]" section header line. */
static inline bool eli_settings_parse_section(const char *line, char *out, size_t out_size)
{
    static const char prefix[] = "[Window][";
    size_t prefix_len = sizeof(prefix) - 1;
    if (strncmp(line, prefix, prefix_len) != 0)
        return false;
    const char *name = line + prefix_len;
    const char *close = strchr(name, ']');
    if (!close)
        return false;
    size_t len = (size_t)(close - name);
    if (len >= out_size)
        len = out_size - 1;
    memcpy(out, name, len);
    out[len] = '\0';
    return true;
}

/* ---------------------------------------------------------------------------
 * Memory load / save
 * ------------------------------------------------------------------------- */

/**
 * Parse INI text into the settings store. Unrecognized or malformed lines are
 * ignored; Pos/Size/Collapsed lines appearing before a section header are
 * dropped. Existing entries with matching names are updated in place.
 *
 * @param ini_data  INI text (need not be NUL-terminated).
 * @param ini_size  Length of ini_data in bytes.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_load_ini_settings_from_memory(const char *ini_data, size_t ini_size)
{
    if (!ini_data || ini_size == 0)
        return;

    char *copy = (char *)eli_mem_alloc(ini_size + 1);
    if (!copy)
        return;
    memcpy(copy, ini_data, ini_size);
    copy[ini_size] = '\0';

    eli_window_settings *cur = NULL;
    char section_name[ELI_SETTINGS_MAX_NAME];
    char *cursor = copy;
    while (*cursor) {
        char *line = cursor;
        char *newline = strchr(cursor, '\n');
        if (newline) {
            *newline = '\0';
            cursor = newline + 1;
        } else {
            cursor = line + strlen(line);
        }
        size_t line_len = strlen(line);
        if (line_len > 0 && line[line_len - 1] == '\r')
            line[line_len - 1] = '\0';

        if (line[0] == '[') {
            if (eli_settings_parse_section(line, section_name, sizeof(section_name)))
                cur = eli_settings_get_or_add(section_name);
            else
                cur = NULL;
        } else if (cur && strncmp(line, "Pos=", 4) == 0) {
            eli_settings_parse_vec2(line + 4, &cur->pos);
        } else if (cur && strncmp(line, "Size=", 5) == 0) {
            eli_settings_parse_vec2(line + 5, &cur->size);
        } else if (cur && strncmp(line, "Collapsed=", 10) == 0) {
            cur->collapsed = strtol(line + 10, NULL, 10) != 0;
        }
    }

    eli_mem_free(copy);
}

/** Serialize one window into buf; returns the byte count written (excl. NUL). */
static inline int eli_settings_format_window(char *buf, size_t buf_size, const eli_window *win)
{
    return snprintf(buf, buf_size,
                    "[Window][%s]\nPos=%d,%d\nSize=%d,%d\nCollapsed=%d\n\n",
                    win->name ? win->name : "",
                    (int)(win->pos.x + 0.5f), (int)(win->pos.y + 0.5f),
                    (int)(win->size_full.x + 0.5f), (int)(win->size_full.y + 0.5f),
                    win->collapsed ? 1 : 0);
}

/** True if a window should be persisted (named, top-level, saving allowed). */
static inline bool eli_settings_window_is_saveable(const eli_window *win)
{
    return win && win->name && (win->flags & ELI_WINDOW_NO_SAVED_SETTINGS) == 0;
}

/**
 * Serialize the current context's window states to a newly allocated INI string.
 * Windows flagged ELI_WINDOW_NO_SAVED_SETTINGS (children, popups, tooltips) and
 * unnamed windows are skipped.
 *
 * @param out_ini_size  If non-NULL, receives the string length (excl. NUL).
 * @return  Heap string owned by the caller (free with eli_mem_free), or NULL if
 *          there is no current context or on allocation failure.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline char *eli_save_ini_settings_to_memory(size_t *out_ini_size)
{
    if (out_ini_size)
        *out_ini_size = 0;

    eli_context *ctx = eli_get_current_context();
    if (!ctx)
        return NULL;

    char section[ELI_SETTINGS_SECTION_BUFFER];
    size_t total = 0;
    for (int i = 0; i < ctx->windows_count; i++) {
        const eli_window *win = ctx->windows[i];
        if (!eli_settings_window_is_saveable(win))
            continue;
        int n = eli_settings_format_window(section, sizeof(section), win);
        if (n > 0)
            total += (size_t)n;
    }

    char *out = (char *)eli_mem_alloc(total + 1);
    if (!out)
        return NULL;

    size_t offset = 0;
    for (int i = 0; i < ctx->windows_count; i++) {
        const eli_window *win = ctx->windows[i];
        if (!eli_settings_window_is_saveable(win))
            continue;
        int n = eli_settings_format_window(out + offset, total + 1 - offset, win);
        if (n > 0)
            offset += (size_t)n;
    }
    out[offset] = '\0';

    if (out_ini_size)
        *out_ini_size = offset;
    return out;
}

/* ---------------------------------------------------------------------------
 * Lookup accessor
 * ------------------------------------------------------------------------- */

/**
 * Fetch a stored window entry so the caller can re-apply it before eli_begin
 * (via eli_set_next_window_pos/_size/_collapsed).
 *
 * @param name          Window name to look up.
 * @param out_pos       If non-NULL, receives the stored position.
 * @param out_size      If non-NULL, receives the stored size.
 * @param out_collapsed If non-NULL, receives the stored collapsed flag.
 * @return  true if an entry named `name` exists in the store, false otherwise.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline bool eli_settings_get_window(const char *name, eli_vec2 *out_pos,
                                           eli_vec2 *out_size, bool *out_collapsed)
{
    const eli_window_settings *entry = eli_settings_find(name);
    if (!entry)
        return false;
    if (out_pos)
        *out_pos = entry->pos;
    if (out_size)
        *out_size = entry->size;
    if (out_collapsed)
        *out_collapsed = entry->collapsed;
    return true;
}

/* ---------------------------------------------------------------------------
 * Disk load / save (JS-hooked; no-op fallback under hosted builds)
 * ------------------------------------------------------------------------- */

#ifdef ELI_JSIO

/* Host page provides these: read a file's text by path (returns a heap C string
 * the C side frees, or NULL) and write text to a path (returns 0 on success). */
JS_IMPORT(eli_host_read_file) char *eli_host_read_file(const char *path);
JS_IMPORT(eli_host_write_file) int   eli_host_write_file(const char *path, const char *text);

#endif /* ELI_JSIO */

/**
 * Load INI settings from a file path.
 *
 * Under ELI_JSIO this calls the host read-file hook and parses the result. In
 * hosted/test builds there is no disk access: the call is a no-op.
 *
 * @param ini_filename  Path to the INI file.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_load_ini_settings_from_disk(const char *ini_filename)
{
    if (!ini_filename)
        return;
#ifdef ELI_JSIO
    char *text = eli_host_read_file(ini_filename);
    if (text) {
        eli_load_ini_settings_from_memory(text, strlen(text));
        eli_mem_free(text);
    }
#else
    (void)ini_filename; /* No disk access in hosted builds. */
#endif
}

/**
 * Save the current window states to a file path.
 *
 * Under ELI_JSIO this serializes via the memory variant and calls the host
 * write-file hook. In hosted/test builds there is no disk access: the settings
 * are serialized (to validate the emit path) and immediately freed; the call is
 * otherwise a no-op.
 *
 * @param ini_filename  Destination path.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline void eli_save_ini_settings_to_disk(const char *ini_filename)
{
    if (!ini_filename)
        return;
    size_t len = 0;
    char *text = eli_save_ini_settings_to_memory(&len);
    if (!text)
        return;
#ifdef ELI_JSIO
    eli_host_write_file(ini_filename, text);
#else
    (void)ini_filename; /* No disk access in hosted builds. */
#endif
    eli_mem_free(text);
}

#endif /* ELI_UTIL_SETTINGS_H */
