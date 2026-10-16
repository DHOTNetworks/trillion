#pragma once

#include <QString>
#include <QDate>
#include <QStringList>
#include <QMap>
#include <map>

namespace MahadevERP {

class MigrationUtils {
public:
    static QString parseNormalizedDate(const QString& rawDate);
    static QString resolveFinancialYear(const QString& isoDate);
    static QString resolveFirmTypeFromPan(const QString& rawType, const QString& pan, const QString& companyName = "");
    static void cleanBankingDetails(const QString& m2, const QString& b2, const QString& b3,
                                   QString& outBankName, QString& outAccountNo, QString& outIfsc);
    static QString extractFssai(const QString& business, const QString& address);
    static QString extractStateCode(const QString& gstin, const QString& state);
    static QString extractPincode(const QString& address, const QString& fallback = "125055");
    static QString cleanText(const QString& s);
    static QString formatFyVoucherNo(const QString& isoDate, const QString& typeCode, const QString& rawNo);
    // F5: resolve-or-create FY id. Map keys come in two spellings ("FY 2024-25"
    // vs "2024-25") depending on the pipeline; lookups normalize across both,
    // so the old silent `fallback-to-id-1` can never misfile a row again.
    // Unknown-but-parseable years materialize as proper FY rows (choke dates);
    // garbage labels return -1 with a warning and migrate NOTHING (callers
    // must skip + count, never default).
    static int fyIdFor(QMap<QString, int>& map, const QString& fyVal);
    static int fyIdForStd(std::map<QString, int>& map, const QString& fyVal);
};

} // namespace MahadevERP
