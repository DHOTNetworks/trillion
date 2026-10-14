#include "cheque_voucher_report.h"

namespace PrintReports {

QString renderChequeVoucherExcelHtml(const QString& voucherNo, const QString& voucherType) {
    // Excel-dedicated variant: same figures as print/PDF today.
    // Customize here per target (raw numbers + num/date classes)
    // without touching the print layout in cheque_voucher_report.cpp.
    return renderChequeVoucherHtml(voucherNo, voucherType);
}

} // namespace PrintReports
