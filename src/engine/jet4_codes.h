#pragma once
/*
 * Jet4 Column Encoders (Jackcess-compatible IndexCodes & ColumnDescriptors).
 */

#include "jet4_types.h"

extern "C" {
#include "mdbtools.h"
}

namespace Jet4Writer {

class ColumnEncoder {
public:
    static const MdbColumn* getColumn(const MdbTableDef* table, int colnum);
    static const Field* findField(const Field* fields, size_t nfields, int colnum);

    static void encodeInteger(const uint8_t* leData, size_t numBytes, bool isDesc, ByteVec& out);
    static void encodeDouble(const uint8_t* leData8, bool isDesc, ByteVec& out);
    static void encodeText(const uint8_t* data, size_t len, bool isDesc, ByteVec& out);

    static Status appendSegment(const MdbColumn* col, const Field* f, bool isDesc, ByteVec& out);

    static size_t commonPrefixLength(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b);
};

} /* namespace Jet4Writer */
