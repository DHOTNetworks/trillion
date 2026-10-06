/*
 * Jet4Writer — clean C++17 Jet 4 writer. Spec: Jackcess TableImpl/IndexData/
 * IndexPageCache/JetFormat. Memory-safe: std::vector RAII, bounds-checked,
 * no raw new/malloc, no unchecked memcpy.
 */
#include "jet4_writer.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "mdbtools.h"
#include "mdbprivate.h"
}

/* Non-static libmdb helpers reused for page I/O, maps, bitmap unpack. */
extern "C" {
ssize_t mdb_write_pg(MdbHandle* mdb, unsigned long pg);
int mdb_index_unpack_bitmap(MdbHandle* mdb, MdbIndexPage* ipg);
gint32 mdb_alloc_page(MdbTableDef* table);
void mdb_map_sync_to_disk(MdbTableDef* table);
void* mdb_new_leaf_pg(MdbCatalogEntry* entry);
void mdb_index_swap_n(unsigned char* src, int sz, unsigned char* dest);
int mdb_find_row(MdbHandle* mdb, int row_number, int* row_start, size_t* row_size);
extern char idx_to_text_ling[];
}

namespace Jet4Writer {
namespace {

/* ---------------- bounds-checked byte buffer ---------------- */
class ByteVec {
public:
    explicit ByteVec(size_t cap = 512) { buf_.reserve(cap); }
    void putU8(uint8_t v) { buf_.push_back(v); }
    void putU16LE(uint16_t v) {
        buf_.push_back(static_cast<uint8_t>(v & 0xFF));
        buf_.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    }
    void putBytes(const uint8_t* p, size_t n) {
        if (n) buf_.insert(buf_.end(), p, p + n); /* caller guarantees p valid */
    }
    size_t size() const { return buf_.size(); }
    const uint8_t* data() const { return buf_.data(); }
    std::vector<uint8_t> release() { return std::move(buf_); }
private:
    std::vector<uint8_t> buf_;
};

static bool debugOn() {
    static int v = -1;
    if (v < 0) v = (std::getenv("JET4_DEBUG") != nullptr) ? 1 : 0;
    return v == 1;
}
static void debugLog(const std::string& msg) {
    if (debugOn()) std::fprintf(stderr, "[jet4] %s\n", msg.c_str());
}

const MdbColumn* colAt(const MdbTableDef* table, int colnum) {
    if (!table || !table->columns) return nullptr;
    if (colnum < 0 || colnum >= static_cast<int>(table->num_cols)) return nullptr;
    return static_cast<const MdbColumn*>(g_ptr_array_index(table->columns,
                                                           static_cast<guint>(colnum)));
}

const Field* findField(const Field* fields, size_t n, int colnum) {
    for (size_t i = 0; i < n; ++i)
        if (fields[i].colnum == colnum) return &fields[i];
    return nullptr;
}

/* Jackcess IntegerColumnDescriptor: big-endian payload, flipFirstBit,
 * then flipBytes when descending. */
void intPayloadBE(const uint8_t* le, size_t n, bool desc, ByteVec& out) {
    std::vector<uint8_t> be(n);
    for (size_t i = 0; i < n; ++i) be[i] = le[n - 1 - i];
    be[0] ^= 0x80;
    if (desc)
        for (auto& b : be) b = static_cast<uint8_t>(~b);
    out.putBytes(be.data(), be.size());
}

/* Jackcess FloatingPointColumnDescriptor, transcribed exactly:
 *   if (!isNegative) flipFirstBit;
 *   if (isNegative == isAscending()) flipBytes;   (isAscending == !desc) */
void doublePayload(const uint8_t* le8, bool desc, ByteVec& out) {
    uint8_t be[8];
    for (int i = 0; i < 8; ++i) be[i] = le8[7 - i];
    const bool neg = (be[0] & 0x80) != 0;
    if (!neg) be[0] ^= 0x80;
    if (neg == !desc) {
        for (int i = 0; i < 8; ++i) be[i] = static_cast<uint8_t>(~be[i]);
    }
    out.putBytes(be, 8);
}

size_t commonPrefixLen(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    const size_t m = std::min(a.size(), b.size());
    size_t i = 0;
    while (i < m && a[i] == b[i]) ++i;
    return i;
}

/* ---------------- page I/O helpers ---------------- */
uint16_t pgGet16(MdbHandle* mdb, int off) {
    return static_cast<uint16_t>(mdb_get_int16(mdb->pg_buf, off));
}
uint32_t pgGet32(MdbHandle* mdb, int off) {
    return static_cast<uint32_t>(mdb_get_int32(mdb->pg_buf, off));
}
void pgPut16(MdbHandle* mdb, int off, uint16_t v) { mdb_put_int16(mdb->pg_buf, off, v); }
void pgPut32(MdbHandle* mdb, int off, uint32_t v) { mdb_put_int32(mdb->pg_buf, off, v); }
bool pgRead(MdbHandle* mdb, uint32_t pg) { return mdb_read_pg(mdb, pg) != 0; }
bool pgWrite(MdbHandle* mdb, uint32_t pg) { return mdb_write_pg(mdb, pg) != 0; }

/* Full entry bytes on a page, page-prefix model (Jackcess DataPage):
 * prefix stored once at area start; entry 0's bitmap span covers it. */
struct DecodedPage {
    std::vector<std::vector<uint8_t>> full; /* every entry, complete bytes */
    std::vector<uint8_t> prefix;
};

bool decodeEntries(MdbHandle* mdb, DecodedPage& dp, std::string& err) {
    const auto* buf = static_cast<const uint8_t*>(mdb->pg_buf);
    if (buf[0] != kPageLeaf && buf[0] != kPageIndex) {
        err = "decodeEntries: not an index page"; return false;
    }
    MdbIndexPage ipg;
    std::memset(&ipg, 0, sizeof(ipg));
    const int bounds = mdb_index_unpack_bitmap(mdb, &ipg);
    if (bounds < 1) { err = "bitmap unpack failed"; return false; }
    const int nEntries = bounds - 1; /* unpack returns boundaries = entries+1 */
    if (nEntries < 0 || nEntries > 2000) { err = "entry count insane"; return false; }
    const uint16_t cb = pgGet16(mdb, kOffPrefixLen);
    if (cb > 512) { err = "prefix len insane"; return false; }
    if (kEntryAreaStart + cb > kPageSize) { err = "prefix OOB"; return false; }
    dp.prefix.assign(buf + kEntryAreaStart, buf + kEntryAreaStart + cb);
    dp.full.clear();
    dp.full.reserve(static_cast<size_t>(nEntries));
    for (int i = 0; i < nEntries; ++i) {
        const int s = ipg.idx_starts[i];
        const int e = ipg.idx_starts[i + 1];
        if (s < kEntryAreaStart || e > kPageSize || e < s) {
            err = "entry bounds insane"; return false;
        }
        std::vector<uint8_t> full;
        if (i == 0) {
            full.assign(buf + s, buf + e); /* span already contains prefix */
            if (full.size() < dp.prefix.size() ||
                !std::equal(dp.prefix.begin(), dp.prefix.end(), full.begin())) {
                err = "entry0/prefix mismatch"; return false;
            }
        } else {
            full.reserve(dp.prefix.size() + static_cast<size_t>(e - s));
            full.insert(full.end(), dp.prefix.begin(), dp.prefix.end());
            full.insert(full.end(), buf + s, buf + e);
        }
        dp.full.push_back(std::move(full));
    }
    return true;
}

/* B-tree descent to target leaf (full-key comparison, Jackcess ordering).
 * Records the ancestor node chain (root first) for split propagation.
 * Tail-aware: keys beyond all dividers go to childTail when set. */
uint32_t findLeaf(MdbHandle* mdb, MdbIndex* idx,
                  const uint8_t* key, size_t keyLen,
                  std::vector<uint32_t>& ancPath, std::string& err) {
    ancPath.clear();
    uint32_t pg = idx->first_pg;
    for (int depth = 0; depth < 12 && pg > 0; ++depth) {
        if (!pgRead(mdb, pg)) { err = "descent read failed"; return 0; }
        const auto* buf = static_cast<const uint8_t*>(mdb->pg_buf);
        if (buf[0] == kPageLeaf) return pg;
        if (buf[0] != kPageIndex) { err = "descent hit non-index page"; return 0; }
        DecodedPage dp;
        if (!decodeEntries(mdb, dp, err)) return 0;
        if (dp.full.empty()) { err = "empty index node"; return 0; }
        const uint32_t tail = pgGet32(mdb, kOffChildTailPg);
        uint32_t child = 0;
        bool exhausted = true;
        for (size_t i = 0; i < dp.full.size(); ++i) {
            const auto& div = dp.full[i];
            if (div.size() < 4) continue;
            const size_t dlen = div.size() - 4; /* 4-byte BE child pg trailer */
            const uint32_t cpg = (static_cast<uint32_t>(div[dlen]) << 24) |
                                 (static_cast<uint32_t>(div[dlen + 1]) << 16) |
                                 (static_cast<uint32_t>(div[dlen + 2]) << 8) |
                                 static_cast<uint32_t>(div[dlen + 3]);
            const size_t cmpLen = std::min(keyLen, dlen);
            int cmp = std::memcmp(key, div.data(), cmpLen);
            if (cmp == 0) cmp = (keyLen < dlen) ? -1 : (keyLen > dlen ? 1 : 0);
            if (cmp <= 0) { child = cpg; exhausted = false; break; }
            if (i + 1 == dp.full.size()) {
                child = (tail != 0) ? tail : cpg; /* tail else last child */
                exhausted = false;
            }
        }
        if (exhausted || !child) { err = "no child selected"; return 0; }
        ancPath.push_back(pg);
        pg = child;
    }
    err = "descent too deep/empty";
    return 0;
}

/* Forward: defined below (leaf/node page rewrite + key compare). */
static bool writeLeaf(MdbHandle* mdb, uint32_t leafPg, bool isLeaf,
                      const std::vector<std::vector<uint8_t>>& fullEntries,
                      std::string& err);
static int cmpKeys(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b);

static uint32_t childOf(const std::vector<uint8_t>& divEntry) {
    const size_t d = divEntry.size() - 4;
    return (static_cast<uint32_t>(divEntry[d]) << 24) |
           (static_cast<uint32_t>(divEntry[d + 1]) << 16) |
           (static_cast<uint32_t>(divEntry[d + 2]) << 8) |
           static_cast<uint32_t>(divEntry[d + 3]);
}

static std::vector<uint8_t> withChild(const std::vector<uint8_t>& fullKeyNoTrailer,
                                      uint32_t child) {
    std::vector<uint8_t> e = fullKeyNoTrailer;
    e.push_back(static_cast<uint8_t>((child >> 24) & 0xFF));
    e.push_back(static_cast<uint8_t>((child >> 16) & 0xFF));
    e.push_back(static_cast<uint8_t>((child >> 8) & 0xFF));
    e.push_back(static_cast<uint8_t>(child & 0xFF));
    return e;
}

/* Forward: insert a sorted full-entry list into page pg (leaf or node),
 * splitting with parent propagation when it does not fit. ancPath holds
 * ancestor node pages root-first (parent of pg is back, if any). */
static Status insertIntoPage(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                             uint32_t pg, bool isLeaf,
                             const std::vector<std::vector<uint8_t>>& merged,
                             std::vector<uint32_t>& ancPath);

/* ---- usage-map bit helpers (inline type 0 + reference type 1) ---- */
static bool mapBitOp(MdbHandle* mdb, std::vector<uint8_t>& row, uint32_t pg, bool set) {
    if (row.empty()) return false;
    if (row[0] == 0) {
        if (row.size() < 5) return false;
        const uint32_t start = row[1] | (row[2] << 8) | (row[3] << 16) | (row[4] << 24);
        if (pg < start) return false;
        const uint32_t bi = pg - start;
        if (bi >= (row.size() - 5) * 8) return false;
        uint8_t& b = row[5 + bi / 8];
        b = set ? static_cast<uint8_t>(b | (1u << (bi % 8)))
                : static_cast<uint8_t>(b & ~(1u << (bi % 8)));
        return true;
    }
    return false; /* type 1 handled by caller with page I/O */
}

/* Read one map row's bytes (any table/index map row). */
static bool readMapRow(MdbHandle* mdb, uint32_t mapPg, uint16_t mapRow,
                       std::vector<uint8_t>& rowBytes, int& rowStart) {
    if (!mapPg) return false;
    if (!pgRead(mdb, mapPg)) return false;
    int start = 0;
    size_t sz = 0;
    if (mdb_find_row(mdb, mapRow, &start, &sz) != 0) return false; /* 0 = ok */
    start &= 0x0FFF;
    if (start < 0 || sz == 0 || sz > 4096 || start + (int)sz > kPageSize) return false;
    const auto* b = static_cast<const uint8_t*>(mdb->pg_buf);
    rowBytes.assign(b + start, b + start + sz);
    rowStart = start;
    return true;
}

static bool writeMapRow(MdbHandle* mdb, uint32_t mapPg, int rowStart,
                        const std::vector<uint8_t>& rowBytes) {
    if (!pgRead(mdb, mapPg)) return false;
    auto* b = static_cast<uint8_t*>(mdb->pg_buf);
    if (rowStart < 0 || rowStart + (int)rowBytes.size() > kPageSize) return false;
    std::memcpy(b + rowStart, rowBytes.data(), rowBytes.size());
    return pgWrite(mdb, mapPg);
}

/* Set/clear a bit in a reference (type 1) map: map row holds u32 page list. */
static bool refMapBitOp(MdbHandle* mdb, uint32_t mapPg, uint16_t mapRow,
                        uint32_t pg, bool set) {
    std::vector<uint8_t> row;
    int rowStart = 0;
    if (!readMapRow(mdb, mapPg, mapRow, row, rowStart)) return false;
    if (row.empty() || row[0] != 1) return false;
    const uint32_t bitlen = (kPageSize - 4) * 8;
    const uint32_t maxPgs = (row.size() - 1) / 4;
    const uint32_t ind = pg / bitlen;
    if (ind >= maxPgs) return false;
    const size_t o = 1 + ind * 4;
    const uint32_t bmpPg = row[o] | (row[o+1] << 8) | (row[o+2] << 16) | (row[o+3] << 24);
    if (!bmpPg) return false;
    if (!pgRead(mdb, bmpPg)) return false;
    auto* b = static_cast<uint8_t*>(mdb->pg_buf);
    const uint32_t off = pg % bitlen;
    uint8_t& bb = b[4 + off / 8];
    bb = set ? static_cast<uint8_t>(bb | (1u << (off % 8)))
             : static_cast<uint8_t>(bb & ~(1u << (off % 8)));
    if (!pgWrite(mdb, bmpPg)) return false;
    /* Re-read map row page (pg_buf moved) — row bytes unchanged. */
    return true;
}

/* Register a newly allocated INDEX page in its index-owned map and remove
 * it from the table data maps (native: index pages live only in index maps).
 * Mirrors Jackcess IndexData.addOwnedPage. */
static Status registerIndexPage(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                                uint32_t newPg) {
    if (idx->idx_map_pg != 0) {
        std::vector<uint8_t> row;
        int rowStart = 0;
        if (!readMapRow(mdb, idx->idx_map_pg, idx->idx_map_row, row, rowStart))
            return Status::Fail("registerIndexPage: map row unreadable");
        bool ok = false;
        if (!row.empty() && row[0] == 0) {
            ok = mapBitOp(mdb, row, newPg, true);
            if (ok) {
                if (!pgRead(mdb, idx->idx_map_pg))
                    return Status::Fail("registerIndexPage: map reread failed");
                auto* b = static_cast<uint8_t*>(mdb->pg_buf);
                if (rowStart + (int)row.size() <= kPageSize) {
                    std::memcpy(b + rowStart, row.data(), row.size());
                    if (!pgWrite(mdb, idx->idx_map_pg))
                        return Status::Fail("registerIndexPage: map write failed");
                } else {
                    ok = false;
                }
            }
        } else if (!row.empty() && row[0] == 1) {
            ok = refMapBitOp(mdb, idx->idx_map_pg, idx->idx_map_row, newPg, true);
        }
        if (!ok) return Status::Fail("registerIndexPage: bit set failed");
        debugLog("registerIndexPage idx=" + std::string(idx->name) +
                 " pg=" + std::to_string(newPg) + " map=" +
                 std::to_string(idx->idx_map_pg) + "/" + std::to_string(idx->idx_map_row));
    }
    /* Remove from table data/free maps (native index pages are absent there);
     * mdb_alloc_page() had set these bits. */
    if (table->usage_map && table->free_usage_map) {
        auto clearIn = [&](unsigned char* map, unsigned sz) {
            if (!map || sz < 5) return;
            if (map[0] == 0) {
                const uint32_t start = mdb_get_int32(map, 1);
                if (newPg >= start) {
                    const uint32_t bi = newPg - start;
                    if (bi < (sz - 5) * 8) map[5 + bi / 8] &= ~(1u << (bi % 8));
                }
            }
            /* type 1 (paged): clear via page write */
            else if (map[0] == 1) {
                const uint32_t bitlen = (kPageSize - 4) * 8;
                const uint32_t maxPgs = (sz - 1) / 4;
                const uint32_t ind = newPg / bitlen;
                if (ind < maxPgs) {
                    const size_t o = 1 + ind * 4;
                    const uint32_t bmpPg = map[o] | (map[o+1] << 8) | (map[o+2] << 16) | (map[o+3] << 24);
                    if (bmpPg && pgRead(mdb, bmpPg)) {
                        auto* b = static_cast<uint8_t*>(mdb->pg_buf);
                        const uint32_t off = newPg % bitlen;
                        b[4 + off / 8] &= ~(1u << (off % 8));
                        pgWrite(mdb, bmpPg);
                    }
                }
            }
        };
        clearIn(table->usage_map, table->map_sz);
        clearIn(table->free_usage_map, table->freemap_sz);
        mdb_map_sync_to_disk(table);
    }
    return Status::Ok();
}

/* Split overflowing page pg (holding merged, sorted full entries).
 * New page takes the UPPER half and chains AFTER pg. Parent dividers:
 * replace pg's divider with max(L), insert max(R)->new after it
 * (tail child: append max(L)->pg divider, new page becomes tail). */
static Status splitPage(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                        uint32_t pg, bool wasLeaf,
                        const std::vector<std::vector<uint8_t>>& merged,
                        std::vector<uint32_t>& ancPath) {
    if (merged.size() < 2) return Status::Fail("splitPage: nothing to split");
    /* Divide by bytes so both halves fit comfortably. */
    size_t total = 0;
    for (const auto& e : merged) total += e.size();
    size_t acc = 0, cut = 1;
    for (; cut + 1 < merged.size(); ++cut) {
        acc += merged[cut - 1].size();
        if (acc * 2 >= total) break;
    }
    if (cut < 1) cut = 1;
    if (cut >= merged.size()) cut = merged.size() - 1;
    std::vector<std::vector<uint8_t>> lower(merged.begin(), merged.begin() + cut);
    std::vector<std::vector<uint8_t>> upper(merged.begin() + cut, merged.end());
    debugLog("splitPage pg=" + std::to_string(pg) + " total=" +
             std::to_string(merged.size()) + " cut=" + std::to_string(cut));

    if (!pgRead(mdb, pg)) return Status::Fail("splitPage: reread failed");
    const uint32_t oldNext = pgGet32(mdb, kOffNextPg);
    const uint32_t oldTail = pgGet32(mdb, kOffChildTailPg);

    /* Allocate + register FIRST so every fallible global step precedes any
     * page mutation (a later failure must never strand half a split). */
    const gint32 newSigned = mdb_alloc_page(table);
    if (newSigned <= 0) return Status::Fail("splitPage: alloc failed");
    const uint32_t newPg = static_cast<uint32_t>(newSigned);
    {
        Status rg = registerIndexPage(mdb, table, idx, newPg);
        if (!rg.ok) return rg;
    }
    /* Rewrite pg with LOWER half (keeps page identity for lower keys). */
    {
        std::string err;
        if (!writeLeaf(mdb, pg, wasLeaf, lower, err))
            return Status::Fail("splitPage: rewrite lower failed: " + err);
    }
    /* Write UPPER half (stage header first: writeLeaf preserves
     * live prev/next/tail/tdef across its internal reread). */
    {
        if (!pgRead(mdb, newPg)) return Status::Fail("splitPage: newpg read failed");
        auto* b = static_cast<uint8_t*>(mdb->pg_buf);
        b[0] = wasLeaf ? kPageLeaf : kPageIndex;
        b[1] = 0x01;
        pgPut32(mdb, kOffTableDefPg, static_cast<uint32_t>(table->entry->table_pg));
        pgPut32(mdb, kOffUnknown, 0);
        pgPut32(mdb, kOffPrevPg, pg);
        pgPut32(mdb, kOffNextPg, oldNext);
        pgPut32(mdb, kOffChildTailPg, 0);
        if (!pgWrite(mdb, newPg)) return Status::Fail("splitPage: stage failed");
        std::string err;
        if (!writeLeaf(mdb, newPg, wasLeaf, upper, err))
            return Status::Fail("splitPage: write upper failed: " + err);
    }
    /* Splice chain: pg -> newPg -> oldNext (correct Jet4 offsets). */
    if (!pgRead(mdb, pg)) return Status::Fail("splitPage: relink read failed");
    pgPut32(mdb, kOffNextPg, newPg);
    if (!wasLeaf) pgPut32(mdb, kOffChildTailPg, oldTail); /* writeLeaf kept it */
    if (!pgWrite(mdb, pg)) return Status::Fail("splitPage: relink write failed");
    if (oldNext != 0) {
        if (!pgRead(mdb, oldNext)) return Status::Fail("splitPage: fwd read failed");
        const auto* b = static_cast<const uint8_t*>(mdb->pg_buf);
        if (b[0] == kPageLeaf || b[0] == kPageIndex) {
            pgPut32(mdb, kOffPrevPg, newPg);
            if (!pgWrite(mdb, oldNext)) return Status::Fail("splitPage: fwd write failed");
        }
    }

    /* Parent divider maintenance (Jackcess: divider = child max key). */
    const auto& maxL = lower.back();
    const auto& maxR = upper.back();
    auto stripTrailer = [](const std::vector<uint8_t>& e) {
        return std::vector<uint8_t>(e.begin(), e.end() - 4);
    };
    if (ancPath.empty()) {
        /* pg was ROOT: nest it (root page number stays -> no catalog change).
         * Move ALL merged entries to a child; root becomes single-divider node. */
        const gint32 cSigned = mdb_alloc_page(table);
        if (cSigned <= 0) return Status::Fail("splitPage: nest alloc failed");
        const uint32_t childPg = static_cast<uint32_t>(cSigned);
        {
            Status rg = registerIndexPage(mdb, table, idx, childPg);
            if (!rg.ok) return rg;
        }
        /* Child image first (leaf-ness follows old root). */
        if (!pgRead(mdb, childPg)) return Status::Fail("splitPage: nest read failed");
        {
            auto* b = static_cast<uint8_t*>(mdb->pg_buf);
            b[0] = wasLeaf ? kPageLeaf : kPageIndex;
            b[1] = 0x01;
        }
        pgPut32(mdb, kOffTableDefPg, static_cast<uint32_t>(table->entry->table_pg));
        pgPut32(mdb, kOffUnknown, 0);
        pgPut32(mdb, kOffPrevPg, 0);
        pgPut32(mdb, kOffNextPg, 0);
        pgPut32(mdb, kOffChildTailPg, wasLeaf ? 0 : oldTail);
        if (!pgWrite(mdb, childPg)) return Status::Fail("splitPage: nest stage failed");
        std::string err;
        if (!writeLeaf(mdb, childPg, wasLeaf, merged, err))
            return Status::Fail("splitPage: nest write failed: " + err);
        /* Root becomes a node with one divider (child max -> child). */
        if (!pgRead(mdb, pg)) return Status::Fail("splitPage: root reread failed");
        {
            auto* b = static_cast<uint8_t*>(mdb->pg_buf);
            b[0] = kPageIndex;
            b[1] = 0x01;
        }
        pgPut32(mdb, kOffChildTailPg, 0);
        if (!pgWrite(mdb, pg)) return Status::Fail("splitPage: root stage failed");
        std::vector<std::vector<uint8_t>> rootDiv;
        rootDiv.push_back(withChild(stripTrailer(merged.back()), childPg));
        if (!writeLeaf(mdb, pg, false, rootDiv, err))
            return Status::Fail("splitPage: root write failed: " + err);
        return Status::Ok();
    }
    const uint32_t parent = ancPath.back();
    ancPath.pop_back();
    if (!pgRead(mdb, parent)) return Status::Fail("splitPage: parent read failed");
    DecodedPage pdp;
    {
        std::string err;
        if (!decodeEntries(mdb, pdp, err))
            return Status::Fail("splitPage: parent decode: " + err);
    }
    /* Locate orig divider by child link; absent => orig was tail child. */
    size_t divPos = pdp.full.size();
    bool wasTail = true;
    for (size_t i = 0; i < pdp.full.size(); ++i) {
        if (childOf(pdp.full[i]) == pg) { divPos = i; wasTail = false; break; }
    }
    std::vector<std::vector<uint8_t>> pmerged;
    pmerged.reserve(pdp.full.size() + 1);
    if (wasTail) {
        pmerged.insert(pmerged.end(), pdp.full.begin(), pdp.full.end());
        pmerged.push_back(withChild(stripTrailer(maxL), pg)); /* orig now bounded */
        /* new page becomes tail: childTail = newPg (replacing old tail link). */
        if (!pgRead(mdb, parent)) return Status::Fail("splitPage: tail reread failed");
        pgPut32(mdb, kOffChildTailPg, newPg);
        if (!pgWrite(mdb, parent)) return Status::Fail("splitPage: tail write failed");
    } else {
        pmerged.insert(pmerged.end(), pdp.full.begin(), pdp.full.begin() + divPos);
        pmerged.push_back(withChild(stripTrailer(maxL), pg));   /* replace */
        pmerged.push_back(withChild(stripTrailer(maxR), newPg)); /* insert */
        pmerged.insert(pmerged.end(), pdp.full.begin() + divPos + 1, pdp.full.end());
    }
    return insertIntoPage(mdb, table, idx, parent, false, pmerged, ancPath);
}

static Status insertIntoPage(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                             uint32_t pg, bool isLeaf,
                             const std::vector<std::vector<uint8_t>>& merged,
                             std::vector<uint32_t>& ancPath) {
    if (merged.empty() || merged.size() > 2000)
        return Status::Fail("insertIntoPage: bad merged size");
    /* Verify caller's merged list is actually sorted (defensive). */
    for (size_t i = 1; i < merged.size(); ++i) {
        if (merged[i].size() < 4 || merged[i - 1].size() < 4)
            return Status::Fail("insertIntoPage: short entry");
        if (cmpKeys(merged[i], merged[i - 1]) < 0)
            return Status::Fail("insertIntoPage: merged list unsorted");
    }
    std::string err;
    if (!pgRead(mdb, pg)) return Status::Fail("insertIntoPage: reread failed");
    const auto* b = static_cast<const uint8_t*>(mdb->pg_buf);
    const uint8_t want = isLeaf ? kPageLeaf : kPageIndex;
    if (b[0] != want) return Status::Fail("insertIntoPage: page type changed");
    /* Test hook for bisection: JET4_NO_SPLIT=1 turns overflow into a clean
     * skip (legacy libmdb behavior) instead of splitting. */
    if (std::getenv("JET4_NO_SPLIT") != nullptr) {
        /* Probe fit without writing: emulate writeLeaf layout check. */
        size_t cb = 0, stored = 0;
        {
            size_t t0 = 0;
            for (const auto& e : merged) t0 += e.size();
            stored = t0; /* cb=0 */
            size_t maskB = (stored + 7) / 8;
            if (!((kOffEntryMask + maskB <= (size_t)kEntryAreaStart) &&
                  ((size_t)kEntryAreaStart + stored <= (size_t)kPageSize)))
                return Status::Fail("split disabled: page overflow, entry skipped");
        }
    }
    if (writeLeaf(mdb, pg, isLeaf, merged, err)) {
        debugLog("insertIntoPage pg=" + std::to_string(pg) +
                 " entries=" + std::to_string(merged.size()) + " plain-ok");
        return Status::Ok();
    }
    if (err.find("overflow") == std::string::npos)
        return Status::Fail("insertIntoPage: rewrite failed: " + err);
    return splitPage(mdb, table, idx, pg, isLeaf, merged, ancPath);
}

/* Key-only compare (excludes 4-byte trailer), for insert positioning. */
static int cmpKeys(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    const size_t al = a.size() > 4 ? a.size() - 4 : 0;
    const size_t bl = b.size() > 4 ? b.size() - 4 : 0;
    const size_t m = std::min(al, bl);
    int c = std::memcmp(a.data(), b.data(), m);
    if (c == 0) c = (al < bl) ? -1 : (al > bl ? 1 : 0);
    return c;
}

/* Rewrite one leaf OR node page from FULL entries with Jackcess prefix
 * policy: EMPTY prefix when everything fits, else common(first,last).
 * isLeaf selects the page-type byte (nodes must stay 0x03!). */
bool writeLeaf(MdbHandle* mdb, uint32_t leafPg, bool isLeaf,
               const std::vector<std::vector<uint8_t>>& fullEntries,
               std::string& err) {
    if (fullEntries.empty() || fullEntries.size() > 2000) {
        err = "writeLeaf: bad entry count"; return false;
    }
    for (const auto& e : fullEntries) {
        if (e.size() < 4 || e.size() > 1500) { err = "writeLeaf: bad entry size"; return false; }
    }
    /* Layout with EMPTY prefix first. */
    auto layoutFits = [&](size_t cb, size_t& storedTotal) {
        storedTotal = cb;
        for (const auto& e : fullEntries) {
            if (e.size() < cb) return false;
            storedTotal += e.size() - cb;
        }
        /* bitmap bits: one per entry end; highest bit = storedTotal */
        const size_t maskBytes = (storedTotal + 7) / 8;
        return (kOffEntryMask + maskBytes <= static_cast<size_t>(kEntryAreaStart)) &&
               (static_cast<size_t>(kEntryAreaStart) + storedTotal <= static_cast<size_t>(kPageSize));
    };
    size_t cb = 0, storedTotal = 0;
    if (!layoutFits(0, storedTotal)) {
        cb = commonPrefixLen(fullEntries.front(), fullEntries.back());
        /* Prefix must be a valid common prefix AND keep every entry non-empty. */
        if (cb == 0 || !layoutFits(cb, storedTotal)) {
            err = "writeLeaf: page overflow";
            return false;
        }
        /* Jackcess findCommonPrefix may return a prefix longer than some middle
         * entry; clamp so no entry strips to empty. */
        for (const auto& e : fullEntries) cb = std::min(cb, e.size() > 4 ? e.size() - 4 : 0);
        if (!layoutFits(cb, storedTotal)) { err = "writeLeaf: overflow after clamp"; return false; }
    }

    if (!pgRead(mdb, leafPg)) { err = "writeLeaf: reread failed"; return false; }
    std::vector<uint8_t> page(kPageSize, 0);
    /* Preserve header identity fields (page type included: nodes stay nodes). */
    const auto* cur = static_cast<const uint8_t*>(mdb->pg_buf);
    page[0] = isLeaf ? kPageLeaf : kPageIndex;
    page[1] = cur[1];
    auto put16 = [&](int off, uint16_t v) {
        page[static_cast<size_t>(off)] = static_cast<uint8_t>(v & 0xFF);
        page[static_cast<size_t>(off) + 1] = static_cast<uint8_t>((v >> 8) & 0xFF);
    };
    auto put32 = [&](int off, uint32_t v) {
        for (int i = 0; i < 4; ++i)
            page[static_cast<size_t>(off) + i] = static_cast<uint8_t>((v >> (8 * i)) & 0xFF);
    };
    put32(kOffTableDefPg, pgGet32(mdb, kOffTableDefPg));
    put32(kOffUnknown, 0);
    put32(kOffPrevPg, pgGet32(mdb, kOffPrevPg));
    put32(kOffNextPg, pgGet32(mdb, kOffNextPg));
    put32(kOffChildTailPg, pgGet32(mdb, kOffChildTailPg));
    put16(kOffPrefixLen, static_cast<uint16_t>(cb));
    page[kOffPrefixUnknown] = 0;
    /* Stored layout: [prefix][suffix...], bitmap over cumulative ends. */
    size_t pos = kEntryAreaStart;
    std::vector<bool> bits;
    bits.reserve(fullEntries.size() + 1);
    auto emitChunk = [&](const uint8_t* p, size_t n) -> bool {
        if (pos + n > static_cast<size_t>(kPageSize)) return false;
        std::memcpy(page.data() + pos, p, n);
        pos += n;
        return true;
    };
    if (cb > 0) {
        if (!emitChunk(fullEntries.front().data(), cb)) { err = "prefix OOB"; return false; }
    }
    size_t total = cb;
    std::vector<size_t> bounds;
    bounds.reserve(fullEntries.size());
    for (const auto& e : fullEntries) {
        const size_t suffix = e.size() - cb;
        if (!emitChunk(e.data() + cb, suffix)) { err = "entry OOB"; return false; }
        total += suffix;
        bounds.push_back(total);
    }
    std::vector<uint8_t> mask(kSizeEntryMask, 0);
    for (size_t bnd : bounds) {
        const size_t idx = bnd / 8;
        if (idx >= mask.size()) { err = "mask overflow"; return false; }
        mask[idx] |= static_cast<uint8_t>(1u << (bnd % 8));
    }
    std::memcpy(page.data() + kOffEntryMask, mask.data(), mask.size());
    put16(kOffFreeSpace, static_cast<uint16_t>(kPageSize - pos));
    if (pos > static_cast<size_t>(kPageSize)) { err = "page overflow"; return false; }
    std::memcpy(mdb->pg_buf, page.data(), kPageSize);
    (void)bits;
    return pgWrite(mdb, leafPg);
}

} /* namespace */

/* ============================ packRow ============================ */
Status packRow(MdbTableDef* table, const Field* fields, size_t nfields,
               std::vector<uint8_t>& out) {
    if (!table || !table->columns) return Status::Fail("packRow: null table");
    const unsigned numCols = table->num_cols;
    if (numCols == 0 || numCols > 1000) return Status::Fail("packRow: bad num_cols");

    int maxFixed = 0;
    for (unsigned i = 0; i < numCols; ++i) {
        const MdbColumn* c = colAt(table, static_cast<int>(i));
        if (!c) return Status::Fail("packRow: missing column def");
        if (c->is_fixed) {
            if (c->fixed_offset < 0 || c->col_size <= 0 || c->col_size > 512)
                return Status::Fail("packRow: bad fixed geometry");
            maxFixed = std::max(maxFixed, c->fixed_offset + c->col_size);
        }
    }

    std::vector<uint8_t> body;
    body.reserve(1024);
    body.push_back(static_cast<uint8_t>(numCols & 0xFF));
    body.push_back(static_cast<uint8_t>((numCols >> 8) & 0xFF));
    const size_t fixedStart = body.size();
    body.insert(body.end(), static_cast<size_t>(maxFixed), 0);

    for (size_t i = 0; i < nfields; ++i) {
        const Field& f = fields[i];
        const MdbColumn* c = colAt(table, f.colnum);
        if (!c || !c->is_fixed || f.isNull || !f.data || f.size == 0) continue;
        const size_t n = std::min(f.size, static_cast<size_t>(c->col_size));
        const size_t at = fixedStart + static_cast<size_t>(c->fixed_offset);
        if (at + n > body.size()) return Status::Fail("packRow: fixed OOB");
        std::memcpy(body.data() + at, f.data, n);
    }

    const unsigned numVar = table->num_var_cols;
    std::vector<const Field*> byVar(numVar, nullptr);
    for (size_t i = 0; i < nfields; ++i) {
        const Field& f = fields[i];
        const MdbColumn* c = colAt(table, f.colnum);
        if (!c || c->is_fixed) continue;
        if (static_cast<unsigned>(c->var_col_num) < numVar && !byVar[static_cast<size_t>(c->var_col_num)])
            byVar[static_cast<size_t>(c->var_col_num)] = &f;
    }
    if (numVar > 0) {
        std::vector<uint32_t> varOff(numVar + 1, 0);
        for (unsigned v = 0; v < numVar; ++v) {
            varOff[v] = static_cast<uint32_t>(body.size()); /* absolute from row start */
            const Field* f = byVar[v];
            if (f && !f->isNull && f->data && f->size > 0) {
                if (body.size() + f->size > 4000) return Status::Fail("packRow: row too big");
                body.insert(body.end(), f->data, f->data + f->size);
            }
        }
        varOff[numVar] = static_cast<uint32_t>(body.size()); /* EOD */
        for (int v = static_cast<int>(numVar); v >= 0; --v) {
            const uint32_t o = varOff[static_cast<size_t>(v)];
            if (o > 0xFFFF) return Status::Fail("packRow: var offset overflow");
            body.push_back(static_cast<uint8_t>(o & 0xFF));
            body.push_back(static_cast<uint8_t>((o >> 8) & 0xFF));
        }
        body.push_back(static_cast<uint8_t>(numVar & 0xFF));
        body.push_back(static_cast<uint8_t>((numVar >> 8) & 0xFF));
    }

    const size_t maskBytes = (numCols + 7) / 8;
    const size_t maskAt = body.size();
    body.insert(body.end(), maskBytes, 0);
    for (size_t i = 0; i < nfields; ++i) {
        const Field& f = fields[i];
        const MdbColumn* c = colAt(table, f.colnum);
        if (!c || f.isNull) continue;
        const int byte = c->col_num / 8, bit = c->col_num % 8;
        if (byte < 0 || static_cast<size_t>(byte) >= maskBytes)
            return Status::Fail("packRow: col_num OOB");
        body[maskAt + static_cast<size_t>(byte)] |= static_cast<uint8_t>(1u << bit);
    }
    if (body.size() > 4000) return Status::Fail("packRow: row too big");
    out = std::move(body);
    return Status::Ok();
}

/* ============================ buildEntry ============================ */
static Status appendSegment(const MdbColumn* col, const Field* f, bool desc, ByteVec& out) {
    if (!f || f->isNull || !f->data) {
        out.putU8(desc ? kDescNull : kAscNull); /* Jackcess null rule */
        return Status::Ok();
    }
    if (col->col_type == MDB_INT) {
        if (f->size < 2) return Status::Fail("buildEntry: INT short");
        out.putU8(desc ? kDescStart : kAscStart);
        intPayloadBE(f->data, 2, desc, out);
    } else if (col->col_type == MDB_LONGINT) {
        if (f->size < 4) return Status::Fail("buildEntry: LONG short");
        out.putU8(desc ? kDescStart : kAscStart);
        intPayloadBE(f->data, 4, desc, out);
    } else if (col->col_type == MDB_DOUBLE || col->col_type == MDB_DATETIME) {
        if (f->size < 8) return Status::Fail("buildEntry: DOUBLE short");
        out.putU8(desc ? kDescStart : kAscStart);
        doublePayload(f->data, desc, out);
    } else if (col->col_type == MDB_TEXT) {
        out.putU8(desc ? kDescStart : kAscStart);
        const uint8_t* s = f->data;
        const size_t n = f->size;
        if (n >= 2 && s[0] == 0xFF && s[1] == 0xFE) {
            for (size_t b = 2; b < n; ++b) { /* compressed single bytes */
                if (s[b] == 0) break;
                const uint8_t m = static_cast<uint8_t>(idx_to_text_ling[s[b]]);
                out.putU8(desc ? static_cast<uint8_t>(~m) : m);
                if (out.size() > 240) break;
            }
        } else {
            for (size_t b = 0; b < n; b += 2) { /* UCS-2LE, legacy parity */
                if (s[b] == 0) break;
                const uint8_t m = static_cast<uint8_t>(idx_to_text_ling[s[b]]);
                out.putU8(desc ? static_cast<uint8_t>(~m) : m);
                if (out.size() > 240) break;
            }
        }
        out.putU8(desc ? 0xFE : 0x01);
        out.putU8(desc ? 0xFF : 0x00);
    } else {
        size_t n = f->size;
        if (col->col_size > 0 && col->col_size < 32) n = std::min(n, static_cast<size_t>(col->col_size));
        if (n > 32) n = 32;
        out.putBytes(f->data, n);
    }
    if (out.size() > 248) return Status::Fail("buildEntry: key too long");
    return Status::Ok();
}

Status buildEntry(MdbTableDef* table, MdbIndex* idx,
                  const Field* fields, size_t nfields,
                  uint32_t dataPg, uint16_t rownum,
                  std::vector<uint8_t>& outEntry) {
    if (!table || !idx) return Status::Fail("buildEntry: null table/idx");
    if (idx->num_keys == 0 || idx->num_keys > 10) return Status::Fail("buildEntry: bad num_keys");
    ByteVec key(256);
    for (unsigned k = 0; k < idx->num_keys; ++k) {
        const int keycol = idx->key_col_num[k]; /* 1-based */
        if (keycol <= 0 || keycol > static_cast<int>(table->num_cols)) continue;
        const MdbColumn* c = colAt(table, keycol - 1);
        if (!c) return Status::Fail("buildEntry: missing key column");
        const Field* f = findField(fields, nfields, keycol - 1);
        const bool desc = (idx->key_col_order[k] == MDB_DESC);
        Status st = appendSegment(c, f, desc, key);
        if (!st.ok) return st;
    }
    /* pg_row trailer: 3-byte pg + 1-byte (row-1), big-endian (native layout). */
    key.putU8(static_cast<uint8_t>((dataPg >> 16) & 0xFF));
    key.putU8(static_cast<uint8_t>((dataPg >> 8) & 0xFF));
    key.putU8(static_cast<uint8_t>(dataPg & 0xFF));
    key.putU8(static_cast<uint8_t>((rownum - 1) & 0xFF));
    outEntry = key.release();
    return Status::Ok();
}

/* ============================ leafInsert ============================ */
Status leafInsert(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                  const uint8_t* entry, size_t entryLen) {
    if (!mdb || !table || !table->entry || !idx) return Status::Fail("leafInsert: null arg");
    if (!entry || entryLen < 5 || entryLen > 1500) return Status::Fail("leafInsert: bad entry");
    if (idx->first_pg == 0) return Status::Fail("leafInsert: no root");

    std::vector<uint8_t> key(entry, entry + entryLen);
    std::string err;
    std::vector<uint32_t> ancPath;
    const uint32_t leaf = findLeaf(mdb, idx, key.data(), key.size(), ancPath, err);
    if (!leaf) return Status::Fail("leafInsert: descent failed: " + err);
    {
        std::string kb;
        char tmp[8];
        for (size_t i = 0; i < key.size() && i < 8; ++i) {
            std::snprintf(tmp, sizeof(tmp), "%02x", key[i]);
            kb += tmp;
        }
        debugLog(std::string("leafInsert ") + idx->name + " key[" + kb +
                 "] -> leaf " + std::to_string(leaf));
    }

    if (!pgRead(mdb, leaf)) return Status::Fail("leafInsert: leaf reread failed");
    {
        const auto* b = static_cast<const uint8_t*>(mdb->pg_buf);
        if (b[0] != kPageLeaf) return Status::Fail("leafInsert: target not leaf");
    }
    DecodedPage dp;
    if (!decodeEntries(mdb, dp, err)) return Status::Fail("leafInsert: decode: " + err);

    /* Sorted insert position (key-only compare). Duplicates allowed (Jet). */
    std::vector<uint8_t> fullNew(key.begin(), key.end());
    size_t at = dp.full.size();
    for (size_t i = 0; i < dp.full.size(); ++i) {
        if (cmpKeys(fullNew, dp.full[i]) < 0) { at = i; break; }
    }
    std::vector<std::vector<uint8_t>> merged;
    merged.reserve(dp.full.size() + 1);
    merged.insert(merged.end(), dp.full.begin(), dp.full.begin() + static_cast<ptrdiff_t>(at));
    merged.push_back(std::move(fullNew));
    merged.insert(merged.end(), dp.full.begin() + static_cast<ptrdiff_t>(at), dp.full.end());

    /* Rewrite in place, splitting with parent propagation when full. */
    return insertIntoPage(mdb, table, idx, leaf, true, merged, ancPath);
}

/* ============================ updateIndex ============================ */
int updateIndex(MdbTableDef* table, MdbIndex* idx,
                const Field* fields, size_t nfields,
                uint32_t dataPg, uint16_t rownum, std::string& error) {
    if (!table || !table->entry || !table->entry->mdb || !idx) {
        error = "updateIndex: null arg"; return 0;
    }
    if (idx->first_pg == 0 || idx->index_type == 2) return 0;
    MdbHandle* mdb = table->entry->mdb;
    std::vector<uint8_t> entry;
    Status st = buildEntry(table, idx, fields, nfields, dataPg, rownum, entry);
    if (!st.ok) {
        error = st.error;
        debugLog(std::string("updateIndex ") + idx->name + " build failed: " + st.error);
        return 0;
    }
    st = leafInsert(mdb, table, idx, entry.data(), entry.size());
    if (!st.ok) {
        error = st.error;
        debugLog(std::string("updateIndex ") + idx->name + " insert failed: " + st.error);
        return 0;
    }
    idx->num_rows++;
    /* Persist index cardinality on the table definition page (legacy behavior). */
    if (table->entry->table_pg > 0) {
        const int off = mdb->fmt->tab_cols_start_offset +
                        idx->index_num * mdb->fmt->tab_ridx_entry_size;
        if (pgRead(mdb, static_cast<uint32_t>(table->entry->table_pg))) {
            mdb_put_int32(mdb->pg_buf, off, static_cast<guint32>(idx->num_rows));
            pgWrite(mdb, static_cast<uint32_t>(table->entry->table_pg));
        }
    }
    return static_cast<int>(entry.size());
}

/* ============================ walkIndex ============================ */
Status walkIndex(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                 std::vector<IndexEntryInfo>& out) {
    out.clear();
    if (!mdb || !table || !idx) return Status::Fail("walkIndex: null arg");
    if (idx->first_pg == 0) return Status::Fail("walkIndex: no root");
    /* Descend to leftmost leaf. */
    uint32_t pg = idx->first_pg;
    for (int d = 0; d < 12; ++d) {
        if (!pgRead(mdb, pg)) return Status::Fail("walkIndex: read failed");
        const auto* b = static_cast<const uint8_t*>(mdb->pg_buf);
        if (b[0] == kPageLeaf) break;
        if (b[0] != kPageIndex) return Status::Fail("walkIndex: bad node");
        DecodedPage dp;
        std::string err;
        if (!decodeEntries(mdb, dp, err) || dp.full.empty())
            return Status::Fail("walkIndex: node decode: " + err);
        const auto& div = dp.full.front();
        if (div.size() < 4) return Status::Fail("walkIndex: short divider");
        const size_t dl = div.size() - 4;
        pg = (static_cast<uint32_t>(div[dl]) << 24) |
             (static_cast<uint32_t>(div[dl + 1]) << 16) |
             (static_cast<uint32_t>(div[dl + 2]) << 8) |
             static_cast<uint32_t>(div[dl + 3]);
        if (!pg) return Status::Fail("walkIndex: null child");
        if (d == 11) return Status::Fail("walkIndex: too deep");
    }
    /* Follow next-leaf chain (correct Jet4 @16). */
    for (int hops = 0; hops < 100000; ++hops) {
        if (!pgRead(mdb, pg)) return Status::Fail("walkIndex: leaf read failed");
        const auto* b = static_cast<const uint8_t*>(mdb->pg_buf);
        if (b[0] != kPageLeaf) return Status::Fail("walkIndex: chain hit non-leaf");
        DecodedPage dp;
        std::string err;
        if (!decodeEntries(mdb, dp, err)) return Status::Fail("walkIndex: leaf decode: " + err);
        for (const auto& e : dp.full) {
            if (e.size() < 4) continue;
            IndexEntryInfo info;
            info.fullKey.assign(e.begin(), e.end() - 4);
            const size_t t = e.size() - 4;
            /* Trailer is ((dataPg << 8) | rowIdx) packed big-endian. */
            const uint32_t pgRow = (static_cast<uint32_t>(e[t]) << 24) |
                                   (static_cast<uint32_t>(e[t + 1]) << 16) |
                                   (static_cast<uint32_t>(e[t + 2]) << 8) |
                                   static_cast<uint32_t>(e[t + 3]);
            info.dataPg = pgRow >> 8;
            info.rowIdx = static_cast<uint16_t>(pgRow & 0xFF);
            info.leafPg = pg;
            out.push_back(std::move(info));
        }
        const uint32_t nxt = pgGet32(mdb, kOffNextPg);
        if (!nxt || nxt == pg) break;
        pg = nxt;
    }
    return Status::Ok();
}

/* ============================ verifyDatabase ============================ */
VerifyReport verifyDatabase(MdbHandle* mdb) {
    VerifyReport rep;
    if (!mdb) { rep.error = "null handle"; return rep; }
    const char* tables[] = {"Transactions", "Ledgers", "Groups", "CompanyInfo", nullptr};
    for (int ti = 0; tables[ti]; ++ti) {
        MdbTableDef* t = mdb_read_table_by_name(mdb, const_cast<char*>(tables[ti]), MDB_TABLE);
        if (!t) { rep.error = std::string("missing table ") + tables[ti]; return rep; }
        mdb_read_columns(t);
        mdb_read_indices(t);
        ++rep.tablesChecked;

        long cracked = 0;
        {
            std::vector<MdbField> fb(t->num_cols ? t->num_cols : 1);
            mdb_rewind_table(t);
            while (mdb_fetch_row(t)) {
                int rs = 0;
                size_t rsz = 0;
                mdb_find_row(mdb, static_cast<int>(t->cur_row) - 1, &rs, &rsz);
                const int n = mdb_crack_row(t, rs & 0x0FFF, rsz, fb.data());
                if (n < 0) ++rep.badRows;
                else ++cracked;
            }
        }
        rep.dataRows += cracked;

        if (t->indices) {
            for (unsigned ii = 0; ii < t->indices->len; ++ii) {
                MdbIndex* idx = static_cast<MdbIndex*>(g_ptr_array_index(t->indices, ii));
                VerifyIndexInfo vi;
                vi.table = tables[ti];
                vi.name = idx->name ? idx->name : "?";
                vi.catalogRows = idx->num_rows;
                vi.dataRows = cracked;
                std::vector<IndexEntryInfo> entries;
                Status st = walkIndex(mdb, t, idx, entries);
                if (!st.ok) {
                    mdb_free_tabledef(t);
                    rep.error = std::string("walk failed ") + vi.table + "." + vi.name + ": " + st.error;
                    return rep;
                }
                vi.walked = static_cast<long>(entries.size());
                vi.sorted = true;
                for (size_t i = 1; i < entries.size(); ++i) {
                    const auto& a = entries[i - 1].fullKey;
                    const auto& b = entries[i].fullKey;
                    const size_t m = std::min(a.size(), b.size());
                    int c = m ? std::memcmp(a.data(), b.data(), m) : 0;
                    if (c == 0) c = (a.size() < b.size()) ? -1 : (a.size() > b.size() ? 1 : 0);
                    if (c > 0) { vi.sorted = false; break; }
                }
                rep.indexes.push_back(std::move(vi));
            }
        }
        mdb_free_tabledef(t);
    }
    rep.ok = rep.badRows == 0;
    for (const auto& vi : rep.indexes) {
        if (!vi.sorted || vi.walked != vi.dataRows) { rep.ok = false; break; }
    }
    return rep;
}

} /* namespace Jet4Writer */

