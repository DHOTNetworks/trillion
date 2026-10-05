#pragma once

#include <QString>
#include <QDate>
#include <QStringList>

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
};

} // namespace MahadevERP
