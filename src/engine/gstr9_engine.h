#pragma once

#include <QString>
#include <QVariantMap>
#include <QList>

namespace MahadevERP {

struct Gstr9Table4_OutwardTaxable {
    double b2bTaxable = 0.0, b2bIgst = 0.0, b2bCgst = 0.0, b2bSgst = 0.0;
    double b2cTaxable = 0.0, b2cIgst = 0.0, b2cCgst = 0.0, b2cSgst = 0.0;
    double exportTaxable = 0.0, exportIgst = 0.0;
    double sezTaxable = 0.0, sezIgst = 0.0;
    double rcmTaxable = 0.0, rcmIgst = 0.0, rcmCgst = 0.0, rcmSgst = 0.0;
    double cdnrTaxable = 0.0, cdnrIgst = 0.0, cdnrCgst = 0.0, cdnrSgst = 0.0;
    double cdnurTaxable = 0.0, cdnurIgst = 0.0, cdnurCgst = 0.0, cdnurSgst = 0.0;
    
    // Sub-totals
    double totalTaxable = 0.0;
    double totalIgst = 0.0;
    double totalCgst = 0.0;
    double totalSgst = 0.0;
};

struct Gstr9Table5_OutwardExempt {
    double exportWithoutTax = 0.0;
    double sezWithoutTax = 0.0;
    double nilRated = 0.0;
    double exempted = 0.0;
    double nonGst = 0.0;
    double totalExempt = 0.0;
};

struct Gstr9Table6_ItcAvailed {
    double gstr3bTotalItc = 0.0;
    double b2bInputsIgst = 0.0, b2bInputsCgst = 0.0, b2bInputsSgst = 0.0;
    double b2bCapitalIgst = 0.0, b2bCapitalCgst = 0.0, b2bCapitalSgst = 0.0;
    double b2bServicesIgst = 0.0, b2bServicesCgst = 0.0, b2bServicesSgst = 0.0;
    double rcmRegisteredIgst = 0.0, rcmRegisteredCgst = 0.0, rcmRegisteredSgst = 0.0;
    double rcmUnregisteredIgst = 0.0, rcmUnregisteredCgst = 0.0, rcmUnregisteredSgst = 0.0;
    double importGoodsIgst = 0.0;
    double totalItcAvailed = 0.0;
};

struct Gstr9Table7_ItcReversed {
    double rule37Reversal = 0.0;
    double rule39Reversal = 0.0;
    double rule42Reversal = 0.0;
    double rule43Reversal = 0.0;
    double sec17Blocked = 0.0;
    double totalItcReversed = 0.0;
    double netItcAvailable = 0.0;
};

struct Gstr9Table8_Reconciliation {
    double gstr2aItc = 0.0;
    double table6bItc = 0.0;
    double difference = 0.0; // 2A - 6B
    double itcAvailableNotAvailed = 0.0;
    double itcIneligible = 0.0;
};

struct Gstr9Table9_TaxPaid {
    double integratedTaxPayable = 0.0, integratedTaxPaidCash = 0.0, integratedTaxPaidItc = 0.0;
    double centralTaxPayable = 0.0, centralTaxPaidCash = 0.0, centralTaxPaidItc = 0.0;
    double stateTaxPayable = 0.0, stateTaxPaidCash = 0.0, stateTaxPaidItc = 0.0;
    double interestPaid = 0.0;
    double lateFeePaid = 0.0;
};

struct Gstr9AnnualSummary {
    QString gstin;
    QString legalName;
    QString tradeName;
    QString financialYear;
    
    Gstr9Table4_OutwardTaxable table4;
    Gstr9Table5_OutwardExempt table5;
    Gstr9Table6_ItcAvailed table6;
    Gstr9Table7_ItcReversed table7;
    Gstr9Table8_Reconciliation table8;
    Gstr9Table9_TaxPaid table9;
};

class Gstr9Engine {
public:
    static Gstr9AnnualSummary computeAnnualReturn(const QString& financialYear = "FY 2025-26");
    static QString exportGstr9Json(const Gstr9AnnualSummary& summary);
    static QString exportGstr9Csv(const Gstr9AnnualSummary& summary);
};

} // namespace MahadevERP
