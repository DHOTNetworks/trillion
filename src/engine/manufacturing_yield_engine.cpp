#include "manufacturing_yield_engine.h"
#include "../services/financial_math_service.h"
#include <algorithm>

namespace MahadevERP {

namespace {
// Single-implementation alias: all manufacturing rounding is FinancialMathService::round2.
inline double round2(double val) { return FinancialMathService::round2(val); }
}

ManufacturingBatchResult ManufacturingYieldEngine::calculateBatchYield(
    const QString& batchNo,
    const QString& paddyVariety,
    double paddyInputQtl,
    double headRiceQtl,
    double brokenRiceQtl,
    double riceBranQtl,
    double riceHuskQtl,
    double standardHeadYieldPct
) {
    ManufacturingBatchResult res;
    res.batchNo = batchNo;
    res.paddyVariety = paddyVariety;
    res.paddyInputQtl = round2(paddyInputQtl);
    res.headRiceQtl = round2(headRiceQtl);
    res.brokenRiceQtl = round2(brokenRiceQtl);
    res.riceBranQtl = round2(riceBranQtl);
    res.riceHuskQtl = round2(riceHuskQtl);
    res.standardHeadYieldPct = standardHeadYieldPct;

    if (res.paddyInputQtl > 0.0) {
        res.headRicePct = round2((res.headRiceQtl / res.paddyInputQtl) * 100.0);
        res.brokenRicePct = round2((res.brokenRiceQtl / res.paddyInputQtl) * 100.0);
        res.riceBranPct = round2((res.riceBranQtl / res.paddyInputQtl) * 100.0);
        res.riceHuskPct = round2((res.riceHuskQtl / res.paddyInputQtl) * 100.0);
        
        res.totalOutputQtl = round2(res.headRiceQtl + res.brokenRiceQtl + res.riceBranQtl + res.riceHuskQtl);
        res.totalRecoveryPct = round2((res.totalOutputQtl / res.paddyInputQtl) * 100.0);
        
        res.manufacturingWastageQtl = std::max(0.0, round2(res.paddyInputQtl - res.totalOutputQtl));
        res.manufacturingWastagePct = round2((res.manufacturingWastageQtl / res.paddyInputQtl) * 100.0);
        
        res.yieldVariancePct = round2(res.headRicePct - standardHeadYieldPct);
        res.isWithinStandardTolerance = (std::abs(res.yieldVariancePct) <= 2.0);
    }

    return res;
}

} // namespace MahadevERP
