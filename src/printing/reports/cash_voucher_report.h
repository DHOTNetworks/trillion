#pragma once
// cash_voucher — print/PDF/ODF renderer + per-target Excel + CSV builders.
// One report, one contract: edit layout here without touching
// any other report or the controller plumbing.
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariant>
#include <QVariantMap>

namespace PrintReports {
QString renderCashVoucherHtml(const QString& voucherNo, const QString& voucherType = "Cash Payment");
QString renderCashVoucherExcelHtml(const QString& voucherNo, const QString& voucherType = "Cash Payment");
} // namespace PrintReports
