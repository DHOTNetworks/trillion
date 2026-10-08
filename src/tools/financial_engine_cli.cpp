#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <iostream>
#include "../database_manager.h"
#include "../engine/bahi_khata_migrator.h"
#include "../engine/bahi_khata_exporter.h"
#include "../engine/busy_data_migrator.h"
#include "../engine/tally_data_migrator.h"
#include "../engine/balance_sheet_calculator.h"
#include "../engine/profit_loss_calculator.h"
#include "../engine/stock_valuation_engine.h"
#include "../engine/fiscal_year_helper.h"
#include "../engine/accounting_engine.h"
#include "../engine/jet4_writer.h"
#include "mdbtools.h"

using namespace MahadevERP;

extern "C" int mdb_find_row(MdbHandle* mdb, int row_number, int* row_start, size_t* row_size);

void printUsage() {
    std::cout << "Financial Engine Inspection & Verification CLI\n"
              << "Usage:\n"
              << "  ./FinancialEngineCLI --migrate <mdb_path> <sqlite_db_path>\n"
              << "  ./FinancialEngineCLI --migrate-busy <busy_folder_or_file> <sqlite_db_path>\n"
              << "  ./FinancialEngineCLI --migrate-tally <tally_folder_or_file> <sqlite_db_path>\n"
              << "  ./FinancialEngineCLI --inspect <sqlite_db_path> [--fy <YYYY-YY> | --as-on <YYYY-MM-DD>]\n"
              << "  ./FinancialEngineCLI --inspect-busy <busy_folder_or_file>\n"
              << "  ./FinancialEngineCLI --verify-jet <mdb_path>\n"
              << "  ./FinancialEngineCLI --all\n"
              << std::endl;
}

/* Jet index/data verification front-end: delegates to the shared
 * Jet4Writer::verifyDatabase used by the export dialog as well. */
int verifyJetDb(const QString& mdbPath) {
    QByteArray pathBytes = QFile::encodeName(mdbPath);
    MdbHandle* mdb = mdb_open(pathBytes.constData(), MDB_NOFLAGS);
    if (!mdb) mdb = mdb_open(mdbPath.toUtf8().constData(), MDB_NOFLAGS);
    if (!mdb) { std::cerr << "Cannot open " << mdbPath.toStdString() << "\n"; return 1; }
    Jet4Writer::VerifyReport rep = Jet4Writer::verifyDatabase(mdb);
    mdb_close(mdb);
    if (!rep.error.empty()) {
        std::cout << "[VERIFY] ERROR: " << rep.error << "\n";
        return 1;
    }
    std::cout << "[VERIFY] tables=" << rep.tablesChecked
              << " datarows=" << rep.dataRows << " badrows=" << rep.badRows << "\n";
    int failures = (rep.badRows > 0) ? 1 : 0;
    for (const auto& vi : rep.indexes) {
        const bool countOk = vi.walked == vi.dataRows;
        std::cout << "[VERIFY] " << vi.table << "." << vi.name
                  << " walked=" << vi.walked << " datarows=" << vi.dataRows
                  << " catalog=" << vi.catalogRows
                  << " sorted=" << (vi.sorted ? "yes" : "NO")
                  << (countOk ? "" : " COUNT-MISMATCH") << "\n";
        if (!vi.sorted || !countOk) failures++;
    }
    std::cout << (failures || !rep.ok ? "[VERIFY] FAILURES\n" : "[VERIFY] ALL OK\n");
    return (failures || !rep.ok) ? 1 : 0;
}int dumpJetDb(const QString& mdbPath, const QString& tableName, int maxRows, const QString& filterCol, const QString& filterVal) {
    QByteArray pathBytes = QFile::encodeName(mdbPath);
    MdbHandle* mdb = mdb_open(pathBytes.constData(), MDB_NOFLAGS);
    if (!mdb) mdb = mdb_open(mdbPath.toUtf8().constData(), MDB_NOFLAGS);
    if (!mdb) { std::cerr << "Cannot open " << mdbPath.toStdString() << "\n"; return 1; }
    MdbTableDef* t = mdb_read_table_by_name(mdb, const_cast<char*>(tableName.toUtf8().constData()), MDB_TABLE);
    if (!t) { std::cerr << "Cannot find table " << tableName.toStdString() << "\n"; mdb_close(mdb); return 1; }
    mdb_read_columns(t);
    mdb_read_indices(t);

    std::cout << "TABLE: " << tableName.toStdString() << " (cols: " << t->num_cols << ", rows: " << t->num_rows << ")\n";
    for (unsigned i = 0; i < t->num_cols; ++i) {
        MdbColumn* col = (MdbColumn*)g_ptr_array_index(t->columns, i);
        std::cout << "  [" << i << "] " << col->name << " type=" << (int)col->col_type << " size=" << col->col_size << " fixed=" << (col->is_fixed ? 1 : 0) << " off=" << col->fixed_offset << "\n";
    }
    if (t->indices) {
        std::cout << "  INDICES (" << t->indices->len << "):\n";
        for (unsigned i = 0; i < t->indices->len; ++i) {
            MdbIndex* idx = (MdbIndex*)g_ptr_array_index(t->indices, i);
            std::cout << "    [" << i << "] " << idx->name << " root=" << idx->first_pg << " rows=" << idx->num_rows << " keys=" << idx->num_keys << " cols=";
            for (unsigned k = 0; k < idx->num_keys; ++k) {
                int kc = idx->key_col_num[k];
                MdbColumn* kc2 = (kc > 0 && kc <= (int)t->num_cols) ? (MdbColumn*)g_ptr_array_index(t->columns, kc - 1) : nullptr;
                std::cout << (kc2 ? kc2->name : "?") << (k + 1 < idx->num_keys ? "," : "");
            }
            std::cout << "\n";
        }
    }

    std::cout << "\n--- ROWS (limit " << maxRows << ") ---\n";
    int printed = 0;
    mdb_rewind_table(t);
    std::vector<char*> bound(t->num_cols);
    for (unsigned i = 0; i < t->num_cols; ++i) {
        bound[i] = (char*)std::malloc(4096);
        mdb_bind_column(t, i + 1, bound[i], nullptr);
    }
    while (mdb_fetch_row(t)) {
        if (!filterCol.isEmpty()) {
            bool match = false;
            for (unsigned i = 0; i < t->num_cols; ++i) {
                MdbColumn* col = (MdbColumn*)g_ptr_array_index(t->columns, i);
                if (filterCol.compare(col->name, Qt::CaseInsensitive) == 0) {
                    if (QString::fromUtf8(bound[i]).trimmed().compare(filterVal, Qt::CaseInsensitive) == 0) {
                        match = true;
                    }
                    break;
                }
            }
            if (!match) continue;
        }
        std::cout << "Row " << t->cur_row << ":\n";
        std::vector<MdbField> ck(t->num_cols);
        {
            int rs2 = 0; size_t rsz2 = 0;
            if (mdb_find_row(mdb, (int)t->cur_row - 1, &rs2, &rsz2) == 0
                && mdb_crack_row(t, rs2 & 0x0FFF, rsz2, ck.data()) < 0) {
                for (auto& f : ck) { f.is_null = 1; }
            }
        }
        for (unsigned i = 0; i < t->num_cols; ++i) {
            MdbColumn* col = (MdbColumn*)g_ptr_array_index(t->columns, i);
            const char* ns = (i < ck.size() && ck[i].is_null) ? " NULL" : (i < ck.size() && ck[i].siz == 0 ? " EMPTY" : "");
            std::cout << "    [" << i << "] " << col->name << " = '" << bound[i] << "'" << ns << "\n";
        }
        {
            int rs = 0; size_t rsz = 0;
            if (mdb_find_row(mdb, (int)t->cur_row - 1, &rs, &rsz) == 0) {
                int start = rs & 0x0FFF;
                const unsigned char* base = (const unsigned char*)mdb->pg_buf;
                std::cout << "    [raw] pg=" << t->cur_phys_pg << " start=" << start
                          << " size=" << rsz << " hex=";
                char hb[8];
                for (size_t bi = 0; bi < rsz && bi < 400; ++bi) {
                    snprintf(hb, sizeof(hb), "%02x", base[start + bi]);
                    std::cout << hb;
                }
                std::cout << "\n";
            }
        }
        printed++;
        if (printed >= maxRows) break;
    }
    for (unsigned i = 0; i < t->num_cols; ++i) std::free(bound[i]);
    mdb_free_tabledef(t);
    mdb_close(mdb);
    return 0;
}

int compareVch(const QString& mdb1Path, const QString& mdb2Path, const QString& tableName, const QString& transType, const QString& vch1, const QString& vch2) {
    auto fetchVch = [](const QString& path, const QString& tbl, const QString& tt, const QString& vn, std::vector<std::map<int, std::pair<std::string, bool>>>& rowsOut, std::vector<std::string>& colNames) {
        QByteArray pathBytes = QFile::encodeName(path);
        MdbHandle* mdb = mdb_open(pathBytes.constData(), MDB_NOFLAGS);
        if (!mdb) mdb = mdb_open(path.toUtf8().constData(), MDB_NOFLAGS);
        if (!mdb) return;
        MdbTableDef* t = mdb_read_table_by_name(mdb, const_cast<char*>(tbl.toUtf8().constData()), MDB_TABLE);
        if (!t) { mdb_close(mdb); return; }
        mdb_read_columns(t);
        colNames.clear();
        for (unsigned i = 0; i < t->num_cols; ++i) {
            MdbColumn* c = (MdbColumn*)g_ptr_array_index(t->columns, i);
            colNames.push_back(c->name);
        }
        std::vector<char*> bound(t->num_cols);
        for (unsigned i = 0; i < t->num_cols; ++i) {
            bound[i] = (char*)std::malloc(4096);
            mdb_bind_column(t, i + 1, bound[i], nullptr);
        }
        mdb_rewind_table(t);
        while (mdb_fetch_row(t)) {
            QString curTT, curVN;
            for (unsigned i = 0; i < t->num_cols; ++i) {
                MdbColumn* c = (MdbColumn*)g_ptr_array_index(t->columns, i);
                if (strcasecmp(c->name, "TransType") == 0) curTT = QString::fromUtf8(bound[i]).trimmed();
                if (strcasecmp(c->name, "VoucherNumber") == 0) curVN = QString::fromUtf8(bound[i]).trimmed();
            }
            if (curTT.compare(tt, Qt::CaseInsensitive) == 0 && curVN == vn) {
                std::map<int, std::pair<std::string, bool>> r;
                for (unsigned i = 0; i < t->num_cols; ++i) {
                    MdbColumn* c = (MdbColumn*)g_ptr_array_index(t->columns, i);
                    r[(int)i] = {std::string(bound[i]), false};
                }
                rowsOut.push_back(r);
            }
        }
        for (unsigned i = 0; i < t->num_cols; ++i) std::free(bound[i]);
        mdb_free_tabledef(t);
        mdb_close(mdb);
    };

    std::vector<std::map<int, std::pair<std::string, bool>>> rows1, rows2;
    std::vector<std::string> cols1, cols2;
    fetchVch(mdb1Path, tableName, transType, vch1, rows1, cols1);
    fetchVch(mdb2Path, tableName, transType, vch2, rows2, cols2);

    std::cout << "COMPARING " << tableName.toStdString() << " " << transType.toStdString() << " " << vch1.toStdString() << " (pristine, " << rows1.size() << " legs) vs " << vch2.toStdString() << " (exported, " << rows2.size() << " legs)\n";

    size_t numLegs = std::max(rows1.size(), rows2.size());
    for (size_t leg = 0; leg < numLegs; ++leg) {
        std::cout << "\n=== LEG " << (leg + 1) << " ===\n";
        for (size_t col = 0; col < cols1.size(); ++col) {
            std::string val1 = (leg < rows1.size() && rows1[leg].count((int)col)) ? rows1[leg][(int)col].first : "<MISSING>";
            std::string val2 = (leg < rows2.size() && rows2[leg].count((int)col)) ? rows2[leg][(int)col].first : "<MISSING>";
            bool diff = (val1 != val2);
            std::cout << (diff ? " [DIFF] " : "        ")
                      << "[" << col << "] " << cols1[col] << ": "
                      << "Pristine='" << val1 << "' | Exported='" << val2 << "'\n";
        }
    }
    return 0;
}

