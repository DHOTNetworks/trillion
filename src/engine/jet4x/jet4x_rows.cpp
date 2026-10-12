/**
 * @file jet4x_rows.cpp
 * @description jet4x row packer implementation.
 */

#include "jet4x_rows.h"

#include <algorithm>
#include <map>

#include "jet4x_pages.h"

namespace jet4x {

std::vector<uint8_t> encodeText(const std::u16string& text, bool compressed) {
    std::vector<uint8_t> out;
    bool useCompressed = compressed && text.size() > 2;
    if (useCompressed) {
        for (char16_t c : text) {
            if (c < 1 || c > 0xFF) {
                useCompressed = false;
                break;
            }
        }
    }
    if (useCompressed) {
        out.reserve(2 + text.size());
        out.push_back(0xFF);
        out.push_back(0xFE);
        for (char16_t c : text)
            out.push_back((uint8_t)c);
    } else {
        out.reserve(text.size() * 2);
        for (char16_t c : text) {
            out.push_back((uint8_t)(c & 0xFF));
            out.push_back((uint8_t)((c >> 8) & 0xFF));
        }
    }
    return out;
}

static void putU16(std::vector<uint8_t>& b, uint16_t v) {
    b.push_back((uint8_t)(v & 0xFF));
    b.push_back((uint8_t)((v >> 8) & 0xFF));
}

static void putU32(std::vector<uint8_t>& b, uint32_t v) {
    b.push_back((uint8_t)(v & 0xFF));
    b.push_back((uint8_t)((v >> 8) & 0xFF));
    b.push_back((uint8_t)((v >> 16) & 0xFF));
    b.push_back((uint8_t)((v >> 24) & 0xFF));
}

bool packRow(PackCtx& ctx, const std::vector<CellValue>& cells,
             std::vector<uint8_t>& out, std::string& error) {
    if (!ctx.spec) {
        error = "packRow: null spec";
        return false;
    }
    const TableSpec& spec = *ctx.spec;
    const size_t n = spec.columns.size();
    if (cells.size() != n) {
        error = "packRow: cell count mismatch";
        return false;
    }
    if (!ctx.layout) {
        ctx.ownedLayout = computeLayout(spec);
        ctx.layout = &ctx.ownedLayout;
    }
    const TableLayout& L = *ctx.layout;
    if (L.cols.size() != n) {
        error = "packRow: layout mismatch";
        return false;
    }

    std::vector<uint8_t> b;
    b.reserve(512);
    putU16(b, (uint16_t)n); /* colcount */

    /* Fixed block (always full fixedLen; nulls stay zero). */
    const size_t fixedStart = b.size();
    b.insert(b.end(), L.fixedLen, 0);
    size_t maxNum = n;
    for (size_t i = 0; i < n; i++) {
        int num = spec.columns[i].num < 0 ? (int)i : spec.columns[i].num;
        if ((size_t)num + 1 > maxNum)
            maxNum = (size_t)num + 1;
    }
    std::vector<uint8_t> mask((maxNum + 7) / 8, 0);
    /* Null-mask bits follow COLUMN NUMBER (not tdef position) — invisible
     * on canonical tables, critical on alien system tables. */
    auto mark = [&](size_t col) {
        int num = spec.columns[col].num < 0 ? (int)col
                                            : spec.columns[col].num;
        mask[(size_t)num / 8] |= (uint8_t)(1 << ((size_t)num % 8));
    };

    struct VarPiece {
        size_t col = 0;
        std::vector<uint8_t> data;
    };
    std::vector<VarPiece> varPieces;

    for (size_t i = 0; i < n; i++) {
        const ColumnSpec& c = spec.columns[i];
        const CellValue& v = cells[i];
        int fs = fixedSize(c.type, c.length);
        if (c.type == DataType::Boolean) {
            if (!v.isNull)
                mark(i); /* true; false/null share the clear bit */
            continue;
        }
        if (isFixedPlaced(c)) {
            int fs = fixedSize(c.type, c.length);
            if (!v.isNull) {
                if ((int)v.fixed.size() != fs) {
                    error = "packRow: fixed size mismatch col " + c.name;
                    return false;
                }
                if ((size_t)L.cols[i].fixOff + (size_t)fs > L.fixedLen) {
                    error = "packRow: fixed span out of block col " + c.name;
                    return false;
                }
                mark(i);
                memcpy(b.data() + fixedStart + L.cols[i].fixOff,
                       v.fixed.data(), (size_t)fs);
            }
            continue;
        }
        /* Variable-placed (TEXT/BINARY/LVAL, or fixed-size types without
         * the FIXED bit): content bytes as one var piece. */
        if (v.isNull)
            continue;
        mark(i);
        if (c.type == DataType::Binary || c.type == DataType::BigBinary) {
            VarPiece p;
            p.col = i;
            p.data = v.blob;
            varPieces.push_back(std::move(p));
            continue;
        }
        if (!isVarType(c.type)) {
            /* Var-placed fixed-size content (NavPane pattern): verbatim. */
            int fs = fixedSize(c.type, c.length);
            if ((int)v.fixed.size() != fs) {
                error = "packRow: var-fixed size mismatch col " + c.name;
                return false;
            }
            VarPiece p;
            p.col = i;
            p.data = v.fixed;
            varPieces.push_back(std::move(p));
            continue;
        }
        if (c.type == DataType::Text) {
            VarPiece p;
            p.col = i;
            p.data = encodeText(v.text, c.extFlags == 0x01);
            varPieces.push_back(std::move(p));
        } else {
            /* MEMO / OLE long value. */
            std::vector<uint8_t> content;
            if (c.type == DataType::Memo)
                content = encodeText(v.text, c.extFlags == 0x01);
            else
                content = v.blob;
            if (content.size() <= kMaxInlineLval) {
                /* Inline: [len|0x80000000][0 x8] + value bytes. */
                VarPiece p;
                p.col = i;
                p.data.reserve(12 + content.size());
                uint32_t lwf =
                    (uint32_t)content.size() | 0x80000000u;
                p.data.push_back((uint8_t)(lwf & 0xFF));
                p.data.push_back((uint8_t)((lwf >> 8) & 0xFF));
                p.data.push_back((uint8_t)((lwf >> 16) & 0xFF));
                p.data.push_back((uint8_t)((lwf >> 24) & 0xFF));
                for (int k = 0; k < 8; k++)
                    p.data.push_back(0);
                p.data.insert(p.data.end(), content.begin(), content.end());
                varPieces.push_back(std::move(p));
                LvalAlloc a;
                a.col = i;
                a.length = content.size();
                a.inlineData = true;
                ctx.lvals.push_back(a);
            } else {
                if (!ctx.img) {
                    error = "packRow: image required for chained LVAL col " +
                            c.name;
                    return false;
                }
                uint32_t lpg = 0;
                uint8_t lrow = 0;
                std::vector<std::pair<uint32_t, uint8_t>> used;
                if (!writeLvalChain(*ctx.img, content.data(), content.size(),
                                    lpg, lrow, used, error))
                    return false;
                VarPiece p;
                p.col = i;
                uint32_t lwf = (uint32_t)content.size() |
                               (content.size() <= kMaxLvalRow ? 0x40000000u
                                                              : 0x00000000u);
                p.data.push_back((uint8_t)(lwf & 0xFF));
                p.data.push_back((uint8_t)((lwf >> 8) & 0xFF));
                p.data.push_back((uint8_t)((lwf >> 16) & 0xFF));
                p.data.push_back((uint8_t)((lwf >> 24) & 0xFF));
                p.data.push_back(lrow);
                p.data.push_back((uint8_t)(lpg & 0xFF));
                p.data.push_back((uint8_t)((lpg >> 8) & 0xFF));
                p.data.push_back((uint8_t)((lpg >> 16) & 0xFF));
                putU32(p.data, 0); /* write stamp */
                varPieces.push_back(std::move(p));
                LvalAlloc a;
                a.col = i;
                a.page = lpg;
                a.row = lrow;
                a.length = content.size();
                a.inlineData = false;
                ctx.lvals.push_back(a);
            }
        }
    }

    /* Variable data in SLOT (varoff) order with positions recorded at
     * visit time — null slots point at the current position (monotonic),
     * never back-patched to EOD. This is what makes slot-order spans
     * (libmdb mdb_crack_row4) read correctly: span[k] = offs[k+1]-offs[k]
     * only works when offsets are non-decreasing. */
    std::vector<uint16_t> slotOff(L.nVar, 0);
    std::vector<size_t> varCols;
    for (size_t i = 0; i < n; i++) {
        if (isVarPlaced(spec.columns[i]))
            varCols.push_back(i);
    }
    std::sort(varCols.begin(), varCols.end(), [&](size_t a, size_t b) {
        if (L.cols[a].varOff != L.cols[b].varOff)
            return L.cols[a].varOff < L.cols[b].varOff;
        return a < b;
    });
    std::map<size_t, const std::vector<uint8_t>*> pieceByCol;
    for (const auto& p : varPieces)
        pieceByCol[p.col] = &p.data;
    /* Forward fill: every slot (claimed or hole) gets the running
     * position, so offsets stay monotonic and slot-order spans read
     * correctly. Trailing holes land on EOD naturally. */
    {
        size_t vi = 0;
        for (uint16_t s = 0; s < L.nVar; s++) {
            slotOff[s] = (uint16_t)b.size();
            while (vi < varCols.size() &&
                   L.cols[varCols[vi]].varOff == s) {
                auto it = pieceByCol.find(varCols[vi]);
                if (it != pieceByCol.end())
                    b.insert(b.end(), it->second->begin(), it->second->end());
                vi++;
            }
        }
        /* Columns claiming out-of-range slots were validated earlier;
         * any var column beyond nVar is a spec error (already rejected). */
    }
    uint16_t eod = (uint16_t)b.size();
    putU16(b, eod);
    for (int s = (int)L.nVar - 1; s >= 0; s--)
        putU16(b, slotOff[(size_t)s]);
    putU16(b, L.nVar);
    b.insert(b.end(), mask.begin(), mask.end());

    if (b.size() > kMaxRowSize) {
        error = "packRow: row exceeds max size";
        return false;
    }
    out = std::move(b);
    return true;
}

} /* namespace jet4x */
