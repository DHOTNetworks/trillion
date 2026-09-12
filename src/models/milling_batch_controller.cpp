#include "milling_batch_controller.h"
#include "milling_model.h"
#include "stock_items_model.h"
#include "financial_years_model.h"
#include "../services/accounting_date_service.h"
#include "../services/financial_math_service.h"
#include <cmath>

// -------------------------------------------------------------
// MillingConsumedModel
// -------------------------------------------------------------
MillingConsumedModel::MillingConsumedModel(QObject *parent)
    : QAbstractListModel(parent) {
}

int MillingConsumedModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_items.size();
}

QVariant MillingConsumedModel::data(const QModelIndex &index, int role) const {
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

bool MillingConsumedModel::setData(const QModelIndex &index, const QVariant &value, int role) {
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

QHash<int, QByteArray> MillingConsumedModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[ItemNameRole] = "itemName";
    roles[BagsRole] = "bags";
    roles[WeightRole] = "weight";
    roles[AmountRole] = "amount";
    return roles;
}

void MillingConsumedModel::appendRow(const QString &itemName, int bags, double weight, double amount) {
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.append({itemName, bags, weight, amount});
    endInsertRows();
    emit itemsChanged();
}

void MillingConsumedModel::removeRowAt(int index) {
    if (index < 0 || index >= m_items.size()) return;
    beginRemoveRows(QModelIndex(), index, index);
    m_items.removeAt(index);
    endRemoveRows();
    emit itemsChanged();
}

void MillingConsumedModel::clear() {
    beginResetModel();
    m_items.clear();
    endResetModel();
    emit itemsChanged();
}

QVariantMap MillingConsumedModel::getRow(int index) const {
    QVariantMap map;
    if (index < 0 || index >= m_items.size()) return map;
    const auto &item = m_items.at(index);
    map["itemName"] = item.itemName;
    map["bags"] = item.bags;
    map["weight"] = item.weight;
    map["amount"] = item.amount;
    return map;
}

void MillingConsumedModel::setRowProperty(int index, const QString &property, const QVariant &value) {
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
// MillingProducedModel
// -------------------------------------------------------------
MillingProducedModel::MillingProducedModel(QObject *parent)
    : QAbstractListModel(parent) {
}

int MillingProducedModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_items.size();
}

QVariant MillingProducedModel::data(const QModelIndex &index, int role) const {
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

bool MillingProducedModel::setData(const QModelIndex &index, const QVariant &value, int role) {
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

QHash<int, QByteArray> MillingProducedModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[ItemNameRole] = "itemName";
    roles[YieldPctRole] = "yieldPct";
    roles[BagsRole] = "bags";
    roles[WeightRole] = "weight";
    roles[AmountRole] = "amount";
    return roles;
}

void MillingProducedModel::appendRow(const QString &itemName, double yieldPct, int bags, double weight, double amount) {
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.append({itemName, yieldPct, bags, weight, amount});
    endInsertRows();
    emit itemsChanged();
}

void MillingProducedModel::removeRowAt(int index) {
    if (index < 0 || index >= m_items.size()) return;
    beginRemoveRows(QModelIndex(), index, index);
    m_items.removeAt(index);
    endRemoveRows();
    emit itemsChanged();
}

void MillingProducedModel::clear() {
    beginResetModel();
    m_items.clear();
    endResetModel();
    emit itemsChanged();
}

QVariantMap MillingProducedModel::getRow(int index) const {
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

void MillingProducedModel::setRowProperty(int index, const QString &property, const QVariant &value) {
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
// MillingBatchController
// -------------------------------------------------------------
MillingBatchController::MillingBatchController(QObject *parent)
    : QObject(parent) {
    connect(&m_consumedModel, &MillingConsumedModel::itemsChanged, this, &MillingBatchController::recalculateTotals);
    connect(&m_producedModel, &MillingProducedModel::itemsChanged, this, &MillingBatchController::recalculateTotals);
}

void MillingBatchController::setBatchDate(const QString &v) {
    QString resolved = AccountingDateService::instance().resolveDate(v);
    if (m_batchDate != resolved) {
        m_batchDate = resolved;
        emit batchDateChanged();
        emit dayOfWeekChanged();
    }
}

QString MillingBatchController::dayOfWeek() const {
    return AccountingDateService::instance().getDayOfWeek(m_batchDate);
}

void MillingBatchController::resetForm(const QString &workingDate) {
    MillingModel mModel;
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

void MillingBatchController::autoPickStandardItems() {
    m_producedModel.clear();
    double consWt = m_totalConsumedWeight > 0 ? m_totalConsumedWeight : 0.0;
    int bTotal = m_totalConsumedBags > 0 ? m_totalConsumedBags : 0;

    m_producedModel.appendRow("Rice Basmati(Non Branded)", 65.0, bTotal > 0 ? std::round(bTotal * 0.65) : 0, consWt > 0 ? (consWt * 0.65) : 0.0, 0.0);
    m_producedModel.appendRow("Rice Bran", 15.0, bTotal > 0 ? std::round(bTotal * 0.15) : 0, consWt > 0 ? (consWt * 0.15) : 0.0, 0.0);
    m_producedModel.appendRow("Rice Broken", 10.0, bTotal > 0 ? std::round(bTotal * 0.10) : 0, consWt > 0 ? (consWt * 0.10) : 0.0, 0.0);
    m_producedModel.appendRow("Rice Nakku", 5.0, bTotal > 0 ? std::round(bTotal * 0.05) : 0, consWt > 0 ? (consWt * 0.05) : 0.0, 0.0);
    m_producedModel.appendRow("Paddy Husk", 5.0, bTotal > 0 ? std::round(bTotal * 0.05) : 0, consWt > 0 ? (consWt * 0.05) : 0.0, 0.0);

    recalculateTotals();
}

void MillingBatchController::addConsumedRow() {
    m_consumedModel.appendRow("", 0, 0.0, 0.0);
    recalculateTotals();
}

void MillingBatchController::removeConsumedRow(int idx) {
    if (m_consumedModel.count() > 1) {
        m_consumedModel.removeRowAt(idx);
        recalculateTotals();
    }
}

void MillingBatchController::addProducedRow() {
    m_producedModel.appendRow("", 0.0, 0, 0.0, 0.0);
    recalculateTotals();
}

void MillingBatchController::removeProducedRow(int idx) {
    if (m_producedModel.count() > 1) {
        m_producedModel.removeRowAt(idx);
        recalculateTotals();
    }
}

void MillingBatchController::updateProducedFromYields() {
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

void MillingBatchController::recalculateTotals() {
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
    m_totalConsumedWeight = std::round(cWt * 1000.0) / 1000.0;
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

    m_totalProducedYieldPct = std::round(pPct * 1000.0) / 1000.0;
    m_totalProducedBags = pBags;
    m_totalProducedWeight = std::round(pWt * 1000.0) / 1000.0;
    m_totalProducedAmount = FinancialMathService::instance().round2(pAmt);

    m_shortagePct = std::max(0.0, std::round((100.0 - m_totalProducedYieldPct) * 1000.0) / 1000.0);
    m_shortageWeight = std::max(0.0, std::round((m_totalConsumedWeight - m_totalProducedWeight) * 1000.0) / 1000.0);
    m_shortageBags = std::max(0, m_totalConsumedBags - m_totalProducedBags);

    emit totalsChanged();
}

bool MillingBatchController::saveVoucher() {
    m_statusMessage.clear();
    m_isError = false;

    if (m_totalConsumedWeight <= 0) {
        m_statusMessage = "Please enter valid Consumed Paddy Weight.";
        m_isError = true;
        emit statusChanged();
        return false;
    }

    if (m_totalProducedWeight <= 0) {
        m_statusMessage = "Please enter valid Produced Items Weight.";
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

    // Extract primary input item
    auto cFirst = m_consumedModel.getRow(0);
    QString paddyItem = cFirst.value("itemName").toString().trimmed();
    if (paddyItem.isEmpty()) paddyItem = "Paddy Common";
    int paddyBags = m_totalConsumedBags;
    double paddyWeight = m_totalConsumedWeight;

    // Extract primary output items
    QString riceItem = "Rice Basmati";
    int riceBags = 0;
    double riceWeight = 0.0;
    double riceCost = 0.0;
    double outturnPct = 0.0;

    QString branItem = "Rice Bran";
    int branBags = 0;
    double branWeight = 0.0;
    double branCost = 0.0;

    QString huskItem = "Paddy Husk";
    int huskBags = 0;
    double huskWeight = 0.0;
    double huskCost = 0.0;

    QString nakkuItem = "Rice Nakku";
    int nakkuBags = 0;
    double nakkuWeight = 0.0;
    double nakkuCost = 0.0;

    QString otherItem = "";
    int otherBags = 0;
    double otherWeight = 0.0;
    double otherCost = 0.0;

    for (int j = 0; j < m_producedModel.count(); ++j) {
        auto pr = m_producedModel.getRow(j);
        QString name = pr.value("itemName").toString().toLower();
        int b = pr.value("bags").toInt();
        double w = pr.value("weight").toDouble();
        double a = pr.value("amount").toDouble();
        double y = pr.value("yieldPct").toDouble();

        if (name.contains("bran")) {
            branItem = pr.value("itemName").toString();
            branBags = b; branWeight = w; branCost = a;
        } else if (name.contains("husk") || name.contains("phak")) {
            huskItem = pr.value("itemName").toString();
            huskBags = b; huskWeight = w; huskCost = a;
        } else if (name.contains("nakku") || name.contains("broken")) {
            nakkuItem = pr.value("itemName").toString();
            nakkuBags = b; nakkuWeight = w; nakkuCost = a;
        } else if (riceWeight == 0.0) {
            riceItem = pr.value("itemName").toString();
            riceBags = b; riceWeight = w; riceCost = a;
            outturnPct = y > 0 ? y : (paddyWeight > 0 ? (w / paddyWeight * 100.0) : 0.0);
        } else {
            otherItem = pr.value("itemName").toString();
            otherBags += b; otherWeight += w; otherCost += a;
        }
    }

    int totalByproductBags = branBags + huskBags + nakkuBags + otherBags;
    double totalByproductWeight = branWeight + huskWeight + nakkuWeight + otherWeight;
    double totalByproductCost = branCost + huskCost + nakkuCost + otherCost;

    MillingModel mModel;
    bool ok = mModel.add_milling_voucher_full(
        m_batchNo, isoDate, paddyItem, paddyBags, paddyWeight,
        riceItem, riceBags, riceWeight, outturnPct, riceCost,
        branItem, branBags, branWeight, branCost,
        huskItem, huskBags, huskWeight, huskCost,
        nakkuItem, nakkuBags, nakkuWeight, nakkuCost,
        otherItem, otherBags, otherWeight, otherCost,
        totalByproductBags, totalByproductWeight, totalByproductCost,
        m_shortageWeight, m_shortagePct, 0.0, m_totalConsumedAmount,
        "System Admin", m_notes.trimmed()
    );

    if (ok) {
        m_statusMessage = QString("Milling Batch %1 saved & posted successfully!").arg(m_batchNo);
        m_isError = false;
        emit statusChanged();
        emit voucherSaved();
        resetForm();
        return true;
    } else {
        m_statusMessage = "Failed to save Milling Batch in database.";
        m_isError = true;
        emit statusChanged();
        return false;
    }
}
