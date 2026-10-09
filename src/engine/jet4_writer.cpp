#include "jet4_writer.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
extern "C" {
int mdb_find_row(MdbHandle* mdb, int row_number, int* row_start, size_t* row_size);
}

static Jet4Writer::Field toJetField(const MdbField& m);

namespace Jet4Writer {

/* ============================ packRow ============================ */
Status packRow(MdbTableDef* table, const Field* fields, size_t nfields,
               std::vector<uint8_t>& out) {
    if (!table || !table->columns) return Status::Fail("packRow: null table");
    const unsigned numCols = table->num_cols;
    if (numCols == 0 || numCols > 1000) return Status::Fail("packRow: bad num_cols");

    int maxFixed = 0;
    // Jackcess TableImpl/NullMask: the row's column count AND nullmask span
    // _maxColumnCount (live + deleted), with bits indexed by the physical
    // col_num (deleted numbers are gaps, left null). Sizing by num_cols
    // breaks tables with deleted columns (CompanyInfo: 37 live, col_num to
    // 52) with col_num OOB, and writes short masks/colcounts that strict
    // readers (Jackcess/UCanAccess) misparse.
    unsigned maxCol = numCols;
    for (unsigned i = 0; i < numCols; ++i) {
        const MdbColumn* c = ColumnEncoder::getColumn(table, static_cast<int>(i));
        if (!c) return Status::Fail("packRow: missing column def");
        if (c->col_num >= 0)
            maxCol = std::max(maxCol, static_cast<unsigned>(c->col_num) + 1);
        if (c->is_fixed) {
            if (c->fixed_offset < 0 || c->col_size <= 0 || c->col_size > 512)
                return Status::Fail("packRow: bad fixed geometry");
            maxFixed = std::max(maxFixed, c->fixed_offset + c->col_size);
        }
    }
    if (maxCol == 0 || maxCol > 1000) return Status::Fail("packRow: bad max column count");

    std::vector<uint8_t> body;
    body.reserve(1024);
    body.push_back(static_cast<uint8_t>(maxCol & 0xFF));
    body.push_back(static_cast<uint8_t>((maxCol >> 8) & 0xFF));
    const size_t fixedStart = body.size();
    body.insert(body.end(), static_cast<size_t>(maxFixed), 0);

    for (size_t i = 0; i < nfields; ++i) {
        const Field& f = fields[i];
        const MdbColumn* c = ColumnEncoder::getColumn(table, f.colnum);
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
        const MdbColumn* c = ColumnEncoder::getColumn(table, f.colnum);
        if (!c || c->is_fixed) continue;
        if (static_cast<unsigned>(c->var_col_num) < numVar && !byVar[static_cast<size_t>(c->var_col_num)])
            byVar[static_cast<size_t>(c->var_col_num)] = &f;
    }
    if (numVar > 0) {
        std::vector<uint32_t> varOff(numVar + 1, 0);
        for (unsigned v = 0; v < numVar; ++v) {
            varOff[v] = static_cast<uint32_t>(body.size());
            const Field* f = byVar[v];
            if (f && !f->isNull && f->data && f->size > 0) {
                if (body.size() + f->size > 4000) return Status::Fail("packRow: row too big");
                body.insert(body.end(), f->data, f->data + f->size);
            }
        }
        varOff[numVar] = static_cast<uint32_t>(body.size());
        for (uint32_t o : varOff) {
            if (o > 0xFFFF) return Status::Fail("packRow: var offset overflow");
        }
        const uint16_t eod = static_cast<uint16_t>(body.size());
        body.push_back(static_cast<uint8_t>(eod & 0xFF));
        body.push_back(static_cast<uint8_t>((eod >> 8) & 0xFF));
        for (int v = static_cast<int>(numVar) - 1; v >= 0; --v) {
            const uint16_t o = static_cast<uint16_t>(varOff[static_cast<size_t>(v)]);
            body.push_back(static_cast<uint8_t>(o & 0xFF));
            body.push_back(static_cast<uint8_t>((o >> 8) & 0xFF));
        }
        body.push_back(static_cast<uint8_t>(numVar & 0xFF));
        body.push_back(static_cast<uint8_t>((numVar >> 8) & 0xFF));
    }

    // Null mask: bits indexed by col->col_num (1 = NOT NULL), zero-initialised.
    // Columns absent from `fields` stay NULL. Sized by maxCol (Jackcess
    // _maxColumnCount), so deleted-column gaps stay addressable. Matches
    // mdb_crack_row() logic.
    const size_t maskBytes = (maxCol + 7) / 8;
    std::vector<uint8_t> nullMask(maskBytes, 0);
    for (size_t i = 0; i < nfields; ++i) {
        const Field& f = fields[i];
        if (f.isNull) continue;
        const MdbColumn* c = ColumnEncoder::getColumn(table, f.colnum);
        if (!c) continue;
        const int cn = c->col_num;
        if (cn < 0 || static_cast<size_t>(cn / 8) >= maskBytes)
            return Status::Fail("packRow: col_num OOB");
        nullMask[static_cast<size_t>(cn / 8)] |= static_cast<uint8_t>(1u << (cn % 8));
    }
    body.insert(body.end(), nullMask.begin(), nullMask.end());
    if (body.size() > 4000) return Status::Fail("packRow: row too big");
    out = std::move(body);
    return Status::Ok();
}

/* ============================ updateIndex ============================ */
int updateIndex(MdbTableDef* table, MdbIndex* idx,
                const Field* fields, size_t nfields,
                uint32_t dataPg, uint16_t rownum, std::string& error) {
    if (!table || !table->entry || !table->entry->mdb || !idx) {
        error = "updateIndex: null arg"; return 0;
    }
    if (idx->first_pg == 0 || idx->index_type == 2) {
        if (std::getenv("JET4_DEBUG")) fprintf(stderr, "[jet4] updateIndex skip %s first_pg=%u type=%d\n",
                                               idx->name, idx->first_pg, (int)idx->index_type);
        return 0;
    }
    MdbHandle* mdb = table->entry->mdb;
    std::vector<uint8_t> entry;
    Status st = BTreeEngine::buildEntry(table, idx, fields, nfields, dataPg, rownum, entry);
    if (!st.ok) {
        error = st.error;
        if (std::getenv("JET4_DEBUG")) fprintf(stderr, "[jet4] updateIndex %s build failed: %s\n", idx->name, st.error.c_str());
        return 0;
    }
    st = BTreeEngine::leafInsert(mdb, table, idx, entry.data(), entry.size());
    if (!st.ok) {
        error = st.error;
        if (std::getenv("JET4_DEBUG")) fprintf(stderr, "[jet4] updateIndex %s insert failed: %s\n", idx->name, st.error.c_str());
        return 0;
    }
    idx->num_rows++;
    /* Persist index cardinality on the table definition page (legacy behavior). */
    if (table->entry->table_pg > 0) {
        const int off = mdb->fmt->tab_cols_start_offset +
                        idx->index_num * mdb->fmt->tab_ridx_entry_size;
        if (UsageMapManager::readPage(mdb, static_cast<uint32_t>(table->entry->table_pg))) {
            mdb_put_int32(mdb->pg_buf, off, static_cast<guint32>(idx->num_rows));
            UsageMapManager::writePage(mdb, static_cast<uint32_t>(table->entry->table_pg));
        }
    }
    return static_cast<int>(entry.size());
}

/* ============================ verifyDatabase ============================ */
VerifyReport verifyDatabase(MdbHandle* mdb) {
    VerifyReport rep;
    if (!mdb) { rep.error = "null handle"; return rep; }
    const char* tables[] = {"Transactions", "Ledgers", "Groups", "CompanyInfo",
                              "StockTransactions", "SaleTransportationDetail", nullptr};
    for (int ti = 0; tables[ti]; ++ti) {
        MdbTableDef* t = mdb_read_table_by_name(mdb, const_cast<char*>(tables[ti]), MDB_TABLE);
        if (!t) { rep.error = std::string("missing table ") + tables[ti]; return rep; }
        mdb_read_columns(t);
        mdb_read_indices(t);
        ++rep.tablesChecked;

        // Deterministic brute-force enumeration (never usage maps, never
        // fetch cursors: map-driven scans miss pages after map churn).
        // Live rows = cracked && no 0xC000 Tombstone/deleted/overflow bits.
        // Flagged slots (seed-deleted, tombstones, overflow pointers) are
        // native warts: reported via badRows only when truly uncrackable
        // AND unflagged; flagged-but-cracked rows are simply not indexed.
        std::vector<RowSnapshot> snap;
        Status sst = TableSnapshot::snapshot(mdb, t, snap);
        if (!sst.ok) {
            mdb_free_tabledef(t);
            rep.error = std::string("snapshot failed ") + tables[ti] + ": " + sst.error;
            return rep;
        }
        long cracked = 0;
        std::set<uint32_t> liveIds;
        for (auto& r : snap) {
            if (r.flags & 0xC000) continue; // deleted row / overflow slot: not a live row
            if (!r.cracked) { ++rep.badRows; continue; }
            ++cracked;
            liveIds.insert((static_cast<uint32_t>(r.pg) << 8) |
                           (static_cast<uint32_t>(r.slot) & 0xFF));
        }
        rep.dataRows += cracked;

        if (t->indices) {
            for (unsigned ii = 0; ii < t->indices->len; ++ii) {
                MdbIndex* idx = static_cast<MdbIndex*>(g_ptr_array_index(t->indices, ii));
                VerifyIndexInfo vi;
                vi.table = tables[ti];
                vi.name = idx->name[0] ? idx->name : "?";
                vi.catalogRows = idx->num_rows;
                vi.dataRows = cracked;
                std::vector<IndexEntryInfo> entries;
                Status st = BTreeEngine::walkIndex(mdb, t, idx, entries);
                if (!st.ok) {
                    mdb_free_tabledef(t);
                    rep.error = std::string("walk failed ") + vi.table + "." + vi.name + ": " + st.error;
                    return rep;
                }
                vi.walked = static_cast<long>(entries.size());
                vi.sorted = true;
                // RowId accounting (Jackcess total order: key bytes, then
                // rowId): every LIVE row needs exactly one entry. Entries
                // pointing at non-live slots (seed overflow pointers,
                // tombstones, dead pages) are native warts: excluded from
                // walked, never fail (Bahi-Khata/ODBC/Jackcess tolerate
                // them; restraint is policy). Missing or duplicated live
                // rows are OUR corruption: missing shows as walked !=
                // dataRows; duplicates force sorted=false (same rowId twice
                // is never valid, however ordered).
                {
                    std::set<uint32_t> seen;
                    long dups = 0;
                    for (auto& e : entries) {
                        const uint32_t k = (e.dataPg << 8) | (e.rowIdx & 0xFF);
                        if (liveIds.find(k) == liveIds.end()) continue; // wart
                        if (!seen.insert(k).second) ++dups;
                    }
                    vi.walked = static_cast<long>(seen.size());
                    if (dups > 0) {
                        vi.sorted = false;
                        fprintf(stderr, "[DEBUG-SORT] %s.%s dups=%ld\n", vi.table.c_str(), vi.name.c_str(), dups);
                    }
                }
                if (vi.sorted) for (size_t i = 1; i < entries.size(); ++i) {
                    const auto& A = entries[i - 1];
                    const auto& B = entries[i];
                    const size_t m = std::min(A.fullKey.size(), B.fullKey.size());
                    int c = m ? std::memcmp(A.fullKey.data(), B.fullKey.data(), m) : 0;
                    if (c == 0) c = (A.fullKey.size() < B.fullKey.size()) ? -1 : (A.fullKey.size() > B.fullKey.size() ? 1 : 0);
                    // Jackcess Entry.compareTo: rowId breaks key ties.
                    if (c == 0 && A.dataPg != B.dataPg) c = (A.dataPg < B.dataPg) ? -1 : 1;
                    if (c == 0 && A.rowIdx != B.rowIdx) c = (A.rowIdx < B.rowIdx) ? -1 : 1;
                    if (c > 0) {
                        vi.sorted = false;
                        fprintf(stderr, "[DEBUG-SORT] %s.%s out of order at %zu: c=%d\n", vi.table.c_str(), vi.name.c_str(), i, c);
                        fprintf(stderr, "   A (leafPg=%u, dataPg=%u, slot=%u): ", A.leafPg, A.dataPg, A.rowIdx);
                        for (uint8_t x : A.fullKey) fprintf(stderr, "%02X ", x);
                        fprintf(stderr, "\n   B (leafPg=%u, dataPg=%u, slot=%u): ", B.leafPg, B.dataPg, B.rowIdx);
                        for (uint8_t x : B.fullKey) fprintf(stderr, "%02X ", x);
                        fprintf(stderr, "\n");
                        break;
                    }
                }
                rep.indexes.push_back(std::move(vi));
            }
        }
        mdb_free_tabledef(t);
    }
    rep.ok = (rep.badRows == 0);
    for (const auto& vi : rep.indexes) {
        if (!vi.sorted || vi.walked != vi.dataRows) { rep.ok = false; break; }
    }
    return rep;
}

} /* namespace Jet4Writer */

