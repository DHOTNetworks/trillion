#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <iostream>
#include "../database_manager.h"
#include "../engine/bahi_khata_migrator.h"
#include "../engine/balance_sheet_calculator.h"
#include "../engine/profit_loss_calculator.h"
#include "../engine/stock_valuation_engine.h"
#include "../engine/fiscal_year_helper.h"
#include "../engine/accounting_engine.h"

void printUsage() {
    std::cout << "Financial Engine Inspection & Verification CLI\n"
              << "Usage:\n"
              << "  ./FinancialEngineCLI --migrate <mdb_path> <sqlite_db_path>\n"
              << "  ./FinancialEngineCLI --inspect <sqlite_db_path> [--fy <YYYY-YY> | --as-on <YYYY-MM-DD>]\n"
              << "  ./FinancialEngineCLI --all\n"
              << std::endl;
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

    if (args.contains("--all")) {
        QStringList dbs = {
            "data/sushil_trading_company_data_018.db",
            "data/mahadev_rice_industry_data_004.db",
            "data/sushil_kr_pardeep_kr_data_004.db"
        };
        for (const auto& db : dbs) {
            if (QFile::exists(db)) {
                inspectDatabase(db, "", "");
            }
        }
        return 0;
    }

    printUsage();
    return 0;
}
