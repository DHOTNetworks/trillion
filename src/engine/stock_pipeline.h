#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <QHash>
#include <QMutex>

/**
 * @brief Dynamic Stock & Inventory Pipeline
 * 
 * Manages Stock Categories and Commodity Items, resolves 'qty_not_show_in_trading'
 * and 'include_in_trading', computes physical and financial movements, and generates
 * the dynamic Trading Account Model.
 */
struct StockGroupRecord {
    int id = 0;
    int legacyCode = 0;
    QString groupName;
    bool qtyNotShowInTrading = false;
    double clStockRate = 0.0;
};

struct StockItemRecord {
    int id = 0;
    int legacyCode = 0;
    QString code;
    QString name;
    QString printName;
    QString itemType;
    int groupId = 0;
    QString groupName;
    QString unit;
    QString altUnit;
    double conversionFactor = 1.0;
    double packingKg = 50.0;
    QString hsnCode;
    double gstRate = 5.0;
    double purchaseRate = 0.0;
    double saleRate = 0.0;
    double openingBags = 0.0;
    double openingQty = 0.0;
    double openingRate = 0.0;
    double openingValue = 0.0;
    bool isMillingItem = false;
    bool includeInTrading = true;
    bool calculateStock = true;
};

struct StockItemBalanceResult {
    int itemId = 0;
    QString itemCode;
    QString itemName;
    QString groupName;
    QString unit;
    double packingKg = 50.0;
    bool qtyNotShowInTrading = false;
    bool includeInTrading = true;

    // Opening
    double openingBags = 0.0;
    double openingWeightQtl = 0.0;
    double openingRate = 0.0;
    double openingValue = 0.0;

    // Inward (Purchases / Milling Output)
    double inwardBags = 0.0;
    double inwardWeightQtl = 0.0;
    double inwardValue = 0.0;

    // Outward (Sales / Milling Input)
    double outwardBags = 0.0;
    double outwardWeightQtl = 0.0;
    double outwardValue = 0.0;

    // Closing
    double closingBags = 0.0;
    double closingWeightQtl = 0.0;
    double closingRate = 0.0;
    double closingValue = 0.0;
};

class StockPipeline : public QObject {
    Q_OBJECT

public:
    static StockPipeline& instance();

    void invalidateCache();
    QVector<StockGroupRecord> getAllStockGroups();
    QVector<StockItemRecord> getAllStockItems();
    StockItemRecord getItemById(int itemId);
    StockItemRecord getItemByName(const QString& name);
    StockItemRecord getItemByCode(const QString& code);

    // Dynamic Balance Calculation across date range
    QVector<StockItemBalanceResult> calculateStockBalances(
        const QString& fromDateIso,
        const QString& toDateIso,
        int filterGroupId = 0
    );

    // Dynamic Trading Account Model
    struct TradingAccountSection {
        double openingStockValue = 0.0;
        double purchasesValue = 0.0;
        double directExpensesValue = 0.0;
        double salesValue = 0.0;
        double closingStockValue = 0.0;
        double grossProfit = 0.0;
        double grossLoss = 0.0;
        QVector<StockItemBalanceResult> tradingItems;
    };

    TradingAccountSection generateTradingAccount(const QString& fromDateIso, const QString& toDateIso);

    // Search & UI Feeding for ItemSearchDelegate
    QVariantList searchItems(const QString& query, int limit = 50);
    QStringList getItemNames();
    QStringList getStockGroupNames();
    QStringList getStockUnitNames();

    bool saveStockItem(const StockItemRecord& item, QString* outError = nullptr);
    bool deleteStockItem(int itemId, QString* outError = nullptr);

signals:
    void stockChanged();

private:
    StockPipeline(QObject* parent = nullptr);
    ~StockPipeline() override = default;
    StockPipeline(const StockPipeline&) = delete;
    StockPipeline& operator=(const StockPipeline&) = delete;

    void ensureLoaded();

    QMutex m_mutex;
    bool m_valid = false;
    QHash<int, StockGroupRecord> m_groupsById;
    QHash<int, StockItemRecord> m_itemsById;
    QHash<QString, StockItemRecord> m_itemsByName;
    QHash<QString, StockItemRecord> m_itemsByCode;
};
