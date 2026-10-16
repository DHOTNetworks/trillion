#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>
#include <QVariantMap>
#include <QVariantList>

struct ManufacturingBatchEntry {
    int id = 0;
    QString batchNo;
    QString batchDate;
    QString inputVariety;
    QString inputQty;
    QString outputQty;
    QString byproductQty;
    QString wastage;
    QString yieldPct;
    QString narration;
};

struct ManufacturingItemEntry {
    int rowNo = 0;
    QString drcr; // "Dr" (produced) or "Cr" (consumed)
    bool isInput = false;
    QString itemName;
    QString yieldPct;
    QString bags;
    QString weight;
    QString rate;
    QString amount;
    QString narration;
};

class ManufacturingBatchListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        BatchNoRole,
        BatchDateRole,
        InputVarietyRole,
        InputQtyRole,
        OutputQtyRole,
        ByproductQtyRole,
        WastageRole,
        YieldPctRole,
        NarrationRole
    };

    explicit ManufacturingBatchListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(const QVector<ManufacturingBatchEntry>& entries);
    void clear();
    Q_INVOKABLE QVariantMap get(int row) const;

signals:
    void countChanged();

private:
    QVector<ManufacturingBatchEntry> m_entries;
};

class ManufacturingItemListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        RowNoRole = Qt::UserRole + 1,
        DrcrRole,
        IsInputRole,
        ItemNameRole,
        YieldPctRole,
        BagsRole,
        WeightRole,
        RateRole,
        AmountRole,
        NarrationRole
    };

    explicit ManufacturingItemListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(const QVector<ManufacturingItemEntry>& entries);
    void clear();
    Q_INVOKABLE QVariantMap get(int row) const;

signals:
    void countChanged();

private:
    QVector<ManufacturingItemEntry> m_entries;
};

class ManufacturingStatementController : public QObject {
    Q_OBJECT
    Q_PROPERTY(ManufacturingBatchListModel* batchModel READ batchModel CONSTANT)
    Q_PROPERTY(ManufacturingItemListModel* itemModel READ itemModel CONSTANT)

    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
    Q_PROPERTY(QString selectedVariety READ selectedVariety WRITE setSelectedVariety NOTIFY selectedVarietyChanged)
    Q_PROPERTY(int selectedBatchIndex READ selectedBatchIndex WRITE setSelectedBatchIndex NOTIFY selectedBatchIndexChanged)
    Q_PROPERTY(QString activeBatchNo READ activeBatchNo NOTIFY activeBatchNoChanged)

    Q_PROPERTY(int totalBatchesCount READ totalBatchesCount NOTIFY totalsChanged)
    Q_PROPERTY(QString totalInputFmt READ totalInputFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString totalOutputFmt READ totalOutputFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString totalByproductFmt READ totalByproductFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString totalWastageFmt READ totalWastageFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString avgYieldPctFmt READ avgYieldPctFmt NOTIFY totalsChanged)

public:
    explicit ManufacturingStatementController(QObject* parent = nullptr);

    ManufacturingBatchListModel* batchModel() { return &m_batchModel; }
    ManufacturingItemListModel* itemModel() { return &m_itemModel; }

    QString searchQuery() const { return m_searchQuery; }
    void setSearchQuery(const QString& query);

    QString selectedVariety() const { return m_selectedVariety; }
    void setSelectedVariety(const QString& variety);

    int selectedBatchIndex() const { return m_selectedBatchIndex; }
    void setSelectedBatchIndex(int idx);

    QString activeBatchNo() const { return m_activeBatchNo; }

    int totalBatchesCount() const { return m_totalBatchesCount; }
    QString totalInputFmt() const { return m_totalInputFmt; }
    QString totalOutputFmt() const { return m_totalOutputFmt; }
    QString totalByproductFmt() const { return m_totalByproductFmt; }
    QString totalWastageFmt() const { return m_totalWastageFmt; }
    QString avgYieldPctFmt() const { return m_avgYieldPctFmt; }

    Q_INVOKABLE void reload(const QString& fromDate = "", const QString& toDate = "", const QString& variety = "");
    Q_INVOKABLE void selectBatch(int index);

signals:
    Q_SIGNAL void searchQueryChanged();
    Q_SIGNAL void selectedVarietyChanged();
    Q_SIGNAL void selectedBatchIndexChanged();
    Q_SIGNAL void activeBatchNoChanged();
    Q_SIGNAL void totalsChanged();

private:
    void applyFilter();
    void loadBatchItems(int batchId, const QString& batchNo);

    ManufacturingBatchListModel m_batchModel;
    ManufacturingItemListModel m_itemModel;

    QVector<ManufacturingBatchEntry> m_allBatches;
    QString m_searchQuery;
    QString m_selectedVariety = "All Varieties";
    int m_selectedBatchIndex = -1;
    QString m_activeBatchNo;

    int m_totalBatchesCount = 0;
    QString m_totalInputFmt = "0.00";
    QString m_totalOutputFmt = "0.00";
    QString m_totalByproductFmt = "0.00";
    QString m_totalWastageFmt = "0.00";
    QString m_avgYieldPctFmt = "0.00%";
};
