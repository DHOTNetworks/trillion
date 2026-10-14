#pragma once
// trial_balance — print/PDF/ODF renderer + per-target Excel + CSV builders.
// One report, one contract: edit layout here without touching
// any other report or the controller plumbing.
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariant>
#include <QVariantMap>

namespace PrintReports {
QString renderTrialBalanceHtml(const QString& asOnDate, int mode = 0);
QString renderTrialBalanceExcelHtml(const QString& asOnDate, int mode = 0);
} // namespace PrintReports
