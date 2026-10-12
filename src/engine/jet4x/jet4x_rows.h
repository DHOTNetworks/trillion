/**
 * @file jet4x_rows.h
 * @description jet4x row packer — libmdb-free Jackcess-mirrored row writer.
 * Layout: [colcount u16][fixed block][var data][EOD u16][var offsets,
 * slot_{nVar-1}..slot_0][varcount u16][nullmask]. Encodings verified byte
 * for byte against Bahi-Khata Data.002 rows (see scratch/jet4x/rowtest).
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "jet4x_core.h"
#include "jet4x_layout.h"
#include "jet4x_pages.h"
#include "jet4x_schema.h"

namespace jet4x {

/** Caller-provided cell value (migration path copies source bytes verbatim
 * for fixed numerics/datetimes: zero float-error risk by construction). */
struct CellValue {
    bool isNull = true;
    std::vector<uint8_t> fixed; /* fixed-size cols: exact bytes */
    std::u16string text;        /* TEXT/MEMO: decoded text */
    std::vector<uint8_t> blob;  /* BINARY: exact bytes; OLE-raw: exact bytes */
};

/** A chained-LVAL allocation produced while packing (for umap catalog). */
struct LvalAlloc {
    size_t col = 0;      /* spec column index */
    uint32_t page = 0;   /* first LVAL page */
    uint8_t row = 0;     /* first row on that page */
    size_t length = 0;   /* value bytes */
    bool inlineData = false;
};

struct PackCtx {
    const TableSpec* spec = nullptr;
    const TableLayout* layout = nullptr;
    TableLayout ownedLayout; /* set when spec given without layout */
    DatabaseImage* img = nullptr; /* LVAL page allocation (nullable if none) */
    std::vector<LvalAlloc> lvals; /* out: chained LVAL placements */
};

/** Encodes TEXT/MEMO content: compressed (FF FE + 1B/char) iff allowed. */
std::vector<uint8_t> encodeText(const std::u16string& text, bool compressed);

/** Packs one row. Returns false + error on oversize/unsupported. */
bool packRow(PackCtx& ctx, const std::vector<CellValue>& cells,
             std::vector<uint8_t>& out, std::string& error);

} /* namespace jet4x */
