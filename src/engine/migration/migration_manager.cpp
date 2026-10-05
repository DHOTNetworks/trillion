#include "migration_manager.h"
#include "group_migrator.h"
#include "ledger_migrator.h"
#include "stock_migrator.h"
#include "voucher_migrator.h"
#include "mandi_migrator.h"
#include "migration_utils.h"
#include "../database_manager.h"
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QXmlStreamReader>
#include <QDebug>

#include "mdbtools.h"
#define HAS_LIBMDB 1

namespace MahadevERP {

static QString normalizePathStr(const QString& rawPath) {
    QString path = rawPath.trimmed();
    if (path.startsWith("file://", Qt::CaseInsensitive)) {
        QUrl url(path);
        if (url.isValid() && !url.toLocalFile().isEmpty()) {
            path = url.toLocalFile();
        } else {
            path = path.mid(7);
        }
    }
    if (path.length() >= 3 && path[0] == '/' && path[1].isLetter() && path[2] == ':') {
        path = path.mid(1);
    }
    return QDir::toNativeSeparators(path);
}

#if HAS_LIBMDB
static std::vector<std::map<std::string, std::string>> readTableRows(MdbHandle* mdb, const char* tableName) {
    std::vector<std::map<std::string, std::string>> result;
    if (!mdb || !tableName) return result;

    MdbTableDef* table = mdb_read_table_by_name(mdb, const_cast<char*>(tableName), MDB_TABLE);
    if (!table) return result;

    mdb_read_columns(table);
    if (!table->columns || table->num_cols == 0) {
        mdb_free_tabledef(table);
        return result;
    }

    unsigned int numCols = table->num_cols;
    std::vector<std::string> colNames(numCols);
    const size_t bufSize = MDB_BIND_SIZE + 512;
    std::vector<std::vector<char>> colBuffers(numCols, std::vector<char>(bufSize, 0));

    for (unsigned int j = 0; j < numCols; j++) {
        MdbColumn* col = static_cast<MdbColumn*>(g_ptr_array_index(table->columns, j));
        if (col && col->name[0] != '\0') {
            colNames[j] = col->name;
            mdb_bind_column(table, j + 1, colBuffers[j].data(), nullptr);
        } else {
            colNames[j] = "";
        }
    }

    mdb_rewind_table(table);
    while (mdb_fetch_row(table)) {
        std::map<std::string, std::string> row;
        for (unsigned int j = 0; j < numCols; j++) {
            if (!colNames[j].empty()) {
                row[colNames[j]] = colBuffers[j].data();
            }
        }
        result.push_back(std::move(row));
    }

    mdb_free_tabledef(table);
    return result;
}
#endif

static std::string getFieldStr(const std::map<std::string, std::string>& m, const std::string& key, const std::string& def = "") {
    auto it = m.find(key);
    if (it != m.end()) return it->second;
    for (const auto& kv : m) {
        if (QString::compare(QString::fromStdString(kv.first), QString::fromStdString(key), Qt::CaseInsensitive) == 0) {
            return kv.second;
        }
    }
    return def;
}

MigrationManager::MigrationManager(QObject* parent) : QObject(parent) {}

MigrationManager& MigrationManager::instance() {
    static MigrationManager s_instance;
    return s_instance;
}

QVariantMap MigrationManager::inspect(MigrationProvider provider, const QString& sourcePath) {
    QVariantMap res;
    QString cleanPath = normalizePathStr(sourcePath);
    QFileInfo fi(cleanPath);
    if (!fi.exists()) {
        res["error"] = "Path does not exist: " + cleanPath;
        return res;
    }

    if (provider == MigrationProvider::BahiKhata || provider == MigrationProvider::Busy) {
#if HAS_LIBMDB
        MdbHandle* mdb = mdb_open(QFile::encodeName(cleanPath).constData(), MDB_NOFLAGS);
        if (!mdb) {
            mdb = mdb_open(cleanPath.toUtf8().constData(), MDB_NOFLAGS);
        }
        if (!mdb) {
            res["error"] = "Failed to open JetDB / BDS database.";
            return res;
        }

        if (provider == MigrationProvider::BahiKhata) {
            auto comp = readTableRows(mdb, "CompanyInfo");
            auto ledgers = readTableRows(mdb, "Ledgers");
            auto items = readTableRows(mdb, "StockItems");
            auto tx = readTableRows(mdb, "Transactions");
            res["company"] = comp.empty() ? "" : QString::fromStdString(getFieldStr(comp.back(), "CompanyName"));
            res["ledgersCount"] = static_cast<int>(ledgers.size());
            res["itemsCount"] = static_cast<int>(items.size());
            res["transactionsCount"] = static_cast<int>(tx.size());
        } else {
            auto accs = readTableRows(mdb, "Master1");
            auto grps = readTableRows(mdb, "Master2");
            auto items = readTableRows(mdb, "Master3");
            auto tx = readTableRows(mdb, "Tran1");
            res["ledgersCount"] = static_cast<int>(accs.size());
            res["groupsCount"] = static_cast<int>(grps.size());
            res["itemsCount"] = static_cast<int>(items.size());
            res["vouchersCount"] = static_cast<int>(tx.size());
        }
        mdb_close(mdb);
#endif
    }
    return res;
}

bool MigrationManager::runMigrationPipeline(MigrationContext& ctx) {
    switch (ctx.provider) {
        case MigrationProvider::BahiKhata:
            return runBahiKhataPipeline(ctx);
        case MigrationProvider::Busy:
            return runBusyPipeline(ctx);
        case MigrationProvider::Tally:
            return runTallyPipeline(ctx);
        default:
            return false;
    }
}

bool MigrationManager::runBahiKhataPipeline(MigrationContext& ctx) {
    QString cleanPath = normalizePathStr(ctx.sourcePath);
    QFileInfo fi(cleanPath);
    if (!fi.exists()) {
        ctx.updateProgress(0, "MDB file does not exist.");
        emit migrationFinished(false, "MDB file does not exist.");
        return false;
    }

#if HAS_LIBMDB
    ctx.updateProgress(5, "Opening Bahi Khata JetDB: " + fi.fileName());
    MdbHandle* mdb = mdb_open(QFile::encodeName(cleanPath).constData(), MDB_NOFLAGS);
    if (!mdb) {
        mdb = mdb_open(cleanPath.toUtf8().constData(), MDB_NOFLAGS);
    }
    if (!mdb) {
        ctx.updateProgress(0, "Failed to open MDB database.");
        emit migrationFinished(false, "Failed to open MDB database.");
        return false;
    }

    ctx.updateProgress(10, "Extracting Raw JetDB Tables...");
    auto compRows = readTableRows(mdb, "CompanyInfo");
    auto groupRows = readTableRows(mdb, "Groups");
    auto stockGroupRows = readTableRows(mdb, "StockGroups");
    auto stockUnitRows = readTableRows(mdb, "StockUnits");
    auto ledgerRows = readTableRows(mdb, "Ledgers");
    auto itemRows = readTableRows(mdb, "StockItems");
    auto stockTransRows = readTableRows(mdb, "StockTransactions");
    auto transportRows = readTableRows(mdb, "SaleTransportationDetail");
    auto transRows = readTableRows(mdb, "Transactions");
    auto millingRows = readTableRows(mdb, "MillingVouchers");
    auto customRows = readTableRows(mdb, "CustomClosingStocks");
    auto tdsRows = readTableRows(mdb, "TDSDeductions");
    auto bardanaRows = readTableRows(mdb, "BardanaTransactions");
    auto gatePassRows = readTableRows(mdb, "GatePassVouchers");
    auto gateRegRows = readTableRows(mdb, "GateRegister");
    auto saudaVchRows = readTableRows(mdb, "SaudaVouchers");
    auto saudaTxRows = readTableRows(mdb, "SaudaTransactions");
    auto brokerageRows = readTableRows(mdb, "BrokerageVouchers");

    mdb_close(mdb);

    auto& db = DatabaseManager::instance();
    db.ensureTablesExist();
    db.executeNonQuery("PRAGMA foreign_keys = OFF;");
    db.beginTransaction();

    db.executeNonQuery("DELETE FROM bardana_transactions;");
    db.executeNonQuery("DELETE FROM gate_register;");
    db.executeNonQuery("DELETE FROM sauda_contracts;");
    db.executeNonQuery("DELETE FROM dalali_settlements;");
    db.executeNonQuery("DELETE FROM transport_dispatches;");
    db.executeNonQuery("DELETE FROM debit_credit_notes;");
    db.executeNonQuery("DELETE FROM transactions;");
    db.executeNonQuery("DELETE FROM sales_invoices;");
    db.executeNonQuery("DELETE FROM purchase_invoices;");
    db.executeNonQuery("DELETE FROM paddy_arrivals;");
    db.executeNonQuery("DELETE FROM milling_voucher_items;");
    db.executeNonQuery("DELETE FROM milling_batches;");
    db.executeNonQuery("DELETE FROM stock_transactions;");
    db.executeNonQuery("DELETE FROM jform_vouchers;");
    db.executeNonQuery("DELETE FROM jform_voucher_items;");
    db.executeNonQuery("DELETE FROM tds_vouchers;");
    db.executeNonQuery("DELETE FROM sales_invoice_items;");
    db.executeNonQuery("DELETE FROM purchase_invoice_items;");
    db.executeNonQuery("DELETE FROM custom_closing_stocks;");
    db.executeNonQuery("DELETE FROM vouchers;");
    db.executeNonQuery("DELETE FROM inventory;");
    db.executeNonQuery("DELETE FROM stock_items;");
    db.executeNonQuery("DELETE FROM stock_groups;");
    db.executeNonQuery("DELETE FROM stock_units;");
    db.executeNonQuery("DELETE FROM parties;");
    db.executeNonQuery("DELETE FROM account_groups;");
    db.executeNonQuery("DELETE FROM financial_years;");
    db.executeNonQuery("DELETE FROM company_info;");

    // Company info & FY
    ctx.updateProgress(15, "Migrating Company Info & Financial Years...");
    int booksStartYear = 0;
    QString compName = "Mahadev Modern Rice Mills";
    QString gstin, pan, firmType, address, state, stateCode, pincode, phone, mobile;

    if (!compRows.empty()) {
        const auto& c = compRows.back();
        compName = QString::fromStdString(getFieldStr(c, "CompanyName"));
        gstin = QString::fromStdString(getFieldStr(c, "GSTIN"));
        pan = QString::fromStdString(getFieldStr(c, "PAN_No"));
        if (pan.isEmpty() && gstin.length() == 15) pan = gstin.mid(2, 10);
        firmType = MigrationUtils::resolveFirmTypeFromPan(QString::fromStdString(getFieldStr(c, "FirmType")), pan, compName);
        address = QString::fromStdString(getFieldStr(c, "Address"));
        state = QString::fromStdString(getFieldStr(c, "MySTATE", "Haryana"));
        stateCode = MigrationUtils::extractStateCode(gstin, state);
        pincode = MigrationUtils::extractPincode(address);
        phone = QString::fromStdString(getFieldStr(c, "Phone_O"));
        mobile = QString::fromStdString(getFieldStr(c, "Phone_F", getFieldStr(c, "Mobile1")));

        QString booksFrom = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(c, "BooksBeginingFrom")));
        if (booksFrom.isEmpty()) booksFrom = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(c, "AccYearFrom")));
        if (booksFrom.length() >= 4) booksStartYear = booksFrom.left(4).toInt();

        QString bankName, bankAcc, ifsc;
        MigrationUtils::cleanBankingDetails(
            QString::fromStdString(getFieldStr(c, "Mobile2")),
            QString::fromStdString(getFieldStr(c, "Bank2")),
            QString::fromStdString(getFieldStr(c, "Bank3")),
            bankName, bankAcc, ifsc
        );

        db.executeNonQuery(
            "INSERT INTO company_info ("
            "company_name, firm_type, address, state, state_code, pincode, phone, mobile, "
            "gstin, pan_no, bank_name, bank_account, ifsc_code, books_from"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {compName, firmType, address, state, stateCode, pincode, phone, mobile, gstin, pan, bankName, bankAcc, ifsc, booksFrom}
        );
    }

    ctx.stats.companyName = compName;
    ctx.stats.gstin = gstin;
    ctx.stats.pan = pan;
    ctx.stats.firmType = firmType;

    // Financial years
    QMap<QString, int> fyNameToId;
    int baseStart = (booksStartYear > 0) ? booksStartYear : 2024;
    for (int y = 2000; y <= 2035; ++y) {
        int nextY = (y + 1) % 100;
        QString yName = QString("%1-%2").arg(y).arg(nextY, 2, 10, QChar('0'));
        QString sDate = QString("%1-04-01").arg(y);
        QString eDate = QString("%1-03-31").arg(y + 1);
        int isAct = (y == baseStart) ? 1 : 0;
        db.executeNonQuery(
            "INSERT INTO financial_years (year_name, start_date, end_date, is_active, is_locked) "
            "VALUES (?, ?, ?, ?, 0);",
            {yName, sDate, eDate, isAct}
        );
        fyNameToId[yName] = static_cast<int>(db.lastInsertedId());
    }

    // 1. Groups
    std::map<int, std::string> groupCodeMap;
    std::map<int, qint64> groupCodeToIdMap;
    GroupMigrator::migrate_groups_bahikhata(ctx, groupRows, groupCodeMap, groupCodeToIdMap);

    // 2. Ledgers
    std::map<int, std::string> ledgerCodeToName;
    std::map<int, qint64> ledgerCodeToId;
    LedgerMigrator::migrate_ledgers_bahikhata(ctx, ledgerRows, groupCodeMap, groupCodeToIdMap, ledgerCodeToName, ledgerCodeToId);

    // 3. Stocks
    std::map<int, std::string> stockGroupMap;
    std::map<int, std::string> stockUnitMap;
    std::map<int, std::string> itemCodeToName;
    std::map<int, qint64> itemCodeToId;
    StockMigrator::migrate_stocks_bahikhata(ctx, stockGroupRows, stockUnitRows, itemRows, stockGroupMap, stockUnitMap, itemCodeToName, itemCodeToId);

    // 4. Vouchers & Double Entry
    VoucherMigrator::migrate_vouchers_bahikhata(ctx, transRows, stockTransRows, transportRows, ledgerCodeToName, ledgerCodeToId, itemCodeToName, itemCodeToId, stockUnitMap, fyNameToId);

    // 5. Mandi & Agri Operations
    MandiMigrator::migrate_mandi_bahikhata(ctx, transRows, millingRows, customRows, tdsRows, bardanaRows, gatePassRows, gateRegRows, saudaVchRows, saudaTxRows, brokerageRows, ledgerCodeToName, ledgerCodeToId, itemCodeToName, itemCodeToId, stockUnitMap, fyNameToId);

    db.commit();
    db.executeNonQuery("PRAGMA foreign_keys = ON;");

    ctx.updateProgress(100, "Bahi-Khata Migration Completed Successfully!");
    emit migrationFinished(true, "Bahi-Khata Migration Completed Successfully!");
    return true;
