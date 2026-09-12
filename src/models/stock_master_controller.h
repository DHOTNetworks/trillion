#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class StockMasterController : public QObject {
    Q_OBJECT

    Q_PROPERTY(int itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
    Q_PROPERTY(bool isEditMode READ isEditMode NOTIFY isEditModeChanged)

    // Identity & Hierarchy
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString code READ code WRITE setCode NOTIFY codeChanged)
    Q_PROPERTY(QString itemType READ itemType WRITE setItemType NOTIFY itemTypeChanged)
    Q_PROPERTY(QString goodsType READ goodsType WRITE setGoodsType NOTIFY goodsTypeChanged)
    Q_PROPERTY(QString companyName READ companyName WRITE setCompanyName NOTIFY companyNameChanged)
    Q_PROPERTY(QString stockGroup READ stockGroup WRITE setStockGroup NOTIFY stockGroupChanged)
    Q_PROPERTY(QString unit READ unit WRITE setUnit NOTIFY unitChanged)

    // Pricing & Tax
    Q_PROPERTY(double purchaseRate READ purchaseRate WRITE setPurchaseRate NOTIFY purchaseRateChanged)
    Q_PROPERTY(double saleRate READ saleRate WRITE setSaleRate NOTIFY saleRateChanged)
    Q_PROPERTY(double mrp READ mrp WRITE setMrp NOTIFY mrpChanged)
    Q_PROPERTY(double discount READ discount WRITE setDiscount NOTIFY discountChanged)
    Q_PROPERTY(QString hsnCode READ hsnCode WRITE setHsnCode NOTIFY hsnCodeChanged)
    Q_PROPERTY(double gstRate READ gstRate WRITE setGstRate NOTIFY gstRateChanged)
    Q_PROPERTY(double cessRate READ cessRate WRITE setCessRate NOTIFY cessRateChanged)

    // Physical & Packaging
    Q_PROPERTY(double packingKg READ packingKg WRITE setPackingKg NOTIFY packingKgChanged)

    // Opening Stock
    Q_PROPERTY(int openingBags READ openingBags WRITE setOpeningBags NOTIFY openingStockChanged)
    Q_PROPERTY(double openingQty READ openingQty WRITE setOpeningQty NOTIFY openingStockChanged)
    Q_PROPERTY(double openingRate READ openingRate WRITE setOpeningRate NOTIFY openingStockChanged)
    Q_PROPERTY(double openingValue READ openingValue NOTIFY openingStockChanged)
    Q_PROPERTY(QString openingValueFmt READ openingValueFmt NOTIFY openingStockChanged)

    // Account Mapping
    Q_PROPERTY(QString purchaseLedger READ purchaseLedger WRITE setPurchaseLedger NOTIFY purchaseLedgerChanged)
    Q_PROPERTY(QString saleLedger READ saleLedger WRITE setSaleLedger NOTIFY saleLedgerChanged)
    Q_PROPERTY(QString stockLedger READ stockLedger WRITE setStockLedger NOTIFY stockLedgerChanged)

    // Status
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(bool isError READ isError NOTIFY statusChanged)

