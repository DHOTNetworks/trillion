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
#include "engine/bahi_khata_migrator.h"

int main(int argc, char* argv[]) {
    std::cout << "[INIT] Starting Mahadev Rice Mill ERP native executable..." << std::endl << std::flush;

    // Force Basic Style for dark theme
    QQuickStyle::setStyle("Basic");

    QGuiApplication app(argc, argv);
    app.setApplicationName("Mahadev Rice Mill ERP & Accounting");
    app.setOrganizationName("MahadevAgro");

    QString appDir = QCoreApplication::applicationDirPath();
    // Resolve Database Path: search only in CWD, if not there then create one in CWD
    QString resolvedDbPath = QDir::current().filePath("mahadev_accounting.db");

    std::cout << "[INFO] Initializing SQLite database at: " << resolvedDbPath.toStdString() << std::endl << std::flush;
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
