#include "manufacturing_batch_controller.h"
#include "manufacturing_model.h"
#include "stock_items_model.h"
#include "financial_years_model.h"
#include "../database_manager.h"
#include "../engine/stock_pipeline.h"
#include "../services/accounting_date_service.h"
#include "../services/financial_math_service.h"
#include <cmath>

// -------------------------------------------------------------
// ManufacturingConsumedModel
// -------------------------------------------------------------
ManufacturingConsumedModel::ManufacturingConsumedModel(QObject *parent)
    : QAbstractListModel(parent) {
}

int ManufacturingConsumedModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_items.size();
}

QVariant ManufacturingConsumedModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return QVariant();
    }

    const auto &item = m_items.at(index.row());
    switch (role) {
    case ItemNameRole: return item.itemName;
    case BagsRole: return item.bags > 0 ? QString::number(item.bags) : "";
    case WeightRole: return item.weight > 0.0001 ? QString::number(item.weight, 'f', 3) : "";
    case AmountRole: return item.amount > 0.0001 ? QString::number(item.amount, 'f', 2) : "";
    default: return QVariant();
    }
}

bool ManufacturingConsumedModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return false;
    }

    auto &item = m_items[index.row()];
    switch (role) {
    case ItemNameRole: item.itemName = value.toString(); break;
    case BagsRole: item.bags = value.toInt(); break;
    case WeightRole: item.weight = value.toDouble(); break;
    case AmountRole: item.amount = value.toDouble(); break;
    default: return false;
    }

    emit dataChanged(index, index, {role});
    emit itemsChanged();
    return true;
}

QHash<int, QByteArray> ManufacturingConsumedModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[ItemNameRole] = "itemName";
    roles[BagsRole] = "bags";
    roles[WeightRole] = "weight";
    roles[AmountRole] = "amount";
    return roles;
}

void ManufacturingConsumedModel::appendRow(const QString &itemName, int bags, double weight, double amount) {
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.append({itemName, bags, weight, amount});
    endInsertRows();
    emit itemsChanged();
}

void ManufacturingConsumedModel::removeRowAt(int index) {
    if (index < 0 || index >= m_items.size()) return;
    beginRemoveRows(QModelIndex(), index, index);
    m_items.removeAt(index);
    endRemoveRows();
    emit itemsChanged();
}

void ManufacturingConsumedModel::clear() {
    beginResetModel();
    m_items.clear();
    endResetModel();
    emit itemsChanged();
}

QVariantMap ManufacturingConsumedModel::getRow(int index) const {
    QVariantMap map;
    if (index < 0 || index >= m_items.size()) return map;
    const auto &item = m_items.at(index);
    map["itemName"] = item.itemName;
    map["bags"] = item.bags;
    map["weight"] = item.weight;
    map["amount"] = item.amount;
    return map;
}

void ManufacturingConsumedModel::setRowProperty(int index, const QString &property, const QVariant &value) {
    if (index < 0 || index >= m_items.size()) return;
    auto &item = m_items[index];
    int role = -1;

    if (property == "itemName") { item.itemName = value.toString(); role = ItemNameRole; }
    else if (property == "bags") { item.bags = value.toInt(); role = BagsRole; }
    else if (property == "weight") {
        item.weight = value.typeId() == QMetaType::QString ? value.toString().toDouble() : value.toDouble();
        role = WeightRole;
    }
    else if (property == "amount") {
        item.amount = value.typeId() == QMetaType::QString ? value.toString().toDouble() : value.toDouble();
        role = AmountRole;
    }

    if (role != -1) {
        QModelIndex idx = this->index(index, 0);
        emit dataChanged(idx, idx, {role});
        emit itemsChanged();
    }
}

// -------------------------------------------------------------
// ManufacturingProducedModel
// -------------------------------------------------------------
ManufacturingProducedModel::ManufacturingProducedModel(QObject *parent)
    : QAbstractListModel(parent) {
}

int ManufacturingProducedModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_items.size();
}

