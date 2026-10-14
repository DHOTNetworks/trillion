#pragma once
// stock_register — print/PDF/ODF renderer + per-target Excel + CSV builders.
// One report, one contract: edit layout here without touching
// any other report or the controller plumbing.
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariant>
#include <QVariantMap>

namespace PrintReports {
QString renderStockRegisterHtml(const QString& fromDate, const QString& toDate);
QString renderStockRegisterExcelHtml(const QString& fromDate, const QString& toDate);
void buildStockCsvDocument(const QString& fromDate, const QString& toDate, QStringList* headersOut, QVector<QStringList>* rowsOut, QString* fileNameOut);
} // namespace PrintReports
