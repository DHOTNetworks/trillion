#pragma once
// interest_statement — print/PDF/ODF renderer + per-target Excel + CSV builders.
// One report, one contract: edit layout here without touching
// any other report or the controller plumbing.
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariant>
#include <QVariantMap>

namespace PrintReports {
QString renderInterestCalculationHtml(const QString& partyName, const QString& fromDate, const QString& toDate, double annualRate);
QString renderInterestCalculationExcelHtml(const QString& partyName, const QString& fromDate, const QString& toDate, double annualRate);
} // namespace PrintReports
