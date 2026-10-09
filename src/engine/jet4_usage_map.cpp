#include "jet4_usage_map.h"
#include <cstdio>
#include <cstring>
#include <iostream>

extern "C" {
ssize_t mdb_write_pg(MdbHandle* mdb, unsigned long pg);
int mdb_find_row(MdbHandle* mdb, int row_number, int* row_start, size_t* row_size);
gint32 mdb_alloc_page(MdbTableDef* table);
void mdb_map_sync_to_disk(MdbTableDef* table);
}

namespace Jet4Writer {

bool UsageMapManager::readPage(MdbHandle* mdb, uint32_t pg) {
    if (!mdb) return false;
    // Bypass libmdb's pg_buf cache (mdb_read_pg returns the buffer as-is
    // when cur_pg matches): after libmdb direct-stream writes or our own
    // failed-then-dirty buffers, the cache can serve stale bytes that a
    // later writePage would persist. Always read physically; deterministic.
    mdb->cur_pg = 0;
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
    start &= 0x1FFF; /* Jackcess TableImpl.OFFSET_MASK: low 13 bits are the offset */
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
                                                   uint32_t newPg, std::string& err,
                                                   bool withNewPg) {
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

    /* 1. Allocate a new 0x05 UsageMap page via libmdb so file length,
     * maps and page count stay consistent. Repurposed as usage-map page. */
    gint32 newBmpSigned = allocateIndexPage(mdb, table);
    if (newBmpSigned <= 0) { err = "promoteMap: alloc bmp page failed"; return false; }
    const uint32_t bmpPg = static_cast<uint32_t>(newBmpSigned);

    if (!readPage(mdb, bmpPg)) { err = "promoteMap: read bmp page failed"; return false; }
    auto* bmpBuf = static_cast<uint8_t*>(mdb->pg_buf);
    std::memset(bmpBuf, 0, kPageSize);
    bmpBuf[0] = kPageUsageMap; /* 0x05 per Jackcess PageTypes.USAGE_MAP */
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

    /* Set the new page bit (skipped for pure migrations). */
    if (withNewPg) {
        const uint32_t off = newPg % ((kPageSize - 4) * 8);
        bmpBuf[4 + off / 8] |= static_cast<uint8_t>(1u << (off % 8));
    }
    if (!writePage(mdb, bmpPg)) { err = "promoteMap: write bmp page failed"; return false; }
    // Bitmap pages are not data: drop the data/free bits mdb_alloc_page set.
    clearDataMaps(mdb, table, bmpPg);

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
    // Verify the promotion stuck (read-back), then refresh libmdb's
    // in-memory map copies so later syncs cannot revert it.
    {
        std::vector<uint8_t> chk;
        int rs = 0;
        if (!readMapRow(mdb, mapPg, mapRow, chk, rs) || chk.empty() || chk[0] != kMapTypeReference) {
            err = "promoteMap: read-back mismatch"; return false;
        }
    }
    refreshTableMaps(mdb, table);
    return true;
}

bool UsageMapManager::refMapBitOp(MdbHandle* mdb, MdbTableDef* table,
                                   uint32_t mapPg, uint16_t mapRow, uint32_t pg, bool set) {
    std::vector<uint8_t> row;
    int rowStart = 0;
    if (!readMapRow(mdb, mapPg, mapRow, row, rowStart) || row.size() < 5 || row[0] != 1) {
        if (std::getenv("JET4_DEBUG"))
            fprintf(stderr, "[maps] refMapBitOp: def %u/%u unreadable type=%d size=%zu\n",
                    mapPg, mapRow, row.empty() ? -1 : (int)row[0], row.size());
        return false;
    }
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
        clearDataMaps(mdb, table, bmpPg);
        row[o] = static_cast<uint8_t>(bmpPg & 0xFF);
        row[o+1] = static_cast<uint8_t>((bmpPg >> 8) & 0xFF);
        row[o+2] = static_cast<uint8_t>((bmpPg >> 16) & 0xFF);
        row[o+3] = static_cast<uint8_t>((bmpPg >> 24) & 0xFF);
        if (!writeMapRow(mdb, mapPg, rowStart, row)) return false;
        refreshTableMaps(mdb, table);
    }
    if (!readPage(mdb, bmpPg)) return false;
    auto* b = static_cast<uint8_t*>(mdb->pg_buf);
    const uint32_t off = pg % bitlen;
    uint8_t& bb = b[4 + off / 8];
    const uint8_t bit = static_cast<uint8_t>(1u << (off % 8));
    if (set) {
        if (bb & bit) return true; // already set: avoid a redundant rewrite
        bb = static_cast<uint8_t>(bb | bit);
    } else {
        if (!(bb & bit)) return true;
        bb = static_cast<uint8_t>(bb & ~bit);
    }
    if (!writePage(mdb, bmpPg)) return false;
    return true;
}