QVariant ManufacturingProducedModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return QVariant();
    }

    const auto &item = m_items.at(index.row());
    switch (role) {
    case ItemNameRole: return item.itemName;
    case YieldPctRole: return item.yieldPct > 0.0001 ? QString::number(item.yieldPct, 'f', 2) : "";
    case BagsRole: return item.bags > 0 ? QString::number(item.bags) : "";
    case WeightRole: return item.weight > 0.0001 ? QString::number(item.weight, 'f', 3) : "";
    case AmountRole: return item.amount > 0.0001 ? QString::number(item.amount, 'f', 2) : "";
    default: return QVariant();
    }
}

bool ManufacturingProducedModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return false;
    }

    auto &item = m_items[index.row()];
    switch (role) {
    case ItemNameRole: item.itemName = value.toString(); break;
    case YieldPctRole: item.yieldPct = value.toDouble(); break;
    case BagsRole: item.bags = value.toInt(); break;
    case WeightRole: item.weight = value.toDouble(); break;
    case AmountRole: item.amount = value.toDouble(); break;
    default: return false;
    }

    emit dataChanged(index, index, {role});
    emit itemsChanged();
    return true;
}

QHash<int, QByteArray> ManufacturingProducedModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[ItemNameRole] = "itemName";
    roles[YieldPctRole] = "yieldPct";
    roles[BagsRole] = "bags";
    roles[WeightRole] = "weight";
    roles[AmountRole] = "amount";
    return roles;
}

void ManufacturingProducedModel::appendRow(const QString &itemName, double yieldPct, int bags, double weight, double amount) {
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.append({itemName, yieldPct, bags, weight, amount});
    endInsertRows();
    emit itemsChanged();
}

void ManufacturingProducedModel::removeRowAt(int index) {
    if (index < 0 || index >= m_items.size()) return;
    beginRemoveRows(QModelIndex(), index, index);
    m_items.removeAt(index);
    endRemoveRows();
    emit itemsChanged();
}

void ManufacturingProducedModel::clear() {
    beginResetModel();
    m_items.clear();
    endResetModel();
    emit itemsChanged();
}

QVariantMap ManufacturingProducedModel::getRow(int index) const {
    QVariantMap map;
    if (index < 0 || index >= m_items.size()) return map;
    const auto &item = m_items.at(index);
    map["itemName"] = item.itemName;
    map["yieldPct"] = item.yieldPct;
    map["bags"] = item.bags;
    map["weight"] = item.weight;
    map["amount"] = item.amount;
    return map;
}

void ManufacturingProducedModel::setRowProperty(int index, const QString &property, const QVariant &value) {
    if (index < 0 || index >= m_items.size()) return;
    auto &item = m_items[index];
    int role = -1;

    if (property == "itemName") { item.itemName = value.toString(); role = ItemNameRole; }
    else if (property == "yieldPct") {
        item.yieldPct = value.typeId() == QMetaType::QString ? value.toString().toDouble() : value.toDouble();
        role = YieldPctRole;
    }
    else if (property == "bags") { item.bags = value.toInt(); role = BagsRole; }
    else if (property == "weight") {
        item.weight = value.typeId() == QMetaType::QString ? value.toString().toDouble() : value.toDouble();
        role = WeightRole;
    }
    else if (property == "amount") {
        item.amount = value.typeId() == QMetaType::QString ? value.toString().toDouble() : value.toDouble();
        role = AmountRole;
    }

    if (role != -1) {
        QModelIndex idx = this->index(index, 0);
        emit dataChanged(idx, idx, {role});
        emit itemsChanged();
    }
}

// -------------------------------------------------------------
// ManufacturingBatchController
// -------------------------------------------------------------
ManufacturingBatchController::ManufacturingBatchController(QObject *parent)
    : QObject(parent) {
    connect(&m_consumedModel, &ManufacturingConsumedModel::itemsChanged, this, &ManufacturingBatchController::recalculateTotals);
    connect(&m_producedModel, &ManufacturingProducedModel::itemsChanged, this, &ManufacturingBatchController::recalculateTotals);
}

