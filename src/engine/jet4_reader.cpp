#include "jet4_reader.h"
#include "jet4_usage_map.h"

#include <cmath>
#include <cstdio>
#include <cstring>

extern "C" {
int mdb_find_row(MdbHandle* mdb, int row_number, int* row_start, size_t* row_size);
int mdb_crack_row(MdbTableDef* table, int row_start, size_t row_size, MdbField* fields);
char* mdb_col_to_string(MdbHandle* mdb, void* buf, int start, int datatype, int size);
void mdb_free_tabledef(MdbTableDef* table);
MdbTableDef* mdb_read_table_by_name(MdbHandle* mdb, gchar* table_name, int obj_type);
GPtrArray* mdb_read_columns(MdbTableDef* table);
}

namespace Jet4Reader {

/* Jackcess TableImpl row-flag constants (strictly JET4). */
static constexpr int kDeletedMask = 0x8000;
static constexpr int kOverflowMask = 0x4000;
static constexpr int kOffsetMask = 0x1FFF;
static constexpr uint8_t kPageData = 0x01;
static constexpr int kOffTableDefPg = 4;

/* Jackcess ColumnImpl.readDateValue/getDateTimeFactory: SHORT_DATE_TIME is an
 * IEEE-754 double of days since 1899-12-30 (whole = date, fraction = time of
 * day), decoded locale-free. libmdb renders dates via locale-dependent
 * strftime("%x %X"), so the same database migrates differently under
 * different OS locales (DD/MM vs MM/DD swaps). Decode natively to a fixed
 * "MM/DD/YYYY HH:MM:SS" shape instead: every downstream consumer
 * (parseDateFormatted/parseDateStr) takes the space-separated date part and
 * accepts both MM/DD/YYYY and ISO, so output stays compatible while becoming
 * deterministic. Pure calendar math, no timezone (matches Jackcess
 * LocalDateTime and libmdb's wall-time strftime alike). */
static bool decodeJetDateTime(const uint8_t* bytes, size_t len,
                              std::string& out) {
    if (!bytes || len < 8) return false;
    double td = 0.0;
    std::memcpy(&td, bytes, 8);
    /* Days-from-civil (Howard Hinnant), base 1899-12-30 per Jackcess BASE_LD. */
    auto daysFromCivil = [](int y, int m, int d) -> long {
        y -= (m <= 2);
        const long era = (y >= 0 ? y : y - 399) / 400;
        const unsigned yoe = static_cast<unsigned>(y - era * 400);
        const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
        const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
        return era * 146097 + static_cast<long>(doe) - 719468;
    };
    auto civilFromDays = [](long z, int& y, int& m, int& d) {
        z += 719468;
        const long era = (z >= 0 ? z : z - 146096) / 146097;
        const unsigned doe = static_cast<unsigned>(z - era * 146097);
        const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
        y = static_cast<int>(yoe) + static_cast<int>(era) * 400;
        const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
        const unsigned mp = (5 * doy + 2) / 153;
        d = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
        m = static_cast<int>(mp + (mp < 10 ? 3 : -9));
        y += (m <= 2);
    };
    const long baseDays = daysFromCivil(1899, 12, 30);
    const long whole = static_cast<long>(std::floor(td));
    double frac = td - static_cast<double>(whole);
    if (frac < 0.0) frac = 0.0;
    if (frac >= 1.0) frac = 0.0;
    long long totalSecs = static_cast<long long>(std::floor(frac * 86400.0 + 0.5));
    if (totalSecs >= 86400) totalSecs = 86399;
    if (totalSecs < 0) totalSecs = 0;
    int y = 0, mo = 0, d = 0;
    civilFromDays(baseDays + whole, y, mo, d);
    if (y < 1000 || y > 9999 || mo < 1 || mo > 12 || d < 1 || d > 31) return false;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02d/%02d/%04d %02lld:%02lld:%02lld",
                  mo, d, y, totalSecs / 3600, (totalSecs / 60) % 60, totalSecs % 60);
    out = buf;
    return true;
}

