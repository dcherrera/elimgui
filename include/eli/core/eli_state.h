/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_state.h
 * @brief Storage-class macros for elimgui's file-scope state (single- or multi-TU).
 *
 * Every piece of library state that lives at file scope (current context, widget
 * pools, clipboard fallback, allocator hooks, ...) is declared through these
 * macros so one build flag decides how it is linked:
 *
 *   - default (no ELI_SHARED_STATE): `static` in every translation unit. This is
 *     the single-TU model the WASM demo uses and is byte-for-byte the old layout.
 *   - ELI_SHARED_STATE: `extern` in every TU, defined (with its initializer) only
 *     in the one TU that defines ELI_IMPLEMENTATION before including elimgui. A
 *     multi-file app then shares one context, one clipboard, one allocator.
 *
 * Usage:
 *     ELI_GLOBAL(eli_context *, g_eli_context, NULL);
 *     ELI_GLOBAL_ARRAY(char, g_eli_buf, 64);
 *     ELI_GLOBAL_ARRAY_INIT(char, g_eli_hex, 16, "#000000");
 *
 * Names stay `eli_`/`g_eli_`-prefixed because in shared mode they are link-visible.
 *
 * @status Used by every header that holds file-scope state.
 * @issues None
 * @todo None
 */
#ifndef ELI_CORE_STATE_H
#define ELI_CORE_STATE_H

#if defined(ELI_SHARED_STATE) && !defined(ELI_IMPLEMENTATION)
    #define ELI_GLOBAL(type, name, ...)                 extern type name
    #define ELI_GLOBAL_ARRAY(type, name, n)             extern type name[n]
    #define ELI_GLOBAL_ARRAY_INIT(type, name, n, ...)   extern type name[n]
#elif defined(ELI_SHARED_STATE)
    #define ELI_GLOBAL(type, name, ...)                 type name = __VA_ARGS__
    #define ELI_GLOBAL_ARRAY(type, name, n)             type name[n]
    #define ELI_GLOBAL_ARRAY_INIT(type, name, n, ...)   type name[n] = __VA_ARGS__
#else
    #define ELI_GLOBAL(type, name, ...)                 static type name = __VA_ARGS__
    #define ELI_GLOBAL_ARRAY(type, name, n)             static type name[n]
    #define ELI_GLOBAL_ARRAY_INIT(type, name, n, ...)   static type name[n] = __VA_ARGS__
#endif

#endif /* ELI_CORE_STATE_H */
