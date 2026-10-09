#include "jet4_btree.h"
#include "jet4_codes.h"
#include "jet4_usage_map.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <set>

extern "C" {
int mdb_index_unpack_bitmap(MdbHandle* mdb, MdbIndexPage* ipg);
int mdb_index_pack_bitmap(MdbHandle* mdb, MdbIndexPage* ipg);
}

namespace Jet4Writer {

/* Jackcess Entry.compareTo total order: entryBytes first (unsigned memcmp
 * with length tiebreak), then rowId trailer (data page, then row number).
 * Node divider child links never participate. Duplicate keys MUST be
 * rowId-ordered: dividers use the positional back as the child max, so an
 * unordered duplicate run makes the divider stale-small and descents
 * misroute (keys above the divider but physically on the page). */
int BTreeEngine::cmpIndexEntries(const std::vector<uint8_t>& a, bool aIsLeaf,
                                 const std::vector<uint8_t>& b, bool bIsLeaf) {
    size_t alen = a.size(), blen = b.size();
    if (!aIsLeaf) {
        alen = alen > 4 ? alen - 4 : 0;
    }
    if (!bIsLeaf) {
        blen = blen > 4 ? blen - 4 : 0;
    }
    // Key bytes: everything except the trailing 4-byte rowId.
    const size_t akey = alen > 4 ? alen - 4 : 0;
    const size_t bkey = blen > 4 ? blen - 4 : 0;
    const size_t m = std::min(akey, bkey);
    int c = m ? std::memcmp(a.data(), b.data(), m) : 0;
    if (c == 0 && akey != bkey) c = (akey < bkey) ? -1 : 1;
    if (c == 0) {
        // RowId trailers: 3-byte big-endian data page + 1-byte row.
        const size_t am = alen > akey ? alen - akey : 0;
        const size_t bm = blen > bkey ? blen - bkey : 0;
        const size_t m2 = std::min(am, bm);
        c = m2 ? std::memcmp(a.data() + akey, b.data() + bkey, m2) : 0;
        if (c == 0 && am != bm) c = (am < bm) ? -1 : 1;
    }
    return (c < 0) ? -1 : ((c > 0) ? 1 : 0);
}

int BTreeEngine::cmpIndexOrder(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b, bool isLeaf) {
    return cmpIndexEntries(a, isLeaf, b, isLeaf);
}

uint32_t BTreeEngine::childOf(const std::vector<uint8_t>& divEntry) {
    if (divEntry.size() < 4) return 0;
    const size_t d = divEntry.size() - 4;
    return (static_cast<uint32_t>(divEntry[d]) << 24) |
           (static_cast<uint32_t>(divEntry[d + 1]) << 16) |
           (static_cast<uint32_t>(divEntry[d + 2]) << 8) |
           static_cast<uint32_t>(divEntry[d + 3]);
}

/* Construct Jackcess-compliant node divider:
 * Layout: [column_key_bytes] + [4-byte rowId] + [4-byte childPg].
 * If child was leaf, childMaxEntry is already [key + rowId], so append childPg.
 * If child was node, childMaxEntry has [key + rowId + grandChildPg], so strip last 4 bytes and append childPg. */
std::vector<uint8_t> BTreeEngine::makeDivider(bool wasLeaf, const std::vector<uint8_t>& childMaxEntry, uint32_t childPg) {
    std::vector<uint8_t> base = wasLeaf ? childMaxEntry
                                        : std::vector<uint8_t>(childMaxEntry.begin(), childMaxEntry.end() - 4);
    base.push_back(static_cast<uint8_t>((childPg >> 24) & 0xFF));
    base.push_back(static_cast<uint8_t>((childPg >> 16) & 0xFF));
    base.push_back(static_cast<uint8_t>((childPg >> 8) & 0xFF));
    base.push_back(static_cast<uint8_t>(childPg & 0xFF));
    return base;
}

bool BTreeEngine::decodeEntries(MdbHandle* mdb, DecodedPage& dp, std::string& err) {
    const auto* buf = static_cast<const uint8_t*>(mdb->pg_buf);
    if (buf[0] != kPageLeaf && buf[0] != kPageIndex) {
        err = "decodeEntries: not an index page"; return false;
    }
    MdbIndexPage ipg;
    std::memset(&ipg, 0, sizeof(ipg));
    const int bounds = mdb_index_unpack_bitmap(mdb, &ipg);
    if (bounds < 1) { err = "bitmap unpack failed"; return false; }
    const int nEntries = bounds - 1; /* boundaries = entries + 1 */
    if (nEntries < 0 || nEntries > 2000) { err = "entry count insane"; return false; }
    const uint16_t cb = UsageMapManager::getU16(mdb, kOffPrefixLen);
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
            full.assign(buf + s, buf + e);
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

std::vector<uint8_t> BTreeEngine::getSubtreeMax(MdbHandle* mdb, uint32_t pg) {
    if (!mdb || pg == 0) return {};
    uint32_t cur = pg;
    for (int d = 0; d < 12 && cur > 0; ++d) {
        if (!UsageMapManager::readPage(mdb, cur)) return {};
        const auto* buf = static_cast<const uint8_t*>(mdb->pg_buf);
        if (buf[0] == kPageLeaf) {
            DecodedPage dp;
            std::string err;
            if (!decodeEntries(mdb, dp, err) || dp.full.empty()) return {};
            return dp.full.back(); // [key + rowId]
        }
        if (buf[0] != kPageIndex) return {};
        const uint32_t tail = UsageMapManager::getU32(mdb, kOffChildTailPg);
        if (tail != 0) {
            cur = tail;
            continue;
        }
        // No tail: last physical divider's [key + rowId]
        DecodedPage dp;
        std::string err;
        if (!decodeEntries(mdb, dp, err) || dp.full.empty()) return {};
        const auto& last = dp.full.back();
        if (last.size() < 4) return {};
        return std::vector<uint8_t>(last.begin(), last.end() - 4);
    }
    return {};
}

Status BTreeEngine::buildEntry(MdbTableDef* table, MdbIndex* idx,
                              const Field* fields, size_t nfields,
                              uint32_t dataPg, uint16_t rownum,
                              std::vector<uint8_t>& outEntry) {
    if (!table || !idx) return Status::Fail("buildEntry: null table/idx");
    if (idx->num_keys == 0 || idx->num_keys > 10) return Status::Fail("buildEntry: bad num_keys");
    ByteVec key(256);
    for (unsigned k = 0; k < idx->num_keys; ++k) {
        const int keycol = idx->key_col_num[k];
        if (keycol <= 0 || keycol > static_cast<int>(table->num_cols)) continue;
        const MdbColumn* c = ColumnEncoder::getColumn(table, keycol - 1);
        if (!c) return Status::Fail("buildEntry: missing key column");
        const Field* f = ColumnEncoder::findField(fields, nfields, keycol - 1);
        const bool desc = (idx->key_col_order[k] == MDB_DESC);
        Status st = ColumnEncoder::appendSegment(c, f, desc, key);
        if (!st.ok) return st;
    }
    /* pg_row trailer: 3-byte pg + 1-byte (row-1), big-endian */
    key.putU8(static_cast<uint8_t>((dataPg >> 16) & 0xFF));
    key.putU8(static_cast<uint8_t>((dataPg >> 8) & 0xFF));
    key.putU8(static_cast<uint8_t>(dataPg & 0xFF));
    key.putU8(static_cast<uint8_t>((rownum - 1) & 0xFF));
    outEntry = key.release();
    return Status::Ok();
}

uint32_t BTreeEngine::findLeaf(MdbHandle* mdb, MdbIndex* idx,
                              const uint8_t* key, size_t keyLen,
                              std::vector<uint32_t>& ancPath, std::string& err) {
    ancPath.clear();
    uint32_t pg = idx->first_pg;
    for (int depth = 0; depth < 12 && pg > 0; ++depth) {
        if (!UsageMapManager::readPage(mdb, pg)) { err = "descent read failed"; return 0; }
        const auto* buf = static_cast<const uint8_t*>(mdb->pg_buf);
        if (buf[0] == kPageLeaf) return pg;
        if (buf[0] != kPageIndex) { err = "descent hit non-index page"; return 0; }
        DecodedPage dp;
        if (!decodeEntries(mdb, dp, err)) return 0;
        if (dp.full.empty()) { err = "empty index node"; return 0; }
        const uint32_t tail = UsageMapManager::getU32(mdb, kOffChildTailPg);
        uint32_t child = 0;
        bool exhausted = true;
        const std::vector<uint8_t> keyVec(key, key + keyLen);
        for (size_t i = 0; i < dp.full.size(); ++i) {
            const auto& div = dp.full[i];
            if (div.size() < 4) continue;
            const uint32_t cpg = childOf(div);
            int cmp = cmpIndexEntries(keyVec, true, div, false);
            if (cmp <= 0) { child = cpg; exhausted = false; break; }
            if (i + 1 == dp.full.size()) {
                child = (tail != 0) ? tail : cpg;
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

/* Jackcess prefix policy shared by writeLeaf and the splitter:
 * EMPTY prefix when everything fits, else the clamped common(first,last)
 * prefix. Returns false when even that overflows (page genuinely too full
 * for these entries as one page). Mirrors IndexPageCache prefix handling. */
static bool computeFittingPrefix(const std::vector<std::vector<uint8_t>>& fullEntries,
                                 size_t& cbOut) {
    if (fullEntries.empty() || fullEntries.size() > 2000) return false;
    for (const auto& e : fullEntries) {
        if (e.size() < 4 || e.size() > 1500) return false;
    }
    auto fits = [&](size_t cb, size_t& storedTotal) {
        storedTotal = cb;
        for (const auto& e : fullEntries) {
            if (e.size() < cb) return false;
            storedTotal += e.size() - cb;
        }
        const size_t maskBytes = (storedTotal + 7) / 8;
        return (kOffEntryMask + maskBytes <= static_cast<size_t>(kEntryAreaStart)) &&
               (static_cast<size_t>(kEntryAreaStart) + storedTotal <= static_cast<size_t>(kPageSize));
    };
    size_t storedTotal = 0;
    if (fits(0, storedTotal)) { cbOut = 0; return true; }
    size_t cb = ColumnEncoder::commonPrefixLength(fullEntries.front(), fullEntries.back());
    for (const auto& e : fullEntries) cb = std::min(cb, e.size() > 4 ? e.size() - 4 : 0);
    if (cb == 0 || !fits(cb, storedTotal)) return false;
    for (const auto& e : fullEntries) cb = std::min(cb, e.size() > 4 ? e.size() - 4 : 0);
    if (!fits(cb, storedTotal)) return false;
    cbOut = cb;
    return true;
}

bool BTreeEngine::writeLeaf(MdbHandle* mdb, uint32_t leafPg, bool isLeaf,
                            const std::vector<std::vector<uint8_t>>& fullEntries,
                            std::string& err) {
    size_t cb = 0;
    if (!computeFittingPrefix(fullEntries, cb)) {
        err = "writeLeaf: page overflow";
        return false;
    }

    if (!UsageMapManager::readPage(mdb, leafPg)) { err = "writeLeaf: reread failed"; return false; }
    std::vector<uint8_t> page(kPageSize, 0);
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
    put32(kOffTableDefPg, UsageMapManager::getU32(mdb, kOffTableDefPg));
    put32(kOffUnknown, 0);
    put32(kOffPrevPg, UsageMapManager::getU32(mdb, kOffPrevPg));
    put32(kOffNextPg, UsageMapManager::getU32(mdb, kOffNextPg));
    put32(kOffChildTailPg, UsageMapManager::getU32(mdb, kOffChildTailPg));
    put16(kOffPrefixLen, static_cast<uint16_t>(cb));
    /* Jackcess IndexData.writeDataPage: byte 26 is the tree level (0 = leaf,
     * parent = child + 1). MS Jet validates it; zeroed node levels misread.
     * Preserve the existing page's level (fresh pages are staged by the
     * caller before writeLeaf runs). */
    page[kOffPrefixUnknown] = cur[kOffPrefixUnknown];

    /* Stored layout: [prefix][suffix...], bitmap over cumulative ends. */
    size_t pos = kEntryAreaStart;
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
    return UsageMapManager::writePage(mdb, leafPg);
}

Status BTreeEngine::splitPage(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                              uint32_t pg, bool wasLeaf,
                              const std::vector<std::vector<uint8_t>>& merged,
                              std::vector<uint32_t>& ancPath) {
    if (merged.size() < 2) return Status::Fail("splitPage: nothing to split");
    const bool isRoot = ancPath.empty();
    if (std::getenv("JET4_DEBUG")) fprintf(stderr, "[jet4] splitPage pg=%u leaf=%d n=%zu root=%d\n",
                                           pg, (int)wasLeaf, merged.size(), (int)isRoot);
    /* 0. Read existing links on pg before allocating (allocation clobbers pg_buf). */
    if (!UsageMapManager::readPage(mdb, pg)) return Status::Fail("splitPage: reread failed");
    const uint32_t oldPrev = UsageMapManager::getU32(mdb, kOffPrevPg);
    const uint32_t oldNext = UsageMapManager::getU32(mdb, kOffNextPg);
    const uint32_t oldTail = wasLeaf ? 0 : UsageMapManager::getU32(mdb, kOffChildTailPg);
    /* Jackcess tree level (byte 26): leaves 0, nodes parent = child + 1.
     * Chunk pages inherit pg's level; a promoted root goes one above. */
    const uint8_t oldLevel = wasLeaf ? 0 : static_cast<const uint8_t*>(mdb->pg_buf)[kOffPrefixUnknown];
    std::vector<uint8_t> oldMax = getSubtreeMax(mdb, pg);

    /* 1. Partition merged into page-fitting chunks. Jackcess
     * IndexPageCache.splitDataPage naively moves the FIRST HALF to the new
     * page (orig keeps the second half). Halving leaves both pages half
     * full, so ~half a page of inserts land before the next split
     * (amortized O(1)). Greedy-maximal chunks instead leave the first chunk
     * AT capacity, so the next insert there overflows again: a split per
     * insert with 1-entry dribbles and runaway page growth. If a half does
     * not fit (pathological prefix destruction), halve recursively exactly
     * like Jackcess's updatePages re-split loop. */
    auto chunkFits = [&](size_t begin, size_t end) {
        if (begin >= end || end > merged.size()) return false;
        std::vector<std::vector<uint8_t>> probe(merged.begin() + (ptrdiff_t)begin,
                                                merged.begin() + (ptrdiff_t)end);
        size_t cb = 0;
        return computeFittingPrefix(probe, cb);
    };
    if (!chunkFits(0, 1)) return Status::Fail("splitPage: entry too large");
    std::vector<std::pair<size_t, size_t>> ranges;
    {
        std::vector<std::pair<size_t, size_t>> work;
        work.emplace_back(0, merged.size());
        for (size_t guard = 0; guard < 64 && !work.empty(); ++guard) {
            auto [begin, end] = work.back();
            work.pop_back();
            if (end - begin < 2) {
                // Single entry that still overflows: genuinely too large.
                if (!chunkFits(begin, end))
                    return Status::Fail("splitPage: entry too large");
                ranges.emplace_back(begin, end);
                continue;
            }
            if (chunkFits(begin, end)) {
                ranges.emplace_back(begin, end);
                continue;
            }
            const size_t mid = begin + (end - begin + 1) / 2;
            work.emplace_back(mid, end);
            work.emplace_back(begin, mid);
        }
        if (!work.empty()) return Status::Fail("splitPage: cannot partition");
        std::sort(ranges.begin(), ranges.end());
    }
    if (ranges.size() < 2) {
        // Fits on one page after all; rewrite in place.
        std::string err;
        if (writeLeaf(mdb, pg, wasLeaf, merged, err)) return Status::Ok();
        return Status::Fail("splitPage: cannot partition (" + err + ")");
    }
    const size_t k = ranges.size();
    if (std::getenv("JET4_DEBUG")) {
        fprintf(stderr, "[jet4] splitPage pg=%u chunks=%zu sizes:", pg, k);
        for (auto& rr : ranges) fprintf(stderr, " %zu", rr.second - rr.first);
        fprintf(stderr, "\n");
    }
    auto chunkOf = [&](size_t i) {
        return std::vector<std::vector<uint8_t>>(merged.begin() + (ptrdiff_t)ranges[i].first,
                                                  merged.begin() + (ptrdiff_t)ranges[i].second);
    };

    /* 2. Page assignment, head-off (Jackcess splitDataPage): orig pg keeps
     * the UPPER (last) chunk, so its max — and therefore its existing parent
     * divider and tail status — stay valid; only ADDs are needed afterwards
     * (never replace, never tail surgery). New pages take chunks [0, k-2] and
     * chain BEFORE pg. Root becomes a pure node (page number stays, so no
     * catalog change) and every chunk moves to a fresh page. Allocate +
     * register FIRST so every fallible global step precedes any mutation. */
    std::vector<uint32_t> pages;
    pages.reserve(k);
    if (!isRoot) {
        for (size_t i = 0; i + 1 < k; ++i) {
            const gint32 nSigned = UsageMapManager::allocateIndexPage(mdb, table);
            if (nSigned <= 0) return Status::Fail("splitPage: allocate failed");
            const uint32_t newPg = static_cast<uint32_t>(nSigned);
            Status rg = UsageMapManager::registerIndexPage(mdb, table, idx, newPg);
            if (!rg.ok) return rg;
            pages.push_back(newPg);
        }
        pages.push_back(pg); // last chunk stays home; max unchanged
    } else {
        for (size_t i = 0; i < k; ++i) {
            const gint32 nSigned = UsageMapManager::allocateIndexPage(mdb, table);
            if (nSigned <= 0) return Status::Fail("splitPage: allocate failed");
            const uint32_t newPg = static_cast<uint32_t>(nSigned);
            Status rg = UsageMapManager::registerIndexPage(mdb, table, idx, newPg);
            if (!rg.ok) return rg;
            pages.push_back(newPg);
        }
    }

    /* 3. Stage headers + write chunks in chain order. */
    {
        std::string err;
        for (size_t i = 0; i < k; ++i) {
            const uint32_t cpg = pages[i];
            if (!UsageMapManager::readPage(mdb, cpg)) return Status::Fail("splitPage: stage read failed");
            auto* b = static_cast<uint8_t*>(mdb->pg_buf);
            b[0] = wasLeaf ? kPageLeaf : kPageIndex;
            b[1] = 0x01;
            UsageMapManager::putU32(mdb, kOffTableDefPg, static_cast<uint32_t>(table->entry->table_pg));
            UsageMapManager::putU32(mdb, kOffUnknown, 0);
            uint32_t wantPrev = 0, wantNext = 0;
            if (isRoot) {
                // Fresh chain among chunk pages; root pg itself becomes a node below.
                wantPrev = (i == 0) ? 0 : pages[i - 1];
                wantNext = (i + 1 < k) ? pages[i + 1] : 0;
            } else if (i + 1 < k) {
                // New page before pg: chain into position.
                wantPrev = (i == 0) ? oldPrev : pages[i - 1];
                wantNext = pages[i + 1];
            } else {
                // Last chunk stays on pg: only its prev changes (to last new page).
                wantPrev = pages[i - 1];
                wantNext = oldNext;
            }
            UsageMapManager::putU32(mdb, kOffPrevPg, wantPrev);
            UsageMapManager::putU32(mdb, kOffNextPg, wantNext);
            // Tree level (Jackcess byte 26): chunk pages inherit pg's level
            // (leaves 0). writeLeaf preserves this staged value.
            b[kOffPrefixUnknown] = oldLevel;
            // Subtree tails: Jackcess splitDataPage never moves them — orig
            // keeps its tail, fresh pages start tail-less. Zeroing pg's tail
            // here used to orphan whole subtrees (entire key ranges vanishing
            // from walks while staying present on disk).
            // Root chunks: first k-1 bounded (tail 0); the last chunk page
            // inherits the old subtree tail (it covers the top range).
            // Non-root: pg (last chunk) keeps oldTail; fresh pages tail-less.
            uint32_t wantTail = 0;
            if (!wasLeaf) {
                if (isRoot) wantTail = (i + 1 == k) ? oldTail : 0;
                else wantTail = (cpg == pg) ? oldTail : 0;
            }
            UsageMapManager::putU32(mdb, kOffChildTailPg, wantTail);
            if (!UsageMapManager::writePage(mdb, cpg)) return Status::Fail("splitPage: stage failed");
            if (!writeLeaf(mdb, cpg, wasLeaf, chunkOf(i), err))
                return Status::Fail("splitPage: chunk write failed: " + err);
        }
        /* Peer maintenance on both sides (Jackcess addToPeersBefore): the
         * page before the chain now leads into it. */
        if (!isRoot && oldPrev != 0) {
            if (!UsageMapManager::readPage(mdb, oldPrev)) return Status::Fail("splitPage: prev read failed");
            const auto* b0 = static_cast<const uint8_t*>(mdb->pg_buf);
            if (b0[0] == kPageLeaf || b0[0] == kPageIndex) {
                UsageMapManager::putU32(mdb, kOffNextPg, pages[0]);
                if (!UsageMapManager::writePage(mdb, oldPrev)) return Status::Fail("splitPage: prev write failed");
            }
        }
        /* Forward-link maintenance: oldNext->prev = last chunk page. */
        if (oldNext != 0) {
            if (!UsageMapManager::readPage(mdb, oldNext)) return Status::Fail("splitPage: fwd read failed");
            const auto* b = static_cast<const uint8_t*>(mdb->pg_buf);
            if (b[0] == kPageLeaf || b[0] == kPageIndex) {
                UsageMapManager::putU32(mdb, kOffPrevPg, pages.back());
                if (!UsageMapManager::writePage(mdb, oldNext)) return Status::Fail("splitPage: fwd write failed");
            }
        }
    }

    /* 4. Root pg itself becomes a pure node (type + zeroed links; the table
     * definition page keeps pointing at the same root number). Its level is
     * one above the chunks it now parents (Jackcess nestRootDataPage). */
    if (isRoot) {
        if (!UsageMapManager::readPage(mdb, pg)) return Status::Fail("splitPage: root reread failed");
        {
            auto* b = static_cast<uint8_t*>(mdb->pg_buf);
            b[0] = kPageIndex;
            b[1] = 0x01;
            b[kOffPrefixUnknown] = static_cast<uint8_t>(oldLevel + 1);
        }
        UsageMapManager::putU32(mdb, kOffPrevPg, 0);
        UsageMapManager::putU32(mdb, kOffNextPg, 0);
        UsageMapManager::putU32(mdb, kOffChildTailPg, 0);
        if (!UsageMapManager::writePage(mdb, pg)) return Status::Fail("splitPage: root stage failed");
    }

    /* 5. Parent divider maintenance. Jackcess updateParentEntry: a divider is
     * the child's max entry as a NodeEntry (full entry bytes incl. the 4-byte
     * rowId trailer, plus the 4-byte child page). Non-root: replace orig
     * divider with div(chunk0->pages[0]) and insert the rest in order; tail
     * child (orig divider absent): append dividers for all but the last
     * page, last page takes over the child-tail link. Root: node gets one
     * divider per chunk page, tail = last. */
    std::vector<std::vector<uint8_t>> divs;
    divs.reserve(k);
    for (size_t i = 0; i < k; ++i) {
        const auto ch = chunkOf(i);
        divs.push_back(makeDivider(wasLeaf, ch.back(), pages[i]));
    }
    if (isRoot) {
        std::string err;
        // Jackcess nestRootDataPage: root holds dividers for all-but-last
        // child; the last chunk page is reached via the child-tail link.
        std::vector<std::vector<uint8_t>> rootDivs(divs.begin(), divs.begin() + (ptrdiff_t)(divs.size() - 1));
        if (!writeLeaf(mdb, pg, false, rootDivs, err))
            return Status::Fail("splitPage: root dividers overflow: " + err);
        // writeLeaf preserves links; now point the tail at the last chunk.
        UsageMapManager::putU32(mdb, kOffChildTailPg, pages.back());
        if (!UsageMapManager::writePage(mdb, pg)) return Status::Fail("splitPage: root tail write failed");
        return Status::Ok();
    }
    const uint32_t parent = ancPath.back();
    ancPath.pop_back();
    if (!UsageMapManager::readPage(mdb, parent)) return Status::Fail("splitPage: parent read failed");
    DecodedPage pdp;
    {
        std::string derr;
        if (!decodeEntries(mdb, pdp, derr))
            return Status::Fail("splitPage: parent decode: " + derr);
    }
    /* Parent divider maintenance (Jackcess updateParentEntry/addParentEntry):
     * orig pg kept chunk k-1 and kept oldTail, so its max and tail status in
     * parent remain intact. Only newly created chunk pages [pages[0] .. pages[k-2]]
     * need fresh dividers added to parent. If pg was a leaf whose max changed,
     * update pg's divider in parent. */
    std::vector<std::vector<uint8_t>> pmerged = pdp.full;
    std::vector<uint8_t> newPgMax = getSubtreeMax(mdb, pg);
    if (!oldMax.empty() && newPgMax != oldMax) {
        for (auto& e : pmerged) {
            if (childOf(e) == pg) {
                e = makeDivider(true, newPgMax, pg);
                break;
            }
        }
    }
    for (size_t i = 0; i + 1 < k; ++i) {
        auto d = makeDivider(wasLeaf, chunkOf(i).back(), pages[i]);
        size_t at = pmerged.size();
        for (size_t j = 0; j < pmerged.size(); ++j) {
            if (cmpIndexOrder(d, pmerged[j], false) < 0) { at = j; break; }
        }
        pmerged.insert(pmerged.begin() + (ptrdiff_t)at, d);
    }
    for (size_t i = 1; i < pmerged.size(); ++i) {
        if (cmpIndexOrder(pmerged[i], pmerged[i - 1], false) < 0) {
            std::sort(pmerged.begin(), pmerged.end(), [](const auto& a, const auto& b) {
                return cmpIndexOrder(a, b, false) < 0;
            });
            break;
        }
    }
    return insertIntoPage(mdb, table, idx, parent, false, pmerged, ancPath);
}

Status BTreeEngine::insertIntoPage(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                                  uint32_t pg, bool isLeaf,
                                  const std::vector<std::vector<uint8_t>>& merged,
                                  std::vector<uint32_t>& ancPath) {
    if (merged.empty() || merged.size() > 2000)
        return Status::Fail("insertIntoPage: bad merged size");
    /* Verify caller's merged list is actually sorted (full Jackcess order:
     * key bytes, then rowId; node child links excluded). */
    for (size_t i = 1; i < merged.size(); ++i) {
        if (merged[i].size() < 4 || merged[i - 1].size() < 4)
            return Status::Fail("insertIntoPage: short entry");
        if (cmpIndexOrder(merged[i], merged[i - 1], isLeaf) < 0)
            return Status::Fail("insertIntoPage: merged list unsorted");
    }
    std::string err;
    if (!UsageMapManager::readPage(mdb, pg)) return Status::Fail("insertIntoPage: reread failed");
    const auto* b = static_cast<const uint8_t*>(mdb->pg_buf);
    const uint8_t want = isLeaf ? kPageLeaf : kPageIndex;
    if (b[0] != want) return Status::Fail("insertIntoPage: page type changed");
    // Capture the pre-write max for divider maintenance below.
    std::vector<uint8_t> oldMax = getSubtreeMax(mdb, pg);

    if (writeLeaf(mdb, pg, isLeaf, merged, err)) {
        /* Jackcess updateDataPage -> replaceParentEntry: a plain rewrite
         * that changes this page's max must propagate the new max upward,
         * else parent dividers go stale and later descents misroute. Base is
         * the [key+rowId] max (node child links stripped). */
        std::vector<uint8_t> newMax = getSubtreeMax(mdb, pg);
        if (!ancPath.empty() && !oldMax.empty() && newMax != oldMax) {
            Status ps = propagateMaxChange(mdb, table, idx, pg, newMax, ancPath);
            if (!ps.ok) return ps;
        }
        return Status::Ok();
    }
    if (err.find("overflow") == std::string::npos)
        return Status::Fail("insertIntoPage: rewrite failed: " + err);
    return splitPage(mdb, table, idx, pg, isLeaf, merged, ancPath);
}

Status BTreeEngine::propagateMaxChange(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                                       uint32_t childPg, const std::vector<uint8_t>& baseKeyRowId,
                                       std::vector<uint32_t>& ancPath) {
    if (baseKeyRowId.size() < 5) return Status::Fail("propagateMaxChange: short base");
    uint32_t curChild = childPg;
    // ancPath is root-first; walk from the immediate parent upward.
    for (size_t ai = ancPath.size(); ai-- > 0;) {
        const uint32_t parent = ancPath[ai];
        if (!UsageMapManager::readPage(mdb, parent)) return Status::Fail("propagateMaxChange: parent read failed");
        const auto* pb = static_cast<const uint8_t*>(mdb->pg_buf);
        if (pb[0] != kPageIndex) return Status::Fail("propagateMaxChange: parent not node");
        DecodedPage pdp;
        {
            std::string derr;
            if (!decodeEntries(mdb, pdp, derr))
                return Status::Fail("propagateMaxChange: parent decode: " + derr);
        }
        if (pdp.full.empty()) return Status::Fail("propagateMaxChange: empty parent");
        ssize_t divPos = -1;
        for (size_t i = 0; i < pdp.full.size(); ++i) {
            if (childOf(pdp.full[i]) == curChild) { divPos = (ssize_t)i; break; }
        }
        // Divider for curChild with the new max (child link preserved).
        std::vector<uint8_t> newDiv = baseKeyRowId;
        newDiv.push_back(static_cast<uint8_t>((curChild >> 24) & 0xFF));
        newDiv.push_back(static_cast<uint8_t>((curChild >> 16) & 0xFF));
        newDiv.push_back(static_cast<uint8_t>((curChild >> 8) & 0xFF));
        newDiv.push_back(static_cast<uint8_t>(curChild & 0xFF));
        if (divPos >= 0) {
            if (pdp.full[(size_t)divPos] == newDiv) return Status::Ok(); // already current; higher levels unaffected
            // Erase + sorted re-insert (never in-place replace: a grown max
            // past the next sibling would fail the sorted check AFTER the
            // leaf was already rewritten, leaving a stale divider behind).
            std::vector<std::vector<uint8_t>> pmerged = pdp.full;
            pmerged.erase(pmerged.begin() + divPos);
            size_t at = pmerged.size();
            for (size_t i = 0; i < pmerged.size(); ++i) {
                if (cmpIndexOrder(newDiv, pmerged[i], false) < 0) { at = i; break; }
            }
            pmerged.insert(pmerged.begin() + (ptrdiff_t)at, newDiv);
            std::vector<uint32_t> above(ancPath.begin(), ancPath.begin() + (ptrdiff_t)ai);
            Status st = insertIntoPage(mdb, table, idx, parent, false, pmerged, above);
            if (!st.ok) return st;
            return Status::Ok();
        }
        // No divider: true tail child needs nothing in parent (tail link routes it),
        // but its new max is the parent's new max: continue upward.
        if (UsageMapManager::getU32(mdb, kOffChildTailPg) == curChild) {
            curChild = parent;
            continue;
        }
        // Otherwise historical damage: heal by inserting at sorted position.
        std::vector<std::vector<uint8_t>> pmerged = pdp.full;
        size_t at = pmerged.size();
        for (size_t i = 0; i < pmerged.size(); ++i) {
            if (cmpIndexOrder(newDiv, pmerged[i], false) < 0) { at = i; break; }
        }
        pmerged.insert(pmerged.begin() + (ptrdiff_t)at, newDiv);
        std::vector<uint32_t> above(ancPath.begin(), ancPath.begin() + (ptrdiff_t)ai);
        Status st = insertIntoPage(mdb, table, idx, parent, false, pmerged, above);
        if (!st.ok) return st;
        if (at + 1 != pmerged.size()) return Status::Ok(); // not the new max
        curChild = parent;
    }
    return Status::Ok();
}

/* Remove an emptied index page: unlink peers, drop its divider (or clear a
 * tail link), recurse when the parent empties. Jackcess removeDataPage
 * mirror. ancPath is the root-first path to pg (empty for root). */
static Status removeIndexPage(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                              uint32_t pg, std::vector<uint32_t>& ancPath) {
    using namespace Jet4Writer;
    if (!UsageMapManager::readPage(mdb, pg)) return Status::Fail("removeIndexPage: read failed");
    const auto* b0 = static_cast<const uint8_t*>(mdb->pg_buf);
    const bool wasLeaf = (b0[0] == kPageLeaf);
    if (!wasLeaf && b0[0] != kPageIndex) return Status::Fail("removeIndexPage: not an index page");
    const uint32_t oldPrev = UsageMapManager::getU32(mdb, kOffPrevPg);
    const uint32_t oldNext = UsageMapManager::getU32(mdb, kOffNextPg);
    const uint32_t oldTail = UsageMapManager::getU32(mdb, kOffChildTailPg);
    if (!wasLeaf && oldTail != 0)
        return Status::Fail("removeIndexPage: emptied node still has child tail");
    if (wasLeaf && oldTail != 0)
        return Status::Fail("removeIndexPage: emptied leaf has tail link");
    if (ancPath.empty()) {
        // Root: rewrite as an empty leaf (native empty-leaf shape).
        std::vector<uint8_t> page(kPageSize, 0);
        page[0] = kPageLeaf;
        page[1] = 0x01;
        auto put16 = [&](int off, uint16_t v) {
            page[static_cast<size_t>(off)] = static_cast<uint8_t>(v & 0xFF);
            page[static_cast<size_t>(off) + 1] = static_cast<uint8_t>((v >> 8) & 0xFF);
        };
        auto put32 = [&](int off, uint32_t v) {
            for (int i = 0; i < 4; ++i)
                page[static_cast<size_t>(off) + i] = static_cast<uint8_t>((v >> (8 * i)) & 0xFF);
        };
        put16(kOffFreeSpace, static_cast<uint16_t>(kPageSize - kEntryAreaStart));
        put32(kOffTableDefPg, static_cast<uint32_t>(table->entry->table_pg));
        put32(kOffUnknown, 0);
        put32(kOffPrevPg, 0);
        put32(kOffNextPg, 0);
        put32(kOffChildTailPg, 0);
        put16(kOffPrefixLen, 0);
        page[kOffPrefixUnknown] = 0;
        if (!UsageMapManager::readPage(mdb, pg)) return Status::Fail("removeIndexPage: root reread failed");
        std::memcpy(mdb->pg_buf, page.data(), kPageSize);
        if (!UsageMapManager::writePage(mdb, pg)) return Status::Fail("removeIndexPage: root write failed");
        return Status::Ok();
    }
    const uint32_t parent = ancPath.back();
    std::vector<uint32_t> above(ancPath.begin(), ancPath.end() - 1);
    if (!UsageMapManager::readPage(mdb, parent)) return Status::Fail("removeIndexPage: parent read failed");
    {
        const auto* pb = static_cast<const uint8_t*>(mdb->pg_buf);
        if (pb[0] != kPageIndex) return Status::Fail("removeIndexPage: parent not node");
    }
    const uint32_t parentTail = UsageMapManager::getU32(mdb, kOffChildTailPg);
    const bool isTail = (parentTail == pg);
    DecodedPage pdp;
    {
        std::string derr;
        if (!BTreeEngine::decodeEntries(mdb, pdp, derr))
            return Status::Fail("removeIndexPage: parent decode: " + derr);
    }
    // Collect dividers pointing at pg (exactly one expected unless tail).
    std::vector<size_t> hits;
    for (size_t i = 0; i < pdp.full.size(); ++i) {
        if (BTreeEngine::childOf(pdp.full[i]) == pg) hits.push_back(i);
    }
    if (hits.empty() && !isTail)
        return Status::Fail("removeIndexPage: no divider for child");
    std::vector<std::vector<uint8_t>> pmerged;
    pmerged.reserve(pdp.full.size());
    for (size_t i = 0; i < pdp.full.size(); ++i) {
        if (BTreeEngine::childOf(pdp.full[i]) == pg) continue;
        pmerged.push_back(pdp.full[i]);
    }
    if (isTail) {
        // Jackcess updateParentTail(REMOVE): tail link cleared.
        if (!UsageMapManager::readPage(mdb, parent)) return Status::Fail("removeIndexPage: tail reread failed");
        UsageMapManager::putU32(mdb, kOffChildTailPg, 0);
        if (!UsageMapManager::writePage(mdb, parent)) return Status::Fail("removeIndexPage: tail write failed");
    }
    if (pmerged.empty()) {
        // Parent emptied too: recurse upward (peers unlinked below).
        Status st = removeIndexPage(mdb, table, idx, parent, above);
        if (!st.ok) return st;
    } else {
        Status st = BTreeEngine::insertIntoPage(mdb, table, idx, parent, false, pmerged, above);
        if (!st.ok) return st;
    }
    // Unlink from peers (strict: peers must be index pages).
    if (oldPrev != 0) {
        if (!UsageMapManager::readPage(mdb, oldPrev)) return Status::Fail("removeIndexPage: prev read failed");
        const auto* b = static_cast<const uint8_t*>(mdb->pg_buf);
        if (b[0] != kPageLeaf && b[0] != kPageIndex) return Status::Fail("removeIndexPage: prev not index");
        UsageMapManager::putU32(mdb, kOffNextPg, oldNext);
        if (!UsageMapManager::writePage(mdb, oldPrev)) return Status::Fail("removeIndexPage: prev write failed");
    }
    if (oldNext != 0) {
        if (!UsageMapManager::readPage(mdb, oldNext)) return Status::Fail("removeIndexPage: next read failed");
        const auto* b = static_cast<const uint8_t*>(mdb->pg_buf);
        if (b[0] != kPageLeaf && b[0] != kPageIndex) return Status::Fail("removeIndexPage: next not index");
        UsageMapManager::putU32(mdb, kOffPrevPg, oldPrev);
        if (!UsageMapManager::writePage(mdb, oldNext)) return Status::Fail("removeIndexPage: next write failed");
    }
    return Status::Ok();
}

Status BTreeEngine::eraseEntry(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                               const uint8_t* entry, size_t entryLen) {
    if (!mdb || !table || !table->entry || !idx) return Status::Fail("eraseEntry: null arg");
    if (!entry || entryLen < 5 || entryLen > 1500) return Status::Fail("eraseEntry: bad entry");
    if (idx->first_pg == 0) return Status::Fail("eraseEntry: no root");
    std::vector<uint8_t> key(entry, entry + entryLen);
    std::string err;
    std::vector<uint32_t> ancPath;
    const uint32_t leaf = findLeaf(mdb, idx, key.data(), key.size(), ancPath, err);
    if (!leaf) return Status::Fail("eraseEntry: descent failed: " + err);
    if (!UsageMapManager::readPage(mdb, leaf)) return Status::Fail("eraseEntry: leaf reread failed");
    {
        const auto* b = static_cast<const uint8_t*>(mdb->pg_buf);
        if (b[0] != kPageLeaf) return Status::Fail("eraseEntry: target not leaf");
    }
    DecodedPage dp;
    if (!decodeEntries(mdb, dp, err)) return Status::Fail("eraseEntry: decode: " + err);
    size_t at = dp.full.size();
    for (size_t i = 0; i < dp.full.size(); ++i) {
        if (dp.full[i] == key) { at = i; break; }
    }
    if (at >= dp.full.size()) return Status::Fail("eraseEntry: entry not on descent page");
    const std::vector<uint8_t> oldMax = dp.full.back();
    const bool wasMax = (dp.full[at] == oldMax);
    std::vector<std::vector<uint8_t>> merged;
    merged.reserve(dp.full.size() - 1);
    for (size_t i = 0; i < dp.full.size(); ++i) {
        if (i != at) merged.push_back(dp.full[i]);
    }
    if (merged.empty()) {
        Status st = removeIndexPage(mdb, table, idx, leaf, ancPath);
        if (!st.ok) return st;
    } else {
        std::string werr;
        if (!writeLeaf(mdb, leaf, true, merged, werr))
            return Status::Fail("eraseEntry: rewrite failed: " + werr);
        if (wasMax && !ancPath.empty()) {
            Status ps = propagateMaxChange(mdb, table, idx, leaf, merged.back(), ancPath);
            if (!ps.ok) return ps;
        }
    }
    // Keep cardinality coherent (mirror updateIndex ++).
    idx->num_rows--;
    if (table->entry->table_pg > 0) {
        const int off = mdb->fmt->tab_cols_start_offset +
                        idx->index_num * mdb->fmt->tab_ridx_entry_size;
        if (UsageMapManager::readPage(mdb, static_cast<uint32_t>(table->entry->table_pg))) {
            mdb_put_int32(mdb->pg_buf, off, static_cast<guint32>(idx->num_rows));
            UsageMapManager::writePage(mdb, static_cast<uint32_t>(table->entry->table_pg));
        }
    }
    return Status::Ok();
}

Status BTreeEngine::eraseRowRefs(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                                 uint32_t dataPg, uint16_t rowIdx, int& removed) {
    removed = 0;
    if (!mdb || !table || !table->entry || !idx) return Status::Fail("eraseRowRefs: null arg");
    if (idx->first_pg == 0) return Status::Fail("eraseRowRefs: no root");
    // Descend to leftmost leaf, then walk the chain collecting matches.
    uint32_t pg = idx->first_pg;
    for (int d = 0; d < 12; ++d) {
        if (!UsageMapManager::readPage(mdb, pg)) return Status::Fail("eraseRowRefs: read failed");
        const auto* b = static_cast<const uint8_t*>(mdb->pg_buf);
        if (b[0] == kPageLeaf) break;
        if (b[0] != kPageIndex) return Status::Fail("eraseRowRefs: bad node");
        DecodedPage dp;
        std::string err;
        if (!decodeEntries(mdb, dp, err) || dp.full.empty())
            return Status::Fail("eraseRowRefs: node decode: " + err);
        const auto& div = dp.full.front();
        if (div.size() < 4) return Status::Fail("eraseRowRefs: short divider");
        pg = childOf(div);
        if (!pg) return Status::Fail("eraseRowRefs: null child");
        if (d == 11) return Status::Fail("eraseRowRefs: too deep");
    }
    // Phase 1: Collect all matching hits across leaves without modifying tree
    std::vector<std::vector<uint8_t>> allHits;
    for (int hops = 0; hops < 1000000; ++hops) {
        if (!UsageMapManager::readPage(mdb, pg)) return Status::Fail("eraseRowRefs: leaf read failed");
        const auto* b = static_cast<const uint8_t*>(mdb->pg_buf);
        if (b[0] != kPageLeaf) return Status::Fail("eraseRowRefs: chain hit non-leaf");
        const uint32_t nextPg = UsageMapManager::getU32(mdb, kOffNextPg);
        DecodedPage dp;
        std::string err;
        if (!decodeEntries(mdb, dp, err)) return Status::Fail("eraseRowRefs: leaf decode: " + err);
        for (auto& e : dp.full) {
            if (e.size() < 4) continue;
            const size_t t = e.size() - 4;
            const uint32_t epg = (static_cast<uint32_t>(e[t]) << 16) |
                                 (static_cast<uint32_t>(e[t + 1]) << 8) |
                                 static_cast<uint32_t>(e[t + 2]);
            const uint16_t erow = e[t + 3];
            if (epg == dataPg && erow == (rowIdx & 0xFF)) allHits.push_back(e);
        }
        if (!nextPg) break;
        pg = nextPg;
    }
    // Phase 2: Safely erase collected entries
    for (auto& h : allHits) {
        Status st = eraseEntry(mdb, table, idx, h.data(), h.size());
        if (!st.ok) return st;
        ++removed;
    }
    return Status::Ok();
}

Status BTreeEngine::leafInsert(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                               const uint8_t* entry, size_t entryLen) {
    if (!mdb || !table || !table->entry || !idx) return Status::Fail("leafInsert: null arg");
    if (!entry || entryLen < 5 || entryLen > 1500) return Status::Fail("leafInsert: bad entry");
    if (idx->first_pg == 0) return Status::Fail("leafInsert: no root");

    std::vector<uint8_t> key(entry, entry + entryLen);
    std::string err;
    std::vector<uint32_t> ancPath;
    const uint32_t leaf = findLeaf(mdb, idx, key.data(), key.size(), ancPath, err);
    if (!leaf) return Status::Fail("leafInsert: descent failed: " + err);

    if (!UsageMapManager::readPage(mdb, leaf)) return Status::Fail("leafInsert: leaf reread failed");
    {
        const auto* b = static_cast<const uint8_t*>(mdb->pg_buf);
        if (b[0] != kPageLeaf) return Status::Fail("leafInsert: target not leaf");
    }
    DecodedPage dp;
    if (!decodeEntries(mdb, dp, err)) return Status::Fail("leafInsert: decode: " + err);

    /* Sorted insert position (Jackcess full order: key bytes, then rowId).
     * Duplicates allowed (Jet) but rowId-ordered, so the positional back
     * stays the true max that dividers replicate. */
    std::vector<uint8_t> fullNew(key.begin(), key.end());
    size_t at = dp.full.size();
    for (size_t i = 0; i < dp.full.size(); ++i) {
        if (cmpIndexOrder(fullNew, dp.full[i], true) < 0) { at = i; break; }
    }
    std::vector<std::vector<uint8_t>> merged;
    merged.reserve(dp.full.size() + 1);
    merged.insert(merged.end(), dp.full.begin(), dp.full.begin() + static_cast<ptrdiff_t>(at));
    merged.push_back(std::move(fullNew));
    merged.insert(merged.end(), dp.full.begin() + static_cast<ptrdiff_t>(at), dp.full.end());

    /* Rewrite in place, splitting with parent propagation when full. */
    return insertIntoPage(mdb, table, idx, leaf, true, merged, ancPath);
}

Status BTreeEngine::walkIndex(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                             std::vector<IndexEntryInfo>& out) {
    out.clear();
    (void)table;
    if (!mdb || !idx) return Status::Fail("walkIndex: null arg");
    if (idx->first_pg == 0) return Status::Fail("walkIndex: no root");
    /* Descend to leftmost leaf. */
    uint32_t pg = idx->first_pg;
    for (int d = 0; d < 12; ++d) {
        if (!UsageMapManager::readPage(mdb, pg)) return Status::Fail("walkIndex: read failed");
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
        if (!UsageMapManager::readPage(mdb, pg)) return Status::Fail("walkIndex: leaf read failed");
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
        const uint32_t nxt = UsageMapManager::getU32(mdb, kOffNextPg);
        if (!nxt || nxt == pg) break;
        pg = nxt;
    }
    return Status::Ok();
}

} /* namespace Jet4Writer */