namespace Jet4Writer {

/* Canonical data-page rebuild replacing one slot's bytes: same slots,
 * order (by current offset, descending from page end) and flag bits, exact
 * free-space recompute. Fails cleanly when the new row cannot fit. */
static Status rebuildDataPageRow(MdbHandle* mdb, MdbTableDef* table, uint32_t dataPg,
                                 int slot, const std::vector<uint8_t>& newRow) {
    if (!mdb || !table || !table->entry) return Status::Fail("rebuildDataPageRow: null arg");
    if (newRow.empty() || newRow.size() > 4000) return Status::Fail("rebuildDataPageRow: bad row");
    if (!UsageMapManager::readPage(mdb, dataPg)) return Status::Fail("rebuildDataPageRow: read failed");
    auto* buf = static_cast<uint8_t*>(mdb->pg_buf);
    if (buf[0] != kPageData) return Status::Fail("rebuildDataPageRow: not a data page");
    const int rco = mdb->fmt->row_count_offset;
    const int nrows = UsageMapManager::getU16(mdb, rco);
    if (slot < 0 || slot >= nrows || nrows > 1000) return Status::Fail("rebuildDataPageRow: bad slot");
    const int hdrEnd = rco + 2 + nrows * 2;
    struct Span { int off; int flags; std::vector<uint8_t> bytes; int slot; };
    std::vector<Span> spans;
    spans.reserve((size_t)nrows);
    for (int i = 0; i < nrows; ++i) {
        const int raw = UsageMapManager::getU16(mdb, rco + 2 + i * 2);
        const int off = raw & 0x1FFF; /* Jackcess TableImpl.OFFSET_MASK */
        const int flags = raw & ~0x1FFF;
        if (off < hdrEnd || off > kPageSize) return Status::Fail("rebuildDataPageRow: bad offset");
        if (i == slot) {
            spans.push_back({off, flags, newRow, i});
        } else {
            const int e = (i == 0) ? kPageSize
                          : (UsageMapManager::getU16(mdb, rco + 2 + (i - 1) * 2) & 0x1FFF);
            // Row above (lower slot index) ends where this one starts... rows
            // pack downward: slot i occupies [off, end of next-higher start).
            // Compute end as the smallest start strictly greater than off.
            int end = kPageSize;
            for (int j = 0; j < nrows; ++j) {
                const int oj = UsageMapManager::getU16(mdb, rco + 2 + j * 2) & 0x1FFF;
                if (oj > off && oj < end) end = oj;
            }
            if (end < off || end > kPageSize) return Status::Fail("rebuildDataPageRow: bad span");
            spans.push_back({off, flags, std::vector<uint8_t>(buf + off, buf + end), i});
        }
    }
    // Fit check against current free space.
    const int freeNow = UsageMapManager::getU16(mdb, kOffFreeSpace);
    int oldSize = 0;
    {
        int o = -1;
        for (auto& s : spans) if (s.slot == slot) o = s.off;
        int e = kPageSize;
        for (int i = 0; i < nrows; ++i) {
            const int oj = UsageMapManager::getU16(mdb, rco + 2 + i * 2) & 0x1FFF;
            if (oj > o && oj < e) e = oj;
        }
        oldSize = e - o;
    }
    if ((int)newRow.size() > oldSize + freeNow)
        return Status::Fail("rebuildDataPageRow: no room");
    // Lay out from page end in descending-offset order (stable).
    std::sort(spans.begin(), spans.end(),
              [](const Span& a, const Span& b) { return a.off > b.off; });
    std::vector<uint8_t> page(buf, buf + kPageSize);
    int pos = kPageSize;
    for (auto& s : spans) {
        pos -= (int)s.bytes.size();
        if (pos < hdrEnd) return Status::Fail("rebuildDataPageRow: overflow");
        std::memcpy(page.data() + pos, s.bytes.data(), s.bytes.size());
        const int noff = pos | s.flags;
        if (noff > 0xFFFF) return Status::Fail("rebuildDataPageRow: offset overflow");
        page[rco + 2 + s.slot * 2] = static_cast<uint8_t>(noff & 0xFF);
        page[rco + 2 + s.slot * 2 + 1] = static_cast<uint8_t>((noff >> 8) & 0xFF);
        s.off = pos;
    }
    int minStart = kPageSize;
    for (auto& s : spans) minStart = std::min(minStart, s.off);
    const int newFree = minStart - hdrEnd;
    if (newFree < 0 || newFree > kPageSize) return Status::Fail("rebuildDataPageRow: bad free");
    page[kOffFreeSpace] = static_cast<uint8_t>(newFree & 0xFF);
    page[kOffFreeSpace + 1] = static_cast<uint8_t>((newFree >> 8) & 0xFF);
    if (!UsageMapManager::readPage(mdb, dataPg)) return Status::Fail("rebuildDataPageRow: reread failed");
    std::memcpy(mdb->pg_buf, page.data(), kPageSize);
    if (!UsageMapManager::writePage(mdb, dataPg)) return Status::Fail("rebuildDataPageRow: write failed");
    return Status::Ok();
}

/* Jackcess TableImpl.updateRow overflow path: when a row outgrows its original
 * data page, write the new row bytes to a page with free space marked with 0x8000
 * (deleted / overflow target so normal scans skip it), and update the original
 * slot on dataPg with 0x4000 (overflow pointer) containing:
 *   [targetSlot (1B), targetPg (3B little-endian)].
 * Index entries continue pointing to (dataPg, slot), requiring 0 index changes. */
static Status handleRowOverflow(MdbHandle* mdb, MdbTableDef* table, uint32_t dataPg,
                                int slot, const std::vector<uint8_t>& newRow) {
    if (!mdb || !table || !table->entry || newRow.empty())
        return Status::Fail("handleRowOverflow: null arg");

    // 1. Find a page with room for newRow.
    std::vector<uint32_t> owned;
    UsageMapManager::getOwnedPages(mdb, table, owned);
    uint32_t targetPg = 0;
    const uint32_t tdefPg = static_cast<uint32_t>(table->entry->table_pg);
    const int rco = mdb->fmt->row_count_offset;
    for (auto it = owned.rbegin(); it != owned.rend(); ++it) {
        uint32_t p = *it;
        if (p == dataPg) continue; // original page already full
        if (!UsageMapManager::readPage(mdb, p)) continue;
        const auto* b = static_cast<const uint8_t*>(mdb->pg_buf);
        if (b[0] != kPageData) continue;
        if (UsageMapManager::getU32(mdb, kOffTableDefPg) != tdefPg) continue;
        const uint16_t nrows = UsageMapManager::getU16(mdb, rco);
        if (nrows >= 1000) continue;
        int minStart = kPageSize;
        for (int s = 0; s < nrows; ++s) {
            int off = UsageMapManager::getU16(mdb, rco + 2 + s * 2) & 0x1FFF;
            if (off > 0 && off < minStart) minStart = off;
        }
        const int hdrEnd = rco + 2 + (nrows + 1) * 2;
        if (minStart - hdrEnd >= static_cast<int>(newRow.size())) {
            targetPg = p;
            break;
        }
    }
    if (targetPg == 0) {
        Status ast = UsageMapManager::allocateDataPage(mdb, table, targetPg);
        if (!ast.ok || targetPg == 0)
            return Status::Fail("handleRowOverflow: allocate failed: " + ast.error);
    }

    // 2. Write new row onto targetPg marked 0x8000
    if (!UsageMapManager::readPage(mdb, targetPg))
        return Status::Fail("handleRowOverflow: read target failed");
    auto* tb = static_cast<uint8_t*>(mdb->pg_buf);
    uint16_t tNrows = UsageMapManager::getU16(mdb, rco);
    int tMinStart = kPageSize;
    for (int s = 0; s < tNrows; ++s) {
        int off = UsageMapManager::getU16(mdb, rco + 2 + s * 2) & 0x1FFF;
        if (off > 0 && off < tMinStart) tMinStart = off;
    }
    int tPos = tMinStart - static_cast<int>(newRow.size());
    const int tHdrEnd = rco + 2 + (tNrows + 1) * 2;
    if (tPos < tHdrEnd)
        return Status::Fail("handleRowOverflow: target overflow");

    std::memcpy(tb + tPos, newRow.data(), newRow.size());
    // Write slot offset with 0x8000 DELETED / OVERFLOW_TARGET flag
    UsageMapManager::putU16(mdb, rco + 2 + tNrows * 2, static_cast<uint16_t>(tPos | 0x8000));
    const int newTargetSlot = tNrows;
    tNrows++;
    UsageMapManager::putU16(mdb, rco, tNrows);
    const uint16_t tFree = static_cast<uint16_t>(tPos - (rco + 2 + tNrows * 2));
    UsageMapManager::putU16(mdb, 2, tFree);
    if (!UsageMapManager::writePage(mdb, targetPg))
        return Status::Fail("handleRowOverflow: write target failed");

    // 3. Update original header slot on dataPg: mark 0x4000 and write pointer
    if (!UsageMapManager::readPage(mdb, dataPg))
        return Status::Fail("handleRowOverflow: read dataPg failed");
    auto* db = static_cast<uint8_t*>(mdb->pg_buf);
    const int origRaw = UsageMapManager::getU16(mdb, rco + 2 + slot * 2);
    const int origOff = origRaw & 0x1FFF;
    UsageMapManager::putU16(mdb, rco + 2 + slot * 2, static_cast<uint16_t>(origOff | 0x4000));
    db[origOff] = static_cast<uint8_t>(newTargetSlot & 0xFF);
    db[origOff + 1] = static_cast<uint8_t>(targetPg & 0xFF);
    db[origOff + 2] = static_cast<uint8_t>((targetPg >> 8) & 0xFF);
    db[origOff + 3] = static_cast<uint8_t>((targetPg >> 16) & 0xFF);
    if (!UsageMapManager::writePage(mdb, dataPg))
        return Status::Fail("handleRowOverflow: write dataPg failed");

    return Status::Ok();
}

int reindexRowData(MdbTableDef* table, const Field* newFields, size_t nfields,
                   uint32_t dataPg, uint16_t rownum, std::string& error) {
    if (!table || !table->entry || !table->entry->mdb || !newFields) {
        error = "reindexRowData: null arg"; return 0;
    }
    if (!table->indices) {
        error = "reindexRowData: no indices"; return 0;
    }
    MdbHandle* mdb = table->entry->mdb;
    const int slot = (int)rownum - 1;
    // Materialize ALL caller field bytes up front: cracked-field pointers
    // alias pg_buf, which erase/insert page I/O below legitimately clobbers.
    // Packing from dangling pointers writes well-formed garbage rows.
    // Empty-but-not-null fields (data==nullptr/size==0, e.g. "") carry no
    // bytes, so there is nothing to copy AND nothing dangling: the
    // null-vs-empty distinction must survive (packRow's nullmask and the
    // index null rule both key off isNull alone; collapsing "" to NULL
    // flips stored rows and desyncs index keys from row images).
    std::vector<std::vector<uint8_t>> owned;
    owned.reserve(nfields);
    std::vector<Field> nf;
    nf.reserve(nfields);
    for (size_t i = 0; i < nfields; ++i) {
        Field f = newFields[i];
        if (!f.isNull && f.data && f.size > 0) {
            owned.emplace_back(f.data, f.data + f.size);
            f.data = owned.back().data();
        } else if (f.size == 0) {
            f.data = nullptr; // no bytes: null the pointer, keep isNull
        }
        nf.push_back(f);
    }
    const Field* NFF = nf.data();
    const size_t NFFN = nf.size();
    // 1. Crack the old row image.
    if (!UsageMapManager::readPage(mdb, dataPg)) { error = "reindexRowData: page read failed"; return 0; }
    {
        const auto* b = static_cast<const uint8_t*>(mdb->pg_buf);
        if (b[0] != kPageData) { error = "reindexRowData: not a data page"; return 0; }
    }
    int rs = 0;
    size_t rsz = 0;
    if (mdb_find_row(mdb, slot, &rs, &rsz) != 0 || rsz == 0) {
        error = "reindexRowData: slot not found"; return 0;
    }
    if (rs & 0x4000) { error = "reindexRowData: overflow slot"; return 0; }
    if (rs & 0xC000) { error = "reindexRowData: deleted slot"; return 0; }
    std::vector<MdbField> fb(table->num_cols ? table->num_cols : 1);
    if (mdb_crack_row(table, rs & 0x1FFF, rsz, fb.data()) < 0) {
        error = "reindexRowData: crack failed"; return 0;
    }
    std::vector<Field> oldFs;
    oldFs.reserve(fb.size());
    for (auto& m : fb) oldFs.push_back(toJetField(m));
    // 2. Build old/new entries per index; erase entries whose keys change.
    struct Pending { MdbIndex* idx; std::vector<uint8_t> newE; };
    std::vector<Pending> pend;
    for (unsigned ii = 0; ii < table->indices->len; ++ii) {
        MdbIndex* idx = static_cast<MdbIndex*>(g_ptr_array_index(table->indices, ii));
        if (!idx || idx->first_pg == 0 || idx->index_type == 2) continue;
        std::vector<uint8_t> oldE, newE;
        Status a = BTreeEngine::buildEntry(table, idx, oldFs.data(), oldFs.size(), dataPg, rownum, oldE);
        if (!a.ok) { error = "reindexRowData: old build: " + a.error; return 0; }
        Status b = BTreeEngine::buildEntry(table, idx, NFF, NFFN, dataPg, rownum, newE);
        if (!b.ok) { error = "reindexRowData: new build: " + b.error; return 0; }
        if (oldE == newE) continue; // key unchanged: nothing to do
        int removed = 0;
        Status st = BTreeEngine::eraseRowRefs(mdb, table, idx, dataPg,
                                              static_cast<uint16_t>(slot), removed);
        if (!st.ok) { error = "reindexRowData: eraseRowRefs: " + st.error; return 0; }
        pend.push_back({idx, std::move(newE)});
    }
    // 3. Pack + rebuild the data page (skip only on exact byte no-op).
    std::vector<uint8_t> packed;
    Status ps = packRow(table, NFF, NFFN, packed);
    if (!ps.ok) { error = "reindexRowData: pack: " + ps.error; return 0; }
    {
        if (!UsageMapManager::readPage(mdb, dataPg)) { error = "reindexRowData: reread failed"; return 0; }
        int rs2 = 0;
        size_t rsz2 = 0;
        if (mdb_find_row(mdb, slot, &rs2, &rsz2) != 0) { error = "reindexRowData: slot gone"; return 0; }
        const auto* b2 = static_cast<const uint8_t*>(mdb->pg_buf);
        bool same = (rsz2 == packed.size() &&
                     std::memcmp(b2 + (rs2 & 0x1FFF), packed.data(), packed.size()) == 0);
        if (same && pend.empty()) return 1; // true no-op
        if (!same) {
            Status rs2st = rebuildDataPageRow(mdb, table, dataPg, slot, packed);
            if (!rs2st.ok) {
                Status ovst = handleRowOverflow(mdb, table, dataPg, slot, packed);
                if (!ovst.ok) { error = "reindexRowData: overflow: " + ovst.error; return 0; }
            }
        }
    }
    // 4. Insert new entries (+1 count each to balance erase's -1).
    for (auto& p : pend) {
        Status st = BTreeEngine::leafInsert(mdb, table, p.idx, p.newE.data(), p.newE.size());
        if (!st.ok) { error = "reindexRowData: insert: " + st.error; return 0; }
        p.idx->num_rows++;
        if (table->entry->table_pg > 0) {
            const int off = mdb->fmt->tab_cols_start_offset +
                            p.idx->index_num * mdb->fmt->tab_ridx_entry_size;
            if (UsageMapManager::readPage(mdb, static_cast<uint32_t>(table->entry->table_pg))) {
                mdb_put_int32(mdb->pg_buf, off, static_cast<guint32>(p.idx->num_rows));
                UsageMapManager::writePage(mdb, static_cast<uint32_t>(table->entry->table_pg));
            }
        }
    }
    return 1;
}

int replaceRowData(MdbTableDef* table, const Field* newFields, size_t nfields,
                   uint32_t dataPg, uint16_t rownum, std::string& error) {
    if (!table || !table->entry || !table->entry->mdb || !newFields) {
        error = "replaceRowData: null arg"; return 0;
    }
    MdbHandle* mdb = table->entry->mdb;
    const int slot = (int)rownum - 1;
    // Owned copies (caller bytes may alias pg_buf; empty stays empty).
    std::vector<std::vector<uint8_t>> owned;
    owned.reserve(nfields);
    std::vector<Field> nf;
    nf.reserve(nfields);
    for (size_t i = 0; i < nfields; ++i) {
        Field f = newFields[i];
        if (!f.isNull && f.data && f.size > 0) {
            owned.emplace_back(f.data, f.data + f.size);
            f.data = owned.back().data();
        } else if (f.size == 0) {
            f.data = nullptr;
        }
        nf.push_back(f);
    }
    std::vector<uint8_t> packed;
    Status ps = packRow(table, nf.data(), nf.size(), packed);
    if (!ps.ok) { error = "replaceRowData: pack: " + ps.error; return 0; }
    if (!UsageMapManager::readPage(mdb, dataPg)) {
        error = "replaceRowData: reread failed"; return 0;
    }
    int rs2 = 0;
    size_t rsz2 = 0;
    if (mdb_find_row(mdb, slot, &rs2, &rsz2) != 0) {
        error = "replaceRowData: slot gone"; return 0;
    }
    if (rs2 & 0xC000) { error = "replaceRowData: flagged slot"; return 0; }
    const auto* b2 = static_cast<const uint8_t*>(mdb->pg_buf);
    if (rsz2 == packed.size() &&
        std::memcmp(b2 + (rs2 & 0x1FFF), packed.data(), packed.size()) == 0)
        return 1; // byte-identical no-op
    Status rst = rebuildDataPageRow(mdb, table, dataPg, slot, packed);
    if (!rst.ok) {
        Status ovst = handleRowOverflow(mdb, table, dataPg, slot, packed);
        if (!ovst.ok) {
            error = "replaceRowData: " + rst.error + " | overflow: " + ovst.error;
            return 0;
        }
    }
    return 1;
}

bool resurrectDeletedSlot(MdbHandle* mdb, MdbTableDef* table,
                          uint32_t dataPg, int slot, std::string& error) {
    if (!mdb || !table || !table->entry || !mdb->fmt) {
        error = "resurrectDeletedSlot: null arg"; return false;
    }
    if (!UsageMapManager::readPage(mdb, dataPg)) {
        error = "resurrectDeletedSlot: page read failed"; return false;
    }
    const auto* b = static_cast<const uint8_t*>(mdb->pg_buf);
    if (b[0] != kPageData) { error = "resurrectDeletedSlot: not a data page"; return false; }
    const int rco = mdb->fmt->row_count_offset;
    const int nrows = UsageMapManager::getU16(mdb, rco);
    if (slot < 0 || slot >= nrows) { error = "resurrectDeletedSlot: bad slot"; return false; }
    const int loc = rco + 2 + slot * 2;
    const int raw = UsageMapManager::getU16(mdb, loc);
    if (raw & 0x4000) { error = "resurrectDeletedSlot: overflow fragment"; return false; }
    if (!(raw & 0x8000)) return true; // already live
    if (!UsageMapManager::readPage(mdb, dataPg)) {
        error = "resurrectDeletedSlot: reread failed"; return false;
    }
    auto* wb = static_cast<uint8_t*>(mdb->pg_buf);
    const int raw2 = UsageMapManager::getU16(mdb, loc);
    const uint16_t nv = static_cast<uint16_t>(raw2 & ~0x8000);
    wb[loc] = static_cast<uint8_t>(nv & 0xFF);
    wb[loc + 1] = static_cast<uint8_t>((nv >> 8) & 0xFF);
    if (!UsageMapManager::writePage(mdb, dataPg)) {
        error = "resurrectDeletedSlot: write failed"; return false;
    }
    return true;
}

int ensureRowIndexed(MdbTableDef* table, const Field* fields, size_t nfields,
                     uint32_t dataPg, uint16_t rownum, std::string& error) {
    if (!table || !table->entry || !table->entry->mdb || !fields) {
        error = "ensureRowIndexed: null arg"; return 0;
    }
    if (!table->indices) { error = "ensureRowIndexed: no indices"; return 0; }
    MdbHandle* mdb = table->entry->mdb;
    const int slot = (int)rownum - 1;
    int ensured = 0;
    for (unsigned ii = 0; ii < table->indices->len; ++ii) {
        MdbIndex* idx = static_cast<MdbIndex*>(g_ptr_array_index(table->indices, ii));
        if (!idx || idx->first_pg == 0 || idx->index_type == 2) continue;
        std::vector<uint8_t> e;
        Status b = BTreeEngine::buildEntry(table, idx, fields, nfields, dataPg, rownum, e);
        if (!b.ok) { error = "ensureRowIndexed: build: " + b.error; return 0; }
        int removed = 0;
        Status st = BTreeEngine::eraseRowRefs(mdb, table, idx, dataPg,
                                              static_cast<uint16_t>(slot), removed);
        if (!st.ok) { error = "ensureRowIndexed: erase: " + st.error; return 0; }
        st = BTreeEngine::leafInsert(mdb, table, idx, e.data(), e.size());
        if (!st.ok) { error = "ensureRowIndexed: insert: " + st.error; return 0; }
        idx->num_rows++;
        if (table->entry->table_pg > 0) {
            const int off = mdb->fmt->tab_cols_start_offset +
                            idx->index_num * mdb->fmt->tab_ridx_entry_size;
            if (UsageMapManager::readPage(mdb, static_cast<uint32_t>(table->entry->table_pg))) {
                mdb_put_int32(mdb->pg_buf, off, static_cast<guint32>(idx->num_rows));
                UsageMapManager::writePage(mdb, static_cast<uint32_t>(table->entry->table_pg));
            }
        }
        ++ensured;
    }
    return ensured;
}

void finalizeTable(MdbHandle* mdb, MdbTableDef* table) {
    if (!mdb || !table || !table->entry || table->entry->table_pg <= 0 || !mdb->fmt)
        return;
    const uint32_t tdefPg = static_cast<uint32_t>(table->entry->table_pg);
    if (!UsageMapManager::readPage(mdb, tdefPg)) return;
    mdb_put_int32(mdb->pg_buf, mdb->fmt->tab_num_rows_offset,
                  static_cast<guint32>(table->num_rows));
    if (table->indices) {
        for (unsigned i = 0; i < table->indices->len; ++i) {
            MdbIndex* pidx =
                static_cast<MdbIndex*>(g_ptr_array_index(table->indices, i));
            if (!pidx || pidx->index_type == 2) continue;
            const int off = mdb->fmt->tab_cols_start_offset +
                            pidx->index_num * mdb->fmt->tab_ridx_entry_size;
            mdb_put_int32(mdb->pg_buf, off, static_cast<guint32>(pidx->num_rows));
        }
    }
    UsageMapManager::writePage(mdb, tdefPg);
}

int insertRowData(MdbTableDef* table, const Field* fields, size_t nfields,
                  uint32_t& outPg, uint16_t& outRownum, std::string& error) {
    outPg = 0;
    outRownum = 0;
    if (!table || !table->entry || !table->entry->mdb || !fields) {
        error = "insertRowData: null arg";
        return 0;
    }
    MdbHandle* mdb = table->entry->mdb;
    if (!mdb->f || !mdb->f->writable) {
        error = "insertRowData: database not writable";
        return 0;
    }

    std::vector<uint8_t> packed;
    Status ps = packRow(table, fields, nfields, packed);
    if (!ps.ok) {
        error = "insertRowData: pack failed: " + ps.error;
        return 0;
    }
    const size_t newRowSize = packed.size();
    if (newRowSize == 0 || newRowSize > (kPageSize - 16)) {
        error = "insertRowData: invalid packed row size";
        return 0;
    }

    std::vector<uint32_t> owned;
    UsageMapManager::getOwnedPages(mdb, table, owned);

    uint32_t candidatePg = 0;
    const uint32_t tdefPg = static_cast<uint32_t>(table->entry->table_pg);
    const int rco = mdb->fmt->row_count_offset;

    for (auto it = owned.rbegin(); it != owned.rend(); ++it) {
        uint32_t p = *it;
        if (!UsageMapManager::readPage(mdb, p)) continue;
        const auto* b = static_cast<const uint8_t*>(mdb->pg_buf);
        if (b[0] != kPageData) continue;
        if (UsageMapManager::getU32(mdb, kOffTableDefPg) != tdefPg) continue;
        const uint16_t nrows = UsageMapManager::getU16(mdb, rco);
        if (nrows >= 1000) continue;
        int minStart = kPageSize;
        for (int s = 0; s < nrows; ++s) {
            int off = UsageMapManager::getU16(mdb, (rco + 2) + (s * 2)) & 0x1FFF;
            if (off > 0 && off < minStart) {
                minStart = off;
            }
        }
        int hdrEnd = (rco + 2) + ((nrows + 1) * 2);
        if (minStart - hdrEnd >= static_cast<int>(newRowSize)) {
            candidatePg = p;
            break;
        }
    }

    if (candidatePg == 0) {
        Status ast = UsageMapManager::allocateDataPage(mdb, table, candidatePg);
        if (!ast.ok || candidatePg == 0) {
            error = "insertRowData: allocate page failed: " + ast.error;
            return 0;
        }
    }

    if (!UsageMapManager::readPage(mdb, candidatePg)) {
        error = "insertRowData: read candidate page failed";
        return 0;
    }
    auto* b = static_cast<uint8_t*>(mdb->pg_buf);
    uint16_t nrows = UsageMapManager::getU16(mdb, rco);
    int minStart = kPageSize;
    for (int s = 0; s < nrows; ++s) {
        int off = UsageMapManager::getU16(mdb, (rco + 2) + (s * 2)) & 0x1FFF;
        if (off > 0 && off < minStart) {
            minStart = off;
        }
    }
    int pos = minStart - static_cast<int>(newRowSize);
    int hdrEnd = (rco + 2) + ((nrows + 1) * 2);
    if (pos < hdrEnd) {
        error = "insertRowData: page overflow during write";
        return 0;
    }

    std::memcpy(b + pos, packed.data(), newRowSize);
    UsageMapManager::putU16(mdb, (rco + 2) + (nrows * 2), static_cast<uint16_t>(pos));
    nrows++;
    UsageMapManager::putU16(mdb, rco, nrows);

    const uint16_t newFree = static_cast<uint16_t>(pos - (rco + 2 + (nrows * 2)));
    UsageMapManager::putU16(mdb, 2, newFree);

    if (!UsageMapManager::writePage(mdb, candidatePg)) {
        error = "insertRowData: write data page failed";
        return 0;
    }

    if (table->indices) {
        for (unsigned ii = 0; ii < table->indices->len; ++ii) {
            MdbIndex* idx = static_cast<MdbIndex*>(g_ptr_array_index(table->indices, ii));
            if (!idx || idx->first_pg == 0 || idx->index_type == 2) continue;
            std::string idxErr;
            if (!updateIndex(table, idx, fields, nfields, candidatePg, nrows, idxErr)) {
                error = "insertRowData: updateIndex failed: " + idxErr;
                return 0;
            }
        }
    }

    table->num_rows++;
    finalizeTable(mdb, table);

    outPg = candidatePg;
    outRownum = nrows;
    return 1;
}

} /* namespace Jet4Writer (safe update primitives) */

