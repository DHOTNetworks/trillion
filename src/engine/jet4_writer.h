#pragma once
/*
 * Jet4Writer — clean, memory-safe (C++17, RAII, bounds-checked) Jet 4
 * row packer / index-key builder / B-tree leaf writer.
 *
 * Byte-level spec derived from the Jackcess reference implementation:
 *   TableImpl.createRow      -> packRow()
 *   IndexData.writeValue     -> buildKey()   (start-flag + payload, null rule)
 *   IndexPageCache/DataPage  -> leafInsert() (page-prefix model, prefix recompute)
 *   JetFormat (V2000/V2003)  -> page layout constants below
 *   PageChannel.allocateNewPage -> page allocation (delegated to libmdb map layer)
 *
 * Only Jet 4 (ugur .mdb, Data.00x) is supported. Jet 3 helpers are
 * intentionally absent — mixing the two layouts caused the historic
 * prev/next offset corruption (Jet3 prev@8/next@12 vs Jet4 prev@12/next@16).
 */
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

/* libmdb types (mdbtools.h is C++-compatible). */
#include "mdbtools.h"

namespace Jet4Writer {

/* ------------------------------------------------------------------ */
/* Jet 4 page layout constants (Jackcess JetFormat V2000/V2003)        */
/* ------------------------------------------------------------------ */
inline constexpr int kPageSize = 4096;
inline constexpr uint8_t kPageLeaf = 0x04;
inline constexpr uint8_t kPageIndex = 0x03;
inline constexpr uint8_t kPageData = 0x01;

/* Leaf/node header field offsets (little-endian) */
inline constexpr int kOffFreeSpace = 2;
inline constexpr int kOffTableDefPg = 4;
inline constexpr int kOffUnknown = 8;      /* reserved, must stay 0 */
inline constexpr int kOffPrevPg = 12;      /* Jet4: NOT 8 (that is Jet3) */
inline constexpr int kOffNextPg = 16;      /* Jet4: NOT 12 (that is Jet3) */
inline constexpr int kOffChildTailPg = 20;
inline constexpr int kOffPrefixLen = 24;   /* int16: shared entry-prefix length */
inline constexpr int kOffPrefixUnknown = 26;
inline constexpr int kOffEntryMask = 27;   /* 0x1B */
inline constexpr int kSizeEntryMask = 453;
inline constexpr int kEntryAreaStart = 480; /* 0x1E0 = 27 + 453 */

/* Index key flags (Jackcess IndexCodes) */
inline constexpr uint8_t kAscStart = 0x7F;
inline constexpr uint8_t kAscNull = 0x00;
inline constexpr uint8_t kDescStart = 0x80;
inline constexpr uint8_t kDescNull = 0xFF;

/* ------------------------------------------------------------------ */
/* Field view: non-owning. Caller must keep bytes alive during the call */
/* ------------------------------------------------------------------ */
struct Field {
    int colnum = -1;              /* 0-based table column */
    bool isNull = true;
    const uint8_t* data = nullptr;
    size_t size = 0;
};

struct Status {
    bool ok = false;
    std::string error;
    static Status Ok() { return Status{true, {}}; }
    static Status Fail(const std::string& e) { return Status{false, e}; }
private:
    Status(bool o, const std::string& e) : ok(o), error(e) {}
};

/* Packs one Jet4 row. Output is exactly what mdb_crack_row() expects:
 * [colcount:2][fixed block][var data][EOD:2][var offsets reversed][varcount:2][nullmask] */
Status packRow(MdbTableDef* table, const Field* fields, size_t nfields,
               std::vector<uint8_t>& out);

/* Builds one composite index key (Jackcess writeValue compatible):
 * every non-null column -> start flag + payload; null -> single flag byte.
 * Appends the big-endian (pg << 8 | row) trailer when withTrailer is set. */
Status buildEntry(MdbTableDef* table, MdbIndex* idx,
                  const Field* fields, size_t nfields,
                  uint32_t dataPg, uint16_t rownum,
                  std::vector<uint8_t>& outEntry);

/* Inserts a complete entry (key + trailer) into the index B-tree leaf level.
 * Rewrites the target leaf with a recomputed page prefix (Jackcess model);
 * allocates + chains a new leaf with CORRECT Jet4 offsets on overflow. */
Status leafInsert(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                  const uint8_t* entry, size_t entryLen);

/* Full updateIndex replacement: builds the entry and inserts it, then bumps
 * the in-memory + on-disk index row counts. Returns bytes written (0 = skip). */
int updateIndex(MdbTableDef* table, MdbIndex* idx,
                const Field* fields, size_t nfields,
                uint32_t dataPg, uint16_t rownum, std::string& error);

/* ---- Independent verification (fresh logic, page-prefix model) ---- */
struct IndexEntryInfo {
    std::vector<uint8_t> fullKey; /* without trailer */
    uint32_t dataPg = 0;
    uint16_t rowIdx = 0;          /* 0-based row within data page */
    uint32_t leafPg = 0;
};

/* Walks one index left-to-right via leaf chain, decoding full keys. */
Status walkIndex(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                 std::vector<IndexEntryInfo>& out);

/* Whole-file audit used by the CLI (--verify-jet) and the export dialog:
 * crack-audits data rows and walks every index of the core tables.
 * Invariant per index: walked entries == data rows, strictly sorted. */
struct VerifyIndexInfo {
    std::string table;
    std::string name;
    long walked = 0;
    long dataRows = 0;
    long catalogRows = 0;
    bool sorted = false;
};

struct VerifyReport {
    bool ok = false;
    long tablesChecked = 0;
    long dataRows = 0;
    long badRows = 0;
    std::vector<VerifyIndexInfo> indexes;
    std::string error;
};

VerifyReport verifyDatabase(MdbHandle* mdb);

} /* namespace Jet4Writer */

/* C ABI for delegation from C translation units (libmdb write.c) */
extern "C" {
int jet4_pack_row(MdbTableDef* table, MdbField* fields, unsigned num_fields,
                  unsigned char* out, unsigned out_size);
int jet4_update_index(MdbTableDef* table, MdbIndex* idx, MdbField* fields,
                      unsigned num_fields, unsigned data_pg, unsigned rownum);
}
