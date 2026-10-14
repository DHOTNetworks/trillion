#include "ledger_statement_report.h"

namespace PrintReports {

QString renderLedgerStatementExcelHtml(const QString& partyName, const QString& fromDate, const QString& toDate) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in ledger_statement_report.cpp.
    return renderLedgerStatementHtml(partyName, fromDate, toDate);
}

} // namespace PrintReports
