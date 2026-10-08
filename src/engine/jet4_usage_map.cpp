#include "jet4_usage_map.h"
#include <cstdio>
#include <iostream>

extern "C" {
ssize_t mdb_write_pg(MdbHandle* mdb, unsigned long pg);
int mdb_find_row(MdbHandle* mdb, int row_number, int* row_start, size_t* row_size);
gint32 mdb_alloc_page(MdbTableDef* table);
void mdb_map_sync_to_disk(MdbTableDef* table);
}

namespace Jet4Writer {

bool UsageMapManager::readPage(MdbHandle* mdb, uint32_t pg) {
    return mdb_read_pg(mdb, pg) != 0;
}

bool UsageMapManager::writePage(MdbHandle* mdb, uint32_t pg) {
    return mdb_write_pg(mdb, pg) != 0;
}

uint16_t UsageMapManager::getU16(MdbHandle* mdb, int offset) {
    return static_cast<uint16_t>(mdb_get_int16(mdb->pg_buf, offset));
}

uint32_t UsageMapManager::getU32(MdbHandle* mdb, int offset) {
    return static_cast<uint32_t>(mdb_get_int32(mdb->pg_buf, offset));
}

void UsageMapManager::putU16(MdbHandle* mdb, int offset, uint16_t val) {
    mdb_put_int16(mdb->pg_buf, offset, static_cast<guint16>(val));
}

void UsageMapManager::putU32(MdbHandle* mdb, int offset, uint32_t val) {
    mdb_put_int32(mdb->pg_buf, offset, static_cast<guint32>(val));
}

/* Allocates a new global database page for an INDEX node or leaf via
 * libmdb (creates a DATA page + sets data-map bits). Caller must follow
 * with registerIndexPage() which moves ownership to the index map and
 * clears the data-map bits (native: index pages live only in index maps).
 * Mirrors Jackcess IndexData.addOwnedPage + old proven jet4_writer. */
gint32 UsageMapManager::allocateIndexPage(MdbHandle* mdb, MdbTableDef* table) {
    (void)mdb;
    if (!table) return 0;
    return mdb_alloc_page(table);
}

bool UsageMapManager::readMapRow(MdbHandle* mdb, uint32_t mapPg, uint16_t mapRow,
                                 std::vector<uint8_t>& rowBytes, int& rowStart) {
    if (!mapPg) return false;
    if (!readPage(mdb, mapPg)) return false;
    int start = 0;
    size_t sz = 0;
    if (mdb_find_row(mdb, mapRow, &start, &sz) != 0) return false;
    start &= 0x0FFF;
    if (start < 0 || sz == 0 || sz > 4096 || start + static_cast<int>(sz) > kPageSize) return false;
    const auto* b = static_cast<const uint8_t*>(mdb->pg_buf);
    rowBytes.assign(b + start, b + start + sz);
    rowStart = start;
    return true;
}

bool UsageMapManager::writeMapRow(MdbHandle* mdb, uint32_t mapPg, int rowStart,
                                  const std::vector<uint8_t>& rowBytes) {
    if (!readPage(mdb, mapPg)) return false;
    auto* b = static_cast<uint8_t*>(mdb->pg_buf);
    if (rowStart < 0 || rowStart + static_cast<int>(rowBytes.size()) > kPageSize) return false;
    std::memcpy(b + rowStart, rowBytes.data(), rowBytes.size());
    return writePage(mdb, mapPg);
}

bool UsageMapManager::mapBitOp(MdbHandle* mdb, std::vector<uint8_t>& row, uint32_t pg, bool set) {
    (void)mdb;
    if (row.empty()) return false;
    if (row[0] == kMapTypeInline) {
        if (row.size() < 5) return false;
        const uint32_t start = static_cast<uint32_t>(row[1]) |
                               (static_cast<uint32_t>(row[2]) << 8) |
                               (static_cast<uint32_t>(row[3]) << 16) |
                               (static_cast<uint32_t>(row[4]) << 24);
        if (pg < start) return false;
        const uint32_t bi = pg - start;
        if (bi >= (row.size() - 5) * 8) return false;
        uint8_t& b = row[5 + bi / 8];
        b = set ? static_cast<uint8_t>(b | (1u << (bi % 8)))
                : static_cast<uint8_t>(b & ~(1u << (bi % 8)));
        return true;
    }
    return false;
}

/* Promotes an inline (Type 0) usage map to a reference (Type 1) usage map
 * when an allocated page exceeds the inline map range.
 * Mirrors Jackcess UsageMap.promoteInlineHandlerToReferenceHandler. */
bool UsageMapManager::promoteInlineMapToReference(MdbHandle* mdb, MdbTableDef* table,
                                                   uint32_t mapPg, uint16_t mapRow,
                                                   uint32_t newPg, std::string& err) {
    std::vector<uint8_t> oldRow;
    int rowStart = 0;
    if (!readMapRow(mdb, mapPg, mapRow, oldRow, rowStart) || oldRow.size() < 5 || oldRow[0] != 0) {
        err = "promoteMap: read old row failed"; return false;
    }
    const uint32_t oldStart = static_cast<uint32_t>(oldRow[1]) |
                              (static_cast<uint32_t>(oldRow[2]) << 8) |
                              (static_cast<uint32_t>(oldRow[3]) << 16) |
                              (static_cast<uint32_t>(oldRow[4]) << 24);
    const size_t oldMapBytes = oldRow.size() - 5;

    /* 1. Allocate a new 0x02 UsageMap page via libmdb so file length,
     * maps and page count stay consistent. Repurposed as usage-map page. */
    gint32 newBmpSigned = allocateIndexPage(mdb, table);
    if (newBmpSigned <= 0) { err = "promoteMap: alloc bmp page failed"; return false; }
    const uint32_t bmpPg = static_cast<uint32_t>(newBmpSigned);

    if (!readPage(mdb, bmpPg)) { err = "promoteMap: read bmp page failed"; return false; }
    auto* bmpBuf = static_cast<uint8_t*>(mdb->pg_buf);
    std::memset(bmpBuf, 0, kPageSize);
    bmpBuf[0] = kPageUsageMap; /* 0x02 */
    bmpBuf[1] = 0x01;

    /* 2. Migrate existing inline bits to new bitmap page */
    for (size_t i = 0; i < oldMapBytes; ++i) {
        uint8_t b = oldRow[5 + i];
        if (b == 0) continue;
        for (int bit = 0; bit < 8; ++bit) {
            if (b & (1 << bit)) {
                uint32_t p = oldStart + static_cast<uint32_t>(i * 8 + bit);
                const uint32_t off = p % ((kPageSize - 4) * 8);
                bmpBuf[4 + off / 8] |= static_cast<uint8_t>(1u << (off % 8));
            }
        }
    }

    /* Set the new page bit */
    {
        const uint32_t off = newPg % ((kPageSize - 4) * 8);
        bmpBuf[4 + off / 8] |= static_cast<uint8_t>(1u << (off % 8));
    }
    if (!writePage(mdb, bmpPg)) { err = "promoteMap: write bmp page failed"; return false; }

    /* 3. Convert definition row from Type 0 to Type 1 */
    std::vector<uint8_t> newRow(oldRow.size(), 0);
    newRow[0] = kMapTypeReference; /* 1 */
    newRow[1] = static_cast<uint8_t>(bmpPg & 0xFF);
    newRow[2] = static_cast<uint8_t>((bmpPg >> 8) & 0xFF);
    newRow[3] = static_cast<uint8_t>((bmpPg >> 16) & 0xFF);
    newRow[4] = static_cast<uint8_t>((bmpPg >> 24) & 0xFF);

    if (!writeMapRow(mdb, mapPg, rowStart, newRow)) {
        err = "promoteMap: write new ref row failed"; return false;
    }
    return true;
}

bool UsageMapManager::refMapBitOp(MdbHandle* mdb, MdbTableDef* table,
                                   uint32_t mapPg, uint16_t mapRow, uint32_t pg, bool set) {
    std::vector<uint8_t> row;
    int rowStart = 0;
    if (!readMapRow(mdb, mapPg, mapRow, row, rowStart) || row.size() < 5 || row[0] != 1) return false;
    const uint32_t bitlen = (kPageSize - 4) * 8;
    const uint32_t maxPgs = static_cast<uint32_t>(row.size() - 1) / 4;
    const uint32_t ind = pg / bitlen;
    if (ind >= maxPgs) return false;
    const size_t o = 1 + ind * 4;
    uint32_t bmpPg = static_cast<uint32_t>(row[o]) |
                     (static_cast<uint32_t>(row[o+1]) << 8) |
                     (static_cast<uint32_t>(row[o+2]) << 16) |
                     (static_cast<uint32_t>(row[o+3]) << 24);
    if (!bmpPg) {
        // Jackcess ReferenceHandler: lazily allocate a new 0x02 usage-map page.
        if (!set) return true;
        gint32 newBmpSigned = allocateIndexPage(mdb, table);
        if (newBmpSigned <= 0) return false;
        bmpPg = static_cast<uint32_t>(newBmpSigned);
        if (!readPage(mdb, bmpPg)) return false;
        auto* b = static_cast<uint8_t*>(mdb->pg_buf);
        std::memset(b, 0, kPageSize);
        b[0] = kPageUsageMap; b[1] = 0x01;
        if (!writePage(mdb, bmpPg)) return false;
        row[o] = static_cast<uint8_t>(bmpPg & 0xFF);
        row[o+1] = static_cast<uint8_t>((bmpPg >> 8) & 0xFF);
        row[o+2] = static_cast<uint8_t>((bmpPg >> 16) & 0xFF);
        row[o+3] = static_cast<uint8_t>((bmpPg >> 24) & 0xFF);
        if (!writeMapRow(mdb, mapPg, rowStart, row)) return false;
    }
    if (!readPage(mdb, bmpPg)) return false;
    auto* b = static_cast<uint8_t*>(mdb->pg_buf);
    const uint32_t off = pg % bitlen;
    uint8_t& bb = b[4 + off / 8];
    bb = set ? static_cast<uint8_t>(bb | (1u << (off % 8)))
             : static_cast<uint8_t>(bb & ~(1u << (off % 8)));
    if (!writePage(mdb, bmpPg)) return false;
    return true;
}

Status UsageMapManager::registerIndexPage(MdbHandle* mdb, MdbTableDef* table, MdbIndex* idx, uint32_t newPg) {
    if (idx->idx_map_pg != 0) {
        std::vector<uint8_t> row;
        int rowStart = 0;
        if (!readMapRow(mdb, idx->idx_map_pg, idx->idx_map_row, row, rowStart))
            return Status::Fail("registerIndexPage: map row unreadable");
        bool ok = false;
        if (!row.empty() && row[0] == kMapTypeInline) {
            ok = mapBitOp(mdb, row, newPg, true);
            if (ok) {
                if (!readPage(mdb, idx->idx_map_pg))
                    return Status::Fail("registerIndexPage: map reread failed");
                auto* b = static_cast<uint8_t*>(mdb->pg_buf);
                if (rowStart + static_cast<int>(row.size()) <= kPageSize) {
                    std::memcpy(b + rowStart, row.data(), row.size());
                    if (!writePage(mdb, idx->idx_map_pg))
                        return Status::Fail("registerIndexPage: map write failed");
                } else {
                    ok = false;
                }
            } else {
                /* Out of range for inline Type 0 -> Promote to reference Type 1
                 * (Jackcess UsageMap.promoteInlineHandlerToReferenceHandler). */
                std::string err;
                ok = promoteInlineMapToReference(mdb, table, idx->idx_map_pg, idx->idx_map_row, newPg, err);
            }
        } else if (!row.empty() && row[0] == kMapTypeReference) {
            ok = refMapBitOp(mdb, table, idx->idx_map_pg, idx->idx_map_row, newPg, true);
        }
        if (!ok) return Status::Fail("registerIndexPage: bit set failed");
    }
    /* Remove from table data/free maps (native index pages are absent there);
     * mdb_alloc_page() had set these bits. Mirrors proven jet4_writer. */
    if (table && table->usage_map && table->free_usage_map) {
        auto clearIn = [&](unsigned char* map, unsigned sz) {
            if (!map || sz < 5) return;
            if (map[0] == 0) {
                const uint32_t start = static_cast<uint32_t>(mdb_get_int32(map, 1));
                if (newPg >= start) {
                    const uint32_t bi = newPg - start;
                    if (bi < (sz - 5) * 8) map[5 + bi / 8] &= static_cast<unsigned char>(~(1u << (bi % 8)));
                }
            } else if (map[0] == 1) {
                const uint32_t bitlen = static_cast<uint32_t>((kPageSize - 4) * 8);
                const uint32_t maxPgs = (sz - 1) / 4;
                const uint32_t ind = newPg / bitlen;
                if (ind < maxPgs) {
                    const size_t o = 1 + ind * 4;
                    const uint32_t bmpPg = static_cast<uint32_t>(map[o]) |
                        (static_cast<uint32_t>(map[o+1]) << 8) |
                        (static_cast<uint32_t>(map[o+2]) << 16) |
                        (static_cast<uint32_t>(map[o+3]) << 24);
                    if (bmpPg && readPage(mdb, bmpPg)) {
                        auto* b = static_cast<uint8_t*>(mdb->pg_buf);
                        const uint32_t off = newPg % bitlen;
                        b[4 + off / 8] &= static_cast<uint8_t>(~(1u << (off % 8)));
                        writePage(mdb, bmpPg);
                    }
                }
            }
        };
        clearIn(table->usage_map, table->map_sz);
        clearIn(table->free_usage_map, table->freemap_sz);
        mdb_map_sync_to_disk(table);
    }
    return Status::Ok();
}

} /* namespace Jet4Writer */
