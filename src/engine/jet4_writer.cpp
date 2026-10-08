#include "jet4_writer.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern "C" {
int mdb_find_row(MdbHandle* mdb, int row_number, int* row_start, size_t* row_size);
}

namespace Jet4Writer {

/* ============================ packRow ============================ */
Status packRow(MdbTableDef* table, const Field* fields, size_t nfields,
               std::vector<uint8_t>& out) {
    if (!table || !table->columns) return Status::Fail("packRow: null table");
    const unsigned numCols = table->num_cols;
    if (numCols == 0 || numCols > 1000) return Status::Fail("packRow: bad num_cols");

    int maxFixed = 0;
    for (unsigned i = 0; i < numCols; ++i) {
        const MdbColumn* c = ColumnEncoder::getColumn(table, static_cast<int>(i));
        if (!c) return Status::Fail("packRow: missing column def");
        if (c->is_fixed) {
            if (c->fixed_offset < 0 || c->col_size <= 0 || c->col_size > 512)
                return Status::Fail("packRow: bad fixed geometry");
            maxFixed = std::max(maxFixed, c->fixed_offset + c->col_size);
        }
    }

    std::vector<uint8_t> body;
    body.reserve(1024);
    body.push_back(static_cast<uint8_t>(numCols & 0xFF));
    body.push_back(static_cast<uint8_t>((numCols >> 8) & 0xFF));
    const size_t fixedStart = body.size();
    body.insert(body.end(), static_cast<size_t>(maxFixed), 0);

    for (size_t i = 0; i < nfields; ++i) {
        const Field& f = fields[i];
        const MdbColumn* c = ColumnEncoder::getColumn(table, f.colnum);
        if (!c || !c->is_fixed || f.isNull || !f.data || f.size == 0) continue;
        const size_t n = std::min(f.size, static_cast<size_t>(c->col_size));
        const size_t at = fixedStart + static_cast<size_t>(c->fixed_offset);
        if (at + n > body.size()) return Status::Fail("packRow: fixed OOB");
        std::memcpy(body.data() + at, f.data, n);
    }

    const unsigned numVar = table->num_var_cols;
    std::vector<const Field*> byVar(numVar, nullptr);
    for (size_t i = 0; i < nfields; ++i) {
        const Field& f = fields[i];
        const MdbColumn* c = ColumnEncoder::getColumn(table, f.colnum);
        if (!c || c->is_fixed) continue;
        if (static_cast<unsigned>(c->var_col_num) < numVar && !byVar[static_cast<size_t>(c->var_col_num)])
            byVar[static_cast<size_t>(c->var_col_num)] = &f;
    }
    if (numVar > 0) {
        std::vector<uint32_t> varOff(numVar + 1, 0);
        for (unsigned v = 0; v < numVar; ++v) {
            varOff[v] = static_cast<uint32_t>(body.size());
            const Field* f = byVar[v];
            if (f && !f->isNull && f->data && f->size > 0) {
                if (body.size() + f->size > 4000) return Status::Fail("packRow: row too big");
                body.insert(body.end(), f->data, f->data + f->size);
            }
        }
        varOff[numVar] = static_cast<uint32_t>(body.size());
        for (uint32_t o : varOff) {
            if (o > 0xFFFF) return Status::Fail("packRow: var offset overflow");
        }
        const uint16_t eod = static_cast<uint16_t>(body.size());
        body.push_back(static_cast<uint8_t>(eod & 0xFF));
        body.push_back(static_cast<uint8_t>((eod >> 8) & 0xFF));
        for (int v = static_cast<int>(numVar) - 1; v >= 0; --v) {
            const uint16_t o = static_cast<uint16_t>(varOff[static_cast<size_t>(v)]);
            body.push_back(static_cast<uint8_t>(o & 0xFF));
            body.push_back(static_cast<uint8_t>((o >> 8) & 0xFF));
        }
        body.push_back(static_cast<uint8_t>(numVar & 0xFF));
        body.push_back(static_cast<uint8_t>((numVar >> 8) & 0xFF));
    }

    // Null mask: bits indexed by col->col_num (1 = NOT NULL), zero-initialised.
    // Columns absent from `fields` stay NULL. Matches mdb_crack_row() logic.
    const size_t maskBytes = (numCols + 7) / 8;
    std::vector<uint8_t> nullMask(maskBytes, 0);
    for (size_t i = 0; i < nfields; ++i) {
        const Field& f = fields[i];
        if (f.isNull) continue;
        const MdbColumn* c = ColumnEncoder::getColumn(table, f.colnum);
        if (!c) continue;
        const int cn = c->col_num;
        if (cn < 0 || static_cast<size_t>(cn / 8) >= maskBytes)
            return Status::Fail("packRow: col_num OOB");
        nullMask[static_cast<size_t>(cn / 8)] |= static_cast<uint8_t>(1u << (cn % 8));
    }
    body.insert(body.end(), nullMask.begin(), nullMask.end());
    if (body.size() > 4000) return Status::Fail("packRow: row too big");
    out = std::move(body);
    return Status::Ok();
}

/* ============================ updateIndex ============================ */
int updateIndex(MdbTableDef* table, MdbIndex* idx,
                const Field* fields, size_t nfields,
                uint32_t dataPg, uint16_t rownum, std::string& error) {
    if (!table || !table->entry || !table->entry->mdb || !idx) {
        error = "updateIndex: null arg"; return 0;
    }
    if (idx->first_pg == 0 || idx->index_type == 2) {
        if (std::getenv("JET4_DEBUG")) fprintf(stderr, "[jet4] updateIndex skip %s first_pg=%u type=%d\n",
                                               idx->name, idx->first_pg, (int)idx->index_type);
        return 0;
    }
    MdbHandle* mdb = table->entry->mdb;
    std::vector<uint8_t> entry;
    Status st = BTreeEngine::buildEntry(table, idx, fields, nfields, dataPg, rownum, entry);
    if (!st.ok) {
        error = st.error;
        if (std::getenv("JET4_DEBUG")) fprintf(stderr, "[jet4] updateIndex %s build failed: %s\n", idx->name, st.error.c_str());
        return 0;
    }
    st = BTreeEngine::leafInsert(mdb, table, idx, entry.data(), entry.size());
    if (!st.ok) {
        error = st.error;
        if (std::getenv("JET4_DEBUG")) fprintf(stderr, "[jet4] updateIndex %s insert failed: %s\n", idx->name, st.error.c_str());
        return 0;
    }
    idx->num_rows++;
    /* Persist index cardinality on the table definition page (legacy behavior). */
    if (table->entry->table_pg > 0) {
        const int off = mdb->fmt->tab_cols_start_offset +
                        idx->index_num * mdb->fmt->tab_ridx_entry_size;
        if (UsageMapManager::readPage(mdb, static_cast<uint32_t>(table->entry->table_pg))) {
            mdb_put_int32(mdb->pg_buf, off, static_cast<guint32>(idx->num_rows));
            UsageMapManager::writePage(mdb, static_cast<uint32_t>(table->entry->table_pg));
        }
    }
    return static_cast<int>(entry.size());
}

/* ============================ verifyDatabase ============================ */
VerifyReport verifyDatabase(MdbHandle* mdb) {
    VerifyReport rep;
    if (!mdb) { rep.error = "null handle"; return rep; }
    const char* tables[] = {"Transactions", "Ledgers", "Groups", "CompanyInfo",
                              "StockTransactions", "SaleTransportationDetail", nullptr};
    for (int ti = 0; tables[ti]; ++ti) {
        MdbTableDef* t = mdb_read_table_by_name(mdb, const_cast<char*>(tables[ti]), MDB_TABLE);
        if (!t) { rep.error = std::string("missing table ") + tables[ti]; return rep; }
        mdb_read_columns(t);
        mdb_read_indices(t);
        ++rep.tablesChecked;

        long cracked = 0;
        {
            std::vector<MdbField> fb(t->num_cols ? t->num_cols : 1);
            mdb_rewind_table(t);
            while (mdb_fetch_row(t)) {
                int rs = 0;
                size_t rsz = 0;
                mdb_find_row(mdb, static_cast<int>(t->cur_row) - 1, &rs, &rsz);
                const int n = mdb_crack_row(t, rs & 0x0FFF, rsz, fb.data());
                if (n < 0) ++rep.badRows;
                else ++cracked;
            }
        }
        rep.dataRows += cracked;

        if (t->indices) {
            for (unsigned ii = 0; ii < t->indices->len; ++ii) {
                MdbIndex* idx = static_cast<MdbIndex*>(g_ptr_array_index(t->indices, ii));
                VerifyIndexInfo vi;
                vi.table = tables[ti];
                vi.name = idx->name[0] ? idx->name : "?";
                vi.catalogRows = idx->num_rows;
                vi.dataRows = cracked;
                std::vector<IndexEntryInfo> entries;
                Status st = BTreeEngine::walkIndex(mdb, t, idx, entries);
                if (!st.ok) {
                    mdb_free_tabledef(t);
                    rep.error = std::string("walk failed ") + vi.table + "." + vi.name + ": " + st.error;
                    return rep;
                }
                vi.walked = static_cast<long>(entries.size());
                vi.sorted = true;
                for (size_t i = 1; i < entries.size(); ++i) {
                    const auto& A = entries[i - 1];
                    const auto& B = entries[i];
                    const size_t m = std::min(A.fullKey.size(), B.fullKey.size());
                    int c = m ? std::memcmp(A.fullKey.data(), B.fullKey.data(), m) : 0;
                    if (c == 0) c = (A.fullKey.size() < B.fullKey.size()) ? -1 : (A.fullKey.size() > B.fullKey.size() ? 1 : 0);
                    // Jackcess Entry.compareTo: rowId breaks key ties.
                    if (c == 0 && A.dataPg != B.dataPg) c = (A.dataPg < B.dataPg) ? -1 : 1;
                    if (c == 0 && A.rowIdx != B.rowIdx) c = (A.rowIdx < B.rowIdx) ? -1 : 1;
                    if (c > 0) { vi.sorted = false; break; }
                }
                rep.indexes.push_back(std::move(vi));
            }
        }
        mdb_free_tabledef(t);
    }
    rep.ok = (rep.badRows == 0);
    for (const auto& vi : rep.indexes) {
        if (!vi.sorted || vi.walked != vi.dataRows) { rep.ok = false; break; }
    }
    return rep;
}

} /* namespace Jet4Writer */

