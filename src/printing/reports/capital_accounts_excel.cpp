#include "capital_accounts_report.h"

namespace PrintReports {

QString renderCapitalAccountsExcelHtml(const QString& fromDate, const QString& toDate) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in capital_accounts_report.cpp.
    return renderCapitalAccountsHtml(fromDate, toDate);
}

} // namespace PrintReports
