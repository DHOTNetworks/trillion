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

QStringList PartiesModel::get_account_groups() const {
    QStringList defaults = {
        "Bank Accounts", "Cash-in-hand", "Direct Expenses (Hamali/Freight)",
        "Duties & Taxes (GST)", "Loans & Liabilities", "Paddy Procurement Purchases",
        "Rice Milling Sales Revenue", "Sundry Creditors (Farmers/Vendors)", "Sundry Debtors (Buyers)"
    };
    QVariantList rows = DatabaseManager::instance().executeQuery("SELECT name FROM account_groups ORDER BY name COLLATE NOCASE ASC;");
    QStringList result;
    for (const QVariant& r : rows) {
        QString n = r.toMap().value("name").toString();
        if (!n.isEmpty() && !result.contains(n)) result.append(n);
    }
    for (const QString& d : defaults) {
        if (!result.contains(d)) result.append(d);
    }
    std::sort(result.begin(), result.end(), [](const QString& a, const QString& b) {
        return a.compare(b, Qt::CaseInsensitive) < 0;
    });
    return result;
}

QStringList PartiesModel::get_cities() const {
    QStringList defaults = {"Ballari", "Bengaluru", "Hospet", "Hyderabad", "Kalaburagi", "Koppal", "Raichur", "Vijayanagara"};
    QVariantList rows = DatabaseManager::instance().executeQuery("SELECT DISTINCT city FROM parties WHERE city IS NOT NULL AND city != '';");
    QStringList result;
    for (const QVariant& r : rows) {
        QString n = r.toMap().value("city").toString();
        if (!n.isEmpty() && !result.contains(n)) result.append(n);
    }
    for (const QString& d : defaults) {
        if (!result.contains(d)) result.append(d);
    }
    std::sort(result.begin(), result.end(), [](const QString& a, const QString& b) {
        return a.compare(b, Qt::CaseInsensitive) < 0;
    });
    return result;
}

QStringList PartiesModel::get_districts() const {
    QStringList defaults = {"Ballari", "Bengaluru Urban", "Hyderabad", "Koppal", "Raichur", "Vijayanagara"};
    QVariantList rows = DatabaseManager::instance().executeQuery("SELECT DISTINCT district FROM parties WHERE district IS NOT NULL AND district != '';");
    QStringList result;
    for (const QVariant& r : rows) {
        QString n = r.toMap().value("district").toString();
        if (!n.isEmpty() && !result.contains(n)) result.append(n);
    }
    for (const QString& d : defaults) {
        if (!result.contains(d)) result.append(d);
    }
    std::sort(result.begin(), result.end(), [](const QString& a, const QString& b) {
        return a.compare(b, Qt::CaseInsensitive) < 0;
    });
    return result;
}

QStringList PartiesModel::get_stations() const {
    QStringList defaults = {"Andhra Pradesh", "Bengaluru Ganj", "Hyderabad Market", "Karnataka", "Koppal Mandi", "Raichur APMC Yard", "Telangana"};
    QVariantList rows = DatabaseManager::instance().executeQuery("SELECT DISTINCT state FROM parties WHERE state IS NOT NULL AND state != '';");
    QStringList result;
    for (const QVariant& r : rows) {
        QString n = r.toMap().value("state").toString();
        if (!n.isEmpty() && !result.contains(n)) result.append(n);
    }
    for (const QString& d : defaults) {
        if (!result.contains(d)) result.append(d);
    }
    std::sort(result.begin(), result.end(), [](const QString& a, const QString& b) {
        return a.compare(b, Qt::CaseInsensitive) < 0;
    });
    return result;
}

