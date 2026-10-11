/**
 * @file jet4x_core.cpp
 * @description jet4x core implementation: RC4, Jet date, header synthesis,
 * DatabaseImage allocator + atomic durable commit.
 */

#include "jet4x_core.h"

#include <chrono>
#include <cstdio>
#include <ctime>

#if defined(__APPLE__) || defined(__linux__)
#include <fcntl.h>
#include <unistd.h>
#endif

namespace jet4x {

/** Applies RC4 keystream (symmetric: encrypt == decrypt). */
void rc4Crypt(const uint8_t* key, uint32_t keyLen, uint8_t* data, uint32_t len) {
    uint8_t s[256];
    for (int i = 0; i < 256; i++)
        s[i] = (uint8_t)i;
    uint32_t j = 0;
    for (int i = 0; i < 256; i++) {
        j = (j + s[i] + key[i % keyLen]) & 0xFF;
        uint8_t t = s[i];
        s[i] = s[j];
        s[j] = t;
    }
    uint32_t i = 0;
    j = 0;
    for (uint32_t n = 0; n < len; n++) {
        i = (i + 1) & 0xFF;
        j = (j + s[i]) & 0xFF;
        uint8_t t = s[i];
        s[i] = s[j];
        s[j] = t;
        data[n] ^= s[(s[i] + s[j]) & 0xFF];
    }
}

/** Days (fractional) since the Jet epoch 1899-12-30 00:00 UTC. */
double jetDateNow() {
    using namespace std::chrono;
    const int64_t jetEpochUnix = -2209152000LL; /* 1899-12-30T00:00:00Z */
    const double nowUnix =
        (double)duration_cast<milliseconds>(system_clock::now().time_since_epoch())
            .count() /
        1000.0;
    return (nowUnix - (double)jetEpochUnix) / 86400.0;
}

/* Fixed header prefix: 00 01 00 00 "Standard Jet DB" 00 01 00 00 00. */
static const uint8_t kHeaderPrefix[24] = {
    0x00, 0x01, 0x00, 0x00, 0x53, 0x74, 0x61, 0x6E, 0x64, 0x61, 0x72, 0x64,
    0x20, 0x4A, 0x65, 0x74, 0x20, 0x44, 0x42, 0x00, 0x01, 0x00, 0x00, 0x00,
};

/* Plaintext counters @24..59 (identical in Data.001/Data.002). */
static const uint8_t kHeaderCounters[36] = {
    0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/* No-password sentinel @66..105 (Data.002 ground truth). */
static const uint8_t kNoPassword[4] = { 0xBB, 0xB1, 0x00, 0x00 };

static const uint8_t kJetKey[4] = { 0xC7, 0xDA, 0x39, 0x6B };

bool synthesizeHeader(uint8_t* out, std::string& error) {
    if (!out) {
        error = "null output buffer";
        return false;
    }
    memset(out, 0, kPageSize);
    memcpy(out + kOffMagic, kHeaderPrefix, sizeof(kHeaderPrefix));
    memcpy(out + kOffCryptStart, kHeaderCounters, sizeof(kHeaderCounters));

    uint8_t plain[kOffCryptLen];
    memset(plain, 0, sizeof(plain));
    memcpy(plain, kHeaderCounters, sizeof(kHeaderCounters));
    wrU16(plain + (kOffCodePage - kOffCryptStart), 1252); /* code page */
    wrU32(plain + (kOffDbKey - kOffCryptStart), 0);       /* encoding key */
    for (int k = 0; k < 10; k++)                         /* no password */
        memcpy(plain + (66 - kOffCryptStart) + k * 4, kNoPassword, 4);
    /* Opaque per-DB stamp @106 (unprotected-DB value from Data.002). */
    plain[106 - kOffCryptStart] = 0xA6;
    plain[107 - kOffCryptStart] = 0x11;
    wrU32(plain + (110 - kOffCryptStart), 0x00000409); /* sort order/lang */
    double now = jetDateNow();                        /* live header date */
    memcpy(plain + (kOffHeaderDate - kOffCryptStart), &now, 8);

    rc4Crypt(kJetKey, 4, plain, kOffCryptLen);
    memcpy(out + kOffCryptStart, plain, kOffCryptLen);
    /* 152..4095 stay zero: no reader parses them (libmdb reads only
     * @0x14/0x3c/0x3e/0x6e; per-file blob content is unconstrained —
     * jackcess template clones open in Access with arbitrary blobs). */
    return true;
}

DatabaseImage::DatabaseImage() {}

uint32_t DatabaseImage::allocPage(uint8_t pageType) {
    std::vector<uint8_t> pg(kPageSize, 0);
    pg[0] = pageType;
    pages_.push_back(std::move(pg));
    return (uint32_t)(pages_.size() - 1);
}

uint8_t* DatabaseImage::page(uint32_t pg) {
    return pg < pages_.size() ? pages_[pg].data() : nullptr;
}

const uint8_t* DatabaseImage::page(uint32_t pg) const {
    return pg < pages_.size() ? pages_[pg].data() : nullptr;
}

uint32_t DatabaseImage::pageCount() const { return (uint32_t)pages_.size(); }

bool DatabaseImage::validate(std::string& error) const {
    if (pages_.empty()) {
        error = "empty image: no pages";
        return false;
    }
    for (uint32_t i = 0; i < pages_.size(); i++) {
        if (pages_[i].size() != kPageSize) {
            error = "page " + std::to_string(i) + " size " +
                    std::to_string(pages_[i].size());
            return false;
        }
    }
    const uint8_t* h = pages_[0].data();
    static const char kMagic[] = "Standard Jet DB";
    if (h[0] != 0x00 || h[1] != 0x01 || h[2] != 0x00 || h[3] != 0x00 ||
        memcmp(h + 4, kMagic, 15) != 0) {
        error = "page 0 magic mismatch";
        return false;
    }
    if (h[kOffVersion] != 0x01) {
        error = "page 0 version is not Jet4";
        return false;
    }
    return true;
}

bool DatabaseImage::commitFile(const std::string& path,
                               std::string& error) const {
    if (!validate(error))
        return false;
    const std::string tmp = path + ".tmp";
    FILE* f = fopen(tmp.c_str(), "wb");
    if (!f) {
        error = "cannot open tmp file " + tmp;
        return false;
    }
    bool ok = true;
    for (uint32_t i = 0; i < pages_.size() && ok; i++) {
        if (fwrite(pages_[i].data(), 1, kPageSize, f) != kPageSize) {
            error = "short write at page " + std::to_string(i);
            ok = false;
        }
    }
    if (ok && fflush(f) != 0) {
        error = "fflush failed";
        ok = false;
    }
#if defined(__APPLE__) || defined(__linux__)
    if (ok && fsync(fileno(f)) != 0) {
        error = "fsync failed";
        ok = false;
    }
#endif
    if (fclose(f) != 0 && ok) {
        error = "fclose failed";
        ok = false;
    }
    if (!ok) {
        remove(tmp.c_str());
        return false;
    }
    /* Atomic publish: target appears only ever complete. */
    if (rename(tmp.c_str(), path.c_str()) != 0) {
        error = "rename to target failed";
        remove(tmp.c_str());
        return false;
    }
    return true;
}

} /* namespace jet4x */
