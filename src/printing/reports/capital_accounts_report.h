#pragma once
// capital_accounts — print/PDF/ODF renderer + per-target Excel + CSV builders.
// One report, one contract: edit layout here without touching
// any other report or the controller plumbing.
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariant>
#include <QVariantMap>

namespace PrintReports {
QString renderCapitalAccountsHtml(const QString& fromDate, const QString& toDate);
QString renderCapitalAccountsExcelHtml(const QString& fromDate, const QString& toDate);
} // namespace PrintReports