QString PartiesModel::get_party_live_balance_by_id(int partyId) const {
    if (partyId <= 0) return "0.00 Dr";

    // 1. Opening balance
    QVariantList pRows = DatabaseManager::instance().executeQuery(
        "SELECT opening_balance, balance_type, name FROM parties WHERE id = ? LIMIT 1;",
        {partyId}
    );
    if (pRows.isEmpty()) return "0.00 Dr";

    QVariantMap p = pRows.first().toMap();
    double netDr = 0.0;
    double netCr = 0.0;
    double op = p.value("opening_balance").toDouble();
    QString bType = p.value("balance_type").toString();
    QString pName = p.value("name").toString().trimmed();
    if (bType == "Dr") netDr += op;
    else netCr += op;

    // 2. Sales Invoices (Dr) - match by customer_id or party name fallback
    QVariant sVal = DatabaseManager::instance().executeScalar(
        "SELECT SUM(total_amount) FROM sales_invoices WHERE customer_id = ? OR ((customer_id IS NULL OR customer_id = 0) AND LOWER(customer_name) = LOWER(?));",
        {partyId, pName}
    );
    if (sVal.isValid() && !sVal.isNull()) netDr += sVal.toDouble();

    // 3. Paddy Procurement (Dr)
    QVariant paVal = DatabaseManager::instance().executeScalar(
        "SELECT SUM(total_amount) FROM paddy_procurement WHERE farmer_id = ? OR ((farmer_id IS NULL OR farmer_id = 0) AND LOWER(farmer_name) = LOWER(?));",
        {partyId, pName}
    );
    if (paVal.isValid() && !paVal.isNull()) netDr += paVal.toDouble();

    // 4. Purchase Invoices (Cr)
    QVariant purVal = DatabaseManager::instance().executeScalar(
        "SELECT SUM(total_amount) FROM purchase_invoices WHERE supplier_id = ? OR ((supplier_id IS NULL OR supplier_id = 0) AND LOWER(supplier_name) = LOWER(?));",
        {partyId, pName}
    );
    if (purVal.isValid() && !purVal.isNull()) netCr += purVal.toDouble();

    // 5. Vouchers (Dr/Cr)
    QVariant vDr = DatabaseManager::instance().executeScalar(
        "SELECT SUM(amount) FROM vouchers WHERE voucher_type NOT IN ('Sales', 'Purchase') AND (party_id = ? OR ledger_id = ?);",
        {partyId, partyId}
    );
    if (vDr.isValid() && !vDr.isNull()) netDr += vDr.toDouble();

    double diff = netDr - netCr;
    if (diff >= 0) {
        return AccountingEngine::formatIndianCurrency(diff, false) + " Dr";
    } else {
        return AccountingEngine::formatIndianCurrency(std::abs(diff), false) + " Cr";
    }
}

QString PartiesModel::get_ledger_live_balance(const QString& ledgerName) {
    if (ledgerName.trimmed().isEmpty()) return "0.00 Dr";
    QString cleanName = ledgerName.trimmed();

    QVariant pIdVal = DatabaseManager::instance().executeScalar(
        "SELECT id FROM parties WHERE LOWER(name) = LOWER(?) LIMIT 1;",
        {cleanName}
    );
    if (pIdVal.isValid() && pIdVal.toInt() > 0) {
        return get_party_live_balance_by_id(pIdVal.toInt());
    }

    // 1. Opening balance fallback
    QVariantList pRows = DatabaseManager::instance().executeQuery(
        "SELECT opening_balance, balance_type FROM parties WHERE LOWER(name) = LOWER(?) OR LOWER(name) LIKE ? LIMIT 1;",
        {cleanName, "%" + cleanName.toLower() + "%"}
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
        "SELECT SUM(total_amount) FROM sales_invoices WHERE LOWER(customer_name) = LOWER(?) OR LOWER(customer_name) LIKE ?;",
        {cleanName, "%" + cleanName.toLower() + "%"}
    );
    if (sVal.isValid()) netDr += sVal.toDouble();

    // 3. Paddy Procurement (Dr)
    QVariant paVal = DatabaseManager::instance().executeScalar(
        "SELECT SUM(total_amount) FROM paddy_procurement WHERE LOWER(farmer_name) = LOWER(?) OR LOWER(farmer_name) LIKE ?;",
        {cleanName, "%" + cleanName.toLower() + "%"}
    );
    if (paVal.isValid()) netDr += paVal.toDouble();

    // 4. Purchase Invoices (Cr)
    QVariant purVal = DatabaseManager::instance().executeScalar(
        "SELECT SUM(total_amount) FROM purchase_invoices WHERE LOWER(supplier_name) = LOWER(?) OR LOWER(supplier_name) LIKE ?;",
        {cleanName, "%" + cleanName.toLower() + "%"}
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

        if (!drP.isEmpty() && (drP == cleanName.toLower() || drP.contains(cleanName.toLower()))) netDr += vAmt;
        if (!crP.isEmpty() && (crP == cleanName.toLower() || crP.contains(cleanName.toLower()))) netCr += vAmt;
    }

    double diff = netDr - netCr;
    if (diff >= 0) {
        return AccountingEngine::formatIndianCurrency(diff, false) + " Dr";
    } else {
        return AccountingEngine::formatIndianCurrency(std::abs(diff), false) + " Cr";
    }
}

