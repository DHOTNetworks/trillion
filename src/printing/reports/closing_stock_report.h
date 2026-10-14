#pragma once
// closing_stock — print/PDF/ODF renderer + per-target Excel + CSV builders.
// One report, one contract: edit layout here without touching
// any other report or the controller plumbing.
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariant>
#include <QVariantMap>

namespace PrintReports {
QString renderClosingStockHtml(const QString& asOnDate);
QString renderClosingStockExcelHtml(const QString& asOnDate);
} // namespace PrintReports
