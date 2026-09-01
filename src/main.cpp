#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QDir>
#include <QDebug>

#include "database_manager.h"
#include "models/sales_model.h"
#include "models/purchase_model.h"
#include "models/vouchers_model.h"
#include "models/stock_items_model.h"
#include "models/parties_model.h"
#include "models/milling_model.h"
#include "models/financial_years_model.h"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("Mahadev Rice Mill Accounting ERP");
    app.setOrganizationName("MahadevAgro");

    // Initialize Embedded Database
    DatabaseManager::instance().initDatabase("data/mahadev_erp.db");

    // Instantiate C++ Models
    SalesModel salesModel;
    PurchaseModel purchaseModel;
    VouchersModel vouchersModel;
    StockItemsModel stockItemsModel;
    PartiesModel partiesModel;
    MillingModel millingModel;
    FinancialYearsModel financialYearsModel;

    QQmlApplicationEngine engine;

    // Register Context Properties (Zero QML Changes)
    QQmlContext* ctx = engine.rootContext();
    ctx->setContextProperty("salesModel", &salesModel);
    ctx->setContextProperty("purchaseModel", &purchaseModel);
    ctx->setContextProperty("vouchersModel", &vouchersModel);
    ctx->setContextProperty("stockItemsModel", &stockItemsModel);
    ctx->setContextProperty("partiesModel", &partiesModel);
    ctx->setContextProperty("millingModel", &millingModel);
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

    qInfo() << "Mahadev Rice Mill Accounting ERP (Native C++ Engine) initialized successfully!";
    return app.exec();
}
