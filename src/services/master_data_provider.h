#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QHash>
#include <QMutex>

/**
 * @brief Unified Master Data Provider for Feeding Data across the Application.
 * 
 * Provides dynamically fetched and cached access to Groups, Ledgers/Parties,
 * Stock Items, Units, Voucher Types, and Mandi/GST Rates directly from SQLite.
 * Eliminates hardcoded strings and magic numbers.
 */
class MasterDataProvider : public QObject {
    Q_OBJECT

public:
    static MasterDataProvider& instance();

    // Cache management
    void invalidateAllCaches();
    void invalidateAccountCache();
    void invalidateStockCache();

    // 1. Groups Data Feeding
    QVariantList getAllGroups(bool hierarchical = false);
    QStringList getGroupNames(const QString& filterNature = "");
    QVariantMap getGroupDetails(int groupId);
    QVariantMap getGroupDetailsByCode(int code1st);
    int getGroupIdByName(const QString& groupName);

    // 2. Ledgers & Parties Data Feeding (used by AccountSearchBox, Voucher Entry, Ledgers)
    QVariantList searchAccounts(const QString& query, const QString& filterGroup = "", int limit = 50);
    QVariantList getAccountsByGroup(const QString& groupName);
    QVariantList getAccountsByGroupCodes(const QVector<int>& groupCodes);
    QVariantMap getAccountDetails(int accountId);
    QVariantMap getAccountDetailsByName(const QString& accountName);
    QStringList getAccountNames(const QString& filterGroup = "");

    // 3. Stock Items & Categories Data Feeding (used by ItemSearchDelegate, Milling, Invoices)
    QVariantList searchStockItems(const QString& query, int limit = 50);
    QVariantList getAllStockItems();
    QVariantMap getStockItemDetails(int itemId);
    QVariantMap getStockItemDetailsByName(const QString& itemName);
    QVariantMap getStockItemDetailsByCode(const QString& itemCode);
    QStringList getStockItemNames();
    QStringList getStockCategories();

    // 4. Units of Measurement & Classifications
    QVariantList getAllStockUnits();
    QStringList getUnitNames();
    QVariantMap getUnitDetails(const QString& unitName);
    double getUnitConversionFactor(const QString& fromUnit, const QString& toUnit);

    // 5. Dynamic Voucher Types & Configurations
    QVariantList getAvailableVoucherTypes();
    QVariantMap getVoucherTypeByCode(const QString& code);
    QStringList getVoucherTypeCodes();
    QVariantMap getVoucherSettings(const QString& voucherType);

    // 6. Tax Rates & Statutory Charges (Dami, Market Fee, HRDF, GST, TDS, TCS)
    QVariantList getActiveTaxRates();
    QVariantList getMandiChargeDefinitions();
    double getStandardTaxRate(const QString& hsnCode = QString());
    double getItemGstRate(const QString& itemName, int itemId = 0);
    QString getItemHsnCode(const QString& itemName, int itemId = 0);
    double getTcsRate(bool hasPan = true);
    double getTds194QRate();

    // 7. Company & Financial Year Info
    QVariantMap getActiveCompanyInfo();
    QVariantList getFinancialYearsList();
    QVariantMap getActiveFinancialYear();

signals:
    void masterDataChanged();

private:
    MasterDataProvider(QObject* parent = nullptr);
    ~MasterDataProvider() override = default;
    MasterDataProvider(const MasterDataProvider&) = delete;
    MasterDataProvider& operator=(const MasterDataProvider&) = delete;

    QMutex m_cacheMutex;
};
