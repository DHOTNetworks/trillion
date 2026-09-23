#include "parties_model.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"
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
        QString n = r.toMap().value("name").toString().trimmed();
        if (!n.isEmpty()) list.append(n);
    }
    return list;
}

QStringList PartiesModel::get_account_groups() const {
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT name FROM account_groups "
        "UNION "
        "SELECT DISTINCT TRIM(group_name) AS name FROM parties WHERE group_name IS NOT NULL AND TRIM(group_name) != '' "
        "ORDER BY name COLLATE NOCASE ASC;"
    );
    QStringList result;
    for (const QVariant& r : rows) {
        QString n = r.toMap().value("name").toString().trimmed();
        if (!n.isEmpty() && !result.contains(n, Qt::CaseInsensitive)) {
            result.append(n);
        }
    }
    return result;
}

QStringList PartiesModel::get_stations() const {
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT DISTINCT TRIM(city) AS val FROM parties WHERE city IS NOT NULL AND TRIM(city) != '' "
        "UNION "
        "SELECT DISTINCT TRIM(destination) AS val FROM transport_dispatches WHERE destination IS NOT NULL AND TRIM(destination) != '' "
        "ORDER BY val COLLATE NOCASE ASC;"
    );
    QStringList result;
    for (const QVariant& r : rows) {
        QString n = r.toMap().value("val").toString().trimmed();
        if (!n.isEmpty() && !result.contains(n, Qt::CaseInsensitive)) {
            result.append(n);
        }
    }
    return result;
}

QStringList PartiesModel::get_cities() const {
    return get_stations();
}

QStringList PartiesModel::get_districts() const {
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT DISTINCT TRIM(district) AS val FROM parties WHERE district IS NOT NULL AND TRIM(district) != '' "
        "ORDER BY val COLLATE NOCASE ASC;"
    );
    QStringList result;
    for (const QVariant& r : rows) {
        QString n = r.toMap().value("val").toString().trimmed();
        if (!n.isEmpty() && !result.contains(n, Qt::CaseInsensitive)) {
            result.append(n);
        }
    }
    return result;
}

QStringList PartiesModel::get_states() const {
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT DISTINCT TRIM(state) AS val FROM parties WHERE state IS NOT NULL AND TRIM(state) != '' "
        "ORDER BY val COLLATE NOCASE ASC;"
    );
    QStringList result;
    for (const QVariant& r : rows) {
        QString n = r.toMap().value("val").toString().trimmed();
        if (!n.isEmpty() && !result.contains(n, Qt::CaseInsensitive) && n != "STATE" && n != "INTER-STATE") {
            result.append(n);
        }
    }
    QStringList standardStates = {
        "Andhra Pradesh", "Arunachal Pradesh", "Assam", "Bihar", "Chandigarh", "Chhattisgarh",
        "Dadra & Nagar Haveli and Daman & Diu", "Delhi", "Goa", "Gujarat", "Haryana",
        "Himachal Pradesh", "Jammu & Kashmir", "Jharkhand", "Karnataka", "Kerala", "Ladakh",
        "Lakshadweep", "Madhya Pradesh", "Maharashtra", "Manipur", "Meghalaya", "Mizoram",
        "Nagaland", "Odisha", "Puducherry", "Punjab", "Rajasthan", "Sikkim", "Tamil Nadu",
        "Telangana", "Tripura", "Uttar Pradesh", "Uttarakhand", "West Bengal"
    };
    for (const QString& s : standardStates) {
        if (!result.contains(s, Qt::CaseInsensitive)) {
            result.append(s);
        }
    }
    std::sort(result.begin(), result.end(), [](const QString& a, const QString& b) {
        return a.compare(b, Qt::CaseInsensitive) < 0;
    });
    return result;
}