QVariantMap PartiesModel::get_party_by_id(int partyId) const {
    if (partyId <= 0) return {};
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM parties WHERE id = ? LIMIT 1;",
        {partyId}
    );
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    return {};
}

QVariantMap PartiesModel::get_party_by_name(const QString& name) const {
    QString cleanName = name.trimmed();
    if (cleanName.isEmpty()) return {};

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM parties WHERE name = ? COLLATE NOCASE LIMIT 1;",
        {cleanName}
    );
    if (rows.isEmpty()) {
        rows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM parties WHERE alias = ? COLLATE NOCASE OR name LIKE ? LIMIT 1;",
            {cleanName, "%" + cleanName + "%"}
        );
    }
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    return QVariantMap();
}

QVariantList PartiesModel::search_parties(const QString& query) const {
    QString q = query.trimmed();
    if (q.isEmpty()) return {};
    QString pattern = q;
    pattern.replace(QChar(0x00A0), '%');
    pattern.replace(' ', '%');
    QString wildcard = "%" + pattern + "%";

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT id, legacy_id, name, group_name, party_type, phone, city, gstin, opening_balance, balance_type FROM parties WHERE name LIKE ? OR alias LIKE ? OR phone LIKE ? OR city LIKE ? ORDER BY name COLLATE NOCASE ASC LIMIT 30;",
        {wildcard, wildcard, wildcard, wildcard}
    );
    return rows;
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

