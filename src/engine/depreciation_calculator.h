#pragma once

#include <QString>
#include <QVector>
#include <QDate>
#include <QVariantMap>

namespace MahadevERP {

struct DepreciationAssetItem {
    int ledgerId = 0;
    QString itemName;           // Ledger Name (e.g. Machinery A/c, Building A/c)
    QString groupName;          // e.g. Fixed Assets
    double depRate = 0.0;       // Depreciation % rate (e.g. 15.0, 10.0, 40.0)
    double opBalance = 0.0;     // Opening WDV
    double add1stHalf = 0.0;    // Additions before Oct 3 (>= 180 days: 100% rate)
    double add2ndHalf = 0.0;    // Additions on/after Oct 3 (< 180 days: 50% rate under Sec 32)
    double sales = 0.0;         // Sales / Disposals / Deletions deduction
    double totalBase = 0.0;     // Depreciable Base = Op + Add1 + Add2 - Sales
    double depreciation = 0.0;  // Calculated Depreciation
    double closingBalance = 0.0;// Closing WDV = Total Base - Depreciation
    bool isPosted = false;      // Whether journal voucher is posted in books
};

struct DepreciationSummaryTotals {
    int totalItems = 0;
    double totalOpBalance = 0.0;
    double totalAdd1stHalf = 0.0;
    double totalAdd2ndHalf = 0.0;
    double totalSales = 0.0;
    double totalBase = 0.0;
    double totalDepreciation = 0.0;
    double totalClosingBalance = 0.0;
};

class DepreciationCalculator {
public:
    DepreciationCalculator() = default;

    // Calculate schedule for active date range
    QVector<DepreciationAssetItem> calculateSchedule(const QDate& fromDate, const QDate& toDate, bool isDetailed = true);

    // Compute totals
    DepreciationSummaryTotals calculateTotals(const QVector<DepreciationAssetItem>& items);

    // Automated Double-Entry Book Posting
    // Generates: Dr Depreciation A/c, Cr [Asset Ledger] for each asset having depreciation > 0
    bool postDepreciationToBooks(const QVector<DepreciationAssetItem>& items,
                                 const QString& depreciationLedgerName,
                                 const QDate& voucherDate,
                                 QString& outError);

    // Deletes auto-generated depreciation journal vouchers for the period
    bool deleteDepreciationFromBooks(const QDate& fromDate, const QDate& toDate, QString& outError);

    // Checks if depreciation transactions have already been posted in books
    bool hasExistingDepreciationPostings(const QDate& fromDate, const QDate& toDate);
};

} // namespace MahadevERP
