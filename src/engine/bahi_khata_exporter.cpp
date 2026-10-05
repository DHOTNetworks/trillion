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
#include <set>
#include <tuple>
#include <iostream>

#if defined(HAS_LIBMDB) || __has_include("mdbtools.h")
#include "mdbtools.h"
#define USE_LIBMDB 1

extern "C" ssize_t mdb_write_pg(MdbHandle *mdb, unsigned long pg);
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

#if USE_LIBMDB
static double toOleDate(const QString& dateStr) {
    if (dateStr.isEmpty()) return 0.0;
    QDate d = QDate::fromString(dateStr.left(10), "yyyy-MM-dd");
    if (!d.isValid()) d = QDate::fromString(dateStr.left(10), Qt::ISODate);
    if (!d.isValid()) return 0.0;
    QDate base(1899, 12, 30);
    return static_cast<double>(base.daysTo(d));
}

static QByteArray toJet4Text(const QString& str) {
    if (str.isEmpty()) return QByteArray();
    QByteArray res;
    res.reserve(2 + str.length() * 2);
    res.append(static_cast<char>(0xFF));
    res.append(static_cast<char>(0xFE));
    for (int i = 0; i < str.length(); ++i) {
        ushort u = str.at(i).unicode();
        res.append(static_cast<char>(u & 0xFF));
        res.append(static_cast<char>((u >> 8) & 0xFF));
    }
    return res;
}

static int packJet4Row(MdbTableDef* table, unsigned char* out_buf, MdbField* fields, int num_fields) {
    int max_fixed_size = 0;
    for (int i = 0; i < table->num_cols; i++) {
        MdbColumn *col = (MdbColumn *)g_ptr_array_index(table->columns, i);
        if (col->is_fixed) {
            int end_off = col->fixed_offset + col->col_size;
            if (end_off > max_fixed_size) max_fixed_size = end_off;
        }
    }

    int pos = 0;
    out_buf[pos++] = table->num_cols & 0xFF;
    out_buf[pos++] = (table->num_cols >> 8) & 0xFF;

    int fixed_start = pos;
    memset(&out_buf[fixed_start], 0, max_fixed_size);
    pos += max_fixed_size;

    for (int i = 0; i < num_fields; i++) {
        MdbColumn *col = (MdbColumn *)g_ptr_array_index(table->columns, fields[i].colnum);
        if (col->is_fixed && !fields[i].is_null && fields[i].value) {
            memcpy(&out_buf[fixed_start + col->fixed_offset], fields[i].value, fields[i].siz);
        }
    }

    int num_var = table->num_var_cols;
    std::vector<unsigned int> var_offsets(num_var + 1, 0);

    MdbField *var_fields[128];
    memset(var_fields, 0, sizeof(var_fields));
    for (int i = 0; i < num_fields; i++) {
        MdbColumn *col = (MdbColumn *)g_ptr_array_index(table->columns, fields[i].colnum);
        if (!col->is_fixed && col->var_col_num < num_var && col->var_col_num < 128) {
            var_fields[col->var_col_num] = &fields[i];
        }
    }

    for (int v = 0; v < num_var; v++) {
        var_offsets[v] = pos;
        MdbField *f = var_fields[v];
        if (f && !f->is_null && f->value && f->siz > 0) {
            memcpy(&out_buf[pos], f->value, f->siz);
            pos += f->siz;
        }
    }
    var_offsets[num_var] = pos;

    for (int v = num_var; v >= 0; v--) {
        out_buf[pos++] = var_offsets[v] & 0xFF;
        out_buf[pos++] = (var_offsets[v] >> 8) & 0xFF;
    }

    out_buf[pos++] = num_var & 0xFF;
    out_buf[pos++] = (num_var >> 8) & 0xFF;

    int null_bytes = (table->num_cols + 7) / 8;
    unsigned char *nullmask = &out_buf[pos];
    memset(nullmask, 0, null_bytes);
    pos += null_bytes;

    for (int i = 0; i < num_fields; i++) {
        MdbColumn *col = (MdbColumn *)g_ptr_array_index(table->columns, fields[i].colnum);
        if (!fields[i].is_null) {
            int byte_idx = col->col_num / 8;
            int bit_idx = col->col_num % 8;
            nullmask[byte_idx] |= (1 << bit_idx);
        }
    }

    return pos;
}