QString PartiesModel::get_state_code_for_state(const QString& state) const {
    QString s = state.trimmed().toLower();
    if (s.contains("jammu") || s.contains("kashmir")) return "01";
    if (s.contains("himachal")) return "02";
    if (s.contains("punjab")) return "03";
    if (s.contains("chandigarh")) return "04";
    if (s.contains("uttarakhand") || s.contains("uttaranchal")) return "05";
    if (s.contains("haryana")) return "06";
    if (s.contains("delhi")) return "07";
    if (s.contains("rajasthan")) return "08";
    if (s.contains("uttar pradesh") || s == "up") return "09";
    if (s.contains("bihar")) return "10";
    if (s.contains("sikkim")) return "11";
    if (s.contains("arunachal")) return "12";
    if (s.contains("nagaland")) return "13";
    if (s.contains("manipur")) return "14";
    if (s.contains("mizoram")) return "15";
    if (s.contains("tripura")) return "16";
    if (s.contains("meghalaya")) return "17";
    if (s.contains("assam")) return "18";
    if (s.contains("bengal")) return "19";
    if (s.contains("jharkhand")) return "20";
    if (s.contains("odisha") || s.contains("orissa")) return "21";
    if (s.contains("chhattisgarh")) return "22";
    if (s.contains("madhya pradesh") || s == "mp") return "23";
    if (s.contains("gujarat")) return "24";
    if (s.contains("maharashtra")) return "27";
    if (s.contains("karnataka")) return "29";
    if (s.contains("goa")) return "30";
    if (s.contains("kerala")) return "32";
    if (s.contains("tamil nadu") || s.contains("tamilnadu")) return "33";
    if (s.contains("puducherry") || s.contains("pondicherry")) return "34";
    if (s.contains("telangana")) return "36";
    if (s.contains("andhra")) return "37";
    if (s.contains("ladakh")) return "38";
    return "";
}

QString PartiesModel::get_state_for_gstin(const QString& gstin) const {
    QString g = gstin.trimmed();
    if (g.length() >= 2 && g.at(0).isDigit() && g.at(1).isDigit()) {
        QString code = g.left(2);
        static const QMap<QString, QString> codeMap = {
            {"01", "Jammu & Kashmir"}, {"02", "Himachal Pradesh"}, {"03", "Punjab"},
            {"04", "Chandigarh"}, {"05", "Uttarakhand"}, {"06", "Haryana"},
            {"07", "Delhi"}, {"08", "Rajasthan"}, {"09", "Uttar Pradesh"},
            {"10", "Bihar"}, {"11", "Sikkim"}, {"12", "Arunachal Pradesh"},
            {"13", "Nagaland"}, {"14", "Manipur"}, {"15", "Mizoram"},
            {"16", "Tripura"}, {"17", "Meghalaya"}, {"18", "Assam"},
            {"19", "West Bengal"}, {"20", "Jharkhand"}, {"21", "Odisha"},
            {"22", "Chhattisgarh"}, {"23", "Madhya Pradesh"}, {"24", "Gujarat"},
            {"26", "Dadra & Nagar Haveli and Daman & Diu"}, {"27", "Maharashtra"},
            {"29", "Karnataka"}, {"30", "Goa"}, {"31", "Lakshadweep"},
            {"32", "Kerala"}, {"33", "Tamil Nadu"}, {"34", "Puducherry"},
            {"36", "Telangana"}, {"37", "Andhra Pradesh"}, {"38", "Ladakh"}
        };
        return codeMap.value(code, "");
    }
    return "";
}

QString PartiesModel::get_state_code_for_gstin(const QString& gstin) const {
    QString g = gstin.trimmed();
    if (g.length() >= 2 && g.at(0).isDigit() && g.at(1).isDigit()) {
        return g.left(2);
    }
    return "";
}

QStringList PartiesModel::get_prefixes() const {
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT DISTINCT TRIM(prefix) AS val FROM parties WHERE prefix IS NOT NULL AND TRIM(prefix) != '' "
        "ORDER BY val COLLATE NOCASE ASC;"
    );
    QStringList result;
    for (const QVariant& r : rows) {
        QString n = r.toMap().value("val").toString().trimmed();
        if (!n.isEmpty() && !result.contains(n, Qt::CaseInsensitive)) {
            result.append(n);
        }
    }
    QStringList standard = {"M/s", "Mr.", "Mrs.", "Shri", "Smt.", "Dr.", "Messrs"};
    for (const QString& s : standard) {
        if (!result.contains(s, Qt::CaseInsensitive)) {
            result.append(s);
        }
    }
    return result;
}

