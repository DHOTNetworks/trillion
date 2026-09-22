#pragma once

#include <QString>
#include <QVariantMap>
#include <cmath>

namespace MahadevERP {

enum class SupplyType {
    IntraState, // Same state: CGST + SGST
    InterState, // Different state: IGST
    ExportWP,   // Export with payment of tax
    ExportWOP,  // Export without payment (under LUT)
    SEZWP,      // SEZ with payment
    SEZWOP      // SEZ without payment
};

struct GstTaxBreakdown {
    SupplyType supplyType = SupplyType::IntraState;
    bool isIntraState = true;
    double taxableAmount = 0.0;
    double taxRatePct = 5.0;
    
    double cgstRate = 2.5;
    double cgstAmount = 0.0;
    
    double sgstRate = 2.5;
    double sgstAmount = 0.0;
    
    double igstRate = 0.0;
    double igstAmount = 0.0;
    
    double cessRate = 0.0;
    double cessAmount = 0.0;
    
    double totalTax = 0.0;
    
    // TCS Sec 206C(1H)
    bool applyTcs = false;
    double tcsRatePct = 0.1;
    double tcsAmount = 0.0;
    
    // TDS Sec 194Q
    bool applyTds194Q = false;
    double tdsRatePct = 0.1;
    double tdsAmount = 0.0;
    
    // RCM
    bool isReverseCharge = false;
    
    // Final invoice
    double roundOff = 0.0;
    double finalInvoiceValue = 0.0;
};

class GstTaxEngine {
public:
    static QString extractStateCodeFromGstin(const QString& gstin);
    static bool isStateCodeMatch(const QString& state1, const QString& state2);
    static QString getStateNameByCode(const QString& stateCode);
    
    static SupplyType determineSupplyType(
        const QString& sellerStateCode,
        const QString& buyerStateCode,
        const QString& placeOfSupply = "",
        bool isExport = false,
        bool isSez = false,
        bool isLut = false
    );

    static GstTaxBreakdown computeTax(
        double taxableAmount,
        double taxRatePct,
        const QString& sellerStateCode,
        const QString& buyerStateCode,
        const QString& placeOfSupply = "",
        bool applyTcs = false,
        double cumulativeTurnover = 0.0,
        double tcsThreshold = 5000000.0,
        bool isReverseCharge = false,
        double extraNonTaxable = 0.0
    );
};

} // namespace MahadevERP
