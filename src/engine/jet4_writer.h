#pragma once
/*
 * Jet4Writer — clean, modular Jet 4 row packer, index engine & verifier.
 *
 * Modular architecture:
 *   - jet4_types.h     : Page layout constants, Status, ByteVec, Field, VerifyReport
 *   - jet4_codes.h/.cpp: Column encoders (Integer, Double, Text sort-order, null rules)
 *   - jet4_usage_map.h/.cpp: Global page allocator & Type 0 / Type 1 usage map manager
 *   - jet4_btree.h/.cpp: B-Tree engine (descent, leaf insert, page split, divider propagation, walk)
 *   - jet4_writer.h/.cpp: High-level facade & verification suite
 */

#include "jet4_types.h"
#include "jet4_codes.h"
#include "jet4_usage_map.h"
#include "jet4_btree.h"

extern "C" {
#include "mdbtools.h"
}

namespace Jet4Writer {

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
}