QStringList PartiesModel::get_party_types() const {
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT DISTINCT TRIM(party_type) AS val FROM parties WHERE party_type IS NOT NULL AND TRIM(party_type) != '' "
        "ORDER BY val COLLATE NOCASE ASC;"
    );
    QStringList result;
    for (const QVariant& r : rows) {
        QString n = r.toMap().value("val").toString().trimmed();
        if (!n.isEmpty() && !result.contains(n, Qt::CaseInsensitive)) {
            result.append(n);
        }
    }
    return result;
}

QStringList PartiesModel::get_special_types() const {
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT DISTINCT TRIM(special_type) AS val FROM parties WHERE special_type IS NOT NULL AND TRIM(special_type) != '' "
        "ORDER BY val COLLATE NOCASE ASC;"
    );
    QStringList result;
    for (const QVariant& r : rows) {
        QString n = r.toMap().value("val").toString().trimmed();
        if (!n.isEmpty() && !result.contains(n, Qt::CaseInsensitive)) {
            result.append(n);
        }
    }
    return result;
}

QStringList PartiesModel::get_gst_party_types() const {
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT DISTINCT TRIM(gst_party_type) AS val FROM parties WHERE gst_party_type IS NOT NULL AND TRIM(gst_party_type) != '' "
        "ORDER BY val COLLATE NOCASE ASC;"
    );
    QStringList result;
    for (const QVariant& r : rows) {
        QString n = r.toMap().value("val").toString().trimmed();
        if (!n.isEmpty() && !result.contains(n, Qt::CaseInsensitive)) {
            result.append(n);
        }
    }
    QStringList defaults = {"Registered", "Unregistered", "Composition", "Consumer", "Overseas", "SEZ Developer", "SEZ Unit", "UIN/Embassy", "Exempt"};
    for (const QString& d : defaults) {
        if (!result.contains(d, Qt::CaseInsensitive)) {
            result.append(d);
        }
    }
    return result;
}

QStringList PartiesModel::get_routes() const {
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT DISTINCT TRIM(route) AS val FROM parties WHERE route IS NOT NULL AND TRIM(route) != '' "
        "ORDER BY val COLLATE NOCASE ASC;"
    );
    QStringList result;
    for (const QVariant& r : rows) {
        QString n = r.toMap().value("val").toString().trimmed();
        if (!n.isEmpty() && !result.contains(n, Qt::CaseInsensitive)) {
            result.append(n);
        }
    }
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

    // 1. Exact match
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM parties WHERE name = ? COLLATE NOCASE LIMIT 1;",
        {cleanName}
    );
    if (!rows.isEmpty()) return rows.first().toMap();

    // 2. Match with non-breaking space replaced
    QString normName = cleanName;
    normName.replace(QChar(0x00A0), ' ');
    rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM parties WHERE REPLACE(name, char(160), ' ') = ? COLLATE NOCASE LIMIT 1;",
        {normName}
    );
    if (!rows.isEmpty()) return rows.first().toMap();

    // 3. Space-wildcard match
    QString pattern = normName;
    pattern.replace(' ', '%');
    rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM parties WHERE name LIKE ? OR alias LIKE ? LIMIT 1;",
        {"%" + pattern + "%", "%" + pattern + "%"}
    );
    if (!rows.isEmpty()) return rows.first().toMap();

    // 4. Base name without bracket [City]
    QString baseName = normName;
    int bIdx = baseName.indexOf('[');
    if (bIdx > 0) baseName = baseName.left(bIdx).trimmed();
    if (!baseName.isEmpty()) {
        rows = DatabaseManager::instance().executeQuery(
            "SELECT * FROM parties WHERE name LIKE ? OR alias LIKE ? OR mailing_name LIKE ? LIMIT 1;",
            {"%" + baseName + "%", "%" + baseName + "%", "%" + baseName + "%"}
        );
        if (!rows.isEmpty()) return rows.first().toMap();
    }

    return QVariantMap();
}

