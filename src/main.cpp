#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QIcon>
#include <QFont>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QDateTime>
#include <QDebug>
#include <iostream>

#include "database_manager.h"
#include "models/dashboard_controller.h"
#include "models/paddy_arrivals_model.h"
#include "models/sales_model.h"
#include "models/purchase_model.h"
#include "models/vouchers_model.h"
#include "models/stock_items_model.h"
#include "models/parties_model.h"
#include "models/account_groups_model.h"
#include "models/milling_model.h"
#include "models/financial_years_model.h"
#include "models/firm_manager.h"
#include "models/jform_model.h"
#include "models/tds_model.h"
#include "models/interest_model.h"
#include "models/ledger_statement_model.h"
#include "models/purchase_register_model.h"
#include "models/sales_register_model.h"
#include "models/stock_register_model.h"
#include "models/milling_statement_model.h"
#include "services/print_export_controller.h"
#include "services/accounting_date_service.h"
#include "services/financial_math_service.h"
#include "models/sales_voucher_controller.h"
#include "models/purchase_voucher_controller.h"
#include "models/journal_voucher_controller.h"
#include "models/cheque_voucher_controller.h"
#include "models/tds_voucher_controller.h"
#include "models/jform_voucher_controller.h"
#include "models/milling_batch_controller.h"
#include "models/paddy_procurement_controller.h"
#include "models/ledger_master_controller.h"
#include "models/stock_master_controller.h"
#include "models/bank_statement_controller.h"
#include "models/transport_dispatch_controller.h"
#include "models/debit_credit_note_controller.h"
#include "models/global_key_filter.h"
#include "engine/bahi_khata_migrator.h"
#include "widgets/main_window.h"

int main(int argc, char* argv[]) {
    std::cout << "[INIT] Starting Mahadev Rice Mill ERP native executable..." << std::endl << std::flush;

#ifdef Q_OS_WIN
    // Direct3D 11 backend provides ultra-fast native hardware acceleration on Windows
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);
#endif

    // Force Basic Style for dark theme
    QQuickStyle::setStyle("Basic");

    QApplication app(argc, argv);
    app.setApplicationName("Mahadev Rice Mill ERP & Accounting");
    app.setApplicationVersion("0.2.0");
    app.setOrganizationName("MahadevAgro");

    // Universal cross-platform font configuration (Windows, macOS, Linux x86_64/ARM64)
    QFont defaultAppFont;
#ifdef Q_OS_WIN
    defaultAppFont.setFamilies({"Segoe UI", "Calibri", "Arial"});
    defaultAppFont.setPointSize(10);
#elif defined(Q_OS_MACOS)
    defaultAppFont.setFamilies({"Helvetica Neue", "Arial"});
    defaultAppFont.setPointSize(12);
#else
    defaultAppFont.setFamilies({"Noto Sans", "Liberation Sans", "DejaVu Sans", "Arial"});
    defaultAppFont.setPointSize(10);
#endif
    defaultAppFont.setStyleHint(QFont::SansSerif);
    app.setFont(defaultAppFont);

    // App-wide clean dialog style ensuring high readability on all OS themes (macOS Dark Mode safe)
    app.setStyleSheet(
        "QDialog, QMessageBox, QInputDialog {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, 'Helvetica Neue', 'Noto Sans', 'Liberation Sans', Arial, sans-serif;"
        "}"
        "QDialog QLabel, QMessageBox QLabel, QInputDialog QLabel {"
        "  color: #0F172A;"
        "  background: transparent;"
        "  font-size: 13px;"
        "  font-weight: 600;"
        "}"
        "QDialog QPushButton, QMessageBox QPushButton, QInputDialog QPushButton {"
        "  background-color: #F1F5F9;"
        "  color: #0F172A;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 3px;"
        "  padding: 5px 16px;"
        "  font-size: 12px;"
        "  font-weight: 700;"
        "  min-width: 70px;"
        "  min-height: 24px;"
        "}"
        "QDialog QPushButton:hover, QMessageBox QPushButton:hover, QInputDialog QPushButton:hover {"
        "  background-color: #E2E8F0;"
        "}"
        "QDialog QPushButton:default, QMessageBox QPushButton:default, QInputDialog QPushButton:default {"
        "  background-color: #0284C7;"
        "  color: #FFFFFF;"
        "  border: 1px solid #0369A1;"
        "}"
        "QDialog QPushButton:default:hover, QMessageBox QPushButton:default:hover, QInputDialog QPushButton:default:hover {"
        "  background-color: #0369A1;"
        "}"
        "QDialog QLineEdit, QInputDialog QLineEdit {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  border: 1.5px solid #7F9DB9;"
        "  border-radius: 3px;"
        "  padding: 5px 8px;"
        "  font-size: 13px;"
        "  font-weight: 700;"
        "}"
        "QDialog QLineEdit:focus, QInputDialog QLineEdit:focus {"
        "  border: 2px solid #0284C7;"
        "  background-color: #FFFFDD;"
        "}"
    );

    QString appDir = QCoreApplication::applicationDirPath();
    QString cwd = QDir::currentPath();

    // Determine the directory from where the app is launched
    QDir launchDir;
