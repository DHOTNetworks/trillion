/**
 * @file jet4x_catalog.h
 * @description jet4x catalog tools: MSys tdef byte-mirror with pointer
 * relocation, tdef-page remap tables (ObjectId/NavPane links), autonumber
 * sequencing rule. The catalog rows themselves pack with the standard
 * row packer fed by MSysObject values (see scratch/jet4x/msys_002.txt).
 */

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "jet4x_core.h"

namespace jet4x {

/**
 * Relocates a system-table tdef page image to fresh pages: patches the
 * owned/free umap refs (@55..62), every index-data umap row/page/root,
 * keeps counts/names/definitions/flags byte-identical. indexRoots[i] is
 * the fresh empty root for the i-th index slot; umapPg the fresh umap.
 * Single-page tdefs only (all MSys tdefs qualify; errors otherwise).
 */
bool relocateTdef(const uint8_t* srcPage, uint32_t umapPg,
                  const std::vector<uint32_t>& indexRoots,
                  std::vector<uint8_t>& out, std::string& error);

/**
 * Builds old-tdef-page -> entry-name for every catalog entry whose Id
 * encodes a real tdef page (user tables + MSys* tables).
 */
std::map<uint32_t, std::string> buildTdefNameMap(
    const std::vector<std::pair<uint32_t, std::string>>& idNameEntries);

/**
 * Remaps an ObjectId-style link: old tdef pages translate via
 * oldTdef->name + name->newTdef; anything else (container constants,
 * placeholders 0/1, negatives, garbage) passes through verbatim so
 * observable behavior is bit-identical.
 */
uint32_t remapLink(uint32_t oldPage,
                   const std::map<uint32_t, std::string>& oldTdefToName,
                   const std::map<std::string, uint32_t>& nameToNewTdef,
                   bool& wasMapped);

/** Next autonumber: max existing value, else 0 (ground truth rule). */
inline uint32_t nextAutoNumber(uint32_t maxExisting, bool hasRows) {
    return hasRows ? maxExisting : 0;
}

} /* namespace jet4x */