QVariantList PartiesModel::search_parties(const QString& query) const {
    QString q = query.trimmed().toLower();
    if (q.isEmpty()) return {};

    static const QRegularExpression splitRegex(QStringLiteral("[\\s\\-\\/\\.\\[\\]\\(\\)\\,\\&]+"));
    QStringList queryTokens = q.split(splitRegex, Qt::SkipEmptyParts);
    if (queryTokens.isEmpty()) return {};

    QVariantList allParties = DatabaseManager::instance().executeQuery(
        "SELECT id, legacy_id, name, group_name, party_type, phone, city, gstin, opening_balance, balance_type "
        "FROM parties;"
    );

    struct RankedItem {
        QVariantMap data;
        int tier;          // 0: 1st word prefix, 1: 2nd word prefix, 2: 3rd+ word prefix, 3: City prefix, 4: Group prefix
        int wordIndex;
        int nameLength;
        QString name;
    };

    QList<RankedItem> matches;
    matches.reserve(allParties.size());

    for (const QVariant& rowVar : allParties) {
        QVariantMap row = rowVar.toMap();
        QString name = row.value("name").toString().trimmed();
        QString city = row.value("city").toString().trimmed();
        QString group = row.value("group_name").toString().trimmed();

        QString nameLower = name.toLower();
        QString cityLower = city.toLower();
        QString groupLower = group.toLower();

        QString bracketLocation;
        int bStart = nameLower.indexOf('[');
        int bEnd = nameLower.indexOf(']', bStart);
        if (bStart != -1 && bEnd > bStart) {
            bracketLocation = nameLower.mid(bStart + 1, bEnd - bStart - 1).trimmed();
        }

        QString cleanNamePart = (bStart != -1) ? nameLower.left(bStart).trimmed() : nameLower;
        QStringList nameWords = cleanNamePart.split(splitRegex, Qt::SkipEmptyParts);
        QStringList locationWords = bracketLocation.split(splitRegex, Qt::SkipEmptyParts);
        if (!cityLower.isEmpty()) {
            locationWords.append(cityLower.split(splitRegex, Qt::SkipEmptyParts));
        }
        QStringList groupWords = groupLower.split(splitRegex, Qt::SkipEmptyParts);

        bool allTokensMatched = true;
        int bestTier = 999;
        int bestWordIdx = 999;

        for (int t = 0; t < queryTokens.size(); ++t) {
            const QString& tok = queryTokens.at(t);
            bool tokenFound = false;

            for (int w = 0; w < nameWords.size(); ++w) {
                if (nameWords.at(w).startsWith(tok)) {
                    tokenFound = true;
                    int curTier = (w == 0) ? 0 : ((w == 1) ? 1 : 2);
                    if (curTier < bestTier) {
                        bestTier = curTier;
                        bestWordIdx = w;
                    }
                    break;
                }
            }

            if (!tokenFound) {
                for (int lw = 0; lw < locationWords.size(); ++lw) {
                    if (locationWords.at(lw).startsWith(tok)) {
                        tokenFound = true;
                        int curTier = 3;
                        if (curTier < bestTier) {
                            bestTier = curTier;
                            bestWordIdx = 100 + lw;
                        }
                        break;
                    }
                }
            }

            if (!tokenFound) {
                for (int gw = 0; gw < groupWords.size(); ++gw) {
                    if (groupWords.at(gw).startsWith(tok)) {
                        tokenFound = true;
                        int curTier = 4;
                        if (curTier < bestTier) {
                            bestTier = curTier;
                            bestWordIdx = 200 + gw;
                        }
                        break;
                    }
                }
            }

            if (!tokenFound) {
                allTokensMatched = false;
                break;
            }
        }

        if (allTokensMatched && bestTier < 999) {
            RankedItem item;
            item.data = row;
            item.tier = bestTier;
            item.wordIndex = bestWordIdx;
            item.nameLength = name.length();
            item.name = name;
            matches.append(item);
        }
    }

    std::sort(matches.begin(), matches.end(), [](const RankedItem& a, const RankedItem& b) {
        if (a.tier != b.tier) {
            return a.tier < b.tier;
        }
        if (a.wordIndex != b.wordIndex) {
            return a.wordIndex < b.wordIndex;
        }
        if (a.nameLength != b.nameLength) {
            return a.nameLength < b.nameLength;
        }
        return QString::compare(a.name, b.name, Qt::CaseInsensitive) < 0;
    });

    QVariantList results;
    int limit = qMin(matches.size(), 40);
    for (int i = 0; i < limit; ++i) {
        results.append(matches.at(i).data);
    }

    return results;
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

QVariantMap PartiesModel::get_total_opening_balance_summary(int excludePartyId, double pendingAmount, const QString& pendingBalType) const {
    double totalDr = 0.0;
    double totalCr = 0.0;

    QString sql = "SELECT id, opening_balance, balance_type FROM parties WHERE opening_balance > 0.0001";
    QVariantList params;
    if (excludePartyId > 0) {
        sql += " AND id != ?";
        params << excludePartyId;
    }
    QVariantList rows = DatabaseManager::instance().executeQuery(sql, params);
    for (const auto& r : rows) {
        QVariantMap row = r.toMap();
        double op = row.value("opening_balance").toDouble();
        QString bType = row.value("balance_type").toString().trimmed();
        if (bType.compare("Cr", Qt::CaseInsensitive) == 0) {
            totalCr += op;
        } else {
            totalDr += op;
        }
    }

    if (pendingAmount > 0.0001) {
        if (pendingBalType.compare("Cr", Qt::CaseInsensitive) == 0) {
            totalCr += pendingAmount;
        } else {
            totalDr += pendingAmount;
        }
    }

    double diff = totalDr - totalCr;
    QString diffType = "Balanced";
    if (diff > 0.0001) {
        diffType = "Dr";
    } else if (diff < -0.0001) {
        diffType = "Cr";
    }

    QVariantMap res;
    res["total_dr"] = totalDr;
    res["total_cr"] = totalCr;
    res["diff"] = std::abs(diff);
    res["diff_type"] = diffType;
    return res;
}

QString PartiesModel::get_financial_year_start() const {
    QVariant fyStart = DatabaseManager::instance().executeScalar(
        "SELECT start_date FROM financial_years WHERE is_active = 1 LIMIT 1;"
    );
    if (fyStart.isValid() && !fyStart.toString().trimmed().isEmpty()) {
        QDate d = QDate::fromString(fyStart.toString().trimmed(), "yyyy-MM-dd");
        if (d.isValid()) return d.toString("dd-MM-yyyy");
    }
    return "01-04-2023";
}

bool PartiesModel::add_ledger_extended(const QVariantMap& data) {
    bool ok = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO parties ("
        "name, alias, prefix, group_name, party_type, special_type, "
        "opening_balance, balance_type, mailing_name, address, city, district, state, state_code, pincode, route, "
        "mobile, whatsapp, phone, email, contact_person, pan, aadhaar, tan, gstin, gst_party_type, "
        "bank_name, bank_account, ifsc_code, credit_limit, credit_days, interest_rate, commission_rate, commission_on, "
        "apply_tcs, tcs_exempt, party_station, use_routes, shop_no, tin, urn, stock_not_calc, use_credit_limit, "
        "show_date_totals, calc_direct_expense, set_title_case, ledger_open_from, books_start_from) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {
            data.value("name").toString().trimmed(),
            data.value("alias").toString().trimmed(),
            data.value("prefix", "M/s").toString().trimmed(),
            data.value("group_name", "Sundry Debtors").toString().trimmed(),
            data.value("party_type", "Buyer").toString().trimmed(),
            data.value("special_type").toString().trimmed(),
            data.value("opening_balance", 0.0).toDouble(),
            data.value("balance_type", "Dr").toString().trimmed(),
            data.value("mailing_name").toString().trimmed(),
            data.value("address").toString().trimmed(),
            data.value("city").toString().trimmed(),
            data.value("district").toString().trimmed(),
            data.value("state").toString().trimmed(),
            data.value("state_code").toString().trimmed(),
            data.value("pincode").toString().trimmed(),
            data.value("route").toString().trimmed(),
            data.value("mobile").toString().trimmed(),
            data.value("whatsapp").toString().trimmed(),
            data.value("phone").toString().trimmed(),
            data.value("email").toString().trimmed(),
            data.value("contact_person").toString().trimmed(),
            data.value("pan").toString().trimmed(),
            data.value("aadhaar").toString().trimmed(),
            data.value("tan").toString().trimmed(),
            data.value("gstin").toString().trimmed(),
            data.value("gst_party_type", "Unregistered").toString().trimmed(),
            data.value("bank_name").toString().trimmed(),
            data.value("bank_account").toString().trimmed(),
            data.value("ifsc_code").toString().trimmed(),
            data.value("credit_limit", 0.0).toDouble(),
            data.value("credit_days", 30).toInt(),
            data.value("interest_rate", 0.0).toDouble(),
            data.value("commission_rate", 0.0).toDouble(),
            data.value("commission_on").toString().trimmed(),
            data.value("apply_tcs", 0).toInt(),
            data.value("tcs_exempt", 0).toInt(),
            data.value("party_station").toString().trimmed(),
            data.value("use_routes", 0).toInt(),
            data.value("shop_no").toString().trimmed(),
            data.value("tin").toString().trimmed(),
            data.value("urn").toString().trimmed(),
            data.value("stock_not_calc", 0).toInt(),
            data.value("use_credit_limit", 1).toInt(),
            data.value("show_date_totals", 0).toInt(),
            data.value("calc_direct_expense", 0).toInt(),
            data.value("set_title_case", 1).toInt(),
            data.value("ledger_open_from").toString().trimmed(),
            data.value("books_start_from", "01-04-2023").toString().trimmed()
        }
    );
    if (ok) reload_data();
    return ok;
}

