#include "fy_close_engine.h"
#include "../database_manager.h"
#include "../config/feature_flags.h"
#include "../config/settings_registry.h"
#include "fiscal_year_helper.h"
#include "profit_loss_calculator.h"
#include "../models/account_classifier.h"
#include "../models/vouchers_model.h"
#include <QDate>
#include <cmath>

namespace {
// Double-entry trial difference over a date range (signed Dr+, Cr-).
double signedTrialDiff(const QString& fromIso, const QString& toIso, bool* okOut = nullptr) {
    QVariant v = DatabaseManager::instance().executeScalar(
        "SELECT SUM(CASE WHEN dr_cr = 'Dr' THEN amount ELSE -amount END) "
        "FROM transactions WHERE voucher_date >= ? AND voucher_date <= ?;",
        {fromIso, toIso});
    if (!v.isValid() || v.isNull()) {
        if (okOut) *okOut = false;
        return 0.0;
    }
    if (okOut) *okOut = true;
    return std::fabs(v.toDouble());
}

QVariantMap fyRowByName(const QString& fyName) {
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT id, year_name, start_date, end_date, status, is_locked "
        "FROM financial_years WHERE year_name = ? LIMIT 1;", {fyName.trimmed()});
    return rows.isEmpty() ? QVariantMap() : rows.first().toMap();
}
} // namespace

double FyCloseEngine::trialDiff(const QString& fyName, QString* errorOut) {
    QVariantMap r = fyRowByName(fyName);
    if (r.isEmpty()) {
        if (errorOut) *errorOut = QString("Financial year '%1' not found.").arg(fyName);
        return -1.0;
    }
    bool ok = false;
    const double d = signedTrialDiff(
        FiscalYearHelper::normalizeToIso(r.value("start_date").toString()),
        FiscalYearHelper::normalizeToIso(r.value("end_date").toString()), &ok);
    if (!ok) {
        if (errorOut) *errorOut = QString("Could not read transactions for %1.").arg(fyName);
        return -1.0;
    }
    return d;
}

