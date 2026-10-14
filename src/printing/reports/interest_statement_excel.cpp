#include "interest_statement_report.h"

namespace PrintReports {

QString renderInterestCalculationExcelHtml(const QString& partyName, const QString& fromDate, const QString& toDate, double annualRate) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in interest_statement_report.cpp.
    return renderInterestCalculationHtml(partyName, fromDate, toDate, annualRate);
}

} // namespace PrintReports
