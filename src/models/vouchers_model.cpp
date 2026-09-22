#include "vouchers_model.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"
#include <QDate>
#include <QRegularExpression>

VouchersModel::VouchersModel(QObject* parent)
    : BaseTableModel(
        {"Voucher No", "Date", "Type", "Party", "Account", "Amount (₹)", "Narration"},
        {"voucher_no", "voucher_date", "voucher_type", "party_name", "account_type", "amount", "narration"},
        parent
    )
{
    reload_data();
}

void VouchersModel::reload_data() {
    beginResetModel();
    m_data = DatabaseManager::instance().executeQuery("SELECT * FROM vouchers ORDER BY id DESC;");
    endResetModel();
    emit dataChangedSignal();
    emit countChanged();
}

QString VouchersModel::get_next_voucher_no(const QString& v_type, const QString& fy) {
    QString prefix = v_type.isEmpty() ? "ChPt" : v_type;
    QString targetFy = AccountingEngine::resolveFinancialYear(fy);
    QString fyPattern = "%" + targetFy.mid(3).trimmed() + "%";

    QStringList aliases;
    aliases << prefix;
    if (prefix == "ChPt" || prefix == "Pymt" || prefix == "Payment") {
        aliases << "ChPt" << "Pymt" << "Payment" << "Paym";
    } else if (prefix == "ChRt" || prefix == "Rcpt" || prefix == "Receipt") {
        aliases << "ChRt" << "Rcpt" << "Receipt" << "Rece";
    } else if (prefix == "Jrnl" || prefix == "Jour" || prefix == "Journal") {
        aliases << "Jrnl" << "Jour" << "Journal";
    } else if (prefix == "Purc" || prefix == "Purchase") {
        aliases << "Purc" << "Purchase";
    } else if (prefix == "Sale" || prefix == "Sales") {
        aliases << "Sale" << "Sales";
    }
    aliases.removeDuplicates();

    QStringList whereClauses;
    QVariantList params;
    for (const QString& a : aliases) {
        whereClauses << "voucher_type = ?" << "voucher_no LIKE ?";
        params << a << (a + "-%");
    }

    QString sql = QString("SELECT voucher_no FROM vouchers WHERE (%1) AND (financial_year = ? OR financial_year LIKE ?);")
                      .arg(whereClauses.join(" OR "));
    params << targetFy << fyPattern;

    QVariantList rows = DatabaseManager::instance().executeQuery(sql, params);

    long long maxId = 0;
    QRegularExpression re("(\\d+)$");
    for (const QVariant& r : rows) {
        QString v = r.toMap().value("voucher_no").toString().trimmed();
        if (v.isEmpty()) continue;
        QRegularExpressionMatch m = re.match(v);
        if (m.hasMatch()) {
            long long num = m.captured(1).toLongLong();
            if (num > maxId) maxId = num;
        }
    }
    return QString("%1-%2").arg(prefix, QString::number(maxId + 1));
}

bool VouchersModel::add_voucher(const QString& vch_type, const QString& party_name, const QString& vch_date, const QString& account_type, double amount, const QString& narration) {
    QString dt = vch_date.isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : vch_date;
    QVariantList fyRows = DatabaseManager::instance().executeQuery(
        "SELECT id, year_name FROM financial_years WHERE start_date <= ? AND end_date >= ? LIMIT 1;",
        {dt, dt}
    );
    int fyId = 28;
    QString fyLabel = "FY 2026-27";
    if (!fyRows.isEmpty()) {
        fyId = fyRows.first().toMap().value("id").toInt();
        fyLabel = fyRows.first().toMap().value("year_name").toString();
    }

    QString vchNo = get_next_voucher_no(vch_type, fyLabel);

    QVariant partyRow = DatabaseManager::instance().executeScalar("SELECT id FROM parties WHERE name = ? LIMIT 1;", {party_name});
    int partyId = partyRow.isValid() ? partyRow.toInt() : 1;

    DatabaseManager::instance().beginTransaction();
    bool ok = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO vouchers (fy_id, financial_year, voucher_no, voucher_date, voucher_type, legacy_type, party_id, ledger_id, party_name, account_type, amount, narration) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {fyId, fyLabel, vchNo, dt, vch_type, vch_type, partyId, partyId, party_name, account_type, amount, narration}
    );

    if (ok) {
        // Insert into transactions
        QString rawType = (vch_type == "Payment" ? "ChPt" : (vch_type == "Receipt" ? "ChRt" : vch_type));
        QString drCr = (vch_type == "Payment" ? "Dr" : "Cr");
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, party_id, party_name, opposing_account, dr_cr, amount, narration) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {fyId, fyLabel, vchNo, dt, vch_type, rawType, partyId, party_name, account_type, drCr, amount, narration}
        );
        DatabaseManager::instance().commit();
        reload_data();
    } else {
        DatabaseManager::instance().rollback();
    }
    return ok;
}

