/**
 * @file jet4x_layout.h
 * @description jet4x canonical column layout — single source of truth for
 * slot/offset assignment shared by the tdef writer and the row packer.
 * Ground-truth rules (Bahi-Khata Data.002, incl. alien system tables):
 * - column NUMBER is semantic: fixed placement, var slots and nullmask
 *   all follow number order, not tdef position order (MSysACEs/
 *   MSysRelationships proof). Canonical tables have num == position.
 * - varoff = rank among variable-length (TEXT/BINARY/MEMO/OLE) columns
 *   in number order; TEXT/BINARY/LVAL consume, FIXED/BOOL peek.
 * - fixoff = accumulated fixed sizes in number order (BOOL: none).
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "jet4x_schema.h"

namespace jet4x {

struct ColumnLayout {
    uint16_t varOff = 0;
    uint16_t fixOff = 0;
};

struct TableLayout {
    std::vector<ColumnLayout> cols;
    uint16_t nVar = 0;      /* var slots (== maxVarColumnCount) */
    uint16_t fixedLen = 0;  /* total fixed-data bytes */
};

/** Effective column number (explicit or positional). */
inline int colNum(const TableSpec& spec, size_t i) {
    return spec.columns[i].num < 0 ? (int)i : spec.columns[i].num;
}

/** Computes canonical layout for a spec. Always succeeds for valid specs. */
inline TableLayout computeLayout(const TableSpec& spec) {
    TableLayout L;
    const size_t n = spec.columns.size();
    L.cols.resize(n);
    /* Number order for placement. */
    std::vector<size_t> byNum(n);
    for (size_t i = 0; i < n; i++)
        byNum[i] = i;
    std::sort(byNum.begin(), byNum.end(), [&](size_t a, size_t b) {
        return colNum(spec, a) < colNum(spec, b);
    });
    uint16_t nextVar = 0, nextFix = 0;
    for (size_t i : byNum) {
        const ColumnSpec& c = spec.columns[i];
        if (isVarType(c.type)) {
            L.cols[i].varOff = nextVar++;
        } else if (c.type == DataType::Boolean) {
            L.cols[i].varOff = nextVar;
        } else if (isFixedPlaced(c)) {
            int fs = fixedSize(c.type, c.length);
            L.cols[i].varOff = nextVar;
            L.cols[i].fixOff = nextFix;
            nextFix = (uint16_t)(nextFix + fs);
        } else {
            /* Fixed-size type without the FIXED bit (NavPane pattern):
             * var-placed; consumes a slot like TEXT. */
            L.cols[i].varOff = nextVar++;
        }
    }
    L.nVar = nextVar;
    L.fixedLen = nextFix;
    return L;
}

/**
 * Recomputes fixedLen to cover explicit (mirror) fixoffs, which may be
 * sparse (gaps stay zero). Call after overwriting L.cols fixoffs with
 * tdef-verbatim values. Canonical layouts are unaffected (dense).
 */
inline void finalizeExplicitLayout(TableLayout& L, const TableSpec& spec) {
    uint16_t fl = 0;
    for (size_t i = 0; i < spec.columns.size(); i++) {
        int fs = fixedSize(spec.columns[i].type, spec.columns[i].length);
        if (fs > 0) {
            uint16_t end = (uint16_t)(L.cols[i].fixOff + fs);
            if (end > fl)
                fl = end;
        }
    }
    L.fixedLen = fl;
}

} /* namespace jet4x */
