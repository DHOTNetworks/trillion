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
    if (prefix == "Pymt" || prefix == "Cash Payment") {
        prefix = "Pymt";
        aliases << "Pymt" << "PYMT" << "Cash Payment";
    } else if (prefix == "Rcpt" || prefix == "Cash Receipt") {
        prefix = "Rcpt";
        aliases << "Rcpt" << "RCPT" << "Cash Receipt";
    } else if (prefix == "ChPt" || prefix == "Cheque Payment" || prefix == "Payment") {
        prefix = "ChPt";
        aliases << "ChPt" << "CHPT" << "Cheque Payment" << "Payment" << "Paym";
    } else if (prefix == "ChRt" || prefix == "Cheque Receipt" || prefix == "Receipt") {
        prefix = "ChRt";
        aliases << "ChRt" << "CHRT" << "Cheque Receipt" << "Receipt" << "Rece";
    } else if (prefix == "Jrnl" || prefix == "Jour" || prefix == "Journal") {
        prefix = "Jrnl";
        aliases << "Jrnl" << "Jour" << "Journal";
    } else if (prefix == "Purc" || prefix == "Purchase") {
        prefix = "Purc";
        aliases << "Purc" << "Purchase";
    } else if (prefix == "Sale" || prefix == "Sales") {
        prefix = "Sale";
        aliases << "Sale" << "Sales";
    }
    aliases << prefix;
    aliases.removeDuplicates();

    QStringList whereClauses;
    QVariantList params;
    for (const QString& a : aliases) {
        whereClauses << "voucher_type = ?" << "legacy_type = ?" << "voucher_no LIKE ?";
        params << a << a << (a + "-%");
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
    FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(dt);
    int fyId = 1;
    QString fyLabel = fy.name;
    QVariant fyIdVar = DatabaseManager::instance().executeScalar(
        "SELECT id FROM financial_years WHERE year_name = ? LIMIT 1;",
        {fy.name}
    );
    if (fyIdVar.isValid() && !fyIdVar.isNull()) {
        fyId = fyIdVar.toInt();
    } else {
        QVariant anyFy = DatabaseManager::instance().executeScalar("SELECT id FROM financial_years WHERE is_active = 1 LIMIT 1;");
        if (anyFy.isValid() && !anyFy.isNull()) fyId = anyFy.toInt();
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
    FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(dt);
    int fyId = 1;
    QString fyLabel = fy.name;
    QVariant fyIdVar = DatabaseManager::instance().executeScalar(
        "SELECT id FROM financial_years WHERE year_name = ? LIMIT 1;",
        {fy.name}
    );
    if (fyIdVar.isValid() && !fyIdVar.isNull()) {
        fyId = fyIdVar.toInt();
    } else {
        QVariant anyFy = DatabaseManager::instance().executeScalar("SELECT id FROM financial_years WHERE is_active = 1 LIMIT 1;");
        if (anyFy.isValid() && !anyFy.isNull()) fyId = anyFy.toInt();
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
    FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(dt);
    int fyId = 1;
    QString fyLabel = fy.name;
    QVariant fyIdVar = DatabaseManager::instance().executeScalar(
        "SELECT id FROM financial_years WHERE year_name = ? LIMIT 1;",
        {fy.name}
    );
    if (fyIdVar.isValid() && !fyIdVar.isNull()) {
        fyId = fyIdVar.toInt();
    } else {
        QVariant anyFy = DatabaseManager::instance().executeScalar("SELECT id FROM financial_years WHERE is_active = 1 LIMIT 1;");
        if (anyFy.isValid() && !anyFy.isNull()) fyId = anyFy.toInt();
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
    int txId = 0;
    QString q;
    QString isoDate = FiscalYearHelper::normalizeToIso(dateHint);
    QString party = partyHint.trimmed();
    QString fy;

    if (vchNoOrId.typeId() == QMetaType::QVariantMap) {
        QVariantMap m = vchNoOrId.toMap();
        txId = m.value("id").toInt();
        q = m.value("voucher_no", m.value("voucherNo")).toString().trimmed();
        if (isoDate.isEmpty()) isoDate = FiscalYearHelper::normalizeToIso(m.value("vIso", m.value("voucher_date", m.value("date"))).toString());
        fy = m.value("financial_year", m.value("financialYear")).toString().trimmed();
        if (party.isEmpty()) party = m.value("party_name", m.value("partyName")).toString().trimmed();
    } else {
        q = vchNoOrId.toString().trimmed();
        bool isNum = false;
        int parsedId = q.toInt(&isNum);
        if (isNum && parsedId > 0 && isoDate.isEmpty()) {
            QVariantList chk = DatabaseManager::instance().executeQuery("SELECT * FROM transactions WHERE id = ? LIMIT 1;", {parsedId});
            if (!chk.isEmpty()) {
                txId = parsedId;
            }
        }
    }

    if (txId <= 0 && q.isEmpty() && isoDate.isEmpty() && party.isEmpty()) return {};

    static const QRegularExpression prefixRe(QStringLiteral("^(Sale|Sales|Purc|Purchase|Pur|Jrnl|Journal|ChPt|ChRt|Pymt|Rcpt|TDS|JFrm|J-Form)[-\\s#]*"), QRegularExpression::CaseInsensitiveOption);
    QString cleanQ = q;
    cleanQ = cleanQ.remove(prefixRe).trimmed();
    if (cleanQ.isEmpty()) cleanQ = q;

    // 1. If exact transaction ID is provided
    if (txId > 0) {
        QVariantList rows = DatabaseManager::instance().executeQuery("SELECT * FROM transactions WHERE id = ? LIMIT 1;", {txId});
        if (!rows.isEmpty()) return rows.first().toMap();
        rows = DatabaseManager::instance().executeQuery("SELECT * FROM vouchers WHERE id = ? LIMIT 1;", {txId});
        if (!rows.isEmpty()) return rows.first().toMap();
    }

    if (fy.isEmpty() && !isoDate.isEmpty()) {
        FiscalYearInfo fInfo = FiscalYearHelper::getFiscalYearForDate(isoDate);
        if (fInfo.isValid()) fy = fInfo.name;
    }
    if (fy.isEmpty()) {
        FiscalYearInfo fInfo = FiscalYearHelper::getActiveFiscalYear();
        if (fInfo.isValid()) fy = fInfo.name;
    }

    QVariantList rows;

    // 2. Exact match on dateHint and voucher_no
    if (!isoDate.isEmpty() && (!q.isEmpty() || !cleanQ.isEmpty())) {
        rows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM transactions WHERE (voucher_no = ? OR voucher_no = ?) AND voucher_date = ? ORDER BY id DESC LIMIT 1;",
            {q, cleanQ, isoDate}
        );
        if (rows.isEmpty()) {
            rows = DatabaseManager::instance().executeQuery(
                "SELECT * FROM vouchers WHERE (voucher_no = ? OR voucher_no = ?) AND voucher_date = ? ORDER BY id DESC LIMIT 1;",
                {q, cleanQ, isoDate}
            );
        }
    }

    // 3. Exact match on Financial Year and voucher_no
    if (rows.isEmpty() && !fy.isEmpty() && (!q.isEmpty() || !cleanQ.isEmpty())) {
        rows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM transactions WHERE (voucher_no = ? OR voucher_no = ?) AND (financial_year = ? OR financial_year = ?) ORDER BY id DESC LIMIT 1;",
            {q, cleanQ, fy, "FY " + fy}
        );
        if (rows.isEmpty()) {
            rows = DatabaseManager::instance().executeQuery(
                "SELECT * FROM vouchers WHERE (voucher_no = ? OR voucher_no = ?) AND (financial_year = ? OR financial_year = ?) ORDER BY id DESC LIMIT 1;",
                {q, cleanQ, fy, "FY " + fy}
            );
        }
    }

    // 4. Fallback lookup by voucher_no
    if (rows.isEmpty() && (!q.isEmpty() || !cleanQ.isEmpty())) {
        rows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM transactions WHERE voucher_no = ? OR voucher_no = ? ORDER BY voucher_date DESC, id DESC LIMIT 1;",
            {q, cleanQ}
        );
        if (rows.isEmpty()) {
            rows = DatabaseManager::instance().executeQuery(
                "SELECT * FROM vouchers WHERE voucher_no = ? OR voucher_no = ? ORDER BY voucher_date DESC, id DESC LIMIT 1;",
                {q, cleanQ}
            );
        }
    }

    if (!rows.isEmpty()) return rows.first().toMap();
    return {};
}

