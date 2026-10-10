#pragma once

#include <QString>
#include <QStringList>
#include "gstr1_engine.h"

namespace MahadevERP {

// Writes a GSTR-1 workbook that is structurally identical to the official
// offline-tool template (GSTR1GovtTemplate_v2.1): same 22 sheets in order,
// same row-4 header strings, same whole-column dropdown validations backed by
// the same 18 defined names, same master lists, same Main block — built
// FROM SCRATCH with miniz (no template file needed, no missing-file failure).
// Deterministic: fixed doc timestamps, fixed sheet order, sorted rows,
// locale-free numbers (2 decimals) and English dates (dd-mmm-yyyy).
// The file opens in the official Returns Offline Tool, Validates clean, and
// generates the portal-upload JSON.
struct Gstr1ExcelStats {
    int b2bRows = 0;
    int b2clRows = 0;
    int b2csRows = 0;
    int cdnrRows = 0;
    int cdnurRows = 0;
    int hsnB2BRows = 0;
    int hsnB2CRows = 0;
    int docRows = 0;
};

struct Gstr1ExcelResult {
    bool ok = false;
    QString error;
    Gstr1ExcelStats stats;
};

class Gstr1ExcelWriter {
public:
    static Gstr1ExcelResult write(const Gstr1ReturnPayload& payload, const QString& outPath);
};

} // namespace MahadevERP
