#include "trial_balance_report.h"

namespace PrintReports {

QString renderTrialBalanceExcelHtml(const QString& asOnDate, int mode) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in trial_balance_report.cpp.
    return renderTrialBalanceHtml(asOnDate, mode);
}

} // namespace PrintReports
