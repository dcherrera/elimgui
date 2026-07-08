/**
 * @file eli_draw_channels.h
 * @brief Draw-list channel splitting: temporarily partition a list into ordered
 *        command/index streams so out-of-order emission can be merged back into
 *        a single correctly-ordered command buffer.
 *
 * Only commands and indices are split; vertices stay shared in the parent list.
 * Typical use draws lower channels first (background) and higher channels last
 * (foreground), then merges. Nested splitting is not supported.
 *
 * @status Phase 2 channels in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_DRAW_ELI_DRAW_CHANNELS_H
#define ELI_DRAW_ELI_DRAW_CHANNELS_H

#include "eli_draw_types.h"
#include "eli_draw_list.h"

#include "../core/eli_platform.h"
#include "../core/eli_types.h"

/**
 * Split the draw list into `count` channels. Channel 0 keeps the current live
 * buffers; channels 1..count-1 start empty. Draw into channels via
 * eli_draw_list_channels_set_current, then rejoin with
 * eli_draw_list_channels_merge.
 *
 * @param list   Target draw list (must not already be split).
 * @param count  Number of channels (>= 1).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_channels_split(eli_draw_list *list, int count)
{
    if (count < 1)
        return;

    int old_capacity = list->channels_capacity;
    list->channels = (eli_draw_channel *)eli_draw_buf_grow(
        list->channels, &list->channels_capacity, count, sizeof(*list->channels));
    if (list->channels_capacity > old_capacity)
        memset(&list->channels[old_capacity], 0,
               (size_t)(list->channels_capacity - old_capacity) * sizeof(*list->channels));

    list->channels_count = count;
    list->channels_current = 0;
    /* Channel 0 is a placeholder overwritten when we first switch away from it;
     * channels 1..count-1 are reset for reuse (buffers kept for later frames). */
    for (int i = 1; i < count; i++) {
        list->channels[i].cmd_count = 0;
        list->channels[i].idx_count = 0;
    }
}

/**
 * Switch the active channel. The live command/index buffers are stashed into
 * the current channel and the target channel's buffers become live. A valid
 * trailing command matching the current clip/texture is ensured.
 *
 * @param list  Target draw list (must be split).
 * @param n     Channel index in [0, channels_count).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_channels_set_current(eli_draw_list *list, int n)
{
    if (n < 0 || n >= list->channels_count || n == list->channels_current)
        return;

    eli_draw_channel *cur = &list->channels[list->channels_current];
    cur->cmds = list->cmds;
    cur->cmd_count = (int)list->cmd_count;
    cur->cmd_capacity = (int)list->cmd_capacity;
    cur->idx = list->idx;
    cur->idx_count = (int)list->idx_count;
    cur->idx_capacity = (int)list->idx_capacity;

    eli_draw_channel *next = &list->channels[n];
    list->cmds = next->cmds;
    list->cmd_count = (uint32_t)next->cmd_count;
    list->cmd_capacity = (uint32_t)next->cmd_capacity;
    list->idx = next->idx;
    list->idx_count = (uint32_t)next->idx_count;
    list->idx_capacity = (uint32_t)next->idx_capacity;
    list->channels_current = n;

    if (list->cmd_count == 0) {
        eli_draw_list_add_draw_cmd(list);
    } else {
        eli_draw_cmd *curr = &list->cmds[list->cmd_count - 1];
        eli_rect header_rect = eli_draw_clip_vec4_to_rect(list->cmd_clip_rect);
        if (curr->elem_count == 0) {
            curr->clip_rect = header_rect;
            curr->texture_id = list->cmd_texture_id;
        } else if (curr->texture_id != list->cmd_texture_id ||
                   curr->clip_rect.x != header_rect.x || curr->clip_rect.y != header_rect.y ||
                   curr->clip_rect.w != header_rect.w || curr->clip_rect.h != header_rect.h) {
            eli_draw_list_add_draw_cmd(list);
        }
    }
}

/** Append one command to the live command buffer, growing it as needed. */
static inline void eli_draw_channels_push_cmd(eli_draw_list *list, eli_draw_cmd cmd)
{
    int cap = (int)list->cmd_capacity;
    list->cmds = (eli_draw_cmd *)eli_draw_buf_grow(list->cmds, &cap, (int)list->cmd_count + 1,
                                                   sizeof(*list->cmds));
    list->cmd_capacity = (uint32_t)cap;
    list->cmds[list->cmd_count++] = cmd;
}

/**
 * Merge all channels back into channel 0, concatenating commands and indices in
 * channel order and fixing each command's index offset. Leaves the list with a
 * single channel.
 *
 * @param list  Target draw list (must be split).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_draw_list_channels_merge(eli_draw_list *list)
{
    if (list->channels_count <= 1)
        return;

    eli_draw_list_channels_set_current(list, 0);
    eli_draw_list_pop_unused_draw_cmd(list);

    for (int i = 1; i < list->channels_count; i++) {
        eli_draw_channel *ch = &list->channels[i];
        int cmd_count = ch->cmd_count;
        if (cmd_count > 0 && ch->cmds[cmd_count - 1].elem_count == 0 &&
            ch->cmds[cmd_count - 1].user_callback == NULL)
            cmd_count--;

        uint32_t idx_base = list->idx_count;
        if (ch->idx_count > 0) {
            int cap = (int)list->idx_capacity;
            list->idx = (eli_draw_idx *)eli_draw_buf_grow(
                list->idx, &cap, (int)list->idx_count + ch->idx_count, sizeof(*list->idx));
            list->idx_capacity = (uint32_t)cap;
            memcpy(&list->idx[list->idx_count], ch->idx,
                   (size_t)ch->idx_count * sizeof(*list->idx));
            list->idx_count += (uint32_t)ch->idx_count;
        }

        uint32_t running = idx_base;
        for (int j = 0; j < cmd_count; j++) {
            eli_draw_cmd cmd = ch->cmds[j];
            if (cmd.elem_count == 0 && cmd.user_callback == NULL)
                continue;
            cmd.idx_offset = running;
            running += cmd.elem_count;
            eli_draw_channels_push_cmd(list, cmd);
        }
        ch->cmd_count = 0;
        ch->idx_count = 0;
    }

    list->channels_count = 1;
    list->channels_current = 0;

    /* Dear ImGui invariant (ImDrawListSplitter::Merge): the command buffer must
     * always end with a non-callback draw command so a subsequent clip-rect or
     * texture stack op has a live "current" command to retag. A merge whose
     * channels contained only culled geometry (e.g. fully-transparent draws that
     * emitted no commands) can otherwise leave the buffer empty and dangle a
     * later pop_clip_rect. Restore the trailing command here. */
    if (list->cmd_count == 0 || list->cmds[list->cmd_count - 1].user_callback != NULL)
        eli_draw_list_add_draw_cmd(list);
}

#endif /* ELI_DRAW_ELI_DRAW_CHANNELS_H */
