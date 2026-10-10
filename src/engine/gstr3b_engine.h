#pragma once

#include <QString>
#include <QDate>
#include <QVariantMap>
#include <QList>
#include "gstr1_engine.h"

namespace MahadevERP {

struct Gstr3BTable31 {
    // 3.1(a) Outward taxable supplies (other than zero rated, nil rated and exempted)
    double txValA = 0.0;
    double iAmtA = 0.0;
    double cAmtA = 0.0;
    double sAmtA = 0.0;
    double csAmtA = 0.0;

    // 3.1(b) Outward taxable supplies (zero rated)
    double txValB = 0.0;
    double iAmtB = 0.0;
    double csAmtB = 0.0;

    // 3.1(c) Other outward supplies (Nil rated, exempted)
    double txValC = 0.0;

    // 3.1(d) Inward supplies (liable to reverse charge)
    double txValD = 0.0;
    double iAmtD = 0.0;
    double cAmtD = 0.0;
    double sAmtD = 0.0;
    double csAmtD = 0.0;

    // 3.1(e) Non-GST outward supplies
    double txValE = 0.0;
};

struct Gstr3BTable32Row {
    QString desc; // "Supplies made to Unregistered Persons" (composition/UIN: no source data)
    QString pos;  // "06-Haryana" display form
    double txVal = 0.0;
    double iAmt = 0.0;
};

struct Gstr3BTable32 {
    // POS-wise inter-state supplies to unregistered persons, derived from the
    // SAME GSTR-1 payload (B2CL + inter-state B2CS) so 3.2 always ties to
    // GSTR-1 Tables 5/7B (portal keeps 3.2 non-editable since July 2025).
    QList<Gstr3BTable32Row> rows;
};

struct Gstr3BTable4ITC {
    // 4(A)(1) Import of goods
    double iAmtA1 = 0.0;
    double cAmtA1 = 0.0;
    double sAmtA1 = 0.0;
    double csAmtA1 = 0.0;

    // 4(A)(2) Import of services
    double iAmtA2 = 0.0;
    double cAmtA2 = 0.0;
    double sAmtA2 = 0.0;
    double csAmtA2 = 0.0;

    // 4(A)(3) Inward supplies liable to reverse charge
    double iAmtA3 = 0.0;
    double cAmtA3 = 0.0;
    double sAmtA3 = 0.0;
    double csAmtA3 = 0.0;

    // 4(A)(4) Inward supplies from ISD
    double iAmtA4 = 0.0;
    double cAmtA4 = 0.0;
    double sAmtA4 = 0.0;
    double csAmtA4 = 0.0;

    // 4(A)(5) All other ITC
    double iAmtA5 = 0.0;
    double cAmtA5 = 0.0;
    double sAmtA5 = 0.0;
    double csAmtA5 = 0.0;

    // 4(B)(1) As per rules 38, 42 & 43 and section 17(5)
    double iAmtB1 = 0.0;
    double cAmtB1 = 0.0;
    double sAmtB1 = 0.0;
    double csAmtB1 = 0.0;

    // 4(B)(2) Others
    double iAmtB2 = 0.0;
    double cAmtB2 = 0.0;
    double sAmtB2 = 0.0;
    double csAmtB2 = 0.0;

    // Net ITC Available (4A - 4B)
    double netIgst = 0.0;
    double netCgst = 0.0;
    double netSgst = 0.0;
    double netCess = 0.0;

    // 4(D)(1) Ineligible ITC under section 17(5)
    double iAmtD1 = 0.0;
    double cAmtD1 = 0.0;
    double sAmtD1 = 0.0;
    double csAmtD1 = 0.0;

    // 4(D)(2) Ineligible ITC others / PoS rules
    double iAmtD2 = 0.0;
    double cAmtD2 = 0.0;
    double sAmtD2 = 0.0;
    double csAmtD2 = 0.0;
};

struct Gstr3BTable5 {
    // 5. Values of exempt, nil-rated and non-GST inward supplies
    double interExempt = 0.0; // From supplier under composition, Exempt and Nil rated (Inter-State)
    double intraExempt = 0.0; // From supplier under composition, Exempt and Nil rated (Intra-State)
    double interNonGst = 0.0; // Non-GST supply (Inter-State)
    double intraNonGst = 0.0; // Non-GST supply (Intra-State)
};

struct Gstr3BTable51 {
    // 5.1 Interest & Late Fee
    double interestIgst = 0.0;
    double interestCgst = 0.0;
    double interestSgst = 0.0;
    double interestCess = 0.0;
    double lateFeeCgst = 0.0;
    double lateFeeSgst = 0.0;
};

struct Gstr3BTable61 {
    // 6.1 Payment of Tax
    double taxPayableIgst = 0.0;
    double taxPayableCgst = 0.0;
    double taxPayableSgst = 0.0;
    double taxPayableCess = 0.0;

    double itcPaidIgst = 0.0;
    double itcPaidCgst = 0.0;
    double itcPaidSgst = 0.0;
    double itcPaidCess = 0.0;

    double cashPaidIgst = 0.0;
    double cashPaidCgst = 0.0;
    double cashPaidSgst = 0.0;
    double cashPaidCess = 0.0;

    double interestPaidIgst = 0.0;
    double interestPaidCgst = 0.0;
    double interestPaidSgst = 0.0;
    double interestPaidCess = 0.0;

    double lateFeePaidCgst = 0.0;
    double lateFeePaidSgst = 0.0;
};

struct Gstr3BReturnSummary {
    QString gstin;
    QString legalName;
    QString tradeName;
    QString returnPeriod; // e.g. "042026"
    QString fiscalYear;   // e.g. "2026-27"
    QString monthName;    // e.g. "April"
    QDate fromDate;
    QDate toDate;
    
    Gstr3BTable31 table31;
    Gstr3BTable32 table32;
    Gstr3BTable4ITC table4;
    Gstr3BTable5 table5;
    Gstr3BTable51 table51;
    Gstr3BTable61 table61;
    
    double netPayableIgst = 0.0;
    double netPayableCgst = 0.0;
    double netPayableSgst = 0.0;
    double netPayableTotal = 0.0;
};

class Gstr3BEngine {
public:
    static Gstr3BReturnSummary generateFromDatabase(
        const QString& gstin,
        const QString& legalName,
        const QString& stateCode,
        const QDate& fromDate,
        const QDate& toDate,
        const Gstr1ReturnPayload* gstr1 = nullptr // same-period payload -> Table 3.2
    );

    static QString findTemplatePath(const QString& preferredPath = "");

    static bool exportToExcelTemplate(
        const Gstr3BReturnSummary& summary,
        const QString& outputPath,
        const QString& templatePath = ""
    );
};

} // namespace MahadevERP
