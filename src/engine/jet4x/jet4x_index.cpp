/**
 * @file jet4x_index.cpp
 * @description jet4x index key encoder + bulk builder implementation.
 *
 * Linguistic map: libmdb index.c idx_to_text_ling (MDB Tools, GPL;
 * used here as Unicode collation DATA for Jet-compatible key bytes).
 */

#include "jet4x_index.h"

#include <algorithm>

#include "jet4x_pages.h"

namespace jet4x {

/* libmdb index.c idx_to_text_ling (Jet Red v4 linguistic sort weights). */
static const uint8_t kLing[256] = {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x08, 0x08, 0x08, 0x08, 0x08, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x07, 0x09, 0x0A, 0x0C, 0x0E, 0x10, 0x12, 0x01,
    0x14, 0x16, 0x18, ',', 0x1A, 0x01, 0x1C, 0x1E,
    '6', '8', ':', '<', '>', '@', 'B', 'D',
    'F', 'H', ' ', '"', '.', '0', '2', '$',
    '&', 'J', 'L', 'M', 'O', 'Q', 'S', 'U',
    'W', 'Y', '[', '\\', '^', '`', 'b', 'd',
    'f', 'h', 'i', 'k', 'm', 'o', 'q', 's',
    'u', 'v', 'x', '\'', ')', '*', '+', '+',
    '+', 'J', 'L', 'M', 'O', 'Q', 'S', 'U',
    'W', 'Y', '[', '\\', '^', '`', 'b', 'd',
    'f', 'h', 'i', 'k', 'm', 'o', 'q', 's',
    'u', 'v', 'x', '+', '+', '+', '+', 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x08, '+', '4', '4', '4', '4', '+', '4',
    '+', '4', 'J', '3', '4', 0x01, '4', '+',
    '4', '3', ':', '<', '+', '4', '4', '4',
    '+', '8', 'd', '3', '7', '7', '7', '+',
    'J', 'J', 'J', 'J', 'J', 'J', 'J', 'M',
    'Q', 'Q', 'Q', 'Q', 'Y', 'Y', 'Y', 'Y',
    'O', 'b', 'd', 'd', 'd', 'd', 'd', '3',
    'd', 'o', 'o', 'o', 'o', 'v', 'm', 'k',
    'J', 'J', 'J', 'J', 'J', 'J', 'J', 'M',
    'Q', 'Q', 'Q', 'Q', 'Y', 'Y', 'Y', 'Y',
    'O', 'b', 'd', 'd', 'd', 'd', 'd', '3',
    'd', 'o', 'o', 'o', 'o', 'v', 'm', 'v',
};

namespace {

void putU16(uint8_t* p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
}
void putU32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}

void encodeInteger(const uint8_t* le, size_t n, bool desc,
                   std::vector<uint8_t>& out) {
    std::vector<uint8_t> be(n);
    for (size_t i = 0; i < n; i++)
        be[i] = le[n - 1 - i];
    be[0] ^= 0x80;
    if (desc) {
        for (auto& b : be)
            b = (uint8_t)~b;
    }
    out.insert(out.end(), be.begin(), be.end());
}

void encodeDouble(const uint8_t* le8, bool desc, std::vector<uint8_t>& out) {
    uint8_t be[8];
    for (int i = 0; i < 8; i++)
        be[i] = le8[7 - i];
    bool neg = (be[0] & 0x80) != 0;
    if (!neg)
        be[0] ^= 0x80;
    if (neg == !desc) {
        for (int i = 0; i < 8; i++)
            be[i] = (uint8_t)~be[i];
    }
    out.insert(out.end(), be, be + 8);
}

void encodeText(const uint8_t* data, size_t len, bool desc,
                std::vector<uint8_t>& out) {
    if (len >= 2 && data[0] == 0xFF && data[1] == 0xFE) {
        for (size_t b = 2; b < len; b++) {
            if (data[b] == 0)
                break;
            uint8_t m = kLing[data[b]];
            out.push_back(desc ? (uint8_t)~m : m);
            if (out.size() > 240)
                break;
        }
    } else {
        for (size_t b = 0; b < len; b += 2) {
            if (data[b] == 0)
                break;
            uint8_t m = kLing[data[b]];
            out.push_back(desc ? (uint8_t)~m : m);
            if (out.size() > 240)
                break;
        }
    }
    out.push_back(desc ? 0xFE : 0x01);
    out.push_back(desc ? 0xFF : 0x00);
}

size_t commonPrefix(const std::vector<uint8_t>& a,
                    const std::vector<uint8_t>& b) {
    size_t m = std::min(a.size(), b.size()), i = 0;
    while (i < m && a[i] == b[i])
        i++;
    return i;
}

/* Prefix policy port (BTreeEngine::computeFittingPrefix). */
bool fittingPrefix(const std::vector<std::vector<uint8_t>>& es, size_t& cb) {
    if (es.empty() || es.size() > 2000)
        return false;
    for (const auto& e : es) {
        if (e.size() < 4 || e.size() > 1500)
            return false;
    }
    auto fits = [&](size_t c, size_t& stored) {
        stored = c;
        for (const auto& e : es) {
            if (e.size() < c)
                return false;
            stored += e.size() - c;
        }
        size_t maskBytes = (stored + 7) / 8;
        return (kIxMask + maskBytes <= (size_t)kIxArea) &&
               ((size_t)kIxArea + stored <= (size_t)kPageSize);
    };
    size_t stored = 0;
    if (fits(0, stored)) {
        cb = 0;
        return true;
    }
    size_t c = commonPrefix(es.front(), es.back());
    for (const auto& e : es)
        c = std::min(c, e.size() > 4 ? e.size() - 4 : 0);
    if (c == 0 || !fits(c, stored))
        return false;
    for (const auto& e : es)
        c = std::min(c, e.size() > 4 ? e.size() - 4 : 0);
    if (!fits(c, stored))
        return false;
    cb = c;
    return true;
}

int cmpEntries(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    size_t m = std::min(a.size(), b.size());
    int c = m ? memcmp(a.data(), b.data(), m) : 0;
    if (c == 0 && a.size() != b.size())
        c = (a.size() < b.size()) ? -1 : 1;
    return (c < 0) ? -1 : ((c > 0) ? 1 : 0);
}

/* Writes one index page (leaf or node) with computed prefix + mask. */
bool writeIndexPage(DatabaseImage& img, uint32_t pg, bool isLeaf, uint8_t level,
                    uint32_t tdefPg, uint32_t prev, uint32_t next,
                    uint32_t tail,
                    const std::vector<std::vector<uint8_t>>& fullEntries,
                    std::string& error) {
    size_t cb = 0;
    if (!fullEntries.empty() && !fittingPrefix(fullEntries, cb)) {
        error = "writeIndexPage: entries do not fit";
        return false;
    }
    uint8_t* p = img.page(pg);
    if (!p) {
        error = "writeIndexPage: bad page";
        return false;
    }
    memset(p, 0, kPageSize);
    p[0] = isLeaf ? kPgIndexLeaf : kPgIndexNode;
    p[1] = 0x01;
    putU32(p + 4, tdefPg);
    putU32(p + 12, prev);
    putU32(p + 16, next);
    putU32(p + 20, tail);
    putU16(p + 24, (uint16_t)cb);
    p[26] = level;
    size_t pos = kIxArea;
    if (cb > 0) {
        if (cb > fullEntries.front().size()) {
            error = "writeIndexPage: prefix overrun";
            return false;
        }
        memcpy(p + pos, fullEntries.front().data(), cb);
        pos += cb;
    }
    size_t total = cb;
    std::vector<size_t> bounds;
    for (const auto& e : fullEntries) {
        if (e.size() < cb) {
            error = "writeIndexPage: short entry";
            return false;
        }
        size_t suffix = e.size() - cb;
        if (pos + suffix > kPageSize) {
            error = "writeIndexPage: entry OOB";
            return false;
        }
        memcpy(p + pos, e.data() + cb, suffix);
        pos += suffix;
        total += suffix;
        bounds.push_back(total);
    }
    for (size_t bnd : bounds) {
        size_t idx = bnd / 8;
        if (idx >= kIxMaskSize) {
            error = "writeIndexPage: mask overflow";
            return false;
        }
        p[kIxMask + idx] |= (uint8_t)(1u << (bnd % 8));
    }
    putU16(p + 2, (uint16_t)(kPageSize - pos));
    return true;
}

} /* namespace */

