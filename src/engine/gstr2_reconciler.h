#pragma once

#include <QString>
#include <QDate>
#include <QList>
#include <QVariantMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace MahadevERP {

enum class MatchStatus {
    Matched,             // Fully matched on GSTIN, Invoice No, and values within +/- 1.00
    ExactMatch,          // 100% exact match on GSTIN + Inv# + Date + Tax
    RoundingMatch,       // Matched with minor difference (<= tolerance, default ₹1.00)
    ValueMismatch,       // GSTIN & InvNo match, but amounts differ
    TaxValueMismatch,    // Alias for ValueMismatch
    DateWindowMismatch,  // GSTIN, Inv# & Tax match, but invoice dates differ (> window)
    ProbableFuzzyMatch,  // GSTIN & Exact Amount match, but slight typo in Inv#
    NotInBooks,          // Invoice in Portal GSTR-2B/2A, but missing in local purchase register
    NotInPortal          // In local purchase register, but supplier has not uploaded to GSTR-2A/2B
};

struct Gstr2BookRecord {
    int voucherId = 0;
    QString supplierGstin;
    QString supplierName;
    QString invoiceNo;
    QString normalizedInvNo;
    QDate invoiceDate;
    double taxableValue = 0.0;
    double taxAmount = 0.0;
    double totalValue = 0.0;
    double igst = 0.0;
    double cgst = 0.0;
    double sgst = 0.0;
    double cess = 0.0;
};

struct Gstr2PortalRecord {
    QString supplierGstin;
    QString supplierName;
    QString invoiceNo;
    QString normalizedInvNo;
    QDate invoiceDate;
    double taxableValue = 0.0;
    double taxAmount = 0.0;
    double totalValue = 0.0;
    double igst = 0.0;
    double cgst = 0.0;
    double sgst = 0.0;
    double cess = 0.0;
    QString section = "B2B";    // 'B2B', 'B2BA', 'CDNR', 'CDNRA', 'IMPG'
    QString invoiceType = "R";  // 'R' (Regular), 'SEZWP', 'SEZWOP', 'DE', 'C', 'D'
    QString filingStatus = "Y"; // Filed by supplier
    QString itcEligibility = "Y"; // 'Y' (Eligible), 'N' (Ineligible Sec 17(5))
    QDate supplierFilingDate;
    QString returnPeriod;       // e.g. "042026"
};

struct Gstr2ReconciledItem {
    MatchStatus status = MatchStatus::NotInPortal;
    QString statusText;
    QString discrepancyRemarks;
    
    // Books Data
    int bookVoucherId = 0;
    QString bookGstin;
    QString bookSupplierName;
    QString bookInvNo;
    QDate bookInvDate;
    double bookTaxable = 0.0;
    double bookTax = 0.0;
    double bookTotal = 0.0;
    
    // Portal Data
    QString portalGstin;
    QString portalSupplierName;
    QString portalInvNo;
    QDate portalInvDate;
    double portalTaxable = 0.0;
    double portalTax = 0.0;
    double portalTotal = 0.0;
    QString portalSection = "B2B";
    QString itcEligibility = "Y";
    
    // Discrepancy
    double taxableDiff = 0.0;
    double taxDiff = 0.0;
};

struct Gstr2ReconciliationSummary {
    int totalBookRecords = 0;
    int totalPortalRecords = 0;
    int matchedCount = 0;
    int exactMatchCount = 0;
    int roundingMatchCount = 0;
    int valueMismatchCount = 0;
    int dateMismatchCount = 0;
    int probableMatchCount = 0;
    int notInBooksCount = 0;
    int notInPortalCount = 0;
    
    double matchedItcAmount = 0.0;
    double mismatchItcAmount = 0.0;
    double missingPortalItcAmount = 0.0;
    double unclaimedPortalItcAmount = 0.0;
    
    QList<Gstr2ReconciledItem> items;
};

class Gstr2Reconciler {
public:
    // Punctuation, separator, and leading zero normalization
    static QString normalizeInvoiceNumber(const QString& rawInvNo);
    static QString normalizeGstin(const QString& rawGstin);
    
    // Core 5-Pass Reconciliation Algorithm
    static Gstr2ReconciliationSummary reconcile(
        const QList<Gstr2BookRecord>& bookRecords,
        const QList<Gstr2PortalRecord>& portalRecords,
        double tolerance = 1.0,
        int dateToleranceDays = 30
    );

    // Ingest official GST Portal JSON (GSTR-2B / GSTR-2A format)
    static QList<Gstr2PortalRecord> parseGstr2BJson(const QByteArray& jsonData, const QString& returnPeriod = "");

    // Ingest official GST Portal Excel (GSTR-2B .xlsx format)
    static QList<Gstr2PortalRecord> parseGstr2BExcel(const QString& filePath, const QString& returnPeriod = "");

    // Auto-detect format (.json or .xlsx) and parse records
    static QList<Gstr2PortalRecord> loadPortalRecordsFromFile(const QString& filePath, const QString& returnPeriod = "");

    // Load purchase registers from MahadevAc database for specific date range
    static QList<Gstr2BookRecord> loadBookPurchasesFromDb(const QDate& fromDate, const QDate& toDate);

    // Load purchase registers targeted for reconciliation (selected period + specific portal-referenced suppliers)
    static QList<Gstr2BookRecord> loadBookPurchasesForReconciliation(
        const QDate& fromDate,
        const QDate& toDate,
        const QList<Gstr2PortalRecord>& portalRecords
    );

    // Load purchase registers dynamically for a return period (e.g. "042026") with lookback window
    static QList<Gstr2BookRecord> loadBookPurchasesForPeriod(const QString& returnPeriod, int lookbackMonths = 6);

    // Save reconciliation tag back into database
    static bool saveReconciliationStatus(int voucherId, MatchStatus status, const QString& remarks);
};

} // namespace MahadevERP
