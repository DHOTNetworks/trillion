#pragma once

#include <QString>
#include <QStringList>
#include <QSet>
#include <QVariantList>
#include <QVariantMap>
#include <QRegularExpression>
#include "bill_series.h"
#include "accounting_engine.h"
#include "../database_manager.h"

/// @brief Voucher/invoice numbering integrity reports — single home for
/// duplicate detection and series-gap analysis (Tally "gap in voucher
/// numbers" parity). Read-only: never mutates, safe to run on live files.
namespace VoucherNumbering {

/// Every (voucher_type, financial_year, voucher_no) group posted more than
/// once, most-duplicated first. Each row: voucher_type, financial_year,
/// voucher_no, postings. Empty = numbering is clean.
inline QVariantList findDuplicateVoucherNos() {
    return DatabaseManager::instance().executeQuery(
        "SELECT voucher_type, financial_year, voucher_no, COUNT(*) AS postings "
        "FROM vouchers GROUP BY voucher_type, financial_year, voucher_no "
        "HAVING COUNT(*) > 1 ORDER BY postings DESC, voucher_no ASC;");
}

/// Number family embedded in a voucher_no ("2425/Pymt-31" -> "Pymt").
/// Cash (Pymt) must never be renumbered into the bank (ChPt) family and cash
/// receipts (Rcpt) never into ChRt — the families are separate sequences.
/// Falls back to voucherType when the number carries no token (bare Bahi
/// numerics), matching get_next_voucher_no's convention for that type.
inline QString familyTokenFor(const QString& voucherType, const QString& voucherNo) {
    static const QStringList kFamilies = {
        "ChPt", "Pymt", "ChRt", "Rcpt", "Jrnl", "JFrm", "IFrm",
        "Sale", "Purc", "TDS", "TCS", "DbNt", "CrNt", "Manu",
    };
    for (const QString& fam : kFamilies) {
        if (voucherNo.contains(fam, Qt::CaseInsensitive)) return fam;
    }
    return voucherType;
}

/// Missing numbers in a bill series for one FY, rendered exactly as the
/// series would print them (e.g. "MRI/2627-0013"). Capped at maxGaps so a
/// fresh FY adopted mid-history can never return millions of rows.
inline QStringList findInvoiceGapsForConfig(const BillSeriesConfig& cfg,
                                            const QString& table,
                                            const QString& fyOrDate,
                                            int maxGaps = 500) {
    QStringList gaps;
    if ((table != "sales_invoices" && table != "purchase_invoices") || maxGaps <= 0)
        return gaps;
    const QString targetFy = AccountingEngine::resolveFinancialYear(fyOrDate);
    const QString head = cfg.staticHead(targetFy);

    QVariantList rows;
    if (cfg.resetEachFy) {
        const QString fyPattern = "%" + targetFy.mid(3).trimmed() + "%";
        rows = DatabaseManager::instance().executeQuery(
            "SELECT invoice_no FROM " + table +
            " WHERE (financial_year = ? OR financial_year LIKE ?) "
            "AND invoice_no IS NOT NULL AND invoice_no != '';",
            {targetFy, fyPattern});
    } else {
        rows = DatabaseManager::instance().executeQuery(
            "SELECT invoice_no FROM " + table +
            " WHERE invoice_no IS NOT NULL AND invoice_no != '';");
    }

    static const QRegularExpression leadRe(R"(^(\d+))");
    QSet<long long> seen;
    long long top = 0;
    for (const QVariant& r : rows) {
        QString inv = r.toMap().value("invoice_no").toString().trimmed();
        if (inv.isEmpty()) continue;
        if (!head.isEmpty() && !inv.startsWith(head, Qt::CaseInsensitive)) continue;
        const QString rest = head.isEmpty() ? inv : inv.mid(head.length());
        auto m = leadRe.match(rest);
        if (!m.hasMatch()) continue;
        const long long n = m.captured(1).toLongLong();
        if (n > 0) {
            seen.insert(n);
            if (n > top) top = n;
        }
    }
    for (long long n = 1; n < top && gaps.size() < maxGaps; ++n) {
        if (!seen.contains(n)) gaps.append(cfg.render(n, targetFy));
    }
    return gaps;
}

/// Same, using the firm's effective config for docType ("sale"/"purchase").
inline QStringList findInvoiceGaps(const QString& docType, const QString& table,
                                   const QString& fyOrDate, int maxGaps = 500) {
    return findInvoiceGapsForConfig(BillSeriesManager::load(docType, table),
                                    table, fyOrDate, maxGaps);
}

} // namespace VoucherNumbering
