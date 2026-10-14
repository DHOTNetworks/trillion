#pragma once
// day_book — print/PDF/ODF renderer + per-target Excel + CSV builders.
// One report, one contract: edit layout here without touching
// any other report or the controller plumbing.
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariant>
#include <QVariantMap>

namespace PrintReports {
QString renderDayBookHtml(const QString& fromDate, const QString& toDate);
QString renderDayBookExcelHtml(const QString& fromDate, const QString& toDate);
} // namespace PrintReports
