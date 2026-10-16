#pragma once

#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <cmath>
#include "../database_manager.h"

// InvoiceTotals (S8/D11): single cable for TCS-inclusive invoice totals.
// Stored totals come in two shapes: fresh bills store TCS-INCLUSIVE grand
// totals (both sale and purchase controllers add m_tcsAmount into gross),
// while migrated Bahi-Khata bills store EXCLUSIVE totals with TCS only in
// tcs_amount and NO TCS legs (verified live on MRI/2526-1089: stored
// 1628875.00 + tcs 1628.88, exactly two legs, entry_type NULL).
// The distinguisher is the legs: fresh TCS postings always carry TCS206C
// legs (sales_model save paths); migrated ones carry none. Trial balance
// is the backstop for corrupt partial saves (a missing TCS leg unbalances
// the entry). Purchase invoices never carry TCS (their tcs_amount field is
// TDS 194-Q), so only sales need this. No schema change; stored values
// are never rewritten.
namespace InvoiceTotals {

struct Totals {
    double stored = 0.0;
    double tcs = 0.0;
    double inclusive = 0.0;
    bool includedAlready = false;
};

inline Totals saleTotals(const QString& invoiceNo) {
    Totals t;
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT total_amount, tcs_amount FROM sales_invoices "
        "WHERE invoice_no = ? ORDER BY id DESC LIMIT 1;", {invoiceNo.trimmed()});
    if (rows.isEmpty()) return t;
    QVariantMap row = rows.first().toMap();
    t.stored = row.value("total_amount").toDouble();
    t.tcs = row.value("tcs_amount").toDouble();
    t.inclusive = t.stored;
    if (t.tcs > 0.0) {
        QVariant legs = DatabaseManager::instance().executeScalar(
            "SELECT SUM(amount) FROM transactions WHERE invoice_no = ? "
            "AND entry_type = 'TCS206C';", {invoiceNo.trimmed()});
        const double legSum = (legs.isValid() && !legs.isNull()) ? legs.toDouble() : 0.0;
        if (std::fabs(legSum) > 0.005) {
            t.includedAlready = true; // fresh shape: legs prove the posting ran
        } else {
            t.inclusive = t.stored + t.tcs; // migrated shape
            t.includedAlready = false;
        }
    }
    return t;
}

} // namespace InvoiceTotals
