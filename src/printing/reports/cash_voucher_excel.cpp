#include "cash_voucher_report.h"

namespace PrintReports {

QString renderCashVoucherExcelHtml(const QString& voucherNo, const QString& voucherType) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in cash_voucher_report.cpp.
    return renderCashVoucherHtml(voucherNo, voucherType);
}

} // namespace PrintReports
