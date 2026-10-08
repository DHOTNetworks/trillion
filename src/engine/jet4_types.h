#pragma once
/*
 * Jet 4 Types, Constants, and Common Data Structures.
 * Spec: Jackcess (JetFormat V2000/V2003, IndexCodes, PageChannel, UsageMap).
 */

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

namespace Jet4Writer {

/* ------------------------------------------------------------------ */
/* Jet 4 page layout constants (Jackcess JetFormat V2000/V2003)        */
/* ------------------------------------------------------------------ */
inline constexpr int kPageSize = 4096;
inline constexpr uint8_t kPageData = 0x01;
inline constexpr uint8_t kPageUsageMap = 0x05;
inline constexpr uint8_t kPageIndex = 0x03;
inline constexpr uint8_t kPageLeaf = 0x04;

/* Leaf/node header field offsets (little-endian) */
inline constexpr int kOffFreeSpace = 2;
inline constexpr int kOffTableDefPg = 4;
inline constexpr int kOffUnknown = 8;      /* reserved, must stay 0 */
inline constexpr int kOffPrevPg = 12;      /* Jet4: NOT 8 (that is Jet3) */
inline constexpr int kOffNextPg = 16;      /* Jet4: NOT 12 (that is Jet3) */
inline constexpr int kOffChildTailPg = 20;
inline constexpr int kOffPrefixLen = 24;   /* int16: shared entry-prefix length */
inline constexpr int kOffPrefixUnknown = 26;
inline constexpr int kOffEntryMask = 27;   /* 0x1B */
inline constexpr int kSizeEntryMask = 453;
inline constexpr int kEntryAreaStart = 480; /* 0x1E0 = 27 + 453 */

/* Map Types */
inline constexpr uint8_t kMapTypeInline = 0;
inline constexpr uint8_t kMapTypeReference = 1;

/* Index key flags (Jackcess IndexCodes) */
inline constexpr uint8_t kAscStart = 0x7F;
inline constexpr uint8_t kAscNull = 0x00;
inline constexpr uint8_t kDescStart = 0x80;
inline constexpr uint8_t kDescNull = 0xFF;

/* Maximum lengths */
inline constexpr size_t kMaxEntryLength = 255;
inline constexpr size_t kMaxTreeDepth = 12;

/* ------------------------------------------------------------------ */
/* Status: Operation result with optional error message               */
/* ------------------------------------------------------------------ */
struct Status {
    bool ok = true;
    std::string error;

    static Status Ok() { return Status{true, {}}; }
    static Status Fail(const std::string& msg) { return Status{false, msg}; }
private:
    Status(bool o, const std::string& e) : ok(o), error(e) {}
};

/* ------------------------------------------------------------------ */
/* ByteVec: bounds-checked dynamic byte buffer                        */
/* ------------------------------------------------------------------ */
class ByteVec {
public:
    explicit ByteVec(size_t cap = 512) { buf_.reserve(cap); }
    void putU8(uint8_t v) { buf_.push_back(v); }
    void putU16LE(uint16_t v) {
        buf_.push_back(static_cast<uint8_t>(v & 0xFF));
        buf_.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    }
    void putU16BE(uint16_t v) {
        buf_.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
        buf_.push_back(static_cast<uint8_t>(v & 0xFF));
    }
    void putU32BE(uint32_t v) {
        buf_.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
        buf_.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
        buf_.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
        buf_.push_back(static_cast<uint8_t>(v & 0xFF));
    }
    void putBytes(const uint8_t* p, size_t n) {
        if (p && n) buf_.insert(buf_.end(), p, p + n);
    }
    size_t size() const { return buf_.size(); }
    const uint8_t* data() const { return buf_.data(); }
    uint8_t* data() { return buf_.data(); }
    const std::vector<uint8_t>& bytes() const { return buf_; }
    std::vector<uint8_t> release() { return std::move(buf_); }
private:
    std::vector<uint8_t> buf_;
};

/* ------------------------------------------------------------------ */
/* Field view: non-owning column data for row and index operations    */
/* ------------------------------------------------------------------ */
struct Field {
    int colnum = -1;              /* 0-based table column */
    bool isNull = true;
    bool isFixed = true;
    const uint8_t* data = nullptr;
    size_t size = 0;
};

/* ------------------------------------------------------------------ */
/* Verification report structures                                     */
/* ------------------------------------------------------------------ */
struct IndexEntryInfo {
    std::vector<uint8_t> fullKey; /* without trailer */
    uint32_t dataPg = 0;
    uint16_t rowIdx = 0;          /* 0-based row within data page */
    uint32_t leafPg = 0;
};

struct VerifyIndexInfo {
    std::string table;
    std::string name;
    long walked = 0;
    long dataRows = 0;
    long catalogRows = 0;
    bool sorted = false;
};

struct VerifyReport {
    bool ok = false;
    long tablesChecked = 0;
    long dataRows = 0;
    long badRows = 0;
    std::vector<VerifyIndexInfo> indexes;
    std::string error;
};

} /* namespace Jet4Writer */