/* ============================ C ABI ============================ */
static const uint8_t kEmptySentinel = 0;
static Jet4Writer::Field toJetField(const MdbField& m) {
    Jet4Writer::Field f;
    f.colnum = m.colnum;
    f.isNull = true;
    f.data = nullptr;
    f.size = 0;
    if (m.is_null != 0) return f;
    if (m.value == nullptr) {
        if (m.siz > 0) return f;
        f.isNull = false;
        f.data = &kEmptySentinel;
        return f;
    }
    f.isNull = false;
    f.data = static_cast<const uint8_t*>(m.value);
    f.size = m.siz > 0 ? static_cast<size_t>(m.siz) : 0;
    return f;
}

extern "C" int jet4_pack_row(MdbTableDef* table, MdbField* fields, unsigned num_fields,
                             unsigned char* out, unsigned out_size) {
    if (!table || !fields || !out || !out_size) return -1;
    std::vector<Jet4Writer::Field> fs;
    fs.reserve(num_fields);
    for (unsigned i = 0; i < num_fields; ++i) fs.push_back(toJetField(fields[i]));
    std::vector<uint8_t> packed;
    Jet4Writer::Status st = Jet4Writer::packRow(table, fs.data(), fs.size(), packed);
    if (!st.ok || packed.size() > out_size) return -1;
    std::memcpy(out, packed.data(), packed.size());
    return static_cast<int>(packed.size());
}

extern "C" int jet4_update_index(MdbTableDef* table, MdbIndex* idx, MdbField* fields,
                                unsigned num_fields, unsigned data_pg, unsigned rownum) {
    if (!table || !idx || !fields) return 0;
    std::vector<Jet4Writer::Field> fs;
    fs.reserve(num_fields);
    for (unsigned i = 0; i < num_fields; ++i) fs.push_back(toJetField(fields[i]));
    std::string err;
    return Jet4Writer::updateIndex(table, idx, fs.data(), fs.size(), data_pg,
                                  static_cast<uint16_t>(rownum), err);
}
