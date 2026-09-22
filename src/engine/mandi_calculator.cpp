#include "mandi_calculator.h"

namespace MahadevERP {

double MandiCalculator::round2(double val) {
    return std::round(val * 100.0) / 100.0;
}

double MandiCalculator::roundToRupee(double val) {
    return std::round(val);
}

double MandiCalculator::calculateMoistureDeduction(double grossWeightQtl, double moisturePct, double baseMoistureLimit) {
    if (moisturePct <= baseMoistureLimit) {
        return 0.0;
    }
    double excess = moisturePct - baseMoistureLimit;
    return round2(grossWeightQtl * (excess / 100.0));
}

double MandiCalculator::calculateGrossWeight(double bags, double packingKg) {
    if (bags <= 0.0 || packingKg <= 0.0) return 0.0;
    return round2(bags * (packingKg / 100.0));
}

double MandiCalculator::calculateTareWeight(double bags, double tareWeightPerBagKg) {
    if (bags <= 0.0 || tareWeightPerBagKg <= 0.0) return 0.0;
    return round2(bags * (tareWeightPerBagKg / 100.0));
}

MandiCalculationResult MandiCalculator::computeMandiTransaction(
    double bags,
    double packingKg,
    double ratePerQtl,
    double moisturePct,
    const MandiConfig& config,
    double extraTaxableCharges,
    double extraNonTaxableCharges
) {
    MandiCalculationResult res;
    res.bags = bags;
    res.packingKg = packingKg;
    res.ratePerQtl = ratePerQtl;
    
    // 1. Weights
    res.grossWeightQtl = calculateGrossWeight(bags, packingKg);
    res.tareWeightQtl = calculateTareWeight(bags, config.tareWeightPerBagKg);
    res.moistureDeductionQtl = calculateMoistureDeduction(res.grossWeightQtl, moisturePct);
    res.netWeightQtl = std::max(0.0, round2(res.grossWeightQtl - res.tareWeightQtl - res.moistureDeductionQtl));
    
    // 2. Basic Goods Amount
    double rawBasic = res.netWeightQtl * ratePerQtl;
    res.basicAmount = config.firstTotalRoundOff ? roundToRupee(rawBasic) : round2(rawBasic);
    
    // 3. Mandi Expenses & Cess
    double rawDami = res.basicAmount * (config.damiRate / 100.0);
    res.damiAmount = config.damiRoundOff ? roundToRupee(rawDami) : round2(rawDami);
    
    double rawMarketFee = res.basicAmount * (config.marketFeeRate / 100.0);
    res.marketFeeAmount = config.marketFeeRoundOff ? roundToRupee(rawMarketFee) : round2(rawMarketFee);
    
    double rawHrdf = res.basicAmount * (config.hrdfRate / 100.0);
    res.hrdfAmount = config.hrdfRoundOff ? roundToRupee(rawHrdf) : round2(rawHrdf);
    
    double rawRdf = res.basicAmount * (config.rdfRate / 100.0);
    res.rdfAmount = round2(rawRdf);
    
    // Labour (Palledari) can be per bag or per quintal
    double rawLabour = (bags * config.labourRatePerBag) + (res.netWeightQtl * config.labourRatePerQtl);
    res.labourAmount = config.labourRoundOff ? roundToRupee(rawLabour) : round2(rawLabour);
    
    res.chhanniAmount = round2(bags * config.chhanniRatePerBag);
    res.utraiAmount = round2(bags * config.utraiRatePerBag);
    res.bardanaAmount = round2(bags * config.bardanaRatePerBag);
    
    res.totalMandiExpenses = round2(
        res.damiAmount + res.marketFeeAmount + res.hrdfAmount + res.rdfAmount +
        res.labourAmount + res.chhanniAmount + res.utraiAmount + res.bardanaAmount
    );
    
    // 4. Taxable Base vs Non-Taxable
    // Dami, Labour, Bardana are typically part of Taxable Value under GST if included in contract
    res.taxableAmount = round2(res.basicAmount + res.damiAmount + res.labourAmount + res.bardanaAmount + extraTaxableCharges);
    
    // Statutory Mandi Shulk / HRDF are statutory levies often non-taxable / outside GST or billed separately
    res.nonTaxableExpenses = round2(res.marketFeeAmount + res.hrdfAmount + res.rdfAmount + res.chhanniAmount + res.utraiAmount + extraNonTaxableCharges);
    
    double unroundedTotal = res.taxableAmount + res.nonTaxableExpenses;
    if (config.finalInvoiceRoundOff) {
        res.netPayableAmount = roundToRupee(unroundedTotal);
        res.roundOff = round2(res.netPayableAmount - unroundedTotal);
    } else {
        res.netPayableAmount = round2(unroundedTotal);
        res.roundOff = 0.0;
    }
    
    return res;
}

} // namespace MahadevERP
