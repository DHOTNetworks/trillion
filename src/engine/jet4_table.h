#pragma once
/*
 * Jet4 Table Snapshot (Jackcess TableScanCursor/RowState model).
 *
 * The single safe way to enumerate table rows: brute-force page walk
 * (never usage maps, never fetch cursors), crack each slot, and DEEP-COPY
 * every field's bytes into owned storage. No pg_buf pointer, no map bit,
 * no table-scan cursor state escapes this module, so later page I/O cannot
 * invalidate anything the caller holds. Row flags (deleted/overflow) are
 * reported, never interpreted.
 */

#include "jet4_types.h"
#include <QString>

extern "C" {
#include "mdbtools.h"
}

namespace Jet4Writer {

struct RowSnapshot {
    uint32_t pg = 0;
    int slot = -1;
    int flags = 0; /* location flag bits (0x8000 deleted, 0x4000 overflow) */
    bool cracked = false;
    std::vector<uint8_t> raw;              /* owned row bytes */
    std::vector<std::vector<uint8_t>> colBytes; /* owned per-column bytes */
    std::vector<Field> fields;             /* view into colBytes */
};

class TableSnapshot {
public:
    /* Enumerate every data-page slot of table via owned_pages usage map.
     * Crackable rows get deep-copied fields; others are reported with
     * cracked=false for repair flows. */
    static Status snapshot(MdbHandle* mdb, MdbTableDef* table,
                           std::vector<RowSnapshot>& out);
};

/* Jackcess-faithful Jet4 Unicode/compressed-text decoder */
QString decodeJet4QString(const uint8_t* data, size_t len);
inline QString decodeJet4QString(const Field& f) {
    return (f.isNull || !f.data || f.size == 0) ? QString() : decodeJet4QString(f.data, f.size);
}

} /* namespace Jet4Writer */
