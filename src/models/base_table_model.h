#pragma once

#include <QAbstractTableModel>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QHash>
#include <QByteArray>

class BaseTableModel : public QAbstractTableModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    explicit BaseTableModel(const QStringList& headers, const QStringList& roleKeys, QObject* parent = nullptr);
    virtual ~BaseTableModel() = default;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get_row(int row) const;
    Q_INVOKABLE QVariantList get_all_data() const;
    virtual void reload_data() = 0;

signals:
    void dataChangedSignal();
    void countChanged();

protected:
    QStringList m_headers;
    QStringList m_roleKeys;
    QVariantList m_data;
};