bool encodeIndexEntry(const std::vector<KeyColumn>& keys, uint32_t dataPg,
                      uint8_t slot, std::vector<uint8_t>& out,
                      std::string& error) {
    if (keys.empty() || keys.size() > 10) {
        error = "encodeIndexEntry: bad key count";
        return false;
    }
    std::vector<uint8_t> key;
    key.reserve(128);
    for (const auto& k : keys) {
        if (k.isNull) {
            key.push_back(k.descending ? kKeyDescNull : kKeyAscNull);
            continue;
        }
        if (!k.data && k.size > 0) {
            error = "encodeIndexEntry: null data";
            return false;
        }
        switch (k.type) {
            case DataType::Int16:
                if (k.size < 2) {
                    error = "encodeIndexEntry: INT short";
                    return false;
                }
                key.push_back(k.descending ? kKeyDescStart : kKeyAscStart);
                encodeInteger(k.data, 2, k.descending, key);
                break;
            case DataType::Int32:
                if (k.size < 4) {
                    error = "encodeIndexEntry: LONG short";
                    return false;
                }
                key.push_back(k.descending ? kKeyDescStart : kKeyAscStart);
                encodeInteger(k.data, 4, k.descending, key);
                break;
            case DataType::Double:
            case DataType::DateTime:
                if (k.size < 8) {
                    error = "encodeIndexEntry: DOUBLE short";
                    return false;
                }
                key.push_back(k.descending ? kKeyDescStart : kKeyAscStart);
                encodeDouble(k.data, k.descending, key);
                break;
            case DataType::Text:
                key.push_back(k.descending ? kKeyDescStart : kKeyAscStart);
                encodeText(k.data ? k.data : (const uint8_t*)"", k.size,
                           k.descending, key);
                break;
            default: {
                /* Raw branch (BOOL/Float32/Byte/Binary/...): no flag,
                 * up to 32 bytes capped by declared length. */
                size_t n = k.size;
                if (k.length > 0 && k.length < 32)
                    n = std::min(n, (size_t)k.length);
                if (n > 32)
                    n = 32;
                if (n > 0)
                    key.insert(key.end(), k.data, k.data + n);
                break;
            }
        }
        if (key.size() > 248) {
            error = "encodeIndexEntry: key too long";
            return false;
        }
    }
    key.push_back((uint8_t)((dataPg >> 16) & 0xFF));
    key.push_back((uint8_t)((dataPg >> 8) & 0xFF));
    key.push_back((uint8_t)(dataPg & 0xFF));
    key.push_back(slot);
    out = std::move(key);
    return true;
}

