#pragma once
// cash_bank_flow — print/PDF/ODF renderer + per-target Excel + CSV builders.
// One report, one contract: edit layout here without touching
// any other report or the controller plumbing.
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariant>
#include <QVariantMap>

namespace PrintReports {
QString renderCashBankFlowHtml(int flowType, const QString& fromDate, const QString& toDate);
QString renderCashBankFlowExcelHtml(int flowType, const QString& fromDate, const QString& toDate);
} // namespace PrintReports
