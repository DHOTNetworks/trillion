/**
 * @file jet4x_core.h
 * @description jet4x from-scratch Jet4 engine — core primitives.
 *
 * Key exports:
 * - rdU16/rdU32/wrU16/wrU32 — little-endian page buffer helpers
 * - rc4Crypt() — Jet4 header stream cipher (key C7 DA 39 6B, libmdb file.c)
 * - jetDateNow() — current time as Jet DATE double (days since 1899-12-30)
 * - synthesizeHeader() — builds a valid Jet4 page 0 from embedded constants
 *   (ground truth decrypted from Bahi-Khata Data.002 via RC4; no seed file)
 * - DatabaseImage — in-memory page array, typed page allocator, atomic
 *   commit (tmp + fsync + rename), pre-write validation
 *
 * Frozen code reused, never duplicated: Jet4Writer::packRow / BTreeEngine /
 * jet4_codes encoders (see jet4x_rows / jet4x_index when they land).
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace jet4x {

inline constexpr uint32_t kPageSize = 4096;

/* Page type codes (jackcess PageTypes, Jet4). */
inline constexpr uint8_t kPgInvalid = 0x00;
inline constexpr uint8_t kPgData = 0x01;
inline constexpr uint8_t kPgTableDef = 0x02;
inline constexpr uint8_t kPgIndexNode = 0x03;
inline constexpr uint8_t kPgIndexLeaf = 0x04;
inline constexpr uint8_t kPgUsageMap = 0x05;

/* Header page field offsets (libmdb file.c ground truth). */
inline constexpr uint32_t kOffMagic = 0;       /* 00 01 00 00 "Standard Jet DB" */
inline constexpr uint32_t kOffVersion = 20;    /* 0x01 = Jet4 */
inline constexpr uint32_t kOffCryptStart = 24; /* RC4 region start */
inline constexpr uint32_t kOffCryptLen = 128;  /* RC4 region length (ends 152) */
inline constexpr uint32_t kOffCodePage = 60;   /* u16, plaintext 1252 */
inline constexpr uint32_t kOffLangId = 110;    /* u16, plaintext 0x0409=1033 */
inline constexpr uint32_t kOffDbKey = 62;      /* u32, plaintext 0 */
inline constexpr uint32_t kOffHeaderDate = 114; /* Jet DATE double, plaintext */

/* Little-endian buffer helpers. */
inline uint16_t rdU16(const uint8_t* p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}
inline uint32_t rdU32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
inline void wrU16(uint8_t* p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
}
inline void wrU32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}

/** RC4 stream cipher applied in place (Jet4 header "encryption"). */
void rc4Crypt(const uint8_t* key, uint32_t keyLen, uint8_t* data, uint32_t len);

/** Current UTC time as a Jet DATE (fractional days since 1899-12-30). */
double jetDateNow();

/**
 * Builds a complete valid Jet4 page 0.
 * Constants are ground truth decrypted from Bahi-Khata Data.002
 * (unprotected DB: codepage 1252, sort 0x0409, no-password sentinel);
 * only the header date is live. out must hold >= 4096 bytes.
 */
bool synthesizeHeader(uint8_t* out, std::string& error);

/**
 * In-memory database image under construction.
 * Pages are assembled and validated in RAM; nothing reaches disk
 * until commitFile(), which writes tmp + fsync + atomic rename so a
 * crash can never leave a half-written database at the target path.
 */
class DatabaseImage {
public:
    DatabaseImage();

    /** Allocate a zeroed page with its type byte preset. Returns page no. */
    uint32_t allocPage(uint8_t pageType);

    /** Mutable access to a page image. */
    uint8_t* page(uint32_t pg);
    const uint8_t* page(uint32_t pg) const;
    uint32_t pageCount() const;

    /**
     * Structural validation before any disk write:
     * page 0 magic+version, every page 4096 bytes, page-count sanity.
     * Returns false + reason on any anomaly.
     */
    bool validate(std::string& error) const;

    /**
     * Durable commit: write all pages to path.tmp, fsync, rename to path.
     * The target path is never truncated or partially written.
     */
    bool commitFile(const std::string& path, std::string& error) const;

private:
    std::vector<std::vector<uint8_t>> pages_;
};

} /* namespace jet4x */
