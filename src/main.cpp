#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QIcon>
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
#include "engine/bahi_khata_migrator.h"

int main(int argc, char* argv[]) {
    std::cout << "[INIT] Starting Mahadev Rice Mill ERP native executable..." << std::endl << std::flush;

    // Force Basic Style for dark theme
    QQuickStyle::setStyle("Basic");

    QGuiApplication app(argc, argv);
    app.setApplicationName("Mahadev Rice Mill ERP & Accounting");
    app.setOrganizationName("MahadevAgro");

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

    // In App's Current Launched Directory, create a "data" folder
    QDir dataDir(launchDir.filePath("data"));
    if (!dataDir.exists()) {
        dataDir.mkpath(".");
    }

    // Initialize Multi-Firm Manager
    FirmManager firmManager;
    QString resolvedDbPath = "data/mahadev_rice.db";
    QVariantList registeredFirms = firmManager.get_registered_firms();
    for (const auto& f : registeredFirms) {
        if (f.toMap().value("id").toString() == firmManager.currentFirmId()) {
            resolvedDbPath = f.toMap().value("db_path").toString();
            break;
        }
    }
    if (!QFile::exists(resolvedDbPath)) {
        if (QFile::exists(dataDir.filePath("mahadev_rice.db"))) {
            resolvedDbPath = dataDir.filePath("mahadev_rice.db");
        } else if (QFile::exists(dataDir.filePath("mahadev_accounting.db"))) {
            resolvedDbPath = dataDir.filePath("mahadev_accounting.db");
        }
    }

    std::cout << "[INFO] Launch Directory: " << launchDir.absolutePath().toStdString() << std::endl;
    std::cout << "[INFO] Data Directory: " << dataDir.absolutePath().toStdString() << std::endl;
    std::cout << "[INFO] Active Firm: " << firmManager.currentFirmName().toStdString() << std::endl;
    std::cout << "[INFO] Active SQLite database at: " << resolvedDbPath.toStdString() << std::endl << std::flush;

    bool dbOk = DatabaseManager::instance().initDatabase(resolvedDbPath);
    if (!dbOk) {
        std::cerr << "[ERROR] Failed to initialize SQLite database at: " << resolvedDbPath.toStdString() << std::endl << std::flush;
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

    // Register all Context Properties (100% 1-to-1 match with Python PySide6)
    QQmlContext* ctx = engine.rootContext();
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

    std::cout << "[SUCCESS] Created " << rootObjs.size() << " root window objects! Showing UI window on screen..." << std::endl << std::flush;
    for (QObject* obj : rootObjs) {
        QQuickWindow* window = qobject_cast<QQuickWindow*>(obj);
        if (window) {
            window->show();
            window->raise();
            window->requestActivate();
        }
    }

    return app.exec();
}
