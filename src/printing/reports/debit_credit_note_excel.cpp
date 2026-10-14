#include "debit_credit_note_report.h"

namespace PrintReports {

QString renderDebitCreditNoteExcelHtml(const QString& noteNo) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in debit_credit_note_report.cpp.
    return renderDebitCreditNoteHtml(noteNo);
}

} // namespace PrintReports
