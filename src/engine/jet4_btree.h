#pragma once
/*
 * Jet4 B-Tree Engine (Jackcess-inspired B-Tree node/leaf management).
 * Handles full composite keys, entry comparisons with rowId tie-breaking,
 * descent (findLeaf), leaf insertion, page splitting, parent divider propagation,
 * and traversal (walkIndex).
 */

#include "jet4_types.h"
#include "jet4_codes.h"
#include "jet4_usage_map.h"

extern "C" {
#include "mdbtools.h"
}

namespace Jet4Writer {

struct DecodedPage {
    std::vector<std::vector<uint8_t>> full; /* every entry, complete bytes */
    std::vector<uint8_t> prefix;
};

class BTreeEngine {
public:
    /* Jackcess Entry.compareTo total order: entryBytes first (unsigned
     * memcmp with length tiebreak), then rowId (data page, then row).
     * Node child links never participate. isLeaf=false strips the trailing
     * 4-byte child page before comparing. */
    static int cmpIndexOrder(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b, bool isLeaf);
    static uint32_t childOf(const std::vector<uint8_t>& divEntry);
    static std::vector<uint8_t> makeDivider(bool wasLeaf, const std::vector<uint8_t>& childMaxEntry, uint32_t childPg);

    static bool decodeEntries(MdbHandle* mdb, DecodedPage& dp, std::string& err);

    static Status buildEntry(MdbTableDef* table, MdbIndex* idx,
                             const Field* fields, size_t nfields,
                             uint32_t dataPg, uint16_t rownum,
                             std::vector<uint8_t>& outEntry);

    static uint32_t findLeaf(MdbHandle* mdb, MdbIndex* idx,
                             const uint8_t* key, size_t keyLen,
                             std::vector<uint32_t>& ancPath, std::string& err);

    static bool writeLeaf(MdbHandle* mdb, uint32_t leafPg, bool isLeaf,
                          const std::vector<std::vector<uint8_t>>& fullEntries,
                          std::string& err);

    static Status insertIntoPage(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                                 uint32_t pg, bool isLeaf,
                                 const std::vector<std::vector<uint8_t>>& merged,
                                 std::vector<uint32_t>& ancPath);

    /* Jackcess updateDataPage -> replaceParentEntry: propagate a changed
     * subtree max upward (base = [key+rowId] bytes, no child link).
     * Replaces the divider for childPg, or heals one in if the child is
     * divider-less (non-tail: truly missing). True tail children need
     * nothing (tail link routes them). Continues upward only while a
     * replaced/inserted divider becomes its page's max. */
    static Status propagateMaxChange(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                                     uint32_t childPg, const std::vector<uint8_t>& baseKeyRowId,
                                     std::vector<uint32_t>& ancPath);

    static Status splitPage(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                            uint32_t pg, bool wasLeaf,
                            const std::vector<std::vector<uint8_t>>& merged,
                            std::vector<uint32_t>& ancPath);

    static Status leafInsert(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                             const uint8_t* entry, size_t entryLen);

    static Status walkIndex(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx,
                            std::vector<IndexEntryInfo>& out);
};

} /* namespace Jet4Writer */