/* ============================ C ABI ============================ */
/* CRITICAL: null vs empty must survive conversion. A zero-length but NOT
 * NULL field (e.g. EntryType "") must keep a valid data pointer so the key
 * builder emits an empty-TEXT segment (7F 01 00), not a NULL flag (00).
 * Jet distinguishes them and Seek misses on mismatch (Error 3709). */
static const uint8_t kEmptySentinel = 0;
static Jet4Writer::Field toJetField(const MdbField& m) {
    Jet4Writer::Field f;
    f.colnum = m.colnum;
    f.isNull = true;
    f.data = nullptr;
    f.size = 0;
    if (m.is_null != 0) return f;                    /* explicitly NULL */
    if (m.value == nullptr) {
        if (m.siz > 0) return f;                     /* broken input -> NULL */
        f.isNull = false;                            /* empty (Qt null-data safe) */
        f.data = &kEmptySentinel;
        return f;
    }
    f.isNull = false;                                /* not null (maybe empty) */
    f.data = static_cast<const uint8_t*>(m.value);
    f.size = m.siz > 0 ? static_cast<size_t>(m.siz) : 0;
    return f;
}

extern "C" int jet4_pack_row(MdbTableDef* table, MdbField* fields, unsigned num_fields,
                             unsigned char* out, unsigned out_size) {
    if (!table || !fields || !out || !out_size) return -1;
    std::vector<Jet4Writer::Field> fs;
    fs.reserve(num_fields);
    for (unsigned i = 0; i < num_fields; ++i) fs.push_back(toJetField(fields[i]));
    std::vector<uint8_t> packed;
    Jet4Writer::Status st = Jet4Writer::packRow(table, fs.data(), fs.size(), packed);
    if (!st.ok || packed.size() > out_size) return -1;
    std::memcpy(out, packed.data(), packed.size());
    return static_cast<int>(packed.size());
}

extern "C" int jet4_update_index(MdbTableDef* table, MdbIndex* idx, MdbField* fields,
                                unsigned num_fields, unsigned data_pg, unsigned rownum) {
    if (!table || !idx || !fields) return 0;
    std::vector<Jet4Writer::Field> fs;
    fs.reserve(num_fields);
    for (unsigned i = 0; i < num_fields; ++i) fs.push_back(toJetField(fields[i]));
    std::string err;
    return Jet4Writer::updateIndex(table, idx, fs.data(), fs.size(), data_pg,
                                  static_cast<uint16_t>(rownum), err);
}
