#pragma once
// item_movement — print/PDF/ODF renderer + per-target Excel + CSV builders.
// One report, one contract: edit layout here without touching
// any other report or the controller plumbing.
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariant>
#include <QVariantMap>

namespace PrintReports {
QString renderItemMovementHtml(int itemId, const QString& fromDate, const QString& toDate);
QString renderItemMovementExcelHtml(int itemId, const QString& fromDate, const QString& toDate);
} // namespace PrintReports
