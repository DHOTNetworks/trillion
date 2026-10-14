#pragma once
// sales_invoice — print/PDF/ODF renderer + per-target Excel + CSV builders.
// One report, one contract: edit layout here without touching
// any other report or the controller plumbing.
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariant>
#include <QVariantMap>

namespace PrintReports {
QString renderSalesInvoiceSingleHtml(const QString& invoiceNo, const QString& copySubtitle);
QString renderSalesInvoiceSingleExcelHtml(const QString& invoiceNo, const QString& copySubtitle);
QString renderSalesInvoiceHtml(const QString& invoiceNo, const QString& copyType = "ORIGINAL FOR RECIPIENT");
QString renderSalesInvoiceExcelHtml(const QString& invoiceNo, const QString& copyType = "ORIGINAL FOR RECIPIENT");
} // namespace PrintReports