static bool insertJet4Row(MdbHandle* mdb, MdbTableDef* table, MdbField* fields, int num_fields) {
    unsigned char row_buf[4096];
    int row_size = packJet4Row(table, row_buf, fields, num_fields);
    gint32 pgnum = mdb_map_find_next_freepage(table, row_size);
    if (!pgnum || pgnum == -1) {
        return false;
    }

    guint16 rownum = mdb_add_row_to_pg(table, row_buf, row_size);
    if (!mdb_write_pg(mdb, pgnum)) {
        return false;
    }

    if (table->indices) {
        for (guint i = 0; i < table->indices->len; i++) {
            MdbIndex *idx = (MdbIndex *)g_ptr_array_index(table->indices, i);
            if (idx && idx->index_type == 1) {
                mdb_update_index(table, idx, num_fields, fields, pgnum, rownum);
            }
        }
    }

    table->num_rows++;
    return true;
}

static void finalizeJet4Table(MdbHandle* mdb, MdbTableDef* table) {
    if (!mdb || !table || !table->entry) return;
    mdb_read_pg(mdb, table->entry->table_pg);
    mdb_put_int32(mdb->pg_buf, mdb->fmt->tab_num_rows_offset, table->num_rows);
    mdb_write_pg(mdb, table->entry->table_pg);
}
#endif