#else
    return false;
#endif
}

bool MigrationManager::runBusyPipeline(MigrationContext& ctx) {
    QString cleanPath = normalizePathStr(ctx.sourcePath);
    QFileInfo fi(cleanPath);
    if (!fi.exists()) {
        ctx.updateProgress(0, "Busy path does not exist.");
        emit migrationFinished(false, "Busy path does not exist.");
        return false;
    }

#if HAS_LIBMDB
    QString dbFile = cleanPath;
    if (fi.isDir()) {
        QDir dir(cleanPath);
        QStringList entries = dir.entryList({"*.bds", "*.mdb", "db.bds", "Data.*"}, QDir::Files);
        if (!entries.isEmpty()) {
            dbFile = dir.filePath(entries.first());
        }
    }

    ctx.updateProgress(5, "Opening Busy Database: " + QFileInfo(dbFile).fileName());
    MdbHandle* mdb = mdb_open(QFile::encodeName(dbFile).constData(), MDB_NOFLAGS);
    if (!mdb) {
        mdb = mdb_open(dbFile.toUtf8().constData(), MDB_NOFLAGS);
    }
    if (!mdb) {
        ctx.updateProgress(0, "Failed to open Busy database.");
        emit migrationFinished(false, "Failed to open Busy database.");
        return false;
    }

    auto accRows = readTableRows(mdb, "Master1");
    auto grpRows = readTableRows(mdb, "Master2");
    auto itemRows = readTableRows(mdb, "Master3");
    auto unitRows = readTableRows(mdb, "Master4");
    auto tran1Rows = readTableRows(mdb, "Tran1");
    auto tran2Rows = readTableRows(mdb, "Tran2");
    auto tran3Rows = readTableRows(mdb, "Tran3");
    auto mandiRows = readTableRows(mdb, "MandiForms");
    mdb_close(mdb);

    auto& db = DatabaseManager::instance();
    db.ensureTablesExist();
    db.executeNonQuery("PRAGMA foreign_keys = OFF;");
    db.beginTransaction();

    db.executeNonQuery("DELETE FROM transactions;");
    db.executeNonQuery("DELETE FROM sales_invoices;");
    db.executeNonQuery("DELETE FROM purchase_invoices;");
    db.executeNonQuery("DELETE FROM vouchers;");
    db.executeNonQuery("DELETE FROM inventory;");
    db.executeNonQuery("DELETE FROM stock_items;");
    db.executeNonQuery("DELETE FROM stock_groups;");
    db.executeNonQuery("DELETE FROM stock_units;");
    db.executeNonQuery("DELETE FROM parties;");
    db.executeNonQuery("DELETE FROM account_groups;");
    db.executeNonQuery("DELETE FROM financial_years;");
    db.executeNonQuery("DELETE FROM company_info;");

    // Company & FY
    ctx.updateProgress(15, "Migrating Busy Profile & Financial Years...");
    db.executeNonQuery(
        "INSERT INTO company_info (company_name, state, state_code) "
        "VALUES ('Busy Enterprise Firm', 'Haryana', '06');"
    );

    QMap<QString, int> fyNameToId;
    for (int y = 2000; y <= 2035; ++y) {
        int nextY = (y + 1) % 100;
        QString yName = QString("%1-%2").arg(y).arg(nextY, 2, 10, QChar('0'));
        QString sDate = QString("%1-04-01").arg(y);
        QString eDate = QString("%1-03-31").arg(y + 1);
        int isAct = (y == 2024) ? 1 : 0;
        db.executeNonQuery(
            "INSERT INTO financial_years (year_name, start_date, end_date, is_active, is_locked) "
            "VALUES (?, ?, ?, ?, 0);",
            {yName, sDate, eDate, isAct}
        );
        fyNameToId[yName] = static_cast<int>(db.lastInsertedId());
    }

    std::map<int, std::string> groupCodeMap;
    std::map<int, qint64> groupCodeToIdMap;
    GroupMigrator::migrate_groups_busy(ctx, grpRows, groupCodeMap, groupCodeToIdMap);

    std::map<int, std::string> accountCodeToName;
    std::map<int, qint64> accountCodeToId;
    LedgerMigrator::migrate_ledgers_busy(ctx, accRows, groupCodeMap, groupCodeToIdMap, accountCodeToName, accountCodeToId);

    std::map<int, std::string> itemCodeToName;
    std::map<int, qint64> itemCodeToId;
    StockMigrator::migrate_stocks_busy(ctx, itemRows, unitRows, grpRows, itemCodeToName, itemCodeToId);

    VoucherMigrator::migrate_vouchers_busy(ctx, tran1Rows, tran2Rows, tran3Rows, accountCodeToName, accountCodeToId, itemCodeToName, itemCodeToId, fyNameToId);

    MandiMigrator::migrate_mandi_busy(ctx, mandiRows, accountCodeToName, accountCodeToId, fyNameToId);

    db.commit();
    db.executeNonQuery("PRAGMA foreign_keys = ON;");

    ctx.updateProgress(100, "Busy Migration Completed Successfully!");
    emit migrationFinished(true, "Busy Migration Completed Successfully!");
    return true;
#else
    return false;
#endif
}

