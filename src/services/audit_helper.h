#pragma once

#include <QString>
#include <QJsonDocument>
#include <QJsonObject>

/// @brief One builder for every audit_log payload — voucher and invoice
/// mutations across all models describe themselves identically, so the
/// register, repair tools, and tests can rely on stable keys.
namespace AuditJson {

inline QString voucher(const QString& no, const QString& type, const QString& date,
                       double amount, const QString& party, const QString& fy) {
    QJsonObject o;
    o["voucher_no"] = no;
    o["voucher_type"] = type;
    o["voucher_date"] = date;
    o["amount"] = amount;
    o["party"] = party;
    o["fy"] = fy;
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

inline QString invoice(const QString& no, const QString& date, const QString& party,
                       double total, const QString& fy, const QString& kind) {
    QJsonObject o;
    o["invoice_no"] = no;
    o["invoice_date"] = date;
    o["party"] = party;
    o["amount"] = total;
    o["fy"] = fy;
    o["kind"] = kind; // "Sales" | "Purchase"
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

} // namespace AuditJson
