#pragma once

#include <QString>
#include <QVariantMap>
#include <cmath>

namespace MahadevERP {

struct MillingBatchResult {
    QString batchNo;
    QString paddyVariety; // e.g. "Basmati 1121", "PR-106", "1509 Steam"
    double paddyInputQtl = 0.0;
    
    // Output Recoveries
    double headRiceQtl = 0.0;
    double headRicePct = 0.0;
    
    double brokenRiceQtl = 0.0; // Tibbar / Dubar / Kinki
    double brokenRicePct = 0.0;
    
    double riceBranQtl = 0.0;
    double riceBranPct = 0.0;
    
    double riceHuskQtl = 0.0;
    double riceHuskPct = 0.0;
    
    double totalOutputQtl = 0.0;
    double totalRecoveryPct = 0.0;
    
    double millingWastageQtl = 0.0;
    double millingWastagePct = 0.0;
    
    // Quality & Benchmark comparison
    double standardHeadYieldPct = 67.0;
    double yieldVariancePct = 0.0;
    bool isWithinStandardTolerance = true;
};

class MillingYieldEngine {
public:
    static MillingBatchResult calculateBatchYield(
        const QString& batchNo,
        const QString& paddyVariety,
        double paddyInputQtl,
        double headRiceQtl,
        double brokenRiceQtl,
        double riceBranQtl,
        double riceHuskQtl,
        double standardHeadYieldPct = 67.0
    );
};

} // namespace MahadevERP
