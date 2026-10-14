#pragma once
// iform_voucher — print/PDF/ODF renderer + per-target Excel + CSV builders.
// One report, one contract: edit layout here without touching
// any other report or the controller plumbing.
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariant>
#include <QVariantMap>

namespace PrintReports {
QString renderIFormVoucherHtml(const QString& voucherNo);
QString renderIFormVoucherExcelHtml(const QString& voucherNo);
} // namespace PrintReports
