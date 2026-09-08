#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>
#include <QVariantMap>
#include <QVariantList>

struct MillingBatchEntry {
    int id = 0;
    QString batchNo;
    QString batchDate;
    QString paddyVariety;
    QString paddyInput;
    QString headRice;
    QString brokenRice;
    QString bran;
    QString husk;
    QString wastage;
    QString yieldPct;
    QString narration;
};

struct MillingItemEntry {
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

class MillingBatchListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        BatchNoRole,
        BatchDateRole,
        PaddyVarietyRole,
        PaddyInputRole,
        HeadRiceRole,
        BrokenRiceRole,
        BranRole,
        HuskRole,
        WastageRole,
        YieldPctRole,
        NarrationRole
    };

    explicit MillingBatchListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(const QVector<MillingBatchEntry>& entries);
    void clear();
    Q_INVOKABLE QVariantMap get(int row) const;

signals:
    void countChanged();

private:
    QVector<MillingBatchEntry> m_entries;
};

class MillingItemListModel : public QAbstractListModel {
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

    explicit MillingItemListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(const QVector<MillingItemEntry>& entries);
    void clear();
    Q_INVOKABLE QVariantMap get(int row) const;

signals:
    void countChanged();

private:
    QVector<MillingItemEntry> m_entries;
};

class MillingStatementController : public QObject {
    Q_OBJECT
    Q_PROPERTY(MillingBatchListModel* batchModel READ batchModel CONSTANT)
    Q_PROPERTY(MillingItemListModel* itemModel READ itemModel CONSTANT)

    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
    Q_PROPERTY(QString selectedVariety READ selectedVariety WRITE setSelectedVariety NOTIFY selectedVarietyChanged)
    Q_PROPERTY(int selectedBatchIndex READ selectedBatchIndex WRITE setSelectedBatchIndex NOTIFY selectedBatchIndexChanged)
    Q_PROPERTY(QString activeBatchNo READ activeBatchNo NOTIFY activeBatchNoChanged)

    Q_PROPERTY(int totalBatchesCount READ totalBatchesCount NOTIFY totalsChanged)
    Q_PROPERTY(QString totalPaddyMilledFmt READ totalPaddyMilledFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString totalHeadRiceProducedFmt READ totalHeadRiceProducedFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString totalBranProducedFmt READ totalBranProducedFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString totalBrokenProducedFmt READ totalBrokenProducedFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString totalHuskProducedFmt READ totalHuskProducedFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString totalWastageMilledFmt READ totalWastageMilledFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString avgYieldPctFmt READ avgYieldPctFmt NOTIFY totalsChanged)

public:
    explicit MillingStatementController(QObject* parent = nullptr);

    MillingBatchListModel* batchModel() { return &m_batchModel; }
    MillingItemListModel* itemModel() { return &m_itemModel; }

    QString searchQuery() const { return m_searchQuery; }
    void setSearchQuery(const QString& query);

    QString selectedVariety() const { return m_selectedVariety; }
    void setSelectedVariety(const QString& variety);

    int selectedBatchIndex() const { return m_selectedBatchIndex; }
    void setSelectedBatchIndex(int idx);

    QString activeBatchNo() const { return m_activeBatchNo; }

    int totalBatchesCount() const { return m_totalBatchesCount; }
    QString totalPaddyMilledFmt() const { return m_totalPaddyMilledFmt; }
    QString totalHeadRiceProducedFmt() const { return m_totalHeadRiceProducedFmt; }
    QString totalBranProducedFmt() const { return m_totalBranProducedFmt; }
    QString totalBrokenProducedFmt() const { return m_totalBrokenProducedFmt; }
    QString totalHuskProducedFmt() const { return m_totalHuskProducedFmt; }
    QString totalWastageMilledFmt() const { return m_totalWastageMilledFmt; }
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

    MillingBatchListModel m_batchModel;
    MillingItemListModel m_itemModel;

    QVector<MillingBatchEntry> m_allBatches;
    QString m_searchQuery;
    QString m_selectedVariety = "All Varieties";
    int m_selectedBatchIndex = -1;
    QString m_activeBatchNo;

    int m_totalBatchesCount = 0;
    QString m_totalPaddyMilledFmt = "0.00";
    QString m_totalHeadRiceProducedFmt = "0.00";
    QString m_totalBranProducedFmt = "0.00";
    QString m_totalBrokenProducedFmt = "0.00";
    QString m_totalHuskProducedFmt = "0.00";
    QString m_totalWastageMilledFmt = "0.00";
    QString m_avgYieldPctFmt = "0.00%";
};