FyCloseResult FyCloseEngine::closeYear(const QString& fyName,
                                       const QString& capitalLedgerName,
                                       const QString& actor) {
    FyCloseResult res;
    auto fail = [&](const QString& e) { res.ok = false; res.error = e; return res; };
    auto& db = DatabaseManager::instance();
    const QString who = actor.trimmed().isEmpty() ? QString("local") : actor.trimmed();

    // Already closed once? Reopen first (prevents duplicate closings).
    QVariant closeKey = db.executeScalar(
        "SELECT value FROM app_settings WHERE key = ? LIMIT 1;",
        {SettingsKeys::fy_close_voucher(fyName)});
    if (closeKey.isValid() && !closeKey.isNull()) {
        return fail(QString("%1 was already closed by this procedure. Reopen it first.").arg(fyName));
    }

    QVariantMap r = fyRowByName(fyName);
    if (r.isEmpty()) return fail(QString("Financial year '%1' not found.").arg(fyName));
    const QString start = FiscalYearHelper::normalizeToIso(r.value("start_date").toString());
    const QString end = FiscalYearHelper::normalizeToIso(r.value("end_date").toString());
    const qint64 fyId = r.value("id").toLongLong();

    QString fyStatus = r.value("status").toString().trimmed().toLower();
    if (fyStatus != "open" && fyStatus != "soft_closed") fyStatus = r.value("is_locked").toBool() ? "locked" : "open";
    if (fyStatus == "locked") {
        return fail(QString("%1 is locked. Reopen it before re-closing.").arg(fyName));
    }
    const QString blocked = FiscalYearHelper::writeBlockReason(end);
    if (!blocked.isEmpty()) return fail(blocked);

    QString trialErr;
    const double diff = trialDiff(fyName, &trialErr);
    if (diff < 0.0) return fail(trialErr);
    if (diff >= 0.01) {
        return fail(QString("Trial imbalance %1 in %2: balance the books before closing.")
                        .arg(QString::number(diff, 'f', 2), fyName));
    }

    const double net = ProfitLossCalculator::calculate(start, end).netProfit;
    res.plNet = net;

    // Resolve ledgers (exact names, fail loud — a close must be deterministic).
    QVariantList capRows = db.executeQuery(
        "SELECT p.id AS pid, COALESCE(g.code1st, p.group_code, 0) AS c1, "
        "COALESCE(g.code2nd, 0) AS c2, COALESCE(g.code3rd, 0) AS c3, COALESCE(g.code4th, 0) AS c4 "
        "FROM parties p LEFT JOIN account_groups g ON g.id = p.group_id "
        "WHERE p.name = ? LIMIT 1;", {capitalLedgerName.trimmed()});
    if (capRows.isEmpty()) {
        return fail(QString("Capital ledger '%1' not found. Create it under Capital first.")
                        .arg(capitalLedgerName));
    }
    QVariantMap cap = capRows.first().toMap();
    if (!AccountClassifier::isDescendantOf(cap.value("c1").toInt(), cap.value("c2").toInt(),
                                           cap.value("c3").toInt(), cap.value("c4").toInt(), 1)) {
        return fail(QString("Ledger '%1' is not under Capital. Pick a capital ledger.")
                        .arg(capitalLedgerName));
    }
    QVariantList plRows = db.executeQuery(
        "SELECT id FROM parties WHERE name = 'Profit & Loss' LIMIT 1;");
    if (plRows.isEmpty()) {
        return fail(QString("Ledger 'Profit & Loss' not found. Create it first."));
    }
    const qint64 capId = cap.value("pid").toLongLong();
    const qint64 plId = plRows.first().toMap().value("id").toLongLong();

    db.beginTransaction();
    QString vchNo;
    if (std::fabs(net) >= 0.005) {
        VouchersModel vm;
        vchNo = vm.claimVoucherNo("Journal", "Jrnl", fyName);
        if (vchNo.isEmpty()) {
            db.rollback();
            return fail(QString("No free journal number in %1.").arg(fyName));
        }
        const double amt = std::fabs(net);
        // Profit (net Cr): Dr P&L / Cr Capital. Loss: the reverse.
        const QString drParty = (net > 0) ? QString("Profit & Loss") : capitalLedgerName.trimmed();
        const QString crParty = (net > 0) ? capitalLedgerName.trimmed() : QString("Profit & Loss");
        const qint64 drId = (net > 0) ? plId : capId;
        const qint64 crId = (net > 0) ? capId : plId;
        const QString ref = "FY-CLOSE-" + fyName.trimmed();
        const QString narr = QString("Year-end closing transfer %1 (net %2%3)")
                                 .arg(fyName, net > 0 ? "" : "-", QString::number(amt, 'f', 2));
        bool ok = db.executeNonQuery(
            "INSERT INTO vouchers (fy_id, financial_year, voucher_no, instrument_no, "
            "voucher_date, voucher_type, legacy_type, party_id, ledger_id, party_name, "
            "account_type, amount, narration) "
            "VALUES (?, ?, ?, ?, ?, 'Journal', 'Jrnl', ?, ?, ?, ?, ?, ?);",
            {fyId, fyName, vchNo, ref, end, drId, drId, drParty, crParty, amt, narr});
        if (ok) {
            ok = db.executeNonQuery(
                "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, "
                "voucher_type, trans_type, party_id, party_name, opposing_account, dr_cr, amount, narration) "
                "VALUES (?, ?, ?, ?, 'Journal', 'Jrnl', ?, ?, ?, 'Dr', ?, ?);",
                {fyId, fyName, vchNo, end, drId, drParty, crParty, amt, narr});
        }
        if (ok) {
            ok = db.executeNonQuery(
                "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, "
                "voucher_type, trans_type, party_id, party_name, opposing_account, dr_cr, amount, narration) "
                "VALUES (?, ?, ?, ?, 'Journal', 'Jrnl', ?, ?, ?, 'Cr', ?, ?);",
                {fyId, fyName, vchNo, end, crId, crParty, drParty, amt, narr});
        }
        if (!ok) {
            const QString e = db.lastErrorText();
            db.rollback();
            return fail(QString("Closing journal failed: %1").arg(e));
        }
        res.closingVoucherNo = vchNo;
    }

    // Ensure the next FY exists (inactive until the user switches to it).
    // Sanity bound only: garbage years (unparsable dates) must not mint FY
    // rows; every realistic year incl. far-future sandboxes passes.
    const int y = start.left(4).toInt();
    QString nextName;
    qint64 nextFyId = 0;
    if (y > 1900 && y < 10000) {
        nextName = FiscalCalendar::fyName(y + 1);
        QVariant nid = db.executeScalar(
            "SELECT id FROM financial_years WHERE year_name = ? LIMIT 1;", {nextName});
        if (nid.isValid() && !nid.isNull()) {
            nextFyId = nid.toLongLong();
        } else if (!db.executeNonQuery(
                       "INSERT INTO financial_years (year_name, start_date, end_date, "
                       "is_active, is_locked, status) VALUES (?, ?, ?, 0, 0, 'open');",
                       {nextName, FiscalCalendar::fyStartDate(y + 1),
                        FiscalCalendar::fyEndDate(y + 1)})) {
            const QString e = db.lastErrorText();
            db.rollback();
            return fail(QString("Could not create %1: %2").arg(nextName, e));
        } else {
            nextFyId = db.lastInsertedId();
        }
    }
    if (nextFyId <= 0) {
        db.rollback();
        return fail(QString("Could not resolve the year after %1.").arg(fyName));
    }

    // Snapshot balance-sheet ledgers into the new year (idempotent per year).
    if (!db.executeNonQuery("DELETE FROM fy_opening_balances WHERE fy_id = ?;", {nextFyId})) {
        const QString e = db.lastErrorText();
        db.rollback();
        return fail(QString("Snapshot reset failed: %1").arg(e));
    }
    QVariantList parties = db.executeQuery(
        "SELECT p.id AS pid, p.name AS pname, p.opening_balance AS op, p.balance_type AS bt, "
        "COALESCE(g.code1st, p.group_code, 0) AS c1, COALESCE(g.code2nd, 0) AS c2, "
        "COALESCE(g.code3rd, 0) AS c3, COALESCE(g.code4th, 0) AS c4 "
        "FROM parties p LEFT JOIN account_groups g ON g.id = p.group_id;");
    QVariantList legs = db.executeQuery(
        "SELECT party_id, party_name, "
        "SUM(CASE WHEN dr_cr = 'Dr' THEN amount ELSE -amount END) AS net "
        "FROM transactions WHERE voucher_date <= ? GROUP BY party_id, party_name;",
        {end});
    QHash<QString, double> byId, byName;
    for (const auto& lv : legs) {
        QVariantMap lm = lv.toMap();
        const qint64 pid = lm.value("party_id").toLongLong();
        if (pid > 0) byId[QString::number(pid)] += lm.value("net").toDouble();
        else byName[lm.value("party_name").toString().trimmed().toLower()] += lm.value("net").toDouble();
    }
    int snapCount = 0;
    for (const auto& pv : parties) {
        QVariantMap p = pv.toMap();
        const qint64 pid = p.value("pid").toLongLong();
        if (AccountClassifier::isNominal(p.value("c1").toInt(), p.value("c2").toInt(),
                                        p.value("c3").toInt(), p.value("c4").toInt())) {
            continue; // nominal ledgers restart at zero; no carry-forward
        }
        double netBal = (p.value("bt").toString().trimmed().compare("Dr", Qt::CaseInsensitive) == 0
                         ? p.value("op").toDouble() : -p.value("op").toDouble());
        const QString idKey = QString::number(pid);
        if (pid > 0 && byId.contains(idKey)) netBal += byId.value(idKey);
        else netBal += byName.value(p.value("pname").toString().trimmed().toLower(), 0.0);
        if (std::fabs(netBal) < 0.005) continue;
        if (!db.executeNonQuery(
                "INSERT INTO fy_opening_balances (fy_id, party_id, party_name, amount, balance_type) "
                "VALUES (?, ?, ?, ?, ?);",
                {nextFyId, pid > 0 ? QVariant(pid) : QVariant(),
                 p.value("pname").toString(), std::fabs(netBal),
                 netBal >= 0 ? QString("Dr") : QString("Cr")})) {
            const QString e = db.lastErrorText();
            db.rollback();
            return fail(QString("Snapshot failed: %1").arg(e));
        }
        ++snapCount;
    }
    res.snapshotRows = snapCount;

    QString lockErr;
    if (!FiscalYearHelper::setYearStatus(fyName, "locked", who,
                                         "Year-end close" + (vchNo.isEmpty() ? " (no P&L movement)" : " (" + vchNo + ")"),
                                         &lockErr)) {
        db.rollback();
        return fail(lockErr);
    }
    db.setSetting(SettingsKeys::fy_close_voucher(fyName), vchNo);
    db.commit();
    res.ok = true;
    return res;
}