/* ============================ C ABI ============================ */
static const uint8_t kEmptySentinel = 0;
static Jet4Writer::Field toJetField(const MdbField& m) {
    Jet4Writer::Field f;
    f.colnum = m.colnum;
    f.isNull = true;
    f.data = nullptr;
    f.size = 0;
    if (m.is_null != 0) return f;
    if (m.value == nullptr) {
        if (m.siz > 0) return f;
        f.isNull = false;
        f.data = &kEmptySentinel;
        return f;
    }
    f.isNull = false;
    f.data = static_cast<const uint8_t*>(m.value);
    f.size = m.siz > 0 ? static_cast<size_t>(m.siz) : 0;
    return f;
}

/* Process-wide last jet4_pack_row failure reason (diagnostic only,
 * single-writer assumption like the rest of the engine). */
namespace Jet4Writer { std::string g_lastPackError; }

extern "C" int jet4_pack_row(MdbTableDef* table, MdbField* fields, unsigned num_fields,
                             unsigned char* out, unsigned out_size) {
    if (!table || !fields || !out || !out_size) {
        Jet4Writer::g_lastPackError = "null arg";
        return -1;
    }
    std::vector<Jet4Writer::Field> fs;
    fs.reserve(num_fields);
    for (unsigned i = 0; i < num_fields; ++i) fs.push_back(toJetField(fields[i]));
    std::vector<uint8_t> packed;
    Jet4Writer::Status st = Jet4Writer::packRow(table, fs.data(), fs.size(), packed);
    if (!st.ok) {
        Jet4Writer::g_lastPackError =
            std::string(table->name[0] ? table->name : "?") + ": " + st.error;
        return -1;
    }
    if (packed.size() > out_size) {
        Jet4Writer::g_lastPackError =
            std::string(table->name[0] ? table->name : "?") + ": row too big for buffer";
        return -1;
    }
    std::memcpy(out, packed.data(), packed.size());
    return static_cast<int>(packed.size());
}

extern "C" const char* jet4_last_pack_error(void) {
    return Jet4Writer::g_lastPackError.c_str();
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

extern "C" int jet4_insert_row(MdbTableDef* table, MdbField* fields, unsigned num_fields) {
    if (!table || !fields || num_fields == 0) return 0;
    std::vector<Jet4Writer::Field> fs;
    fs.reserve(num_fields);
    for (unsigned i = 0; i < num_fields; ++i) fs.push_back(toJetField(fields[i]));
    uint32_t outPg = 0;
    uint16_t outRownum = 0;
    std::string err;
    int res = Jet4Writer::insertRowData(table, fs.data(), fs.size(), outPg, outRownum, err);
    if (!res) {
        Jet4Writer::g_lastPackError = err;
    }
    return res;
}
