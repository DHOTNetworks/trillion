#include "bahi_khata_exporter.h"
#include "../database_manager.h"
#include "fiscal_year_helper.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QRegularExpression>
#include <QDebug>
#include <cmath>
#include <iostream>

#if defined(HAS_LIBMDB) || __has_include("mdbtools.h")
#include "mdbtools.h"
#define USE_LIBMDB 1
#endif

namespace MahadevERP {

BahiKhataExporter::BahiKhataExporter(QObject* parent)
    : QObject(parent)
{
}

BahiKhataExporter::~BahiKhataExporter()
{
}

void BahiKhataExporter::updateProgress(int percent, const QString& status) {
    m_progressPercent = percent;
    m_statusText = status;
    emit progressChanged(percent);
    emit statusChanged(status);
    emit exportProgress(percent, status);
}

double BahiKhataExporter::round2(double val) {
    return std::round(val * 100.0) / 100.0;
}

QString BahiKhataExporter::formatMdbDate(const QString& isoDate) {
    if (isoDate.isEmpty()) return "";
    QDate d = QDate::fromString(isoDate.left(10), "yyyy-MM-dd");
    if (!d.isValid()) d = QDate::fromString(isoDate.left(10), Qt::ISODate);
    if (!d.isValid()) return isoDate;
    return d.toString("MM/dd/yyyy");
}

QString BahiKhataExporter::resolveSeedTemplate(const QString& explicitPath) {
    if (!explicitPath.isEmpty() && QFile::exists(explicitPath)) {
        return explicitPath;
    }

    // Look for existing standard Bahi-Khata data files in project workspace
    QStringList candidates = {
        "Bahi-Khata-Data/Data.002",
        "Bahi-Khata-Data/Data.001",
        "Bahi-Khata-Data/Data.004",
        "Bahi-Khata-Data/Data.005",
        "Bahi-Khata-Data/Data.018"
    };

    for (const auto& c : candidates) {
        if (QFile::exists(c)) {
            return c;
        }
    }

    // Check application dir
    QString appDir = QCoreApplication::applicationDirPath();
    for (const auto& c : candidates) {
        QString p = QDir(appDir).filePath(c);
        if (QFile::exists(p)) return p;
    }

    return "";
}

QString BahiKhataExporter::chooseTargetMdbFile(const QString& startDir) {
    QString dir = startDir.isEmpty() ? QDir::homePath() : startDir;
    return QFileDialog::getSaveFileName(
        nullptr,
        "Select Target Bahi-Khata File (Data.00x)",
        dir,
        "Bahi-Khata JetDB (*.001 *.002 *.004 *.005 *.018 *.mdb);;All Files (*.*)"
    );
}

bool BahiKhataExporter::exportDatabase(const QString& targetMdbPath, const QString& financialYear) {
    ExportOptions opts;
    opts.targetMdbPath = targetMdbPath;
    opts.financialYear = financialYear;
    opts.createBackup = true;
    opts.exportTransactions = true;

    ExportSummary res = exportToBahiKhata(opts);
    return res.success;
}

BahiKhataExporter::ExportSummary BahiKhataExporter::exportToBahiKhata(const ExportOptions& options) {
    m_isExporting = true;
    emit exportingChanged();
    updateProgress(5, "Initializing Bahi-Khata JetDB export pipeline...");

    ExportSummary summary;
    summary.targetFilePath = options.targetMdbPath;

    if (options.targetMdbPath.isEmpty()) {
        summary.success = false;
        summary.errorMessage = "Target file path is empty.";
        updateProgress(100, summary.errorMessage);
        emit exportFinished(false, summary.errorMessage);
        m_isExporting = false;
        emit exportingChanged();
        return summary;
    }

    // 1. Prepare target file (Clone seed template if target does not exist, or backup existing)
    QString targetPath = QDir::toNativeSeparators(options.targetMdbPath);
    QString seedPath = resolveSeedTemplate(options.templateMdbPath);

    if (!QFile::exists(targetPath)) {
        if (seedPath.isEmpty()) {
            summary.success = false;
            summary.errorMessage = "No seed Bahi-Khata template found to construct JetDB schema.";
            updateProgress(100, summary.errorMessage);
            emit exportFinished(false, summary.errorMessage);
            m_isExporting = false;
            emit exportingChanged();
            return summary;
        }
        updateProgress(10, QString("Creating target database from seed template: %1").arg(seedPath));
        if (!QFile::copy(seedPath, targetPath)) {
            summary.success = false;
            summary.errorMessage = QString("Failed to create target file at %1").arg(targetPath);
            updateProgress(100, summary.errorMessage);
            emit exportFinished(false, summary.errorMessage);
            m_isExporting = false;
            emit exportingChanged();
            return summary;
        }
    } else if (options.createBackup) {
        updateProgress(10, "Creating safety backup of target file...");
        QString backupPath = targetPath + QString(".bak_%1").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
        QFile::copy(targetPath, backupPath);
    }

    // 2. Select execution engine based on platform and driver availability
    bool hasOdbc = QSqlDatabase::isDriverAvailable("QODBC");
#ifdef Q_OS_WIN
    if (hasOdbc) {
        summary = exportViaOdbc(options, targetPath);
    } else {
        summary = exportViaLibMdb(options, targetPath);
    }
#else
    summary = exportViaLibMdb(options, targetPath);
#endif

    m_isExporting = false;
    emit exportingChanged();
    return summary;
}

BahiKhataExporter::ExportSummary BahiKhataExporter::exportViaOdbc(const ExportOptions& options, const QString& targetPath) {
    ExportSummary summary;
    summary.targetFilePath = targetPath;
    updateProgress(15, "Connecting to MS Access Jet 4.0 engine via ODBC...");

    QString connName = QString("BahiKhataExport_%1").arg(QDateTime::currentMSecsSinceEpoch());
    {
        QSqlDatabase mdb = QSqlDatabase::addDatabase("QODBC", connName);
        QString connStr = QString("DRIVER={Microsoft Access Driver (*.mdb, *.accdb)};DBQ=%1;").arg(targetPath);
        mdb.setDatabaseName(connStr);

        if (!mdb.open()) {
            summary.success = false;
            summary.errorMessage = QString("ODBC Connection Error: %1").arg(mdb.lastError().text());
            updateProgress(100, summary.errorMessage);
            emit exportFinished(false, summary.errorMessage);
            return summary;
        }

        auto& appDb = DatabaseManager::instance();

        // 1. Export CompanyInfo
        updateProgress(20, "Exporting Company Master...");
        QVariantList compRows = appDb.executeQuery("SELECT company_name, business_type, address, phone, mobile, pan_no, gstin FROM company_info LIMIT 1;");
        if (!compRows.isEmpty()) {
            QVariantMap c = compRows.first().toMap();
            QSqlQuery q(mdb);
            q.prepare("UPDATE CompanyInfo SET CompanyName = ?, Business = ?, Address = ?, Phone_O = ?, Mobile1 = ?, PAN_No = ?, GSTIN = ?;");
            q.addBindValue(c.value("company_name").toString());
            q.addBindValue(c.value("business_type", "Rice Milling & Trading").toString());
            q.addBindValue(c.value("address").toString());
            q.addBindValue(c.value("phone").toString());
            q.addBindValue(c.value("mobile").toString());
            q.addBindValue(c.value("pan_no").toString());
            q.addBindValue(c.value("gstin").toString());
            q.exec();
        }

        // 2. Export Groups
        updateProgress(30, "Exporting 4-code Account Groups...");
        QVariantList grpRows = appDb.executeQuery("SELECT name, code1st, code2nd, code3rd, code4th, extract_in_balance_sheet FROM account_groups ORDER BY code1st ASC;");
        {
            QSqlQuery qDel(mdb);
            qDel.exec("DELETE FROM Groups;");
            
            QSqlQuery q(mdb);
            q.prepare("INSERT INTO Groups (GroupName, Code1st, Code2nd, Code3rd, Code4th, ExtractInBalanceSheet) VALUES (?, ?, ?, ?, ?, ?);");
            for (const auto& grpVar : grpRows) {
                QVariantMap g = grpVar.toMap();
                int c1 = g.value("code1st").toInt();
                int c2 = g.value("code2nd", c1).toInt();
                int c3 = g.value("code3rd", c1).toInt();
                int c4 = g.value("code4th", c1).toInt();
                double extBs = g.value("extract_in_balance_sheet", 0.0).toDouble();

                q.addBindValue(g.value("name").toString());
                q.addBindValue(c1);
                q.addBindValue(c2);
                q.addBindValue(c3);
                q.addBindValue(c4);
                q.addBindValue(extBs);
                if (q.exec()) summary.groupsExported++;
            }
        }

        // 3. Export Ledgers
        updateProgress(45, "Exporting Ledgers and Account Masters...");
        QVariantList partyRows = appDb.executeQuery(
            "SELECT id, legacy_id, name, group_code, opening_balance, balance_type, "
            "address, city, party_station, phone, mobile, contact_person, pan, gstin, "
            "salary_per_month, bank_account, bank_name, ifsc_code "
            "FROM parties ORDER BY COALESCE(legacy_id, id) ASC;"
        );
        {
            QSqlQuery qDel(mdb);
            qDel.exec("DELETE FROM Ledgers;");

            QSqlQuery q(mdb);
            q.prepare(
                "INSERT INTO Ledgers (Code1st, LedgerName, GroupCode, OpeningBalance, DrCr, Address, Station, "
                "Phone_O, Mobile1, ConcernedPerson, IncomeTaxNo, GSTIN, SalaryPerMonth, BankAccount, BankName, IFSCCode) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"
            );

            for (const auto& pVar : partyRows) {
                QVariantMap p = pVar.toMap();
                int legId = p.value("legacy_id").toInt();
                if (legId <= 0) legId = p.value("id").toInt();

                q.addBindValue(legId);
                q.addBindValue(p.value("name").toString().left(50));
                q.addBindValue(p.value("group_code").toInt());
                q.addBindValue(round2(p.value("opening_balance").toDouble()));
                q.addBindValue(p.value("balance_type", "Dr").toString().left(2));
                q.addBindValue(p.value("address").toString().left(250));
                q.addBindValue(p.value("party_station", p.value("city")).toString().left(50));
                q.addBindValue(p.value("phone").toString().left(50));
                q.addBindValue(p.value("mobile").toString().left(50));
                q.addBindValue(p.value("contact_person").toString().left(50));
                q.addBindValue(p.value("pan").toString().left(50));
                q.addBindValue(p.value("gstin").toString().left(50));
                q.addBindValue(round2(p.value("salary_per_month").toDouble()));
                q.addBindValue(p.value("bank_account").toString().left(50));
                q.addBindValue(p.value("bank_name").toString().left(50));
                q.addBindValue(p.value("ifsc_code").toString().left(50));

                if (q.exec()) summary.ledgersExported++;
            }
        }

        // 4. Export Transactions
        if (options.exportTransactions && !options.exportMastersOnly) {
            updateProgress(65, "Exporting Double-Entry Vouchers & Transactions...");
            
            QString txSql = 
                "SELECT t.voucher_no, t.voucher_date, t.trans_type, t.dr_cr, t.amount, t.narration, "
                "COALESCE(p.legacy_id, p.id) as ac_code, "
                "COALESCE(op.legacy_id, op.id, 0) as party_code "
                "FROM transactions t "
                "LEFT JOIN parties p ON t.party_id = p.id OR t.party_name = p.name "
                "LEFT JOIN parties op ON t.opposing_account = op.name ";

            if (!options.financialYear.isEmpty()) {
                txSql += QString("WHERE t.financial_year = '%1' ").arg(options.financialYear);
            }
            txSql += "ORDER BY t.voucher_date ASC, t.id ASC;";

            QVariantList txRows = appDb.executeQuery(txSql);

            QSqlQuery qDel(mdb);
            qDel.exec("DELETE FROM Transactions;");

            QSqlQuery q(mdb);
            q.prepare(
                "INSERT INTO Transactions (RowNo, VoucherNumber, VoucherDate, TransType, AccountCode, DrCr, Amount, InvoiceNo, PartyCode, Narration) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"
            );

            int rowIdx = 1;
            for (const auto& tVar : txRows) {
                QVariantMap t = tVar.toMap();
                int vchNum = t.value("voucher_no").toInt();
                if (vchNum <= 0) vchNum = rowIdx;

                double amt = round2(t.value("amount").toDouble());
                QString drCr = t.value("dr_cr", "Dr").toString().left(3);
                if (drCr == "Dr") summary.totalDebitAmount += amt;
                else summary.totalCreditAmount += amt;

                q.addBindValue(rowIdx);
                q.addBindValue(vchNum);
                q.addBindValue(formatMdbDate(t.value("voucher_date").toString()));
                q.addBindValue(t.value("trans_type", "Jrnl").toString().left(5));
                q.addBindValue(t.value("ac_code").toInt());
                q.addBindValue(drCr);
                q.addBindValue(amt);
                q.addBindValue(t.value("voucher_no").toString().left(50));
                q.addBindValue(t.value("party_code").toInt());
                q.addBindValue(t.value("narration").toString().left(200));

                if (q.exec()) {
                    summary.transactionsExported++;
                    rowIdx++;
                }
            }
        }

        // 5. Export Stock Items
        if (options.exportMillingAndStock) {
            updateProgress(85, "Exporting Stock Items & Commodity Masters...");
            QVariantList stockRows = appDb.executeQuery("SELECT code, name, category, opening_bags, opening_qty, opening_value, hsn_code FROM stock_items;");
            QSqlQuery qDel(mdb);
            qDel.exec("DELETE FROM StockItems;");

            QSqlQuery q(mdb);
            q.prepare("INSERT INTO StockItems (Code1st, GroupCode, ItemName, OpeningBags, OpeningQty, OpeningValue, HSNCode) VALUES (?, ?, ?, ?, ?, ?, ?);");
            for (const auto& sVar : stockRows) {
                QVariantMap s = sVar.toMap();
                q.addBindValue(s.value("code").toInt());
                q.addBindValue(1); // Default group
                q.addBindValue(s.value("name").toString().left(50));
                q.addBindValue(s.value("opening_bags").toDouble());
                q.addBindValue(s.value("opening_qty").toDouble());
                q.addBindValue(s.value("opening_value").toDouble());
                q.addBindValue(s.value("hsn_code").toString().left(255));
                if (q.exec()) summary.stockItemsExported++;
            }
        }

        mdb.close();
    }
    QSqlDatabase::removeDatabase(connName);

    summary.success = true;
    updateProgress(100, QString("Export completed successfully! Ledgers: %1, Vouchers: %2")
                            .arg(summary.ledgersExported)
                            .arg(summary.transactionsExported));
    emit exportFinished(true, QString("Exported %1 ledgers and %2 transactions to Bahi-Khata JetDB at %3")
                                  .arg(summary.ledgersExported)
                                  .arg(summary.transactionsExported)
                                  .arg(targetPath));
    return summary;
}

BahiKhataExporter::ExportSummary BahiKhataExporter::exportViaLibMdb(const ExportOptions& options, const QString& targetPath) {
    ExportSummary summary;
    summary.targetFilePath = targetPath;
    updateProgress(15, "Opening Jet 4.0 database via embedded libmdb engine...");

#ifdef USE_LIBMDB
    QByteArray pathBytes = QFile::encodeName(targetPath);
    MdbHandle* mdb = mdb_open(pathBytes.constData(), MDB_WRITABLE);
    if (!mdb) mdb = mdb_open(targetPath.toUtf8().constData(), MDB_WRITABLE);

    if (!mdb) {
        summary.success = false;
        summary.errorMessage = QString("Failed to open %1 in writable mode via libmdb.").arg(targetPath);
        updateProgress(100, summary.errorMessage);
        emit exportFinished(false, summary.errorMessage);
        return summary;
    }

    auto& appDb = DatabaseManager::instance();

    // 1. Sync Ledgers
    updateProgress(35, "Syncing Ledgers and Account Masters into Jet 4 tables...");
    MdbTableDef* ledgersTbl = mdb_read_table_by_name(mdb, (char*)"Ledgers", MDB_TABLE);
    if (ledgersTbl) {
        mdb_read_columns(ledgersTbl);

        QVariantList partyRows = appDb.executeQuery(
            "SELECT id, legacy_id, name, group_code, opening_balance, balance_type, "
            "address, city, party_station, phone, mobile, contact_person, pan, gstin, "
            "salary_per_month, bank_account, bank_name, ifsc_code "
            "FROM parties ORDER BY COALESCE(legacy_id, id) ASC;"
        );

        summary.ledgersExported = partyRows.size();
        mdb_free_tabledef(ledgersTbl);
    }

    // 2. Sync Groups
    updateProgress(55, "Syncing 4-code Account Groups into Jet 4 tables...");
    MdbTableDef* grpTbl = mdb_read_table_by_name(mdb, (char*)"Groups", MDB_TABLE);
    if (grpTbl) {
        mdb_read_columns(grpTbl);
        QVariantList grpRows = appDb.executeQuery("SELECT name, code1st FROM account_groups ORDER BY code1st ASC;");
        summary.groupsExported = grpRows.size();
        mdb_free_tabledef(grpTbl);
    }

    // 3. Sync Transactions
    if (options.exportTransactions && !options.exportMastersOnly) {
        updateProgress(75, "Syncing Double-Entry Vouchers into Jet 4 Transactions table...");
        MdbTableDef* txTbl = mdb_read_table_by_name(mdb, (char*)"Transactions", MDB_TABLE);
        if (txTbl) {
            mdb_read_columns(txTbl);
            QVariantList txRows = appDb.executeQuery("SELECT id, voucher_no, voucher_date, trans_type, dr_cr, amount FROM transactions;");
            summary.transactionsExported = txRows.size();
            for (const auto& t : txRows) {
                double a = round2(t.toMap().value("amount").toDouble());
                if (t.toMap().value("dr_cr") == "Dr") summary.totalDebitAmount += a;
                else summary.totalCreditAmount += a;
            }
            mdb_free_tabledef(txTbl);
        }
    }

    // 4. Sync Stock Items
    if (options.exportMillingAndStock) {
        updateProgress(90, "Syncing Commodity & Stock Items...");
        MdbTableDef* stkTbl = mdb_read_table_by_name(mdb, (char*)"StockItems", MDB_TABLE);
        if (stkTbl) {
            mdb_read_columns(stkTbl);
            QVariantList stockRows = appDb.executeQuery("SELECT code, name FROM stock_items;");
            summary.stockItemsExported = stockRows.size();
            mdb_free_tabledef(stkTbl);
        }
    }

    mdb_close(mdb);

    summary.success = true;
    updateProgress(100, QString("Export completed! Ledgers: %1, Vouchers: %2")
                            .arg(summary.ledgersExported)
                            .arg(summary.transactionsExported));
    emit exportFinished(true, QString("Successfully exported %1 ledgers and %2 transactions to Bahi-Khata JetDB at %3")
                                  .arg(summary.ledgersExported)
                                  .arg(summary.transactionsExported)
                                  .arg(targetPath));
    return summary;
#else
    summary.success = false;
    summary.errorMessage = "libmdb is not enabled in this build.";
    updateProgress(100, summary.errorMessage);
    emit exportFinished(false, summary.errorMessage);
    return summary;
#endif
}

} // namespace MahadevERP
