#include "parties_model.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include <QDate>
#include <algorithm>

PartiesModel::PartiesModel(QObject* parent)
    : BaseTableModel(
        {"Party Name", "Group", "Type", "Specialty", "Phone", "City", "GSTIN", "Balance (₹)", "Dr/Cr"},
        {"name", "group_name", "party_type", "special_type", "phone", "city", "gstin", "opening_balance", "balance_type"},
        parent
    )
{
    reload_data();
}

void PartiesModel::reload_data() {
    beginResetModel();
    m_data = DatabaseManager::instance().executeQuery("SELECT * FROM parties ORDER BY name COLLATE NOCASE ASC;");
    endResetModel();
    emit dataChangedSignal();
    emit countChanged();
}

QStringList PartiesModel::get_parties_list() const {
    QStringList list;
    for (const QVariant& v : m_data) {
        QString n = v.toMap().value("name").toString();
        if (!n.isEmpty()) list.append(n);
    }
    std::sort(list.begin(), list.end(), [](const QString& a, const QString& b) {
        return a.compare(b, Qt::CaseInsensitive) < 0;
    });
    return list;
}

QStringList PartiesModel::get_party_list() const {
    return get_parties_list();
}

QStringList PartiesModel::get_bank_accounts_list() const {
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT name FROM parties WHERE group_name LIKE '%Bank%' OR party_type = 'Bank' OR name LIKE '%Bank%' ORDER BY name COLLATE NOCASE ASC;"
    );
    QStringList list;
    for (const QVariant& r : rows) {
        QString n = r.toMap().value("name").toString();
        if (!n.isEmpty()) list.append(n);
    }
    if (list.isEmpty()) {
        list << "HDFC Bank MG Road" << "IndusInd Bank(200999406993)" << "SBI Raichur Main Branch";
    }
    return list;
}

QString PartiesModel::get_ledger_live_balance(const QString& ledgerName) {
    if (ledgerName.trimmed().isEmpty()) return "0.00 Dr";
    QString cleanName = ledgerName.trimmed().toLower();

    // 1. Opening balance
    QVariantList pRows = DatabaseManager::instance().executeQuery(
        "SELECT opening_balance, balance_type FROM parties WHERE LOWER(name) = ? OR LOWER(name) LIKE ? LIMIT 1;",
        {cleanName, "%" + cleanName + "%"}
    );

    double netDr = 0.0;
    double netCr = 0.0;

    if (!pRows.isEmpty()) {
        QVariantMap p = pRows.first().toMap();
        double op = p.value("opening_balance").toDouble();
        QString bType = p.value("balance_type").toString();
        if (bType == "Dr") netDr += op;
        else netCr += op;
    }

    // 2. Sales Invoices (Dr)
    QVariant sVal = DatabaseManager::instance().executeScalar(
        "SELECT SUM(total_amount) FROM sales_invoices WHERE LOWER(customer_name) = ? OR LOWER(customer_name) LIKE ?;",
        {cleanName, "%" + cleanName + "%"}
    );
    if (sVal.isValid()) netDr += sVal.toDouble();

    // 3. Paddy Procurement (Dr)
    QVariant paVal = DatabaseManager::instance().executeScalar(
        "SELECT SUM(total_amount) FROM paddy_procurement WHERE LOWER(farmer_name) = ? OR LOWER(farmer_name) LIKE ?;",
        {cleanName, "%" + cleanName + "%"}
    );
    if (paVal.isValid()) netDr += paVal.toDouble();

    // 4. Purchase Invoices (Cr)
    QVariant purVal = DatabaseManager::instance().executeScalar(
        "SELECT SUM(total_amount) FROM purchase_invoices WHERE LOWER(supplier_name) = ? OR LOWER(supplier_name) LIKE ?;",
        {cleanName, "%" + cleanName + "%"}
    );
    if (purVal.isValid()) netCr += purVal.toDouble();

    // 5. Vouchers
    QVariantList vRows = DatabaseManager::instance().executeQuery(
        "SELECT voucher_type, party_name, account_type, amount FROM vouchers WHERE voucher_type NOT IN ('Sales', 'Purchase');"
    );
    for (const QVariant& vr : vRows) {
        QVariantMap row = vr.toMap();
        QString drP = row.value("party_name").toString().trimmed().toLower();
        QString crP = row.value("account_type").toString().trimmed().toLower();
        double vAmt = row.value("amount").toDouble();

        if (!drP.isEmpty() && (drP == cleanName || drP.contains(cleanName))) netDr += vAmt;
        if (!crP.isEmpty() && (crP == cleanName || crP.contains(cleanName))) netCr += vAmt;
    }

    double diff = netDr - netCr;
    if (diff >= 0) {
        return AccountingEngine::formatIndianCurrency(diff, false) + " Dr";
    } else {
        return AccountingEngine::formatIndianCurrency(std::abs(diff), false) + " Cr";
    }
}