bool PartiesModel::update_ledger_extended(int party_id, const QVariantMap& data) {
    if (party_id <= 0) return false;
    bool ok = DatabaseManager::instance().executeNonQuery(
        "UPDATE parties SET "
        "name = ?, alias = ?, prefix = ?, group_name = ?, party_type = ?, special_type = ?, "
        "opening_balance = ?, balance_type = ?, mailing_name = ?, address = ?, city = ?, district = ?, state = ?, state_code = ?, pincode = ?, route = ?, "
        "mobile = ?, whatsapp = ?, phone = ?, email = ?, contact_person = ?, pan = ?, aadhaar = ?, tan = ?, gstin = ?, gst_party_type = ?, "
        "bank_name = ?, bank_account = ?, ifsc_code = ?, credit_limit = ?, credit_days = ?, interest_rate = ?, commission_rate = ?, commission_on = ?, "
        "apply_tcs = ?, tcs_exempt = ?, party_station = ?, use_routes = ?, shop_no = ?, tin = ?, urn = ?, stock_not_calc = ?, use_credit_limit = ?, "
        "show_date_totals = ?, calc_direct_expense = ?, set_title_case = ?, ledger_open_from = ?, books_start_from = ? "
        "WHERE id = ?;",
        {
            data.value("name").toString().trimmed(),
            data.value("alias").toString().trimmed(),
            data.value("prefix", "M/s").toString().trimmed(),
            data.value("group_name", "Sundry Debtors").toString().trimmed(),
            data.value("party_type", "Buyer").toString().trimmed(),
            data.value("special_type").toString().trimmed(),
            data.value("opening_balance", 0.0).toDouble(),
            data.value("balance_type", "Dr").toString().trimmed(),
            data.value("mailing_name").toString().trimmed(),
            data.value("address").toString().trimmed(),
            data.value("city").toString().trimmed(),
            data.value("district").toString().trimmed(),
            data.value("state").toString().trimmed(),
            data.value("state_code").toString().trimmed(),
            data.value("pincode").toString().trimmed(),
            data.value("route").toString().trimmed(),
            data.value("mobile").toString().trimmed(),
            data.value("whatsapp").toString().trimmed(),
            data.value("phone").toString().trimmed(),
            data.value("email").toString().trimmed(),
            data.value("contact_person").toString().trimmed(),
            data.value("pan").toString().trimmed(),
            data.value("aadhaar").toString().trimmed(),
            data.value("tan").toString().trimmed(),
            data.value("gstin").toString().trimmed(),
            data.value("gst_party_type", "Unregistered").toString().trimmed(),
            data.value("bank_name").toString().trimmed(),
            data.value("bank_account").toString().trimmed(),
            data.value("ifsc_code").toString().trimmed(),
            data.value("credit_limit", 0.0).toDouble(),
            data.value("credit_days", 30).toInt(),
            data.value("interest_rate", 0.0).toDouble(),
            data.value("commission_rate", 0.0).toDouble(),
            data.value("commission_on").toString().trimmed(),
            data.value("apply_tcs", 0).toInt(),
            data.value("tcs_exempt", 0).toInt(),
            data.value("party_station").toString().trimmed(),
            data.value("use_routes", 0).toInt(),
            data.value("shop_no").toString().trimmed(),
            data.value("tin").toString().trimmed(),
            data.value("urn").toString().trimmed(),
            data.value("stock_not_calc", 0).toInt(),
            data.value("use_credit_limit", 1).toInt(),
            data.value("show_date_totals", 0).toInt(),
            data.value("calc_direct_expense", 0).toInt(),
            data.value("set_title_case", 1).toInt(),
            data.value("ledger_open_from").toString().trimmed(),
            data.value("books_start_from", "01-04-2023").toString().trimmed(),
            party_id
        }
    );
    if (ok) reload_data();
    return ok;
}

