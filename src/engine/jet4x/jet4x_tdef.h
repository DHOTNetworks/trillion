/**
 * @file jet4x_tdef.h
 * @description jet4x table-definition page writer.
 * Byte layout mirrors jackcess TableImpl.writeTableDefinition (+ColumnImpl /
 * IndexData / IndexImpl writeDefinitions) for Jet4; values are canonical
 * little-endian. Column numbers/ids, varoffs, fixoffs and flags follow the
 * Bahi-Khata ground-truth rules (number-semantic, flag-driven placement).
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "jet4x_core.h"
#include "jet4x_schema.h"

namespace jet4x {

/* Magic numbers (jackcess TableImpl / IndexData). */
inline constexpr uint32_t kMagicTable = 1625;
inline constexpr uint32_t kMagicIndex = 1923;

/* Column flag bits (jackcess ColumnImpl). */
inline constexpr uint8_t kFlagFixedLen = 0x01;
inline constexpr uint8_t kFlagUpdatable = 0x02;

/* Default text sort order DWORD (General, from Bahi-Khata tdefs). */
inline constexpr uint32_t kSortGeneral = 0x00000409;

/**
 * Builds one table's complete definition image.
 * tdefPageNo/umapPageNo are the reserved pages for this table; indexRoots[i]
 * is the reserved empty root page for indexes[i]; lvalUmaps[j] places the
 * j-th long-value column in COLUMN-NUMBER order (ground truth: MSys R
 * refs run in num order). Unused index key slots are FF FF 00.
 * Output is the logical byte stream; use placeTdef() to paginate it.
 */
bool buildTdef(const TableSpec& spec, uint32_t tdefPageNo, uint32_t umapPageNo,
               const std::vector<uint32_t>& indexRoots,
               const std::vector<LvalUmap>& lvalUmaps,
               std::vector<uint8_t>& out, std::string& error);

/**
 * Paginates a buildTdef stream into image pages starting at tdefPageNo.
 * Single page when it fits (free space patched as PAGE_SIZE-total-8);
 * otherwise chained with next-pointers (Transactions-style). Allocates
 * continuation pages from img. Returns all tdef pages (first = tdefPageNo).
 */
std::vector<uint32_t> placeTdef(DatabaseImage& img, uint32_t tdefPageNo,
                                const std::vector<uint8_t>& stream);

} /* namespace jet4x */
