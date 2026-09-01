#include "base_table_model.h"

BaseTableModel::BaseTableModel(const QStringList& headers, const QStringList& roleKeys, QObject* parent)
    : QAbstractTableModel(parent), m_headers(headers), m_roleKeys(roleKeys) {}

int BaseTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_data.size();
}

int BaseTableModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_headers.size();
}

QVariant BaseTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_data.size()) {
        return QVariant();
    }

    const QVariantMap row = m_data.at(index.row()).toMap();

    if (role == Qt::DisplayRole) {
        if (index.column() >= 0 && index.column() < m_roleKeys.size()) {
            return row.value(m_roleKeys.at(index.column()));
        }
    } else if (role >= Qt::UserRole) {
        int roleIndex = role - Qt::UserRole;
        if (roleIndex >= 0 && roleIndex < m_roleKeys.size()) {
            return row.value(m_roleKeys.at(roleIndex));
        }
    }
    return QVariant();
}

QVariant BaseTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (section >= 0 && section < m_headers.size()) {
            return m_headers.at(section);
        }
    }
    return QVariant();
}

QHash<int, QByteArray> BaseTableModel::roleNames() const {
    QHash<int, QByteArray> roles;
    for (int i = 0; i < m_roleKeys.size(); ++i) {
        roles[Qt::UserRole + i] = m_roleKeys.at(i).toUtf8();
    }
    return roles;
}

QVariantMap BaseTableModel::get_row(int row) const {
    if (row >= 0 && row < m_data.size()) {
        return m_data.at(row).toMap();
    }
    return QVariantMap();
}

QVariantList BaseTableModel::get_all_data() const {
    return m_data;
}
