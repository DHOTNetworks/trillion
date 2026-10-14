#include "journal_voucher_report.h"

namespace PrintReports {

QString renderJournalVoucherExcelHtml(const QString& voucherNo) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in journal_voucher_report.cpp.
    return renderJournalVoucherHtml(voucherNo);
}

} // namespace PrintReports
