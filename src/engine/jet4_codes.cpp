#include "jet4_codes.h"

extern "C" {
extern char idx_to_text_ling[];
}

namespace Jet4Writer {

/* Jet 4 text sort-order map: use libmdb's proven idx_to_text_ling
 * (third_party/libmdb/index.c), the same table the pre-regression engine
 * used. Verified byte-identical against pristine native keys (--audit-keys).
 * Do NOT substitute a different linguistic table: every TEXT index key
 * byte must match native Jet or VB6 seeks miss the rows (blank Day Book). */

const MdbColumn* ColumnEncoder::getColumn(const MdbTableDef* table, int colnum) {
    if (!table || !table->columns) return nullptr;
    if (colnum < 0 || colnum >= static_cast<int>(table->num_cols)) return nullptr;
    return static_cast<const MdbColumn*>(g_ptr_array_index(table->columns, static_cast<guint>(colnum)));
}

const Field* ColumnEncoder::findField(const Field* fields, size_t nfields, int colnum) {
    for (size_t i = 0; i < nfields; ++i) {
        if (fields[i].colnum == colnum) return &fields[i];
    }
    return nullptr;
}

/* Jackcess IntegerColumnDescriptor: big-endian payload, flipFirstBit,
 * then flipBytes when descending. */
void ColumnEncoder::encodeInteger(const uint8_t* leData, size_t numBytes, bool isDesc, ByteVec& out) {
    std::vector<uint8_t> be(numBytes);
    for (size_t i = 0; i < numBytes; ++i) be[i] = leData[numBytes - 1 - i];
    be[0] ^= 0x80;
    if (isDesc) {
        for (auto& b : be) b = static_cast<uint8_t>(~b);
    }
    out.putBytes(be.data(), be.size());
}

/* Jackcess FloatingPointColumnDescriptor, transcribed exactly:
 *   if (!isNegative) flipFirstBit;
 *   if (isNegative == isAscending()) flipBytes;   (isAscending == !desc) */
void ColumnEncoder::encodeDouble(const uint8_t* leData8, bool isDesc, ByteVec& out) {
    uint8_t be[8];
    for (int i = 0; i < 8; ++i) be[i] = leData8[7 - i];
    const bool neg = (be[0] & 0x80) != 0;
    if (!neg) be[0] ^= 0x80;
    if (neg == !isDesc) {
        for (int i = 0; i < 8; ++i) be[i] = static_cast<uint8_t>(~be[i]);
    }
    out.putBytes(be, 8);
}

void ColumnEncoder::encodeText(const uint8_t* data, size_t len, bool isDesc, ByteVec& out) {
    if (len >= 2 && data[0] == 0xFF && data[1] == 0xFE) {
        /* Compressed single bytes */
        for (size_t b = 2; b < len; ++b) {
            if (data[b] == 0) break;
            const uint8_t m = static_cast<uint8_t>(idx_to_text_ling[data[b]]);
            out.putU8(isDesc ? static_cast<uint8_t>(~m) : m);
            if (out.size() > 240) break;
        }
    } else {
        /* UCS-2LE uncompressed */
        for (size_t b = 0; b < len; b += 2) {
            if (data[b] == 0) break;
            const uint8_t m = static_cast<uint8_t>(idx_to_text_ling[data[b]]);
            out.putU8(isDesc ? static_cast<uint8_t>(~m) : m);
            if (out.size() > 240) break;
        }
    }
    out.putU8(isDesc ? 0xFE : 0x01);
    out.putU8(isDesc ? 0xFF : 0x00);
}

Status ColumnEncoder::appendSegment(const MdbColumn* col, const Field* f, bool isDesc, ByteVec& out) {
    if (!f || f->isNull || !f->data) {
        out.putU8(isDesc ? kDescNull : kAscNull); /* Jackcess null rule */
        return Status::Ok();
    }
    if (col->col_type == MDB_INT) {
        if (f->size < 2) return Status::Fail("appendSegment: INT short");
        out.putU8(isDesc ? kDescStart : kAscStart);
        encodeInteger(f->data, 2, isDesc, out);
    } else if (col->col_type == MDB_LONGINT) {
        if (f->size < 4) return Status::Fail("appendSegment: LONG short");
        out.putU8(isDesc ? kDescStart : kAscStart);
        encodeInteger(f->data, 4, isDesc, out);
    } else if (col->col_type == MDB_DOUBLE || col->col_type == MDB_DATETIME) {
        if (f->size < 8) return Status::Fail("appendSegment: DOUBLE short");
        out.putU8(isDesc ? kDescStart : kAscStart);
        encodeDouble(f->data, isDesc, out);
    } else if (col->col_type == MDB_TEXT) {
        out.putU8(isDesc ? kDescStart : kAscStart);
        encodeText(f->data, f->size, isDesc, out);
    } else {
        size_t n = f->size;
        if (col->col_size > 0 && col->col_size < 32) n = std::min(n, static_cast<size_t>(col->col_size));
        if (n > 32) n = 32;
        out.putBytes(f->data, n);
    }
    if (out.size() > 248) return Status::Fail("appendSegment: key too long");
    return Status::Ok();
}

size_t ColumnEncoder::commonPrefixLength(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    const size_t m = std::min(a.size(), b.size());
    size_t i = 0;
    while (i < m && a[i] == b[i]) ++i;
    return i;
}

} /* namespace Jet4Writer */
