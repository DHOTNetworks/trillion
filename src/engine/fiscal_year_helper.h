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
    int partyId = 0;
    QString partyName;
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
    static QString extractFiscalYearToken(const QString& str);
    static FiscalYearInfo resolveFiscalYear(const QString& invOrVchStr, const QString& dateHint = "", const QString& explicitFy = "");
    static QList<FiscalYearInfo> getAllFiscalYears();
    static void setActiveFiscalYear(const QString& fyNameOrLabel);
    static void setActiveCustomPeriod(const QString& fromIso, const QString& toIso, const QString& label = "");

    // 2. Normalization & Formatting
    static QString normalizeToIso(const QString& dateStr);
    static QString formatDisplayDate(const QString& dateStr); // Returns DD-MM-YYYY

    // 2b. Canonical voucher identity: "{yyYY}/{Type}-{raw}" e.g. "2627/Sale-247".
    // Single source of truth for every voucher_no written (migration + all
    // controllers) so identity is exact-matchable — no prefix stripping,
    // token guessing, or substring LIKE at read time. Idempotent: canonical
    // input passes through unchanged. invoice_no is NOT canonicalized here:
    // Bahi-Khata invoices already embed FY ("MRI/2526-247", byte-identical
    // round-trip for export) and supplier bills must stay verbatim.
    static QString fyShortToken(const QString& fyName); // "FY 2026-27" -> "2627"
    static QString canonicalTypeToken(const QString& type); // Sales/Sale->Sale, Purchase->Purc, ...
    static QString canonicalVoucherNo(const QString& fyNameOrToken, const QString& type, const QString& rawNo);
    static bool parseCanonicalVoucherNo(const QString& vchNo, QString& fyTokenOut, QString& typeOut, QString& rawOut);
    static QString rawVoucherNo(const QString& vchNo); // canonical/app-prefixed -> Bahi-Khata raw ("247"); others unchanged

    // 2c. FY-ordered month selection (single source of truth for every
    // month-wise view: GSTR pages, registers, statements). Month dropdowns run
    // April(0)..March(11); Apr-Dec belong to the FY's first calendar year,
    // Jan-Mar roll into the next year automatically — no free year combo.
    static QStringList fyMonthNames(); // April..March
    static int fyMonthIndex(int calendarMonth); // 4->0 .. 12->8, 1->9 .. 3->11
    static int calendarMonthForFyIndex(int fyIdx); // 0->4 .. 8->12, 9->1 ..
    static int fyStartYear(const QString& fyName); // "FY 2026-27" -> 2026, else 0
    static int calendarYearForFyMonth(const QString& fyName, int fyIdx);
    static QDate fyMonthStart(const QString& fyName, int fyIdx);
    static QDate fyMonthEnd(const QString& fyName, int fyIdx);

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
