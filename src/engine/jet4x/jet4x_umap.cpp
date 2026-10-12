/**
 * @file jet4x_umap.cpp
 * @description jet4x usage-map writer implementation.
 */

#include "jet4x_umap.h"

#include <algorithm>

#include "jet4x_pages.h"

namespace jet4x {

namespace {

void putU32v(std::vector<uint8_t>& b, uint32_t v) {
    b.push_back((uint8_t)(v & 0xFF));
    b.push_back((uint8_t)((v >> 8) & 0xFF));
    b.push_back((uint8_t)((v >> 16) & 0xFF));
    b.push_back((uint8_t)((v >> 24) & 0xFF));
}

/* Builds one 69B INLINE def row: [0x00][startPage][bitmap]. */
std::vector<uint8_t> inlineRow(const std::vector<uint32_t>& pages) {
    std::vector<uint8_t> r(kUmapRowLen, 0);
    r[0] = 0x00;
    uint32_t start = 0;
    if (!pages.empty()) {
        uint32_t mn = *std::min_element(pages.begin(), pages.end());
        start = umapStartPage(mn);
    }
    r[1] = (uint8_t)(start & 0xFF);
    r[2] = (uint8_t)((start >> 8) & 0xFF);
    r[3] = (uint8_t)((start >> 16) & 0xFF);
    r[4] = (uint8_t)((start >> 24) & 0xFF);
    for (uint32_t pg : pages) {
        uint32_t bit = pg - start;
        if (5 + bit / 8 >= kUmapRowLen)
            return {}; /* span overflow: caller must use reference */
        r[5 + bit / 8] |= (uint8_t)(1 << (bit % 8));
    }
    return r;
}

/* Capacity of an inline row in pages. */
size_t inlineCapacity() { return (kUmapRowLen - 5) * 8; }

/* Max span (max-min+1) of a page set; 0 when empty. */
uint32_t pageSpan(const std::vector<uint32_t>& pages) {
    if (pages.empty())
        return 0;
    auto mm = std::minmax_element(pages.begin(), pages.end());
    return *mm.second - *mm.first + 1;
}

} /* namespace */

uint32_t writeBitmapPage(DatabaseImage& img,
                         const std::vector<uint32_t>& setPages,
                         std::string& error) {
    uint32_t pg = img.allocPage(kPgUsageMap);
    uint8_t* p = img.page(pg);
    p[1] = 0x01;
    p[2] = 0;
    p[3] = 0;
    for (uint32_t page : setPages) {
        if (page / 8 >= kPageSize - 4) {
            error = "writeBitmapPage: page out of range";
            return 0;
        }
        p[4 + page / 8] |= (uint8_t)(1 << (page % 8));
    }
    return pg;
}

static bool appendUmapRow(DatabaseImage& img, uint32_t umapPg,
                          const std::vector<uint8_t>& row,
                          std::string& error) {
    if (row.size() != kUmapRowLen) {
        error = "appendUmapRow: bad row length";
        return false;
    }
    return addPageRow(img, umapPg, row.data(), row.size(), error);
}

bool writeUmapPage(DatabaseImage& img, const UmapTableMaps& m,
                   std::string& error) {
    /* Row order is LVAL-FIRST (matches the tdef umapRow numbering:
     * owned 0, free 1, LVAL owned/free pairs, then index rows).
     * Jackcess creates index-first, but Bahi-Khata/Access files are
     * lval-first (TempSelf/NewTable/saleTrans ground truth). */
    struct RowReq {
        std::vector<uint32_t> pages;
    };
    std::vector<RowReq> reqs;
    reqs.push_back({m.dataPages});
    reqs.push_back({m.freeDataPages});
    for (const auto& lv : m.lvals) {
        reqs.push_back({lv.owned});
        reqs.push_back({lv.free});
    }
    for (const auto& ip : m.indexPages)
        reqs.push_back({ip});
    for (const auto& rq : reqs) {
        if (pageSpan(rq.pages) <= inlineCapacity()) {
            std::vector<uint8_t> row = inlineRow(rq.pages);
            if (row.empty()) {
                error = "inlineRow overflow";
                return false;
            }
            if (!appendUmapRow(img, m.umapPage, row, error))
                return false;
        } else {
            /* REFERENCE row with one bitmap page (32736 capacity covers
             * any single-row map; multi-ref rows unneeded in practice). */
            uint32_t bmp = writeBitmapPage(img, rq.pages, error);
            if (!bmp)
                return false;
            std::vector<uint8_t> r(kUmapRowLen, 0);
            r[0] = 0x01;
            r[1] = (uint8_t)(bmp & 0xFF);
            r[2] = (uint8_t)((bmp >> 8) & 0xFF);
            r[3] = (uint8_t)((bmp >> 16) & 0xFF);
            r[4] = (uint8_t)((bmp >> 24) & 0xFF);
            if (!appendUmapRow(img, m.umapPage, r, error))
                return false;
        }
    }
    return true;
}

bool writeGlobalPage(DatabaseImage& img, uint32_t totalPages,
                     std::string& error) {
    /* Page 1 must already exist (allocated by the assembler). */
    uint8_t* p = img.page(1);
    if (!p) {
        error = "writeGlobalPage: page 1 missing";
        return false;
    }
    /* Row0: INLINE free inventory, empty (every page owned). Sized to
     * cover totalPages (mirrors Data.002's fitted bitmap). */
    size_t mapBytes = (totalPages + 7) / 8;
    std::vector<uint8_t> r0(5 + mapBytes, 0);
    r0[0] = 0x00;
    if (!addPageRow(img, 1, r0.data(), r0.size(), error))
        return false;
    /* Row1: REFERENCE with one empty bitmap page (mirrors Data.002). */
    uint32_t bmp = writeBitmapPage(img, {}, error);
    if (!bmp)
        return false;
    std::vector<uint8_t> r1(kUmapRowLen, 0);
    r1[0] = 0x01;
    r1[1] = (uint8_t)(bmp & 0xFF);
    r1[2] = (uint8_t)((bmp >> 8) & 0xFF);
    r1[3] = (uint8_t)((bmp >> 16) & 0xFF);
    r1[4] = (uint8_t)((bmp >> 24) & 0xFF);
    if (!addPageRow(img, 1, r1.data(), r1.size(), error))
        return false;
    return true;
}

} /* namespace jet4x */
