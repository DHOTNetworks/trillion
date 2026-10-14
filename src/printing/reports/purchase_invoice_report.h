#pragma once
// purchase_invoice — print/PDF/ODF renderer + per-target Excel + CSV builders.
// One report, one contract: edit layout here without touching
// any other report or the controller plumbing.
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariant>
#include <QVariantMap>

namespace PrintReports {
QString renderPurchaseInvoiceHtml(const QString& invoiceNo);
QString renderPurchaseInvoiceExcelHtml(const QString& invoiceNo);
} // namespace PrintReports