bool buildIndex(DatabaseImage& img, uint32_t tdefPg,
                std::vector<std::vector<uint8_t>> entries,
                IndexBuildResult& out, std::string& error) {
    out = IndexBuildResult();
    out.entryCount = entries.size();
    if (entries.empty()) {
        uint32_t rp = newIndexRoot(img, tdefPg);
        out.root = rp;
        out.levels = 1;
        out.allPages.push_back(rp);
        return true;
    }
    for (const auto& e : entries) {
        if (e.size() < 4 || e.size() > 1500) {
            error = "buildIndex: bad entry size";
            return false;
        }
    }
    std::sort(entries.begin(), entries.end(),
              [](const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
                  return cmpEntries(a, b) < 0;
              });
    /* Bottom-up levels: leaves (level 0, chained), then nodes. */
    std::vector<uint32_t> levelPages;
    std::vector<std::vector<uint8_t>> levelMax; /* max full entry per page */
    {
        /* Greedy leaf fill. */
        std::vector<std::vector<uint8_t>> page;
        auto flushLeaf = [&](bool& ok) {
            ok = true;
            if (page.empty())
                return;
            uint32_t pg = img.allocPage(kPgIndexLeaf);
            if (!writeIndexPage(img, pg, true, 0, tdefPg, 0, 0, 0, page,
                                error)) {
                ok = false;
                return;
            }
            levelPages.push_back(pg);
            levelMax.push_back(page.back());
            out.allPages.push_back(pg);
            page.clear();
        };
        for (const auto& e : entries) {
            page.push_back(e);
            size_t cb = 0;
            if (!fittingPrefix(page, cb)) {
                page.pop_back();
                bool ok = true;
                flushLeaf(ok);
                if (!ok)
                    return false;
                page.push_back(e);
                if (!fittingPrefix(page, cb)) {
                    error = "buildIndex: single entry overflow";
                    return false;
                }
            }
        }
        bool ok = true;
        flushLeaf(ok);
        if (!ok)
            return false;
        /* Sibling chain. */
        for (size_t i = 0; i < levelPages.size(); i++) {
            uint8_t* p = img.page(levelPages[i]);
            putU32(p + 12, i == 0 ? 0 : levelPages[i - 1]);
            putU32(p + 16,
                   i + 1 < levelPages.size() ? levelPages[i + 1] : 0);
        }
    }
    uint8_t level = 0;
    while (levelPages.size() > 1) {
        level++;
        /* Uniform dividers: one per child (max key+rowId + childPg) plus
         * tail = last child. The redundant last divider keeps even
         * single-child nodes descent-safe (the frozen descent rejects
         * divider-less nodes); routing is unaffected. Max bases carry
         * NO child suffix (stripped at restack). */
        std::vector<uint32_t> nextPages;
        std::vector<std::vector<uint8_t>> nextBases;
        std::vector<uint32_t> kids;
        std::vector<std::vector<uint8_t>> bases;
        auto dividersFor = [&](const std::vector<uint32_t>& kpgs,
                               const std::vector<std::vector<uint8_t>>& kb) {
            std::vector<std::vector<uint8_t>> divs;
            for (size_t k = 0; k < kpgs.size(); k++) {
                std::vector<uint8_t> d = kb[k];
                d.push_back((uint8_t)((kpgs[k] >> 24) & 0xFF));
                d.push_back((uint8_t)((kpgs[k] >> 16) & 0xFF));
                d.push_back((uint8_t)((kpgs[k] >> 8) & 0xFF));
                d.push_back((uint8_t)(kpgs[k] & 0xFF));
                divs.push_back(std::move(d));
            }
            return divs;
        };
        auto flushNode = [&](bool& ok) {
            ok = true;
            if (kids.empty())
                return;
            uint32_t pg = img.allocPage(kPgIndexNode);
            uint32_t tail = kids.back();
            std::vector<std::vector<uint8_t>> divs =
                dividersFor(kids, bases);
            if (!writeIndexPage(img, pg, false, level, tdefPg, 0, 0, tail,
                                divs, error)) {
                ok = false;
                return;
            }
            nextPages.push_back(pg);
            nextBases.push_back(bases.back());
            out.allPages.push_back(pg);
            kids.clear();
            bases.clear();
        };
        for (size_t i = 0; i < levelPages.size(); i++) {
            kids.push_back(levelPages[i]);
            bases.push_back(levelMax[i]);
            if (kids.size() >= 2 && i + 1 < levelPages.size()) {
                /* Seal if one more child would overflow: probe. */
                std::vector<uint32_t> pk = kids;
                std::vector<std::vector<uint8_t>> pb = bases;
                pk.push_back(levelPages[i + 1]);
                pb.push_back(levelMax[i + 1]);
                size_t cb = 0;
                if (!fittingPrefix(dividersFor(pk, pb), cb)) {
                    bool ok = true;
                    flushNode(ok);
                    if (!ok)
                        return false;
                }
            }
        }
        {
            bool ok = true;
            flushNode(ok);
            if (!ok)
                return false;
        }
        /* Chain this level's siblings. */
        for (size_t i = 0; i < nextPages.size(); i++) {
            uint8_t* p = img.page(nextPages[i]);
            putU32(p + 12, i == 0 ? 0 : nextPages[i - 1]);
            putU32(p + 16, i + 1 < nextPages.size() ? nextPages[i + 1] : 0);
        }
        levelPages = std::move(nextPages);
        levelMax = std::move(nextBases);
    }
    out.root = levelPages[0];
    out.levels = (uint8_t)(level + 1);
    return true;
}

} /* namespace jet4x */