static std::pair<QString, QString> parseDates(const QString& dStr) {
    if (dStr.trimmed().isEmpty()) return {"9999-12-31", ""};
    QString s = dStr.trimmed();
    if (s.contains('-')) {
        QStringList p = s.split('-');
        if (p.size() == 3) {
            if (p[0].length() == 4) {
                // "2026-03-24" -> iso="2026-03-24", fmt="24-03-2026"
                int y = p[0].toInt();
                int m = p[1].toInt();
                int d = p[2].toInt();
                return {
                    QString("%1-%2-%3").arg(y, 4, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(d, 2, 10, QChar('0')),
                    QString("%1-%2-%3").arg(d, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(y, 4, 10, QChar('0'))
                };
            } else if (p[2].length() == 4) {
                // "24-03-2026" -> iso="2026-03-24", fmt="24-03-2026"
                int d = p[0].toInt();
                int m = p[1].toInt();
                int y = p[2].toInt();
                return {
                    QString("%1-%2-%3").arg(y, 4, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(d, 2, 10, QChar('0')),
                    QString("%1-%2-%3").arg(d, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(y, 4, 10, QChar('0'))
                };
            }
        }
    } else if (s.contains('/')) {
        QStringList p = s.split('/');
        if (p.size() == 3 && p[2].length() == 4) {
            int d = p[0].toInt();
            int m = p[1].toInt();
            int y = p[2].toInt();
            return {
                QString("%1-%2-%3").arg(y, 4, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(d, 2, 10, QChar('0')),
                QString("%1-%2-%3").arg(d, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(y, 4, 10, QChar('0'))
            };
        }
    }
    return {s, s};
}

static QString computeFyForDate(const QString& isoDate, const QString& explicitFy = "") {
    if (!explicitFy.trimmed().isEmpty()) return explicitFy.trimmed();
    if (isoDate >= "2026-04-01" && isoDate <= "2027-03-31") return "FY 2026-27";
    if (isoDate >= "2025-04-01" && isoDate <= "2026-03-31") return "FY 2025-26";
    if (isoDate >= "2024-04-01" && isoDate <= "2025-03-31") return "FY 2024-25";
    if (isoDate >= "2023-04-01" && isoDate <= "2024-03-31") return "FY 2023-24";
    return "FY 2025-26";
}

QVariantMap PartiesModel::get_party_statement(const QString& partyName) {
    QVariantList drItems, crItems;
    QString cleanName = partyName.trimmed();

    QString activeFromDate = AccountingEngine::getActiveFromDate();
    QString activeToDate = AccountingEngine::getActiveToDate();
    QString activeFyName = AccountingEngine::getActiveFyLabel();

    if (activeFromDate.isEmpty() && activeToDate.isEmpty()) {
        QVariantList fyActiveRows = DatabaseManager::instance().executeQuery("SELECT year_name, start_date, end_date FROM financial_years WHERE is_active = 1 LIMIT 1;");
        if (!fyActiveRows.isEmpty()) {
            QVariantMap act = fyActiveRows.first().toMap();
            activeFyName = act.value("year_name").toString();
            activeFromDate = act.value("start_date").toString();
            activeToDate = act.value("end_date").toString();
        }
    }

    if (!cleanName.isEmpty()) {
        QString cleanNameLower = cleanName.toLower();
        QString namePattern = cleanName;
        namePattern.replace(QChar(0x00A0), '%');
        namePattern.replace(' ', '%');
        QString wildcard = "%" + namePattern + "%";

        // Find party info from master
        QVariantList pRows = DatabaseManager::instance().executeQuery(
            "SELECT id, legacy_id, opening_balance, balance_type FROM parties WHERE name = ? OR name LIKE ? OR name LIKE ? LIMIT 1;",
            {cleanName, "%" + cleanName + "%", wildcard}
        );
        int partyId = 0;
        int legacyCode = 0;
        double initialOp = 0.0;
        QString initialOpType = "Cr";
        if (!pRows.isEmpty()) {
            partyId = pRows.first().toMap().value("id").toInt();
            legacyCode = pRows.first().toMap().value("legacy_id").toInt();
            initialOp = pRows.first().toMap().value("opening_balance").toDouble();
            initialOpType = pRows.first().toMap().value("balance_type").toString();
        }

        // 1. Calculate Opening Balance from transactions prior to active period
        double priorDr = 0.0;
        double priorCr = 0.0;

        if (initialOpType == "Dr") priorDr += initialOp;
        else priorCr += initialOp;

        if (!activeFromDate.isEmpty()) {
            QVariantList priorRows = DatabaseManager::instance().executeQuery(
                "SELECT dr_cr, SUM(amount) as total_amt FROM transactions "
                "WHERE (party_name = ? OR party_name LIKE ? OR party_name LIKE ? OR party_id = ? OR (account_code > 0 AND account_code = ?)) "
                "AND voucher_date < ? GROUP BY dr_cr;",
                {cleanName, "%" + cleanName + "%", wildcard, partyId, legacyCode, activeFromDate}
            );
            for (const auto& pr : priorRows) {
                QVariantMap m = pr.toMap();
                if (m.value("dr_cr").toString().compare("Dr", Qt::CaseInsensitive) == 0) {
                    priorDr += m.value("total_amt").toDouble();
                } else {
                    priorCr += m.value("total_amt").toDouble();
                }
            }
        }

        double netOp = priorDr - priorCr;
        if (std::abs(netOp) > 0.001) {
            auto [opIso, opFmt] = parseDates(!activeFromDate.isEmpty() ? activeFromDate : "2024-04-01");
            QVariantMap opItem;
            opItem["isSelected"] = false;
            opItem["vIso"] = opIso;
            opItem["vDate"] = opFmt;
            opItem["refNo"] = "OP-BAL";
            opItem["voucher_no"] = "OP";
            opItem["invoice_no"] = "";
            opItem["voucher_type"] = "OBal";
            opItem["legacy_type"] = "OBal";
            opItem["trans_type"] = "OBal";
            opItem["particulars"] = QString("Opening Balance (%1)").arg(netOp >= 0 ? "Dr" : "Cr");
            opItem["amount"] = std::abs(netOp);
            opItem["financial_year"] = !activeFyName.isEmpty() ? activeFyName : "Opening";
            opItem["fy"] = !activeFyName.isEmpty() ? activeFyName : "Opening";
            if (netOp >= 0) drItems.append(opItem);
            else crItems.append(opItem);
        }

        // 2. Fetch all period transactions
        QString sql = "SELECT voucher_no, voucher_date, voucher_type, trans_type, opposing_account, dr_cr, amount, invoice_no, narration, financial_year, broker_name, vehicle_no, gr_no, taxable_amount, tds_amount FROM transactions WHERE (party_name = ? OR party_name LIKE ? OR party_name LIKE ? OR party_id = ? OR (account_code > 0 AND account_code = ?))";
        QVariantList params = {cleanName, "%" + cleanName + "%", wildcard, partyId, legacyCode};
        if (!activeFromDate.isEmpty() && !activeToDate.isEmpty()) {
            sql += " AND voucher_date >= ? AND voucher_date <= ?";
            params << activeFromDate << activeToDate;
        }
        sql += " ORDER BY voucher_date ASC, id ASC;";

        QVariantList rows = DatabaseManager::instance().executeQuery(sql, params);
        for (const auto& r : rows) {
            QVariantMap t = r.toMap();
            QString vNo = t.value("voucher_no").toString();
            QString rawType = t.value("trans_type").toString();
            QString vType = t.value("voucher_type").toString();
            QString opposing = t.value("opposing_account").toString();
            QString drCr = t.value("dr_cr").toString();
            double amt = t.value("amount").toDouble();
            QString invNo = t.value("invoice_no").toString();
            QString narr = t.value("narration").toString().trimmed();
            QString veh = t.value("vehicle_no").toString().trimmed();
            QString broker = t.value("broker_name").toString().trimmed();
            auto [isoD, fmtD] = parseDates(t.value("voucher_date").toString());
            QString fyStr = computeFyForDate(isoD, t.value("financial_year").toString());

            QString displayRef = !rawType.isEmpty() ? QString("%1 %2").arg(rawType, vNo).trimmed() : QString("%1 %2").arg(vType, vNo).trimmed();
            if (displayRef.isEmpty()) displayRef = vNo;

            QString desc;
            if (rawType == "Sale" || vType == "Sales") {
                desc = QString("Sales Invoice: %1").arg(!invNo.isEmpty() ? invNo : vNo);
            } else if (rawType == "Purc" || vType == "Purchase") {
                desc = QString("Purchase Bill: %1").arg(!invNo.isEmpty() ? invNo : vNo);
            } else if (rawType == "ChRt" || rawType == "Rcpt" || vType == "Receipt") {
                desc = QString("Receipt via %1").arg(!opposing.isEmpty() ? opposing : "Bank/Cash");
            } else if (rawType == "ChPt" || rawType == "Pymt" || vType == "Payment") {
                desc = QString("Payment to %1").arg(!opposing.isEmpty() ? opposing : "Bank/Cash");
            } else if (rawType == "Jrnl" || vType == "Journal") {
                desc = QString("Journal: %1").arg(!opposing.isEmpty() ? opposing : "A/c");
            } else if (rawType == "JFrm" || vType == "J-Form") {
                desc = QString("J-Form: %1").arg(!opposing.isEmpty() ? opposing : "Paddy Purchase");
            } else {
                desc = QString("%1: %2").arg(vType, opposing);
            }

            if (!veh.isEmpty()) desc += " | Veh: " + veh;
            if (!broker.isEmpty()) desc += " | Broker: " + broker;
            if (!narr.isEmpty()) desc += " | " + narr;

            double tdsAmt = t.value("tds_amount").toDouble();
            if ((rawType == "Purc" || vType == "Purchase") && tdsAmt > 0.0) {
                // Gross up purchase bill on Cr side
                QVariantMap item;
                item["isSelected"] = false;
                item["vIso"] = isoD;
                item["vDate"] = fmtD;
                item["refNo"] = displayRef;
                item["voucher_no"] = vNo;
                item["invoice_no"] = invNo;
                item["voucher_type"] = vType;
                item["legacy_type"] = rawType;
                item["trans_type"] = rawType;
                item["particulars"] = QString("B.No. %1").arg(!invNo.isEmpty() ? invNo : vNo);
                if (!veh.isEmpty()) item["particulars"] = item["particulars"].toString() + " | Veh: " + veh;
                if (!broker.isEmpty()) item["particulars"] = item["particulars"].toString() + " | Broker: " + broker;
                if (!narr.isEmpty()) item["particulars"] = item["particulars"].toString() + " | " + narr;
                item["amount"] = amt + tdsAmt;
                item["financial_year"] = fyStr;
                item["fy"] = fyStr;
                crItems.append(item);

                // Add separate TDS deduction on Dr side matching Bahi Khata
                QVariantMap tdsItem;
                tdsItem["isSelected"] = false;
                tdsItem["vIso"] = isoD;
                tdsItem["vDate"] = fmtD;
                tdsItem["refNo"] = displayRef;
                tdsItem["voucher_no"] = vNo;
                tdsItem["invoice_no"] = invNo;
                tdsItem["voucher_type"] = "TDS";
                tdsItem["legacy_type"] = "TDS";
                tdsItem["trans_type"] = "TDS";
                tdsItem["particulars"] = QString("T.D.S. U/S 194Q (B.No. %1)").arg(!invNo.isEmpty() ? invNo : vNo);
                tdsItem["amount"] = tdsAmt;
                tdsItem["financial_year"] = fyStr;
                tdsItem["fy"] = fyStr;
                drItems.append(tdsItem);
            } else {
                QVariantMap item;
                item["isSelected"] = false;
                item["vIso"] = isoD;
                item["vDate"] = fmtD;
                item["refNo"] = displayRef;
                item["voucher_no"] = vNo;
                item["invoice_no"] = invNo;
                item["voucher_type"] = vType;
                item["legacy_type"] = rawType;
                item["trans_type"] = rawType;
                item["particulars"] = desc;
                item["amount"] = amt;
                item["financial_year"] = fyStr;
                item["fy"] = fyStr;

                if (drCr.compare("Dr", Qt::CaseInsensitive) == 0) {
                    drItems.append(item);
                } else {
                    crItems.append(item);
                }
            }
        }
    } else {
        // Load all transactions across all parties
        QString sql = "SELECT voucher_no, voucher_date, voucher_type, trans_type, party_name, opposing_account, dr_cr, amount, invoice_no, narration, financial_year, broker_name, vehicle_no, taxable_amount, tds_amount FROM transactions";
        QVariantList params;
        if (!activeFromDate.isEmpty() && !activeToDate.isEmpty()) {
            sql += " WHERE voucher_date >= ? AND voucher_date <= ?";
            params << activeFromDate << activeToDate;
        }
        sql += " ORDER BY voucher_date ASC, id ASC;";

        QVariantList rows = DatabaseManager::instance().executeQuery(sql, params);
        for (const auto& r : rows) {
            QVariantMap t = r.toMap();
            QString vNo = t.value("voucher_no").toString();
            QString rawType = t.value("trans_type").toString();
            QString vType = t.value("voucher_type").toString();
            QString pName = t.value("party_name").toString();
            QString opposing = t.value("opposing_account").toString();
            QString drCr = t.value("dr_cr").toString();
            double amt = t.value("amount").toDouble();
            QString invNo = t.value("invoice_no").toString();
            QString narr = t.value("narration").toString().trimmed();
            QString veh = t.value("vehicle_no").toString().trimmed();
            QString broker = t.value("broker_name").toString().trimmed();
            double tdsAmt = t.value("tds_amount").toDouble();
            auto [isoD, fmtD] = parseDates(t.value("voucher_date").toString());
            QString fyStr = computeFyForDate(isoD, t.value("financial_year").toString());

            QString displayRef = !rawType.isEmpty() ? QString("%1 %2").arg(rawType, vNo).trimmed() : QString("%1 %2").arg(vType, vNo).trimmed();

            if ((rawType == "Purc" || vType == "Purchase") && tdsAmt > 0.0) {
                QString desc = QString("[%1] B.No. %2").arg(pName, !invNo.isEmpty() ? invNo : vNo);
                if (!veh.isEmpty()) desc += " | Veh: " + veh;
                if (!broker.isEmpty()) desc += " | Broker: " + broker;
                if (!narr.isEmpty()) desc += " | " + narr;

                QVariantMap item;
                item["isSelected"] = false; item["vIso"] = isoD; item["vDate"] = fmtD;
                item["refNo"] = displayRef; item["voucher_no"] = vNo; item["invoice_no"] = invNo;
                item["voucher_type"] = vType; item["legacy_type"] = rawType; item["trans_type"] = rawType;
                item["particulars"] = desc; item["amount"] = amt + tdsAmt;
                item["financial_year"] = fyStr; item["fy"] = fyStr;
                crItems.append(item);

                QVariantMap tdsItem;
                tdsItem["isSelected"] = false; tdsItem["vIso"] = isoD; tdsItem["vDate"] = fmtD;
                tdsItem["refNo"] = displayRef; tdsItem["voucher_no"] = vNo; tdsItem["invoice_no"] = invNo;
                tdsItem["voucher_type"] = "TDS"; tdsItem["legacy_type"] = "TDS"; tdsItem["trans_type"] = "TDS";
                tdsItem["particulars"] = QString("[%1] T.D.S. U/S 194Q (B.No. %2)").arg(pName, !invNo.isEmpty() ? invNo : vNo);
                tdsItem["amount"] = tdsAmt; tdsItem["financial_year"] = fyStr; tdsItem["fy"] = fyStr;
                drItems.append(tdsItem);
            } else {
                QString desc = QString("[%1] %2").arg(pName, !opposing.isEmpty() ? opposing : vType);
                if (!veh.isEmpty()) desc += " | Veh: " + veh;
                if (!broker.isEmpty()) desc += " | Broker: " + broker;
                if (!narr.isEmpty()) desc += " | " + narr;

                QVariantMap item;
                item["isSelected"] = false; item["vIso"] = isoD; item["vDate"] = fmtD;
                item["refNo"] = displayRef; item["voucher_no"] = vNo; item["invoice_no"] = invNo;
                item["voucher_type"] = vType; item["legacy_type"] = rawType; item["trans_type"] = rawType;
                item["particulars"] = desc; item["amount"] = amt;
                item["financial_year"] = fyStr; item["fy"] = fyStr;

                if (drCr.compare("Dr", Qt::CaseInsensitive) == 0) {
                    drItems.append(item);
                } else {
                    crItems.append(item);
                }
            }
        }
    }

    std::sort(drItems.begin(), drItems.end(), [](const QVariant& a, const QVariant& b) {
        return a.toMap().value("vIso").toString() < b.toMap().value("vIso").toString();
    });
    std::sort(crItems.begin(), crItems.end(), [](const QVariant& a, const QVariant& b) {
        return a.toMap().value("vIso").toString() < b.toMap().value("vIso").toString();
    });

    QVariantMap res;
    res["dr_items"] = drItems;
    res["cr_items"] = crItems;
    return res;
}
