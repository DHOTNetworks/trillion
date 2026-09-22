#pragma once

#include <QString>
#include <QDate>
#include <QVariantMap>

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

struct Gstr3BTable4ITC {
    // 4(A)(3) Inward supplies liable to reverse charge
    double iAmtA3 = 0.0;
    double cAmtA3 = 0.0;
    double sAmtA3 = 0.0;
    double csAmtA3 = 0.0;

    // 4(A)(5) All other ITC
    double iAmtA5 = 0.0;
    double cAmtA5 = 0.0;
    double sAmtA5 = 0.0;
    double csAmtA5 = 0.0;

    // 4(D)(1) As per section 17(5) (Ineligible / Blocked Credit)
    double iAmtD1 = 0.0;
    double cAmtD1 = 0.0;
    double sAmtD1 = 0.0;
    double csAmtD1 = 0.0;

    // Net ITC Available (4A - 4B)
    double netIgst = 0.0;
    double netCgst = 0.0;
    double netSgst = 0.0;
    double netCess = 0.0;
};

struct Gstr3BReturnSummary {
    QString gstin;
    QString legalName;
    QString returnPeriod; // e.g. "092026"
    QDate fromDate;
    QDate toDate;
    
    Gstr3BTable31 table31;
    Gstr3BTable4ITC table4;
    
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
        const QDate& toDate
    );
};

} // namespace MahadevERP
