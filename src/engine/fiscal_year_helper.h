#pragma once

#include <QString>
#include <QVector>
#include <QList>
#include <QVariantMap>
#include <QVariantList>

struct FiscalYearInfo {
    QString name;        // e.g. "FY 2024-25"
    QString startDate;   // "YYYY-MM-DD" e.g. "2024-04-01"
    QString endDate;     // "YYYY-MM-DD" e.g. "2025-03-31"
    bool isActive = false;
    bool isLocked = false;

    bool isValid() const {
        return !name.isEmpty() && !startDate.isEmpty() && !endDate.isEmpty();
    }
};

struct LedgerStatementEntry {
    int id = 0;
    bool isSelected = false;
    QString vIso;
    QString vDate;
    QString refNo;
    QString voucherNo;
    QString invoiceNo;
    QString voucherType;
    QString legacyType;
    QString transType;
    QString particulars;
    double amount = 0.0;
    QString amountFmt;
    QString financialYear;
    QString side; // "Dr" or "Cr"
};

struct PartitionedLedgerData {
    FiscalYearInfo activeFy;
    QString effectiveFromDate; // ISO format (YYYY-MM-DD)
    QString effectiveToDate;   // ISO format (YYYY-MM-DD)
    
    double priorDebit = 0.0;
    double priorCredit = 0.0;
    double netOpeningBalance = 0.0; // priorDebit - priorCredit
    QString openingBalanceType = "Cr"; // "Dr" or "Cr"
    
    QVector<LedgerStatementEntry> drEntries;
    QVector<LedgerStatementEntry> crEntries;
};

class FiscalYearHelper {
public:
    // 1. Fiscal Year Resolution & Dynamic Discovery
    static void ensureFiscalYearsDiscovered();
    static FiscalYearInfo getActiveFiscalYear();
    static FiscalYearInfo getFiscalYearForDate(const QString& dateStr);
    static FiscalYearInfo getFiscalYearByName(const QString& fyName);
    static QList<FiscalYearInfo> getAllFiscalYears();

    // 2. Normalization & Formatting
    static QString normalizeToIso(const QString& dateStr);
    static QString formatDisplayDate(const QString& dateStr); // Returns DD-MM-YYYY

    // 3. Clamping & Boundary Guards (The Sorting Machine)
    static void clampDateRangeToFiscalYear(QString& fromIso, QString& toIso, const FiscalYearInfo& fy);
    static bool isDateInFiscalYear(const QString& dateStr, const FiscalYearInfo& fy);
    static bool isDatePriorTo(const QString& dateStr, const QString& boundaryIso);

    // 4. Guaranteed Zero-Leakage Transaction Partitioning
    static PartitionedLedgerData partitionPartyTransactions(
        const QString& partyName,
        const QString& requestedFromDate = "",
        const QString& requestedToDate = ""
    );
};
