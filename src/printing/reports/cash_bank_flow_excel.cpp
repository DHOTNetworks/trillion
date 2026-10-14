#include "cash_bank_flow_report.h"

namespace PrintReports {

QString renderCashBankFlowExcelHtml(int flowType, const QString& fromDate, const QString& toDate) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in cash_bank_flow_report.cpp.
    return renderCashBankFlowHtml(flowType, fromDate, toDate);
}

} // namespace PrintReports
