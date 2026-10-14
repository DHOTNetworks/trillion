#pragma once
// debit_credit_note — print/PDF/ODF renderer + per-target Excel + CSV builders.
// One report, one contract: edit layout here without touching
// any other report or the controller plumbing.
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariant>
#include <QVariantMap>

namespace PrintReports {
QString renderDebitCreditNoteHtml(const QString& noteNo);
QString renderDebitCreditNoteExcelHtml(const QString& noteNo);
} // namespace PrintReports