void inspectDatabase(const QString& dbPath, const QString& fyParam, const QString& asOnParam) {
    std::cout << "======================================================================\n";
    std::cout << "INSPECTING DATABASE: " << dbPath.toStdString() << "\n";
    std::cout << "======================================================================\n";

    DatabaseManager::instance().closeDatabase();
    if (!DatabaseManager::instance().initDatabase(dbPath)) {
        std::cerr << "Failed to open SQLite database at: " << dbPath.toStdString() << "\n";
        return;
    }

    QVariantList fyList = DatabaseManager::instance().executeQuery(
        "SELECT id, year_name, start_date, end_date, is_locked, is_active FROM financial_years ORDER BY start_date ASC;"
    );

    if (fyList.isEmpty()) {
        std::cout << "No financial years found in database.\n";
        return;
    }

    for (const auto& fyVar : fyList) {
        QVariantMap fyMap = fyVar.toMap();
        QString fyCode = fyMap.value("year_name").toString();
        QString sDate = fyMap.value("start_date").toString();
        QString eDate = fyMap.value("end_date").toString();
        bool isAudited = fyMap.value("is_locked").toBool();

        if (!fyParam.isEmpty() && !fyCode.contains(fyParam, Qt::CaseInsensitive)) {
            continue;
        }

        QString targetDate = asOnParam.isEmpty() ? eDate : asOnParam;

        std::cout << "\n----------------------------------------------------------------------\n";
        std::cout << "FISCAL YEAR: " << fyCode.toStdString()
                  << " (" << sDate.toStdString() << " to " << targetDate.toStdString() << ")"
                  << (isAudited ? " [AUDITED]" : " [PROVISIONAL]") << "\n";
        std::cout << "----------------------------------------------------------------------\n";

        // 1. Profit & Loss Calculation
        ProfitLossData pl = ProfitLossCalculator::calculate(sDate, targetDate);
        std::cout << "\n[PROFIT & LOSS STATEMENT]\n";
        std::cout << "  Firm Name:            " << pl.firmName.toStdString() << "\n";
        std::cout << "  Opening Stock:        " << AccountingEngine::formatIndianCurrency(pl.openingStockValue, true).toStdString() << "\n";
        std::cout << "  Purchases/Procure:    " << AccountingEngine::formatIndianCurrency(pl.totalProcurement, true).toStdString() << "\n";
        std::cout << "  Direct Expenses:      " << AccountingEngine::formatIndianCurrency(pl.totalDirectExpenses, true).toStdString() << "\n";
        std::cout << "  Sales Revenue:        " << AccountingEngine::formatIndianCurrency(pl.totalSalesRevenue, true).toStdString() << "\n";
        std::cout << "  Closing Stock:        " << AccountingEngine::formatIndianCurrency(pl.closingStockValue, true).toStdString() << "\n";
        std::cout << "  Gross Profit:         " << AccountingEngine::formatIndianCurrency(pl.grossProfit, true).toStdString() << "\n";
        std::cout << "  Gross Loss:           " << AccountingEngine::formatIndianCurrency(pl.grossLoss, true).toStdString() << "\n";
        std::cout << "  Indirect Incomes:     " << AccountingEngine::formatIndianCurrency(pl.indirectIncomes, true).toStdString() << "\n";
        std::cout << "  Indirect Expenses:    " << AccountingEngine::formatIndianCurrency(pl.indirectExpenses, true).toStdString() << "\n";
        std::cout << "  NET PROFIT:           " << AccountingEngine::formatIndianCurrency(pl.netProfit, true).toStdString() << "\n";
        std::cout << "  NET LOSS:             " << AccountingEngine::formatIndianCurrency(pl.netLoss, true).toStdString() << "\n";

        // 2. Balance Sheet Calculation
        BalanceSheetData bs = BalanceSheetCalculator::calculate(targetDate);
        std::cout << "\n[BALANCE SHEET - AS ON " << targetDate.toStdString() << "]\n";
        
        std::cout << "\n  --- LIABILITIES SIDE (" << bs.liabilitiesGroups.size() << " Groups) ---\n";
        for (const auto& g : bs.liabilitiesGroups) {
            std::cout << "    * " << g.name.toStdString() << ": " << g.amountFmt.toStdString()
                      << " (" << g.children.size() << " parties)\n";
            for (const auto& c : g.children) {
                std::cout << "        - " << c.name.toStdString() << ": " << c.amountFmt.toStdString() << "\n";
            }
        }
        std::cout << "    >> GRAND TOTAL LIABILITIES: " << bs.totalLiabilitiesFmt.toStdString() << "\n";

        std::cout << "\n  --- ASSETS SIDE (" << bs.assetsGroups.size() << " Groups) ---\n";
        for (const auto& g : bs.assetsGroups) {
            std::cout << "    * " << g.name.toStdString() << ": " << g.amountFmt.toStdString()
                      << " (" << g.children.size() << " parties)\n";
            for (const auto& c : g.children) {
                std::cout << "        - " << c.name.toStdString() << ": " << c.amountFmt.toStdString() << "\n";
            }
        }
        std::cout << "    >> GRAND TOTAL ASSETS: " << bs.totalAssetsFmt.toStdString() << "\n";

        std::cout << "\n  >> DIFFERENCE: " << AccountingEngine::formatIndianCurrency(bs.difference, true).toStdString()
                  << (bs.isBalanced ? " [BALANCED]" : " [UNBALANCED / PROVISIONAL]") << "\n";
    }
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    QStringList args = app.arguments();
    if (args.size() < 2) {
        printUsage();
        return 0;
    }

    if (args.contains("--migrate")) {
        int idx = args.indexOf("--migrate");
        if (idx + 2 >= args.size()) {
            std::cerr << "Error: --migrate requires <mdb_path> <sqlite_db_path>\n";
            return 1;
        }
        QString mdbPath = args[idx + 1];
        QString dbPath = args[idx + 2];

        std::cout << "Starting migration from: " << mdbPath.toStdString() << " -> " << dbPath.toStdString() << "\n";
        DatabaseManager::instance().closeDatabase();
        if (QFile::exists(dbPath)) {
            QFile::remove(dbPath);
        }
        if (!DatabaseManager::instance().initDatabase(dbPath)) {
            std::cerr << "Failed to create/init SQLite database at " << dbPath.toStdString() << "\n";
            return 1;
        }

        BahiKhataMigrator migrator;
        bool ok = migrator.migrate_mdb_file(mdbPath);

        if (ok) {
            std::cout << "Migration SUCCESSFUL!\n";
            inspectDatabase(dbPath, "", "");
            return 0;
        } else {
            std::cerr << "Migration FAILED.\n";
            return 1;
        }
    }

    if (args.contains("--migrate-busy")) {
        int idx = args.indexOf("--migrate-busy");
        if (idx + 2 >= args.size()) {
            std::cerr << "Error: --migrate-busy requires <busy_folder_or_file> <sqlite_db_path>\n";
            return 1;
        }
        QString busyPath = args[idx + 1];
        QString dbPath = args[idx + 2];

        std::cout << "Starting Busy migration from: " << busyPath.toStdString() << " -> " << dbPath.toStdString() << "\n";
        DatabaseManager::instance().closeDatabase();
        if (QFile::exists(dbPath)) {
            QFile::remove(dbPath);
        }
        if (!DatabaseManager::instance().initDatabase(dbPath)) {
            std::cerr << "Failed to create/init SQLite database at " << dbPath.toStdString() << "\n";
            return 1;
        }

        BusyDataMigrator migrator;
        bool ok = migrator.migrate_busy_data(busyPath);

        if (ok) {
            std::cout << "Busy Migration SUCCESSFUL!\n";
            inspectDatabase(dbPath, "", "");
            return 0;
        } else {
            std::cerr << "Busy Migration FAILED.\n";
            return 1;
        }
    }

    if (args.contains("--migrate-tally")) {
        int idx = args.indexOf("--migrate-tally");
        if (idx + 2 >= args.size()) {
            std::cerr << "Error: --migrate-tally requires <tally_folder_or_file> <sqlite_db_path>\n";
            return 1;
        }
        QString tallyPath = args[idx + 1];
        QString dbPath = args[idx + 2];

        std::cout << "Starting Tally migration from: " << tallyPath.toStdString() << " -> " << dbPath.toStdString() << "\n";
        DatabaseManager::instance().closeDatabase();
        if (QFile::exists(dbPath)) {
            QFile::remove(dbPath);
        }
        if (!DatabaseManager::instance().initDatabase(dbPath)) {
            std::cerr << "Failed to create/init SQLite database at " << dbPath.toStdString() << "\n";
            return 1;
        }

        TallyDataMigrator migrator;
        bool ok = migrator.migrate_tally_data(tallyPath);

        if (ok) {
            std::cout << "Tally Migration SUCCESSFUL!\n";
            inspectDatabase(dbPath, "", "");
            return 0;
        } else {
            std::cerr << "Tally Migration FAILED.\n";
            return 1;
        }
    }

    if (args.contains("--inspect")) {
        int idx = args.indexOf("--inspect");
        if (idx + 1 >= args.size()) {
            std::cerr << "Error: --inspect requires <sqlite_db_path>\n";
            return 1;
        }
        QString dbPath = args[idx + 1];
        QString fyParam;
        QString asOnParam;
        if (args.contains("--fy")) {
            int fIdx = args.indexOf("--fy");
            if (fIdx + 1 < args.size()) fyParam = args[fIdx + 1];
        }
        if (args.contains("--as-on")) {
            int aIdx = args.indexOf("--as-on");
            if (aIdx + 1 < args.size()) asOnParam = args[aIdx + 1];
        }
        inspectDatabase(dbPath, fyParam, asOnParam);
        return 0;
    }

    if (args.contains("--inspect-busy")) {
        int idx = args.indexOf("--inspect-busy");
        if (idx + 1 >= args.size()) {
            std::cerr << "Error: --inspect-busy requires <busy_folder_or_file>\n";
            return 1;
        }
        QString busyPath = args[idx + 1];
        BusyDataMigrator migrator;
        QVariantMap insp = migrator.inspect_busy_data(busyPath);
        std::cout << "======================================================================\n";
        std::cout << "INSPECTING BUSY SOURCE: " << busyPath.toStdString() << "\n";
        std::cout << "======================================================================\n";
        std::cout << "  Company Name:       " << insp.value("companyName").toString().toStdString() << "\n";
        std::cout << "  GSTIN:              " << insp.value("gstin").toString().toStdString() << "\n";
        std::cout << "  Financial Year:     " << insp.value("financialYear").toString().toStdString() << "\n";
        std::cout << "  Account Groups:     " << insp.value("groupsCount").toInt() << "\n";
        std::cout << "  Accounts/Parties:   " << insp.value("accountsCount").toInt() << "\n";
        std::cout << "  Stock Items:        " << insp.value("itemsCount").toInt() << "\n";
        std::cout << "  Stock Units:        " << insp.value("unitsCount").toInt() << "\n";
        std::cout << "  Vouchers:           " << insp.value("totalVouchersCount").toInt() << "\n";
        std::cout << "  GL Transactions:    " << insp.value("glTransactionsCount").toInt() << "\n";
        std::cout << "  Total Debit:        " << AccountingEngine::formatIndianCurrency(insp.value("debitSum").toDouble(), true).toStdString() << "\n";
        std::cout << "  Total Credit:       " << AccountingEngine::formatIndianCurrency(insp.value("creditSum").toDouble(), true).toStdString() << "\n";
        std::cout << "  GL Discrepancy:     " << AccountingEngine::formatIndianCurrency(insp.value("glDiscrepancy").toDouble(), true).toStdString() << "\n";
        std::cout << "  Status:             " << (insp.value("isBalanced").toBool() ? "BALANCED" : "DISCREPANCY DETECTED") << "\n";
        return 0;
    }

    if (args.contains("--export-bahi-khata")) {
        int idx = args.indexOf("--export-bahi-khata");
        if (idx + 2 >= args.size()) {
            std::cerr << "Error: --export-bahi-khata requires <sqlite_db_path> <target_mdb_path> [financial_year]\n";
            return 1;
        }
        QString dbPath = args[idx + 1];
        QString targetMdbPath = args[idx + 2];
        QString fyParam = (idx + 3 < args.size()) ? args[idx + 3] : "";

        std::cout << "Exporting from SQLite: " << dbPath.toStdString() << " -> JetDB: " << targetMdbPath.toStdString() << "\n";
        DatabaseManager::instance().closeDatabase();
        if (!DatabaseManager::instance().initDatabase(dbPath)) {
            std::cerr << "Failed to open SQLite database at " << dbPath.toStdString() << "\n";
            return 1;
        }

        BahiKhataExporter exporter;
        bool ok = exporter.exportDatabase(targetMdbPath, fyParam);

        if (ok) {
            std::cout << "Bahi-Khata JetDB Export SUCCESSFUL!\n";
            return 0;
        } else {
            std::cerr << "Bahi-Khata JetDB Export FAILED.\n";
            return 1;
        }
    }

    if (args.contains("--verify-jet")) {
        int idx = args.indexOf("--verify-jet");
        if (idx + 1 >= args.size()) {
            std::cerr << "Error: --verify-jet requires <mdb_path>\n";
            return 1;
        }
        return verifyJetDb(args[idx + 1]);
    }
    if (args.contains("--dump-jet")) {
        int idx = args.indexOf("--dump-jet");
        if (idx + 2 >= args.size()) {
            std::cerr << "Error: --dump-jet requires <mdb_path> <table_name> [max_rows] [filter_col] [filter_val]\n";
            return 1;
        }
        QString mdbPath = args[idx + 1];
        QString tblName = args[idx + 2];
        int maxRows = (idx + 3 < args.size()) ? args[idx + 3].toInt() : 5;
        QString filterCol = (idx + 4 < args.size()) ? args[idx + 4] : "";
        QString filterVal = (idx + 5 < args.size()) ? args[idx + 5] : "";
        return dumpJetDb(mdbPath, tblName, maxRows, filterCol, filterVal);
    }
    if (args.contains("--compare-vch")) {
        int idx = args.indexOf("--compare-vch");
        if (idx + 6 >= args.size()) {
            std::cerr << "Error: --compare-vch requires <mdb1_path> <mdb2_path> <table_name> <trans_type> <vch1_no> <vch2_no>\n";
            return 1;
        }
        return compareVch(args[idx + 1], args[idx + 2], args[idx + 3], args[idx + 4], args[idx + 5], args[idx + 6]);
    }
    if (args.contains("--copy-voucher")) {
        int idx = args.indexOf("--copy-voucher");
        if (idx + 5 >= args.size()) {
            std::cerr << "Error: --copy-voucher requires <src_mdb> <dst_mdb> <trans_type> <vch_no> <date_substr>\n";
            return 1;
        }
        QString srcPath = args[idx + 1], dstPath = args[idx + 2];
        QString tt = args[idx + 3], vn = args[idx + 4], dt = args[idx + 5];
        QByteArray sp = QFile::encodeName(srcPath);
        MdbHandle* smdb = mdb_open(sp.constData(), MDB_NOFLAGS);
        if (!smdb) { std::cerr << "Cannot open src\n"; return 1; }
        MdbTableDef* st = mdb_read_table_by_name(smdb, (char*)"Transactions", MDB_TABLE);
        mdb_read_columns(st);
        struct RawRow { std::vector<QByteArray> cols; std::vector<char> isNull; };
        std::vector<RawRow> rows;
        {
            std::vector<MdbField> fb(st->num_cols);
            std::vector<char*> bound(st->num_cols);
            for (unsigned i = 0; i < st->num_cols; ++i) { bound[i] = (char*)malloc(4096); mdb_bind_column(st, i + 1, bound[i], nullptr); }
            mdb_rewind_table(st);
            while (mdb_fetch_row(st)) {
                QString cTT, cVN, cDT;
                for (unsigned i = 0; i < st->num_cols; ++i) {
                    MdbColumn* c = (MdbColumn*)g_ptr_array_index(st->columns, i);
                    if (!strcasecmp(c->name, "TransType")) cTT = QString::fromUtf8(bound[i]).trimmed();
                    if (!strcasecmp(c->name, "VoucherNumber")) cVN = QString::fromUtf8(bound[i]).trimmed();
                    if (!strcasecmp(c->name, "VoucherDate")) cDT = QString::fromUtf8(bound[i]).trimmed();
                }
                if (cTT.compare(tt, Qt::CaseInsensitive) == 0 && cVN == vn && cDT.contains(dt)) {
                    int rs = 0; size_t rsz = 0;
                    if (mdb_find_row(smdb, (int)st->cur_row - 1, &rs, &rsz) != 0) continue;
                    std::vector<MdbField> f2(st->num_cols);
                    if (mdb_crack_row(st, rs & 0x0FFF, rsz, f2.data()) < 0) continue;
                    RawRow r; r.cols.resize(st->num_cols); r.isNull.resize(st->num_cols);
                    for (unsigned i = 0; i < st->num_cols; ++i) {
                        r.isNull[i] = (char)(f2[i].is_null ? 1 : 0);
                        if (!f2[i].is_null && f2[i].value && f2[i].siz > 0)
                            r.cols[i] = QByteArray((const char*)f2[i].value, (int)f2[i].siz);
                    }
                    rows.push_back(std::move(r));
                }
            }
            for (unsigned i = 0; i < st->num_cols; ++i) free(bound[i]);
        }
        mdb_close(smdb);
        if (rows.empty()) { std::cerr << "No matching rows\n"; return 1; }
        QByteArray dp = QFile::encodeName(dstPath);
        MdbHandle* dmdb = mdb_open(dp.constData(), MDB_WRITABLE);
        if (!dmdb) { std::cerr << "Cannot open dst writable\n"; return 1; }
        MdbTableDef* dtbl = mdb_read_table_by_name(dmdb, (char*)"Transactions", MDB_TABLE);
        mdb_read_columns(dtbl);
        mdb_read_indices(dtbl);
        int done = 0;
        for (auto& r : rows) {
            std::vector<MdbField> f(dtbl->num_cols);
            for (unsigned i = 0; i < dtbl->num_cols && i < r.cols.size(); ++i) {
                f[i].colnum = (int)i;
                if (r.isNull[i]) { f[i].is_null = 1; f[i].value = nullptr; f[i].siz = 0; }
                else { f[i].is_null = 0; f[i].value = (void*)r.cols[i].data(); f[i].siz = r.cols[i].size(); }
            }
            if (mdb_insert_row(dtbl, dtbl->num_cols, f.data())) done++;
        }
        mdb_close(dmdb);
        std::cout << "Copied " << done << "/" << rows.size() << " legs\n";
        return done == (int)rows.size() ? 0 : 1;
    }
    if (args.contains("--audit-enc")) {
        int idx = args.indexOf("--audit-enc");
        if (idx + 2 >= args.size()) {
            std::cerr << "Error: --audit-enc requires <mdb_path> <table_name> [date_substr] [max_samples]\n";
            return 1;
        }
        QString mdbPath = args[idx + 1], tableName = args[idx + 2];
        QString dateSub = (idx + 3 < args.size() && !args[idx + 3].startsWith("--")) ? args[idx + 3] : "";
        int maxSamp = 3;
        if (idx + 4 < args.size()) maxSamp = args[idx + 4].toInt();
        QByteArray pb = QFile::encodeName(mdbPath);
        MdbHandle* mdb = mdb_open(pb.constData(), MDB_NOFLAGS);
        if (!mdb) { std::cerr << "Cannot open\n"; return 1; }
        MdbTableDef* t = mdb_read_table_by_name(mdb, const_cast<char*>(tableName.toUtf8().constData()), MDB_TABLE);
        if (!t) { std::cerr << "No table\n"; mdb_close(mdb); return 1; }
        mdb_read_columns(t);
        struct Samp { int len; std::string head; bool comp; std::string str; };
        std::map<int, std::vector<Samp>> samples;
        std::vector<char*> bound(t->num_cols);
        for (unsigned i = 0; i < t->num_cols; ++i) { bound[i] = (char*)malloc(8192); mdb_bind_column(t, i + 1, bound[i], nullptr); }
        mdb_rewind_table(t);
        while (mdb_fetch_row(t)) {
            if (!dateSub.isEmpty()) {
                bool ok = false;
                for (unsigned i = 0; i < t->num_cols; ++i) {
                    MdbColumn* c = (MdbColumn*)g_ptr_array_index(t->columns, i);
                    if (!strcasecmp(c->name, "VoucherDate") && QString::fromUtf8(bound[i]).contains(dateSub)) { ok = true; break; }
                }
                if (!ok) continue;
            }
            for (unsigned i = 0; i < t->num_cols; ++i) {
                MdbColumn* c = (MdbColumn*)g_ptr_array_index(t->columns, i);
                if (c->col_type != MDB_TEXT) continue;
                if ((int)samples[i].size() >= maxSamp) continue;
                if (c->cur_value_len <= 0) continue;
                const unsigned char* p = (const unsigned char*)mdb->pg_buf + c->cur_value_start;
                char hb[32]; std::string hs;
                for (int b = 0; b < c->cur_value_len && b < 8; ++b) { snprintf(hb, sizeof(hb), "%02x", p[b]); hs += hb; }
                bool comp = (c->cur_value_len >= 2 && p[0] == 0xFF && p[1] == 0xFE);
                samples[i].push_back({c->cur_value_len, hs, comp, std::string(bound[i])});
            }
            bool full = true;
            for (unsigned i = 0; i < t->num_cols; ++i) {
                MdbColumn* c = (MdbColumn*)g_ptr_array_index(t->columns, i);
                if (c->col_type == MDB_TEXT && (int)samples[i].size() < maxSamp) { full = false; break; }
            }
            if (full) break;
        }
        for (unsigned i = 0; i < t->num_cols; ++i) {
            MdbColumn* c = (MdbColumn*)g_ptr_array_index(t->columns, i);
            if (c->col_type != MDB_TEXT) continue;
            std::cout << "[" << i << "] " << c->name << " samples=" << samples[i].size();
            for (auto& s : samples[i])
                std::cout << " {len=" << s.len << " head=" << s.head << (s.comp ? " COMPRESSED" : " RAW-UCS2") << " str='" << s.str << "'}";
            std::cout << "\n";
        }
        for (unsigned i = 0; i < t->num_cols; ++i) free(bound[i]);
        mdb_free_tabledef(t);
        mdb_close(mdb);
        return 0;
    }

    if (args.contains("--audit-keys")) {
        int idx = args.indexOf("--audit-keys");
        if (idx + 1 >= args.size()) {
            std::cerr << "Error: --audit-keys requires <mdb_path> [table_name]\n";
            return 1;
        }
        QString mdbPath = args[idx + 1];
        QString tableName = (idx + 2 < args.size() && !args[idx + 2].startsWith("--")) ? args[idx + 2] : "Transactions";
        QByteArray pb = QFile::encodeName(mdbPath);
        MdbHandle* mdb = mdb_open(pb.constData(), MDB_NOFLAGS);
        if (!mdb) { std::cerr << "Cannot open\n"; return 1; }
        MdbTableDef* t = mdb_read_table_by_name(mdb, const_cast<char*>(tableName.toUtf8().constData()), MDB_TABLE);
        if (!t) { std::cerr << "No table\n"; mdb_close(mdb); return 1; }
        mdb_read_columns(t);
        mdb_read_indices(t);
        // Collect distinct data pages via physical fetch order.
        std::vector<uint32_t> dataPages;
        {
            std::vector<char*> bound(t->num_cols);
            for (unsigned i = 0; i < t->num_cols; ++i) { bound[i] = (char*)malloc(4096); mdb_bind_column(t, i + 1, bound[i], nullptr); }
            mdb_rewind_table(t);
            uint32_t lastPg = 0;
            while (mdb_fetch_row(t)) {
                if (dataPages.empty() || t->cur_phys_pg != lastPg) { dataPages.push_back(t->cur_phys_pg); lastPg = t->cur_phys_pg; }
            }
            for (unsigned i = 0; i < t->num_cols; ++i) free(bound[i]);
        }
        std::cout << "Data pages: " << dataPages.size() << "\n";
        int totalMismatch = 0;
        if (t->indices) {
            for (unsigned ii = 0; ii < t->indices->len; ++ii) {
                MdbIndex* mIdx = (MdbIndex*)g_ptr_array_index(t->indices, ii);
                std::vector<Jet4Writer::IndexEntryInfo> entries;
                Jet4Writer::Status st = Jet4Writer::BTreeEngine::walkIndex(mdb, t, mIdx, entries);
                if (!st.ok) { std::cout << "walk failed " << mIdx->name << ": " << st.error << "\n"; continue; }
                // key: (dataPg<<8|local) -> stored full entry bytes
                std::map<uint32_t, std::vector<uint8_t>> stored;
                for (auto& e : entries) {
                    uint32_t k = (e.dataPg << 8) | (e.rowIdx & 0xFF);
                    if (!stored.count(k)) {
                        std::vector<uint8_t> full = e.fullKey;
                        uint32_t pgRow = (e.dataPg << 8) | (e.rowIdx & 0xFF);
                        full.push_back((pgRow >> 24) & 0xFF); full.push_back((pgRow >> 16) & 0xFF);
                        full.push_back((pgRow >> 8) & 0xFF); full.push_back(pgRow & 0xFF);
                        stored[k] = std::move(full);
                    }
                }
                int mism = 0, checked = 0, missing = 0, missingFlagged = 0;
                // Rebuild each row's key by enumerating page slots authoritatively.
                std::vector<MdbField> fb(t->num_cols);
                int rco = mdb->fmt->row_count_offset;
                for (uint32_t pg : dataPages) {
                    if (!Jet4Writer::UsageMapManager::readPage(mdb, pg)) continue;
                    if (mdb->pg_buf[0] != 0x01) continue; // data pages only
                    int nrows = mdb_get_int16(mdb->pg_buf, rco);
                    if (nrows <= 0 || nrows > 1000) continue;
                    for (int s = 0; s < nrows; ++s) {
                        int rs = 0; size_t rsz = 0;
                        if (mdb_find_row(mdb, s, &rs, &rsz) != 0 || rsz == 0) continue;
                        if (rs & 0x4000) continue; // chain-link/overflow slots (libmdb parity): not standalone rows
                        bool flagged = (rs & 0x8000) != 0;
                        if (mdb_crack_row(t, rs & 0x0FFF, rsz, fb.data()) < 0) continue;
                        std::vector<Jet4Writer::Field> fs;
                        fs.reserve(t->num_cols);
                        for (unsigned i = 0; i < t->num_cols; ++i) {
                            Jet4Writer::Field f; f.colnum = (int)i; f.isNull = fb[i].is_null != 0;
                            f.data = (const uint8_t*)fb[i].value; f.size = fb[i].siz > 0 ? (size_t)fb[i].siz : 0;
                            if (f.isNull) { f.data = nullptr; f.size = 0; }
                            fs.push_back(f);
                        }
                        std::vector<uint8_t> rebuilt;
                        Jet4Writer::Status bs = Jet4Writer::BTreeEngine::buildEntry(t, mIdx, fs.data(), fs.size(), pg, (uint16_t)(s + 1), rebuilt);
                        if (!bs.ok) continue;
                        uint32_t k = (pg << 8) | ((uint32_t)s & 0xFF);
                        auto it = stored.find(k);
                        if (it == stored.end()) {
                            if (flagged) { missingFlagged++; continue; }
                            if (missing < 2000)
                                std::cout << "  MISSING idx=" << mIdx->name << " pg=" << pg << " slot=" << s << "\n";
                            missing++;
                            continue;
                        }
                        checked++;
                        if (it->second != rebuilt) {
                            if (mism < 3) {
                                std::cout << "  MISMATCH idx=" << mIdx->name << " pg=" << pg << " slot=" << s << "\n    stored : ";
                                for (auto b : it->second) printf("%02x", b);
                                std::cout << "\n    rebuilt: ";
                                for (auto b : rebuilt) printf("%02x", b);
                                std::cout << "\n";
                            }
                            mism++;
                        }
                    }
                }
                std::cout << "Index " << mIdx->name << ": checked=" << checked << " mismatches=" << mism << " missing=" << missing << " (flagged-skipped=" << missingFlagged << ")\n";
                totalMismatch += mism + missing;
            }
        }
        mdb_free_tabledef(t);
        mdb_close(mdb);
        std::cout << (totalMismatch ? "KEYS DIVERGE\n" : "KEYS CONSISTENT\n");
        return totalMismatch ? 1 : 0;
    }

    if (args.contains("--audit-seek")) {
        int idx = args.indexOf("--audit-seek");
        if (idx + 1 >= args.size()) {
            std::cerr << "Error: --audit-seek requires <mdb_path> [table_name]\n";
            return 1;
        }
        QString mdbPath = args[idx + 1];
        QString tableName = (idx + 2 < args.size() && !args[idx + 2].startsWith("--")) ? args[idx + 2] : "Transactions";
        QByteArray pb = QFile::encodeName(mdbPath);
        MdbHandle* mdb = mdb_open(pb.constData(), MDB_NOFLAGS);
        if (!mdb) { std::cerr << "Cannot open\n"; return 1; }
        MdbTableDef* t = mdb_read_table_by_name(mdb, const_cast<char*>(tableName.toUtf8().constData()), MDB_TABLE);
        if (!t) { std::cerr << "No table\n"; mdb_close(mdb); return 1; }
        mdb_read_columns(t);
        mdb_read_indices(t);
        int totalBad = 0;
        if (t->indices) {
            for (unsigned ii = 0; ii < t->indices->len; ++ii) {
                MdbIndex* mIdx = (MdbIndex*)g_ptr_array_index(t->indices, ii);
                std::vector<Jet4Writer::IndexEntryInfo> entries;
                Jet4Writer::Status st = Jet4Writer::BTreeEngine::walkIndex(mdb, t, mIdx, entries);
                if (!st.ok) { std::cout << "walk failed " << mIdx->name << ": " << st.error << "\n"; totalBad++; continue; }
                // Group entries by leaf page (walk order follows the chain).
                std::map<uint32_t, std::vector<std::vector<uint8_t>>> byLeaf;
                for (auto& e : entries) {
                    std::vector<uint8_t> full = e.fullKey;
                    uint32_t pgRow = (e.dataPg << 8) | (e.rowIdx & 0xFF);
                    full.push_back((pgRow >> 24) & 0xFF); full.push_back((pgRow >> 16) & 0xFF);
                    full.push_back((pgRow >> 8) & 0xFF); full.push_back(pgRow & 0xFF);
                    byLeaf[e.leafPg].push_back(std::move(full));
                }
                int bad = 0, checked = 0;
                for (auto& kv : byLeaf) {
                    // Test first, middle, last entry of each leaf (descent must land on this leaf).
                    std::vector<size_t> picks = {0, kv.second.size() / 2, kv.second.size() - 1};
                    for (size_t pi : picks) {
                        if (pi >= kv.second.size()) continue;
                        auto& key = kv.second[pi];
                        std::vector<uint32_t> anc; std::string err;
                        uint32_t leaf = Jet4Writer::BTreeEngine::findLeaf(mdb, mIdx, key.data(), key.size(), anc, err);
                        checked++;
                        if (!leaf || leaf != kv.first) {
                            if (bad < 3) {
                                std::string kh;
                                char hb[4];
                                for (size_t bi = 0; bi < key.size() && bi < 24; ++bi) { snprintf(hb, sizeof(hb), "%02x", key[bi]); kh += hb; }
                                std::cout << "  SEEKMISS idx=" << mIdx->name << " want leaf=" << kv.first
                                          << " got=" << leaf << " err=" << err << " key=" << kh << "\n";
                            }
                            bad++;
                        }
                    }
                }
                std::cout << "Seek " << mIdx->name << ": checked=" << checked << " misses=" << bad << "\n";
                totalBad += bad;
            }
        }
        mdb_free_tabledef(t);
        mdb_close(mdb);
        std::cout << (totalBad ? "SEEK BROKEN\n" : "SEEK OK\n");
        return totalBad ? 1 : 0;
    }

    if (args.contains("--verify-seeks")) {
        int idx = args.indexOf("--verify-seeks");
        if (idx + 1 >= args.size()) {
            std::cerr << "Error: --verify-seeks requires <mdb_path> [table_name]\n";
            return 1;
        }
        QString mdbPath = args[idx + 1];
        QString tableName = (idx + 2 < args.size() && !args[idx + 2].startsWith("--")) ? args[idx + 2] : "Transactions";
        QByteArray pb = QFile::encodeName(mdbPath);
        MdbHandle* mdb = mdb_open(pb.constData(), MDB_NOFLAGS);
        if (!mdb) { std::cerr << "Cannot open\n"; return 1; }
        MdbTableDef* t = mdb_read_table_by_name(mdb, const_cast<char*>(tableName.toUtf8().constData()), MDB_TABLE);
        if (!t) { std::cerr << "No table\n"; mdb_close(mdb); return 1; }
        mdb_read_columns(t);
        mdb_read_indices(t);
        int totalBad = 0;
        if (t->indices) {
            for (unsigned ii = 0; ii < t->indices->len; ++ii) {
                MdbIndex* mIdx = (MdbIndex*)g_ptr_array_index(t->indices, ii);
                std::vector<Jet4Writer::IndexEntryInfo> entries;
                Jet4Writer::Status st = Jet4Writer::BTreeEngine::walkIndex(mdb, t, mIdx, entries);
                if (!st.ok) { std::cout << "walk failed " << mIdx->name << ": " << st.error << "\n"; totalBad++; continue; }
                int bad = 0, checked = 0;
                for (auto& e : entries) {
                    std::vector<uint8_t> full = e.fullKey;
                    uint32_t pgRow = (e.dataPg << 8) | (e.rowIdx & 0xFF);
                    full.push_back((pgRow >> 24) & 0xFF); full.push_back((pgRow >> 16) & 0xFF);
                    full.push_back((pgRow >> 8) & 0xFF); full.push_back(pgRow & 0xFF);
                    std::vector<uint32_t> anc; std::string err;
                    uint32_t leaf = Jet4Writer::BTreeEngine::findLeaf(mdb, mIdx, full.data(), full.size(), anc, err);
                    checked++;
                    if (!leaf || leaf != e.leafPg) {
                        if (bad < 5) {
                            std::string kh; char hb[4];
                            for (size_t bi = 0; bi < full.size() && bi < 32; ++bi) { snprintf(hb, sizeof(hb), "%02x", full[bi]); kh += hb; }
                            std::cout << "  ROUTEMISS idx=" << mIdx->name << " want leaf=" << e.leafPg
                                      << " got=" << leaf << " dataPg=" << e.dataPg << " row=" << e.rowIdx
                                      << " key=" << kh << "\n";
                        }
                        bad++;
                    }
                }
                std::cout << "Route " << mIdx->name << ": checked=" << checked << " misses=" << bad << "\n";
                totalBad += bad;
            }
        }
        mdb_free_tabledef(t);
        mdb_close(mdb);
        std::cout << (totalBad ? "ROUTES BROKEN\n" : "ROUTES OK\n");
        return totalBad ? 1 : 0;
    }

    if (args.contains("--selftest-enc")) {
        int idx = args.indexOf("--selftest-enc");
        if (idx + 1 >= args.size()) {
            std::cerr << "Error: --selftest-enc requires <scratch_mdb_path> (/tmp/ only, never live)\n";
            return 1;
        }
        QString mdbPath = args[idx + 1];
        if (!mdbPath.startsWith("/tmp/")) { std::cerr << "Refusing: scratch path must be under /tmp/\n"; return 1; }
        QByteArray pb = QFile::encodeName(mdbPath);
        MdbHandle* mdb = mdb_open(pb.constData(), MDB_WRITABLE);
        if (!mdb) { std::cerr << "Cannot open scratch\n"; return 1; }
        MdbTableDef* t = mdb_read_table_by_name(mdb, (char*)"Transactions", MDB_TABLE);
        if (!t) { std::cerr << "No table\n"; mdb_close(mdb); return 1; }
        mdb_read_columns(t);
        mdb_read_indices(t);
        auto colByName = [&](const char* n) -> MdbColumn* {
            for (unsigned i = 0; i < t->num_cols; ++i) {
                MdbColumn* c = (MdbColumn*)g_ptr_array_index(t->columns, i);
                if (!strcasecmp(c->name, n)) return c;
            }
            return nullptr;
        };
        // Mirror the exporter's byte-exact text encoder (allowCompress per column).
        auto encFor = [&](const QString& s, const char* col) -> QByteArray {
            MdbColumn* c = colByName(col);
            bool allow = true;
            if (c) {
                // keep in sync with jetColCompress(): RAW list
                const char* rawCols[] = {"CurrentBalance","DrCr","InvoiceNo","TaxInvoiceNo","SalePurcAgainst",
                    "PlaceOfSupply","TempInv","ECommGSTIN","LtNo","BrokerName","FormVAT47No","ZimidarName",
                    "ExpUnit","E1Details1","E1Details2","Spare1","DrCrNoteMode","TDS194QChallanTransType",
                    "TaxType","VoucherType","TaxIncluding","TransportMode","EWayOthers","EInvTransType",
                    "ShipFrom_Stcd","ShipTo_Stcd","BuyerPINCode","BillItemName","OtherInfo","ShippingAddress",
                    "IRNNo","EInvStatus","EWayStatus","DispatchDate","PurchaseOrderNo","Grade","ChallanNo",
                    "TransporterGSTIN","Distance","TransportDocNo","TransportDocDt","BuyerLocation","ShipFrom_Nm",
                    "ShipFrom_Pin","ShipTo_Gstin","ShipTo_Loc","ShipTo_Others","ShippingPortCode","EInvAckNo",
                    "EInvAckDate","LedgerAlais","OpeningType","BankAccount","STATE","PartyTAN","PartyType",
                    "ConcernedPerson","PartyState","PartyPINcode","VATDealer","PartyStation","MobNoForSMS",
                    "SpecialPartyType","ShopNo","CommnCalcOn","Email","GSTIN","AadharNo","GSTPartyType",
                    "WhatsappNo","IFSCCode","BankName","BRN","URN","MyStation","MySTATE","Bank2", nullptr};
                for (int k = 0; rawCols[k]; ++k)
                    if (!strcasecmp(c->name, rawCols[k])) { allow = false; break; }
            }
            if (s.isEmpty()) return QByteArray();
            bool ascii = true;
            for (int i = 0; i < s.length(); ++i) if (s.at(i).unicode() > 0xFF) { ascii = false; break; }
            QByteArray ucs2;
            for (int i = 0; i < s.length(); ++i) { ushort u = s.at(i).unicode(); ucs2.append(char(u & 0xFF)); ucs2.append(char((u >> 8) & 0xFF)); }
            if (!allow || !ascii || ucs2.size() <= 4) return ucs2;
            QByteArray r; r.append(char(0xFF)); r.append(char(0xFE));
            for (int i = 0; i < s.length(); ++i) r.append(s.at(i).toLatin1());
            return r;
        };
        struct Case { QString narr; QString drCr; };
        std::vector<Case> cases = {
            {"A", "Dr"}, {"Dr", "Cr"}, {"Hello World, Test 123", "Dr"},
            {"धान खरीद 1509", "Cr"}, {"GST", "Dr"}, {"Self Purchase", "Cr"},
        };
        int pass = 0, fail = 0;
        for (size_t ci = 0; ci < cases.size(); ++ci) {
            std::vector<MdbField> f(t->num_cols);
            std::vector<QByteArray> hold(t->num_cols);
            for (unsigned i = 0; i < t->num_cols; ++i) { f[i].colnum = (int)i; f[i].is_null = 1; }
            auto ST = [&](int col, const QByteArray& b) {
                hold[col] = b; f[col].is_null = 0; f[col].value = (void*)hold[col].data(); f[col].siz = hold[col].size();
            };
            guint16 zero16 = 0; guint32 vch = 9900 + (guint32)ci, zero32 = 0;
            double amt = 100.0 + ci, oleD = 46000.0;
            f[0].is_null = 0; f[0].value = &zero16; f[0].siz = 2;
            f[1].is_null = 0; f[1].value = &vch; f[1].siz = 4;
            f[2].is_null = 0; f[2].value = &oleD; f[2].siz = 8;
            ST(3, encFor("Jrnl", "TransType"));
            guint16 ac = 44; f[4].is_null = 0; f[4].value = &ac; f[4].siz = 2;
            ST(5, encFor(cases[ci].drCr, "DrCr"));
            f[6].is_null = 0; f[6].value = &amt; f[6].siz = 8;
            ST(7, encFor("SELFTEST", "InvoiceNo"));
            ST(14, encFor(cases[ci].narr, "Narration"));
            if (!mdb_insert_row(t, t->num_cols, f.data())) { std::cout << "INSERT FAIL case " << ci << "\n"; fail++; continue; }
            // read back via bound-text fetch
            std::vector<char*> bound(t->num_cols);
            for (unsigned i = 0; i < t->num_cols; ++i) { bound[i] = (char*)malloc(8192); mdb_bind_column(t, i + 1, bound[i], nullptr); }
            bool found = false; QString gotN, gotD;
            mdb_rewind_table(t);
            while (mdb_fetch_row(t)) {
                MdbColumn* cN = colByName("Narration");
                MdbColumn* cV = colByName("VoucherNumber");
                // rescan: find our voucher by number through a fresh crack-free compare
                (void)cN; (void)cV;
                // locate via bound InvoiceNo+Vch
                for (unsigned i = 0; i < t->num_cols; ++i) {
                    MdbColumn* c = (MdbColumn*)g_ptr_array_index(t->columns, i);
                    if (!strcasecmp(c->name, "VoucherNumber") && QString::fromUtf8(bound[i]).trimmed() == QString::number(9900 + ci)) {
                        // confirm narration column in same row
                        for (unsigned j = 0; j < t->num_cols; ++j) {
                            MdbColumn* cj = (MdbColumn*)g_ptr_array_index(t->columns, j);
                            if (!strcasecmp(cj->name, "Narration")) gotN = QString::fromUtf8(bound[j]);
                            if (!strcasecmp(cj->name, "DrCr")) gotD = QString::fromUtf8(bound[j]);
                        }
                        found = true; break;
                    }
                }
                if (found) break;
            }
            for (unsigned i = 0; i < t->num_cols; ++i) free(bound[i]);
            bool ok = found && gotN == cases[ci].narr && gotD == cases[ci].drCr;
            std::cout << (ok ? "PASS" : "FAIL") << " case " << ci << " narr='" << cases[ci].narr.toStdString()
                      << "' got='" << gotN.toStdString() << "' drCr='" << gotD.toStdString() << "'\n";
            if (ok) pass++; else fail++;
        }
        // File must still verify clean afterwards (proves index + row integrity).
        Jet4Writer::VerifyReport rep = Jet4Writer::verifyDatabase(mdb);
        std::cout << "post-test verify: " << (rep.ok ? "ALL OK" : "FAILURES") << " badRows=" << rep.badRows << "\n";
        mdb_free_tabledef(t);
        mdb_close(mdb);
        std::cout << "enc selftest: pass=" << pass << " fail=" << fail << "\n";
        return (fail == 0 && rep.ok) ? 0 : 1;
    }

    if (args.contains("--dump-index")) {
        int idx = args.indexOf("--dump-index");
        if (idx + 3 >= args.size()) {
            std::cerr << "Error: --dump-index requires <mdb_path> <table> <index> [keyhex_prefix]\n";
            return 1;
        }
        QString mdbPath = args[idx + 1], tableName = args[idx + 2], indexName = args[idx + 3];
        QString pref = (idx + 4 < args.size() && !args[idx + 4].startsWith("--")) ? args[idx + 4] : "";
        QByteArray pb = QFile::encodeName(mdbPath);
        MdbHandle* mdb = mdb_open(pb.constData(), MDB_NOFLAGS);
        if (!mdb) { std::cerr << "Cannot open\n"; return 1; }
        MdbTableDef* t = mdb_read_table_by_name(mdb, const_cast<char*>(tableName.toUtf8().constData()), MDB_TABLE);
        if (!t) { std::cerr << "No table\n"; mdb_close(mdb); return 1; }
        mdb_read_columns(t);
        mdb_read_indices(t);
        MdbIndex* want = nullptr;
        if (t->indices) for (unsigned i = 0; i < t->indices->len; ++i) {
            MdbIndex* mIdx = (MdbIndex*)g_ptr_array_index(t->indices, i);
            if (!strcasecmp(mIdx->name, indexName.toUtf8().constData())) { want = mIdx; break; }
        }
        if (!want) { std::cerr << "No index\n"; mdb_free_tabledef(t); mdb_close(mdb); return 1; }
        std::vector<Jet4Writer::IndexEntryInfo> entries;
        Jet4Writer::Status st = Jet4Writer::BTreeEngine::walkIndex(mdb, t, want, entries);
        if (!st.ok) { std::cout << "walk failed: " << st.error << "\n"; return 1; }
        std::cout << "entries=" << entries.size() << "\n";
        for (auto& e : entries) {
            std::string hx;
            char hb[4];
            for (auto b : e.fullKey) { snprintf(hb, sizeof(hb), "%02x", b); hx += hb; }
            if (!pref.isEmpty() && QString::fromStdString(hx).startsWith(pref, Qt::CaseInsensitive)) {
                std::cout << "HIT leaf=" << e.leafPg << " dataPg=" << e.dataPg << " row=" << e.rowIdx << " key=" << hx << "\n";
                continue;
            }
            if (pref.isEmpty())
                std::cout << "leaf=" << e.leafPg << " dataPg=" << e.dataPg << " row=" << e.rowIdx << " key=" << hx << "\n";
        }
        // Descent check per entry is covered by --audit-seek; here also verify a
        // caller-supplied data location resolves: print dividers on the path is manual.
        mdb_free_tabledef(t);
        mdb_close(mdb);
        return 0;
    }

    if (args.contains("--repair-index")) {
        int idx = args.indexOf("--repair-index");
        if (idx + 1 >= args.size()) {
            std::cerr << "Error: --repair-index requires <mdb_path> [table_name]\n";
            return 1;
        }
        QString mdbPath = args[idx + 1];
        QString onlyTable = (idx + 2 < args.size() && !args[idx + 2].startsWith("--")) ? args[idx + 2] : "";
        QByteArray pb = QFile::encodeName(mdbPath);
        MdbHandle* mdb = mdb_open(pb.constData(), MDB_WRITABLE);
        if (!mdb) { std::cerr << "Cannot open writable\n"; return 1; }
        const char* tables[] = {"Transactions", "StockTransactions", "SaleTransportationDetail", "Ledgers", "Groups", nullptr};
        int totalFixed = 0;
        for (int ti = 0; tables[ti]; ++ti) {
            if (!onlyTable.isEmpty() && onlyTable.compare(tables[ti], Qt::CaseInsensitive) != 0) continue;
            MdbTableDef* t = mdb_read_table_by_name(mdb, const_cast<char*>(tables[ti]), MDB_TABLE);
            if (!t) continue;
            mdb_read_columns(t);
            mdb_read_indices(t);
            if (!t->indices) { mdb_free_tabledef(t); continue; }
            for (unsigned ii = 0; ii < t->indices->len; ++ii) {
                MdbIndex* mIdx = (MdbIndex*)g_ptr_array_index(t->indices, ii);
                if (!mIdx || mIdx->first_pg == 0 || mIdx->index_type == 2) continue;
                std::vector<Jet4Writer::IndexEntryInfo> entries;
                if (!Jet4Writer::BTreeEngine::walkIndex(mdb, t, mIdx, entries).ok) continue;
                std::set<uint32_t> covered;
                for (auto& e : entries) covered.insert((e.dataPg << 8) | (e.rowIdx & 0xFF));
                // Enumerate data pages fresh (tree may grow during repair).
                std::vector<uint32_t> dataPages;
                {
                    std::vector<char*> bound(t->num_cols);
                    for (unsigned i = 0; i < t->num_cols; ++i) { bound[i] = (char*)malloc(4096); mdb_bind_column(t, i + 1, bound[i], nullptr); }
                    mdb_rewind_table(t);
                    while (mdb_fetch_row(t)) {
                        if (dataPages.empty() || dataPages.back() != t->cur_phys_pg) dataPages.push_back(t->cur_phys_pg);
                    }
                    for (unsigned i = 0; i < t->num_cols; ++i) free(bound[i]);
                }
                std::vector<MdbField> fb(t->num_cols ? t->num_cols : 1);
                int rco = mdb->fmt->row_count_offset;
                for (uint32_t pg : dataPages) {
                    if (!Jet4Writer::UsageMapManager::readPage(mdb, pg)) continue;
                    if (mdb->pg_buf[0] != 0x01) continue;
                    int nrows = mdb_get_int16(mdb->pg_buf, rco);
                    if (nrows <= 0 || nrows > 1000) continue;
                    for (int s = 0; s < nrows; ++s) {
                        uint32_t k = (pg << 8) | ((uint32_t)s & 0xFF);
                        if (covered.count(k)) continue;
                        // updateIndex below pages other buffers in/out, so
                        // re-read our data page fresh for every slot.
                        if (!Jet4Writer::UsageMapManager::readPage(mdb, pg)) break;
                        int rs = 0; size_t rsz = 0;
                        if (mdb_find_row(mdb, s, &rs, &rsz) != 0 || rsz == 0) continue;
                        if (rs & 0x4000) continue; // chain-link/overflow slots (libmdb parity)
                        if (rs & 0x8000) continue; // flagged rows: pre-existing state, never fabricate coverage
                        if (mdb_crack_row(t, rs & 0x0FFF, rsz, fb.data()) < 0) continue;
                        std::vector<Jet4Writer::Field> fs;
                        for (unsigned i = 0; i < t->num_cols; ++i) {
                            Jet4Writer::Field f; f.colnum = (int)i; f.isNull = fb[i].is_null != 0;
                            f.data = (const uint8_t*)fb[i].value; f.size = fb[i].siz > 0 ? (size_t)fb[i].siz : 0;
                            if (f.isNull) { f.data = nullptr; f.size = 0; }
                            fs.push_back(f);
                        }
                        std::string err;
                        int w = Jet4Writer::updateIndex(t, mIdx, fs.data(), fs.size(), pg, (uint16_t)(s + 1), err);
                        if (w > 0) { covered.insert(k); totalFixed++; }
                        else if (std::getenv("JET4_DEBUG")) fprintf(stderr, "[repair] %s.%s pg=%u slot=%d failed: %s\n", tables[ti], mIdx->name, pg, s, err.c_str());
                    }
                }
                // Refresh in-memory + on-disk cardinality is handled by updateIndex.
                std::cout << "Repair " << tables[ti] << "." << mIdx->name << " done\n";
            }
            mdb_free_tabledef(t);
        }
        mdb_close(mdb);
        std::cout << "repair-index: fixed=" << totalFixed << "\n";
        return 0;
    }

    if (args.contains("--stress-index")) {
        int idx = args.indexOf("--stress-index");
        if (idx + 3 >= args.size()) {
            std::cerr << "Error: --stress-index requires <mdb_path> <table> <rows> [desc|asc|over]\n";
            return 1;
        }
        QString mdbPath = args[idx + 1], tableName = args[idx + 2];
        int nrows = args[idx + 3].toInt();
        QString mode = (idx + 4 < args.size()) ? args[idx + 4] : "over";
        QByteArray pb = QFile::encodeName(mdbPath);
        MdbHandle* mdb = mdb_open(pb.constData(), MDB_WRITABLE);
        if (!mdb) { std::cerr << "Cannot open writable\n"; return 1; }
        MdbTableDef* t = mdb_read_table_by_name(mdb, const_cast<char*>(tableName.toUtf8().constData()), MDB_TABLE);
        if (!t) { std::cerr << "No table\n"; mdb_close(mdb); return 1; }
        mdb_read_columns(t);
        mdb_read_indices(t);
        // Build order: over = pseudo-random pattern forcing deep cascades.
        std::vector<int> seq(nrows);
        for (int i = 0; i < nrows; ++i) seq[i] = i;
        unsigned seed = 12345;
        auto rnd = [&]() -> unsigned { seed = seed * 1103515245 + 12345; return (seed >> 16) & 0x7FFF; };
        if (mode == "desc") std::reverse(seq.begin(), seq.end());
        else if (mode == "over") { for (int i = nrows - 1; i > 0; --i) std::swap(seq[(size_t)i], seq[rnd() % (size_t)(i + 1)]); }
        // else asc: natural order
        auto colIdx = [&](const char* n) -> int {
            for (unsigned i = 0; i < t->num_cols; ++i) {
                MdbColumn* c = (MdbColumn*)g_ptr_array_index(t->columns, i);
                if (!strcasecmp(c->name, n)) return (int)i;
            }
            return -1;
        };
        int cItem = colIdx("ItemCode"), cDate = colIdx("VoucherDate"), cType = colIdx("TransType"),
            cVch = colIdx("VoucherNumber"), cRow = colIdx("RowNo");
        if (cItem < 0 || cDate < 0 || cType < 0 || cVch < 0 || cRow < 0) {
            std::cerr << "Table lacks stock key columns\n"; mdb_free_tabledef(t); mdb_close(mdb); return 1;
        }
        const char* types[] = {"Sale", "Purc", "Jrnl"};
        int done = 0, fail = 0;
        for (int v : seq) {
            std::vector<MdbField> f(t->num_cols);
            for (unsigned i = 0; i < t->num_cols; ++i) { f[i].colnum = (int)i; f[i].is_null = 1; }
            guint16 rowNo = 1, item = (guint16)(20000 + (v % 500));
            double oleD = 46000.0 + (v % 30);
            guint32 vn = (guint32)(90000 + v);
            QString tt = QString::fromLatin1(types[v % 3]);
            QByteArray ttB; ttB.append(char(0xFF)); ttB.append(char(0xFE));
            for (char ch : tt.toLatin1()) ttB.append(ch);
            f[(unsigned)cRow].is_null = 0; f[(unsigned)cRow].value = &rowNo; f[(unsigned)cRow].siz = 2;
            f[(unsigned)cItem].is_null = 0; f[(unsigned)cItem].value = &item; f[(unsigned)cItem].siz = 2;
            f[(unsigned)cDate].is_null = 0; f[(unsigned)cDate].value = &oleD; f[(unsigned)cDate].siz = 8;
            f[(unsigned)cType].is_null = 0; f[(unsigned)cType].value = (void*)ttB.data(); f[(unsigned)cType].siz = ttB.size();
            f[(unsigned)cVch].is_null = 0; f[(unsigned)cVch].value = &vn; f[(unsigned)cVch].siz = 4;
            if (mdb_insert_row(t, (int)t->num_cols, f.data())) done++; else fail++;
        }
        std::cout << "stress: done=" << done << " fail=" << fail << "\n";
        mdb_free_tabledef(t);
        mdb_close(mdb);
        return fail ? 1 : 0;
    }

    if (args.contains("--seek-key")) {
        int idx = args.indexOf("--seek-key");
        if (idx + 4 >= args.size()) {
            std::cerr << "Error: --seek-key requires <mdb> <table> <index> <dataPg> <slot>\n";
            return 1;
        }
        QString mdbPath = args[idx + 1], tableName = args[idx + 2], indexName = args[idx + 3];
        uint32_t pg = args[idx + 4].toUInt();
        int slot = (idx + 5 < args.size()) ? args[idx + 5].toInt() : 0;
        QByteArray pb = QFile::encodeName(mdbPath);
        MdbHandle* mdb = mdb_open(pb.constData(), MDB_NOFLAGS);
        if (!mdb) return 1;
        MdbTableDef* t = mdb_read_table_by_name(mdb, const_cast<char*>(tableName.toUtf8().constData()), MDB_TABLE);
        if (!t) return 1;
        mdb_read_columns(t);
        mdb_read_indices(t);
        MdbIndex* want = nullptr;
        if (t->indices) for (unsigned i = 0; i < t->indices->len; ++i) {
            MdbIndex* mIdx = (MdbIndex*)g_ptr_array_index(t->indices, i);
            if (!strcasecmp(mIdx->name, indexName.toUtf8().constData())) { want = mIdx; break; }
        }
        if (!want) return 1;
        if (!Jet4Writer::UsageMapManager::readPage(mdb, pg)) return 1;
        int rs = 0; size_t rsz = 0;
        if (mdb_find_row(mdb, slot, &rs, &rsz) != 0) return 1;
        std::vector<MdbField> fb(t->num_cols);
        if (mdb_crack_row(t, rs & 0x0FFF, rsz, fb.data()) < 0) return 1;
        std::vector<Jet4Writer::Field> fs;
        for (unsigned i = 0; i < t->num_cols; ++i) {
            Jet4Writer::Field f; f.colnum = (int)i; f.isNull = fb[i].is_null != 0;
            f.data = (const uint8_t*)fb[i].value; f.size = fb[i].siz > 0 ? (size_t)fb[i].siz : 0;
            if (f.isNull) { f.data = nullptr; f.size = 0; }
            fs.push_back(f);
        }
        std::vector<uint8_t> key;
        if (!Jet4Writer::BTreeEngine::buildEntry(t, want, fs.data(), fs.size(), pg, (uint16_t)(slot + 1), key).ok) return 1;
        std::vector<uint32_t> anc; std::string err;
        uint32_t leaf = Jet4Writer::BTreeEngine::findLeaf(mdb, want, key.data(), key.size(), anc, err);
        if (!leaf) { std::cout << "DESCENT FAILED: " << err << "\n"; return 1; }
        if (!Jet4Writer::UsageMapManager::readPage(mdb, leaf)) return 1;
        Jet4Writer::DecodedPage dp;
        if (!Jet4Writer::BTreeEngine::decodeEntries(mdb, dp, err)) return 1;
        bool found = false;
        for (auto& e : dp.full) if (e == key) { found = true; break; }
        std::cout << "seek -> leaf " << leaf << (found ? " HIT" : " MISS") << " depth=" << anc.size() << "\n";
        mdb_free_tabledef(t);
        mdb_close(mdb);
        return found ? 0 : 1;
    }

    if (args.contains("--audit-dividers")) {
        int idx = args.indexOf("--audit-dividers");
        if (idx + 1 >= args.size()) {
            std::cerr << "Error: --audit-dividers requires <mdb_path> [table_name]\n";
            return 1;
        }
        QString mdbPath = args[idx + 1];
        QString onlyTable = (idx + 2 < args.size() && !args[idx + 2].startsWith("--")) ? args[idx + 2] : "";
        QByteArray pb = QFile::encodeName(mdbPath);
        MdbHandle* mdb = mdb_open(pb.constData(), MDB_NOFLAGS);
        if (!mdb) { std::cerr << "Cannot open\n"; return 1; }
        const char* tables[] = {"Transactions", "StockTransactions", "SaleTransportationDetail", "Ledgers", "Groups", nullptr};
        int totalStale = 0;
        for (int ti = 0; tables[ti]; ++ti) {
            if (!onlyTable.isEmpty() && onlyTable.compare(tables[ti], Qt::CaseInsensitive) != 0) continue;
            MdbTableDef* t = mdb_read_table_by_name(mdb, const_cast<char*>(tables[ti]), MDB_TABLE);
            if (!t) continue;
            mdb_read_columns(t);
            mdb_read_indices(t);
            if (!t->indices) { mdb_free_tabledef(t); continue; }
            for (unsigned ii = 0; ii < t->indices->len; ++ii) {
                MdbIndex* mIdx = (MdbIndex*)g_ptr_array_index(t->indices, ii);
                if (!mIdx || mIdx->first_pg == 0 || mIdx->index_type == 2) continue;
                // BFS from root over dividers + tails (live-reachable only).
                std::vector<uint32_t> stack;
                stack.push_back(mIdx->first_pg);
                std::set<uint32_t> seen;
                int checked = 0, stale = 0;
                while (!stack.empty()) {
                    uint32_t cp = stack.back(); stack.pop_back();
                    if (!seen.insert(cp).second) continue;
                    if (!Jet4Writer::UsageMapManager::readPage(mdb, cp)) {
                        std::cout << "  UNREADABLE node=" << cp << "\n"; stale++; continue;
                    }
                    const auto* cb2 = static_cast<const uint8_t*>(mdb->pg_buf);
                    if (cb2[0] != 0x03) continue; // leaves checked via parents
                    Jet4Writer::DecodedPage cdp; std::string derr;
                    if (!Jet4Writer::BTreeEngine::decodeEntries(mdb, cdp, derr)) {
                        std::cout << "  UNDECODABLE node=" << cp << " " << derr << "\n"; stale++; continue;
                    }
                    uint32_t tail = Jet4Writer::UsageMapManager::getU32(mdb, Jet4Writer::kOffChildTailPg);
                    for (auto& div : cdp.full) {
                        if (div.size() < 8) { std::cout << "  SHORTDIV node=" << cp << "\n"; stale++; continue; }
                        uint32_t ch = Jet4Writer::BTreeEngine::childOf(div);
                        std::vector<uint8_t> wantDiv(div.begin(), div.end() - 4);
                        if (!Jet4Writer::UsageMapManager::readPage(mdb, ch)) {
                            std::cout << "  UNREADABLE child=" << ch << " of node=" << cp << "\n"; stale++; continue;
                        }
                        const auto* hb = static_cast<const uint8_t*>(mdb->pg_buf);
                        bool chLeaf = (hb[0] == 0x04);
                        if (!chLeaf && hb[0] != 0x03) {
                            std::cout << "  NOTINDEX node=" << cp << " child=" << ch << " type=" << (int)hb[0] << "\n"; stale++; continue;
                        }
                        Jet4Writer::DecodedPage chd; std::string cerr2;
                        if (!Jet4Writer::BTreeEngine::decodeEntries(mdb, chd, cerr2) || chd.full.empty()) {
                            std::cout << "  UNDECODABLE child=" << ch << " of node=" << cp << "\n"; stale++; continue;
                        }
                        std::vector<uint8_t> realMax = chLeaf
                            ? chd.full.back()
                            : std::vector<uint8_t>(chd.full.back().begin(), chd.full.back().end() - 4);
                        ++checked;
                        if (realMax != wantDiv) {
                            std::string dh, rh; char tmp[4];
                            for (auto b : wantDiv) { snprintf(tmp, sizeof(tmp), "%02x", b); dh += tmp; }
                            for (auto b : realMax) { snprintf(tmp, sizeof(tmp), "%02x", b); rh += tmp; }
                            std::cout << "  STALE node=" << cp << " child=" << ch << (chLeaf ? "(leaf)" : "(node)")
                                      << " divmax=" << dh << " realmax=" << rh << "\n";
                            stale++;
                        }
                        stack.push_back(ch);
                    }
                    if (tail) stack.push_back(tail);
                }
                std::cout << "DivAudit " << tables[ti] << "." << mIdx->name << ": checked=" << checked << " stale=" << stale << "\n";
                totalStale += stale;
            }
            mdb_free_tabledef(t);
        }
        mdb_close(mdb);
        std::cout << (totalStale ? "DIVIDERS STALE\n" : "DIVIDERS OK\n");
        return totalStale ? 1 : 0;
    }

    if (args.contains("--fix-levels")) {
        int idx = args.indexOf("--fix-levels");
        if (idx + 1 >= args.size()) {
            std::cerr << "Error: --fix-levels requires <mdb_path> [table_name]\n";
            return 1;
        }
        QString mdbPath = args[idx + 1];
        QString onlyTable = (idx + 2 < args.size() && !args[idx + 2].startsWith("--")) ? args[idx + 2] : "";
        QByteArray pb = QFile::encodeName(mdbPath);
        MdbHandle* mdb = mdb_open(pb.constData(), MDB_WRITABLE);
        if (!mdb) { std::cerr << "Cannot open writable\n"; return 1; }
        const char* tables[] = {"Transactions", "StockTransactions", "SaleTransportationDetail", "Ledgers", "Groups", nullptr};
        int totalFixed = 0;
        for (int ti = 0; tables[ti]; ++ti) {
            if (!onlyTable.isEmpty() && onlyTable.compare(tables[ti], Qt::CaseInsensitive) != 0) continue;
            MdbTableDef* t = mdb_read_table_by_name(mdb, const_cast<char*>(tables[ti]), MDB_TABLE);
            if (!t) continue;
            mdb_read_columns(t);
            mdb_read_indices(t);
            if (!t->indices) { mdb_free_tabledef(t); continue; }
            for (unsigned ii = 0; ii < t->indices->len; ++ii) {
                MdbIndex* mIdx = (MdbIndex*)g_ptr_array_index(t->indices, ii);
                if (!mIdx || mIdx->first_pg == 0 || mIdx->index_type == 2) continue;
                // Collect reachable pages (BFS over dividers + tails).
                std::vector<uint32_t> stack;
                stack.push_back(mIdx->first_pg);
                std::set<uint32_t> seen;
                std::vector<uint32_t> order;
                while (!stack.empty()) {
                    uint32_t cp = stack.back(); stack.pop_back();
                    if (!seen.insert(cp).second) continue;
                    order.push_back(cp);
                    if (!Jet4Writer::UsageMapManager::readPage(mdb, cp)) continue;
                    const auto* cb2 = static_cast<const uint8_t*>(mdb->pg_buf);
                    if (cb2[0] != 0x03) continue;
                    Jet4Writer::DecodedPage cdp; std::string derr;
                    if (!Jet4Writer::BTreeEngine::decodeEntries(mdb, cdp, derr)) continue;
                    for (auto& div : cdp.full) {
                        if (div.size() >= 4) stack.push_back(Jet4Writer::BTreeEngine::childOf(div));
                    }
                    uint32_t tail = Jet4Writer::UsageMapManager::getU32(mdb, Jet4Writer::kOffChildTailPg);
                    if (tail) stack.push_back(tail);
                }
                // Bottom-up fixpoint: leaf=0, node=1+max(children).
                std::map<uint32_t, int> lvl;
                for (int pass = 0; pass < 32; ++pass) {
                    bool changed = false;
                    for (uint32_t cp : order) {
                        if (lvl.count(cp)) continue;
                        if (!Jet4Writer::UsageMapManager::readPage(mdb, cp)) continue;
                        const auto* cb2 = static_cast<const uint8_t*>(mdb->pg_buf);
                        if (cb2[0] == 0x04) { lvl[cp] = 0; changed = true; continue; }
                        if (cb2[0] != 0x03) continue;
                        Jet4Writer::DecodedPage cdp; std::string derr;
                        if (!Jet4Writer::BTreeEngine::decodeEntries(mdb, cdp, derr)) continue;
                        int mx = -1; bool ok = true;
                        for (auto& div : cdp.full) {
                            if (div.size() < 4) { ok = false; break; }
                            uint32_t ch = Jet4Writer::BTreeEngine::childOf(div);
                            auto it = lvl.find(ch);
                            if (it == lvl.end()) { ok = false; break; }
                            mx = std::max(mx, it->second);
                        }
                        if (ok) {
                            uint32_t tail = Jet4Writer::UsageMapManager::getU32(mdb, Jet4Writer::kOffChildTailPg);
                            if (tail) {
                                auto it = lvl.find(tail);
                                if (it == lvl.end()) ok = false;
                                else mx = std::max(mx, it->second);
                            }
                        }
                        if (ok && mx >= 0) { lvl[cp] = mx + 1; changed = true; }
                    }
                    if (!changed) break;
                }
                for (uint32_t cp : order) {
                    auto it = lvl.find(cp);
                    if (it == lvl.end() || it->second <= 0) continue;
                    if (!Jet4Writer::UsageMapManager::readPage(mdb, cp)) continue;
                    auto* b = static_cast<uint8_t*>(mdb->pg_buf);
                    if (b[0] != 0x03) continue;
                    if (b[Jet4Writer::kOffPrefixUnknown] != (uint8_t)it->second) {
                        b[Jet4Writer::kOffPrefixUnknown] = (uint8_t)it->second;
                        if (Jet4Writer::UsageMapManager::writePage(mdb, cp)) {
                            std::cout << "  fixed level node=" << cp << " -> " << it->second << "\n";
                            totalFixed++;
                        }
                    }
                }
            }
            mdb_free_tabledef(t);
        }
        mdb_close(mdb);
        std::cout << "fix-levels: fixed=" << totalFixed << "\n";
        return 0;
    }

    if (args.contains("--audit-maps")) {
        int idx = args.indexOf("--audit-maps");
        if (idx + 1 >= args.size()) {
            std::cerr << "Error: --audit-maps requires <mdb_path> [table_name]\n";
            return 1;
        }
        QString mdbPath = args[idx + 1];
        QString onlyTable = (idx + 2 < args.size() && !args[idx + 2].startsWith("--")) ? args[idx + 2] : "";
        QByteArray pb = QFile::encodeName(mdbPath);
        MdbHandle* mdb = mdb_open(pb.constData(), MDB_NOFLAGS);
        if (!mdb) { std::cerr << "Cannot open\n"; return 1; }
        fseeko(mdb->f->stream, 0, SEEK_END);
        int npages = (int)(ftello(mdb->f->stream) / mdb->fmt->pg_size);
        const char* tables[] = {"Transactions", "StockTransactions", "SaleTransportationDetail", "Ledgers", "Groups", "CompanyInfo", nullptr};
        int totalBad = 0;
        for (int ti = 0; tables[ti]; ++ti) {
            if (!onlyTable.isEmpty() && onlyTable.compare(tables[ti], Qt::CaseInsensitive) != 0) continue;
            MdbTableDef* t = mdb_read_table_by_name(mdb, const_cast<char*>(tables[ti]), MDB_TABLE);
            if (!t) continue;
            mdb_read_columns(t);
            int tpg = t->entry ? t->entry->table_pg : 0;
            // Strict map walk (no brute-force fallback): pages a map-following
            // reader (MS Jet / Jackcess TableScanCursor) can ever visit.
            std::set<uint32_t> viaMap;
            if (t->usage_map && t->map_sz >= 5) {
                uint32_t cur = 0;
                for (int guard = 0; guard < npages + 10; ++guard) {
                    gint32 nxt = mdb_map_find_next(mdb, t->usage_map, (unsigned)t->map_sz, cur);
                    if (nxt < 0) { std::cout << "  map type unsupported\n"; break; }
                    if (!nxt || (uint32_t)nxt == cur) break;
                    cur = (uint32_t)nxt;
                    if (!mdb_read_pg(mdb, cur)) break;
                    if (mdb->pg_buf[0] == 0x01 && mdb_get_int32(mdb->pg_buf, 4) == tpg) viaMap.insert(cur);
                }
            }
            // Brute-force walk: every real data page of this table.
            std::set<uint32_t> brute;
            for (int p = 0; p < npages; ++p) {
                if (!mdb_read_pg(mdb, (unsigned)p)) continue;
                if (mdb->pg_buf[0] == 0x01 && mdb_get_int32(mdb->pg_buf, 4) == tpg) brute.insert((uint32_t)p);
            }
            std::vector<uint32_t> invisible;
            for (uint32_t p : brute) if (!viaMap.count(p)) invisible.push_back(p);
            long invisRows = 0;
            for (uint32_t p : invisible) {
                if (!mdb_read_pg(mdb, p)) continue;
                int nr = mdb_get_int16(mdb->pg_buf, mdb->fmt->row_count_offset);
                if (nr > 0 && nr < 1000) invisRows += nr;
            }
            std::cout << "MapAudit " << tables[ti] << ": brutePages=" << brute.size()
                      << " mapPages=" << viaMap.size() << " invisiblePages=" << invisible.size()
                      << " invisibleRows~" << invisRows << "\n";
            for (size_t i = 0; i < invisible.size() && i < 10; ++i)
                std::cout << "  INVISIBLE data page=" << invisible[i] << "\n";
            if (!invisible.empty()) totalBad++;
            mdb_free_tabledef(t);
        }
        mdb_close(mdb);
        std::cout << (totalBad ? "MAPS BROKEN\n" : "MAPS OK\n");
        return totalBad ? 1 : 0;
    }

    if (args.contains("--resort-leaf")) {
        int idx = args.indexOf("--resort-leaf");
        if (idx + 3 >= args.size()) {
            std::cerr << "Error: --resort-leaf requires <mdb_path> <table_name> <index_name> <leaf_pg>\n";
            return 1;
        }
        QString mdbPath = args[idx + 1], tableName = args[idx + 2], indexName = args[idx + 3];
        uint32_t leafPg = args[idx + 4].toUInt();
        QByteArray pb = QFile::encodeName(mdbPath);
        MdbHandle* mdb = mdb_open(pb.constData(), MDB_WRITABLE);
        if (!mdb) { std::cerr << "Cannot open writable\n"; return 1; }
        MdbTableDef* t = mdb_read_table_by_name(mdb, const_cast<char*>(tableName.toUtf8().constData()), MDB_TABLE);
        if (!t) { std::cerr << "No table\n"; mdb_close(mdb); return 1; }
        mdb_read_columns(t);
        mdb_read_indices(t);
        MdbIndex* want = nullptr;
        if (t->indices) for (unsigned i = 0; i < t->indices->len; ++i) {
            MdbIndex* mIdx = (MdbIndex*)g_ptr_array_index(t->indices, i);
            if (!strcasecmp(mIdx->name, indexName.toUtf8().constData())) { want = mIdx; break; }
        }
        if (!want) { std::cerr << "No index\n"; mdb_free_tabledef(t); mdb_close(mdb); return 1; }
        if (!Jet4Writer::UsageMapManager::readPage(mdb, leafPg)) { std::cerr << "No page\n"; return 1; }
        {
            const auto* b0 = static_cast<const uint8_t*>(mdb->pg_buf);
            if (b0[0] != Jet4Writer::kPageLeaf) { std::cerr << "Not a leaf\n"; return 1; }
        }
        Jet4Writer::DecodedPage dp; std::string derr;
        if (!Jet4Writer::BTreeEngine::decodeEntries(mdb, dp, derr)) { std::cerr << "Decode fail\n"; return 1; }
        std::sort(dp.full.begin(), dp.full.end(), [](const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
            return Jet4Writer::BTreeEngine::cmpIndexOrder(a, b, true) < 0;
        });
        // Descend for the new max to get a valid ancestor path, then rewrite
        // (insertIntoPage propagates divider updates / splits as needed).
        std::vector<uint32_t> anc; std::string err;
        uint32_t got = Jet4Writer::BTreeEngine::findLeaf(mdb, want, dp.full.back().data(), dp.full.back().size(), anc, err);
        if (!got) { std::cerr << "Descent failed: " << err << "\n"; return 1; }
        Jet4Writer::Status st = Jet4Writer::BTreeEngine::insertIntoPage(mdb, t, want, leafPg, true, dp.full, anc);
        if (!st.ok) { std::cerr << "Rewrite failed: " << st.error << "\n"; return 1; }
        mdb_free_tabledef(t);
        mdb_close(mdb);
        std::cout << "resort-leaf: page " << leafPg << " sorted (" << dp.full.size() << " entries)\n";
        return 0;
    }

    if (args.contains("--restore-group-codes")) {
        int idx = args.indexOf("--restore-group-codes");
        if (idx + 1 >= args.size()) {
            std::cerr << "Error: --restore-group-codes requires <mdb_path>\n";
            return 1;
        }
        QString mdbPath = args[idx + 1];
        QByteArray pb = QFile::encodeName(mdbPath);
        MdbHandle* mdb = mdb_open(pb.constData(), MDB_WRITABLE);
        if (!mdb) { std::cerr << "Cannot open writable\n"; return 1; }
        MdbTableDef* grpTbl = mdb_read_table_by_name(mdb, (char*)"Groups", MDB_TABLE);
        if (!grpTbl) { std::cerr << "No table\n"; mdb_close(mdb); return 1; }
        mdb_read_columns(grpTbl);
        // Native Bahi-Khata shape: these custom groups are TOP-LEVEL
        // (Code2nd=Code3rd=Code4th=0). The exporter once nested them under
        // sqlite's hierarchy, which drops them from Bahi-Khata's balance
        // sheet grand total (exact gap math). Restore native shape in place
        // (same-size u16 writes; index entries still carry the native codes,
        // so no index repair is needed).
        static const int kTop[] = {36, 37, 38, 39, 41, 43, 47};
        MdbColumn* colC1 = (MdbColumn*)g_ptr_array_index(grpTbl->columns, 1);
        MdbColumn* colC2 = (MdbColumn*)g_ptr_array_index(grpTbl->columns, 2);
        MdbColumn* colC3 = (MdbColumn*)g_ptr_array_index(grpTbl->columns, 3);
        MdbColumn* colC4 = (MdbColumn*)g_ptr_array_index(grpTbl->columns, 4);
        guint16 zero = 0;
        int fixed = 0;
        mdb_rewind_table(grpTbl);
        while (mdb_fetch_row(grpTbl)) {
            char* cStr = mdb_col_to_string(mdb, mdb->pg_buf, colC1->cur_value_start, colC1->col_type, colC1->cur_value_len);
            if (!cStr) continue;
            int c1 = atoi(cStr);
            g_free(cStr);
            bool want = false;
            for (int k : kTop) if (k == c1) { want = true; break; }
            if (!want) continue;
            bool modified = false;
            MdbColumn* cols[3] = {colC2, colC3, colC4};
            for (int ci = 0; ci < 3; ++ci) {
                MdbColumn* cc = cols[ci];
                if (cc && cc->cur_value_start > 0 && cc->cur_value_len == (int)sizeof(guint16)) {
                    if (memcmp(mdb->pg_buf + cc->cur_value_start, &zero, sizeof(guint16)) != 0) {
                        memcpy(mdb->pg_buf + cc->cur_value_start, &zero, sizeof(guint16));
                        modified = true;
                    }
                }
            }
            if (modified) {
                if (Jet4Writer::UsageMapManager::writePage(mdb, grpTbl->cur_phys_pg)) fixed++;
            }
        }
        mdb_free_tabledef(grpTbl);
        mdb_close(mdb);
        std::cout << "restore-group-codes: fixed=" << fixed << "\n";
        return 0;
    }

    if (args.contains("--find-key")) {
        int idx = args.indexOf("--find-key");
        if (idx + 5 >= args.size()) {
            std::cerr << "Error: --find-key requires <mdb> <table> <index> <dataPg> <slot>\n";
            return 1;
        }
        QString mdbPath = args[idx + 1], tableName = args[idx + 2], indexName = args[idx + 3];
        uint32_t pg = args[idx + 4].toUInt();
        int slot = args[idx + 5].toInt();
        QByteArray pb = QFile::encodeName(mdbPath);
        MdbHandle* mdb = mdb_open(pb.constData(), MDB_NOFLAGS);
        if (!mdb) return 1;
        MdbTableDef* t = mdb_read_table_by_name(mdb, const_cast<char*>(tableName.toUtf8().constData()), MDB_TABLE);
        if (!t) return 1;
        mdb_read_columns(t);
        mdb_read_indices(t);
        MdbIndex* want = nullptr;
        if (t->indices) for (unsigned i = 0; i < t->indices->len; ++i) {
            MdbIndex* mIdx = (MdbIndex*)g_ptr_array_index(t->indices, i);
            if (!strcasecmp(mIdx->name, indexName.toUtf8().constData())) { want = mIdx; break; }
        }
        if (!want || !Jet4Writer::UsageMapManager::readPage(mdb, pg)) return 1;
        int rs = 0; size_t rsz = 0;
        if (mdb_find_row(mdb, slot, &rs, &rsz) != 0) return 1;
        std::vector<MdbField> fb(t->num_cols);
        if (mdb_crack_row(t, rs & 0x0FFF, rsz, fb.data()) < 0) return 1;
        std::vector<Jet4Writer::Field> fs;
        for (unsigned i = 0; i < t->num_cols; ++i) {
            Jet4Writer::Field f; f.colnum = (int)i; f.isNull = fb[i].is_null != 0;
            f.data = (const uint8_t*)fb[i].value; f.size = fb[i].siz > 0 ? (size_t)fb[i].siz : 0;
            if (f.isNull) { f.data = nullptr; f.size = 0; }
            fs.push_back(f);
        }
        std::vector<uint8_t> key;
        if (!Jet4Writer::BTreeEngine::buildEntry(t, want, fs.data(), fs.size(), pg, (uint16_t)(slot + 1), key).ok) return 1;
        fseeko(mdb->f->stream, 0, SEEK_END);
        int npg = (int)(ftello(mdb->f->stream) / mdb->fmt->pg_size);
        int hits = 0;
        for (int p = 0; p < npg; ++p) {
            if (!mdb_read_pg(mdb, (unsigned)p)) continue;
            unsigned char* b = (unsigned char*)mdb->pg_buf;
            if (b[0] != 0x04) continue;
            Jet4Writer::DecodedPage dp; std::string err;
            if (!Jet4Writer::BTreeEngine::decodeEntries(mdb, dp, err)) continue;
            for (auto& e : dp.full) {
                if (e == key) { std::cout << "HIT leaf pg=" << p << "\n"; hits++; }
            }
        }
        std::cout << "total hits=" << hits << " keylen=" << key.size() << "\n";
        mdb_free_tabledef(t);
        mdb_close(mdb);
        return 0;
    }

    if (args.contains("--chain-audit")) {
        int idx = args.indexOf("--chain-audit");
        if (idx + 2 >= args.size()) {
            std::cerr << "Error: --chain-audit requires <mdb_path> <table>\n";
            return 1;
        }
        QString mdbPath = args[idx + 1], tableName = args[idx + 2];
        QByteArray pb = QFile::encodeName(mdbPath);
        MdbHandle* mdb = mdb_open(pb.constData(), MDB_NOFLAGS);
        if (!mdb) return 1;
        MdbTableDef* t = mdb_read_table_by_name(mdb, const_cast<char*>(tableName.toUtf8().constData()), MDB_TABLE);
        if (!t) return 1;
        mdb_read_columns(t);
        mdb_read_indices(t);
        // Collect leaf pages per index by walking each index's chain from its
        // leftmost, plus find ALL index leaves to spot off-chain pages.
        fseeko(mdb->f->stream, 0, SEEK_END);
        int npg = (int)(ftello(mdb->f->stream) / mdb->fmt->pg_size);
        unsigned tdef = t->entry->table_pg;
        std::set<unsigned> allLeaves;
        for (int pg = 0; pg < npg; ++pg) {
            if (!mdb_read_pg(mdb, (unsigned)pg)) continue;
            unsigned char* b = (unsigned char*)mdb->pg_buf;
            if (b[0] != 0x04) continue;
            if ((unsigned)mdb_get_int32(mdb->pg_buf, 4) != tdef) continue;
            allLeaves.insert((unsigned)pg);
        }
        std::cout << "total index-leaf pages for table: " << allLeaves.size() << "\n";
        std::set<unsigned> everWalked;
        if (t->indices) for (unsigned ii = 0; ii < t->indices->len; ++ii) {
            MdbIndex* mIdx = (MdbIndex*)g_ptr_array_index(t->indices, ii);
            std::vector<Jet4Writer::IndexEntryInfo> entries;
            if (!Jet4Writer::BTreeEngine::walkIndex(mdb, t, mIdx, entries).ok) {
                std::cout << mIdx->name << ": WALK FAILED\n"; continue;
            }
            std::set<unsigned> walked;
            for (auto& e : entries) { walked.insert(e.leafPg); everWalked.insert(e.leafPg); }
            // Descend to leftmost the way walkIndex does, then follow chain manually to show segments.
            std::cout << mIdx->name << ": walkedLeaves=" << walked.size() << " entries=" << entries.size() << "\n";
        }
        std::cout << "never-walked leaf pages:";
        for (unsigned pg : allLeaves) {
            // membership test without printing all
            (void)pg;
        }
        {
            // recompute: pages in allLeaves but not everWalked
            bool first = true;
            for (unsigned pg : allLeaves) {
                if (everWalked.find(pg) == everWalked.end()) {
                    mdb_read_pg(mdb, pg);
                    unsigned pr = (unsigned)mdb_get_int32(mdb->pg_buf, 12);
                    unsigned nx = (unsigned)mdb_get_int32(mdb->pg_buf, 16);
                    std::cout << (first ? " " : ", ") << pg << "(p" << pr << ">n" << nx << ")";
                    first = false;
                }
            }
            std::cout << "\n";
        }
        // For each leaf, show prev/next and whether prev/next are leaves of same table.
        for (unsigned pg : allLeaves) {
            mdb_read_pg(mdb, pg);
            unsigned pr = (unsigned)mdb_get_int32(mdb->pg_buf, 12);
            unsigned nx = (unsigned)mdb_get_int32(mdb->pg_buf, 16);
            bool prLeaf = allLeaves.count(pr) > 0, nxLeaf = allLeaves.count(nx) > 0;
            if (pr != 0 && !prLeaf) std::cout << "  pg " << pg << " prev->" << pr << " (NON-LEAF/MISSING)" << "\n";
            if (nx != 0 && !nxLeaf) std::cout << "  pg " << pg << " next->" << nx << " (NON-LEAF/MISSING)" << "\n";
        }
        mdb_free_tabledef(t);
        mdb_close(mdb);
        return 0;
    }

    if (args.contains("--find-inversion")) {
        int idx = args.indexOf("--find-inversion");
        if (idx + 2 >= args.size()) {
            std::cerr << "Error: --find-inversion requires <mdb> <table> <index>\n";
            return 1;
        }
        QString mdbPath = args[idx + 1], tableName = args[idx + 2], indexName = args[idx + 3];
        QByteArray pb = QFile::encodeName(mdbPath);
        MdbHandle* mdb = mdb_open(pb.constData(), MDB_NOFLAGS);
        if (!mdb) return 1;
        MdbTableDef* t = mdb_read_table_by_name(mdb, const_cast<char*>(tableName.toUtf8().constData()), MDB_TABLE);
        if (!t) return 1;
        mdb_read_columns(t);
        mdb_read_indices(t);
        MdbIndex* want = nullptr;
        if (t->indices) for (unsigned i = 0; i < t->indices->len; ++i) {
            MdbIndex* mIdx = (MdbIndex*)g_ptr_array_index(t->indices, i);
            if (!strcasecmp(mIdx->name, indexName.toUtf8().constData())) { want = mIdx; break; }
        }
        if (!want) return 1;
        std::vector<Jet4Writer::IndexEntryInfo> entries;
        if (!Jet4Writer::BTreeEngine::walkIndex(mdb, t, want, entries).ok) return 1;
        int inv = 0;
        for (size_t i = 1; i < entries.size() && inv < 5; ++i) {
            const auto& A = entries[i - 1];
            const auto& B = entries[i];
            size_t m = std::min(A.fullKey.size(), B.fullKey.size());
            int c = m ? memcmp(A.fullKey.data(), B.fullKey.data(), m) : 0;
            if (c == 0) c = (A.fullKey.size() < B.fullKey.size()) ? -1 : (A.fullKey.size() > B.fullKey.size() ? 1 : 0);
            if (c == 0 && A.dataPg != B.dataPg) c = (A.dataPg < B.dataPg) ? -1 : 1;
            if (c == 0 && A.rowIdx != B.rowIdx) c = (A.rowIdx < B.rowIdx) ? -1 : 1;
            if (c > 0) {
                auto hx = [](const std::vector<uint8_t>& v) {
                    std::string s; char hb[4];
                    for (auto x : v) { snprintf(hb, sizeof(hb), "%02x", x); s += hb; }
                    return s;
                };
                std::cout << "INVERSION at walk pos " << i << " leaf " << entries[i-1].leafPg
                          << " -> leaf " << entries[i].leafPg << "\n  A=" << hx(A.fullKey) << "\n  B=" << hx(B.fullKey) << "\n";
                inv++;
            }
        }
        std::cout << "inversions=" << inv << "\n";
        mdb_free_tabledef(t);
        mdb_close(mdb);
        return 0;
    }

    if (args.contains("--rechain-index")) {
        int idx = args.indexOf("--rechain-index");
        if (idx + 2 >= args.size()) {
            std::cerr << "Error: --rechain-index requires <mdb_path> <table> [index]\n";
            return 1;
        }
        QString mdbPath = args[idx + 1], tableName = args[idx + 2];
        QString onlyIndex = (idx + 3 < args.size() && !args[idx + 3].startsWith("--")) ? args[idx + 3] : "";
        QByteArray pb = QFile::encodeName(mdbPath);
        MdbHandle* mdb = mdb_open(pb.constData(), MDB_WRITABLE);
        if (!mdb) { std::cerr << "Cannot open writable\n"; return 1; }
        MdbTableDef* t = mdb_read_table_by_name(mdb, const_cast<char*>(tableName.toUtf8().constData()), MDB_TABLE);
        if (!t) { std::cerr << "No table\n"; mdb_close(mdb); return 1; }
        mdb_read_columns(t);
        mdb_read_indices(t);
        // In-order leaf collection via dividers (authoritative tree order),
        // then relink prev/next along it. Heals chain gaps/side-chains.
        std::function<bool(uint32_t, std::vector<uint32_t>&, std::set<uint32_t>&, int)> collect =
            [&](uint32_t pg, std::vector<uint32_t>& out, std::set<uint32_t>& seen, int depth) -> bool {
            if (depth > 32 || pg == 0 || seen.count(pg)) return !seen.count(pg);
            seen.insert(pg);
            if (!Jet4Writer::UsageMapManager::readPage(mdb, pg)) return false;
            unsigned char* b = (unsigned char*)mdb->pg_buf;
            if (b[0] == 0x04) { out.push_back(pg); return true; }
            if (b[0] != 0x03) return false;
            Jet4Writer::DecodedPage dp; std::string err;
            if (!Jet4Writer::BTreeEngine::decodeEntries(mdb, dp, err)) return false;
            for (auto& e : dp.full) {
                if (e.size() < 4) return false;
                size_t d = e.size() - 4;
                uint32_t c = ((uint32_t)e[d] << 24) | ((uint32_t)e[d+1] << 16) | ((uint32_t)e[d+2] << 8) | (uint32_t)e[d+3];
                if (!collect(c, out, seen, depth + 1)) return false;
            }
            uint32_t tail = Jet4Writer::UsageMapManager::getU32(mdb, 20);
            if (tail != 0) {
                if (!collect(tail, out, seen, depth + 1)) return false;
            }
            return true;
        };
        if (t->indices) for (unsigned ii = 0; ii < t->indices->len; ++ii) {
            MdbIndex* mIdx = (MdbIndex*)g_ptr_array_index(t->indices, ii);
            if (!onlyIndex.isEmpty() && onlyIndex.compare(mIdx->name, Qt::CaseInsensitive) != 0) continue;
            if (!mIdx || mIdx->first_pg == 0) continue;
            std::vector<uint32_t> order;
            std::set<uint32_t> seen;
            if (!collect(mIdx->first_pg, order, seen, 0)) {
                std::cout << "rechain " << mIdx->name << ": traversal failed\n";
                continue;
            }
            for (size_t i = 0; i < order.size(); ++i) {
                if (!Jet4Writer::UsageMapManager::readPage(mdb, order[i])) break;
                Jet4Writer::UsageMapManager::putU32(mdb, 12, i == 0 ? 0 : order[i - 1]);
                Jet4Writer::UsageMapManager::putU32(mdb, 16, (i + 1 < order.size()) ? order[i + 1] : 0);
                if (!Jet4Writer::UsageMapManager::writePage(mdb, order[i])) break;
            }
            std::cout << "rechain " << mIdx->name << ": relinked " << order.size() << " leaves\n";
        }
        mdb_free_tabledef(t);
        mdb_close(mdb);
        return 0;
    }

    printUsage();
    return 0;
}