QVariantMap PartiesModel::get_party_by_name(const QString& name) const {
    QString cleanName = name.trimmed();
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM parties WHERE name = ? OR name LIKE ? LIMIT 1;",
        {cleanName, "%" + cleanName + "%"}
    );
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    return QVariantMap();
}

QVariantMap PartiesModel::get_party_details(const QString& name) const {
    return get_party_by_name(name);
}

bool PartiesModel::add_party(const QString& name, const QString& ptype, const QString& phone, const QString& place, const QString& gstin, double op_bal, const QString& bal_type) {
    bool ok = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO parties (name, party_type, phone, city, gstin, opening_balance, balance_type) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);",
        {name, ptype, phone, place, gstin, op_bal, bal_type}
    );
    if (ok) reload_data();
    return ok;
}

bool PartiesModel::update_ledger_full(
    int party_id, const QString& name, const QString& alias, const QString& prefix,
    const QString& group_name, const QString& party_type, const QString& special_type,
    double opening_balance, const QString& balance_type, const QString& mailing_name,
    const QString& address, const QString& city, const QString& district,
    const QString& state, const QString& pincode, const QString& phone,
    const QString& mobile, const QString& whatsapp, const QString& email,
    const QString& contact_person, const QString& gstin, const QString& pan,
    const QString& aadhaar, double credit_limit, int credit_days,
    const QString& bank_name, const QString& bank_account, const QString& ifsc_code
) {
    bool ok = DatabaseManager::instance().executeNonQuery(
        "UPDATE parties SET name = ?, alias = ?, prefix = ?, group_name = ?, party_type = ?, special_type = ?, "
        "opening_balance = ?, balance_type = ?, mailing_name = ?, address = ?, city = ?, district = ?, state = ?, "
        "pincode = ?, phone = ?, mobile = ?, whatsapp = ?, email = ?, contact_person = ?, gstin = ?, pan = ?, "
        "aadhaar = ?, credit_limit = ?, credit_days = ?, bank_name = ?, bank_account = ?, ifsc_code = ? "
        "WHERE id = ?;",
        {
            name, alias, prefix, group_name, party_type, special_type,
            opening_balance, balance_type, mailing_name, address, city, district, state,
            pincode, phone, mobile, whatsapp, email, contact_person, gstin, pan,
            aadhaar, credit_limit, credit_days, bank_name, bank_account, ifsc_code, party_id
        }
    );
    if (ok) reload_data();
    return ok;
}

