/**
 * @file test_shared_state_b.c
 * @brief Second translation unit for test_shared_state.c (multi-TU state, D4).
 *
 * Compiled WITHOUT ELI_IMPLEMENTATION and linked into test_shared_state. Each
 * function touches library state from this TU so the main TU can check whether
 * both TUs see the same context, clipboard and allocator.
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#include <eli/elimgui.h>

/** @return the current context as seen from this TU. */
eli_context *shared_state_b_current_context(void)
{
    return eli_get_current_context();
}

/** Write clipboard text from this TU. */
void shared_state_b_set_clipboard(const char *text)
{
    eli_set_clipboard_text(text);
}

/** @return clipboard text as seen from this TU. */
const char *shared_state_b_get_clipboard(void)
{
    return eli_get_clipboard_text();
}

/** Allocate and free one block through eli_mem from this TU. */
void shared_state_b_alloc_and_free(void)
{
    void *p = eli_mem_alloc(16);
    eli_mem_free(p);
}
