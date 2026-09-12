#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

struct MillingConsumedItem {
    QString itemName;
    int bags = 0;
    double weight = 0.0;
    double amount = 0.0;
};

struct MillingProducedItem {
    QString itemName;
    double yieldPct = 0.0;
    int bags = 0;
    double weight = 0.0;
    double amount = 0.0;
};

class MillingConsumedModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        ItemNameRole = Qt::UserRole + 1,
        BagsRole,
        WeightRole,
        AmountRole
    };

    explicit MillingConsumedModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void appendRow(const QString &itemName = "", int bags = 0, double weight = 0.0, double amount = 0.0);
    Q_INVOKABLE void removeRowAt(int index);
    Q_INVOKABLE void clear();
    Q_INVOKABLE int count() const { return m_items.size(); }
    Q_INVOKABLE QVariantMap getRow(int index) const;
    Q_INVOKABLE void setRowProperty(int index, const QString &property, const QVariant &value);

    const QVector<MillingConsumedItem>& items() const { return m_items; }

signals:
    void itemsChanged();

private:
    QVector<MillingConsumedItem> m_items;
};

class MillingProducedModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        ItemNameRole = Qt::UserRole + 1,
        YieldPctRole,
        BagsRole,
        WeightRole,
        AmountRole
    };

    explicit MillingProducedModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void appendRow(const QString &itemName = "", double yieldPct = 0.0, int bags = 0, double weight = 0.0, double amount = 0.0);
    Q_INVOKABLE void removeRowAt(int index);
    Q_INVOKABLE void clear();
    Q_INVOKABLE int count() const { return m_items.size(); }
    Q_INVOKABLE QVariantMap getRow(int index) const;
    Q_INVOKABLE void setRowProperty(int index, const QString &property, const QVariant &value);

    const QVector<MillingProducedItem>& items() const { return m_items; }

signals:
    void itemsChanged();

private:
    QVector<MillingProducedItem> m_items;
};

class MillingBatchController : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString batchNo READ batchNo WRITE setBatchNo NOTIFY batchNoChanged)
    Q_PROPERTY(QString batchDate READ batchDate WRITE setBatchDate NOTIFY batchDateChanged)
    Q_PROPERTY(QString dayOfWeek READ dayOfWeek NOTIFY dayOfWeekChanged)
    Q_PROPERTY(QString notes READ notes WRITE setNotes NOTIFY notesChanged)

    Q_PROPERTY(int totalConsumedBags READ totalConsumedBags NOTIFY totalsChanged)
    Q_PROPERTY(double totalConsumedWeight READ totalConsumedWeight NOTIFY totalsChanged)
    Q_PROPERTY(double totalConsumedAmount READ totalConsumedAmount NOTIFY totalsChanged)

    Q_PROPERTY(double totalProducedYieldPct READ totalProducedYieldPct NOTIFY totalsChanged)
    Q_PROPERTY(int totalProducedBags READ totalProducedBags NOTIFY totalsChanged)
    Q_PROPERTY(double totalProducedWeight READ totalProducedWeight NOTIFY totalsChanged)
    Q_PROPERTY(double totalProducedAmount READ totalProducedAmount NOTIFY totalsChanged)

    Q_PROPERTY(double shortagePct READ shortagePct NOTIFY totalsChanged)
    Q_PROPERTY(double shortageWeight READ shortageWeight NOTIFY totalsChanged)
    Q_PROPERTY(int shortageBags READ shortageBags NOTIFY totalsChanged)

    Q_PROPERTY(MillingConsumedModel* consumedModel READ consumedModel CONSTANT)
    Q_PROPERTY(MillingProducedModel* producedModel READ producedModel CONSTANT)

    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(bool isError READ isError NOTIFY statusChanged)

public:
    explicit MillingBatchController(QObject *parent = nullptr);

    QString batchNo() const { return m_batchNo; }
    void setBatchNo(const QString &v) { if (m_batchNo != v) { m_batchNo = v; emit batchNoChanged(); } }

    QString batchDate() const { return m_batchDate; }
    void setBatchDate(const QString &v);

    QString dayOfWeek() const;

    QString notes() const { return m_notes; }
    void setNotes(const QString &v) { if (m_notes != v) { m_notes = v; emit notesChanged(); } }

    int totalConsumedBags() const { return m_totalConsumedBags; }
    double totalConsumedWeight() const { return m_totalConsumedWeight; }
    double totalConsumedAmount() const { return m_totalConsumedAmount; }

    double totalProducedYieldPct() const { return m_totalProducedYieldPct; }
    int totalProducedBags() const { return m_totalProducedBags; }
    double totalProducedWeight() const { return m_totalProducedWeight; }
    double totalProducedAmount() const { return m_totalProducedAmount; }

    double shortagePct() const { return m_shortagePct; }
    double shortageWeight() const { return m_shortageWeight; }
    int shortageBags() const { return m_shortageBags; }

    MillingConsumedModel* consumedModel() { return &m_consumedModel; }
    MillingProducedModel* producedModel() { return &m_producedModel; }

    QString statusMessage() const { return m_statusMessage; }
    bool isError() const { return m_isError; }

    Q_INVOKABLE void resetForm(const QString &workingDate = "");
    Q_INVOKABLE void autoPickStandardItems();
    Q_INVOKABLE void addConsumedRow();
    Q_INVOKABLE void removeConsumedRow(int idx);
    Q_INVOKABLE void addProducedRow();
    Q_INVOKABLE void removeProducedRow(int idx);
    Q_INVOKABLE void updateProducedFromYields();
    Q_INVOKABLE void recalculateTotals();
    Q_INVOKABLE bool saveVoucher();

signals:
    void batchNoChanged();
    void batchDateChanged();
    void dayOfWeekChanged();
    void notesChanged();
    void totalsChanged();
    void statusChanged();
    void voucherSaved();

private:
    QString m_batchNo;
    QString m_batchDate;
    QString m_notes;

    int m_totalConsumedBags = 0;
    double m_totalConsumedWeight = 0.0;
    double m_totalConsumedAmount = 0.0;

    double m_totalProducedYieldPct = 0.0;
    int m_totalProducedBags = 0;
    double m_totalProducedWeight = 0.0;
    double m_totalProducedAmount = 0.0;

    double m_shortagePct = 100.0;
    double m_shortageWeight = 0.0;
    int m_shortageBags = 0;

    QString m_statusMessage;
    bool m_isError = false;

    MillingConsumedModel m_consumedModel;
    MillingProducedModel m_producedModel;
};