bool VouchersModel::add_cheque_voucher(const QString& vch_type, const QString& dr_party, const QString& cr_party, double amount, const QString& chq_no, const QString& narration, const QString& vch_date) {
    QString dt = vch_date.isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : vch_date;
    QVariantList fyRows = DatabaseManager::instance().executeQuery(
        "SELECT id, year_name FROM financial_years WHERE start_date <= ? AND end_date >= ? LIMIT 1;",
        {dt, dt}
    );
    int fyId = 28;
    QString fyLabel = "FY 2026-27";
    if (!fyRows.isEmpty()) {
        fyId = fyRows.first().toMap().value("id").toInt();
        fyLabel = fyRows.first().toMap().value("year_name").toString();
    }

    QString vchNo = get_next_voucher_no(vch_type, fyLabel);
    QString fullNarr = chq_no.isEmpty() ? "" : ("Ch. No. " + chq_no);
    if (!narration.isEmpty()) {
        fullNarr += fullNarr.isEmpty() ? narration : (" | " + narration);
    }

    QVariant partyRow = DatabaseManager::instance().executeScalar("SELECT id FROM parties WHERE name = ? COLLATE NOCASE LIMIT 1;", {dr_party});
    if (!partyRow.isValid()) {
        partyRow = DatabaseManager::instance().executeScalar("SELECT id FROM parties WHERE alias = ? COLLATE NOCASE OR name LIKE ? LIMIT 1;", {dr_party, "%" + dr_party + "%"});
    }
    int partyId = partyRow.isValid() ? partyRow.toInt() : 1;

    DatabaseManager::instance().beginTransaction();
    bool ok = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO vouchers (fy_id, financial_year, voucher_no, instrument_no, voucher_date, voucher_type, legacy_type, party_id, ledger_id, party_name, account_type, amount, narration) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {fyId, fyLabel, vchNo, chq_no, dt, vch_type, vch_type, partyId, partyId, dr_party, cr_party, amount, fullNarr}
    );

    if (ok) {
        // Multi-leg transaction entries
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, party_id, party_name, opposing_account, dr_cr, amount, narration) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, 'Dr', ?, ?);",
            {fyId, fyLabel, vchNo, dt, vch_type, vch_type, partyId, dr_party, cr_party, amount, fullNarr}
        );
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, party_id, party_name, opposing_account, dr_cr, amount, narration) "
            "VALUES (?, ?, ?, ?, ?, ?, 0, ?, ?, 'Cr', ?, ?);",
            {fyId, fyLabel, vchNo, dt, vch_type, vch_type, cr_party, dr_party, amount, fullNarr}
        );
        DatabaseManager::instance().commit();
        reload_data();
    } else {
        DatabaseManager::instance().rollback();
    }
    return ok;
}

