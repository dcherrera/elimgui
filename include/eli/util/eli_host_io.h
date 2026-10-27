/**
 * @file eli_host_io.h
 * @brief Host services behind one seam: read/write a text file, print to the TTY.
 *
 * Settings and logging call these instead of touching stdio or JS directly:
 *   - ELI_JSIO (wasm32 + JAClibc): JS host imports provided by web/elimgui.js.
 *   - ELI_LIBC_SYSTEM (native apps, tests): stdio (fopen/fread/fwrite/fputs).
 *   - JAClibc on a native target: no host services; calls report failure.
 * Paths are used as given. On iOS the cwd is the read-only bundle, so apps pass an
 * absolute path (e.g. from SDL_GetPrefPath).
 *
 * @status In use by eli_settings.h and eli_logging.h.
 * @issues None
 * @todo None
 */
#ifndef ELI_UTIL_HOST_IO_H
#define ELI_UTIL_HOST_IO_H

#include "../core/eli_platform.h"

#include "eli_mem.h"

#ifdef ELI_JSIO
/* Host page provides these (web/elimgui.js). read returns a heap C string the C
 * side frees with eli_mem_free, or NULL; the writers return 0 on success. */
JS_IMPORT(eli_host_read_file) char *eli_host_read_file(const char *path);
JS_IMPORT(eli_host_write_file) int eli_host_write_file(const char *path, const char *text);
JS_IMPORT(eli_host_log_print) void eli_host_log_print(const char *text);
JS_IMPORT(eli_host_log_write_file) int eli_host_log_write_file(const char *path,
                                                               const char *text);
#endif /* ELI_JSIO */

#if defined(ELI_LIBC_SYSTEM)
/** Read an open stream to EOF into a NUL-terminated eli_mem buffer, or NULL. */
static inline char *eli_host_io__read_stream(FILE *f)
{
    size_t cap = 4096, len = 0;
    char *buf = (char *)eli_mem_alloc(cap);
    while (buf) {
        len += fread(buf + len, 1, cap - len - 1, f);
        if (len < cap - 1)
            break;
        char *grown = (char *)eli_mem_alloc(cap * 2);
        if (grown)
            memcpy(grown, buf, len);
        eli_mem_free(buf);
        buf = grown;
        cap *= 2;
    }
    if (!buf)
        return NULL;
    if (ferror(f)) {
        eli_mem_free(buf);
        return NULL;
    }
    buf[len] = '\0';
    return buf;
}
#endif /* ELI_LIBC_SYSTEM */

/**
 * Read a whole text file.
 *
 * @param path  File path (NULL returns NULL).
 * @return      NUL-terminated contents allocated with eli_mem_alloc (caller frees
 *              with eli_mem_free), or NULL if missing/unreadable/unsupported.
 *
 * Thread-safe: yes (no shared state)  Reentrant: yes  Signal-safe: no
 */
static inline char *eli_host_io_read_text_file(const char *path)
{
    if (!path)
        return NULL;
#if defined(ELI_JSIO)
    return eli_host_read_file(path);
#elif defined(ELI_LIBC_SYSTEM)
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;
    char *text = eli_host_io__read_stream(f);
    fclose(f);
    return text;
#else
    return NULL;
#endif
}

/**
 * Write (replace) or append text to a file.
 *
 * @param path    File path.
 * @param text    NUL-terminated text.
 * @param append  true appends, false truncates first.
 * @return        true on success.
 *
 * Thread-safe: yes (no shared state)  Reentrant: yes  Signal-safe: no
 */
static inline bool eli_host_io_write_text_file(const char *path, const char *text, bool append)
{
    if (!path || !text)
        return false;
#if defined(ELI_JSIO)
    return (append ? eli_host_log_write_file(path, text) : eli_host_write_file(path, text)) == 0;
#elif defined(ELI_LIBC_SYSTEM)
    FILE *f = fopen(path, append ? "ab" : "wb");
    if (!f)
        return false;
    size_t len = strlen(text);
    bool ok = fwrite(text, 1, len, f) == len;
    return (fclose(f) == 0) && ok;
#else
    (void)append;
    return false;
#endif
}

/**
 * Print text to the developer console (browser) or stdout (native).
 *
 * @param text  NUL-terminated text (NULL is ignored).
 *
 * Thread-safe: as stdio  Reentrant: yes  Signal-safe: no
 */
static inline void eli_host_io_print(const char *text)
{
    if (!text)
        return;
#if defined(ELI_JSIO)
    eli_host_log_print(text);
#elif defined(ELI_LIBC_SYSTEM)
    fputs(text, stdout);
    fflush(stdout);
#endif
}

#endif /* ELI_UTIL_HOST_IO_H */
