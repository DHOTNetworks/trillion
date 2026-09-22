#include "milling_yield_engine.h"
#include <algorithm>

namespace MahadevERP {

static double round2(double val) {
    return std::round(val * 100.0) / 100.0;
}

MillingBatchResult MillingYieldEngine::calculateBatchYield(
    const QString& batchNo,
    const QString& paddyVariety,
    double paddyInputQtl,
    double headRiceQtl,
    double brokenRiceQtl,
    double riceBranQtl,
    double riceHuskQtl,
    double standardHeadYieldPct
) {
    MillingBatchResult res;
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
        
        res.millingWastageQtl = std::max(0.0, round2(res.paddyInputQtl - res.totalOutputQtl));
        res.millingWastagePct = round2((res.millingWastageQtl / res.paddyInputQtl) * 100.0);
        
        res.yieldVariancePct = round2(res.headRicePct - standardHeadYieldPct);
        res.isWithinStandardTolerance = (std::abs(res.yieldVariancePct) <= 2.0);
    }

    return res;
}

} // namespace MahadevERP
