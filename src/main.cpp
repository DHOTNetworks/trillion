#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <QDir>
#include <QDebug>

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

int main(int argc, char* argv[]) {
    // Set Basic style to ensure custom dark theme renders across all platforms
    QQuickStyle::setStyle("Basic");

    QGuiApplication app(argc, argv);
    app.setApplicationName("Mahadev Rice Mill ERP & Accounting");
    app.setOrganizationName("MahadevAgro");

    // Initialize SQLite Database
    DatabaseManager::instance().initDatabase("mahadev_accounting.db");

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

    QQmlApplicationEngine engine;

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

    // Support both QRC embedded build and file system execution
    const QUrl url = QUrl("qrc:/qml/main.qml");
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject* obj, const QUrl& objUrl) {
        if (!obj && url == objUrl) {
            qCritical() << "Failed to load QML interface!";
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);

    if (QFile::exists(":/qml/main.qml")) {
        engine.load(url);
    } else {
        engine.load(QUrl::fromLocalFile(QDir::current().absoluteFilePath("qml/main.qml")));
    }

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    qInfo() << "Mahadev Rice Mill ERP (Native C++ Engine) launched successfully!";
    return app.exec();
}
