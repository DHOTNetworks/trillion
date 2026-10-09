#pragma once
/*
 * Jet4Writer — clean, modular Jet 4 row packer, index engine & verifier.
 *
 * Modular architecture:
 *   - jet4_types.h     : Page layout constants, Status, ByteVec, Field, VerifyReport
 *   - jet4_codes.h/.cpp: Column encoders (Integer, Double, Text sort-order, null rules)
 *   - jet4_usage_map.h/.cpp: Global page allocator & Type 0 / Type 1 usage map manager
 *   - jet4_btree.h/.cpp: B-Tree engine (descent, leaf insert, page split, divider propagation, walk)
 *   - jet4_table.h/.cpp: Brute-force TableSnapshot (owned buffers, no cursor/map state)
 *   - jet4_writer.h/.cpp: High-level facade & verification suite (self-sufficient entry point)
 *
 * Include this header alone to get the complete engine (Jackcess-mirrored).
 */

#include "jet4_types.h"
#include "jet4_codes.h"
#include "jet4_usage_map.h"
#include "jet4_btree.h"
#include "jet4_table.h"

extern "C" {
#include "mdbtools.h"
}

namespace Jet4Writer {

/* Process-wide last jet4_pack_row failure reason (diagnostic only). */
extern std::string g_lastPackError;

/* Packs one Jet4 row. Output is exactly what mdb_crack_row() expects:
 * [colcount:2][fixed block][var data][EOD:2][var offsets reversed][varcount:2][nullmask] */
Status packRow(MdbTableDef* table, const Field* fields, size_t nfields,
               std::vector<uint8_t>& out);

/* Builds one composite index key (Jackcess writeValue compatible):
 * every non-null column -> start flag + payload; null -> single flag byte.
 * Appends the big-endian (pg << 8 | (row - 1)) trailer. */
inline Status buildEntry(MdbTableDef* table, MdbIndex* idx,
                         const Field* fields, size_t nfields,
                         uint32_t dataPg, uint16_t rownum,
                         std::vector<uint8_t>& outEntry) {
    return BTreeEngine::buildEntry(table, idx, fields, nfields, dataPg, rownum, outEntry);
}

/* Inserts a complete entry (key + trailer) into the index B-tree leaf level. */
inline Status leafInsert(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                         const uint8_t* entry, size_t entryLen) {
    return BTreeEngine::leafInsert(mdb, table, idx, entry, entryLen);
}

/* Full updateIndex: builds the entry, inserts it into leaf B-tree, and bumps
 * in-memory + on-disk index row counts. Returns bytes written (0 = skip/fail). */
int updateIndex(MdbTableDef* table, MdbIndex* idx,
                const Field* fields, size_t nfields,
                uint32_t dataPg, uint16_t rownum, std::string& error);

/* Safe in-place row update for an existing (dataPg, rownum-1 slot) row:
 * erases all index entries built from the old image, rebuilds the data page
 * canonically with the new packed row (same slots/order/flags), then
 * inserts index entries for the new image. Fails cleanly (no partial page
 * writes); single-writer assumption. Requires mdb_read_columns + indices. */
int reindexRowData(MdbTableDef* table, const Field* newFields, size_t nfields,
                   uint32_t dataPg, uint16_t rownum, std::string& error);

/* Clears the 0x8000 deleted bit on one slot (seed-deleted row resurrection).
 * Never touches 0x4000 overflow fragments. Uses bypassed page I/O so no
 * cursor or pg_buf cache state leaks. Returns false on any anomaly. */
bool resurrectDeletedSlot(MdbHandle* mdb, MdbTableDef* table,
                          uint32_t dataPg, int slot, std::string& error);

/* Canonical same-slot row replacement WITHOUT index maintenance, for
 * index-less tables (TempLastEnteredVoucher et al.): packs fields and
 * rebuilds the data page canonically (same slots/order/flags, exact
 * free-space recompute). Fails cleanly; never pokes bytes in place. */
int replaceRowData(MdbTableDef* table, const Field* newFields, size_t nfields,
                   uint32_t dataPg, uint16_t rownum, std::string& error);

/* Guarantees one index entry per index for (dataPg, rownum): erases every
 * entry for the rowId (heals ghosts/duplicates, tolerates absence), then
 * inserts the entry built from fields. Required after resurrectDeletedSlot:
 * a resurrected row whose values are unchanged takes reindexRowData's
 * oldE==newE fast path and would otherwise stay live with NO entries.
 * Cardinality stays coherent via eraseEntry's -1 / insert's +1.
 * Fields must be owned buffers (snapshot / reindex nf): page I/O between
 * indexes clobbers pg_buf, so cracked-field pointers would dangle. */
int ensureRowIndexed(MdbTableDef* table, const Field* fields, size_t nfields,
                     uint32_t dataPg, uint16_t rownum, std::string& error);

/* Persists table + index cardinalities to the table-definition page using
 * bypassed page I/O (never the stale pg_buf cache). Mirrors Jackcess
 * TableImpl.setRowCount / IndexData.setRowCount semantics. */
void finalizeTable(MdbHandle* mdb, MdbTableDef* table);

/* Native row insert: finds candidate data page or allocates new data page,
 * packs row, writes data page canonically, inserts entries into all table
 * B-Tree indices, and finalizes TableDef cardinality. Completely replaces
 * libmdb's mdb_insert_row. */
int insertRowData(MdbTableDef* table, const Field* fields, size_t nfields,
                  uint32_t& outPg, uint16_t& outRownum, std::string& error);

/* Walks one index left-to-right via leaf chain, decoding full keys. */
inline Status walkIndex(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                        std::vector<IndexEntryInfo>& out) {
    return BTreeEngine::walkIndex(mdb, table, idx, out);
}

/* Whole-file audit used by the CLI (--verify-jet) and the export dialog:
 * crack-audits data rows and walks every index of the core tables.
 * Invariant per index: walked entries == data rows, strictly sorted. */
VerifyReport verifyDatabase(MdbHandle* mdb);

} /* namespace Jet4Writer */

/* C ABI for delegation from C translation units (libmdb write.c) */
extern "C" {
int jet4_pack_row(MdbTableDef* table, MdbField* fields, unsigned num_fields,
                  unsigned char* out, unsigned out_size);
int jet4_update_index(MdbTableDef* table, MdbIndex* idx, MdbField* fields,
                      unsigned num_fields, unsigned data_pg, unsigned rownum);
int jet4_insert_row(MdbTableDef* table, MdbField* fields, unsigned num_fields);
/* Reason for the last jet4_pack_row failure (never null; empty = none yet).
 * Lets libmdb report WHY a row was rejected instead of a bare "failed". */
const char* jet4_last_pack_error(void);
}