bool PartiesModel::add_ledger_full(
    const QString& name, const QString& alias, const QString& prefix,
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
        "INSERT INTO parties ("
        "name, alias, prefix, group_name, party_type, special_type, "
        "opening_balance, balance_type, mailing_name, address, city, district, state, "
        "pincode, phone, mobile, whatsapp, email, contact_person, gstin, pan, "
        "aadhaar, credit_limit, credit_days, bank_name, bank_account, ifsc_code"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {
            name, alias, prefix, group_name, party_type, special_type,
            opening_balance, balance_type, mailing_name, address, city, district, state,
            pincode, phone, mobile, whatsapp, email, contact_person, gstin, pan,
            aadhaar, credit_limit, credit_days, bank_name, bank_account, ifsc_code
        }
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
    FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(isoDate);
    return fy.name;
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
        sql += " ORDER BY voucher_date ASC, CAST(voucher_no AS INTEGER) ASC, id ASC;";

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

            if (rawType == "TDS" || vType == "TDS") {
                rawType = "TDS";
                vType = "TDS";
            }
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
            } else if (rawType == "TDS" || vType == "TDS") {
                desc = !narr.isEmpty() ? narr : QString("TDS: %1").arg(!opposing.isEmpty() ? opposing : "TDS");
            } else if (rawType == "Jrnl" || vType == "Journal") {
                desc = QString("Journal: %1").arg(!opposing.isEmpty() ? opposing : "A/c");
            } else if (rawType == "JFrm" || vType == "J-Form") {
                desc = QString("J-Form: %1").arg(!opposing.isEmpty() ? opposing : "Paddy Purchase");
            } else {
                desc = QString("%1: %2").arg(vType, opposing);
            }

            if (!veh.isEmpty()) desc += " | Veh: " + veh;
            if (!broker.isEmpty()) desc += " | Broker: " + broker;
            if (!narr.isEmpty() && rawType != "TDS" && vType != "TDS" && !desc.contains(narr)) desc += " | " + narr;

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
        sql += " ORDER BY voucher_date ASC, CAST(voucher_no AS INTEGER) ASC, id ASC;";

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
