/**
 * @file jet4x_catalog.cpp
 * @description jet4x catalog tools implementation.
 */

#include "jet4x_catalog.h"

namespace jet4x {

namespace {

uint16_t rU16(const uint8_t* p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}
uint32_t rU32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
void wU16(uint8_t* p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
}
void wU32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}
void w3(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
}

} /* namespace */

bool relocateTdef(const uint8_t* srcPage, uint32_t umapPg,
                  const std::vector<uint32_t>& indexRoots,
                  std::vector<uint8_t>& out, std::string& error) {
    if (!srcPage) {
        error = "relocateTdef: null page";
        return false;
    }
    if (srcPage[0] != kPgTableDef) {
        error = "relocateTdef: not a tdef page";
        return false;
    }
    if (rU32(srcPage + 4) != 0) {
        error = "relocateTdef: chained MSys tdef unsupported";
        return false;
    }
    uint32_t tdefLen = rU32(srcPage + 8);
    if (tdefLen > kPageSize || tdefLen < 63) {
        error = "relocateTdef: bad tdef length";
        return false;
    }
    if (rU32(srcPage + 12) != 1625) {
        error = "relocateTdef: bad table magic";
        return false;
    }
    uint32_t nSlots = rU32(srcPage + 47);
    if (indexRoots.size() != nSlots) {
        error = "relocateTdef: root count mismatch";
        return false;
    }
    out.assign(srcPage, srcPage + kPageSize);
    uint8_t* b = out.data();
    /* Owned + free umap refs. */
    b[55] = 0;
    w3(b + 56, umapPg);
    b[59] = 1;
    w3(b + 60, umapPg);
    /* Index-data blocks: umap row/page + root page. */
    size_t o = 63 + 12 * nSlots;
    /* Skip column defs (25B each) + column names to reach index data. */
    uint16_t nCols = rU16(b + 45);
    o += 25 * nCols;
    for (uint16_t i = 0; i < nCols; i++) {
        uint16_t ln = rU16(b + o);
        o += 2 + ln;
        if (o >= kPageSize) {
            error = "relocateTdef: name overrun";
            return false;
        }
    }
    for (uint32_t i = 0; i < nSlots; i++) {
        if (rU32(b + o) != 1923) {
            error = "relocateTdef: bad index magic";
            return false;
        }
        /* Umap row: lval-first (2 + 2*nlval + i). Count LVAL columns
         * (MEMO/OLE) from the definition blocks. */
        uint32_t nlval = 0;
        {
            size_t co = 63 + 12 * nSlots;
            for (uint16_t c = 0; c < nCols; c++) {
                uint8_t ct = b[co + c * 25];
                if (ct == 0x0B || ct == 0x0C)
                    nlval++;
            }
        }
        b[o + 34] = (uint8_t)(2 + 2 * nlval + i);
        w3(b + o + 35, umapPg);
        wU32(b + o + 38, indexRoots[i]);
        o += 52;
    }
    /* Free-space + next pointer: single page, recompute free. */
    uint16_t free =
        (uint16_t)(tdefLen + 8 >= kPageSize ? 0 : kPageSize - tdefLen - 8);
    wU16(b + 2, free);
    wU32(b + 4, 0);
    return true;
}

std::map<uint32_t, std::string> buildTdefNameMap(
    const std::vector<std::pair<uint32_t, std::string>>& idNameEntries) {
    std::map<uint32_t, std::string> m;
    for (const auto& e : idNameEntries) {
        if (e.first > 1) /* 0/1 are header/global placeholders, not tdefs */
            m[e.first] = e.second;
    }
    return m;
}

uint32_t remapLink(uint32_t oldPage,
                   const std::map<uint32_t, std::string>& oldTdefToName,
                   const std::map<std::string, uint32_t>& nameToNewTdef,
                   bool& wasMapped) {
    wasMapped = false;
    auto it = oldTdefToName.find(oldPage);
    if (it == oldTdefToName.end())
        return oldPage;
    auto jt = nameToNewTdef.find(it->second);
    if (jt == nameToNewTdef.end())
        return oldPage;
    wasMapped = true;
    return jt->second;
}

} /* namespace jet4x */