FyCloseResult FyCloseEngine::reopenYear(const QString& fyName, const QString& actor) {
    FyCloseResult res;
    auto fail = [&](const QString& e) { res.ok = false; res.error = e; return res; };
    auto& db = DatabaseManager::instance();
    const QString who = actor.trimmed().isEmpty() ? QString("local") : actor.trimmed();

    QVariant keyRow = db.executeScalar(
        "SELECT value FROM app_settings WHERE key = ? LIMIT 1;",
        {SettingsKeys::fy_close_voucher(fyName)});
    if (!keyRow.isValid() || keyRow.isNull()) {
        return fail(QString("%1 was not closed by this procedure (no record).").arg(fyName));
    }
    const QString vchNo = keyRow.toString();
    QVariantMap r = fyRowByName(fyName);
    if (r.isEmpty()) return fail(QString("Financial year '%1' not found.").arg(fyName));
    const QString end = FiscalYearHelper::normalizeToIso(r.value("end_date").toString());
    const qint64 fyId = r.value("id").toLongLong();

    QString lockErr;
    if (!FiscalYearHelper::setYearStatus(fyName, "open", who, "FY reopen", &lockErr)) {
        return fail(lockErr); // already open counts as success path below? No: explicit is fine
    }
    if (!vchNo.isEmpty()) {
        // Skip when a reversal already exists (rerun-safe).
        QVariant rev = db.executeScalar(
            "SELECT 1 FROM vouchers WHERE voucher_no = ? AND financial_year = ? LIMIT 1;",
            {"REV-" + vchNo, fyName});
        if (!rev.isValid() || rev.isNull()) {
            const QString today = QDate::currentDate().toString("yyyy-MM-dd");
            const QString wbr = FiscalYearHelper::writeBlockReason(today);
            if (!wbr.isEmpty()) {
                FiscalYearHelper::setYearStatus(fyName, "locked", who, "reopen aborted: reversal unwritable", nullptr);
                return fail(QString("Reversal unwritable today (%1); year re-locked.").arg(wbr));
            }
            VouchersModel vm;
            const QString revNo = vm.claimVoucherNo("Journal", "Jrnl", fyName);
            if (revNo.isEmpty()) {
                FiscalYearHelper::setYearStatus(fyName, "locked", who, "reopen aborted: no number", nullptr);
                return fail(QString("No free journal number in %1; year re-locked.").arg(fyName));
            }
            // Mirror the original legs with Dr/Cr swapped.
            QVariantList legs = db.executeQuery(
                "SELECT party_id, party_name, opposing_account, dr_cr, amount, narration "
                "FROM transactions WHERE voucher_no = ? AND financial_year = ? ORDER BY id ASC;",
                {vchNo, fyName});
            if (legs.isEmpty()) {
                FiscalYearHelper::setYearStatus(fyName, "locked", who, "reopen aborted: voucher gone", nullptr);
                return fail(QString("Closing voucher %1 not found; year re-locked.").arg(vchNo));
            }
            db.beginTransaction();
            // Mirror the original header's parties: literal 0 FK-ids violate
            // vouchers.ledger_id REFERENCES parties(id) under
            // PRAGMA foreign_keys=ON — NULL or real ids only, never 0.
            QVariantMap origHead;
            {
                QVariantList hr = db.executeQuery(
                    "SELECT party_id, ledger_id, party_name, account_type "
                    "FROM vouchers WHERE voucher_no = ? AND financial_year = ? LIMIT 1;",
                    {vchNo, fyName});
                if (!hr.isEmpty()) origHead = hr.first().toMap();
            }
            if (origHead.isEmpty()) {
                db.rollback();
                FiscalYearHelper::setYearStatus(fyName, "locked", who, "reopen aborted: voucher gone", nullptr);
                return fail(QString("Closing voucher %1 not found; year re-locked.").arg(vchNo));
            }
            // FK-safe ids: NULL or real parties(id) only, never 0.
            auto fkId = [](const QVariant& v) -> QVariant {
                return (v.isNull() || v.toLongLong() <= 0) ? QVariant() : v;
            };
            const QVariant partyParam = fkId(origHead.value("party_id"));
            const QVariant ledgerParam = fkId(origHead.value("ledger_id"));
            bool ok = db.executeNonQuery(
                "INSERT INTO vouchers (fy_id, financial_year, voucher_no, instrument_no, "
                "voucher_date, voucher_type, legacy_type, party_id, ledger_id, party_name, "
                "account_type, amount, narration) "
                "VALUES (?, ?, ?, ?, ?, 'Journal', 'Jrnl', ?, ?, ?, ?, ?, ?);",
                {fyId, fyName, revNo, "REV-" + vchNo, today, partyParam, ledgerParam,
                 origHead.value("party_name").toString(),
                 origHead.value("account_type").toString(), 0.0,
                 QString("Reversal of year-end closing %1").arg(vchNo)});
            double tot = 0.0;
            for (const auto& lv : legs) {
                if (!ok) break;
                QVariantMap lm = lv.toMap();
                const QString flip = (lm.value("dr_cr").toString() == "Dr") ? "Cr" : "Dr";
                tot += lm.value("amount").toDouble();
                ok = db.executeNonQuery(
                    "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, "
                    "voucher_type, trans_type, party_id, party_name, opposing_account, dr_cr, amount, narration) "
                    "VALUES (?, ?, ?, ?, 'Journal', 'Jrnl', ?, ?, ?, ?, ?, ?);",
                    {fyId, fyName, revNo, today, lm.value("party_id").toLongLong(),
                     lm.value("party_name").toString(), lm.value("opposing_account").toString(),
                     flip, lm.value("amount").toDouble(),
                     QString("Reversal of year-end closing %1").arg(vchNo)});
            }
            if (ok) {
                // Fix the header amount to the mirrored total.
                ok = db.executeNonQuery("UPDATE vouchers SET amount = ? WHERE voucher_no = ? AND financial_year = ?;",
                                        {tot, revNo, fyName});
            }
            if (!ok) {
                const QString e = db.lastErrorText();
                db.rollback();
                FiscalYearHelper::setYearStatus(fyName, "locked", who, "reopen aborted: reversal failed", nullptr);
                return fail(QString("Reversal failed (%1); year re-locked.").arg(e));
            }
            db.commit();
            res.closingVoucherNo = revNo;
        }
    }
    db.executeNonQuery("DELETE FROM app_settings WHERE key = ?;",
                       {SettingsKeys::fy_close_voucher(fyName)});
    res.ok = true;
    return res;
}