bool VouchersModel::add_journal_voucher(const QString& dr_party, const QString& cr_party, double amount, const QString& ref_no, const QString& narration, const QString& vch_date, const QString& vch_type) {
    QString dt = vch_date.isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : vch_date;
    QVariantList fyRows = DatabaseManager::instance().executeQuery(
        "SELECT id, year_name FROM financial_years WHERE start_date <= ? AND end_date >= ? LIMIT 1;",
        {dt, dt}
    );
    int fyId = 28;
    QString fyLabel = "FY 2026-27";
    if (!fyRows.isEmpty()) {
        fyId = fyRows.first().toMap().value("id").toInt();
        fyLabel = fyRows.first().toMap().value("year_name").toString();
    }

    QString vchNo = get_next_voucher_no("Jrnl", fyLabel);
    QString fullNarr = ref_no.isEmpty() ? "" : ("Ref: " + ref_no);
    if (!narration.isEmpty()) {
        fullNarr += fullNarr.isEmpty() ? narration : (" | " + narration);
    }

    QVariant partyRow = DatabaseManager::instance().executeScalar("SELECT id FROM parties WHERE name = ? COLLATE NOCASE LIMIT 1;", {dr_party});
    if (!partyRow.isValid()) {
        partyRow = DatabaseManager::instance().executeScalar("SELECT id FROM parties WHERE alias = ? COLLATE NOCASE OR name LIKE ? LIMIT 1;", {dr_party, "%" + dr_party + "%"});
    }
    int partyId = partyRow.isValid() ? partyRow.toInt() : 1;

    DatabaseManager::instance().beginTransaction();
    bool ok = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO vouchers (fy_id, financial_year, voucher_no, instrument_no, voucher_date, voucher_type, legacy_type, party_id, ledger_id, party_name, account_type, amount, narration) "
        "VALUES (?, ?, ?, ?, ?, ?, 'Jrnl', ?, ?, ?, ?, ?, ?);",
        {fyId, fyLabel, vchNo, ref_no, dt, vch_type, partyId, partyId, dr_party, cr_party, amount, fullNarr}
    );

    if (ok) {
        // Multi-leg transaction entries
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, party_id, party_name, opposing_account, dr_cr, amount, narration) "
            "VALUES (?, ?, ?, ?, 'Journal', 'Jrnl', ?, ?, ?, 'Dr', ?, ?);",
            {fyId, fyLabel, vchNo, dt, partyId, dr_party, cr_party, amount, fullNarr}
        );
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, party_id, party_name, opposing_account, dr_cr, amount, narration) "
            "VALUES (?, ?, ?, ?, 'Journal', 'Jrnl', 0, ?, ?, 'Cr', ?, ?);",
            {fyId, fyLabel, vchNo, dt, cr_party, dr_party, amount, fullNarr}
        );
        DatabaseManager::instance().commit();
        reload_data();
    } else {
        DatabaseManager::instance().rollback();
    }
    return ok;
}

QVariantMap VouchersModel::get_voucher(const QVariant& vchNoOrId, const QString& dateHint, const QString& partyHint) {
    QString q = vchNoOrId.toString().trimmed();
    if (q.isEmpty() && dateHint.isEmpty() && partyHint.isEmpty()) return {};

    static const QRegularExpression prefixRe(QStringLiteral("^(Sale|Sales|Purc|Purchase|Pur|Jrnl|Journal|ChPt|ChRt|Pymt|Rcpt|TDS|JFrm|J-Form)[-\\s#]*"), QRegularExpression::CaseInsensitiveOption);
    QString cleanQ = q;
    cleanQ = cleanQ.remove(prefixRe).trimmed();

    QString isoDate = FiscalYearHelper::normalizeToIso(dateHint);
    bool isNum = false;
    int numId = cleanQ.toInt(&isNum);

    QVariantList rows;

    // 0. If dateHint provided and exact voucher_no matches on that date
    if (!isoDate.isEmpty() && (!q.isEmpty() || !cleanQ.isEmpty())) {
        rows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM vouchers WHERE (voucher_no = ? OR voucher_no = ?) AND voucher_date = ? ORDER BY id DESC LIMIT 1;",
            {q, cleanQ, isoDate}
        );
        if (rows.isEmpty()) {
            rows = DatabaseManager::instance().executeQuery(
                "SELECT * FROM transactions WHERE (voucher_no = ? OR voucher_no = ?) AND voucher_date = ? ORDER BY id DESC LIMIT 1;",
                {q, cleanQ, isoDate}
            );
        }
    }

    // 0b. If dateHint provided and matches by Financial Year
    if (rows.isEmpty() && !isoDate.isEmpty() && (!q.isEmpty() || !cleanQ.isEmpty())) {
        FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(isoDate);
        if (fy.isValid()) {
            rows = DatabaseManager::instance().executeQuery(
                "SELECT * FROM vouchers WHERE (voucher_no = ? OR voucher_no = ?) AND (financial_year = ? OR (voucher_date >= ? AND voucher_date <= ?)) ORDER BY id DESC LIMIT 1;",
                {q, cleanQ, fy.name, fy.startDate, fy.endDate}
            );
            if (rows.isEmpty()) {
                rows = DatabaseManager::instance().executeQuery(
                    "SELECT * FROM transactions WHERE (voucher_no = ? OR voucher_no = ?) AND (financial_year = ? OR (voucher_date >= ? AND voucher_date <= ?)) ORDER BY id DESC LIMIT 1;",
                    {q, cleanQ, fy.name, fy.startDate, fy.endDate}
                );
            }
        }
    }

    // 1. If exact numeric ID in vouchers or transactions
    if (rows.isEmpty() && isNum && numId > 0) {
        rows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM vouchers WHERE id = ? LIMIT 1;",
            {numId}
        );
        if (rows.isEmpty()) {
            rows = DatabaseManager::instance().executeQuery(
                "SELECT * FROM transactions WHERE id = ? LIMIT 1;",
                {numId}
            );
        }
    }

    // 2. Lookup by voucher_no fallback
    if (rows.isEmpty() && (!q.isEmpty() || !cleanQ.isEmpty())) {
        rows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM vouchers WHERE voucher_no = ? OR voucher_no = ? ORDER BY voucher_date DESC, id DESC LIMIT 1;",
            {q, cleanQ}
        );
        if (rows.isEmpty()) {
            rows = DatabaseManager::instance().executeQuery(
                "SELECT * FROM transactions WHERE voucher_no = ? OR voucher_no = ? ORDER BY voucher_date DESC, id DESC LIMIT 1;",
                {q, cleanQ}
            );
        }
    }

    if (!rows.isEmpty()) return rows.first().toMap();
    return {};
}

