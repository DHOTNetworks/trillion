#pragma once

#include <QString>
#include <QVariantMap>
#include <cmath>
#include <algorithm>

namespace MahadevERP {

struct MandiConfig {
    double damiRate = 2.0;            // Commission % (Dami)
    double marketFeeRate = 2.0;       // Mandi Market Fee % (Shulk)
    double hrdfRate = 2.0;            // Haryana Rural Development Fund %
    double rdfRate = 0.0;             // Rural Development Fund %
    double labourRatePerBag = 4.0;    // Palledari per bag (₹)
    double labourRatePerQtl = 0.0;    // Palledari per Quintal (₹)
    double chhanniRatePerBag = 0.0;   // Sieving / Cleaning rate per bag (₹)
    double utraiRatePerBag = 0.0;     // Unloading rate per bag (₹)
    double bardanaRatePerBag = 0.0;   // Gunny Bag rate (₹)
    double tareWeightPerBagKg = 0.0;  // Tare deduction per bag in Kg
    
    // Rounding switches
    bool firstTotalRoundOff = false;  // Round basic goods value
    bool damiRoundOff = true;         // Round Dami amount
    bool marketFeeRoundOff = true;    // Round Mandi Market Fee
    bool hrdfRoundOff = true;         // Round HRDF amount
    bool labourRoundOff = true;       // Round Labour amount
    bool finalInvoiceRoundOff = true; // Round total invoice amount to nearest rupee
};

struct MandiCalculationResult {
    double bags = 0.0;
    double packingKg = 0.0;
    double grossWeightQtl = 0.0;
    double tareWeightQtl = 0.0;
    double moistureDeductionQtl = 0.0;
    double netWeightQtl = 0.0;
    double ratePerQtl = 0.0;
    
    double basicAmount = 0.0;
    double damiAmount = 0.0;
    double marketFeeAmount = 0.0;
    double hrdfAmount = 0.0;
    double rdfAmount = 0.0;
    double labourAmount = 0.0;
    double chhanniAmount = 0.0;
    double utraiAmount = 0.0;
    double bardanaAmount = 0.0;
    double totalMandiExpenses = 0.0;
    
    double taxableAmount = 0.0;
    double nonTaxableExpenses = 0.0;
    double roundOff = 0.0;
    double netPayableAmount = 0.0;
};

class MandiCalculator {
public:
    static double round2(double val);
    static double roundToRupee(double val);
    
    static double calculateMoistureDeduction(double grossWeightQtl, double moisturePct, double baseMoistureLimit = 14.0);
    static double calculateGrossWeight(double bags, double packingKg);
    static double calculateTareWeight(double bags, double tareWeightPerBagKg);
    
    static MandiCalculationResult computeMandiTransaction(
        double bags,
        double packingKg,
        double ratePerQtl,
        double moisturePct,
        const MandiConfig& config,
        double extraTaxableCharges = 0.0,
        double extraNonTaxableCharges = 0.0
    );
};

} // namespace MahadevERP
