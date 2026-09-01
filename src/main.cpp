#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
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

// File logging handler to write diagnostic logs to debug_log.txt
void customLogMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QString txt;
    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    switch (type) {
        case QtDebugMsg:
            txt = QString("[%1] [DEBUG] %2").arg(timeStr, msg);
            break;
        case QtInfoMsg:
            txt = QString("[%1] [INFO] %2").arg(timeStr, msg);
            break;
        case QtWarningMsg:
            txt = QString("[%1] [WARNING] %2 (%3:%4)").arg(timeStr, msg, context.file ? context.file : "", QString::number(context.line));
            break;
        case QtCriticalMsg:
            txt = QString("[%1] [CRITICAL] %2 (%3:%4)").arg(timeStr, msg, context.file ? context.file : "", QString::number(context.line));
            break;
        case QtFatalMsg:
            txt = QString("[%1] [FATAL] %2 (%3:%4)").arg(timeStr, msg, context.file ? context.file : "", QString::number(context.line));
            break;
    }

    QFile outFile(QDir::current().filePath("debug_log.txt"));
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream ts(&outFile);
        ts << txt << "\n";
        outFile.close();
    }
}

int main(int argc, char* argv[]) {
    // Install log handler
    qInstallMessageHandler(customLogMessageHandler);

    qInfo() << "Starting Mahadev Rice Mill ERP application...";

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

    // Resolve QML Main entry point (prefer QRC embedded, fallback to local app directory)
    QUrl mainQmlUrl;
    if (QFile::exists(":/qml/main.qml")) {
        qInfo() << "Loading QML from embedded resource: qrc:/qml/main.qml";
        mainQmlUrl = QUrl("qrc:/qml/main.qml");
    } else {
        QString localPath = QDir::current().absoluteFilePath("qml/main.qml");
        qInfo() << "Loading QML from local filesystem path:" << localPath;
        mainQmlUrl = QUrl::fromLocalFile(localPath);
    }

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [mainQmlUrl](QObject* obj, const QUrl& objUrl) {
        if (!obj && mainQmlUrl == objUrl) {
            qCritical() << "FATAL: Failed to load root QML interface from" << objUrl;
            QCoreApplication::exit(-1);
        } else {
            qInfo() << "Root QML interface loaded successfully!";
        }
    }, Qt::QueuedConnection);

    engine.load(mainQmlUrl);

    return app.exec();
}