public:
    explicit StockMasterController(QObject *parent = nullptr);

    int itemId() const { return m_itemId; }
    void setItemId(int id);
    bool isEditMode() const { return m_itemId > 0; }

    QString name() const { return m_name; }
    void setName(const QString &v) { if (m_name != v) { m_name = v; emit nameChanged(); } }

    QString code() const { return m_code; }
    void setCode(const QString &v) { if (m_code != v) { m_code = v; emit codeChanged(); } }

    QString itemType() const { return m_itemType; }
    void setItemType(const QString &v) { if (m_itemType != v) { m_itemType = v; emit itemTypeChanged(); } }

    QString goodsType() const { return m_goodsType; }
    void setGoodsType(const QString &v) { if (m_goodsType != v) { m_goodsType = v; emit goodsTypeChanged(); } }

    QString companyName() const { return m_companyName; }
    void setCompanyName(const QString &v) { if (m_companyName != v) { m_companyName = v; emit companyNameChanged(); } }

    QString stockGroup() const { return m_stockGroup; }
    void setStockGroup(const QString &v) { if (m_stockGroup != v) { m_stockGroup = v; emit stockGroupChanged(); } }

    QString unit() const { return m_unit; }
    void setUnit(const QString &v) { if (m_unit != v) { m_unit = v; emit unitChanged(); } }

    double purchaseRate() const { return m_purchaseRate; }
    void setPurchaseRate(double v) { if (std::abs(m_purchaseRate - v) > 0.001) { m_purchaseRate = v; emit purchaseRateChanged(); } }

    double saleRate() const { return m_saleRate; }
    void setSaleRate(double v) { if (std::abs(m_saleRate - v) > 0.001) { m_saleRate = v; emit saleRateChanged(); } }

    double mrp() const { return m_mrp; }
    void setMrp(double v) { if (std::abs(m_mrp - v) > 0.001) { m_mrp = v; emit mrpChanged(); } }

    double discount() const { return m_discount; }
    void setDiscount(double v) { if (std::abs(m_discount - v) > 0.001) { m_discount = v; emit discountChanged(); } }

    QString hsnCode() const { return m_hsnCode; }
    void setHsnCode(const QString &v) { if (m_hsnCode != v) { m_hsnCode = v; emit hsnCodeChanged(); } }

    double gstRate() const { return m_gstRate; }
    void setGstRate(double v) { if (std::abs(m_gstRate - v) > 0.001) { m_gstRate = v; emit gstRateChanged(); } }

    double cessRate() const { return m_cessRate; }
    void setCessRate(double v) { if (std::abs(m_cessRate - v) > 0.001) { m_cessRate = v; emit cessRateChanged(); } }

    double packingKg() const { return m_packingKg; }
    void setPackingKg(double v) { if (std::abs(m_packingKg - v) > 0.001) { m_packingKg = v; emit packingKgChanged(); recalculateOpeningValue(); } }

    int openingBags() const { return m_openingBags; }
    void setOpeningBags(int v);

    double openingQty() const { return m_openingQty; }
    void setOpeningQty(double v);

    double openingRate() const { return m_openingRate; }
    void setOpeningRate(double v);

    double openingValue() const { return m_openingValue; }
    QString openingValueFmt() const;

    QString purchaseLedger() const { return m_purchaseLedger; }
    void setPurchaseLedger(const QString &v) { if (m_purchaseLedger != v) { m_purchaseLedger = v; emit purchaseLedgerChanged(); } }

    QString saleLedger() const { return m_saleLedger; }
    void setSaleLedger(const QString &v) { if (m_saleLedger != v) { m_saleLedger = v; emit saleLedgerChanged(); } }

    QString stockLedger() const { return m_stockLedger; }
    void setStockLedger(const QString &v) { if (m_stockLedger != v) { m_stockLedger = v; emit stockLedgerChanged(); } }

    QString statusMessage() const { return m_statusMessage; }
    bool isError() const { return m_isError; }

    // Invokables
    Q_INVOKABLE void resetForm();
    Q_INVOKABLE bool loadItem(int id);
    Q_INVOKABLE bool loadItemByName(const QString &name);
    Q_INVOKABLE void recalculateOpeningValue();
    Q_INVOKABLE bool saveItem();
    Q_INVOKABLE bool deleteItem(int id);
    Q_INVOKABLE bool addStockGroup(const QString &groupName);

signals:
    void itemIdChanged();
    void isEditModeChanged();
    void nameChanged();
    void codeChanged();
    void itemTypeChanged();
    void goodsTypeChanged();
    void companyNameChanged();
    void stockGroupChanged();
    void unitChanged();
    void purchaseRateChanged();
    void saleRateChanged();
    void mrpChanged();
    void discountChanged();
    void hsnCodeChanged();
    void gstRateChanged();
    void cessRateChanged();
    void packingKgChanged();
    void openingStockChanged();
    void purchaseLedgerChanged();
    void saleLedgerChanged();
    void stockLedgerChanged();
    void statusChanged();
    void itemSaved();
    void itemDeleted();

private:
    int m_itemId = 0;

    QString m_name;
    QString m_code;
    QString m_itemType = "Paddy";
    QString m_goodsType = "Goods";
    QString m_companyName;
    QString m_stockGroup = "Raw Material";
    QString m_unit = "QTL";

    double m_purchaseRate = 0.0;
    double m_saleRate = 0.0;
    double m_mrp = 0.0;
    double m_discount = 0.0;
    QString m_hsnCode = "1006";
    double m_gstRate = 5.0;
    double m_cessRate = 0.0;

    double m_packingKg = 50.0;

    int m_openingBags = 0;
    double m_openingQty = 0.0;
    double m_openingRate = 0.0;
    double m_openingValue = 0.0;

    QString m_purchaseLedger = "Purchase Account";
    QString m_saleLedger = "Sales Account";
    QString m_stockLedger = "Stock In Hand";

    QString m_statusMessage;
    bool m_isError = false;
};