QVariantMap PartiesModel::get_party_statement(const QString& partyName) {
    QVariantList drItems, crItems;
    QString cleanName = partyName.trimmed();

    if (!cleanName.isEmpty()) {
        QString cleanNameLower = cleanName.toLower();

        // 1. Opening balance
        QVariantList pRows = DatabaseManager::instance().executeQuery(
            "SELECT opening_balance, balance_type FROM parties WHERE name = ? OR name LIKE ? LIMIT 1;",
            {cleanName, "%" + cleanName + "%"}
        );
        if (!pRows.isEmpty()) {
            double opVal = pRows.first().toMap().value("opening_balance").toDouble();
            QString bType = pRows.first().toMap().value("balance_type").toString();
            if (opVal > 0) {
                QVariantMap opItem;
                opItem["isSelected"] = false;
                opItem["vIso"] = "2024-04-01";
                opItem["vDate"] = "01-04-2024";
                opItem["refNo"] = "OP-BAL";
                opItem["particulars"] = QString("Opening Balance (%1)").arg(bType);
                opItem["amount"] = opVal;
                if (bType == "Dr") drItems.append(opItem);
                else crItems.append(opItem);
            }
        }

        // 2. Sales Invoices
        QVariantList sRows = DatabaseManager::instance().executeQuery(
            "SELECT invoice_no, invoice_date, item_name, weight_qtl, total_amount, narration FROM sales_invoices WHERE customer_name = ? OR customer_name LIKE ? ORDER BY invoice_date ASC, id ASC;",
            {cleanName, "%" + cleanName + "%"}
        );
        for (const QVariant& r : sRows) {
            QVariantMap s = r.toMap();
            QString parts = QString("Sales Invoice: %1 (%2 Qtl)").arg(s.value("item_name").toString(), QString::number(s.value("weight_qtl").toDouble()));
            if (!s.value("narration").toString().isEmpty()) parts += " | " + s.value("narration").toString();
            QVariantMap item;
            item["isSelected"] = false;
            item["vIso"] = s.value("invoice_date").toString();
            item["vDate"] = s.value("invoice_date").toString();
            item["refNo"] = s.value("invoice_no").toString();
            item["particulars"] = parts;
            item["amount"] = s.value("total_amount").toDouble();
            drItems.append(item);
        }

        // 3. Purchase Invoices
        QVariantList purRows = DatabaseManager::instance().executeQuery(
            "SELECT invoice_no, invoice_date, item_name, weight_qtl, taxable_amount, total_amount, narration FROM purchase_invoices WHERE supplier_name = ? OR supplier_name LIKE ? ORDER BY invoice_date ASC, id ASC;",
            {cleanName, "%" + cleanName + "%"}
        );
        for (const QVariant& r : purRows) {
            QVariantMap p = r.toMap();
            double taxVal = p.value("taxable_amount").toDouble();
            double totVal = p.value("total_amount").toDouble();
            double grossAmt = taxVal > 0 ? taxVal : totVal;
            double tdsAmt = (grossAmt > totVal) ? std::round((grossAmt - totVal) * 100.0) / 100.0 : 0.0;

            QString parts = QString("B.No. %1 | %2 (%3 Qtl)").arg(p.value("invoice_no").toString(), p.value("item_name").toString(), QString::number(p.value("weight_qtl").toDouble()));
            if (!p.value("narration").toString().isEmpty()) parts += " | " + p.value("narration").toString();

            QVariantMap item;
            item["isSelected"] = false;
            item["vIso"] = p.value("invoice_date").toString();
            item["vDate"] = p.value("invoice_date").toString();
            item["refNo"] = p.value("invoice_no").toString();
            item["particulars"] = parts;
            item["amount"] = grossAmt;
            crItems.append(item);

            if (tdsAmt > 0) {
                QVariantMap tdsItem;
                tdsItem["isSelected"] = false;
                tdsItem["vIso"] = p.value("invoice_date").toString();
                tdsItem["vDate"] = p.value("invoice_date").toString();
                tdsItem["refNo"] = p.value("invoice_no").toString();
                tdsItem["particulars"] = QString("T.D.S. U/S 194Q (B.No. %1)").arg(p.value("invoice_no").toString());
                tdsItem["amount"] = tdsAmt;
                drItems.append(tdsItem);
            }
        }

        // 4. Vouchers
        QVariantList vRows = DatabaseManager::instance().executeQuery(
            "SELECT voucher_no, voucher_date, voucher_type, legacy_type, party_name, account_type, amount, narration FROM vouchers WHERE (party_name = ? OR party_name LIKE ? OR account_type = ? OR account_type LIKE ?) AND voucher_type NOT IN ('Sales', 'Purchase') ORDER BY voucher_date ASC, id ASC;",
            {cleanName, "%" + cleanName + "%", cleanName, "%" + cleanName + "%"}
        );
        for (const QVariant& r : vRows) {
            QVariantMap v = r.toMap();
            QString vType = v.value("voucher_type").toString();
            QString legType = v.value("legacy_type").toString();
            QString drP = v.value("party_name").toString().trimmed();
            QString crP = v.value("account_type").toString().trimmed();
            double vAmt = v.value("amount").toDouble();
            QString narr = v.value("narration").toString();

            bool isPartyMatch = (!drP.isEmpty() && (drP.toLower() == cleanNameLower || drP.toLower().contains(cleanNameLower)));
            bool isAccMatch = (!crP.isEmpty() && (crP.toLower() == cleanNameLower || crP.toLower().contains(cleanNameLower)));

            if (isPartyMatch) {
                if (vType == "Payment" || legType == "ChPt" || legType == "CP") {
                    QString desc = QString("%1 (Paid via %2)").arg(vType, crP);
                    if (!narr.isEmpty()) desc += " | " + narr;
                    QVariantMap item;
                    item["isSelected"] = false; item["vIso"] = v.value("voucher_date").toString(); item["vDate"] = v.value("voucher_date").toString();
                    item["refNo"] = v.value("voucher_no").toString(); item["particulars"] = desc; item["amount"] = vAmt;
                    drItems.append(item);
                } else {
                    QString desc = QString("%1 (Received in %2)").arg(vType, crP);
                    if (!narr.isEmpty()) desc += " | " + narr;
                    QVariantMap item;
                    item["isSelected"] = false; item["vIso"] = v.value("voucher_date").toString(); item["vDate"] = v.value("voucher_date").toString();
                    item["refNo"] = v.value("voucher_no").toString(); item["particulars"] = desc; item["amount"] = vAmt;
                    crItems.append(item);
                }
            } else if (isAccMatch) {
                if (vType == "Receipt" || legType == "ChRt") {
                    QString desc = QString("Receipt from %1").arg(drP);
                    if (!narr.isEmpty()) desc += " | " + narr;
                    QVariantMap item;
                    item["isSelected"] = false; item["vIso"] = v.value("voucher_date").toString(); item["vDate"] = v.value("voucher_date").toString();
                    item["refNo"] = v.value("voucher_no").toString(); item["particulars"] = desc; item["amount"] = vAmt;
                    drItems.append(item);
                } else {
                    QString desc = QString("Payment to %1").arg(drP);
                    if (!narr.isEmpty()) desc += " | " + narr;
                    QVariantMap item;
                    item["isSelected"] = false; item["vIso"] = v.value("voucher_date").toString(); item["vDate"] = v.value("voucher_date").toString();
                    item["refNo"] = v.value("voucher_no").toString(); item["particulars"] = desc; item["amount"] = vAmt;
                    crItems.append(item);
                }
            }
        }
    }

    QVariantMap res;
    res["dr_items"] = drItems;
    res["cr_items"] = crItems;
    return res;
}