void ManufacturingBatchController::setBatchDate(const QString &v) {
    QString resolved = AccountingDateService::instance().resolveDate(v);
    if (m_batchDate != resolved) {
        m_batchDate = resolved;
        emit batchDateChanged();
        emit dayOfWeekChanged();
    }
}

QString ManufacturingBatchController::dayOfWeek() const {
    return AccountingDateService::instance().getDayOfWeek(m_batchDate);
}

void ManufacturingBatchController::resetForm(const QString &workingDate) {
    ManufacturingModel mModel;
    m_batchNo = mModel.get_next_batch_no();
    emit batchNoChanged();

    FinancialYearsModel fyModel;
    QString wDate = !workingDate.isEmpty() ? workingDate : fyModel.get_working_date();
    if (wDate.isEmpty()) {
        wDate = QDate::currentDate().toString("dd/MM/yyyy");
    }
    setBatchDate(wDate);

    m_notes.clear();
    emit notesChanged();

    m_consumedModel.clear();
    m_producedModel.clear();

    m_consumedModel.appendRow("", 0, 0.0, 0.0);
    m_producedModel.appendRow("", 0.0, 0, 0.0, 0.0);

    m_statusMessage.clear();
    m_isError = false;
    emit statusChanged();

    recalculateTotals();
}

void ManufacturingBatchController::autoPickStandardItems() {
    m_producedModel.clear();
    // Data-driven: pick the firm's role-tagged manufacturing items (finished
    // output first, then by-products). No rice-mill yield template — a
    // dal / oil / plastic manufacturer auto-picks the same way once its
    // items carry a manufacturing_role.
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT name FROM stock_items "
        "WHERE manufacturing_role IN ('output','byproduct') "
        "ORDER BY CASE manufacturing_role WHEN 'output' THEN 0 ELSE 1 END, name;");
    for (const QVariant& v : rows) {
        m_producedModel.appendRow(v.toMap().value("name").toString(), 0.0, 0, 0.0, 0.0);
    }
    if (rows.isEmpty()) {
        m_producedModel.appendRow("", 0.0, 0, 0.0, 0.0);
    }
    recalculateTotals();
}

void ManufacturingBatchController::addConsumedRow() {
    m_consumedModel.appendRow("", 0, 0.0, 0.0);
    recalculateTotals();
}

void ManufacturingBatchController::removeConsumedRow(int idx) {
    if (m_consumedModel.count() > 1) {
        m_consumedModel.removeRowAt(idx);
        recalculateTotals();
    }
}

void ManufacturingBatchController::addProducedRow() {
    m_producedModel.appendRow("", 0.0, 0, 0.0, 0.0);
    recalculateTotals();
}

void ManufacturingBatchController::removeProducedRow(int idx) {
    if (m_producedModel.count() > 1) {
        m_producedModel.removeRowAt(idx);
        recalculateTotals();
    }
}

void ManufacturingBatchController::updateProducedFromYields() {
    if (m_totalConsumedWeight <= 0) return;
    StockItemsModel sModel;

    for (int i = 0; i < m_producedModel.count(); ++i) {
        auto pr = m_producedModel.getRow(i);
        double yVal = pr.value("yieldPct").toDouble();
        if (yVal > 0) {
            double autoW = (m_totalConsumedWeight * yVal / 100.0);
            QString itemName = pr.value("itemName").toString();
            double pkgQtl = 0.500;
            if (!itemName.isEmpty()) {
                auto item = sModel.get_item_by_name(itemName);
                if (item.contains("packing_kg")) {
                    double pkg = item.value("packing_kg").toDouble();
                    pkgQtl = pkg > 2.0 ? pkg / 100.0 : (pkg > 0 ? pkg : 0.500);
                }
            }
            m_producedModel.setRowProperty(i, "weight", autoW);
            m_producedModel.setRowProperty(i, "bags", std::round(autoW / pkgQtl));
        }
    }
    recalculateTotals();
}

