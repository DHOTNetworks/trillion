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
                                            uint32_t newPg, std::string& err);

    static Status registerIndexPage(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx, uint32_t newPg);
};

} /* namespace Jet4Writer */