QVariantMap VouchersModel::get_cheque_voucher(const QVariant& vchNoOrId, const QString& dateHint, const QString& partyHint) {
    QVariantMap v;
    int txId = 0;
    QString vNo;
    QString vDate = FiscalYearHelper::normalizeToIso(dateHint);
    QString party = partyHint.trimmed();
    QString fy;

    if (vchNoOrId.typeId() == QMetaType::QVariantMap) {
        QVariantMap m = vchNoOrId.toMap();
        txId = m.value("id").toInt();
        vNo = m.value("voucher_no", m.value("voucherNo")).toString().trimmed();
        if (vDate.isEmpty()) vDate = FiscalYearHelper::normalizeToIso(m.value("vIso", m.value("voucher_date", m.value("date"))).toString());
        fy = m.value("financial_year", m.value("financialYear")).toString().trimmed();
        if (party.isEmpty()) party = m.value("party_name", m.value("partyName")).toString().trimmed();
    } else {
        bool isNum = false;
        int parsedId = vchNoOrId.toInt(&isNum);
        if (isNum && parsedId > 0 && vDate.isEmpty()) {
            QVariantList chk = DatabaseManager::instance().executeQuery("SELECT * FROM transactions WHERE id = ? LIMIT 1;", {parsedId});
            if (!chk.isEmpty()) {
                txId = parsedId;
            } else {
                vNo = QString::number(parsedId);
            }
        } else {
            vNo = vchNoOrId.toString().trimmed();
        }
    }

    static const QRegularExpression prefixRe(QStringLiteral("^(Sale|Sales|Purc|Purchase|Pur|Jrnl|Journal|ChPt|ChRt|Pymt|Rcpt|TDS|JFrm|J-Form)[-\\s#]*"), QRegularExpression::CaseInsensitiveOption);
    QString cleanVNo = vNo;
    cleanVNo = cleanVNo.remove(prefixRe).trimmed();
    if (cleanVNo.isEmpty()) cleanVNo = vNo;

    // 1. If specific transaction ID provided
    if (txId > 0) {
        QVariantList txRow = DatabaseManager::instance().executeQuery("SELECT * FROM transactions WHERE id = ? LIMIT 1;", {txId});
        if (!txRow.isEmpty()) {
            v = txRow.first().toMap();
            vNo = v.value("voucher_no").toString();
            cleanVNo = vNo;
            cleanVNo = cleanVNo.remove(prefixRe).trimmed();
            vDate = v.value("voucher_date").toString();
            fy = v.value("financial_year").toString();
        }
    }

    if (fy.isEmpty() && !vDate.isEmpty()) {
        FiscalYearInfo fInfo = FiscalYearHelper::getFiscalYearForDate(vDate);
        if (fInfo.isValid()) fy = fInfo.name;
    }
    if (fy.isEmpty()) {
        FiscalYearInfo fInfo = FiscalYearHelper::getActiveFiscalYear();
        if (fInfo.isValid()) fy = fInfo.name;
    }

    // 2. Locate header row if not found by ID
    if (v.isEmpty()) {
        QVariantList foundRows;
        if (!vDate.isEmpty() && (!vNo.isEmpty() || !cleanVNo.isEmpty())) {
            foundRows = DatabaseManager::instance().executeQuery(
                "SELECT * FROM transactions WHERE (voucher_no = ? OR voucher_no = ?) AND voucher_date = ? "
                "AND (voucher_type IN ('Payment', 'Receipt', 'ChPt', 'ChRt', 'Bank', 'Pymt', 'Rcpt', 'Cash Payment', 'Cash Receipt') "
                "OR trans_type IN ('Payment', 'Receipt', 'ChPt', 'ChRt', 'Bank', 'Pymt', 'Rcpt', 'Cash Payment', 'Cash Receipt')) ORDER BY id ASC LIMIT 1;",
                {vNo, cleanVNo, vDate}
            );
        }
        if (foundRows.isEmpty() && !vDate.isEmpty() && (!vNo.isEmpty() || !cleanVNo.isEmpty())) {
            foundRows = DatabaseManager::instance().executeQuery(
                "SELECT * FROM vouchers WHERE (voucher_no = ? OR voucher_no = ?) AND voucher_date = ? "
                "AND (voucher_type IN ('Payment', 'Receipt', 'ChPt', 'ChRt', 'Bank', 'Pymt', 'Rcpt', 'Cash Payment', 'Cash Receipt') "
                "OR legacy_type IN ('Payment', 'Receipt', 'ChPt', 'ChRt', 'Bank', 'Pymt', 'Rcpt', 'Cash Payment', 'Cash Receipt')) ORDER BY id ASC LIMIT 1;",
                {vNo, cleanVNo, vDate}
            );
        }
        if (foundRows.isEmpty() && !fy.isEmpty() && (!vNo.isEmpty() || !cleanVNo.isEmpty())) {
            foundRows = DatabaseManager::instance().executeQuery(
                "SELECT * FROM transactions WHERE (voucher_no = ? OR voucher_no = ?) AND (financial_year = ? OR financial_year = ?) "
                "AND (voucher_type IN ('Payment', 'Receipt', 'ChPt', 'ChRt', 'Bank', 'Pymt', 'Rcpt', 'Cash Payment', 'Cash Receipt') "
                "OR trans_type IN ('Payment', 'Receipt', 'ChPt', 'ChRt', 'Bank', 'Pymt', 'Rcpt', 'Cash Payment', 'Cash Receipt')) ORDER BY id ASC LIMIT 1;",
                {vNo, cleanVNo, fy, "FY " + fy}
            );
        }
        if (foundRows.isEmpty() && !fy.isEmpty() && (!vNo.isEmpty() || !cleanVNo.isEmpty())) {
            foundRows = DatabaseManager::instance().executeQuery(
                "SELECT * FROM vouchers WHERE (voucher_no = ? OR voucher_no = ?) AND (financial_year = ? OR financial_year = ?) "
                "AND (voucher_type IN ('Payment', 'Receipt', 'ChPt', 'ChRt', 'Bank', 'Pymt', 'Rcpt', 'Cash Payment', 'Cash Receipt') "
                "OR legacy_type IN ('Payment', 'Receipt', 'ChPt', 'ChRt', 'Bank', 'Pymt', 'Rcpt', 'Cash Payment', 'Cash Receipt')) ORDER BY id ASC LIMIT 1;",
                {vNo, cleanVNo, fy, "FY " + fy}
            );
        }

        if (!foundRows.isEmpty()) {
            v = foundRows.first().toMap();
            vNo = v.value("voucher_no").toString();
            cleanVNo = vNo;
            cleanVNo = cleanVNo.remove(prefixRe).trimmed();
            vDate = v.value("voucher_date").toString();
            if (fy.isEmpty()) fy = v.value("financial_year").toString();
        }
    }

    if (v.isEmpty()) return {};

    // 3. Assemble all sibling line items for this exact voucher (prioritize matching by date)
    QVariantList txRows;
    if (!vDate.isEmpty() && (!vNo.isEmpty() || !cleanVNo.isEmpty())) {
        txRows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM transactions WHERE (voucher_no = ? OR voucher_no = ?) AND voucher_date = ? "
            "AND (voucher_type IN ('Payment', 'Receipt', 'ChPt', 'ChRt', 'Bank', 'Pymt', 'Rcpt', 'Cash Payment', 'Cash Receipt') "
            "OR trans_type IN ('Payment', 'Receipt', 'ChPt', 'ChRt', 'Bank', 'Pymt', 'Rcpt', 'Cash Payment', 'Cash Receipt')) ORDER BY row_no ASC, id ASC;",
            {vNo, cleanVNo, vDate}
        );
    }
    if (txRows.isEmpty() && !fy.isEmpty() && (!vNo.isEmpty() || !cleanVNo.isEmpty())) {
        txRows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM transactions WHERE (voucher_no = ? OR voucher_no = ?) AND (financial_year = ? OR financial_year = ?) "
            "AND (voucher_type IN ('Payment', 'Receipt', 'ChPt', 'ChRt', 'Bank', 'Pymt', 'Rcpt', 'Cash Payment', 'Cash Receipt') "
            "OR trans_type IN ('Payment', 'Receipt', 'ChPt', 'ChRt', 'Bank', 'Pymt', 'Rcpt', 'Cash Payment', 'Cash Receipt')) ORDER BY row_no ASC, id ASC;",
            {vNo, cleanVNo, fy, "FY " + fy}
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

        if (items.size() == 1) {
            QString opp = v.value("opposing_account").toString().trimmed();
            if (opp.isEmpty()) opp = v.value("account_type").toString().trimmed();
            if (!opp.isEmpty()) {
                QVariantMap firstItm = items[0].toMap();
                QVariantMap oppItem;
                bool isDr = (firstItm.value("drcr").toString().toUpper() == "DR");
                oppItem["drcr"] = isDr ? "Cr" : "Dr";
                oppItem["ledgerName"] = opp;
                if (isDr) {
                    oppItem["debitAmt"] = "";
                    oppItem["creditAmt"] = firstItm.value("debitAmt");
                } else {
                    oppItem["debitAmt"] = firstItm.value("creditAmt");
                    oppItem["creditAmt"] = "";
                }
                oppItem["refNo"] = firstItm.value("refNo");
                items.append(oppItem);
            }
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
        crItem["ledgerName"] = v.value("account_type", v.value("opposing_account")).toString();
        crItem["debitAmt"] = "";
        crItem["creditAmt"] = QString::number(v.value("amount").toDouble(), 'f', 2);
        crItem["refNo"] = v.value("instrument_no").toString();
        items.append(crItem);
    }
    v["rows"] = items;
    return v;
}

