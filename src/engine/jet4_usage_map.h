#pragma once
/*
 * Jet4 Usage Map Manager (Jackcess PageChannel & UsageMap model).
 * Allocates index pages globally and registers them exclusively in index maps.
 */

#include "jet4_types.h"

extern "C" {
#include "mdbtools.h"
}

namespace Jet4Writer {

class UsageMapManager {
public:
    static bool readPage(MdbHandle* mdb, uint32_t pg);
    static bool writePage(MdbHandle* mdb, uint32_t pg);

    static uint16_t getU16(MdbHandle* mdb, int offset);
    static uint32_t getU32(MdbHandle* mdb, int offset);
    static void putU16(MdbHandle* mdb, int offset, uint16_t val);
    static void putU32(MdbHandle* mdb, int offset, uint32_t val);

    static gint32 allocateIndexPage(MdbHandle* mdb, MdbTableDef* table);

    static bool readMapRow(MdbHandle* mdb, uint32_t mapPg, uint16_t mapRow,
                           std::vector<uint8_t>& rowBytes, int& rowStart);
    static bool writeMapRow(MdbHandle* mdb, uint32_t mapPg, int rowStart,
                            const std::vector<uint8_t>& rowBytes);

    static bool mapBitOp(MdbHandle* mdb, std::vector<uint8_t>& row, uint32_t pg, bool set);
    static bool refMapBitOp(MdbHandle* mdb, MdbTableDef* table,
                            uint32_t mapPg, uint16_t mapRow, uint32_t pg, bool set);
    static bool promoteInlineMapToReference(MdbHandle* mdb, MdbTableDef* table,
                                            uint32_t mapPg, uint16_t mapRow,
                                            uint32_t newPg, std::string& err,
                                            bool withNewPg = true);

    /* Re-read the table's data + free map def rows from disk into the
     * in-memory copies libmdb syncs from. Required after any direct-disk
     * mutation of those rows (promotion, bitmap-pointer growth): otherwise
     * the next mdb_alloc_page/mdb_map_sync_to_disk pushes stale memory
     * over the fresh disk state (silent revert). */
    static bool refreshTableMaps(MdbHandle* mdb, MdbTableDef* table);

    /* One-time migration (Jackcess on-demand promotion): ensure a table's
     * data + free usage maps are reference type, then set bits for every
     * real data page (brute-force enumerated). Inline maps address only 512
     * pages; files grown past that lose map visibility for new pages
     * (scans miss rows) until promoted. Reference maps are permanent. */
    static Status ensureReferenceMaps(MdbHandle* mdb, MdbTableDef* table);

    static Status registerIndexPage(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx, uint32_t newPg);

    static Status getOwnedPages(MdbHandle* mdb, MdbTableDef* table, std::vector<uint32_t>& outPages);
    static Status allocateDataPage(MdbHandle* mdb, MdbTableDef* table, uint32_t& newPg);

    /* Remove a page from a table's data + free maps (for pages that change
     * role: fresh index/bitmap pages allocated via mdb_alloc_page start life
     * flagged as data). Mirrors the proven clear logic; memory-coherent. */
    static void clearDataMaps(MdbHandle* mdb, MdbTableDef* table, uint32_t pg);
};

} /* namespace Jet4Writer */
