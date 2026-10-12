/**
 * @file jet4x_layout.h
 * @description jet4x canonical column layout — single source of truth for
 * slot/offset assignment shared by the tdef writer and the row packer.
 * Rules are Bahi-Khata ground truth (NOT jackcess defaults):
 * shared var-slot counter consumed by TEXT/MEMO/OLE in column order,
 * FIXED/BOOL peek without consuming.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "jet4x_schema.h"

namespace jet4x {

struct ColumnLayout {
    uint16_t varOff = 0; /* var slot (TEXT/LVAL consume; FIXED/BOOL peek) */
    uint16_t fixOff = 0; /* fixed-data offset (accumulated sizes) */
};

struct TableLayout {
    std::vector<ColumnLayout> cols;
    uint16_t nVar = 0;      /* var slots (== maxVarColumnCount) */
    uint16_t fixedLen = 0;  /* total fixed-data bytes */
};

/** Computes canonical layout for a spec. Always succeeds for valid specs. */
inline TableLayout computeLayout(const TableSpec& spec) {
    TableLayout L;
    L.cols.resize(spec.columns.size());
    uint16_t nextVar = 0, nextFix = 0;
    for (size_t i = 0; i < spec.columns.size(); i++) {
        const ColumnSpec& c = spec.columns[i];
        int fs = fixedSize(c.type, c.length);
        if (fs == -1 || fs == -2) {
            L.cols[i].varOff = nextVar++;
        } else if (c.type == DataType::Boolean) {
            L.cols[i].varOff = nextVar;
        } else {
            L.cols[i].varOff = nextVar;
            L.cols[i].fixOff = nextFix;
            nextFix = (uint16_t)(nextFix + fs);
        }
    }
    L.nVar = nextVar;
    L.fixedLen = nextFix;
    return L;
}

} /* namespace jet4x */