QVariantMap VouchersModel::get_journal_voucher(const QVariant& vchNoOrId, const QString& dateHint, const QString& partyHint) {
    QVariantMap v;
    int txId = 0;
    QString vNo;
    QString vDate = FiscalYearHelper::normalizeToIso(dateHint);
    QString party = partyHint.trimmed();
    QString fy;

    if (vchNoOrId.typeId() == QMetaType::QVariantMap) {
        QVariantMap m = vchNoOrId.toMap();
        txId = m.value("id").toInt();
        vNo = m.value("voucher_no", m.value("voucherNo")).toString().trimmed();
        if (vDate.isEmpty()) vDate = FiscalYearHelper::normalizeToIso(m.value("vIso", m.value("voucher_date", m.value("date"))).toString());
        fy = m.value("financial_year", m.value("financialYear")).toString().trimmed();
        if (party.isEmpty()) party = m.value("party_name", m.value("partyName")).toString().trimmed();
    } else {
        bool isNum = false;
        int parsedId = vchNoOrId.toInt(&isNum);
        if (isNum && parsedId > 0 && vDate.isEmpty()) {
            QVariantList chk = DatabaseManager::instance().executeQuery("SELECT * FROM transactions WHERE id = ? LIMIT 1;", {parsedId});
            if (!chk.isEmpty()) {
                txId = parsedId;
            } else {
                QVariantList chkV = DatabaseManager::instance().executeQuery("SELECT * FROM vouchers WHERE id = ? LIMIT 1;", {parsedId});
                if (!chkV.isEmpty()) {
                    txId = parsedId;
                } else {
                    vNo = QString::number(parsedId);
                }
            }
        } else {
            vNo = vchNoOrId.toString().trimmed();
        }
    }

    static const QRegularExpression prefixRe(QStringLiteral("^(Sale|Sales|Purc|Purchase|Pur|Jrnl|Journal|ChPt|ChRt|Pymt|Rcpt|TDS|JFrm|J-Form)[-\\s#]*"), QRegularExpression::CaseInsensitiveOption);
    QString cleanVNo = vNo;
    cleanVNo = cleanVNo.remove(prefixRe).trimmed();
    if (cleanVNo.isEmpty()) cleanVNo = vNo;

    // Support structured voucher numbers like 2526/JRNL-1 or FY 2025-26/1
    if (vNo.contains("/")) {
        QStringList parts = vNo.split("/");
        if (parts.size() >= 2) {
            QString p0 = parts[0].trimmed();
            if (p0.length() == 4 && p0.toInt() > 1000) {
                int y1 = p0.left(2).toInt() + 2000;
                int y2 = p0.mid(2, 2).toInt() + 2000;
                fy = QString("FY %1-%2").arg(y1).arg(QString::number(y2).right(2));
            } else if (p0.startsWith("FY", Qt::CaseInsensitive)) {
                fy = p0;
            }
            cleanVNo = parts.last().trimmed();
            cleanVNo = cleanVNo.remove(prefixRe).trimmed();
        }
    }

    // 1. If exact transaction or voucher ID provided
    if (txId > 0) {
        QVariantList txRow = DatabaseManager::instance().executeQuery("SELECT * FROM transactions WHERE id = ? LIMIT 1;", {txId});
        if (!txRow.isEmpty()) {
            v = txRow.first().toMap();
            vNo = v.value("voucher_no").toString();
            cleanVNo = vNo;
            cleanVNo = cleanVNo.remove(prefixRe).trimmed();
            vDate = v.value("voucher_date").toString();
            fy = v.value("financial_year").toString();
        } else {
            QVariantList vRow = DatabaseManager::instance().executeQuery("SELECT * FROM vouchers WHERE id = ? LIMIT 1;", {txId});
            if (!vRow.isEmpty()) {
                v = vRow.first().toMap();
                vNo = v.value("voucher_no").toString();
                cleanVNo = vNo;
                cleanVNo = cleanVNo.remove(prefixRe).trimmed();
                vDate = v.value("voucher_date").toString();
                fy = v.value("financial_year").toString();
            }
        }
    }

    if (fy.isEmpty() && !vDate.isEmpty()) {
        FiscalYearInfo fInfo = FiscalYearHelper::getFiscalYearForDate(vDate);
        if (fInfo.isValid()) fy = fInfo.name;
    }
    if (fy.isEmpty()) {
        FiscalYearInfo fInfo = FiscalYearHelper::getActiveFiscalYear();
        if (fInfo.isValid()) fy = fInfo.name;
    }

    // 2. Locate header row if not found by ID
    if (v.isEmpty()) {
        QVariantList foundRows;
        if (!vDate.isEmpty() && (!vNo.isEmpty() || !cleanVNo.isEmpty())) {
            foundRows = DatabaseManager::instance().executeQuery(
                "SELECT * FROM transactions WHERE (voucher_no = ? OR voucher_no = ?) AND voucher_date = ? "
                "AND (UPPER(voucher_type) IN ('JOURNAL', 'JRNL', 'JV', 'JL', 'JRN') OR UPPER(trans_type) IN ('JOURNAL', 'JRNL', 'JV', 'JL', 'JRN')) ORDER BY id ASC LIMIT 1;",
                {vNo, cleanVNo, vDate}
            );
        }
        if (foundRows.isEmpty() && !vDate.isEmpty() && (!vNo.isEmpty() || !cleanVNo.isEmpty())) {
            foundRows = DatabaseManager::instance().executeQuery(
                "SELECT * FROM vouchers WHERE (voucher_no = ? OR voucher_no = ?) AND voucher_date = ? "
                "AND (UPPER(voucher_type) IN ('JOURNAL', 'JRNL', 'JV', 'JL', 'JRN') OR UPPER(legacy_type) IN ('JOURNAL', 'JRNL', 'JV', 'JL', 'JRN')) ORDER BY id ASC LIMIT 1;",
                {vNo, cleanVNo, vDate}
            );
        }
        if (foundRows.isEmpty() && !fy.isEmpty() && (!vNo.isEmpty() || !cleanVNo.isEmpty())) {
            foundRows = DatabaseManager::instance().executeQuery(
                "SELECT * FROM transactions WHERE (voucher_no = ? OR voucher_no = ?) AND (financial_year = ? OR financial_year = ?) "
                "AND (UPPER(voucher_type) IN ('JOURNAL', 'JRNL', 'JV', 'JL', 'JRN') OR UPPER(trans_type) IN ('JOURNAL', 'JRNL', 'JV', 'JL', 'JRN')) ORDER BY id ASC LIMIT 1;",
                {vNo, cleanVNo, fy, "FY " + fy}
            );
        }
        if (foundRows.isEmpty() && !fy.isEmpty() && (!vNo.isEmpty() || !cleanVNo.isEmpty())) {
            foundRows = DatabaseManager::instance().executeQuery(
                "SELECT * FROM vouchers WHERE (voucher_no = ? OR voucher_no = ?) AND (financial_year = ? OR financial_year = ?) "
                "AND (UPPER(voucher_type) IN ('JOURNAL', 'JRNL', 'JV', 'JL', 'JRN') OR UPPER(legacy_type) IN ('JOURNAL', 'JRNL', 'JV', 'JL', 'JRN')) ORDER BY id ASC LIMIT 1;",
                {vNo, cleanVNo, fy, "FY " + fy}
            );
        }

        if (!foundRows.isEmpty()) {
            v = foundRows.first().toMap();
            vNo = v.value("voucher_no").toString();
            cleanVNo = vNo;
            cleanVNo = cleanVNo.remove(prefixRe).trimmed();
            vDate = v.value("voucher_date").toString();
            if (fy.isEmpty()) fy = v.value("financial_year").toString();
        }
    }

    if (v.isEmpty()) return {};

    // 3. Assemble all sibling line items for this exact Journal Voucher (prioritize matching by date)
    QVariantList txRows;
    if (!vDate.isEmpty() && (!vNo.isEmpty() || !cleanVNo.isEmpty())) {
        txRows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM transactions WHERE (voucher_no = ? OR voucher_no = ?) AND voucher_date = ? "
            "AND (UPPER(voucher_type) IN ('JOURNAL', 'JRNL', 'JV', 'JL', 'JRN') OR UPPER(trans_type) IN ('JOURNAL', 'JRNL', 'JV', 'JL', 'JRN')) ORDER BY row_no ASC, id ASC;",
            {vNo, cleanVNo, vDate}
        );
    }
    if (txRows.isEmpty() && !fy.isEmpty() && (!vNo.isEmpty() || !cleanVNo.isEmpty())) {
        txRows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM transactions WHERE (voucher_no = ? OR voucher_no = ?) AND (financial_year = ? OR financial_year = ?) "
            "AND (UPPER(voucher_type) IN ('JOURNAL', 'JRNL', 'JV', 'JL', 'JRN') OR UPPER(trans_type) IN ('JOURNAL', 'JRNL', 'JV', 'JL', 'JRN')) ORDER BY row_no ASC, id ASC;",
            {vNo, cleanVNo, fy, "FY " + fy}
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
            if (item["drcr"].toString().toUpper() == "DR") {
                item["debitAmt"] = QString::number(amt, 'f', 2);
                item["creditAmt"] = "";
            } else {
                item["debitAmt"] = "";
                item["creditAmt"] = QString::number(amt, 'f', 2);
            }
            item["refNo"] = t.value("invoice_no").toString();
            items.append(item);
        }

        if (items.size() == 1) {
            QString opp = v.value("opposing_account").toString().trimmed();
            if (opp.isEmpty()) opp = v.value("account_type").toString().trimmed();
            if (!opp.isEmpty()) {
                QVariantMap firstItm = items[0].toMap();
                QVariantMap oppItem;
                bool isDr = (firstItm.value("drcr").toString().toUpper() == "DR");
                oppItem["drcr"] = isDr ? "Cr" : "Dr";
                oppItem["ledgerName"] = opp;
                if (isDr) {
                    oppItem["debitAmt"] = "";
                    oppItem["creditAmt"] = firstItm.value("debitAmt");
                } else {
                    oppItem["debitAmt"] = firstItm.value("creditAmt");
                    oppItem["creditAmt"] = "";
                }
                oppItem["refNo"] = firstItm.value("refNo");
                items.append(oppItem);
            }
        }
    } else {
        // Check vouchers table for sibling rows
        QVariantList vRows;
        if (!vDate.isEmpty() && (!vNo.isEmpty() || !cleanVNo.isEmpty())) {
            vRows = DatabaseManager::instance().executeQuery(
                "SELECT * FROM vouchers WHERE (voucher_no = ? OR voucher_no = ?) AND voucher_date = ? "
                "AND (UPPER(voucher_type) IN ('JOURNAL', 'JRNL', 'JV', 'JL', 'JRN') OR UPPER(legacy_type) IN ('JOURNAL', 'JRNL', 'JV', 'JL', 'JRN')) ORDER BY id ASC;",
                {vNo, cleanVNo, vDate}
            );
        }
        if (vRows.isEmpty() && !fy.isEmpty() && (!vNo.isEmpty() || !cleanVNo.isEmpty())) {
            vRows = DatabaseManager::instance().executeQuery(
                "SELECT * FROM vouchers WHERE (voucher_no = ? OR voucher_no = ?) AND (financial_year = ? OR financial_year = ?) "
                "AND (UPPER(voucher_type) IN ('JOURNAL', 'JRNL', 'JV', 'JL', 'JRN') OR UPPER(legacy_type) IN ('JOURNAL', 'JRNL', 'JV', 'JL', 'JRN')) ORDER BY id ASC;",
                {vNo, cleanVNo, fy, "FY " + fy}
            );
        }

        if (vRows.size() > 1) {
            for (const auto& vr : vRows) {
                QVariantMap vm = vr.toMap();
                QVariantMap item;
                item["drcr"] = vm.value("drcr", vm.value("dr_cr")).toString().trimmed().isEmpty() ? "Dr" : vm.value("drcr", vm.value("dr_cr")).toString();
                item["ledgerName"] = !vm.value("party_name").toString().isEmpty() ? vm.value("party_name").toString() : vm.value("ledger_name").toString();
                double amt = vm.value("amount").toDouble();
                if (item["drcr"].toString().toUpper() == "DR") {
                    item["debitAmt"] = QString::number(amt, 'f', 2);
                    item["creditAmt"] = "";
                } else {
                    item["debitAmt"] = "";
                    item["creditAmt"] = QString::number(amt, 'f', 2);
                }
                item["refNo"] = vm.value("instrument_no", vm.value("invoice_no")).toString();
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
            crItem["ledgerName"] = v.value("account_type", v.value("opposing_account")).toString();
            crItem["debitAmt"] = "";
            crItem["creditAmt"] = QString::number(v.value("amount").toDouble(), 'f', 2);
            crItem["refNo"] = v.value("instrument_no").toString();
            items.append(crItem);
        }
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

    FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(dt);
    QString fyLabel = fy.name;
    QVariant fyIdVar = DatabaseManager::instance().executeScalar(
        "SELECT id FROM financial_years WHERE year_name = ? LIMIT 1;",
        {fy.name}
    );
    QVariant fyIdParam;
    if (fyIdVar.isValid() && !fyIdVar.isNull() && fyIdVar.toInt() > 0) {
        fyIdParam = fyIdVar.toInt();
    } else {
        QVariant anyFy = DatabaseManager::instance().executeScalar("SELECT id FROM financial_years WHERE is_active = 1 LIMIT 1;");
        if (anyFy.isValid() && !anyFy.isNull() && anyFy.toInt() > 0) {
            fyIdParam = anyFy.toInt();
        } else {
            fyIdParam = QVariant();
        }
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

    QString rawType = vch_type;
    QString normalizedVType = vch_type;
    if (vch_type == "Cash Payment" || vch_type == "Pymt") {
        rawType = "Pymt";
        normalizedVType = "Payment";
    } else if (vch_type == "Cash Receipt" || vch_type == "Rcpt") {
        rawType = "Rcpt";
        normalizedVType = "Receipt";
    } else if (vch_type == "Cheque Payment" || vch_type == "Payment" || vch_type == "ChPt") {
        rawType = "ChPt";
        normalizedVType = "Payment";
    } else if (vch_type == "Cheque Receipt" || vch_type == "Receipt" || vch_type == "ChRt") {
        rawType = "ChRt";
        normalizedVType = "Receipt";
    } else if (vch_type == "Journal" || vch_type == "Jrnl") {
        rawType = "Jrnl";
        normalizedVType = "Journal";
    }

    QVariant pRow = DatabaseManager::instance().executeScalar(
        "SELECT id FROM parties WHERE name = ? COLLATE NOCASE OR alias = ? COLLATE NOCASE LIMIT 1;",
        {primaryDrParty, primaryDrParty}
    );
    QVariant pIdParam = (pRow.isValid() && !pRow.isNull() && pRow.toInt() > 0) ? pRow.toInt() : QVariant();

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
            {fyIdParam, fyLabel, activeVchNo, primaryRef, dt, normalizedVType, rawType, pIdParam, pIdParam, primaryDrParty, primaryCrParty, totalAmount, narration, editId}
        );

        // Delete old transactions for this voucher
        DatabaseManager::instance().executeNonQuery(
            "DELETE FROM transactions WHERE (voucher_no = ? OR voucher_no = ?) AND (voucher_type = ? OR trans_type = ?);",
            {oldVchNo, activeVchNo, normalizedVType, rawType}
        );
    } else {
        // Insert new voucher
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO vouchers (fy_id, financial_year, voucher_no, instrument_no, voucher_date, voucher_type, legacy_type, party_id, ledger_id, party_name, account_type, amount, narration) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {fyIdParam, fyLabel, activeVchNo, primaryRef, dt, normalizedVType, rawType, pIdParam, pIdParam, primaryDrParty, primaryCrParty, totalAmount, narration}
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
        QVariant partyIdParam = (partyIdVar.isValid() && !partyIdVar.isNull() && partyIdVar.toInt() > 0) ? partyIdVar.toInt() : QVariant();

        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, party_id, party_name, opposing_account, dr_cr, amount, invoice_no, narration, row_no) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {fyIdParam, fyLabel, activeVchNo, dt, normalizedVType, rawType, partyIdParam, party, opposing, drcr, amt, ref, narration, (i + 1)}
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