bool UsageMapManager::refreshTableMaps(MdbHandle* mdb, MdbTableDef* table) {
    if (!mdb || !table || !table->entry || table->entry->table_pg <= 0 || !mdb->fmt)
        return false;
    const uint32_t tdefPg = static_cast<uint32_t>(table->entry->table_pg);
    if (!readPage(mdb, tdefPg)) return false;
    bool ok = true;
    // usage_map
    {
        const uint32_t pgrow = getU32(mdb, mdb->fmt->tab_usage_map_offset);
        const uint32_t mp = pgrow >> 8;
        const uint16_t mr = static_cast<uint16_t>(pgrow & 0xFF);
        std::vector<uint8_t> row;
        int rs = 0;
        if (mp && readMapRow(mdb, mp, mr, row, rs) && !row.empty() &&
            table->usage_map && row.size() <= table->map_sz) {
            std::memcpy(table->usage_map, row.data(), row.size());
        } else {
            ok = false;
        }
    }
    // free_usage_map
    {
        const uint32_t pgrow = getU32(mdb, mdb->fmt->tab_free_map_offset);
        const uint32_t mp = pgrow >> 8;
        const uint16_t mr = static_cast<uint16_t>(pgrow & 0xFF);
        std::vector<uint8_t> row;
        int rs = 0;
        if (mp && readMapRow(mdb, mp, mr, row, rs) && !row.empty() &&
            table->free_usage_map && row.size() <= table->freemap_sz) {
            std::memcpy(table->free_usage_map, row.data(), row.size());
        } else {
            ok = false;
        }
    }
    return ok;
}

