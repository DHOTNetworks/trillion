#pragma once

// Centralized statement data resolvers — the ONLY query layer print/report
// modules may use for statement figures (daybook legs, trial rows, ...).
//
// Why this exists (Doc 10 §10, Doc 12): widgets and print modules each grew
// their own copy of the same SQL. One resolver = one execution plan, one
// index contract, one place to harden. Resolvers are pure functions over
// (from, to): no shared mutable state, no static caches — every call goes
// through DatabaseManager, whose QMutex serializes the shared sqlite3
// handle (SQLITE_THREADSAFE=1 build). That makes resolvers safe to invoke
// from worker threads today (future: ReportTask background rendering needs
// no resolver changes), and re-entrant from the UI thread.
//
// Performance contract: single covering-index scan per resolver
// (idx_transactions_date, idx_*_party_id_date), never N+1. Result shapes
// are plain Qt value structs — no QObjects, no widgets, freely copyable
// across threads.

#include <QString>
#include <QStringList>
#include <QDate>
#include <QVector>
#include <QHash>
#include <QVariantMap>
#include <cmath>

#include "../../database_manager.h"

namespace StatementData {

/// One double-entry leg, original side up: account = the leg's OWN account
/// (transactions.party_name), never the opposite entry.
struct Leg {
    QString date;       // ISO yyyy-MM-dd
    QString voucherNo;
    QString voucherType;
    QString transType;
    QString financialYear;
    QString account;    // ORIGINAL leg account
    double debit = 0.0;
    double credit = 0.0;
};

/// Full-range daybook legs, date-then-id order. No row cap: callers group
/// output (per-day tables) instead of truncating, so money never lies and
/// QTextDocument never chokes on one giant table.
inline QVector<Leg> dayBookLegs(const QString& fromDate, const QString& toDate) {
    QVector<Leg> out;
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT voucher_date, voucher_no, voucher_type, trans_type, "
        "COALESCE(financial_year, '') AS financial_year, "
        "COALESCE(party_name, '') AS party_name, dr_cr, amount "
        "FROM transactions WHERE voucher_date >= ? AND voucher_date <= ? "
        "ORDER BY voucher_date ASC, id ASC;",
        {fromDate, toDate});
    out.reserve(rows.size());
    for (const QVariant& v : rows) {
        const QVariantMap t = v.toMap();
        Leg l;
        l.date = t.value("voucher_date").toString();
        l.voucherNo = t.value("voucher_no").toString();
        l.voucherType = t.value("voucher_type").toString();
        l.transType = t.value("trans_type").toString();
        l.financialYear = t.value("financial_year").toString();
        l.account = t.value("party_name").toString();
        const double amt = t.value("amount").toDouble();
        if (t.value("dr_cr").toString() == "Dr") l.debit = amt;
        else l.credit = amt;
        out.append(l);
    }
    return out;
}

/// One voucher, Tally-daybook style: a SINGLE row per voucher. primary is
/// the original entry (first leg by id); opposing names the counter leg(s)
/// instead of printing them as individual rows. debit/credit are the
/// voucher's summed sides, so day/grand totals are identical to the leg
/// sums — money never lies, rows halve.
struct Voucher {
    QString date;         // ISO yyyy-MM-dd
    QString voucherNo;
    QString voucherType;
    QString primary;      // original entry account
    QString opposing;     // counter account(s), ", "-joined
    double debit = 0.0;
    double credit = 0.0;
};

