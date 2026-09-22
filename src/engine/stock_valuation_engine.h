#pragma once

#include <QString>
#include <QVector>
#include <QVariantMap>
#include <QVariantList>
#include <QDate>

struct StockValuationItem {
    int itemId = 0;
    QString itemCode;
    QString itemName;
    QString unit = "QTL";
    int bags = 0;
    double weightQtl = 0.0;
    double rate = 0.0;
    double amount = 0.0;
    QString amountFmt;
    bool isAudited = false;
};

struct StockValuationReport {
    QString asOnDate;
    QString financialYear;
    bool isAuditedSnapshot = false;
    int totalBags = 0;
    double totalWeightQtl = 0.0;
    double totalValuation = 0.0;
    QString totalValuationFmt;
    QVector<StockValuationItem> items;
};

class StockValuationEngine {
public:
    // 1. Calculate live dynamic physical stock on date (without requiring pre-existing custom closing stock)
    static StockValuationReport calculateLivePhysicalStock(const QString& asOnDateIso);

    // 2. Fetch audited closing stock snapshot from custom_closing_stocks if present
    static StockValuationReport getAuditedClosingStock(const QString& asOnDateIso);

    // 3. Unified method: Returns audited snapshot if exists, otherwise computes live dynamic physical stock
    static StockValuationReport getEffectiveClosingStock(const QString& asOnDateIso);

    // 4. Save or Lock audited custom closing stock snapshot for a specific date
    static bool saveAuditedClosingStock(const QString& closingDateIso, const QVector<StockValuationItem>& items, QString& errorOut);

    // 5. Delete audited custom closing stock snapshot for a specific date
    static bool deleteAuditedClosingStock(const QString& closingDateIso, QString& errorOut);

    // 6. Check if audited closing stock exists on date
    static bool hasAuditedClosingStock(const QString& asOnDateIso);

    // 7. Auto-populate live stock into custom_closing_stocks (1-click Year-End Stock Audit Lock)
    static bool autoLockYearEndClosingStock(const QString& asOnDateIso, QString& errorOut);
};
