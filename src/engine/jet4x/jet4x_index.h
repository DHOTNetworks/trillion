/**
 * @file jet4x_index.h
 * @description jet4x index key encoder + B-tree bulk builder (libmdb-free).
 * Key bytes mirror the proven Jet4Writer::ColumnEncoder semantics exactly
 * (verified against Bahi-Khata native keys --audit-keys lineage):
 * ASC start 0x7F / null 0x00, DESC start 0x80 / null 0xFF; integers
 * big-endian flip-first-bit (inverted fully when DESC); doubles
 * sign-magnitude ordered; TEXT via the libmdb linguistic map + terminator.
 * Page codec mirrors BTreeEngine::writeLeaf (prefix policy, entry mask,
 * sibling links, levels, childTail).
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "jet4x_core.h"
#include "jet4x_schema.h"

namespace jet4x {

/* Key flags (jet4_types parity). */
inline constexpr uint8_t kKeyAscStart = 0x7F;
inline constexpr uint8_t kKeyAscNull = 0x00;
inline constexpr uint8_t kKeyDescStart = 0x80;
inline constexpr uint8_t kKeyDescNull = 0xFF;

/* Index page layout (Jet4). */
inline constexpr uint32_t kIxPrev = 12;
inline constexpr uint32_t kIxNext = 16;
inline constexpr uint32_t kIxTail = 20;
inline constexpr uint32_t kIxPrefixLen = 24;
inline constexpr uint32_t kIxLevel = 26;
inline constexpr uint32_t kIxMask = 27;
inline constexpr uint32_t kIxMaskSize = 453;
inline constexpr uint32_t kIxArea = 480;

/** One key column's stored value for key encoding. */
struct KeyColumn {
    DataType type = DataType::Text;
    uint16_t length = 0; /* TEXT max / BINARY max (raw cap rule) */
    const uint8_t* data = nullptr; /* stored bytes (may be empty) */
    size_t size = 0;
    bool isNull = true;  /* mask-authoritative null */
    bool descending = false;
};

/**
 * Encodes one full index entry: key segments + 3-byte page-BE + 1-byte
 * slot trailer. Returns false + error on oversize/unsupported.
 */
bool encodeIndexEntry(const std::vector<KeyColumn>& keys, uint32_t dataPg,
                      uint8_t slot, std::vector<uint8_t>& out,
                      std::string& error);

struct IndexBuildResult {
    uint32_t root = 0;
    uint8_t levels = 0; /* 1 = single leaf root */
    std::vector<uint32_t> allPages; /* every index page, creation order */
    size_t entryCount = 0;
};

/**
 * Bottom-up bulk build from unsorted entries (sorts by key, then rowId).
 * Empty input yields a single empty root leaf. Pages are sibling-linked
 * at every level; nodes carry dividers for all-but-last child + tail.
 */
bool buildIndex(DatabaseImage& img, uint32_t tdefPg,
                std::vector<std::vector<uint8_t>> entries,
                IndexBuildResult& out, std::string& error);

} /* namespace jet4x */