QVariantMap VouchersModel::get_cheque_voucher(const QVariant& vchNoOrId, const QString& dateHint, const QString& partyHint) {
    QVariantMap v = get_voucher(vchNoOrId, dateHint, partyHint);
    if (v.isEmpty()) return {};

    QString vNo = v.value("voucher_no").toString();
    QString vDate = v.value("voucher_date").toString();

    QVariantList txRows;
    if (!vDate.isEmpty()) {
        txRows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM transactions WHERE voucher_no = ? AND voucher_date = ? ORDER BY id ASC;",
            {vNo, vDate}
        );
    }
    if (txRows.isEmpty()) {
        txRows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM transactions WHERE voucher_no = ? ORDER BY id ASC;",
            {vNo}
        );
    }

    QVariantList items;
    if (!txRows.isEmpty()) {
        for (const auto& tr : txRows) {
            QVariantMap t = tr.toMap();
            QVariantMap item;
            item["drcr"] = t.value("dr_cr").toString().trimmed().isEmpty() ? "Dr" : t.value("dr_cr").toString();
            item["ledgerName"] = t.value("party_name").toString();
            double amt = t.value("amount").toDouble();
            if (item["drcr"] == "Dr") {
                item["debitAmt"] = QString::number(amt, 'f', 2);
                item["creditAmt"] = "";
            } else {
                item["debitAmt"] = "";
                item["creditAmt"] = QString::number(amt, 'f', 2);
            }
            item["refNo"] = t.value("invoice_no").toString();
            items.append(item);
        }
    } else {
        // Fallback to 2-row standard Cheque Voucher
        QVariantMap drItem;
        drItem["drcr"] = "Dr";
        drItem["ledgerName"] = v.value("party_name").toString();
        drItem["debitAmt"] = QString::number(v.value("amount").toDouble(), 'f', 2);
        drItem["creditAmt"] = "";
        drItem["refNo"] = v.value("instrument_no").toString();
        items.append(drItem);

        QVariantMap crItem;
        crItem["drcr"] = "Cr";
        crItem["ledgerName"] = v.value("account_type").toString();
        crItem["debitAmt"] = "";
        crItem["creditAmt"] = QString::number(v.value("amount").toDouble(), 'f', 2);
        crItem["refNo"] = v.value("instrument_no").toString();
        items.append(crItem);
    }
    v["rows"] = items;
    return v;
}