static std::string decodeField(MdbHandle* mdb, MdbColumn* col, const MdbField& f) {
    if (!col) return {};
    /* Boolean is stored in the null mask (Jackcess ColumnImpl: booleans have
     * no row bytes). libmdb defaults boolean text to "0"/"1" numbers. */
    if (col->col_type == MDB_BOOL) {
        return f.is_null ? "0" : "1";
    }
    if (f.is_null) return {};
    if (!f.value || f.siz == 0) return {};
    /* OLE blobs are never consumed by migration logic; the old bind path left
     * a 12-byte memo header in the buffer (garbage downstream). Return empty. */
    if (col->col_type == MDB_OLE) return {};
    /* DATETIME is decoded natively (see decodeJetDateTime): locale-free,
     * unlike libmdb's strftime path used by mdb_col_to_string. */
    if (col->col_type == MDB_DATETIME) {
        std::string iso;
        if (decodeJetDateTime(static_cast<const uint8_t*>(f.value), f.siz, iso))
            return iso;
        return {};
    }
    char* s = mdb_col_to_string(mdb, mdb->pg_buf, f.start, col->col_type, f.siz);
    if (!s) return {};
    std::string out(s);
    g_free(s);
    return out;
}

/* Shared brute-force scan core. wantLive collects DELETED-skipped,
 * OVERFLOW-followed live rows; wantDeleted collects flags&0x8000 slots
 * (genuinely deleted rows plus overflow-target fragments). */
