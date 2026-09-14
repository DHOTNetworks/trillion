#include "generic_list_model.h"

GenericListModel::GenericListModel(QObject* parent)
    : QAbstractListModel(parent)
{
    const QStringList standardRoles = {
        "id", "itemName", "item_name", "itemCode", "item_code", "hsnCode", "hsn_code",
        "bags", "bag_count", "packing", "pkng", "weight", "weight_qtl", "loose", "loose_weight",
        "rate", "rate_per_qtl", "amount", "taxable_amount", "total_amount", "gstPct", "gst_pct",
        "cgst", "cgst_amount", "sgst", "sgst_amount", "igst", "igst_amount",
        "drCr", "drcr", "side", "account", "accountName", "partyName", "party_name",
        "ledgerName", "ledger_name", "debitAmt", "debit_amt", "creditAmt", "credit_amt",
        "yieldPct", "yield_pct", "yield_percentage", "loss_weight", "loss_pct", "cost",
        "particulars", "narration", "remarks", "notes", "date", "vDate", "vIso", "refNo", "voucherNo", "voucher_no",
        "invoiceNo", "invoice_no", "status", "selected", "isSelected", "type", "voucherType",
        "batchNo", "batch_no", "issueDate", "totalInputBags", "totalInputWeight", "totalOutputBags", "totalOutputWeight",
        "paddyVariety", "paddy_variety", "paddyInput", "paddy_input", "headRice", "head_rice",
        "brokenRice", "broken_rice", "bran", "husk", "wastage"
    };
    for (const QString& r : standardRoles) {
        QByteArray roleName = r.toUtf8();
        int newRoleId = Qt::UserRole + 1 + static_cast<int>(m_roleIds.size());
        m_roleIds[roleName] = newRoleId;
        m_roles[newRoleId] = roleName;
    }
}

int GenericListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_items.size());
}

QVariant GenericListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();

    QByteArray roleName = m_roles.value(role);
    if (roleName.isEmpty()) return QVariant();
    return m_items.at(index.row()).value(QString::fromUtf8(roleName));
}

QHash<int, QByteArray> GenericListModel::roleNames() const {
    return m_roles;
}

bool GenericListModel::ensureRoles(const QVariantMap& item) {
    bool roleAdded = false;
    for (auto it = item.constBegin(); it != item.constEnd(); ++it) {
        QByteArray roleName = it.key().toUtf8();
        if (!m_roleIds.contains(roleName)) {
            int newRoleId = Qt::UserRole + 1 + static_cast<int>(m_roleIds.size());
            m_roleIds[roleName] = newRoleId;
            m_roles[newRoleId] = roleName;
            roleAdded = true;
        }
    }
    return roleAdded;
}

void GenericListModel::clear() {
    if (m_items.isEmpty()) return;
    beginResetModel();
    m_items.clear();
    endResetModel();
    emit countChanged();
}

void GenericListModel::append(const QVariantMap& item) {
    bool roleAdded = ensureRoles(item);
    int newRow = static_cast<int>(m_items.size());

    if (roleAdded) {
        beginResetModel();
        m_items.append(item);
        endResetModel();
    } else {
        beginInsertRows(QModelIndex(), newRow, newRow);
        m_items.append(item);
        endInsertRows();
    }
    emit countChanged();
}

void GenericListModel::appendList(const QVariantList& items) {
    if (items.isEmpty()) return;
    for (const auto& it : items) {
        ensureRoles(it.toMap());
    }
    beginResetModel();
    for (const auto& it : items) {
        m_items.append(it.toMap());
    }
    endResetModel();
    emit countChanged();
}

void GenericListModel::resetWithList(const QVariantList& items) {
    beginResetModel();
    m_items.clear();
    for (const auto& it : items) {
        QVariantMap map = it.toMap();
        ensureRoles(map);
        m_items.append(map);
    }
    endResetModel();
    emit countChanged();
}

void GenericListModel::insert(int index, const QVariantMap& item) {
    if (index < 0 || index > m_items.size()) return;
    bool roleAdded = ensureRoles(item);

    if (roleAdded) {
        beginResetModel();
        m_items.insert(index, item);
        endResetModel();
    } else {
        beginInsertRows(QModelIndex(), index, index);
        m_items.insert(index, item);
        endInsertRows();
    }
    emit countChanged();
}

void GenericListModel::remove(int index, int count) {
    if (index < 0 || index + count > m_items.size() || count <= 0) return;
    beginRemoveRows(QModelIndex(), index, index + count - 1);
    for (int i = 0; i < count; ++i) {
        m_items.removeAt(index);
    }
    endRemoveRows();
    emit countChanged();
}

QVariantMap GenericListModel::get(int index) const {
    if (index < 0 || index >= m_items.size()) return QVariantMap();
    return m_items.at(index);
}

void GenericListModel::setProperty(int index, const QString& propertyName, const QVariant& value) {
    if (index < 0 || index >= m_items.size()) return;
    QByteArray roleName = propertyName.toUtf8();
    if (!m_roleIds.contains(roleName)) {
        int newRoleId = Qt::UserRole + 1 + static_cast<int>(m_roleIds.size());
        m_roleIds[roleName] = newRoleId;
        m_roles[newRoleId] = roleName;
    }
    m_items[index][propertyName] = value;
    int roleId = m_roleIds.value(roleName);
    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex, { roleId });
}