Status UsageMapManager::ensureReferenceMaps(MdbHandle* mdb, MdbTableDef* table) {
    if (!mdb || !table || !table->entry || table->entry->table_pg <= 0 || !mdb->fmt)
        return Status::Fail("ensureReferenceMaps: null arg");
    const uint32_t tdefPg = static_cast<uint32_t>(table->entry->table_pg);
    struct DefLoc { uint32_t pg = 0; uint16_t row = 0; };
    auto readDefLoc = [&](int tdefOff, DefLoc& out) -> bool {
        if (!readPage(mdb, tdefPg)) return false;
        const uint32_t pgrow = getU32(mdb, tdefOff);
        out.pg = pgrow >> 8;
        out.row = static_cast<uint16_t>(pgrow & 0xFF);
        return out.pg > 0;
    };
    DefLoc dataLoc, freeLoc;
    if (!readDefLoc(mdb->fmt->tab_usage_map_offset, dataLoc))
        return Status::Fail("ensureReferenceMaps: no data map loc");
    if (!readDefLoc(mdb->fmt->tab_free_map_offset, freeLoc))
        return Status::Fail("ensureReferenceMaps: no free map loc");
    // Promote inline maps (one-time; reference maps cover 556k pages).
    for (int pass = 0; pass < 2; ++pass) {
        const DefLoc& loc = (pass == 0) ? dataLoc : freeLoc;
        std::vector<uint8_t> row;
        int rowStart = 0;
        if (!readMapRow(mdb, loc.pg, loc.row, row, rowStart) || row.empty())
            return Status::Fail("ensureReferenceMaps: map row unreadable");
        if (std::getenv("JET4_DEBUG"))
            fprintf(stderr, "[maps] %s map def %u/%u type=%d size=%zu\n",
                    pass == 0 ? "data" : "free", loc.pg, loc.row,
                    row.empty() ? -1 : (int)row[0], row.size());
        if (row[0] == kMapTypeInline) {
            std::string err;
            if (!promoteInlineMapToReference(mdb, table, loc.pg, loc.row, 0, err, false))
                return Status::Fail("ensureReferenceMaps: promote failed: " + err);
            if (std::getenv("JET4_DEBUG")) {
                std::vector<uint8_t> chk;
                int rs = 0;
                if (readMapRow(mdb, loc.pg, loc.row, chk, rs))
                    fprintf(stderr, "[maps] post-promote type=%d size=%zu\n",
                            chk.empty() ? -1 : (int)chk[0], chk.size());
            }
        } else if (row[0] != kMapTypeReference) {
            return Status::Fail("ensureReferenceMaps: unknown map type");
        }
    }
    // Bitmap pages must not linger in data maps (mismatched-type warnings
    // for map followers). Collect them from table + index map def rows.
    {
        std::vector<std::pair<uint32_t, uint16_t>> defs;
        auto defOf = [&](int tdefOff) {
            if (!readPage(mdb, tdefPg)) return;
            const uint32_t pgrow = getU32(mdb, tdefOff);
            if ((pgrow >> 8) > 0) defs.emplace_back(pgrow >> 8, (uint16_t)(pgrow & 0xFF));
        };
        defOf(mdb->fmt->tab_usage_map_offset);
        defOf(mdb->fmt->tab_free_map_offset);
        if (table->indices) {
            for (unsigned ii = 0; ii < table->indices->len; ++ii) {
                MdbIndex* mix = static_cast<MdbIndex*>(g_ptr_array_index(table->indices, ii));
                if (mix && mix->idx_map_pg > 0) defs.emplace_back((uint32_t)mix->idx_map_pg, mix->idx_map_row);
            }
        }
        for (auto& dl : defs) {
            std::vector<uint8_t> row;
            int rs0 = 0;
            if (!readMapRow(mdb, dl.first, dl.second, row, rs0)) continue;
            if (row.empty() || row[0] != kMapTypeReference || row.size() < 5) continue;
            const uint32_t maxPgs = static_cast<uint32_t>(row.size() - 1) / 4;
            for (uint32_t ind = 0; ind < maxPgs; ++ind) {
                const size_t o = 1 + ind * 4;
                const uint32_t bmpPg = static_cast<uint32_t>(row[o]) |
                    (static_cast<uint32_t>(row[o + 1]) << 8) |
                    (static_cast<uint32_t>(row[o + 2]) << 16) |
                    (static_cast<uint32_t>(row[o + 3]) << 24);
                if (bmpPg > 0) clearDataMaps(mdb, table, bmpPg);
            }
        }
    }
    return Status::Ok();
}

