#include "aank_interest_engine.h"
#include <algorithm>

namespace MahadevERP {

double AankInterestEngine::computeInterest(double productSum, double annualRate, bool isLeapYear) {
    if (annualRate == 0.0 || productSum == 0.0) return 0.0;
    double divisor = isLeapYear ? 36600.0 : 36500.0;
    return std::round((productSum * annualRate / divisor) * 100.0) / 100.0;
}

AankInterestStatement AankInterestEngine::calculateStatement(
    int accountCode,
    const QString& accountName,
    double opBal,
    const QString& opDrCr,
    const QDate& fromDate,
    const QDate& toDate,
    double drInterestRate,
    double crInterestRate,
    const QList<QVariantMap>& rawVouchers,
    bool isLeapYearDivisor
) {
    AankInterestStatement stmt;
    stmt.accountCode = accountCode;
    stmt.accountName = accountName;
    stmt.fromDate = fromDate;
    stmt.toDate = toDate;
    stmt.drInterestRate = drInterestRate;
    stmt.crInterestRate = crInterestRate;

    // Signed opening balance: Dr is positive, Cr is negative
    double currentBal = (opDrCr.trimmed().toUpper() == "CR") ? -std::abs(opBal) : std::abs(opBal);

    // Initial Opening entry
    AankTransactionEntry opEntry;
    opEntry.date = fromDate;
    opEntry.voucherNo = "-";
    opEntry.voucherType = "OPBAL";
    opEntry.narration = "Opening Balance";
    if (currentBal >= 0) {
        opEntry.debitAmount = currentBal;
        opEntry.creditAmount = 0.0;
    } else {
        opEntry.debitAmount = 0.0;
        opEntry.creditAmount = std::abs(currentBal);
    }
    opEntry.runningBalance = currentBal;
    stmt.entries.append(opEntry);

    // Parse and sort all voucher items
    struct RawItem {
        QDate dt;
        QString vNo;
        QString vType;
        QString narr;
        double dr;
        double cr;
    };
    QList<RawItem> sortedVchs;
    for (const auto& v : rawVouchers) {
        QString dateStr = v.value("date").toString();
        QDate d = QDate::fromString(dateStr, "yyyy-MM-dd");
        if (!d.isValid()) d = QDate::fromString(dateStr, "dd-MM-yyyy");
        if (!d.isValid()) d = QDate::fromString(dateStr, Qt::ISODate);
        if (!d.isValid()) continue;

        if (d < fromDate || d > toDate) continue;

        RawItem it;
        it.dt = d;
        it.vNo = v.value("voucher_no").toString();
        it.vType = v.value("voucher_type").toString();
        it.narr = v.value("narration").toString();
        it.dr = v.value("debit").toDouble();
        it.cr = v.value("credit").toDouble();
        sortedVchs.append(it);
    }

    std::sort(sortedVchs.begin(), sortedVchs.end(), [](const RawItem& a, const RawItem& b) {
        if (a.dt != b.dt) return a.dt < b.dt;
        return a.vNo < b.vNo;
    });

    // Populate transaction entries
    for (const auto& it : sortedVchs) {
        currentBal += (it.dr - it.cr);
        AankTransactionEntry entry;
        entry.date = it.dt;
        entry.voucherNo = it.vNo;
        entry.voucherType = it.vType;
        entry.narration = it.narr;
        entry.debitAmount = it.dr;
        entry.creditAmount = it.cr;
        entry.runningBalance = std::round(currentBal * 100.0) / 100.0;
        stmt.entries.append(entry);
    }

    // Now calculate Days and Product Aank between consecutive rows
    for (int i = 0; i < stmt.entries.size(); ++i) {
        QDate curDate = stmt.entries[i].date;
        QDate nextDate = (i + 1 < stmt.entries.size()) ? stmt.entries[i + 1].date : toDate;
        
        int days = curDate.daysTo(nextDate);
        if (days < 0) days = 0;
        
        // For the last entry on toDate, if it's the exact toDate, give at least 1 day or 0 if same day
        if (i == stmt.entries.size() - 1 && days == 0 && curDate <= toDate) {
            days = 0; // standard convention is end-of-period closing
        }
        
        stmt.entries[i].days = days;
        double bal = stmt.entries[i].runningBalance;
        double aank = std::round((bal * days) * 100.0) / 100.0;
        stmt.entries[i].productAank = aank;

        if (bal >= 0) {
            stmt.totalDrAank += aank;
            double intAmt = computeInterest(aank, drInterestRate, isLeapYearDivisor);
            stmt.entries[i].interestAmount = intAmt;
        } else {
            stmt.totalCrAank += std::abs(aank);
            double intAmt = computeInterest(std::abs(aank), crInterestRate, isLeapYearDivisor);
            stmt.entries[i].interestAmount = -intAmt;
        }
    }

    stmt.totalDrInterest = computeInterest(stmt.totalDrAank, drInterestRate, isLeapYearDivisor);
    stmt.totalCrInterest = computeInterest(stmt.totalCrAank, crInterestRate, isLeapYearDivisor);
    stmt.netInterest = std::round((stmt.totalDrInterest - stmt.totalCrInterest) * 100.0) / 100.0;

    return stmt;
}

} // namespace MahadevERP
