/**
 * @file jet4x_pages.cpp
 * @description jet4x page assembler implementation.
 */

#include "jet4x_pages.h"

namespace jet4x {

static void putU16(uint8_t* p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
}

static void putU32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}

uint32_t newDataPage(DatabaseImage& img, uint32_t tdefPg) {
    uint32_t pg = img.allocPage(kPgData);
    uint8_t* p = img.page(pg);
    p[1] = 0x01;
    putU16(p + kDpFree, (uint16_t)(kPageSize - 14));
    putU32(p + kDpTdef, tdefPg);
    return pg;
}

uint32_t newIndexRoot(DatabaseImage& img, uint32_t tdefPg) {
    uint32_t pg = img.allocPage(kPgIndexLeaf);
    uint8_t* p = img.page(pg);
    p[1] = 0x01;
    putU16(p + kDpFree, (uint16_t)(kPageSize - kIndexRootUsed));
    putU32(p + kDpTdef, tdefPg);
    return pg;
}

uint32_t newLvalPage(DatabaseImage& img) {
    uint32_t pg = img.allocPage(kPgData);
    uint8_t* p = img.page(pg);
    p[1] = 0x01;
    putU16(p + kDpFree, (uint16_t)(kPageSize - 14));
    p[4] = kLvalMagic[0];
    p[5] = kLvalMagic[1];
    p[6] = kLvalMagic[2];
    p[7] = kLvalMagic[3];
    return pg;
}

bool addPageRow(DatabaseImage& img, uint32_t pg, const uint8_t* row,
                size_t len, std::string& error) {
    uint8_t* p = img.page(pg);
    if (!p) {
        error = "addPageRow: bad page";
        return false;
    }
    uint16_t nrows = (uint16_t)(p[kDpRows] | ((uint16_t)p[kDpRows + 1] << 8));
    if (nrows > 512) {
        error = "addPageRow: insane row count";
        return false;
    }
    /* Lowest live row start (page end when empty). */
    uint16_t low = kPageSize;
    for (uint16_t s = 0; s < nrows; s++) {
        uint16_t so =
            (uint16_t)(p[kDpSlots + 2 * s] | ((uint16_t)p[kDpSlots + 2 * s + 1] << 8));
        uint16_t off = so & 0x1FFF;
        if ((so & 0xC000) == 0 && off < low)
            low = off;
    }
    /* Consistency + fit, overflow-proof (all unsigned-widened). */
    uint32_t slotsEnd = (uint32_t)kDpSlots + 2u * (uint32_t)nrows;
    if ((uint32_t)low < slotsEnd + 2u) {
        error = "addPageRow: page inconsistent";
        return false;
    }
    if ((uint32_t)len > (uint32_t)low - slotsEnd - 2u) {
        error = "addPageRow: row does not fit";
        return false;
    }
    uint16_t start = (uint16_t)((uint32_t)low - (uint32_t)len);
    memcpy(p + start, row, len);
    p[kDpSlots + 2 * nrows] = (uint8_t)(start & 0xFF);
    p[kDpSlots + 2 * nrows + 1] = (uint8_t)((start >> 8) & 0xFF);
    nrows++;
    p[kDpRows] = (uint8_t)(nrows & 0xFF);
    p[kDpRows + 1] = (uint8_t)((nrows >> 8) & 0xFF);
    uint16_t used = (uint16_t)(kPageSize - start);
    uint16_t free =
        (uint16_t)(kPageSize - kDpSlots - 2 * nrows - used);
    putU16(p + kDpFree, free);
    return true;
}

bool writeLvalChain(DatabaseImage& img, const uint8_t* data, size_t len,
                    uint32_t& outPg, uint8_t& outRow,
                    std::vector<std::pair<uint32_t, uint8_t>>& pagesOut,
                    std::string& error) {
    if (len == 0) {
        error = "writeLvalChain: use inline form for empty values";
        return false;
    }
    if (len <= kMaxLvalRow) {
        uint32_t pg = newLvalPage(img);
        uint8_t* p = img.page(pg);
        (void)p;
        if (!addPageRow(img, pg, data, len, error))
            return false;
        outPg = pg;
        outRow = 0;
        pagesOut.push_back({pg, 0});
        return true;
    }
    /* Multi-page chain: rows of [nextRow,nextPage3]+chunk, next pointers
     * patched after all pages land (placeholder zeros first). */
    size_t pos = 0;
    while (pos < len) {
        size_t chunk = len - pos;
        if (chunk > kMaxLvalRow - 4)
            chunk = kMaxLvalRow - 4;
        uint32_t pg = newLvalPage(img);
        uint8_t* pp0 = img.page(pg);
        uint8_t myRow = (uint8_t)(pp0[kDpRows] |
                                  ((uint16_t)pp0[kDpRows + 1] << 8));
        std::vector<uint8_t> row(4 + chunk, 0);
        memcpy(row.data() + 4, data + pos, chunk);
        if (!addPageRow(img, pg, row.data(), row.size(), error))
            return false;
        pagesOut.push_back({pg, myRow});
        pos += chunk;
    }
    outPg = pagesOut[0].first;
    outRow = pagesOut[0].second;
    for (size_t i = 0; i + 1 < pagesOut.size(); i++) {
        uint8_t* pp = img.page(pagesOut[i].first);
        uint16_t so = (uint16_t)(pp[kDpSlots + 2 * pagesOut[i].second] |
                                 ((uint16_t)pp[kDpSlots + 2 * pagesOut[i].second + 1] << 8));
        uint16_t st = so & 0x1FFF;
        pp[st] = pagesOut[i + 1].second;
        pp[st + 1] = (uint8_t)(pagesOut[i + 1].first & 0xFF);
        pp[st + 2] = (uint8_t)((pagesOut[i + 1].first >> 8) & 0xFF);
        pp[st + 3] = (uint8_t)((pagesOut[i + 1].first >> 16) & 0xFF);
    }
    return true;
}

} /* namespace jet4x */