QString BahiKhataExporter::resolveSeedTemplate(const QString& explicitPath) {
    if (!explicitPath.isEmpty() && QFile::exists(explicitPath)) {
        return explicitPath;
    }

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
            QVariantList stockRows = appDb.executeQuery("SELECT code, name, category_name, opening_bags, opening_qty, opening_value, hsn_code FROM stock_items;");
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
    updateProgress(15, "Opening Jet 4.0 database via embedded binary engine...");

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

    // 1. Sync Groups
    updateProgress(25, "Syncing Account Groups into Jet 4 Groups table...");
    MdbTableDef* grpTbl = mdb_read_table_by_name(mdb, (char*)"Groups", MDB_TABLE);
    if (grpTbl) {
        mdb_read_columns(grpTbl);
        mdb_read_indices(grpTbl);

        std::set<int> existingGroupCodes;
        mdb_rewind_table(grpTbl);
        MdbColumn* colC1 = (MdbColumn*)g_ptr_array_index(grpTbl->columns, 1);
        while (mdb_fetch_row(grpTbl)) {
            char* cStr = mdb_col_to_string(mdb, mdb->pg_buf, colC1->cur_value_start, colC1->col_type, colC1->cur_value_len);
            if (cStr) {
                existingGroupCodes.insert(atoi(cStr));
                g_free(cStr);
            }
        }

        QVariantList grpRows = appDb.executeQuery("SELECT name, code1st, code2nd, code3rd, code4th, extract_in_balance_sheet FROM account_groups ORDER BY code1st ASC;");
        for (const auto& gVar : grpRows) {
            QVariantMap g = gVar.toMap();
            int c1 = g.value("code1st").toInt();
            if (existingGroupCodes.find(c1) == existingGroupCodes.end()) {
                // Insert new group
                std::vector<MdbField> fieldsVec(grpTbl->num_cols);
                MdbField* fields = fieldsVec.data();
                for (int i = 0; i < grpTbl->num_cols; i++) {
                    MdbColumn* col = (MdbColumn*)g_ptr_array_index(grpTbl->columns, i);
                    fields[i].colnum = i;
                    fields[i].is_fixed = col->is_fixed;
                    fields[i].is_null = 1;
                }

                QByteArray gNameBytes = toJet4Text(g.value("name").toString().left(50));
                fields[0].is_null = 0; fields[0].value = gNameBytes.data(); fields[0].siz = gNameBytes.size();

                guint16 c1Val = c1;
                fields[1].is_null = 0; fields[1].value = &c1Val; fields[1].siz = 2;

                guint16 c2Val = g.value("code2nd", c1).toInt();
                fields[2].is_null = 0; fields[2].value = &c2Val; fields[2].siz = 2;

                guint16 c3Val = g.value("code3rd", c1).toInt();
                fields[3].is_null = 0; fields[3].value = &c3Val; fields[3].siz = 2;

                guint16 c4Val = g.value("code4th", c1).toInt();
                fields[4].is_null = 0; fields[4].value = &c4Val; fields[4].siz = 2;

                double extBs = g.value("extract_in_balance_sheet", 0.0).toDouble();
                fields[5].is_null = 0; fields[5].value = &extBs; fields[5].siz = 8;

                if (insertJet4Row(mdb, grpTbl, fields, grpTbl->num_cols)) {
                    summary.groupsExported++;
                }
            } else {
                summary.groupsExported++;
            }
        }
        finalizeJet4Table(mdb, grpTbl);
        mdb_free_tabledef(grpTbl);
    }

    // 2. Sync Ledgers
    updateProgress(45, "Syncing Ledgers and Account Masters into Jet 4 Ledgers table...");
    MdbTableDef* ledgersTbl = mdb_read_table_by_name(mdb, (char*)"Ledgers", MDB_TABLE);
    if (ledgersTbl) {
        mdb_read_columns(ledgersTbl);
        mdb_read_indices(ledgersTbl);

        std::map<int, int> ledgerCodeToPhysPg;
        mdb_rewind_table(ledgersTbl);
        MdbColumn* colCode = (MdbColumn*)g_ptr_array_index(ledgersTbl->columns, 3);
        MdbColumn* colOpBal = (MdbColumn*)g_ptr_array_index(ledgersTbl->columns, 6);
        MdbColumn* colSalary = (MdbColumn*)g_ptr_array_index(ledgersTbl->columns, 19);

        // In-place updates for existing ledgers
        QVariantList partyRows = appDb.executeQuery(
            "SELECT id, legacy_id, name, group_code, opening_balance, balance_type, "
            "address, city, party_station, phone, mobile, contact_person, pan, gstin, "
            "salary_per_month, bank_account, bank_name, ifsc_code "
            "FROM parties ORDER BY COALESCE(legacy_id, id) ASC;"
        );

        std::map<int, QVariantMap> partyMap;
        for (const auto& pVar : partyRows) {
            QVariantMap p = pVar.toMap();
            int legId = p.value("legacy_id").toInt();
            if (legId <= 0) legId = p.value("id").toInt();
            partyMap[legId] = p;
        }

        while (mdb_fetch_row(ledgersTbl)) {
            char* cStr = mdb_col_to_string(mdb, mdb->pg_buf, colCode->cur_value_start, colCode->col_type, colCode->cur_value_len);
            if (cStr) {
                int c = atoi(cStr);
                g_free(cStr);
                ledgerCodeToPhysPg[c] = ledgersTbl->cur_phys_pg;

                auto it = partyMap.find(c);
                if (it != partyMap.end()) {
                    bool modified = false;
                    double opBal = round2(it->second.value("opening_balance").toDouble());
                    if (colOpBal->cur_value_start > 0) {
                        memcpy(mdb->pg_buf + colOpBal->cur_value_start, &opBal, sizeof(double));
                        modified = true;
                    }
                    double sal = round2(it->second.value("salary_per_month").toDouble());
                    if (colSalary->cur_value_start > 0) {
                        memcpy(mdb->pg_buf + colSalary->cur_value_start, &sal, sizeof(double));
                        modified = true;
                    }
                    if (modified) {
                        mdb_write_pg(mdb, ledgersTbl->cur_phys_pg);
                    }
                    summary.ledgersExported++;
                }
            }
        }

        // Insert new ledgers not existing in Jet 4
        for (const auto& pair : partyMap) {
            int code = pair.first;
            if (ledgerCodeToPhysPg.find(code) == ledgerCodeToPhysPg.end()) {
                const QVariantMap& p = pair.second;
                std::vector<MdbField> fieldsVec(ledgersTbl->num_cols);
                MdbField* fields = fieldsVec.data();
                for (int i = 0; i < ledgersTbl->num_cols; i++) {
                    MdbColumn* col = (MdbColumn*)g_ptr_array_index(ledgersTbl->columns, i);
                    fields[i].colnum = i;
                    fields[i].is_fixed = col->is_fixed;
                    fields[i].is_null = 1;
                }

                QByteArray prefixBytes = toJet4Text("None");
                fields[0].is_null = 0; fields[0].value = prefixBytes.data(); fields[0].siz = prefixBytes.size();

                QByteArray nameBytes = toJet4Text(p.value("name").toString().left(50));
                fields[1].is_null = 0; fields[1].value = nameBytes.data(); fields[1].siz = nameBytes.size();

                guint16 codeVal = code;
                fields[3].is_null = 0; fields[3].value = &codeVal; fields[3].siz = 2;

                guint16 grpVal = p.value("group_code", 1).toInt();
                fields[4].is_null = 0; fields[4].value = &grpVal; fields[4].siz = 2;

                QByteArray invChoice = toJet4Text("No");
                fields[5].is_null = 0; fields[5].value = invChoice.data(); fields[5].siz = invChoice.size();

                double opBal = round2(p.value("opening_balance").toDouble());
                fields[6].is_null = 0; fields[6].value = &opBal; fields[6].siz = 8;

                QByteArray drCrBytes = toJet4Text(p.value("balance_type", "Dr").toString().left(2));
                fields[7].is_null = 0; fields[7].value = drCrBytes.data(); fields[7].siz = drCrBytes.size();

                QByteArray mailName = toJet4Text("01/04/2023");
                fields[8].is_null = 0; fields[8].value = mailName.data(); fields[8].siz = mailName.size();

                QByteArray addBytes = toJet4Text(p.value("address").toString().left(250));
                fields[9].is_null = 0; fields[9].value = addBytes.data(); fields[9].siz = addBytes.size();

                QByteArray panBytes = toJet4Text(p.value("pan").toString().left(50));
                fields[10].is_null = 0; fields[10].value = panBytes.data(); fields[10].siz = panBytes.size();

                QByteArray stnBytes = toJet4Text(p.value("party_station", p.value("city")).toString().left(50));
                fields[13].is_null = 0; fields[13].value = stnBytes.data(); fields[13].siz = stnBytes.size();

                QByteArray noneBytes = toJet4Text("None");
                fields[14].is_null = 0; fields[14].value = noneBytes.data(); fields[14].siz = noneBytes.size(); // Route
                fields[15].is_null = 0; fields[15].value = noneBytes.data(); fields[15].siz = noneBytes.size(); // Distt

                QByteArray curBalBytes = toJet4Text("0.00");
                fields[16].is_null = 0; fields[16].value = curBalBytes.data(); fields[16].siz = curBalBytes.size(); // CurrentBalance

                double zeroDbl = 0.0;
                guint32 zero32 = 0;
                fields[17].is_null = 0; fields[17].value = &zeroDbl; fields[17].siz = 8; // Percentage
                fields[18].is_null = 0; fields[18].value = &zeroDbl; fields[18].siz = 8; // InterestRate

                double sal = round2(p.value("salary_per_month").toDouble());
                fields[19].is_null = 0; fields[19].value = &sal; fields[19].siz = 8;

                QByteArray acctBytes = toJet4Text(p.value("bank_account").toString().left(50));
                fields[20].is_null = 0; fields[20].value = acctBytes.data(); fields[20].siz = acctBytes.size();

                QByteArray stateBytes = toJet4Text("STATE");
                fields[21].is_null = 0; fields[21].value = stateBytes.data(); fields[21].siz = stateBytes.size();

                fields[22].is_null = 0; fields[22].value = &zero32; fields[22].siz = 4; // ShowDateTotals
                fields[23].is_null = 0; fields[23].value = &zero32; fields[23].siz = 4; // AutoSplitUpdate

                QByteArray pTypeBytes = toJet4Text("Proprietorship Firm");
                fields[25].is_null = 0; fields[25].value = pTypeBytes.data(); fields[25].siz = pTypeBytes.size();

                QByteArray cpBytes = toJet4Text(p.value("contact_person").toString().left(50));
                fields[26].is_null = 0; fields[26].value = cpBytes.data(); fields[26].siz = cpBytes.size();

                QByteArray pStateBytes = toJet4Text("Haryana");
                fields[27].is_null = 0; fields[27].value = pStateBytes.data(); fields[27].siz = pStateBytes.size();

                QByteArray vatDlrBytes = toJet4Text("NON-VAT DEALER");
                fields[29].is_null = 0; fields[29].value = vatDlrBytes.data(); fields[29].siz = vatDlrBytes.size();

                fields[30].is_null = 0; fields[30].value = stnBytes.data(); fields[30].siz = stnBytes.size(); // PartyStation
                fields[31].is_null = 0; fields[31].value = &zeroDbl; fields[31].siz = 8; // CreditLimit
                fields[33].is_null = 0; fields[33].value = &zero32; fields[33].siz = 4; // CalculateInJointTrading

                QByteArray naBytes = toJet4Text("N/A");
                fields[34].is_null = 0; fields[34].value = naBytes.data(); fields[34].siz = naBytes.size(); // SpecialPartyType
                fields[36].is_null = 0; fields[36].value = naBytes.data(); fields[36].siz = naBytes.size(); // CommnCalcOn

                fields[38].is_null = 0; fields[38].value = &zero32; fields[38].siz = 4; // StockNotCalculateInLedger

                QByteArray gstinBytes = toJet4Text(p.value("gstin").toString().left(50));
                fields[39].is_null = 0; fields[39].value = gstinBytes.data(); fields[39].siz = gstinBytes.size();

                QByteArray gstTypeBytes = toJet4Text("Normal Dealer");
                fields[41].is_null = 0; fields[41].value = gstTypeBytes.data(); fields[41].siz = gstTypeBytes.size();

                fields[43].is_null = 0; fields[43].value = &zero32; fields[43].siz = 4; // SyncEnabled

                QByteArray ifscBytes = toJet4Text(p.value("ifsc_code").toString().left(50));
                fields[44].is_null = 0; fields[44].value = ifscBytes.data(); fields[44].siz = ifscBytes.size();

                QByteArray bankBytes = toJet4Text(p.value("bank_name").toString().left(50));
                fields[45].is_null = 0; fields[45].value = bankBytes.data(); fields[45].siz = bankBytes.size();

                fields[46].is_null = 0; fields[46].value = &zero32; fields[46].siz = 4; // ApplyTCSForParty
                fields[49].is_null = 0; fields[49].value = &zero32; fields[49].siz = 4; // TCSNotApplyInSale

                if (insertJet4Row(mdb, ledgersTbl, fields, ledgersTbl->num_cols)) {
                    summary.ledgersExported++;
                }
            }
        }

        finalizeJet4Table(mdb, ledgersTbl);
        mdb_free_tabledef(ledgersTbl);
    }

    // 3. Sync Transactions
    if (options.exportTransactions && !options.exportMastersOnly) {
        updateProgress(65, "Syncing Double-Entry Vouchers into Jet 4 Transactions table...");
        MdbTableDef* txTbl = mdb_read_table_by_name(mdb, (char*)"Transactions", MDB_TABLE);
        if (txTbl) {
            mdb_read_columns(txTbl);
            mdb_read_indices(txTbl);

            std::set<std::tuple<int, QString, int, int, QString>> existingTxSignatures;
            mdb_rewind_table(txTbl);

            MdbColumn* colVchNo = (MdbColumn*)g_ptr_array_index(txTbl->columns, 1);
            MdbColumn* colVchDate = (MdbColumn*)g_ptr_array_index(txTbl->columns, 2);
            MdbColumn* colAcCode = (MdbColumn*)g_ptr_array_index(txTbl->columns, 4);
            MdbColumn* colDrCr = (MdbColumn*)g_ptr_array_index(txTbl->columns, 5);
            MdbColumn* colAmt = (MdbColumn*)g_ptr_array_index(txTbl->columns, 6);

            while (mdb_fetch_row(txTbl)) {
                char* vNoStr = mdb_col_to_string(mdb, mdb->pg_buf, colVchNo->cur_value_start, colVchNo->col_type, colVchNo->cur_value_len);
                char* dStr = mdb_col_to_string(mdb, mdb->pg_buf, colVchDate->cur_value_start, colVchDate->col_type, colVchDate->cur_value_len);
                char* acStr = mdb_col_to_string(mdb, mdb->pg_buf, colAcCode->cur_value_start, colAcCode->col_type, colAcCode->cur_value_len);
                char* dcStr = mdb_col_to_string(mdb, mdb->pg_buf, colDrCr->cur_value_start, colDrCr->col_type, colDrCr->cur_value_len);
                char* amtStr = mdb_col_to_string(mdb, mdb->pg_buf, colAmt->cur_value_start, colAmt->col_type, colAmt->cur_value_len);

                int vNo = vNoStr ? atoi(vNoStr) : 0;
                QString dVal = dStr ? QString::fromUtf8(dStr).left(10) : "";
                int ac = acStr ? atoi(acStr) : 0;
                QString dc = dcStr ? QString::fromUtf8(dcStr) : "";
                int amtCents = amtStr ? static_cast<int>(std::round(atof(amtStr) * 100.0)) : 0;

                existingTxSignatures.insert(std::make_tuple(vNo, dVal, ac, amtCents, dc));

                if (vNoStr) g_free(vNoStr);
                if (dStr) g_free(dStr);
                if (acStr) g_free(acStr);
                if (dcStr) g_free(dcStr);
                if (amtStr) g_free(amtStr);
            }

            QString txSql = 
                "SELECT t.id, t.voucher_no, t.voucher_date, t.trans_type, t.dr_cr, t.amount, t.narration, "
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
            int rowIdx = 1;

            for (const auto& tVar : txRows) {
                QVariantMap t = tVar.toMap();
                int rawVchNo = t.value("voucher_no").toInt();
                if (rawVchNo <= 0) {
                    // Extract digits from voucher_no like "Jrnl-28"
                    QString vStr = t.value("voucher_no").toString();
                    static QRegularExpression reDigits(R"(\d+)");
                    QRegularExpressionMatch m = reDigits.match(vStr);
                    if (m.hasMatch()) rawVchNo = m.captured(0).toInt();
                    else rawVchNo = rowIdx;
                }

                QString isoDate = t.value("voucher_date").toString().left(10);
                int acCode = t.value("ac_code").toInt();
                double amt = round2(t.value("amount").toDouble());
                int amtCents = static_cast<int>(std::round(amt * 100.0));
                QString drCr = t.value("dr_cr", "Dr").toString().left(2);

                // Date key formatting
                QDate qd = QDate::fromString(isoDate, "yyyy-MM-dd");
                QString dKey = qd.isValid() ? qd.toString("MM/dd/yy") : isoDate;

                auto sig = std::make_tuple(rawVchNo, dKey, acCode, amtCents, drCr);
                if (existingTxSignatures.find(sig) == existingTxSignatures.end()) {
                    // Insert new transaction row
                    std::vector<MdbField> fieldsVec(txTbl->num_cols);
                    MdbField* fields = fieldsVec.data();
                    for (int i = 0; i < txTbl->num_cols; i++) {
                        MdbColumn* col = (MdbColumn*)g_ptr_array_index(txTbl->columns, i);
                        fields[i].colnum = i;
                        fields[i].is_fixed = col->is_fixed;
                        fields[i].is_null = 1;
                    }

                    guint16 rowNo = (drCr == "Dr") ? 1 : 2;
                    fields[0].is_null = 0; fields[0].value = &rowNo; fields[0].siz = 2;

                    guint32 vchNoVal = rawVchNo;
                    fields[1].is_null = 0; fields[1].value = &vchNoVal; fields[1].siz = 4;

                    double oleD = toOleDate(isoDate);
                    fields[2].is_null = 0; fields[2].value = &oleD; fields[2].siz = 8;

                    QByteArray ttBytes = toJet4Text(t.value("trans_type", "Jrnl").toString().left(5));
                    fields[3].is_null = 0; fields[3].value = ttBytes.data(); fields[3].siz = ttBytes.size();

                    guint16 acVal = acCode;
                    fields[4].is_null = 0; fields[4].value = &acVal; fields[4].siz = 2;

                    QByteArray dcBytes = toJet4Text(drCr);
                    fields[5].is_null = 0; fields[5].value = dcBytes.data(); fields[5].siz = dcBytes.size();

                    fields[6].is_null = 0; fields[6].value = &amt; fields[6].siz = 8;

                    QByteArray invBytes = toJet4Text(t.value("voucher_no").toString().left(50));
                    fields[7].is_null = 0; fields[7].value = invBytes.data(); fields[7].siz = invBytes.size();

                    guint16 ptVal = t.value("party_code").toInt();
                    fields[8].is_null = 0; fields[8].value = &ptVal; fields[8].siz = 2;

                    QByteArray narrBytes = toJet4Text(t.value("narration").toString().left(200));
                    fields[14].is_null = 0; fields[14].value = narrBytes.data(); fields[14].siz = narrBytes.size();

                    // Canonical Bahi-Khata default values for complete schema recognition
                    guint16 zero16 = 0;
                    guint32 zero32 = 0;
                    double zeroDbl = 0.0;
                    float zeroFlt = 0.0f;
                    QByteArray zeroStr = toJet4Text("0");

                    fields[16].is_null = 0; fields[16].value = &zero16; fields[16].siz = 2; // DueDays
                    fields[17].is_null = 0; fields[17].value = &zero16; fields[17].siz = 2; // ItemCode
                    fields[29].is_null = 0; fields[29].value = &oleD; fields[29].siz = 8;   // BankDate
                    fields[30].is_null = 0; fields[30].value = &zero32; fields[30].siz = 4; // VoucherReconcile
                    fields[35].is_null = 0; fields[35].value = &zeroDbl; fields[35].siz = 8;// ExpRate
                    fields[39].is_null = 0; fields[39].value = &zeroDbl; fields[39].siz = 8;// EstimatedAmount
                    fields[42].is_null = 0; fields[42].value = &zero32; fields[42].siz = 4; // MktCommttSrNo
                    fields[45].is_null = 0; fields[45].value = &zero32; fields[45].siz = 4; // URDPurc
                    fields[46].is_null = 0; fields[46].value = &zero32; fields[46].siz = 4; // E1PartyCode
                    fields[50].is_null = 0; fields[50].value = &zero32; fields[50].siz = 4; // CompositionVch
                    fields[52].is_null = 0; fields[52].value = zeroStr.data(); fields[52].siz = zeroStr.size(); // PlaceOfSupply
                    fields[53].is_null = 0; fields[53].value = zeroStr.data(); fields[53].siz = zeroStr.size(); // ECommGSTIN
                    fields[54].is_null = 0; fields[54].value = &zero32; fields[54].siz = 4; // TransReturn
                    fields[55].is_null = 0; fields[55].value = &zero32; fields[55].siz = 4; // GroupTick
                    fields[57].is_null = 0; fields[57].value = &zero32; fields[57].siz = 4; // ActualInv
                    fields[58].is_null = 0; fields[58].value = &zero32; fields[58].siz = 4; // ITCNotClaim
                    fields[59].is_null = 0; fields[59].value = &zero32; fields[59].siz = 4; // ReverseChargePayable
                    fields[60].is_null = 0; fields[60].value = &zero32; fields[60].siz = 4; // GSTOnGoodsAmount
                    fields[62].is_null = 0; fields[62].value = &zeroFlt; fields[62].siz = 4; // TCSRate
                    fields[63].is_null = 0; fields[63].value = &zeroDbl; fields[63].siz = 8; // TCSTaxable
                    fields[65].is_null = 0; fields[65].value = &zero32; fields[65].siz = 4; // ChallanVchNo
                    fields[68].is_null = 0; fields[68].value = &zeroFlt; fields[68].siz = 4; // TDSRate194Q
                    fields[69].is_null = 0; fields[69].value = &zeroDbl; fields[69].siz = 8; // Taxable194Q
                    fields[70].is_null = 0; fields[70].value = &zero32; fields[70].siz = 4; // TDS194QChallanVchNo
                    fields[73].is_null = 0; fields[73].value = &oleD; fields[73].siz = 8;   // TaxInputDate
                    fields[74].is_null = 0; fields[74].value = &zero32; fields[74].siz = 4; // PymtDone
                    fields[75].is_null = 0; fields[75].value = &zeroDbl; fields[75].siz = 8;// tmpBookNo
                    fields[76].is_null = 0; fields[76].value = &zeroDbl; fields[76].siz = 8;// tmpSlipNo
                    fields[78].is_null = 0; fields[78].value = &zeroDbl; fields[78].siz = 8;// RunningBNo

                    if (insertJet4Row(mdb, txTbl, fields, txTbl->num_cols)) {
                        summary.transactionsExported++;
                        if (drCr == "Dr") summary.totalDebitAmount += amt;
                        else summary.totalCreditAmount += amt;
                        existingTxSignatures.insert(sig);
                    }
                } else {
                    summary.transactionsExported++;
                    if (drCr == "Dr") summary.totalDebitAmount += amt;
                    else summary.totalCreditAmount += amt;
                }
                rowIdx++;
            }

            finalizeJet4Table(mdb, txTbl);
            mdb_free_tabledef(txTbl);
        }
    }

    // 4. Sync Stock Items
    if (options.exportMillingAndStock) {
        updateProgress(90, "Syncing Commodity & Stock Items...");
        MdbTableDef* stkTbl = mdb_read_table_by_name(mdb, (char*)"StockItems", MDB_TABLE);
        if (stkTbl) {
            mdb_read_columns(stkTbl);
            mdb_read_indices(stkTbl);

            std::set<int> existingStockCodes;
            mdb_rewind_table(stkTbl);
            MdbColumn* colCode = (MdbColumn*)g_ptr_array_index(stkTbl->columns, 1);
            while (mdb_fetch_row(stkTbl)) {
                char* cStr = mdb_col_to_string(mdb, mdb->pg_buf, colCode->cur_value_start, colCode->col_type, colCode->cur_value_len);
                if (cStr) {
                    existingStockCodes.insert(atoi(cStr));
                    g_free(cStr);
                }
            }

            QVariantList stockRows = appDb.executeQuery("SELECT code, name, category_name, opening_bags, opening_qty, opening_value, hsn_code FROM stock_items;");
            for (const auto& sVar : stockRows) {
                QVariantMap s = sVar.toMap();
                int c = s.value("code").toInt();
                if (existingStockCodes.find(c) == existingStockCodes.end()) {
                    std::vector<MdbField> fieldsVec(stkTbl->num_cols);
                    MdbField* fields = fieldsVec.data();
                    for (int i = 0; i < stkTbl->num_cols; i++) {
                        MdbColumn* col = (MdbColumn*)g_ptr_array_index(stkTbl->columns, i);
                        fields[i].colnum = i;
                        fields[i].is_fixed = col->is_fixed;
                        fields[i].is_null = 1;
                    }

                    QByteArray nameBytes = toJet4Text(s.value("name").toString().left(50));
                    fields[0].is_null = 0; fields[0].value = nameBytes.data(); fields[0].siz = nameBytes.size();

                    guint16 cVal = c;
                    fields[1].is_null = 0; fields[1].value = &cVal; fields[1].siz = 2;

                    guint16 grpVal = 1;
                    fields[2].is_null = 0; fields[2].value = &grpVal; fields[2].siz = 2;

                    double opBags = s.value("opening_bags").toDouble();
                    fields[18].is_null = 0; fields[18].value = &opBags; fields[18].siz = 8;

                    double opQty = s.value("opening_qty").toDouble();
                    fields[19].is_null = 0; fields[19].value = &opQty; fields[19].siz = 8;

                    double opVal = s.value("opening_value").toDouble();
                    fields[20].is_null = 0; fields[20].value = &opVal; fields[20].siz = 8;

                    if (insertJet4Row(mdb, stkTbl, fields, stkTbl->num_cols)) {
                        summary.stockItemsExported++;
                    }
                } else {
                    summary.stockItemsExported++;
                }
            }

            finalizeJet4Table(mdb, stkTbl);
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