Status UsageMapManager::getOwnedPages(MdbHandle* mdb, MdbTableDef* table, std::vector<uint32_t>& outPages) {
    outPages.clear();
    if (!mdb || !table || !table->entry || table->entry->table_pg <= 0 || !mdb->fmt)
        return Status::Fail("getOwnedPages: null arg");
    const uint32_t tdefPg = static_cast<uint32_t>(table->entry->table_pg);
    if (!readPage(mdb, tdefPg)) return Status::Fail("getOwnedPages: read tdef failed");
    const uint32_t pgrow = getU32(mdb, mdb->fmt->tab_usage_map_offset);
    const uint32_t mapPg = pgrow >> 8;
    const uint16_t mapRow = static_cast<uint16_t>(pgrow & 0xFF);
    if (!mapPg) return Status::Fail("getOwnedPages: null map pointer");
    std::vector<uint8_t> row;
    int rowStart = 0;
    if (!readMapRow(mdb, mapPg, mapRow, row, rowStart) || row.empty())
        return Status::Fail("getOwnedPages: read map row failed");
    if (row[0] == kMapTypeInline) {
        if (row.size() < 5) return Status::Fail("getOwnedPages: inline map too short");
        const uint32_t start = static_cast<uint32_t>(row[1]) |
                               (static_cast<uint32_t>(row[2]) << 8) |
                               (static_cast<uint32_t>(row[3]) << 16) |
                               (static_cast<uint32_t>(row[4]) << 24);
        const size_t nbytes = row.size() - 5;
        for (size_t i = 0; i < nbytes; ++i) {
            uint8_t b = row[5 + i];
            if (!b) continue;
            for (int bit = 0; bit < 8; ++bit) {
                if ((b >> bit) & 1) {
                    outPages.push_back(start + static_cast<uint32_t>(i * 8 + bit));
                }
            }
        }
    } else if (row[0] == kMapTypeReference) {
        const uint32_t bitlen = (kPageSize - 4) * 8;
        const size_t maxInd = (row.size() - 1) / 4;
        for (size_t ind = 0; ind < maxInd; ++ind) {
            const size_t o = 1 + ind * 4;
            const uint32_t bmpPg = static_cast<uint32_t>(row[o]) |
                                   (static_cast<uint32_t>(row[o+1]) << 8) |
                                   (static_cast<uint32_t>(row[o+2]) << 16) |
                                   (static_cast<uint32_t>(row[o+3]) << 24);
            if (!bmpPg) continue;
            if (!readPage(mdb, bmpPg)) continue;
            const auto* b = static_cast<const uint8_t*>(mdb->pg_buf);
            for (uint32_t off = 0; off < bitlen; ++off) {
                if ((b[4 + off / 8] >> (off % 8)) & 1) {
                    outPages.push_back(static_cast<uint32_t>(ind * bitlen + off));
                }
            }
        }
    } else {
        return Status::Fail("getOwnedPages: unknown map type");
    }
    std::sort(outPages.begin(), outPages.end());
    return Status::Ok();
}

