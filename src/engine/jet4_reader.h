#pragma once
/*
 * Jet4Reader — Jackcess-faithful strictly-JET4 table reader.
 *
 * Replaces the buggy libmdb fetch-cursor read path
 * (mdb_bind_column + mdb_rewind_table + mdb_fetch_row + cur_row-1 flag peek)
 * used by the Bahi-Khata / Busy migrators.
 *
 * Why the old path fails (root cause):
 *   libmdb data.c mdb_read_row() treats 0x4000 as "delflag" (skip) and 0x8000
 *   as "lookupflag" (keep), which is the exact inverse of the JET4 spec that
 *   Jackcess documents (TableImpl: DELETED_ROW_MASK = 0x8000,
 *   OVERFLOW_ROW_MASK = 0x4000). With table->noskip_del = 1 the migrators
 *   additionally forced 4-byte overflow POINTER slots to be cracked as if
 *   they were real rows (garbage), while 0x8000 deleted rows were kept and
 *   then filtered with mdb_find_row(cur_row - 1) — a slot index that is
 *   already wrong after internal skips / page turns, with stale bind buffers.
 *   The first deleted entry in a Bahi-Khata database therefore corrupts every
 *   subsequent row of that table.
 *
 * Jackcess model implemented here (TableImpl.RowState / TableScanCursor):
 *   RowStatus  = INVALID_PAGE | INVALID_ROW | VALID | DELETED | NORMAL | OVERFLOW
 *   isDeletedRow(rowStart)  = (rowStart & 0x8000) != 0   // skip, never decode
 *   isOverflowRow(rowStart) = (rowStart & 0x4000) != 0   // follow pointer chain
 *   cleanRowStart(rowStart) = rowStart & 0x1FFF
 *   positionAtRowHeader sets DELETED / OVERFLOW / NORMAL;
 *   positionAtRowData follows the overflow-pointer chain (hop-capped) and
 *   ignores DELETED bits on overflow targets (always marked deleted).
 * Page handling mirrors Jackcess PageChannel: one buffered page (curPg),
 * re-read only on page change — MEMO resolution is buffer-safe
 * (mdb_find_pg_row pairs every alt-buffer swap; crack is read-only).
 *
 * Enumeration is brute-force over owned data pages (same source as
 * Jet4Writer::TableSnapshot, never the fetch cursor, never usage-map
 * assisted index scans), so migration and export observe identical row sets.
 * Value formatting reuses libmdb's mdb_col_to_string() byte-for-byte, so
 * downstream migration logic (parseInt/parseDouble/parseDate) is unchanged —
 * only row MEMBERSHIP changes (correct live-row set).
 *
 * Performance: single sequential pass per table, O(pages x slots), one
 * mdb_crack_row per live row, no per-column heap churn beyond the output map.
 */

#include <map>
#include <string>
#include <vector>

extern "C" {
#include "mdbtools.h"
}

namespace Jet4Reader {

/* Per-table scan diagnostics: proves DELETED vs OVERFLOW differentiation. */
struct ReadStats {
    long liveRows = 0;
    long deletedSkipped = 0;    /* slots with 0x8000 (incl. 0xC000 tombstones) */
    long overflowFollowed = 0;  /* 0x4000 pointers successfully resolved */
    long overflowBroken = 0;    /* 0x4000 pointers with bad target */
    long invalidSlots = 0;      /* mdb_find_row failure (bounds-rejected) */
    long emptySlots = 0;        /* zero-length slots (freed/never-used) */
    long crackFailed = 0;       /* mdb_crack_row rejected the row buffer */
};

/* Drop-in replacement for the old fetch-cursor readers.
 * Returns live rows only, as {columnName -> formatted string} maps.
 * Formatting is identical to the old mdb_bind_column path. */
std::vector<std::map<std::string, std::string>> readTable(MdbHandle* mdb,
                                                           const char* tableName);

/* Same, with diagnostics out-param (may be nullptr). */
std::vector<std::map<std::string, std::string>> readTable(MdbHandle* mdb,
                                                           const char* tableName,
                                                           ReadStats* stats);

/* Deleted-slot rows (flags & 0x8000), cracked the same way as live rows.
 * Display-only use: lets voucher/ledger resolution show the true name of a
 * ledger that was later deleted in Bahi-Khata instead of "Party #<code>".
 * Never use for party creation, balances, or counts. Overflow-target
 * fragments (also 0x8000-marked) may appear here; callers must prefer the
 * live map and consult this only for codes the live map lacks. */
std::vector<std::map<std::string, std::string>> readDeletedRows(MdbHandle* mdb,
                                                                 const char* tableName);

} /* namespace Jet4Reader */
