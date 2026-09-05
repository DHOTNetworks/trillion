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

        // 1. Opening balance (Initial + prior transactions before activeFromDate)
        double netPriorDr = 0.0;
        double netPriorCr = 0.0;

        QVariantList pRows = DatabaseManager::instance().executeQuery(
            "SELECT opening_balance, balance_type FROM parties WHERE name = ? OR name LIKE ? OR name LIKE ? LIMIT 1;",
            {cleanName, "%" + cleanName + "%", wildcard}
        );
        if (!pRows.isEmpty()) {
            double initialOp = pRows.first().toMap().value("opening_balance").toDouble();
            QString bType = pRows.first().toMap().value("balance_type").toString();
            if (bType == "Dr") netPriorDr += initialOp;
            else netPriorCr += initialOp;
        }

        if (!activeFromDate.isEmpty()) {
            QVariant sPrior = DatabaseManager::instance().executeScalar(
                "SELECT SUM(total_amount) FROM sales_invoices WHERE (customer_name = ? OR customer_name LIKE ? OR customer_name LIKE ?) AND invoice_date < ?;",
                {cleanName, "%" + cleanName + "%", wildcard, activeFromDate}
            );
            if (sPrior.isValid()) netPriorDr += sPrior.toDouble();

            QVariant purPrior = DatabaseManager::instance().executeScalar(
                "SELECT SUM(total_amount) FROM purchase_invoices WHERE (supplier_name = ? OR supplier_name LIKE ? OR supplier_name LIKE ?) AND invoice_date < ?;",
                {cleanName, "%" + cleanName + "%", wildcard, activeFromDate}
            );
            if (purPrior.isValid()) netPriorCr += purPrior.toDouble();

            QVariant padPrior = DatabaseManager::instance().executeScalar(
                "SELECT SUM(total_amount) FROM paddy_procurement WHERE (farmer_name = ? OR farmer_name LIKE ? OR farmer_name LIKE ?) AND arrival_date < ?;",
                {cleanName, "%" + cleanName + "%", wildcard, activeFromDate}
            );
            if (padPrior.isValid()) netPriorCr += padPrior.toDouble();

            QVariantList vPriorRows = DatabaseManager::instance().executeQuery(
                "SELECT voucher_type, legacy_type, party_name, account_type, amount FROM vouchers WHERE (party_name = ? OR party_name LIKE ? OR party_name LIKE ? OR account_type = ? OR account_type LIKE ? OR account_type LIKE ?) AND voucher_type NOT IN ('Sales', 'Purchase') AND voucher_date < ?;",
                {cleanName, "%" + cleanName + "%", wildcard, cleanName, "%" + cleanName + "%", wildcard, activeFromDate}
            );
            for (const QVariant& vr : vPriorRows) {
                QVariantMap v = vr.toMap();
                QString vType = v.value("voucher_type").toString();
                QString legType = v.value("legacy_type").toString();
                QString drP = v.value("party_name").toString().trimmed();
                QString crP = v.value("account_type").toString().trimmed();
                double vAmt = v.value("amount").toDouble();
                bool isPartyMatch = (!drP.isEmpty() && (drP.toLower() == cleanNameLower || drP.toLower().contains(cleanNameLower)));
                bool isAccMatch = (!crP.isEmpty() && (crP.toLower() == cleanNameLower || crP.toLower().contains(cleanNameLower)));

                if (isPartyMatch) {
                    if (vType == "Payment" || legType == "ChPt" || legType == "CP" || legType == "BP") netPriorDr += vAmt;
                    else netPriorCr += vAmt;
                } else if (isAccMatch) {
                    if (vType == "Receipt" || legType == "ChRt" || legType == "CR" || legType == "BR") netPriorDr += vAmt;
                    else netPriorCr += vAmt;
                }
            }
        }

        double finalOpAmt = netPriorDr - netPriorCr;
        if (std::abs(finalOpAmt) > 0.001) {
            auto [opIso, opFmt] = parseDates(!activeFromDate.isEmpty() ? activeFromDate : "2024-04-01");
            QVariantMap opItem;
            opItem["isSelected"] = false;
            opItem["vIso"] = opIso;
            opItem["vDate"] = opFmt;
            opItem["refNo"] = "OP-BAL";
            opItem["particulars"] = QString("Opening Balance (%1)").arg(finalOpAmt >= 0 ? "Dr" : "Cr");
            opItem["amount"] = std::abs(finalOpAmt);
            opItem["financial_year"] = !activeFyName.isEmpty() ? activeFyName : "Opening";
            opItem["fy"] = !activeFyName.isEmpty() ? activeFyName : "Opening";
            if (finalOpAmt >= 0) drItems.append(opItem);
            else crItems.append(opItem);
        }

        // 2. Sales Invoices (Respect active FY date range)
        QString sSql = "SELECT invoice_no, invoice_date, item_name, bag_count, weight_qtl, rate_per_qtl, vehicle_no, broker_name, total_amount, narration, financial_year FROM sales_invoices WHERE (customer_name = ? OR customer_name LIKE ? OR customer_name LIKE ?)";
        QVariantList sParams = {cleanName, "%" + cleanName + "%", wildcard};
        if (!activeFromDate.isEmpty() && !activeToDate.isEmpty()) {
            sSql += " AND invoice_date >= ? AND invoice_date <= ?";
            sParams << activeFromDate << activeToDate;
        }
        sSql += " ORDER BY invoice_date ASC, id ASC;";
        QVariantList sRows = DatabaseManager::instance().executeQuery(sSql, sParams);
        for (const QVariant& r : sRows) {
            QVariantMap s = r.toMap();
            auto [isoD, fmtD] = parseDates(s.value("invoice_date").toString());
            QString fyStr = computeFyForDate(isoD, s.value("financial_year").toString());

            QString parts = QString("Sales Invoice: %1").arg(s.value("item_name").toString());
            int bags = s.value("bag_count").toInt();
            double wt = s.value("weight_qtl").toDouble();
            double rate = s.value("rate_per_qtl").toDouble();
            QString veh = s.value("vehicle_no").toString().trimmed();
            QString broker = s.value("broker_name").toString().trimmed();
            QString narr = s.value("narration").toString().trimmed();

            QStringList details;
            if (bags > 0) details << QString("%1 Bags").arg(bags);
            if (wt > 0.0) details << QString("%1 Qtl").arg(wt, 0, 'f', 2);
            if (rate > 0.0) details << QString("@ ₹%1/Qtl").arg(rate, 0, 'f', 2);
            if (!details.isEmpty()) parts += " (" + details.join(" | ") + ")";

            if (!veh.isEmpty()) parts += " | Veh: " + veh;
            if (!broker.isEmpty()) parts += " | Broker: " + broker;
            if (!narr.isEmpty()) parts += " | " + narr;

            QVariantMap item;
            item["isSelected"] = false;
            item["vIso"] = isoD;
            item["vDate"] = fmtD;
            item["refNo"] = s.value("invoice_no").toString();
            item["particulars"] = parts;
            item["amount"] = s.value("total_amount").toDouble();
            item["financial_year"] = fyStr;
            item["fy"] = fyStr;
            drItems.append(item);
        }

        // 3. Purchase Invoices (Respect active FY date range)
        QString purSql = "SELECT invoice_no, invoice_date, item_name, bag_count, weight_qtl, rate_per_qtl, vehicle_no, broker_name, taxable_amount, total_amount, narration, financial_year FROM purchase_invoices WHERE (supplier_name = ? OR supplier_name LIKE ? OR supplier_name LIKE ?)";
        QVariantList purParams = {cleanName, "%" + cleanName + "%", wildcard};
        if (!activeFromDate.isEmpty() && !activeToDate.isEmpty()) {
            purSql += " AND invoice_date >= ? AND invoice_date <= ?";
            purParams << activeFromDate << activeToDate;
        }
        purSql += " ORDER BY invoice_date ASC, id ASC;";
        QVariantList purRows = DatabaseManager::instance().executeQuery(purSql, purParams);
        for (const QVariant& r : purRows) {
            QVariantMap p = r.toMap();
            auto [isoD, fmtD] = parseDates(p.value("invoice_date").toString());
            QString fyStr = computeFyForDate(isoD, p.value("financial_year").toString());
            double taxVal = p.value("taxable_amount").toDouble();
            double totVal = p.value("total_amount").toDouble();
            double grossAmt = taxVal > 0 ? taxVal : totVal;
            double tdsAmt = (grossAmt > totVal) ? std::round((grossAmt - totVal) * 100.0) / 100.0 : 0.0;

            QString parts = QString("B.No. %1 | %2").arg(p.value("invoice_no").toString(), p.value("item_name").toString());
            int bags = p.value("bag_count").toInt();
            double wt = p.value("weight_qtl").toDouble();
            double rate = p.value("rate_per_qtl").toDouble();
            QString veh = p.value("vehicle_no").toString().trimmed();
            QString broker = p.value("broker_name").toString().trimmed();
            QString narr = p.value("narration").toString().trimmed();

            QStringList details;
            if (bags > 0) details << QString("%1 Bags").arg(bags);
            if (wt > 0.0) details << QString("%1 Qtl").arg(wt, 0, 'f', 2);
            if (rate > 0.0) details << QString("@ ₹%1/Qtl").arg(rate, 0, 'f', 2);
            if (!details.isEmpty()) parts += " (" + details.join(" | ") + ")";

            if (!veh.isEmpty()) parts += " | Veh: " + veh;
            if (!broker.isEmpty()) parts += " | Broker: " + broker;
            if (!narr.isEmpty()) parts += " | " + narr;

            QVariantMap item;
            item["isSelected"] = false;
            item["vIso"] = isoD;
            item["vDate"] = fmtD;
            item["refNo"] = p.value("invoice_no").toString();
            item["particulars"] = parts;
            item["amount"] = grossAmt;
            item["financial_year"] = fyStr;
            item["fy"] = fyStr;
            crItems.append(item);

            if (tdsAmt > 0) {
                QVariantMap tdsItem;
                tdsItem["isSelected"] = false;
                tdsItem["vIso"] = isoD;
                tdsItem["vDate"] = fmtD;
                tdsItem["refNo"] = p.value("invoice_no").toString();
                tdsItem["particulars"] = QString("T.D.S. U/S 194Q (B.No. %1)").arg(p.value("invoice_no").toString());
                tdsItem["amount"] = tdsAmt;
                tdsItem["financial_year"] = fyStr;
                tdsItem["fy"] = fyStr;
                drItems.append(tdsItem);
            }
        }

        // 4. Paddy Procurement / Arrivals (Respect active FY date range)
        QString padSql = "SELECT slip_no, arrival_date, paddy_variety, bag_count, net_weight_qtl, rate_per_qtl, net_amount FROM paddy_arrivals WHERE (farmer_name = ? OR farmer_name LIKE ? OR farmer_name LIKE ?)";
        QVariantList padParams = {cleanName, "%" + cleanName + "%", wildcard};
        if (!activeFromDate.isEmpty() && !activeToDate.isEmpty()) {
            padSql += " AND arrival_date >= ? AND arrival_date <= ?";
            padParams << activeFromDate << activeToDate;
        }
        padSql += " ORDER BY arrival_date ASC, id ASC;";
        QVariantList paRows = DatabaseManager::instance().executeQuery(padSql, padParams);
        for (const QVariant& r : paRows) {
            QVariantMap pa = r.toMap();
            auto [isoD, fmtD] = parseDates(pa.value("arrival_date").toString());
            QString fyStr = computeFyForDate(isoD);

            QString parts = QString("Paddy Arrival: %1").arg(pa.value("paddy_variety").toString());
            int bags = pa.value("bag_count").toInt();
            double wt = pa.value("net_weight_qtl").toDouble();
            double rate = pa.value("rate_per_qtl").toDouble();
            QStringList details;
            if (bags > 0) details << QString("%1 Bags").arg(bags);
            if (wt > 0.0) details << QString("%1 Qtl").arg(wt, 0, 'f', 2);
            if (rate > 0.0) details << QString("@ ₹%1/Qtl").arg(rate, 0, 'f', 2);
            if (!details.isEmpty()) parts += " (" + details.join(" | ") + ")";

            QVariantMap item;
            item["isSelected"] = false;
            item["vIso"] = isoD;
            item["vDate"] = fmtD;
            item["refNo"] = pa.value("slip_no").toString();
            item["particulars"] = parts;
            item["amount"] = pa.value("net_amount").toDouble();
            item["financial_year"] = fyStr;
            item["fy"] = fyStr;
            crItems.append(item);
        }

        // 5. Vouchers (Respect active FY date range)
        QString vSql = "SELECT voucher_no, voucher_date, voucher_type, legacy_type, party_name, account_type, amount, narration, vehicle_no, broker_name, financial_year FROM vouchers WHERE (party_name = ? OR party_name LIKE ? OR party_name LIKE ? OR account_type = ? OR account_type LIKE ? OR account_type LIKE ?) AND voucher_type NOT IN ('Sales', 'Purchase')";
        QVariantList vParams = {cleanName, "%" + cleanName + "%", wildcard, cleanName, "%" + cleanName + "%", wildcard};
        if (!activeFromDate.isEmpty() && !activeToDate.isEmpty()) {
            vSql += " AND voucher_date >= ? AND voucher_date <= ?";
            vParams << activeFromDate << activeToDate;
        }
        vSql += " ORDER BY voucher_date ASC, id ASC;";
        QVariantList vRows = DatabaseManager::instance().executeQuery(vSql, vParams);
        for (const QVariant& r : vRows) {
            QVariantMap v = r.toMap();
            QString vType = v.value("voucher_type").toString();
            QString legType = v.value("legacy_type").toString();
            QString drP = v.value("party_name").toString().trimmed();
            QString crP = v.value("account_type").toString().trimmed();
            double vAmt = v.value("amount").toDouble();
            QString narr = v.value("narration").toString().trimmed();
            QString veh = v.value("vehicle_no").toString().trimmed();
            QString broker = v.value("broker_name").toString().trimmed();
            auto [isoD, fmtD] = parseDates(v.value("voucher_date").toString());
            QString fyStr = computeFyForDate(isoD, v.value("financial_year").toString());

            bool isPartyMatch = (!drP.isEmpty() && (drP.toLower() == cleanNameLower || drP.toLower().contains(cleanNameLower)));
            bool isAccMatch = (!crP.isEmpty() && (crP.toLower() == cleanNameLower || crP.toLower().contains(cleanNameLower)));

            if (isPartyMatch) {
                if (vType == "Payment" || legType == "ChPt" || legType == "CP" || legType == "BP") {
                    QString desc = QString("%1 (Paid via %2)").arg(vType, !crP.isEmpty() ? crP : "Bank/Cash");
                    if (!veh.isEmpty()) desc += " | Veh: " + veh;
                    if (!broker.isEmpty()) desc += " | Broker: " + broker;
                    if (!narr.isEmpty()) desc += " | " + narr;
                    QVariantMap item;
                    item["isSelected"] = false; item["vIso"] = isoD; item["vDate"] = fmtD;
                    item["refNo"] = v.value("voucher_no").toString(); item["particulars"] = desc; item["amount"] = vAmt;
                    item["financial_year"] = fyStr; item["fy"] = fyStr;
                    drItems.append(item);
                } else {
                    QString desc = QString("%1 (Received in %2)").arg(vType, !crP.isEmpty() ? crP : "Bank/Cash");
                    if (!veh.isEmpty()) desc += " | Veh: " + veh;
                    if (!broker.isEmpty()) desc += " | Broker: " + broker;
                    if (!narr.isEmpty()) desc += " | " + narr;
                    QVariantMap item;
                    item["isSelected"] = false; item["vIso"] = isoD; item["vDate"] = fmtD;
                    item["refNo"] = v.value("voucher_no").toString(); item["particulars"] = desc; item["amount"] = vAmt;
                    item["financial_year"] = fyStr; item["fy"] = fyStr;
                    crItems.append(item);
                }
            } else if (isAccMatch) {
                if (vType == "Receipt" || legType == "ChRt" || legType == "CR" || legType == "BR") {
                    QString desc = QString("Receipt from %1").arg(drP);
                    if (!veh.isEmpty()) desc += " | Veh: " + veh;
                    if (!broker.isEmpty()) desc += " | Broker: " + broker;
                    if (!narr.isEmpty()) desc += " | " + narr;
                    QVariantMap item;
                    item["isSelected"] = false; item["vIso"] = isoD; item["vDate"] = fmtD;
                    item["refNo"] = v.value("voucher_no").toString(); item["particulars"] = desc; item["amount"] = vAmt;
                    item["financial_year"] = fyStr; item["fy"] = fyStr;
                    drItems.append(item);
                } else {
                    QString desc = QString("Payment to %1").arg(drP);
                    if (!veh.isEmpty()) desc += " | Veh: " + veh;
                    if (!broker.isEmpty()) desc += " | Broker: " + broker;
                    if (!narr.isEmpty()) desc += " | " + narr;
                    QVariantMap item;
                    item["isSelected"] = false; item["vIso"] = isoD; item["vDate"] = fmtD;
                    item["refNo"] = v.value("voucher_no").toString(); item["particulars"] = desc; item["amount"] = vAmt;
                    item["financial_year"] = fyStr; item["fy"] = fyStr;
                    crItems.append(item);
                }
            }
        }
    } else {
        // Load all transactions across all parties
        QVariantList sRows = DatabaseManager::instance().executeQuery(
            "SELECT invoice_no, invoice_date, customer_name, item_name, weight_qtl, total_amount, narration FROM sales_invoices ORDER BY invoice_date ASC, id ASC;"
        );
        for (const QVariant& r : sRows) {
            QVariantMap s = r.toMap();
            auto [isoD, fmtD] = parseDates(s.value("invoice_date").toString());
            QString parts = QString("[%1] Sales Invoice: %2 (%3 Qtl)").arg(s.value("customer_name").toString(), s.value("item_name").toString(), QString::number(s.value("weight_qtl").toDouble()));
            if (!s.value("narration").toString().isEmpty()) parts += " | " + s.value("narration").toString();
            QVariantMap item;
            item["isSelected"] = false; item["vIso"] = isoD; item["vDate"] = fmtD;
            item["refNo"] = s.value("invoice_no").toString(); item["particulars"] = parts; item["amount"] = s.value("total_amount").toDouble();
            drItems.append(item);
        }

        QVariantList purRows = DatabaseManager::instance().executeQuery(
            "SELECT invoice_no, invoice_date, supplier_name, item_name, weight_qtl, taxable_amount, total_amount, narration FROM purchase_invoices ORDER BY invoice_date ASC, id ASC;"
        );
        for (const QVariant& r : purRows) {
            QVariantMap p = r.toMap();
            auto [isoD, fmtD] = parseDates(p.value("invoice_date").toString());
            double taxVal = p.value("taxable_amount").toDouble();
            double totVal = p.value("total_amount").toDouble();
            double grossAmt = taxVal > 0 ? taxVal : totVal;
            double tdsAmt = (grossAmt > totVal) ? std::round((grossAmt - totVal) * 100.0) / 100.0 : 0.0;

            QString parts = QString("[%1] Purchase Bill: %2 (%3 Qtl)").arg(p.value("supplier_name").toString(), p.value("item_name").toString(), QString::number(p.value("weight_qtl").toDouble()));
            if (!p.value("narration").toString().isEmpty()) parts += " | " + p.value("narration").toString();
            QVariantMap item;
            item["isSelected"] = false; item["vIso"] = isoD; item["vDate"] = fmtD;
            item["refNo"] = p.value("invoice_no").toString(); item["particulars"] = parts; item["amount"] = grossAmt;
            crItems.append(item);

            if (tdsAmt > 0) {
                QVariantMap tdsItem;
                tdsItem["isSelected"] = false; tdsItem["vIso"] = isoD; tdsItem["vDate"] = fmtD;
                tdsItem["refNo"] = p.value("invoice_no").toString();
                tdsItem["particulars"] = QString("[%1] T.D.S. U/S 194Q (%2)").arg(p.value("supplier_name").toString(), p.value("invoice_no").toString());
                tdsItem["amount"] = tdsAmt;
                drItems.append(tdsItem);
            }
        }

        QVariantList vRows = DatabaseManager::instance().executeQuery(
            "SELECT voucher_no, voucher_date, voucher_type, legacy_type, party_name, account_type, amount, narration FROM vouchers WHERE voucher_type NOT IN ('Sales', 'Purchase') ORDER BY voucher_date ASC, id ASC;"
        );
        for (const QVariant& r : vRows) {
            QVariantMap v = r.toMap();
            QString vType = v.value("voucher_type").toString();
            QString legType = v.value("legacy_type").toString();
            QString drP = v.value("party_name").toString().trimmed();
            QString crP = v.value("account_type").toString().trimmed();
            double vAmt = v.value("amount").toDouble();
            QString narr = v.value("narration").toString();
            auto [isoD, fmtD] = parseDates(v.value("voucher_date").toString());

            if (vType == "Payment" || legType == "ChPt" || legType == "CP" || legType == "BP") {
                QVariantMap dItem;
                dItem["isSelected"] = false; dItem["vIso"] = isoD; dItem["vDate"] = fmtD;
                dItem["refNo"] = v.value("voucher_no").toString();
                dItem["particulars"] = QString("[%1] %2 (Paid via %3) - %4").arg(drP, vType, crP, narr);
                dItem["amount"] = vAmt;
                drItems.append(dItem);

                QVariantMap cItem;
                cItem["isSelected"] = false; cItem["vIso"] = isoD; cItem["vDate"] = fmtD;
                cItem["refNo"] = v.value("voucher_no").toString();
                cItem["particulars"] = QString("[%1] %2 (Dr: %3) - %4").arg(crP, vType, drP, narr);
                cItem["amount"] = vAmt;
                crItems.append(cItem);
            } else if (vType == "Receipt" || legType == "ChRt" || legType == "CR" || legType == "BR") {
                QVariantMap dItem;
                dItem["isSelected"] = false; dItem["vIso"] = isoD; dItem["vDate"] = fmtD;
                dItem["refNo"] = v.value("voucher_no").toString();
                dItem["particulars"] = QString("[%1] Receipt from %2 - %3").arg(crP, drP, narr);
                dItem["amount"] = vAmt;
                drItems.append(dItem);

                QVariantMap cItem;
                cItem["isSelected"] = false; cItem["vIso"] = isoD; cItem["vDate"] = fmtD;
                cItem["refNo"] = v.value("voucher_no").toString();
                cItem["particulars"] = QString("[%1] %2 (Received in %3) - %4").arg(drP, vType, crP, narr);
                cItem["amount"] = vAmt;
                crItems.append(cItem);
            } else {
                QVariantMap dItem;
                dItem["isSelected"] = false; dItem["vIso"] = isoD; dItem["vDate"] = fmtD;
                dItem["refNo"] = v.value("voucher_no").toString();
                dItem["particulars"] = QString("[%1] %2 (Cr: %3) - %4").arg(drP, vType, crP, narr);
                dItem["amount"] = vAmt;
                drItems.append(dItem);

                QVariantMap cItem;
                cItem["isSelected"] = false; cItem["vIso"] = isoD; cItem["vDate"] = fmtD;
                cItem["refNo"] = v.value("voucher_no").toString();
                cItem["particulars"] = QString("[%1] %2 (Dr: %3) - %4").arg(crP, vType, drP, narr);
                cItem["amount"] = vAmt;
                crItems.append(cItem);
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