Status UsageMapManager::allocateDataPage(MdbHandle* mdb, MdbTableDef* table, uint32_t& newPg) {
    newPg = 0;
    if (!mdb || !table || !table->entry || table->entry->table_pg <= 0 || !mdb->f || !mdb->f->stream || !mdb->fmt)
        return Status::Fail("allocateDataPage: null arg");
    if (fseeko(mdb->f->stream, 0, SEEK_END) != 0) return Status::Fail("allocateDataPage: seek failed");
    const off_t curLen = ftello(mdb->f->stream);
    if (curLen < 0 || curLen % kPageSize != 0) return Status::Fail("allocateDataPage: bad file size");
    const uint32_t allocatedPg = static_cast<uint32_t>(curLen / kPageSize);

    // Format new data page
    std::vector<uint8_t> pageBuf(kPageSize, 0);
    pageBuf[0] = kPageData;
    pageBuf[1] = 0x01;
    const uint16_t initialFree = static_cast<uint16_t>(kPageSize - 14);
    pageBuf[2] = static_cast<uint8_t>(initialFree & 0xFF);
    pageBuf[3] = static_cast<uint8_t>((initialFree >> 8) & 0xFF);
    const uint32_t tdefPg = static_cast<uint32_t>(table->entry->table_pg);
    pageBuf[4] = static_cast<uint8_t>(tdefPg & 0xFF);
    pageBuf[5] = static_cast<uint8_t>((tdefPg >> 8) & 0xFF);
    pageBuf[6] = static_cast<uint8_t>((tdefPg >> 16) & 0xFF);
    pageBuf[7] = static_cast<uint8_t>((tdefPg >> 24) & 0xFF);
    pageBuf[12] = 0;
    pageBuf[13] = 0;

    fseeko(mdb->f->stream, curLen, SEEK_SET);
    if (fwrite(pageBuf.data(), 1, kPageSize, mdb->f->stream) != kPageSize)
        return Status::Fail("allocateDataPage: fwrite failed");
    fflush(mdb->f->stream);

    // Register in table's owned_pages map
    if (!readPage(mdb, tdefPg)) return Status::Fail("allocateDataPage: read tdef for owned map failed");
    const uint32_t pgrow = getU32(mdb, mdb->fmt->tab_usage_map_offset);
    const uint32_t dataMapPg = pgrow >> 8;
    const uint16_t dataMapRow = static_cast<uint16_t>(pgrow & 0xFF);
    std::vector<uint8_t> mapRowBytes;
    int rowStart = 0;
    if (readMapRow(mdb, dataMapPg, dataMapRow, mapRowBytes, rowStart) && !mapRowBytes.empty()) {
        if (mapRowBytes[0] == kMapTypeInline) {
            if (!mapBitOp(mdb, mapRowBytes, allocatedPg, true)) {
                std::string perr;
                if (!promoteInlineMapToReference(mdb, table, dataMapPg, dataMapRow, allocatedPg, perr, true)) {
                    return Status::Fail("allocateDataPage: promote data map failed: " + perr);
                }
            } else {
                writeMapRow(mdb, dataMapPg, rowStart, mapRowBytes);
            }
        } else if (mapRowBytes[0] == kMapTypeReference) {
            if (!refMapBitOp(mdb, table, dataMapPg, dataMapRow, allocatedPg, true)) {
                return Status::Fail("allocateDataPage: refMapBitOp data failed");
            }
        }
    } else {
        return Status::Fail("allocateDataPage: read owned map row failed");
    }

    // Register in table's free space map
    if (!readPage(mdb, tdefPg)) return Status::Fail("allocateDataPage: read tdef for free map failed");
    const uint32_t freePgrow = getU32(mdb, mdb->fmt->tab_free_map_offset);
    const uint32_t freeMapPg = freePgrow >> 8;
    const uint16_t freeMapRow = static_cast<uint16_t>(freePgrow & 0xFF);
    if (readMapRow(mdb, freeMapPg, freeMapRow, mapRowBytes, rowStart) && !mapRowBytes.empty()) {
        if (mapRowBytes[0] == kMapTypeInline) {
            if (!mapBitOp(mdb, mapRowBytes, allocatedPg, true)) {
                std::string perr;
                if (!promoteInlineMapToReference(mdb, table, freeMapPg, freeMapRow, allocatedPg, perr, true)) {
                    return Status::Fail("allocateDataPage: promote free map failed: " + perr);
                }
            } else {
                writeMapRow(mdb, freeMapPg, rowStart, mapRowBytes);
            }
        } else if (mapRowBytes[0] == kMapTypeReference) {
            if (!refMapBitOp(mdb, table, freeMapPg, freeMapRow, allocatedPg, true)) {
                return Status::Fail("allocateDataPage: refMapBitOp free failed");
            }
        }
    } else {
        return Status::Fail("allocateDataPage: read free map row failed");
    }

    refreshTableMaps(mdb, table);
    newPg = allocatedPg;
    return Status::Ok();
}

void UsageMapManager::clearDataMaps(MdbHandle* mdb, MdbTableDef* table, uint32_t newPg) {
    if (!mdb || !table || !table->usage_map || !table->free_usage_map) return;
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
                    if (std::memcmp(b + rowStart, row.data(), row.size()) != 0) {
                        std::memcpy(b + rowStart, row.data(), row.size());
                        if (!writePage(mdb, idx->idx_map_pg))
                            return Status::Fail("registerIndexPage: map write failed");
                    }
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
    clearDataMaps(mdb, table, newPg);
    return Status::Ok();
}

} /* namespace Jet4Writer */
