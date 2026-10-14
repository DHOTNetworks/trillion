#pragma once
// ledger_statement — print/PDF/ODF renderer + per-target Excel + CSV builders.
// One report, one contract: edit layout here without touching
// any other report or the controller plumbing.
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariant>
#include <QVariantMap>

namespace PrintReports {
QString renderLedgerStatementHtml(const QString& partyName, const QString& fromDate, const QString& toDate);
QString renderLedgerStatementExcelHtml(const QString& partyName, const QString& fromDate, const QString& toDate);
void buildLedgerCsvDocument(const QString& partyName, const QString& fromDate, const QString& toDate, QStringList* headersOut, QVector<QStringList>* rowsOut, QString* fileNameOut);
} // namespace PrintReports