QVariantMap VouchersModel::get_journal_voucher(const QVariant& vchNoOrId, const QString& dateHint, const QString& partyHint) {
    QVariantMap v = get_voucher(vchNoOrId, dateHint, partyHint);
    if (v.isEmpty()) return {};

    QString vNo = v.value("voucher_no").toString();
    QString vDate = v.value("voucher_date").toString();

    QVariantList txRows;
    if (!vDate.isEmpty()) {
        txRows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM transactions WHERE voucher_no = ? AND voucher_date = ? ORDER BY id ASC;",
            {vNo, vDate}
        );
    }
    if (txRows.isEmpty()) {
        txRows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM transactions WHERE voucher_no = ? ORDER BY id ASC;",
            {vNo}
        );
    }

    QVariantList items;
    if (!txRows.isEmpty()) {
        for (const auto& tr : txRows) {
            QVariantMap t = tr.toMap();
            QVariantMap item;
            item["drcr"] = t.value("dr_cr").toString().trimmed().isEmpty() ? "Dr" : t.value("dr_cr").toString();
            item["ledgerName"] = t.value("party_name").toString();
            double amt = t.value("amount").toDouble();
            if (item["drcr"] == "Dr") {
                item["debitAmt"] = QString::number(amt, 'f', 2);
                item["creditAmt"] = "";
            } else {
                item["debitAmt"] = "";
                item["creditAmt"] = QString::number(amt, 'f', 2);
            }
            item["refNo"] = t.value("invoice_no").toString();
            items.append(item);
        }
    } else {
        QVariantMap drItem;
        drItem["drcr"] = "Dr";
        drItem["ledgerName"] = v.value("party_name").toString();
        drItem["debitAmt"] = QString::number(v.value("amount").toDouble(), 'f', 2);
        drItem["creditAmt"] = "";
        drItem["refNo"] = v.value("instrument_no").toString();
        items.append(drItem);

        QVariantMap crItem;
        crItem["drcr"] = "Cr";
        crItem["ledgerName"] = v.value("account_type").toString();
        crItem["debitAmt"] = "";
        crItem["creditAmt"] = QString::number(v.value("amount").toDouble(), 'f', 2);
        crItem["refNo"] = v.value("instrument_no").toString();
        items.append(crItem);
    }
    v["rows"] = items;
    return v;
}