void ManufacturingBatchController::recalculateTotals() {
    int cBags = 0;
    double cWt = 0.0;
    double cAmt = 0.0;

    for (int i = 0; i < m_consumedModel.count(); ++i) {
        auto cr = m_consumedModel.getRow(i);
        cBags += cr.value("bags").toInt();
        cWt += cr.value("weight").toDouble();
        cAmt += cr.value("amount").toDouble();
    }

    m_totalConsumedBags = cBags;
    m_totalConsumedWeight = FinancialMathService::instance().round3(cWt);
    m_totalConsumedAmount = FinancialMathService::instance().round2(cAmt);

    double pPct = 0.0;
    int pBags = 0;
    double pWt = 0.0;
    double pAmt = 0.0;

    for (int j = 0; j < m_producedModel.count(); ++j) {
        auto pr = m_producedModel.getRow(j);
        int pb = pr.value("bags").toInt();
        double pw = pr.value("weight").toDouble();
        double pa = pr.value("amount").toDouble();
        double yVal = pr.value("yieldPct").toDouble();

        if (cWt > 0 && pw > 0) {
            yVal = (pw / cWt * 100.0);
        }

        pPct += yVal;
        pBags += pb;
        pWt += pw;
        pAmt += pa;
    }

    m_totalProducedYieldPct = FinancialMathService::instance().round3(pPct);
    m_totalProducedBags = pBags;
    m_totalProducedWeight = FinancialMathService::instance().round3(pWt);
    m_totalProducedAmount = FinancialMathService::instance().round2(pAmt);

    m_shortagePct = std::max(0.0, FinancialMathService::instance().round3((100.0 - m_totalProducedYieldPct)));
    m_shortageWeight = std::max(0.0, FinancialMathService::instance().round3((m_totalConsumedWeight - m_totalProducedWeight)));
    m_shortageBags = std::max(0, m_totalConsumedBags - m_totalProducedBags);

    emit totalsChanged();
}

bool ManufacturingBatchController::saveVoucher() {
    m_statusMessage.clear();
    m_isError = false;

    if (m_totalConsumedWeight <= 0) {
        m_statusMessage = "Please enter a valid raw-material input weight.";
        m_isError = true;
        emit statusChanged();
        return false;
    }

    if (m_totalProducedWeight <= 0) {
        m_statusMessage = "Please enter a valid produced-items weight.";
        m_isError = true;
        emit statusChanged();
        return false;
    }

    // FY date check
    FinancialYearsModel fyModel;
    auto activeFy = fyModel.get_active_year();
    int fyStartYear = QDate::fromString(activeFy.value("start_date").toString(), "yyyy-MM-dd").year();
    if (fyStartYear <= 0) fyStartYear = 2026;

    if (!AccountingDateService::instance().validateDateInFy(m_batchDate, fyStartYear)) {
        m_statusMessage = "Batch Date is outside the active Financial Year.";
        m_isError = true;
        emit statusChanged();
        return false;
    }

    QString isoDate = AccountingDateService::instance().toIso(m_batchDate);

    // Extract primary input item (data-driven; no rice assumption)
    auto cFirst = m_consumedModel.getRow(0);
    QString inputItem = cFirst.value("itemName").toString().trimmed();
    if (inputItem.isEmpty()) inputItem = "Raw Material";

    // Produced lines pass straight through — the model splits finished
    // output vs by-products by each item's manufacturing_role.
    QVariantList produced;
    for (int j = 0; j < m_producedModel.count(); ++j) {
        auto pr = m_producedModel.getRow(j);
        if (pr.value("itemName").toString().trimmed().isEmpty()) continue;
        produced.append(pr);
    }

    ManufacturingModel mModel;
    bool ok = mModel.add_manufacturing_voucher_full(
        m_batchNo, isoDate, inputItem, m_totalConsumedBags, m_totalConsumedWeight,
        produced, m_shortageWeight, m_shortagePct, 0.0, m_totalConsumedAmount,
        "System Admin", m_notes.trimmed()
    );

    if (ok) {
        m_statusMessage = QString("Manufacturing Batch %1 saved & posted successfully!").arg(m_batchNo);
        m_isError = false;
        emit statusChanged();
        emit voucherSaved();
        resetForm();
        return true;
    } else {
        m_statusMessage = "Failed to save Manufacturing Batch in database.";
        m_isError = true;
        emit statusChanged();
        return false;
    }
}
