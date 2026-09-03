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
        groupMap[g.at("Code1st")] = g.at("GroupName");
    }

    std::map<std::string, std::string> ledgerMap; // code -> name
    std::map<std::string, std::string> ledgerGroupMap; // code -> group
    for (const auto& l : ledgersRows) {
        std::string code = l.at("Code1st");
        std::string name = l.at("LedgerName");
        std::string grpCode = l.at("GroupCode");
        ledgerMap[code] = name;
        ledgerGroupMap[code] = groupMap[grpCode];
    }

    std::map<std::string, std::string> itemMap; // code -> name
    for (const auto& item : stockItemsRows) {
        itemMap[item.at("Code1st")] = item.at("ItemName");
    }

    // Build Voucher -> (Party, InvoiceNo) map
    std::map<std::tuple<std::string, std::string, std::string>, std::pair<std::string, std::string>> vchInfoMap;
    for (const auto& t : transactionsRows) {
        std::string vNum = t.at("VoucherNumber");
        std::string tType = t.at("TransType");
        std::string vDate = parseMdbDate(QString::fromStdString(t.at("VoucherDate"))).toStdString();
        std::string invNo = t.at("InvoiceNo");
        std::string acc = t.at("AccountCode");
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

    // 1. Sync Stock Transactions
    updateProgress(50, QString("Importing %1 Stock Transactions...").arg(stockTxRows.size()));
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

    QString summary = QString("Successfully imported %1 Stock Transactions and %2 Milling Batches from %3")
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