bool VouchersModel::save_multi_row_voucher(
    int editId,
    const QString& vch_type,
    const QString& vch_no,
    const QString& vch_date,
    const QString& narration,
    const QVariantList& rows
) {
    if (rows.size() < 2) return false;

    // Normalize date
    QString dt = vch_date.trimmed();
    if (dt.contains("-") || dt.contains(".")) {
        QString s = dt;
        s.replace(".", "-");
        QStringList pts = s.split("-");
        if (pts.size() == 3) {
            if (pts[0].length() == 4) {
                dt = QString("%1-%2-%3").arg(pts[0], pts[1].rightJustified(2, '0'), pts[2].rightJustified(2, '0'));
            } else if (pts[2].length() == 4) {
                dt = QString("%1-%2-%3").arg(pts[2], pts[1].rightJustified(2, '0'), pts[0].rightJustified(2, '0'));
            }
        }
    }
    if (dt.isEmpty()) dt = QDate::currentDate().toString("yyyy-MM-dd");

    QVariantList fyRows = DatabaseManager::instance().executeQuery(
        "SELECT id, year_name FROM financial_years WHERE start_date <= ? AND end_date >= ? LIMIT 1;",
        {dt, dt}
    );
    int fyId = 28;
    QString fyLabel = "FY 2026-27";
    if (!fyRows.isEmpty()) {
        fyId = fyRows.first().toMap().value("id").toInt();
        fyLabel = fyRows.first().toMap().value("year_name").toString();
    }

    QString activeVchNo = vch_no.trimmed();
    if (activeVchNo.isEmpty()) {
        activeVchNo = get_next_voucher_no(vch_type, fyLabel);
    }

    // Extract primary Dr and Cr parties and total amount
    QString primaryDrParty, primaryCrParty, primaryRef;
    double totalAmount = 0.0;
    for (const auto& rVar : rows) {
        QVariantMap r = rVar.toMap();
        QString drcr = r.value("drcr").toString();
        QString party = r.value("ledgerName").toString().trimmed();
        double dAmt = r.value("debitAmt").toDouble();
        double cAmt = r.value("creditAmt").toDouble();
        QString ref = r.value("refNo").toString().trimmed();
        if (!ref.isEmpty() && primaryRef.isEmpty()) primaryRef = ref;

        if (drcr == "Dr" && !party.isEmpty()) {
            if (primaryDrParty.isEmpty()) primaryDrParty = party;
            totalAmount += dAmt;
        } else if (drcr == "Cr" && !party.isEmpty()) {
            if (primaryCrParty.isEmpty()) primaryCrParty = party;
        }
    }

    QString rawType = (vch_type == "Payment" ? "ChPt" : (vch_type == "Receipt" ? "ChRt" : (vch_type == "Journal" ? "Jrnl" : vch_type)));

    QVariant pRow = DatabaseManager::instance().executeScalar(
        "SELECT id FROM parties WHERE name = ? COLLATE NOCASE OR alias = ? COLLATE NOCASE LIMIT 1;",
        {primaryDrParty, primaryDrParty}
    );
    int pId = pRow.isValid() ? pRow.toInt() : 1;

    DatabaseManager::instance().beginTransaction();

    if (editId > 0) {
        // Fetch previous voucher number to clean up old transactions
        QVariant oldVchNoVar = DatabaseManager::instance().executeScalar(
            "SELECT voucher_no FROM vouchers WHERE id = ? LIMIT 1;",
            {editId}
        );
        QString oldVchNo = oldVchNoVar.isValid() ? oldVchNoVar.toString() : activeVchNo;

        // Update vouchers record
        DatabaseManager::instance().executeNonQuery(
            "UPDATE vouchers SET fy_id = ?, financial_year = ?, voucher_no = ?, instrument_no = ?, "
            "voucher_date = ?, voucher_type = ?, legacy_type = ?, party_id = ?, ledger_id = ?, party_name = ?, "
            "account_type = ?, amount = ?, narration = ? WHERE id = ?;",
            {fyId, fyLabel, activeVchNo, primaryRef, dt, vch_type, rawType, pId, pId, primaryDrParty, primaryCrParty, totalAmount, narration, editId}
        );

        // Delete old transactions for this voucher
        DatabaseManager::instance().executeNonQuery(
            "DELETE FROM transactions WHERE (voucher_no = ? OR voucher_no = ?) AND (voucher_type = ? OR trans_type = ?);",
            {oldVchNo, activeVchNo, vch_type, rawType}
        );
    } else {
        // Insert new voucher
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO vouchers (fy_id, financial_year, voucher_no, instrument_no, voucher_date, voucher_type, legacy_type, party_id, ledger_id, party_name, account_type, amount, narration) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {fyId, fyLabel, activeVchNo, primaryRef, dt, vch_type, rawType, pId, pId, primaryDrParty, primaryCrParty, totalAmount, narration}
        );
    }

    // Insert all Dr and Cr legs into transactions
    for (int i = 0; i < rows.size(); ++i) {
        QVariantMap r = rows.at(i).toMap();
        QString drcr = r.value("drcr").toString();
        QString party = r.value("ledgerName").toString().trimmed();
        if (party.isEmpty()) continue;

        double amt = (drcr == "Dr") ? r.value("debitAmt").toDouble() : r.value("creditAmt").toDouble();
        if (amt <= 0.0) continue;

        QString ref = r.value("refNo").toString().trimmed();
        QString opposing = (drcr == "Dr") ? primaryCrParty : primaryDrParty;

        QVariant partyIdVar = DatabaseManager::instance().executeScalar(
            "SELECT id FROM parties WHERE name = ? COLLATE NOCASE OR alias = ? COLLATE NOCASE LIMIT 1;",
            {party, party}
        );
        int partyId = partyIdVar.isValid() ? partyIdVar.toInt() : 0;

        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, party_id, party_name, opposing_account, dr_cr, amount, invoice_no, narration, row_no) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {fyId, fyLabel, activeVchNo, dt, vch_type, rawType, partyId, party, opposing, drcr, amt, ref, narration, (i + 1)}
        );
    }

    DatabaseManager::instance().commit();
    reload_data();
    return true;
}

bool VouchersModel::delete_voucher(int voucherId) {
    if (voucherId <= 0) return false;
    auto& db = DatabaseManager::instance();

    QVariantList rows = db.executeQuery(
        "SELECT voucher_no, voucher_type, legacy_type FROM vouchers WHERE id = ? LIMIT 1;",
        {voucherId}
    );
    if (rows.isEmpty()) return false;

    QString vNo = rows.first().toMap().value("voucher_no").toString();
    QString vType = rows.first().toMap().value("voucher_type").toString();
    QString lType = rows.first().toMap().value("legacy_type").toString();

    db.beginTransaction();
    db.executeNonQuery("DELETE FROM vouchers WHERE id = ?;", {voucherId});
    db.executeNonQuery(
        "DELETE FROM transactions WHERE voucher_no = ? AND (voucher_type = ? OR trans_type = ?);",
        {vNo, vType, lType}
    );
    db.commit();

    reload_data();
    return true;
}

