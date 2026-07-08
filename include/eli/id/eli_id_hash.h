/**
 * @file eli_id_hash.h
 * @brief CRC32-based identity hashing for elimgui, matching Dear ImGui's
 *        ImHashStr/ImHashData semantics (standard reflected poly 0xEDB88320).
 *
 * The hash honors Dear ImGui's label conventions:
 *   - A plain "###" run resets the accumulator to the seed, so everything before
 *     it is discarded ("A###X" and "B###X" hash equal, giving stable IDs).
 *   - A "##" run is NOT special to the hash: the full string is hashed, so
 *     "Label" and "Label##x" differ (the visible-label truncation is a rendering
 *     concern handled elsewhere, not here).
 *
 * @status Phase 5 ID hashing in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_ID_ELI_ID_HASH_H
#define ELI_ID_ELI_ID_HASH_H

#include "../core/eli_platform.h"
#include "../core/eli_types.h"

/** Standard reflected CRC-32 polynomial (same table Dear ImGui uses). */
#define ELI_CRC32_POLY 0xEDB88320u

/** Number of entries in the byte-indexed CRC-32 lookup table. */
#define ELI_CRC32_LUT_SIZE 256

/**
 * Return the process-wide CRC-32 lookup table, building it on first use.
 * The table is derived from ELI_CRC32_POLY so no large literal blob is needed.
 *
 * @return  Pointer to a 256-entry table (never NULL).
 *
 * Thread-safe: no (lazily initializes a static table)
 * Reentrant: no
 */
static inline const uint32_t *eli_crc32_lut(void)
{
    static uint32_t table[ELI_CRC32_LUT_SIZE];
    static bool built = false;
    if (!built) {
        for (uint32_t i = 0; i < ELI_CRC32_LUT_SIZE; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++)
                c = (c & 1u) ? (ELI_CRC32_POLY ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        built = true;
    }
    return table;
}

/**
 * Hash a sized byte range into an eli_id, seeded by a parent id.
 * A "###" run inside the range resets the accumulator to the seed.
 *
 * @param data       Pointer to the bytes to hash (may be NULL only if size is 0).
 * @param data_size  Number of bytes to hash.
 * @param seed       Seed id (0 for a root hash; the ID-stack seed otherwise).
 * @return           The resulting identity hash.
 *
 * Thread-safe: no (uses the lazily-built CRC table)
 * Reentrant: no
 */
static inline eli_id eli_hash_data(const void *data, size_t data_size, eli_id seed)
{
    const uint32_t seeded = ~(uint32_t)seed;
    uint32_t crc = seeded;
    const unsigned char *p = (const unsigned char *)data;
    const uint32_t *lut = eli_crc32_lut();

    while (data_size-- != 0) {
        unsigned char c = *p++;
        if (c == '#' && data_size >= 2 && p[0] == '#' && p[1] == '#')
            crc = seeded;
        crc = (crc >> 8) ^ lut[(crc & 0xFFu) ^ c];
    }
    return (eli_id)~crc;
}

/**
 * Hash a NUL-terminated string into an eli_id, seeded by a parent id.
 * A "###" run inside the string resets the accumulator to the seed.
 *
 * @param str   NUL-terminated string to hash (must be non-NULL).
 * @param seed  Seed id (0 for a root hash; the ID-stack seed otherwise).
 * @return      The resulting identity hash.
 *
 * Thread-safe: no (uses the lazily-built CRC table)
 * Reentrant: no
 */
static inline eli_id eli_hash_str(const char *str, eli_id seed)
{
    const uint32_t seeded = ~(uint32_t)seed;
    uint32_t crc = seeded;
    const unsigned char *p = (const unsigned char *)str;
    const uint32_t *lut = eli_crc32_lut();
    unsigned char c;

    while ((c = *p++) != 0) {
        /* p already points past c; p[0]/p[1] read the two chars after c. If
         * data ends, p[0] is the NUL (not '#') so the check short-circuits. */
        if (c == '#' && p[0] == '#' && p[1] == '#')
            crc = seeded;
        crc = (crc >> 8) ^ lut[(crc & 0xFFu) ^ c];
    }
    return (eli_id)~crc;
}

#endif /* ELI_ID_ELI_ID_HASH_H */
