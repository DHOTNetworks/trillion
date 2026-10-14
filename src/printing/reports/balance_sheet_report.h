#pragma once
// balance_sheet — print/PDF/ODF renderer + per-target Excel + CSV builders.
// One report, one contract: edit layout here without touching
// any other report or the controller plumbing.
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariant>
#include <QVariantMap>

namespace PrintReports {
QString renderBalanceSheetHtml(const QString& asOnDate);
QString renderBalanceSheetExcelHtml(const QString& asOnDate);
} // namespace PrintReports
