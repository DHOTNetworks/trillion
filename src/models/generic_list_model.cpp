#include "generic_list_model.h"
#include <QJSValue>

static QVariantMap toSafeVariantMap(const QVariant& v) {
    if (v.canConvert<QJSValue>()) {
        QJSValue jsVal = v.value<QJSValue>();
        QVariant var = jsVal.toVariant();
        if (var.canConvert<QVariantMap>()) {
            return var.toMap();
        }
    }
    if (v.canConvert<QVariantMap>()) {
        return v.toMap();
    }
    return QVariantMap();
}

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

    const QVariantMap& item = m_items.at(index.row());
    QString key = QString::fromUtf8(roleName);
    if (item.contains(key)) {
        return item.value(key);
    }

    // Role fallbacks across camelCase and snake_case
    if (key == "itemName") return item.value("item_name");
    if (key == "item_name") return item.value("itemName");
    if (key == "itemCode") return item.value("item_code");
    if (key == "item_code") return item.value("itemCode");
    if (key == "hsnCode") return item.value("hsn_code");
    if (key == "hsn_code") return item.value("hsnCode");
    if (key == "bags") return item.value("bag_count", item.value("bags"));
    if (key == "bag_count") return item.value("bags", item.value("bag_count"));
    if (key == "weight") return item.value("weight_qtl", item.value("weight"));
    if (key == "weight_qtl") return item.value("weight", item.value("weight_qtl"));
    if (key == "rate") return item.value("rate_per_qtl", item.value("rate"));
    if (key == "rate_per_qtl") return item.value("rate", item.value("rate_per_qtl"));
    if (key == "amount") return item.value("total_amount", item.value("taxable_amount", item.value("amount")));
    if (key == "taxable_amount") return item.value("amount", item.value("total_amount"));
    if (key == "total_amount") return item.value("amount", item.value("taxable_amount"));
    if (key == "gstPct") return item.value("gst_pct", item.value("gstPct"));
    if (key == "gst_pct") return item.value("gstPct", item.value("gst_pct"));
    if (key == "packing") return item.value("pkng", item.value("packing", "0.500"));
    if (key == "pkng") return item.value("packing", item.value("pkng", "0.500"));
    if (key == "partyName") return item.value("party_name");
    if (key == "party_name") return item.value("partyName");
    if (key == "voucherNo") return item.value("voucher_no");
    if (key == "voucher_no") return item.value("voucherNo");
    if (key == "invoiceNo") return item.value("invoice_no");
    if (key == "invoice_no") return item.value("invoiceNo");

    return QVariant();
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

void GenericListModel::append(const QVariant& item) {
    QVariantMap map = toSafeVariantMap(item);
    bool roleAdded = ensureRoles(map);
    int newRow = static_cast<int>(m_items.size());

    if (roleAdded) {
        beginResetModel();
        m_items.append(map);
        endResetModel();
    } else {
        beginInsertRows(QModelIndex(), newRow, newRow);
        m_items.append(map);
        endInsertRows();
    }
    emit countChanged();
}

void GenericListModel::appendList(const QVariantList& items) {
    if (items.isEmpty()) return;
    for (const auto& it : items) {
        ensureRoles(toSafeVariantMap(it));
    }
    beginResetModel();
    for (const auto& it : items) {
        m_items.append(toSafeVariantMap(it));
    }
    endResetModel();
    emit countChanged();
}

void GenericListModel::resetWithList(const QVariantList& items) {
    beginResetModel();
    m_items.clear();
    for (const auto& it : items) {
        QVariantMap map = toSafeVariantMap(it);
        ensureRoles(map);
        m_items.append(map);
    }
    endResetModel();
    emit countChanged();
}

void GenericListModel::insert(int index, const QVariant& item) {
    if (index < 0 || index > m_items.size()) return;
    QVariantMap map = toSafeVariantMap(item);
    bool roleAdded = ensureRoles(map);

    if (roleAdded) {
        beginResetModel();
        m_items.insert(index, map);
        endResetModel();
    } else {
        beginInsertRows(QModelIndex(), index, index);
        m_items.insert(index, map);
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
    QVariantMap item = m_items.at(index);

    // Auto populate common aliases if missing
    if (!item.contains("itemName") && item.contains("item_name")) item["itemName"] = item.value("item_name");
    if (!item.contains("item_name") && item.contains("itemName")) item["item_name"] = item.value("itemName");
    if (!item.contains("bags") && item.contains("bag_count")) item["bags"] = item.value("bag_count");
    if (!item.contains("bag_count") && item.contains("bags")) item["bag_count"] = item.value("bags");
    if (!item.contains("weight") && item.contains("weight_qtl")) item["weight"] = item.value("weight_qtl");
    if (!item.contains("weight_qtl") && item.contains("weight")) item["weight_qtl"] = item.value("weight");
    if (!item.contains("rate") && item.contains("rate_per_qtl")) item["rate"] = item.value("rate_per_qtl");
    if (!item.contains("rate_per_qtl") && item.contains("rate")) item["rate_per_qtl"] = item.value("rate");
    if (!item.contains("amount") && item.contains("total_amount")) item["amount"] = item.value("total_amount");
    if (!item.contains("amount") && item.contains("taxable_amount")) item["amount"] = item.value("taxable_amount");
    if (!item.contains("total_amount") && item.contains("amount")) item["total_amount"] = item.value("amount");
    if (!item.contains("taxable_amount") && item.contains("amount")) item["taxable_amount"] = item.value("amount");
    if (!item.contains("gstPct") && item.contains("gst_pct")) item["gstPct"] = item.value("gst_pct");
    if (!item.contains("gst_pct") && item.contains("gstPct")) item["gst_pct"] = item.value("gstPct");
    if (!item.contains("packing") && item.contains("pkng")) item["packing"] = item.value("pkng");
    if (!item.contains("pkng") && item.contains("packing")) item["pkng"] = item.value("packing");

    return item;
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
