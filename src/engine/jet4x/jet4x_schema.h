/**
 * @file jet4x_schema.h
 * @description jet4x schema model — libmdb-free table/column/index specs.
 * Mirrors jackcess TableBuilder/ColumnBuilder/IndexBuilder (declarative only).
 * Bahi-Khata inventories: scratch/bahikhata_schema_001/002.txt (1:1 source).
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace jet4x {

/* Jet4 column type codes (jackcess DataType, Jet4-supported subset). */
enum class DataType : uint8_t {
    Boolean = 0x01,
    Byte = 0x02,
    Int16 = 0x03,
    Int32 = 0x04,
    Money = 0x05,
    Float32 = 0x06,
    Double = 0x07,
    DateTime = 0x08,
    Binary = 0x09,
    Text = 0x0A,
    Ole = 0x0B,
    Memo = 0x0C,
    Guid = 0x0F,
    Numeric = 0x10,
    BigBinary = 0x11, /* variable-length (MSys binary blobs) */
};

/** Fixed storage size; -1 = variable length, -2 = long value (LVAL). */
inline int fixedSize(DataType t, uint16_t len) {
    switch (t) {
        case DataType::Boolean:
            return 0; /* lives in the null mask */
        case DataType::Byte:
            return 1;
        case DataType::Int16:
            return 2;
        case DataType::Int32:
        case DataType::Float32:
            return 4;
        case DataType::Money:
        case DataType::Double:
        case DataType::DateTime:
            return 8;
        case DataType::Guid:
            return 16;
        case DataType::Numeric:
            return 17;
        case DataType::Binary:
            return -1; /* variable-length (MSys SID/Owner pattern) */
        default:
            break;
    }
    if (t == DataType::Text || t == DataType::Binary ||
        t == DataType::BigBinary)
        return -1;
    return -2; /* Memo / Ole */
}

inline bool isLongValue(DataType t) {
    return t == DataType::Memo || t == DataType::Ole;
}

/** Variable-length types (content-sized; always var-placed). */
inline bool isVarType(DataType t) {
    return t == DataType::Text || t == DataType::Binary ||
           t == DataType::BigBinary || t == DataType::Memo ||
           t == DataType::Ole;
}

struct ColumnSpec {
    std::string name;
    DataType type = DataType::Text;
    uint16_t length = 0; /* TEXT: max bytes (510 = 255 UCS-2); BINARY: bytes */
    uint8_t extFlags = 0; /* 0x01 = compressed unicode (mirrored per column) */
    bool autoNumber = false; /* LONG autonumber: adds flag bit 0x04 */
    /* Catalog identity (default -1 = positional, i.e. canonical tables).
     * System tables carry alien numbers which are SEMANTIC (fixed placement
     * and nullmask follow column number, not tdef position). */
    int num = -1; /* column number (tdef field @5, single byte for Jet4) */
    int id = -1;  /* column id (tdef field @9) */
    bool hasFlags = false; /* true: use flags verbatim (system tables) */
    uint8_t flags = 0;
};

struct IndexKeySpec {
    std::string column;
    bool descending = false;
};

struct IndexSpec {
    std::string name;
    uint32_t indexNumber = 0;     /* logical index number (from inventory) */
    uint32_t indexDataNumber = 0; /* index-data number (from inventory) */
    uint8_t flags = 0x80;         /* Bahi-Khata default; 0x81 PK-ish */
    uint8_t type = 0;             /* 0 = normal (no FK in Bahi-Khata) */
    uint8_t cascUp = 0;           /* cascade-update byte (mirrored) */
    uint8_t cascDel = 0;          /* cascade-delete byte (mirrored) */
    std::vector<IndexKeySpec> keys;
};

struct TableSpec {
    std::string name;
    std::vector<ColumnSpec> columns;
    std::vector<IndexSpec> indexes;
};

/** LVAL column usage-map placement (both rows live on the table umap page). */
struct LvalUmap {
    uint32_t page = 0;
    uint8_t ownedRow = 0;
    uint8_t freeRow = 0;
};

/** Canonical flags for fresh tables (mirrors use tdef-verbatim flags). */
inline uint8_t defaultFlags(DataType t, int fs, bool autoNumber) {
    uint8_t f = 0x02; /* UPDATABLE */
    if (fs > 0)
        f |= 0x01; /* FIXED_LEN */
    if (t == DataType::Boolean)
        f = (uint8_t)(0x02 | 0x01); /* 0x03, nullmask-stored */
    if (autoNumber)
        f |= 0x04;
    return f;
}

/**
 * Fixed-vs-variable PLACEMENT (not storage size!). Ground truth:
 * placement follows the flags FIXED bit for fixed-size types; variable-
 * length types are always var-placed (a FIXED bit on TEXT is a fossil
 * lie — NewTable c2 proof); Booleans live in the nullmask. Fresh-table
 * defaults make this identical to the old type-driven rule.
 */
inline bool isFixedPlaced(const ColumnSpec& c) {
    if (c.type == DataType::Boolean)
        return false;
    if (isVarType(c.type))
        return false;
    int fs = fixedSize(c.type, c.length);
    if (fs <= 0)
        return false;
    if (c.hasFlags)
        return (c.flags & 0x01) != 0;
    return true;
}

/** Var-placed columns (occupy a var slot): everything not fixed-placed
 * and not Boolean (nullmask). */
inline bool isVarPlaced(const ColumnSpec& c) {
    if (c.type == DataType::Boolean)
        return false;
    return !isFixedPlaced(c);
}

} /* namespace jet4x */
