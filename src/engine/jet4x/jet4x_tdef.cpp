/**
 * @file jet4x_tdef.cpp
 * @description jet4x table-definition writer implementation.
 * Layout mirrors jackcess writeTableDefinition for Jet4, corrected against
 * Bahi-Khata ground truth: shared var-slot counter (TEXT+LVAL consume in
 * column order, FIXED/BOOL peek), key flag ASC=0x01/DESC=0x00, per-index
 * cascade bytes, BOOL flags=0x03/len=1, order coldefs/names/idxdata/
 * idxinfo/idxnames/lvalrefs/trailer.
 */

#include "jet4x_tdef.h"

#include "jet4x_core.h"
#include "jet4x_layout.h"

namespace jet4x {

namespace {

void putU16(std::vector<uint8_t>& b, uint16_t v) {
    b.push_back((uint8_t)(v & 0xFF));
    b.push_back((uint8_t)((v >> 8) & 0xFF));
}
void putU32(std::vector<uint8_t>& b, uint32_t v) {
    b.push_back((uint8_t)(v & 0xFF));
    b.push_back((uint8_t)((v >> 8) & 0xFF));
    b.push_back((uint8_t)((v >> 16) & 0xFF));
    b.push_back((uint8_t)((v >> 24) & 0xFF));
}
void put3(std::vector<uint8_t>& b, uint32_t v) {
    b.push_back((uint8_t)(v & 0xFF));
    b.push_back((uint8_t)((v >> 8) & 0xFF));
    b.push_back((uint8_t)((v >> 16) & 0xFF));
}
/* writeName: u16 byte-length + UTF-16LE (jackcess TableImpl.writeName). */
void putName(std::vector<uint8_t>& b, const std::string& s) {
    std::vector<uint8_t> enc;
    enc.reserve(s.size() * 2);
    for (unsigned char c : s) {
        enc.push_back(c);
        enc.push_back(0);
    }
    putU16(b, (uint16_t)enc.size());
    b.insert(b.end(), enc.begin(), enc.end());
}

int colIndex(const TableSpec& spec, const std::string& name) {
    for (size_t i = 0; i < spec.columns.size(); i++) {
        if (spec.columns[i].name == name)
            return (int)i;
    }
    return -1;
}

} /* namespace */

bool buildTdef(const TableSpec& spec, uint32_t tdefPageNo, uint32_t umapPageNo,
               const std::vector<uint32_t>& indexRoots,
               const std::vector<LvalUmap>& lvalUmaps,
               std::vector<uint8_t>& out, std::string& error) {
    if (spec.columns.empty() || spec.columns.size() > 255) {
        error = "column count out of range for " + spec.name;
        return false;
    }
    if (spec.indexes.size() != indexRoots.size()) {
        error = "index root count mismatch for " + spec.name;
        return false;
    }
    size_t nLval = 0;
    for (const auto& c : spec.columns) {
        if (isLongValue(c.type))
            nLval++;
    }
    if (lvalUmaps.size() != nLval) {
        error = "lval umap count mismatch for " + spec.name;
        return false;
    }
    for (const auto& idx : spec.indexes) {
        if (idx.keys.empty() || idx.keys.size() > 10) {
            error = "index key count out of range in " + idx.name;
            return false;
        }
        for (const auto& k : idx.keys) {
            if (colIndex(spec, k.column) < 0) {
                error = "index " + idx.name + " refers to missing column " +
                        k.column;
                return false;
            }
        }
    }

    /* Canonical layout shared with the row packer (jet4x_layout.h). */
    const size_t n = spec.columns.size();
    TableLayout L = computeLayout(spec);
    uint16_t nVar = L.nVar;

    std::vector<uint8_t> b;
    b.reserve(8192);

    /* Page header: type TABLE_DEF, 0x01, free-space placeholder, next=0. */
    b.push_back(kPgTableDef);
    b.push_back(0x01);
    putU16(b, 0); /* patched by placeTdef */
    putU32(b, 0); /* next tdef page */

    /* Tdef header. */
    putU32(b, 0); /* total def length: patched below */
    putU32(b, kMagicTable);
    putU32(b, 0); /* row count */
    putU32(b, 0); /* last autonumber */
    b.push_back(0x01);
    for (int i = 0; i < 15; i++)
        b.push_back(0);
    b.push_back(0x4E); /* TYPE_USER */
    putU16(b, (uint16_t)n);
    putU16(b, nVar);
    putU16(b, (uint16_t)n);
    putU32(b, (uint32_t)spec.indexes.size());
    putU32(b, (uint32_t)spec.indexes.size());
    b.push_back(0);
    put3(b, umapPageNo);
    b.push_back(1);
    put3(b, umapPageNo);

    /* Index row-count blocks (12 zero bytes per slot for empty tables). */
    for (size_t i = 0; i < spec.indexes.size(); i++) {
        putU32(b, 0);
        putU32(b, 0);
        putU32(b, 0);
    }

    /* Column definitions (25 bytes each). */
    for (size_t i = 0; i < n; i++) {
        const ColumnSpec& c = spec.columns[i];
        int fs = fixedSize(c.type, c.length);
        b.push_back((uint8_t)c.type);
        putU32(b, kMagicTable);
        putU16(b, (uint16_t)i);
        putU16(b, L.cols[i].varOff);
        putU16(b, (uint16_t)i);
        putU32(b, kSortGeneral);
        uint8_t flags = kFlagUpdatable;
        if (fs > 0)
            flags |= kFlagFixedLen;
        if (c.type == DataType::Boolean)
            flags = (uint8_t)(kFlagUpdatable | kFlagFixedLen); /* 0x03 */
        if (c.autoNumber)
            flags |= 0x04;
        b.push_back(flags);
        b.push_back(c.extFlags);
        putU32(b, 0);
        putU16(b, fs > 0 ? L.cols[i].fixOff : 0);
        uint16_t len = 0;
        if (fs > 0)
            len = (uint16_t)fs;
        else if (fs == -1)
            len = c.length;
        else if (c.type == DataType::Boolean)
            len = 1;
        putU16(b, len);
    }

    /* Column names immediately follow the definitions. */
    for (size_t i = 0; i < n; i++)
        putName(b, spec.columns[i].name);

    /* Index-data definitions (4 + 30 + 18 = 52 bytes each).
     * Umap rows are lval-first: table owns 0,1; lval owned/free rows;
     * then index rows (Bahi-Khata/Access creation order). */
    size_t nLvalCols = 0;
    for (size_t c = 0; c < n; c++) {
        if (isLongValue(spec.columns[c].type))
            nLvalCols++;
    }
    for (size_t i = 0; i < spec.indexes.size(); i++) {
        const IndexSpec& idx = spec.indexes[i];
        putU32(b, kMagicIndex);
        for (int k = 0; k < 10; k++) {
            if (k < (int)idx.keys.size()) {
                int ci = colIndex(spec, idx.keys[k].column);
                putU16(b, (uint16_t)ci);
                b.push_back(idx.keys[k].descending ? 0x00 : 0x01);
            } else {
                putU16(b, 0xFFFF);
                b.push_back(0);
            }
        }
        b.push_back((uint8_t)(2 + 2 * nLvalCols + i));
        put3(b, umapPageNo);
        putU32(b, indexRoots[i]);
        putU32(b, 0);
        b.push_back(idx.flags);
        b.push_back(0);
        putU32(b, 0);
    }

    /* Index-info definitions (28 bytes each). */
    for (size_t i = 0; i < spec.indexes.size(); i++) {
        const IndexSpec& idx = spec.indexes[i];
        putU32(b, kMagicTable);
        putU32(b, idx.indexNumber);
        putU32(b, idx.indexDataNumber);
        b.push_back(0);
        putU32(b, 0xFFFFFFFF);
        putU32(b, 0);
        b.push_back(idx.cascUp);
        b.push_back(idx.cascDel);
        b.push_back(idx.type);
        putU32(b, 0);
    }

    /* Index names. */
    for (size_t i = 0; i < spec.indexes.size(); i++)
        putName(b, spec.indexes[i].name);

    /* LVAL column usage-map refs (10 bytes each, spec column order). */
    {
        size_t li = 0;
        for (size_t i = 0; i < n; i++) {
            if (!isLongValue(spec.columns[i].type))
                continue;
            const LvalUmap& u = lvalUmaps[li++];
            putU16(b, (uint16_t)i);
            b.push_back(u.ownedRow);
            put3(b, u.page);
            b.push_back(u.freeRow);
            put3(b, u.page);
        }
    }

    b.push_back(0xFF);
    b.push_back(0xFF);

    uint32_t total = (uint32_t)b.size();
    b[8] = (uint8_t)(total & 0xFF);
    b[9] = (uint8_t)((total >> 8) & 0xFF);
    b[10] = (uint8_t)((total >> 16) & 0xFF);
    b[11] = (uint8_t)((total >> 24) & 0xFF);

    out = std::move(b);
    return true;
}

std::vector<uint32_t> placeTdef(DatabaseImage& img, uint32_t tdefPageNo,
                                const std::vector<uint8_t>& stream) {
    /* The stream already contains page 0's 8-byte header image (free space
     * @2 and next-page @4 patched below); continuation pages get fresh
     * headers (jackcess writeTableDefinitionBuffer). */
    std::vector<uint32_t> pages;
    pages.push_back(tdefPageNo);
    size_t pos = 0;
    {
        uint8_t* pg = img.page(tdefPageNo);
        size_t take = stream.size() < kPageSize ? stream.size() : kPageSize;
        memcpy(pg, stream.data(), take);
        if (take < kPageSize)
            memset(pg + take, 0, kPageSize - take);
        pos = take;
        if (pos >= stream.size()) {
            uint32_t total = (uint32_t)stream.size();
            uint16_t free =
                (uint16_t)(total + 8 >= kPageSize ? 0 : kPageSize - total - 8);
            pg[2] = (uint8_t)(free & 0xFF);
            pg[3] = (uint8_t)((free >> 8) & 0xFF);
            return pages;
        }
        pg[2] = 0;
        pg[3] = 0; /* full first page: no free space */
    }
    uint32_t prev = tdefPageNo;
    while (pos < stream.size()) {
        uint32_t cur = img.allocPage(kPgTableDef);
        uint8_t* pg = img.page(cur);
        memset(pg, 0, kPageSize);
        pg[0] = kPgTableDef;
        pg[1] = 0x01;
        size_t take = stream.size() - pos;
        if (take > kPageSize - 8)
            take = kPageSize - 8;
        memcpy(pg + 8, stream.data() + pos, take);
        pos += take;
        size_t rem = kPageSize - (8 + take);
        uint16_t free = (uint16_t)(rem > 8 ? rem - 8 : 0);
        pg[2] = (uint8_t)(free & 0xFF);
        pg[3] = (uint8_t)((free >> 8) & 0xFF);
        /* Link previous page -> cur via next-pointer @4. */
        uint8_t* pp = img.page(prev);
        pp[4] = (uint8_t)(cur & 0xFF);
        pp[5] = (uint8_t)((cur >> 8) & 0xFF);
        pp[6] = (uint8_t)((cur >> 16) & 0xFF);
        pp[7] = (uint8_t)((cur >> 24) & 0xFF);
        pages.push_back(cur);
        prev = cur;
    }
    return pages;
}

} /* namespace jet4x */