bool MigrationManager::runTallyPipeline(MigrationContext& ctx) {
    QString cleanPath = normalizePathStr(ctx.sourcePath);
    QFileInfo fi(cleanPath);
    if (!fi.exists()) {
        ctx.updateProgress(0, "Tally path does not exist.");
        emit migrationFinished(false, "Tally path does not exist.");
        return false;
    }

    ctx.updateProgress(10, "Parsing Tally XML...");
    QFile file(cleanPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit migrationFinished(false, "Failed to open Tally XML file.");
        return false;
    }

    QXmlStreamReader xml(&file);
    QList<QString> groupNames;
    QMap<QString, QString> parentMap;
    QList<QVariantMap> ledgerRecords;
    QList<QVariantMap> unitRecords;
    QList<QVariantMap> itemRecords;
    QList<QVariantMap> voucherRecords;

    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();
        if (xml.isStartElement()) {
            QString tag = xml.name().toString().toUpper();
            if (tag == "GROUP") {
                QString gName = xml.attributes().value("NAME").toString();
                groupNames.append(gName);
            } else if (tag == "LEDGER") {
                QVariantMap l;
                l["name"] = xml.attributes().value("NAME").toString();
                ledgerRecords.append(l);
            } else if (tag == "UNIT") {
                QVariantMap u;
                u["name"] = xml.attributes().value("NAME").toString();
                unitRecords.append(u);
            } else if (tag == "STOCKITEM") {
                QVariantMap i;
                i["name"] = xml.attributes().value("NAME").toString();
                itemRecords.append(i);
            } else if (tag == "VOUCHER") {
                QVariantMap v;
                v["voucher_type"] = xml.attributes().value("VCHTYPE").toString();
                voucherRecords.append(v);
            }
        }
    }
    file.close();

    auto& db = DatabaseManager::instance();
    db.ensureTablesExist();
    db.executeNonQuery("PRAGMA foreign_keys = OFF;");
    db.beginTransaction();

    db.executeNonQuery("DELETE FROM transactions;");
    db.executeNonQuery("DELETE FROM sales_invoices;");
    db.executeNonQuery("DELETE FROM purchase_invoices;");
    db.executeNonQuery("DELETE FROM vouchers;");
    db.executeNonQuery("DELETE FROM inventory;");
    db.executeNonQuery("DELETE FROM stock_items;");
    db.executeNonQuery("DELETE FROM stock_groups;");
    db.executeNonQuery("DELETE FROM stock_units;");
    db.executeNonQuery("DELETE FROM parties;");
    db.executeNonQuery("DELETE FROM account_groups;");
    db.executeNonQuery("DELETE FROM financial_years;");
    db.executeNonQuery("DELETE FROM company_info;");

    db.executeNonQuery(
        "INSERT INTO company_info (company_name, state, state_code) "
        "VALUES ('Tally Imported Enterprise', 'Haryana', '06');"
    );

    QMap<QString, int> fyNameToId;
    for (int y = 2000; y <= 2035; ++y) {
        int nextY = (y + 1) % 100;
        QString yName = QString("%1-%2").arg(y).arg(nextY, 2, 10, QChar('0'));
        QString sDate = QString("%1-04-01").arg(y);
        QString eDate = QString("%1-03-31").arg(y + 1);
        int isAct = (y == 2024) ? 1 : 0;
        db.executeNonQuery(
            "INSERT INTO financial_years (year_name, start_date, end_date, is_active, is_locked) "
            "VALUES (?, ?, ?, ?, 0);",
            {yName, sDate, eDate, isAct}
        );
        fyNameToId[yName] = static_cast<int>(db.lastInsertedId());
    }

    std::map<std::string, qint64> groupNameToIdMap;
    GroupMigrator::migrate_groups_tally(ctx, parentMap, groupNames, groupNameToIdMap);

    std::map<std::string, qint64> ledgerNameToId;
    LedgerMigrator::migrate_ledgers_tally(ctx, ledgerRecords, groupNameToIdMap, ledgerNameToId);

    std::map<std::string, qint64> itemNameToId;
    StockMigrator::migrate_stocks_tally(ctx, unitRecords, itemRecords, itemNameToId);

    VoucherMigrator::migrate_vouchers_tally(ctx, voucherRecords, ledgerNameToId, itemNameToId, fyNameToId);

    db.commit();
    db.executeNonQuery("PRAGMA foreign_keys = ON;");

    ctx.updateProgress(100, "Tally Migration Completed Successfully!");
    emit migrationFinished(true, "Tally Migration Completed Successfully!");
    return true;
}

} // namespace MahadevERP