static void scanTable(MdbHandle* mdb, const char* tableName, bool wantLive,
                      bool wantDeleted,
                      std::vector<std::map<std::string, std::string>>* liveOut,
                      std::vector<std::map<std::string, std::string>>* deletedOut,
                      ReadStats* stats) {
    ReadStats local;
    if (!mdb || !tableName || !tableName[0] || !mdb->fmt) {
        if (stats) *stats = local;
        return;
    }

    MdbTableDef* table =
        mdb_read_table_by_name(mdb, const_cast<char*>(tableName), MDB_TABLE);
    if (!table) {
        if (stats) *stats = local;
        return;
    }

    mdb_read_columns(table);
    if (!table->columns || table->num_cols == 0) {
        mdb_free_tabledef(table);
        if (stats) *stats = local;
        return;
    }
    const unsigned numCols = table->num_cols;
    if (numCols > 1000) {
        mdb_free_tabledef(table);
        if (stats) *stats = local;
        return;
    }
    const uint32_t tdefPg =
        (table->entry) ? static_cast<uint32_t>(table->entry->table_pg) : 0;
    if (tdefPg == 0) {
        mdb_free_tabledef(table);
        if (stats) *stats = local;
        return;
    }

    std::vector<std::string> colNames(numCols);
    std::vector<MdbColumn*> cols(numCols, nullptr);
    for (unsigned j = 0; j < numCols; ++j) {
        auto* col = static_cast<MdbColumn*>(g_ptr_array_index(table->columns, j));
        cols[j] = col;
        if (col && col->name[0] != '\0') colNames[j] = col->name;
    }

    /* Owned data pages: same source as Jet4Writer::TableSnapshot so that
     * migration observes exactly the row set the exporter writes. */
    std::vector<uint32_t> ownedPages;
    if (!Jet4Writer::UsageMapManager::getOwnedPages(mdb, table, ownedPages).ok) {
        mdb_free_tabledef(table);
        if (stats) *stats = local;
        return;
    }

    std::vector<MdbField> fb(numCols);

    auto emitRow = [&](int crackOff, size_t crackSz,
                       std::vector<std::map<std::string, std::string>>* out) {
        if (mdb_crack_row(table, crackOff, crackSz, fb.data()) < 0) {
            local.crackFailed++;
            return;
        }
        std::map<std::string, std::string> row;
        for (unsigned j = 0; j < numCols; ++j) {
            if (colNames[j].empty() || !cols[j]) continue;
            /* Column order in fb[] is table order; colnum echoes index. */
            row[colNames[j]] = decodeField(mdb, cols[j], fb[j]);
        }
        out->push_back(std::move(row));
    };

    /* Page cache: pg_buf always hosts curPg. MEMO resolution is buffer-safe
     * (mdb_find_pg_row pairs every alt-buffer swap, mdb_crack_row is
     * read-only), so re-read only on page change — never serve a stale page
     * after an overflow follow, never waste a pread per slot. */
    uint32_t curPg = 0;
    auto ensurePage = [&](uint32_t pg) -> bool {
        if (pg == 0) return false; /* page 0 is the DB header, never data */
        if (pg == curPg) return true;
        if (!Jet4Writer::UsageMapManager::readPage(mdb, pg)) return false;
        curPg = pg;
        return true;
    };

    for (uint32_t p : ownedPages) {
        curPg = 0; /* force physical re-read: guards against any pg_buf use
                    * between tables (catalog reads, memo alt-buffer trails). */
        if (!ensurePage(p)) continue;
        const auto* b0 = static_cast<const uint8_t*>(mdb->pg_buf);
        if (b0[0] != kPageData) continue;
        if (Jet4Writer::UsageMapManager::getU32(mdb, kOffTableDefPg) != tdefPg)
            continue;
        const int nrows =
            Jet4Writer::UsageMapManager::getU16(mdb, mdb->fmt->row_count_offset);
        if (nrows <= 0 || nrows > 1000) continue;

        for (int s = 0; s < nrows; ++s) {
            if (!ensurePage(p)) break;
            int rs = 0;
            size_t rsz = 0;
            if (mdb_find_row(mdb, s, &rs, &rsz) != 0) {
                local.invalidSlots++;
                continue;
            }
            if (rsz == 0) {
                local.emptySlots++;
                continue;
            }
            int flags = rs & (kDeletedMask | kOverflowMask);
            int off = rs & kOffsetMask;

            if (flags & kDeletedMask) {
                /* Jackcess positionAtRowHeader -> DELETED (covers 0xC000
                 * tombstones and overflow targets): never a live row. */
                local.deletedSkipped++;
                if (wantDeleted && deletedOut) {
                    emitRow(off, rsz, deletedOut);
                }
                continue;
            }

            if (!wantLive) continue;

            int crackOff = off;
            size_t crackSz = rsz;

            /* Jackcess positionAtRowData: follow the overflow-pointer chain.
             * The header row decides DELETED first (0x8000, incl. 0xC000);
             * once following, ONLY the 0x4000 bit matters — subsequent
             * DELETED bits are ignored (overflow targets are always marked
             * deleted). Hop cap so a corrupt cycle can never spin. */
            int hops = 0;
            while ((flags & kOverflowMask) != 0) {
                if (++hops > 8 || crackSz < 4) {
                    crackSz = 0;
                    break;
                }
                const uint8_t* rb = static_cast<const uint8_t*>(mdb->pg_buf);
                const uint8_t ovSlot = rb[crackOff];
                const uint32_t ovPg =
                    static_cast<uint32_t>(rb[crackOff + 1]) |
                    (static_cast<uint32_t>(rb[crackOff + 2]) << 8) |
                    (static_cast<uint32_t>(rb[crackOff + 3]) << 16);
                if (ovPg == 0 || !ensurePage(ovPg)) {
                    crackSz = 0;
                    break;
                }
                int ovRs = 0;
                size_t ovRsz = 0;
                if (mdb_find_row(mdb, static_cast<int>(ovSlot), &ovRs, &ovRsz) != 0 ||
                    ovRsz == 0) {
                    crackSz = 0;
                    break;
                }
                flags = ovRs & (kDeletedMask | kOverflowMask);
                crackOff = ovRs & kOffsetMask;
                crackSz = ovRsz;
            }
            if (crackSz == 0) {
                local.overflowBroken++;
                continue;
            }
            if (hops > 0) local.overflowFollowed++;

            if (liveOut) {
                const size_t before = liveOut->size();
                emitRow(crackOff, crackSz, liveOut);
                if (liveOut->size() != before) local.liveRows++;
            }
        }
    }

    mdb_free_tabledef(table);
    if (stats) *stats = local;
}

std::vector<std::map<std::string, std::string>> readTable(MdbHandle* mdb,
                                                           const char* tableName) {
    return readTable(mdb, tableName, nullptr);
}

std::vector<std::map<std::string, std::string>> readTable(MdbHandle* mdb,
                                                           const char* tableName,
                                                           ReadStats* stats) {
    std::vector<std::map<std::string, std::string>> result;
    scanTable(mdb, tableName, true, false, &result, nullptr, stats);
    return result;
}

std::vector<std::map<std::string, std::string>> readDeletedRows(MdbHandle* mdb,
                                                                 const char* tableName) {
    std::vector<std::map<std::string, std::string>> result;
    scanTable(mdb, tableName, false, true, nullptr, &result, nullptr);
    return result;
}

} /* namespace Jet4Reader */
