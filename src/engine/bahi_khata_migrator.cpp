#include "bahi_khata_migrator.h"
#include "../database_manager.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QDebug>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cmath>

#include "mdbtools.h"
#define HAS_LIBMDB 1

static QString normalizeMdbPath(const QString& rawPath) {
    QString path = rawPath.trimmed();
    if (path.startsWith("file://", Qt::CaseInsensitive)) {
        QUrl url(path);
        if (url.isValid() && !url.toLocalFile().isEmpty()) {
            path = url.toLocalFile();
        } else {
            path = path.mid(7);
        }
    }
    // Remove leading slash for Windows drive letters like "/Z:/..." or "/C:/..."
    if (path.length() >= 3 && path[0] == '/' && path[1].isLetter() && path[2] == ':') {
        path = path.mid(1);
    }
    return QDir::toNativeSeparators(path);
}

BahiKhataMigrator::BahiKhataMigrator(QObject* parent)
    : QObject(parent)
{
}

BahiKhataMigrator::~BahiKhataMigrator() = default;

void BahiKhataMigrator::updateProgress(int percent, const QString& status) {
    m_progressPercent = percent;
    m_statusText = status;
    emit progressChanged(percent);
    emit statusChanged(status);
    emit migrationProgress(percent, status);
    QCoreApplication::processEvents();
}