#ifdef Q_OS_WIN
    // On Windows, use application directory (folder containing .exe) or CWD if run from CLI
    launchDir = QDir(appDir);
    if (cwd != "/" && !cwd.endsWith("System32", Qt::CaseInsensitive)) {
        launchDir = QDir(cwd);
    }
#else
    // On macOS: If running from terminal, use CWD
    if (cwd != "/" && QFileInfo(cwd).isWritable()) {
        launchDir = QDir(cwd);
    } else {
        // If launched via Finder/bundle: Contents/MacOS/../../../.. -> project root
        QDir projDir(appDir + "/../../../..");
        if (projDir.exists("CMakeLists.txt") || projDir.exists("mahadev_accounting.db") || projDir.exists("data")) {
            launchDir = projDir;
        } else {
            launchDir = QDir(appDir + "/../../..");
        }
    }
#endif
    // Set working directory to launchDir so relative paths (like "data/") work consistently
    QDir::setCurrent(launchDir.absolutePath());
    std::cout << "[INIT] Working directory set to: " << launchDir.absolutePath().toStdString() << std::endl;

    // In App's Current Launched Directory, create a "data" folder
    QDir dataDir(launchDir.filePath("data"));
    if (!dataDir.exists()) {
        dataDir.mkpath(".");
    }

    // Initialize Multi-Firm Manager
    FirmManager firmManager;
    QString resolvedDbPath = "";
    QString activeFirmId = firmManager.currentFirmId();
    if (!activeFirmId.isEmpty()) {
        QVariantList registeredFirms = firmManager.get_registered_firms();
        for (const auto& f : registeredFirms) {
            if (f.toMap().value("id").toString() == activeFirmId) {
                QString candPath = f.toMap().value("db_path").toString();
                if (QFile::exists(candPath)) {
                    resolvedDbPath = candPath;
                }
                break;
            }
        }
    }

    std::cout << "[INFO] Launch Directory: " << launchDir.absolutePath().toStdString() << std::endl;
    std::cout << "[INFO] Data Directory: " << dataDir.absolutePath().toStdString() << std::endl;
    std::cout << "[INFO] Active Firm: " << (firmManager.currentFirmName().isEmpty() ? "None (Firm Selector)" : firmManager.currentFirmName().toStdString()) << std::endl;

    if (!resolvedDbPath.isEmpty()) {
        std::cout << "[INFO] Active SQLite database at: " << resolvedDbPath.toStdString() << std::endl << std::flush;
        bool dbOk = DatabaseManager::instance().initDatabase(resolvedDbPath);
        if (!dbOk) {
            std::cerr << "[ERROR] Failed to initialize SQLite database at: " << resolvedDbPath.toStdString() << std::endl << std::flush;
        }
    } else {
        std::cout << "[INFO] No firm database pre-selected. App will present Firm Selector." << std::endl << std::flush;
    }

    // Instantiate C++ Models matching Python backend
    DashboardController dashboardCtrl;
    PaddyArrivalsModel paddyModel;
    MillingModel millingModel;
    SalesModel salesModel;
    PurchaseModel purchaseModel;
    VouchersModel vouchersModel;
    PartiesModel partiesModel;
    AccountGroupsModel groupsModel;
    StockItemsModel stockItemsModel;
    FinancialYearsModel financialYearsModel;
    BahiKhataMigrator bahiKhataMigrator;
    JFormModel jformModel;
    TdsModel tdsModel;
    InterestModel interestModel;
    LedgerStatementController ledgerStatementCtrl;
    PurchaseRegisterController purchaseRegisterCtrl;
    SalesRegisterController salesRegisterCtrl;
    StockRegisterController stockRegisterCtrl;
    MillingStatementController millingStatementCtrl;
    PrintExportController printExportCtrl;
    AccountingDateService dateService;
    FinancialMathService mathService;
    SalesVoucherController salesVoucherCtrl;
    PurchaseVoucherController purchaseVoucherCtrl;
    JournalVoucherController journalVoucherCtrl;
    ChequeVoucherController chequeVoucherCtrl;
    TdsVoucherController tdsVoucherCtrl;
    JFormVoucherController jformVoucherCtrl;
    MillingBatchController millingBatchCtrl;
    PaddyProcurementController paddyProcurementCtrl;
    LedgerMasterController ledgerMasterCtrl;
    StockMasterController stockMasterCtrl;
    BankStatementController bankStatementCtrl;
    TransportDispatchController transportDispatchCtrl;
    DebitCreditNoteController debitCreditNoteCtrl;

    for (int i = 1; i < argc; ++i) {
        if (QString(argv[i]) == "--render-preview") {
            QString origPdf = "/tmp/final_sales_preview.pdf";
            printExportCtrl.export_sales_invoice_pdf("12640", origPdf, "ORIGINAL FOR RECIPIENT");
            QString dupPdf = "/tmp/duplicate_sales_preview.pdf";
            printExportCtrl.export_sales_invoice_duplicate_pdf("12640", dupPdf);
            std::cout << "[RENDER] Exported previews to " << origPdf.toStdString() << " and " << dupPdf.toStdString() << std::endl;
            return 0;
        }
    }

    // Reload models automatically when firm switches
    QObject::connect(&firmManager, &FirmManager::firmSwitched, [&](const QString& firmId, const QString& firmName) {
        std::cout << "[INFO] Firm switched to: " << firmName.toStdString() << " (" << firmId.toStdString() << ")" << std::endl;
        dashboardCtrl.refresh_stats();
        paddyModel.reload_data();
        millingModel.reload_data();
        salesModel.reload_data();
        purchaseModel.reload_data();
        vouchersModel.reload_data();
        partiesModel.reload_data();
        groupsModel.reload_data();
        stockItemsModel.reload_data();
        financialYearsModel.reload_data();
        ledgerStatementCtrl.refresh();
        purchaseRegisterCtrl.reload();
        salesRegisterCtrl.reload();
        stockRegisterCtrl.reload();
        millingStatementCtrl.reload();
        transportDispatchCtrl.reload();
        debitCreditNoteCtrl.reload();
    });

    for (int i = 1; i < argc; ++i) {
        QString arg = argv[i];
        if (arg == "--migrate" && i + 1 < argc) {
            QString mdbPath = argv[++i];
            std::cout << "[CLI] Running headless migration on: " << mdbPath.toStdString() << std::endl;
            bool ok = bahiKhataMigrator.migrate_mdb_file(mdbPath);
            std::cout << "[CLI] Migration result: " << (ok ? "SUCCESS" : "FAILED") << std::endl;
            return ok ? 0 : 1;
        }
        if (arg == "--inspect" && i + 1 < argc) {
            QString mdbPath = argv[++i];
            std::cout << "[CLI] Running headless inspection on: " << mdbPath.toStdString() << std::endl;
            QVariantMap res = bahiKhataMigrator.inspect_mdb_file(mdbPath);
            std::cout << "[CLI] Valid: " << res.value("valid").toBool() << std::endl;
            std::cout << "[CLI] Tables: " << res.value("tableCount").toInt() << std::endl;
            std::cout << "[CLI] Stock Items: " << res.value("stockItemsCount").toInt() << std::endl;
            std::cout << "[CLI] Ledgers: " << res.value("ledgersCount").toInt() << std::endl;
            std::cout << "[CLI] Error: " << res.value("error").toString().toStdString() << std::endl;
            return res.value("valid").toBool() ? 0 : 1;
        }
        if (arg == "--stats") {
            dashboardCtrl.refresh_stats();
            std::cout << "[STATS] Paddy Stock: " << dashboardCtrl.paddyStock().toStdString() << std::endl;
            std::cout << "[STATS] Rice Stock: " << dashboardCtrl.riceStock().toStdString() << std::endl;
            std::cout << "[STATS] Total Revenue: " << dashboardCtrl.totalSales().toStdString() << std::endl;
            std::cout << "[STATS] Parties count: " << partiesModel.rowCount() << std::endl;
            std::cout << "[STATS] Stock Items count: " << stockItemsModel.rowCount() << std::endl;
            std::cout << "[STATS] Sales Invoices count: " << salesModel.rowCount() << std::endl;
            std::cout << "[STATS] Purchase Invoices count: " << purchaseModel.rowCount() << std::endl;
            std::cout << "[STATS] Milling Batches count: " << millingModel.rowCount() << std::endl;
            return 0;
        }
        if (arg == "--test-custom-period") {
            dashboardCtrl.refresh_stats("2025-04-01", "2027-03-31", "Custom Period");
            std::cout << "[CUSTOM-PERIOD] Paddy Stock: " << dashboardCtrl.paddyStock().toStdString() << std::endl;
            std::cout << "[CUSTOM-PERIOD] Rice Stock: " << dashboardCtrl.riceStock().toStdString() << std::endl;
            std::cout << "[CUSTOM-PERIOD] Total Revenue: " << dashboardCtrl.totalSales().toStdString() << std::endl;
            return 0;
        }
        if (arg == "--import-firm" && i + 1 < argc) {
            QString mdbPath = argv[++i];
            QString fName = (i + 1 < argc && !QString(argv[i + 1]).startsWith("--")) ? argv[++i] : "";
            bool ok = firmManager.import_bahi_khata_firm(mdbPath, fName);
            std::cout << "[IMPORT] Firm import result: " << (ok ? "SUCCESS" : "FAILED") << std::endl;
            return ok ? 0 : 1;
        }
    }

    QQmlApplicationEngine engine;

    // Capture all QML engine warnings & errors
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, [](const QList<QQmlError>& warnings) {
        for (const auto& w : warnings) {
            std::cerr << "[QML WARNING] " << w.toString().toStdString() << std::endl << std::flush;
        }
    });

    GlobalKeyFilter globalKeyFilter;
    app.installEventFilter(&globalKeyFilter);

    // Register all Context Properties (100% 1-to-1 match with Python PySide6)
    QQmlContext* ctx = engine.rootContext();
    ctx->setContextProperty("globalKeyFilter", &globalKeyFilter);
    ctx->setContextProperty("dashboardCtrl", &dashboardCtrl);
    ctx->setContextProperty("paddyModel", &paddyModel);
    ctx->setContextProperty("millingModel", &millingModel);
    ctx->setContextProperty("salesModel", &salesModel);
    ctx->setContextProperty("purchaseModel", &purchaseModel);
    ctx->setContextProperty("vouchersModel", &vouchersModel);
    ctx->setContextProperty("partiesModel", &partiesModel);
    ctx->setContextProperty("groupsModel", &groupsModel);
    ctx->setContextProperty("stockItemsModel", &stockItemsModel);
    ctx->setContextProperty("financialYearsModel", &financialYearsModel);
    ctx->setContextProperty("bahiKhataMigrator", &bahiKhataMigrator);
    ctx->setContextProperty("firmManager", &firmManager);
    ctx->setContextProperty("jformModel", &jformModel);
    ctx->setContextProperty("tdsModel", &tdsModel);
    ctx->setContextProperty("interestModel", &interestModel);
    ctx->setContextProperty("ledgerStatementCtrl", &ledgerStatementCtrl);
    ctx->setContextProperty("purchaseRegisterCtrl", &purchaseRegisterCtrl);
    ctx->setContextProperty("salesRegisterCtrl", &salesRegisterCtrl);
    ctx->setContextProperty("stockRegisterCtrl", &stockRegisterCtrl);
    ctx->setContextProperty("millingStatementCtrl", &millingStatementCtrl);
    ctx->setContextProperty("printExportCtrl", &printExportCtrl);
    ctx->setContextProperty("dateService", &dateService);
    ctx->setContextProperty("mathService", &mathService);
    ctx->setContextProperty("salesVoucherCtrl", &salesVoucherCtrl);
    ctx->setContextProperty("purchaseVoucherCtrl", &purchaseVoucherCtrl);
    ctx->setContextProperty("journalVoucherCtrl", &journalVoucherCtrl);
    ctx->setContextProperty("chequeVoucherCtrl", &chequeVoucherCtrl);
    ctx->setContextProperty("tdsVoucherCtrl", &tdsVoucherCtrl);
    ctx->setContextProperty("jformVoucherCtrl", &jformVoucherCtrl);
    ctx->setContextProperty("millingBatchCtrl", &millingBatchCtrl);
    ctx->setContextProperty("paddyProcurementCtrl", &paddyProcurementCtrl);
    ctx->setContextProperty("ledgerMasterCtrl", &ledgerMasterCtrl);
    ctx->setContextProperty("stockMasterCtrl", &stockMasterCtrl);
    ctx->setContextProperty("bankStatementCtrl", &bankStatementCtrl);
    ctx->setContextProperty("transportDispatchCtrl", &transportDispatchCtrl);
    ctx->setContextProperty("debitCreditNoteCtrl", &debitCreditNoteCtrl);

    // Add import paths (Embedded QRC + local file fallbacks)
    engine.addImportPath(":/");
    engine.addImportPath(":/MahadevERP");
    engine.addImportPath(":/MahadevERP/qml");
    engine.addImportPath("qrc:/");
    engine.addImportPath("qrc:/MahadevERP");
    engine.addImportPath("qrc:/MahadevERP/qml");
    engine.addImportPath(QDir(appDir).filePath("qml"));
    engine.addImportPath(QDir(appDir).filePath("qml/components"));
    engine.addImportPath(QDir(appDir).filePath("qml/views"));
    engine.addImportPath(QDir(appDir).filePath("qml/dialogs"));
    engine.addImportPath(QDir::current().filePath("qml"));

    // Determine Main QML URL (prefer compiled QRC resource)
    QUrl mainQmlUrl("qrc:/MahadevERP/qml/main.qml");
    if (!QFile::exists(":/MahadevERP/qml/main.qml")) {
        QString localAppQml = QDir(appDir).filePath("qml/main.qml");
        QString localCurQml = QDir::current().filePath("qml/main.qml");
        if (QFile::exists(localAppQml)) {
            mainQmlUrl = QUrl::fromLocalFile(localAppQml);
        } else if (QFile::exists(localCurQml)) {
            mainQmlUrl = QUrl::fromLocalFile(localCurQml);
        } else if (QFile::exists("/Users/karan/MahadevAc/qml/main.qml")) {
            mainQmlUrl = QUrl::fromLocalFile("/Users/karan/MahadevAc/qml/main.qml");
        }
    }

    std::cout << "[INFO] Loading main QML: " << mainQmlUrl.toString().toStdString() << std::endl << std::flush;

    engine.load(mainQmlUrl);

    const auto rootObjs = engine.rootObjects();
    if (rootObjs.isEmpty()) {
        std::cerr << "[FATAL] No root QML objects created after engine.load()!" << std::endl << std::flush;
        return -1;
    }

    QQuickWindow* qmlWindow = nullptr;
    for (QObject* obj : rootObjs) {
        qmlWindow = qobject_cast<QQuickWindow*>(obj);
        if (qmlWindow) {
            break;
        }
    }

    if (!qmlWindow) {
        std::cerr << "[FATAL] Root QML object is not a QQuickWindow!" << std::endl << std::flush;
        return -1;
    }

    std::cout << "[SUCCESS] Creating Hybrid MainWindow..." << std::endl << std::flush;
    MainWindow mainWindow(qmlWindow, &ledgerStatementCtrl, &printExportCtrl, &dashboardCtrl, &firmManager, &bahiKhataMigrator);
    mainWindow.show();
    mainWindow.raise();
    mainWindow.activateWindow();

    return app.exec();
}