/// Full-range daybook vouchers, date-then-id order. Same single indexed
/// scan as dayBookLegs; grouping is O(n) over contiguous same-voucher legs.
inline QVector<Voucher> dayBookVouchers(const QString& fromDate, const QString& toDate) {
    QVector<Voucher> out;
    const QVector<Leg> legs = dayBookLegs(fromDate, toDate);
    out.reserve(legs.size() / 2 + 1);
    for (const Leg& l : legs) {
        if (!out.isEmpty() && out.last().voucherNo == l.voucherNo
                && out.last().date == l.date) {
            Voucher& v = out.last();
            const QStringList have = v.opposing.split(", ");
            if (l.account != v.primary && !have.contains(l.account)) {
                if (!v.opposing.isEmpty()) v.opposing += ", ";
                v.opposing += l.account;
            }
            v.debit += l.debit;
            v.credit += l.credit;
        } else {
            Voucher v;
            v.date = l.date;
            v.voucherNo = l.voucherNo;
            v.voucherType = l.voucherType;
            v.primary = l.account;
            v.debit = l.debit;
            v.credit = l.credit;
            out.append(v);
        }
    }
    return out;
}

inline void dayBookTotals(const QString& fromDate, const QString& toDate,
                          double* totalDr, double* totalCr) {
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT COALESCE(SUM(CASE WHEN dr_cr = 'Dr' THEN amount ELSE 0 END), 0) AS dr, "
        "COALESCE(SUM(CASE WHEN dr_cr = 'Cr' THEN amount ELSE 0 END), 0) AS cr "
        "FROM transactions WHERE voucher_date >= ? AND voucher_date <= ?;",
        {fromDate, toDate});
    if (!rows.isEmpty()) {
        if (totalDr) *totalDr = rows.first().toMap().value("dr").toDouble();
        if (totalCr) *totalCr = rows.first().toMap().value("cr").toDouble();
    }
}

/// One trial-balance line (non-zero closing only).
struct TrialRow {
    QString account;
    QString group;
    double closeDebit = 0.0;
    double closeCredit = 0.0;
    bool isGroupHeader = false;
};

/// Trial rows as on a date. Single GROUP BY, never per-party scalars.
inline QVector<TrialRow> trialRows(const QString& asOnDate,
                                   double* totalDr = nullptr,
                                   double* totalCr = nullptr) {
    QVector<TrialRow> out;
    QVariantList partyList = DatabaseManager::instance().executeQuery(
        "SELECT p.id, p.name, p.group_name, p.opening_balance, p.balance_type "
        "FROM parties p ORDER BY p.group_name, p.name ASC;");
    QHash<int, double> sumDr, sumCr;
    QVariantList balRows = DatabaseManager::instance().executeQuery(
        "SELECT party_id, dr_cr, COALESCE(SUM(amount), 0.0) AS tot FROM transactions "
        "WHERE voucher_date <= ? AND party_id IS NOT NULL AND party_id > 0 GROUP BY party_id, dr_cr;",
        {asOnDate});
    for (const QVariant& bv : balRows) {
        const QVariantMap bm = bv.toMap();
        const int pid = bm.value("party_id").toInt();
        if (bm.value("dr_cr").toString() == "Dr") sumDr[pid] = bm.value("tot").toDouble();
        else sumCr[pid] = bm.value("tot").toDouble();
    }
    double tDr = 0.0, tCr = 0.0;
    for (const QVariant& var : partyList) {
        const QVariantMap p = var.toMap();
        const int pId = p.value("id").toInt();
        const double op = p.value("opening_balance").toDouble();
        const QString opType = p.value("balance_type").toString();
        const double net = ((opType.compare("Dr", Qt::CaseInsensitive) == 0) ? op : 0.0) + sumDr.value(pId, 0.0)
                         - ((opType.compare("Cr", Qt::CaseInsensitive) == 0) ? op : 0.0) - sumCr.value(pId, 0.0);
        if (std::abs(net) < 0.01) continue;
        TrialRow r;
        r.account = p.value("name").toString();
        r.group = p.value("group_name").toString();
        if (net > 0) { r.closeDebit = net; tDr += net; }
        else { r.closeCredit = -net; tCr += -net; }
        out.append(r);
    }
    if (totalDr) *totalDr = tDr;
    if (totalCr) *totalCr = tCr;
    return out;
}

} // namespace StatementData