QString BahiKhataMigrator::parseMdbDate(const QString& rawDate) {
    if (rawDate.trimmed().isEmpty()) return "";
    QString clean = rawDate.trimmed().split(' ').first();
    QStringList parts = clean.split('/');
    if (parts.size() == 3) {
        int m = parts[0].toInt();
        int d = parts[1].toInt();
        int y = parts[2].toInt();
        int fullYear = (y < 100) ? (2000 + y) : y;
        return QString("%1-%2-%3")
            .arg(fullYear, 4, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(d, 2, 10, QChar('0'));
    }
    return rawDate;
}

QString BahiKhataMigrator::determineFinancialYear(const QString& isoDate) {
    if (isoDate.length() < 7) return "FY 2025-26";
    int year = isoDate.left(4).toInt();
    int month = isoDate.mid(5, 2).toInt();
    if (month >= 4) {
        return QString("FY %1-%2").arg(year).arg(QString::number(year + 1).right(2));
    } else {
        return QString("FY %1-%2").arg(year - 1).arg(QString::number(year).right(2));
    }
}

#if HAS_LIBMDB
static std::string getField(const std::map<std::string, std::string>& m, const std::string& key, const std::string& def = "") {
    auto it = m.find(key);
    if (it != m.end()) {
        return it->second;
    }
    for (const auto& kv : m) {
        if (kv.first.size() == key.size()) {
            bool eq = true;
            for (size_t i = 0; i < key.size(); ++i) {
                if (std::tolower(static_cast<unsigned char>(kv.first[i])) != std::tolower(static_cast<unsigned char>(key[i]))) {
                    eq = false;
                    break;
                }
            }
            if (eq) return kv.second;
        }
    }
    return def;
}

static std::vector<std::map<std::string, std::string>> readTableRows(MdbHandle* mdb, const char* tableName) {
    std::vector<std::map<std::string, std::string>> result;
    MdbTableDef* table = mdb_read_table_by_name(mdb, const_cast<char*>(tableName), MDB_TABLE);
    if (!table) return result;

    mdb_read_columns(table);
    unsigned int numCols = table->num_cols;
    if (numCols == 0) {
        mdb_free_tabledef(table);
        return result;
    }

    std::vector<std::string> colNames(numCols);
    std::vector<std::vector<char>> colBuffers(numCols, std::vector<char>(4096, 0));

    for (unsigned int j = 0; j < numCols; j++) {
        MdbColumn* col = static_cast<MdbColumn*>(g_ptr_array_index(table->columns, j));
        colNames[j] = col->name;
        mdb_bind_column(table, j + 1, colBuffers[j].data(), nullptr);
    }

    mdb_rewind_table(table);
    while (mdb_fetch_row(table)) {
        std::map<std::string, std::string> row;
        for (unsigned int j = 0; j < numCols; j++) {
            row[colNames[j]] = colBuffers[j].data();
        }
        result.push_back(std::move(row));
    }

    mdb_free_tabledef(table);
    return result;
}
#endif

QVariantMap BahiKhataMigrator::inspect_mdb_file(const QString& mdbFilePath) {
    QVariantMap result;
    result["valid"] = false;

    QString cleanPath = normalizeMdbPath(mdbFilePath);

    QFileInfo fi(cleanPath);
    if (!fi.exists()) {
        result["error"] = "File not found: " + cleanPath;
        return result;
    }

#if HAS_LIBMDB
    MdbHandle* mdb = mdb_open(cleanPath.toUtf8().constData(), MDB_NOFLAGS);
    if (!mdb) {
        result["error"] = "Unable to open Jet database file. Ensure it is a valid .mdb / .004 file.";
        return result;
    }

    GPtrArray* tables = mdb_read_catalog(mdb, MDB_TABLE);
    QVariantList tableNames;
    if (tables) {
        for (unsigned int i = 0; i < tables->len; i++) {
            MdbCatalogEntry* entry = static_cast<MdbCatalogEntry*>(g_ptr_array_index(tables, i));
            if (entry && entry->object_type == MDB_TABLE) {
                tableNames.append(QString::fromUtf8(entry->object_name));
            }
        }
    }

    result["valid"] = true;
    result["filePath"] = cleanPath;
    result["fileName"] = fi.fileName();
    result["fileSize"] = fi.size();
    result["tables"] = tableNames;
    result["tableCount"] = tableNames.size();

    // Quick counts of key tables
    auto stockItems = readTableRows(mdb, "StockItems");
    auto ledgers = readTableRows(mdb, "Ledgers");
    auto stockTx = readTableRows(mdb, "StockTransactions");
    auto milling = readTableRows(mdb, "MillingVouchers");

    result["stockItemsCount"] = static_cast<int>(stockItems.size());
    result["ledgersCount"] = static_cast<int>(ledgers.size());
    result["stockTxCount"] = static_cast<int>(stockTx.size());
    result["millingCount"] = static_cast<int>(milling.size());

    mdb_close(mdb);
#else
    result["error"] = "libmdb is not compiled into this build.";
#endif

    return result;
}

bool BahiKhataMigrator::migrate_mdb_file(const QString& mdbFilePath) {
    QString cleanPath = normalizeMdbPath(mdbFilePath);

    QFileInfo fi(cleanPath);
    if (!fi.exists()) {
        emit migrationFinished(false, "MDB file does not exist: " + cleanPath);
        return false;
    }

#if HAS_LIBMDB
    m_isMigrating = true;
    emit migratingChanged();

    updateProgress(5, "Opening Bahi Khata Database: " + fi.fileName());

    MdbHandle* mdb = mdb_open(cleanPath.toUtf8().constData(), MDB_NOFLAGS);
    if (!mdb) {
        m_isMigrating = false;
        emit migratingChanged();
        emit migrationFinished(false, "Failed to read MDB database.");
        return false;
    }

    updateProgress(15, "Reading Master Ledgers and Groups...");
    auto groupsRows = readTableRows(mdb, "Groups");
    auto ledgersRows = readTableRows(mdb, "Ledgers");
    auto stockItemsRows = readTableRows(mdb, "StockItems");
    auto transactionsRows = readTableRows(mdb, "Transactions");
    auto stockTxRows = readTableRows(mdb, "StockTransactions");
    auto millingRows = readTableRows(mdb, "MillingVouchers");

    updateProgress(35, "Connecting to SQLite Database...");
    auto& db = DatabaseManager::instance();
    db.beginTransaction();

    // Map Lookups
    std::map<std::string, std::string> groupMap; // code -> name
    for (const auto& g : groupsRows) {
        groupMap[getField(g, "Code1st")] = getField(g, "GroupName");
    }

    std::map<std::string, std::string> ledgerMap; // code -> name
    std::map<std::string, std::string> ledgerGroupMap; // code -> group
    for (const auto& l : ledgersRows) {
        std::string code = getField(l, "Code1st");
        std::string name = getField(l, "LedgerName");
        std::string grpCode = getField(l, "GroupCode");
        ledgerMap[code] = name;
        ledgerGroupMap[code] = groupMap[grpCode];
    }

    std::map<std::string, std::string> itemMap; // code -> name
    for (const auto& item : stockItemsRows) {
        itemMap[getField(item, "Code1st")] = getField(item, "ItemName");
    }

    // Ensure active financial years exist
    QVariant fyCount = db.executeScalar("SELECT COUNT(*) FROM financial_years;");
    if (!fyCount.isValid() || fyCount.toLongLong() == 0) {
        db.executeNonQuery(
            "INSERT INTO financial_years (year_name, start_date, end_date, is_active, is_locked) "
            "VALUES ('FY 2024-25', '2024-04-01', '2025-03-31', 0, 0), "
            "       ('FY 2025-26', '2025-04-01', '2026-03-31', 1, 0), "
            "       ('FY 2026-27', '2026-04-01', '2027-03-31', 0, 0);"
        );
    }

    // 1. Sync Account Groups
    updateProgress(38, QString("Importing %1 Account Groups...").arg(groupsRows.size()));
    db.executeNonQuery("DELETE FROM account_groups;");
    for (const auto& g : groupsRows) {
        std::string gName = getField(g, "GroupName");
        if (gName.empty()) continue;
        std::string pName = getField(g, "ParentName", "Primary");
        std::string nature = "Assets";
        if (gName.find("Income") != std::string::npos || gName.find("Sale") != std::string::npos) nature = "Income";
        else if (gName.find("Expense") != std::string::npos || gName.find("Purchase") != std::string::npos) nature = "Expense";
        else if (gName.find("Liabilit") != std::string::npos || gName.find("Creditor") != std::string::npos || gName.find("Capital") != std::string::npos) nature = "Liabilities";

        db.executeNonQuery(
            "INSERT OR REPLACE INTO account_groups (name, parent_group_name, nature, description, extract_in_balance_sheet, is_system) "
            "VALUES (?, ?, ?, ?, 1, 0);",
            {QString::fromStdString(gName), QString::fromStdString(pName), QString::fromStdString(nature), QString::fromStdString(gName)}
        );
    }

    // 2. Sync Parties / Ledgers
    updateProgress(42, QString("Importing %1 Parties / Ledgers...").arg(ledgersRows.size()));
    db.executeNonQuery("DELETE FROM parties;");
    for (const auto& l : ledgersRows) {
        std::string lName = getField(l, "LedgerName");
        if (lName.empty()) continue;
        std::string code = getField(l, "Code1st");
        int legacyId = 0;
        try { if (!code.empty()) legacyId = std::stoi(code); } catch (...) {}
        std::string grpCode = getField(l, "GroupCode");
        std::string grpName = groupMap[grpCode];
        if (grpName.empty()) grpName = "Sundry Debtors";

        double opBal = 0.0;
        try { std::string b = getField(l, "OpeningBal"); if (!b.empty()) opBal = std::stod(b); } catch (...) {}
        std::string balType = getField(l, "OpeningType", "Dr");

        std::string partyType = (grpName.find("Debtor") != std::string::npos) ? "Buyer" :
                                ((grpName.find("Creditor") != std::string::npos) ? "Vendor" : "Merchant");
        std::string specialType = (partyType == "Buyer") ? "Rice Buyer" : "Paddy Seller";

        db.executeNonQuery(
            "INSERT INTO parties (name, alias, prefix, group_name, party_type, special_type, "
            "opening_balance, balance_type, mailing_name, address, city, district, state, state_code, pincode, route, "
            "mobile, whatsapp, phone, email, contact_person, pan, aadhaar, tan, gstin, gst_party_type, "
            "bank_name, bank_account, ifsc_code, credit_limit, credit_days, interest_rate, commission_rate, "
            "commission_on, apply_tcs, tcs_exempt, legacy_id) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 30, ?, ?, ?, ?, ?, ?);",
            {
                QString::fromStdString(lName),
                QString::fromStdString(getField(l, "LedgerAlais")),
                QString::fromStdString(getField(l, "Prefix", "M/s")),
                QString::fromStdString(grpName),
                QString::fromStdString(partyType),
                QString::fromStdString(specialType),
                opBal,
                QString::fromStdString(balType),
                QString::fromStdString(getField(l, "MailingName", lName)),
                QString::fromStdString(getField(l, "MailingAdd")),
                QString::fromStdString(getField(l, "Station")),
                QString::fromStdString(getField(l, "Distt")),
                QString::fromStdString(getField(l, "STATE", "Haryana")),
                QString::fromStdString(getField(l, "PartyState")),
                QString::fromStdString(getField(l, "PartyPINcode")),
                QString::fromStdString(getField(l, "Route")),
                QString::fromStdString(getField(l, "MobNoForSMS")),
                QString::fromStdString(getField(l, "WhatsappNo")),
                QString::fromStdString(getField(l, "Phone_O")),
                QString::fromStdString(getField(l, "Email")),
                QString::fromStdString(getField(l, "ConcernedPerson")),
                QString::fromStdString(getField(l, "IncomeTaxNo")),
                QString::fromStdString(getField(l, "AadharNo")),
                QString::fromStdString(getField(l, "PartyTAN")),
                QString::fromStdString(getField(l, "GSTIN")),
                QString::fromStdString(getField(l, "GSTPartyType")),
                QString::fromStdString(getField(l, "BankName")),
                QString::fromStdString(getField(l, "BankAccount")),
                QString::fromStdString(getField(l, "IFSCCode")),
                0.0,
                0.0,
                0.0,
                "",
                0,
                0,
                legacyId
            }
        );
    }

    // 3. Sync Stock Items & Inventory
    updateProgress(46, QString("Importing %1 Stock Items...").arg(stockItemsRows.size()));
    db.executeNonQuery("DELETE FROM stock_items;");
    db.executeNonQuery("DELETE FROM inventory;");
    for (const auto& item : stockItemsRows) {
        std::string iName = getField(item, "ItemName");
        if (iName.empty()) continue;
        std::string code = getField(item, "Code1st");
        int legacyCode = 0;
        try { if (!code.empty()) legacyCode = std::stoi(code); } catch (...) {}

        double purcRate = 0.0, saleRate = 0.0, mrp = 0.0, discount = 0.0, packing = 26.0;
        int opBags = 0;
        double opQty = 0.0, opRate = 0.0, opVal = 0.0;
        try { std::string v = getField(item, "PurcRate"); if (!v.empty()) purcRate = std::stod(v); } catch (...) {}
        try { std::string v = getField(item, "SaleRate"); if (!v.empty()) saleRate = std::stod(v); } catch (...) {}
        try { std::string v = getField(item, "MRP"); if (!v.empty()) mrp = std::stod(v); } catch (...) {}
        try { std::string v = getField(item, "Discount"); if (!v.empty()) discount = std::stod(v); } catch (...) {}
        try { std::string v = getField(item, "Packing"); if (!v.empty()) packing = std::stod(v); } catch (...) {}
        try { std::string v = getField(item, "OpeningBags"); if (!v.empty()) opBags = std::stoi(v); } catch (...) {}
        try { std::string v = getField(item, "OpeningQty"); if (!v.empty()) opQty = std::stod(v); } catch (...) {}
        try { std::string v = getField(item, "OpeningRate"); if (!v.empty()) opRate = std::stod(v); } catch (...) {}
        try { std::string v = getField(item, "OpeningValue"); if (!v.empty()) opVal = std::stod(v); } catch (...) {}
        if (opVal <= 0.0 && opQty > 0.0 && opRate > 0.0) opVal = opQty * opRate;

        std::string gstSlab = getField(item, "GSTRateSlab");
        double gstRate = 5.0;
        if (!gstSlab.empty()) {
            try { gstRate = std::stod(gstSlab); } catch (...) {}
        } else {
            try { std::string v = getField(item, "VAT"); if (!v.empty()) gstRate = std::stod(v); } catch (...) {}
        }

        std::string itemType = "Finished Rice";
        std::string nLower = iName;
        std::transform(nLower.begin(), nLower.end(), nLower.begin(), ::tolower);
        if (nLower.find("paddy") != std::string::npos) {
            itemType = (nLower.find("husk") != std::string::npos) ? "By-Product" : "Raw Paddy";
        } else if (nLower.find("bran") != std::string::npos || nLower.find("husk") != std::string::npos ||
                   nLower.find("broken") != std::string::npos || nLower.find("nakku") != std::string::npos) {
            itemType = "By-Product";
        } else if (nLower.find("bag") != std::string::npos || nLower.find("bardana") != std::string::npos) {
            itemType = "Packing Material";
        }

        db.executeNonQuery(
            "INSERT INTO stock_items ("
            "name, code, item_type, goods_type, company_name, category_name, unit, "
            "purchase_rate, sale_rate, mrp, discount, hsn_code, gst_rate, cess_rate, packing_kg, "
            "opening_bags, opening_qty, opening_rate, opening_value, "
            "purchase_ledger, sale_ledger, stock_ledger, is_milling_item, include_in_trading, calculate_stock, "
            "dami_rate, market_fee_rate, hrdf_rate, legacy_code) "
            "VALUES (?, ?, ?, 'Goods', 'Mill Master', ?, 'Qtl', "
            "?, ?, ?, ?, ?, ?, 0.0, ?, "
            "?, ?, ?, ?, "
            "'Purchase Accounts', 'Sales Accounts', 'Stock-in-Hand', 0, 1, 1, "
            "0.0, 0.0, 0.0, ?);",
            {
                QString::fromStdString(iName),
                QString::fromStdString(code),
                QString::fromStdString(itemType),
                QString::fromStdString(itemType),
                purcRate,
                saleRate,
                mrp,
                discount,
                QString::fromStdString(getField(item, "HSNCode", "1006")),
                gstRate,
                packing,
                opBags,
                opQty,
                opRate,
                opVal,
                legacyCode
            }
        );

        db.executeNonQuery(
            "INSERT INTO inventory (item_code, item_name, category, current_stock_qtl, sale_rate, gst_rate, packing_kg) "
            "VALUES (?, ?, ?, ?, ?, ?, ?);",
            {
                QString("ITEM-%1").arg(legacyCode),
                QString::fromStdString(iName),
                QString::fromStdString(itemType),
                opQty,
                saleRate,
                QString("%1%").arg(gstRate),
                static_cast<int>(packing)
            }
        );
    }

    // 4. Sync Vouchers
    updateProgress(50, QString("Importing %1 Vouchers...").arg(transactionsRows.size()));
    db.executeNonQuery("DELETE FROM vouchers;");
    for (const auto& t : transactionsRows) {
        std::string vNum = getField(t, "VoucherNumber");
        if (vNum.empty()) continue;
        std::string rawType = getField(t, "TransType");
        std::string vType = "Journal";
        if (rawType == "SL" || rawType == "SALE" || rawType == "SALES") vType = "Sales";
        else if (rawType == "PU" || rawType == "PURC" || rawType == "PURCHASE" || rawType == "PR") vType = "Purchase";
        else if (rawType == "PY" || rawType == "PAYMENT" || rawType == "P") vType = "Payment";
        else if (rawType == "RC" || rawType == "RECEIPT" || rawType == "R") vType = "Receipt";
        else if (rawType == "CN" || rawType == "CONTRA" || rawType == "C") vType = "Contra";

        QString vDate = parseMdbDate(QString::fromStdString(getField(t, "VoucherDate")));
        QString fy = determineFinancialYear(vDate);
        std::string acc = getField(t, "AccountCode");
        std::string lName = ledgerMap[acc];
        if (lName.empty()) lName = "Account " + acc;

        double amt = 0.0;
        try { std::string v = getField(t, "Amount"); if (!v.empty()) amt = std::stod(v); } catch (...) {}
        std::string drcr = getField(t, "DrCr", "Dr");

        db.executeNonQuery(
            "INSERT INTO vouchers (financial_year, voucher_no, voucher_date, voucher_type, legacy_type, "
            "party_name, account_type, amount, narration, instrument_no) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                fy,
                QString::fromStdString(vNum),
                vDate,
                QString::fromStdString(vType),
                QString::fromStdString(rawType),
                QString::fromStdString(lName),
                QString::fromStdString(drcr),
                amt,
                QString::fromStdString(getField(t, "Narration")),
                QString::fromStdString(getField(t, "ChequeNo"))
            }
        );
    }

    // Build Voucher -> (Party, InvoiceNo) map
    std::map<std::tuple<std::string, std::string, std::string>, std::pair<std::string, std::string>> vchInfoMap;
    for (const auto& t : transactionsRows) {
        std::string vNum = getField(t, "VoucherNumber");
        std::string tType = getField(t, "TransType");
        std::string vDate = parseMdbDate(QString::fromStdString(getField(t, "VoucherDate"))).toStdString();
        std::string invNo = getField(t, "InvoiceNo");
        std::string acc = getField(t, "AccountCode");
        std::string lName = ledgerMap[acc];
        std::string grpName = ledgerGroupMap[acc];

        bool isParty = (grpName.find("Debtor") != std::string::npos ||
                        grpName.find("Creditor") != std::string::npos ||
                        grpName.find("Cash") != std::string::npos ||
                        grpName.find("Bank") != std::string::npos ||
                        grpName.find("Party") != std::string::npos ||
                        lName.find("A/c") == std::string::npos);

        if (lName.find("Sale") != std::string::npos || lName.find("Purc") != std::string::npos ||
            lName.find("T.D.S.") != std::string::npos || lName.find("GST") != std::string::npos ||
            lName.find("Round") != std::string::npos || lName.find("Tax") != std::string::npos) {
            isParty = false;
        }

        auto key = std::make_tuple(vNum, tType, vDate);
        if (vchInfoMap.find(key) == vchInfoMap.end()) {
            vchInfoMap[key] = {isParty ? lName : "", invNo};
        } else {
            if (isParty && vchInfoMap[key].first.empty()) vchInfoMap[key].first = lName;
            if (!invNo.empty() && vchInfoMap[key].second.empty()) vchInfoMap[key].second = invNo;
        }
    }

    // 5. Sync Stock Transactions
    updateProgress(65, QString("Importing %1 Stock Transactions...").arg(stockTxRows.size()));
    db.executeQuery("DELETE FROM stock_transactions;");

    int stIndex = 1;
    for (const auto& st : stockTxRows) {
        QString vDate = parseMdbDate(QString::fromStdString(st.at("VoucherDate")));
        QString fy = determineFinancialYear(vDate);
        std::string vNum = st.at("VoucherNumber");
        std::string tType = st.at("TransType");
        std::string itemCode = st.at("ItemCode");
        std::string itemName = itemMap[itemCode];
        if (itemName.empty()) itemName = "Item " + itemCode;

        auto key = std::make_tuple(vNum, tType, vDate.toStdString());
        std::string partyName = vchInfoMap[key].first;
        if (partyName.empty()) partyName = "Cash";

        std::string billNo = vchInfoMap[key].second;
        if (billNo.empty()) billNo = st.at("DheriBillNo");
        if (billNo.empty()) billNo = vNum;

        long long bags = static_cast<long long>(std::round(std::stod(st.at("Bags").empty() ? "0" : st.at("Bags"))));
        double packing = std::stod(st.at("Packing").empty() ? "0" : st.at("Packing"));
        double weight = std::stod(st.at("Weight").empty() ? "0" : st.at("Weight"));
        double rate = std::stod(st.at("Rate").empty() ? "0" : st.at("Rate"));
        double amount = std::stod(st.at("Amount").empty() ? "0" : st.at("Amount"));
        double taxAmt = std::stod(st.at("TaxableAmount").empty() ? "0" : st.at("TaxableAmount"));
        double tax = std::stod(st.at("Tax").empty() ? "0" : st.at("Tax"));

        db.executeQuery(
            "INSERT INTO stock_transactions (id, financial_year, voucher_no, voucher_date, trans_type, voucher_type, "
            "party_name, bill_no, item_code, item_name, bags, packing, weight_qtl, rate, amount, taxable_amount, tax, tax_type, narration, row_no) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                stIndex++,
                fy,
                QString::fromStdString(vNum),
                vDate,
                QString::fromStdString(tType),
                QString::fromStdString(st.at("VoucherType")),
                QString::fromStdString(partyName),
                QString::fromStdString(billNo),
                QString::fromStdString(itemCode),
                QString::fromStdString(itemName),
                bags,
                packing,
                weight,
                rate,
                amount,
                taxAmt,
                tax,
                QString::fromStdString(st.at("TaxType")),
                QString::fromStdString(st.at("Narration")),
                std::stoi(st.at("RowNo").empty() ? "1" : st.at("RowNo"))
            }
        );
    }

    // 2. Sync Milling Vouchers & Batches
    updateProgress(75, QString("Importing %1 Milling Vouchers...").arg(millingRows.size()));
    db.executeQuery("DELETE FROM milling_voucher_items;");
    db.executeQuery("DELETE FROM milling_batches;");

    std::map<std::pair<std::string, std::string>, std::vector<std::map<std::string, std::string>>> millingBatchMap;
    for (const auto& mv : millingRows) {
        std::string vNum = mv.at("VoucherNumber");
        std::string vDate = parseMdbDate(QString::fromStdString(mv.at("VoucherDate"))).toStdString();
        millingBatchMap[{vNum, vDate}].push_back(mv);
    }

    int batchId = 1;
    int mviId = 1;
    for (const auto& pair : millingBatchMap) {
        std::string vNum = pair.first.first;
        std::string vDate = pair.first.second;
        QString fy = determineFinancialYear(QString::fromStdString(vDate));

        double paddyInput = 0.0;
        double headRice = 0.0;
        double brokenRice = 0.0;
        double bran = 0.0;
        double husk = 0.0;

        for (const auto& r : pair.second) {
            std::string code = r.at("ItemCode");
            std::string name = itemMap[code];
            std::string drcr = r.at("DrCr");
            double wt = std::stod(r.at("Weight").empty() ? "0" : r.at("Weight"));
            double pct = std::stod(r.at("Percentage").empty() ? "0" : r.at("Percentage"));
            int rowNo = std::stoi(r.at("RowNo").empty() ? "1" : r.at("RowNo"));

            if (drcr == "Cr") {
                paddyInput += wt;
            } else {
                if (code == "30" || name.find("Basmati") != std::string::npos || (name.find("Rice") != std::string::npos && name.find("Bran") == std::string::npos && name.find("Nakku") == std::string::npos && name.find("Phak") == std::string::npos)) {
                    headRice += wt;
                } else if (name.find("Nakku") != std::string::npos || code == "94") {
                    brokenRice += wt;
                } else if (name.find("Bran") != std::string::npos || code == "11") {
                    bran += wt;
                } else if (name.find("Phak") != std::string::npos || name.find("Husk") != std::string::npos || code == "95") {
                    husk += wt;
                }
            }

            db.executeQuery(
                "INSERT INTO milling_voucher_items (id, batch_id, batch_no, batch_date, row_no, drcr, item_code, item_name, percentage, weight_qtl, bags, rate, amount) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {
                    mviId++,
                    batchId,
                    QString::fromStdString(vNum),
                    QString::fromStdString(vDate),
                    rowNo,
                    QString::fromStdString(drcr),
                    QString::fromStdString(code),
                    QString::fromStdString(name),
                    pct,
                    wt,
                    0,
                    0.0,
                    0.0
                }
            );
        }

        double yieldPct = (paddyInput > 0) ? (headRice / paddyInput * 100.0) : 0.0;
        double wastage = paddyInput - (headRice + brokenRice + bran + husk);

        db.executeQuery(
            "INSERT INTO milling_batches (id, financial_year, batch_no, batch_date, paddy_variety, paddy_input_qtl, head_rice_qtl, broken_rice_qtl, bran_qtl, husk_qtl, wastage_qtl, yield_pct) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                batchId++,
                fy,
                QString::fromStdString(vNum),
                QString::fromStdString(vDate),
                "Paddy Basmati",
                paddyInput,
                headRice,
                brokenRice,
                bran,
                husk,
                wastage,
                yieldPct
            }
        );
    }

    db.commit();
    mdb_close(mdb);

    updateProgress(100, "Migration Completed Successfully!");
    m_isMigrating = false;
    emit migratingChanged();

    QString summary = QString("Successfully imported %1 Parties, %2 Stock Items, %3 Vouchers, %4 Stock Movements, and %5 Milling Batches from %6")
        .arg(ledgersRows.size())
        .arg(stockItemsRows.size())
        .arg(transactionsRows.size())
        .arg(stockTxRows.size())
        .arg(millingBatchMap.size())
        .arg(fi.fileName());

    emit migrationFinished(true, summary);
    return true;
#else
    emit migrationFinished(false, "libmdb not available in this build.");
    return false;
#endif
}
