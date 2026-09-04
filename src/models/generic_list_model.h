#pragma once

#include <QAbstractListModel>
#include <QVariantMap>
#include <QHash>
#include <QByteArray>
#include <QList>
#include <QtQml/qqmlregistration.h>

class GenericListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    explicit GenericListModel(QObject* parent = nullptr);
    ~GenericListModel() override = default;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void clear();
    Q_INVOKABLE void append(const QVariantMap& item);
    Q_INVOKABLE void insert(int index, const QVariantMap& item);
    Q_INVOKABLE void remove(int index, int count = 1);
    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE void setProperty(int index, const QString& propertyName, const QVariant& value);

signals:
    void countChanged();

private:
    bool ensureRoles(const QVariantMap& item);

    QList<QVariantMap> m_items;
    QHash<int, QByteArray> m_roles;
    QHash<QByteArray, int> m_roleIds;
};
