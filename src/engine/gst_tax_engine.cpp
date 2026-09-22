#include "gst_tax_engine.h"
#include <algorithm>

namespace MahadevERP {

static double round2(double val) {
    return std::round(val * 100.0) / 100.0;
}

static double roundRupee(double val) {
    return std::round(val);
}

QString GstTaxEngine::extractStateCodeFromGstin(const QString& gstin) {
    QString trimmed = gstin.trimmed();
    if (trimmed.length() >= 2 && trimmed[0].isDigit() && trimmed[1].isDigit()) {
        return trimmed.left(2);
    }
    return "";
}

bool GstTaxEngine::isStateCodeMatch(const QString& state1, const QString& state2) {
    QString s1 = state1.trimmed().rightJustified(2, '0');
    QString s2 = state2.trimmed().rightJustified(2, '0');
    if (s1.isEmpty() || s2.isEmpty()) return false;
    return s1 == s2;
}

QString GstTaxEngine::getStateNameByCode(const QString& stateCode) {
    QString c = stateCode.trimmed().rightJustified(2, '0');
    if (c == "06") return "Haryana";
    if (c == "08") return "Rajasthan";
    if (c == "03") return "Punjab";
    if (c == "07") return "Delhi";
    if (c == "09") return "Uttar Pradesh";
    if (c == "02") return "Himachal Pradesh";
    if (c == "01") return "Jammu & Kashmir";
    if (c == "05") return "Uttarakhand";
    if (c == "24") return "Gujarat";
    if (c == "23") return "Madhya Pradesh";
    if (c == "27") return "Maharashtra";
    if (c == "10") return "Bihar";
    if (c == "19") return "West Bengal";
    if (c == "21") return "Odisha";
    if (c == "36") return "Telangana";
    if (c == "37") return "Andhra Pradesh";
    if (c == "29") return "Karnataka";
    if (c == "33") return "Tamil Nadu";
    return "Other State";
}

SupplyType GstTaxEngine::determineSupplyType(
    const QString& sellerStateCode,
    const QString& buyerStateCode,
    const QString& placeOfSupply,
    bool isExport,
    bool isSez,
    bool isLut
) {
    if (isExport) {
        return isLut ? SupplyType::ExportWOP : SupplyType::ExportWP;
    }
    if (isSez) {
        return isLut ? SupplyType::SEZWOP : SupplyType::SEZWP;
    }

    QString targetPos = placeOfSupply.trimmed().isEmpty() ? buyerStateCode.trimmed() : placeOfSupply.trimmed();
    if (isStateCodeMatch(sellerStateCode, targetPos)) {
        return SupplyType::IntraState;
    }
    return SupplyType::InterState;
}

GstTaxBreakdown GstTaxEngine::computeTax(
    double taxableAmount,
    double taxRatePct,
    const QString& sellerStateCode,
    const QString& buyerStateCode,
    const QString& placeOfSupply,
    bool applyTcs,
    double cumulativeTurnover,
    double tcsThreshold,
    bool isReverseCharge,
    double extraNonTaxable
) {
    GstTaxBreakdown res;
    res.taxableAmount = round2(taxableAmount);
    res.taxRatePct = taxRatePct;
    res.isReverseCharge = isReverseCharge;

    res.supplyType = determineSupplyType(sellerStateCode, buyerStateCode, placeOfSupply);
    res.isIntraState = (res.supplyType == SupplyType::IntraState);

    if (res.isIntraState) {
        res.cgstRate = taxRatePct / 2.0;
        res.sgstRate = taxRatePct / 2.0;
        res.igstRate = 0.0;
        
        res.cgstAmount = round2(res.taxableAmount * (res.cgstRate / 100.0));
        res.sgstAmount = round2(res.taxableAmount * (res.sgstRate / 100.0));
        res.igstAmount = 0.0;
    } else {
        res.cgstRate = 0.0;
        res.sgstRate = 0.0;
        res.igstRate = taxRatePct;
        
        res.cgstAmount = 0.0;
        res.sgstAmount = 0.0;
        res.igstAmount = round2(res.taxableAmount * (res.igstRate / 100.0));
    }

    res.totalTax = round2(res.cgstAmount + res.sgstAmount + res.igstAmount + res.cessAmount);

    // TCS Sec 206C(1H) calculation (applies if turnover exceeds 50 Lakhs)
    res.applyTcs = applyTcs;
    if (applyTcs) {
        double priorSubjectToTcs = std::max(0.0, cumulativeTurnover - tcsThreshold);
        double currentPortionTaxable = 0.0;
        if (cumulativeTurnover + res.taxableAmount > tcsThreshold) {
            currentPortionTaxable = (cumulativeTurnover >= tcsThreshold) ? res.taxableAmount : (cumulativeTurnover + res.taxableAmount - tcsThreshold);
        }
        res.tcsAmount = round2(currentPortionTaxable * (res.tcsRatePct / 100.0));
    }

    // Invoice Total and Rounding
    double rawTotal = res.taxableAmount + res.totalTax + res.tcsAmount + extraNonTaxable;
    res.finalInvoiceValue = roundRupee(rawTotal);
    res.roundOff = round2(res.finalInvoiceValue - rawTotal);

    return res;
}

} // namespace MahadevERP
