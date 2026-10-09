#include "jet4_table.h"
#include "jet4_usage_map.h"
#include "jet4_codes.h"
#include <cstdio>
#include <QString>

extern "C" {
int mdb_find_row(MdbHandle* mdb, int row_number, int* row_start, size_t* row_size);
}

namespace Jet4Writer {

QString decodeJet4QString(const uint8_t* data, size_t len) {
    if (!data || len == 0) return QString();
    // Jackcess ColumnImpl.decodeTextValue:
    // Check if starts with compressed unicode header (0xFF 0xFE)
    if (len >= 2 && data[0] == 0xFF && data[1] == 0xFE) {
        QString result;
        size_t i = 2;
        bool compressed = true;
        while (i < len) {
            if (data[i] == 0x00) {
                compressed = !compressed;
                ++i;
                continue;
            }
            if (compressed) {
                result += QChar(data[i]);
                ++i;
            } else {
                if (i + 1 < len) {
                    result += QChar(static_cast<ushort>(data[i] | (data[i + 1] << 8)));
                    i += 2;
                } else {
                    break;
                }
            }
        }
        return result;
    }
    // Standard UTF-16LE
    QString result;
    for (size_t b = 0; b + 1 < len; b += 2) {
        result += QChar(static_cast<ushort>(data[b] | (data[b + 1] << 8)));
    }
    return result;
}

Status TableSnapshot::snapshot(MdbHandle* mdb, MdbTableDef* table,
                               std::vector<RowSnapshot>& out) {
    out.clear();
    if (!mdb || !table || !table->entry || table->entry->table_pg <= 0 || !mdb->fmt)
        return Status::Fail("snapshot: null arg");
    if (!mdb->f || !mdb->f->stream) return Status::Fail("snapshot: no stream");
    const uint32_t tdefPg = static_cast<uint32_t>(table->entry->table_pg);
    const unsigned ncols = table->num_cols;
    if (ncols == 0 || ncols > 1000) return Status::Fail("snapshot: bad column count");

    // Strictly iterate owned pages from usage map (Jackcess TableImpl model)
    std::vector<uint32_t> ownedPages;
    Status ost = UsageMapManager::getOwnedPages(mdb, table, ownedPages);
    if (!ost.ok) return ost;

    std::vector<MdbField> fb(ncols);
    for (uint32_t p : ownedPages) {
        if (!UsageMapManager::readPage(mdb, p)) continue;
        const auto* b = static_cast<const uint8_t*>(mdb->pg_buf);
        if (b[0] != kPageData) continue;
        if (UsageMapManager::getU32(mdb, kOffTableDefPg) != tdefPg) continue;
        const int nrows = UsageMapManager::getU16(mdb, mdb->fmt->row_count_offset);
        if (nrows <= 0 || nrows > 1000) continue;
        for (int s = 0; s < nrows; ++s) {
            if (!UsageMapManager::readPage(mdb, p)) break;
            RowSnapshot row;
            row.pg = p;
            row.slot = s;
            int rs = 0;
            size_t rsz = 0;
            if (mdb_find_row(mdb, s, &rs, &rsz) != 0 || rsz == 0) continue;
            row.flags = rs & 0xC000;
            const auto* b2 = static_cast<const uint8_t*>(mdb->pg_buf);
            const size_t off = static_cast<size_t>(rs & 0x1FFF);
            if (off + rsz > kPageSize) continue;
            row.raw.assign(b2 + off, b2 + off + rsz);

            if ((row.flags & 0xC000) == 0x4000) {
                // Jackcess TableImpl.positionAtRowData: follow overflow pointer
                if (rsz >= 4) {
                    const uint8_t ovSlot = b2[off];
                    const uint32_t ovPg = static_cast<uint32_t>(b2[off + 1]) |
                                         (static_cast<uint32_t>(b2[off + 2]) << 8) |
                                         (static_cast<uint32_t>(b2[off + 3]) << 16);
                    if (ovPg > 0 && UsageMapManager::readPage(mdb, ovPg)) {
                        int ovRs = 0; size_t ovRsz = 0;
                        if (mdb_find_row(mdb, static_cast<int>(ovSlot), &ovRs, &ovRsz) == 0 && ovRsz > 0) {
                            const size_t ovOff = static_cast<size_t>(ovRs & 0x1FFF);
                            if (ovOff + ovRsz <= kPageSize && mdb_crack_row(table, ovOff, ovRsz, fb.data()) >= 0) {
                                row.colBytes.reserve(ncols);
                                row.fields.reserve(ncols);
                                for (unsigned i = 0; i < ncols; ++i) {
                                    const MdbColumn* c = ColumnEncoder::getColumn(table, (int)i);
                                    Field f;
                                    f.colnum = (int)i;
                                    f.isNull = fb[i].is_null != 0;
                                    f.isFixed = c ? c->is_fixed != 0 : true;
                                    f.data = nullptr;
                                    f.size = 0;
                                    if (!f.isNull && fb[i].value && fb[i].siz > 0) {
                                        row.colBytes.emplace_back((const uint8_t*)fb[i].value,
                                                                  (const uint8_t*)fb[i].value + fb[i].siz);
                                        f.data = row.colBytes.back().data();
                                        f.size = row.colBytes.back().size();
                                    }
                                    row.fields.push_back(f);
                                }
                                row.cracked = true;
                                out.push_back(std::move(row));
                                continue;
                            }
                        }
                    }
                }
                out.push_back(std::move(row));
                continue;
            }

            if (row.flags & 0x8000) {
                // Deleted row slot (0xC000) or overflow target (0x8000): not a live row header
                out.push_back(std::move(row));
                continue;
            }

            // Normal row crack
            if (mdb_crack_row(table, rs & 0x1FFF, rsz, fb.data()) < 0) {
                out.push_back(std::move(row));
                continue;
            }
            row.colBytes.reserve(ncols);
            row.fields.reserve(ncols);
            for (unsigned i = 0; i < ncols; ++i) {
                const MdbColumn* c = ColumnEncoder::getColumn(table, (int)i);
                Field f;
                f.colnum = (int)i;
                f.isNull = fb[i].is_null != 0;
                f.isFixed = c ? c->is_fixed != 0 : true;
                f.data = nullptr;
                f.size = 0;
                if (!f.isNull && fb[i].value && fb[i].siz > 0) {
                    row.colBytes.emplace_back((const uint8_t*)fb[i].value,
                                              (const uint8_t*)fb[i].value + fb[i].siz);
                    f.data = row.colBytes.back().data();
                    f.size = row.colBytes.back().size();
                }
                row.fields.push_back(f);
            }
            row.cracked = true;
            out.push_back(std::move(row));
        }
    }
    return Status::Ok();
}

} /* namespace Jet4Writer */
