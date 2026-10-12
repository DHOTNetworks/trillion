/**
 * @file jet4x_pages.h
 * @description jet4x page assemblers: table data pages and LVAL pages.
 * Slot mechanics mirror libmdb mdb_find_row (slots @14+2s, rows from end).
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "jet4x_core.h"

namespace jet4x {

/** LVAL placement thresholds (jackcess JetFormat Jet4). */
inline constexpr size_t kMaxInlineLval = 64;
inline constexpr size_t kMaxLvalRow = 4076;
inline constexpr size_t kMaxRowSize = 4060;

/* Data-page header offsets. */
inline constexpr uint32_t kDpFree = 2;
inline constexpr uint32_t kDpTdef = 4;
inline constexpr uint32_t kDpRows = 12;
inline constexpr uint32_t kDpSlots = 14;

/** LVAL page magic at [4..7]. */
inline constexpr uint8_t kLvalMagic[4] = {'L', 'V', 'A', 'L'};

/** Allocates a table data page bound to tdefPg (free=4082, rows=0). */
uint32_t newDataPage(DatabaseImage& img, uint32_t tdefPg);

/** Allocates an LVAL page ("LVAL" magic, rows=0). */
uint32_t newLvalPage(DatabaseImage& img);

/**
 * Appends a raw row to a data/LVAL page (canonical slot layout).
 * Fails cleanly when the row does not fit (caller allocates a new page).
 */
bool addPageRow(DatabaseImage& img, uint32_t pg, const uint8_t* row,
                size_t len, std::string& error);

/**
 * Chains a long value across LVAL pages.
 * Single row when len <= kMaxLvalRow, else [nextRow,nextPage3]+chunk rows.
 * Returns first (page,row); appends every used (page,row) to pagesOut.
 */
bool writeLvalChain(DatabaseImage& img, const uint8_t* data, size_t len,
                    uint32_t& outPg, uint8_t& outRow,
                    std::vector<std::pair<uint32_t, uint8_t>>& pagesOut,
                    std::string& error);

} /* namespace jet4x */
