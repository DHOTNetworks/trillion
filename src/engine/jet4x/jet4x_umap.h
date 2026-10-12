/**
 * @file jet4x_umap.h
 * @description jet4x usage-map writer — table maps, LVAL maps, global maps.
 * Formats mirror jackcess UsageMap (Inline/Reference handlers) as observed
 * in Bahi-Khata Data.001/Data.002 ground truth:
 * - def rows are 69B (5 + 64) on DATA pages (tdef 0) via slot machinery
 * - INLINE row: [0x00][startPage u32][bitmap]; startPage = align8(min)
 *   (0 when empty); capacity (rowLen-5)*8 pages
 * - REFERENCE row (69B): [0x01][17 x bitmap-page u32]; bitmap pages are
 *   type 0x05 with raw bits at offset 4
 * - table owned rows track DATA pages only; free rows track pages with
 *   free space; index/LVAL rows track their own pages (lval-first order)
 * - global page 1: row0 = INLINE free-page inventory (all zero = append
 *   at EOF), row1 = REFERENCE with one empty bitmap page (mirrors Data.002)
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "jet4x_core.h"

namespace jet4x {

inline constexpr size_t kUmapRowLen = 69;
inline constexpr size_t kUmapRefsPerRow = 17;

/** 8-alignment for inline start pages (matches observed files). */
inline uint32_t umapStartPage(uint32_t minPage) {
    return minPage - (minPage % 8);
}

struct UmapTableMaps {
    uint32_t umapPage = 0; /* allocated def page (DATA, tdef 0) */
    std::vector<uint32_t> dataPages; /* owned DATA pages */
    std::vector<uint32_t> freeDataPages; /* subset with free space */
    /* Per index (tdef order): owned index pages (root + levels). */
    std::vector<std::vector<uint32_t>> indexPages;
    struct LvalMaps {
        std::vector<uint32_t> owned;
        std::vector<uint32_t> free;
    };
    /* Per LVAL column IN COLUMN-NUMBER ORDER (matches tdef R refs). */
    std::vector<LvalMaps> lvals;
};

/**
 * Writes the table's usage-map definition page (rows 0,1 owned/free,
 * LVAL owned/free pairs, then index rows — lval-first, matching the tdef
 * umapRow numbering). Allocates bitmap pages from img when a map exceeds
 * inline capacity. Returns false + error on any anomaly (never partial:
 * page committed only when complete).
 */
bool writeUmapPage(DatabaseImage& img, const UmapTableMaps& m,
                   std::string& error);

/**
 * Writes the global usage-map page (page 1): row0 INLINE free inventory
 * (empty: every page owned), row1 REFERENCE with one empty bitmap page.
 * totalPages = final file page count (bitmap sized to cover it).
 */
bool writeGlobalPage(DatabaseImage& img, uint32_t totalPages,
                     std::string& error);

/** Writes one type-0x05 bitmap page (absolute page-number bits at @4). */
uint32_t writeBitmapPage(DatabaseImage& img,
                         const std::vector<uint32_t>& setPages,
                         std::string& error);

} /* namespace jet4x */
