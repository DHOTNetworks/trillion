#include "manufacturing_statement_model.h"
#include "manufacturing_model.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include <cmath>

ManufacturingBatchListModel::ManufacturingBatchListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int ManufacturingBatchListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_entries.size();
}

QVariant ManufacturingBatchListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return QVariant();

    const auto& item = m_entries.at(index.row());
    switch (role) {
    case IdRole: return item.id;
    case BatchNoRole: return item.batchNo;
    case BatchDateRole: return item.batchDate;
    case InputVarietyRole: return item.inputVariety;
    case InputQtyRole: return item.inputQty;
    case OutputQtyRole: return item.outputQty;
    case ByproductQtyRole: return item.byproductQty;
    case WastageRole: return item.wastage;
    case YieldPctRole: return item.yieldPct;
    case NarrationRole: return item.narration;
    default: return QVariant();
    }
}

QHash<int, QByteArray> ManufacturingBatchListModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[BatchNoRole] = "batchNo";
    roles[BatchDateRole] = "batchDate";
    roles[InputVarietyRole] = "inputVariety";
    roles[InputQtyRole] = "inputQty";
    roles[OutputQtyRole] = "outputQty";
    roles[ByproductQtyRole] = "byproductQty";
    roles[WastageRole] = "wastage";
    roles[YieldPctRole] = "yieldPct";
    roles[NarrationRole] = "narration";
    return roles;
}

void ManufacturingBatchListModel::setEntries(const QVector<ManufacturingBatchEntry>& entries) {
    beginResetModel();
    m_entries = entries;
    endResetModel();
    emit countChanged();
}

void ManufacturingBatchListModel::clear() {
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}

QVariantMap ManufacturingBatchListModel::get(int row) const {
    if (row < 0 || row >= m_entries.size()) return QVariantMap();
    const auto& item = m_entries.at(row);
    QVariantMap m;
    m["id"] = item.id;
    m["batchNo"] = item.batchNo;
    m["batchDate"] = item.batchDate;
    m["inputVariety"] = item.inputVariety;
    m["inputQty"] = item.inputQty;
    m["outputQty"] = item.outputQty;
    m["byproductQty"] = item.byproductQty;
    m["wastage"] = item.wastage;
    m["yieldPct"] = item.yieldPct;
    m["narration"] = item.narration;
    return m;
}

ManufacturingItemListModel::ManufacturingItemListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int ManufacturingItemListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_entries.size();
}

QVariant ManufacturingItemListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return QVariant();

    const auto& item = m_entries.at(index.row());
    switch (role) {
    case RowNoRole: return item.rowNo;
    case DrcrRole: return item.drcr;
    case IsInputRole: return item.isInput;
    case ItemNameRole: return item.itemName;
    case YieldPctRole: return item.yieldPct;
    case BagsRole: return item.bags;
    case WeightRole: return item.weight;
    case RateRole: return item.rate;
    case AmountRole: return item.amount;
    case NarrationRole: return item.narration;
    default: return QVariant();
    }
}

QHash<int, QByteArray> ManufacturingItemListModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[RowNoRole] = "rowNo";
    roles[DrcrRole] = "drcr";
    roles[IsInputRole] = "isInput";
    roles[ItemNameRole] = "itemName";
    roles[YieldPctRole] = "yieldPct";
    roles[BagsRole] = "bags";
    roles[WeightRole] = "weight";
    roles[RateRole] = "rate";
    roles[AmountRole] = "amount";
    roles[NarrationRole] = "narration";
    return roles;
}

void ManufacturingItemListModel::setEntries(const QVector<ManufacturingItemEntry>& entries) {
    beginResetModel();
    m_entries = entries;
    endResetModel();
    emit countChanged();
}

void ManufacturingItemListModel::clear() {
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}

QVariantMap ManufacturingItemListModel::get(int row) const {
    if (row < 0 || row >= m_entries.size()) return QVariantMap();
    const auto& item = m_entries.at(row);
    QVariantMap m;
    m["rowNo"] = item.rowNo;
    m["drcr"] = item.drcr;
    m["isInput"] = item.isInput;
    m["itemName"] = item.itemName;
    m["yieldPct"] = item.yieldPct;
    m["bags"] = item.bags;
    m["weight"] = item.weight;
    m["rate"] = item.rate;
    m["amount"] = item.amount;
    m["narration"] = item.narration;
    return m;
}

ManufacturingStatementController::ManufacturingStatementController(QObject* parent)
    : QObject(parent)
{
}

void ManufacturingStatementController::setSearchQuery(const QString& query) {
    if (m_searchQuery != query) {
        m_searchQuery = query;
        emit searchQueryChanged();
        applyFilter();
    }
}

void ManufacturingStatementController::setSelectedVariety(const QString& variety) {
    if (m_selectedVariety != variety) {
        m_selectedVariety = variety;
        emit selectedVarietyChanged();
        applyFilter();
    }
}

void ManufacturingStatementController::setSelectedBatchIndex(int idx) {
    if (m_selectedBatchIndex != idx) {
        m_selectedBatchIndex = idx;
        emit selectedBatchIndexChanged();
    }
}

void ManufacturingStatementController::selectBatch(int index) {
    if (index >= 0 && index < m_batchModel.rowCount()) {
        m_selectedBatchIndex = index;
        emit selectedBatchIndexChanged();

        QVariantMap row = m_batchModel.get(index);
        m_activeBatchNo = row.value("batchNo").toString();
        emit activeBatchNoChanged();

        int bId = row.value("id").toInt();
        loadBatchItems(bId, m_activeBatchNo);
    } else {
        m_selectedBatchIndex = -1;
        emit selectedBatchIndexChanged();
        m_activeBatchNo = "";
        emit activeBatchNoChanged();
        m_itemModel.clear();
    }
}

void ManufacturingStatementController::loadBatchItems(int batchId, const QString& batchNo) {
    ManufacturingModel tempModel;
    QVariantList items = tempModel.get_batch_items(batchId, batchNo);

    QVector<ManufacturingItemEntry> itemEntries;
    itemEntries.reserve(items.size());

    for (int i = 0; i < items.size(); ++i) {
        QVariantMap it = items[i].toMap();
        ManufacturingItemEntry e;
        e.rowNo = it.value("row_no", i + 1).toInt();
        e.drcr = it.value("drcr", e.rowNo == 1 ? "Cr" : "Dr").toString();
        e.isInput = (e.drcr == "Cr");
        e.itemName = it.value("item_name").toString();
        if (e.itemName.isEmpty()) e.itemName = QString("Item #%1").arg(it.value("item_code").toString());

        double pct = it.value("percentage").toDouble();
        e.yieldPct = pct > 0.001 ? QString::number(pct, 'f', 2) + "%" : "-";

        long long bg = it.value("bags").toLongLong();
        e.bags = QString::number(bg);

        double wt = it.value("weight_qtl").toDouble();
        e.weight = it.value("weight_fmt", QString::number(wt, 'f', 3)).toString();

        e.rate = it.value("rate_fmt", "-").toString();
        e.amount = it.value("amount_fmt", "₹0.00").toString();
        e.narration = it.value("narration").toString();

        itemEntries.append(e);
    }

    m_itemModel.setEntries(itemEntries);
}

void ManufacturingStatementController::reload(const QString& fromDate, const QString& toDate, const QString& variety) {
    QString fDate = fromDate.trimmed();
    QString tDate = toDate.trimmed();

    if (fDate.isEmpty() && tDate.isEmpty()) {
        fDate = AccountingEngine::getActiveFromDate();
        tDate = AccountingEngine::getActiveToDate();
    }

    ManufacturingModel tempModel;
    QVariantList rawList = tempModel.get_manufacturing_statement(fDate, tDate, variety);
    QVariantMap totals = tempModel.get_manufacturing_totals(fDate, tDate);

    m_totalBatchesCount = totals.value("total_batches", rawList.size()).toInt();
    m_totalInputFmt = totals.value("total_input_fmt", "0.00").toString();
    m_totalOutputFmt = totals.value("total_output_fmt", "0.00").toString();
    m_totalByproductFmt = totals.value("total_byproduct_fmt", "0.00").toString();
    m_totalWastageFmt = totals.value("total_wastage_fmt", "0.00").toString();
    m_avgYieldPctFmt = totals.value("avg_yield_fmt", "0.00%").toString();

    m_allBatches.clear();
    m_allBatches.reserve(rawList.size());

    for (int i = 0; i < rawList.size(); ++i) {
        QVariantMap b = rawList[i].toMap();
        ManufacturingBatchEntry e;
        e.id = b.value("id", i + 1).toInt();
        e.batchNo = b.value("batch_no").toString();
        e.batchDate = b.value("batch_date").toString();
        e.inputVariety = b.value("input_variety").toString();
        e.inputQty = b.value("input_qtl_fmt", QString::number(b.value("input_qtl").toDouble(), 'f', 3)).toString();
        e.outputQty = b.value("output_qtl_fmt", QString::number(b.value("output_qtl").toDouble(), 'f', 3)).toString();
        e.byproductQty = b.value("byproduct_qtl_fmt", QString::number(b.value("byproduct_qtl").toDouble(), 'f', 3)).toString();
        e.wastage = b.value("wastage_fmt", QString::number(b.value("wastage_qtl").toDouble(), 'f', 3)).toString();

        double yPct = b.value("yield_pct").toDouble();
        e.yieldPct = b.value("yield_pct_fmt", QString::number(yPct, 'f', 2) + "%").toString();
        e.narration = b.value("narration").toString();

        m_allBatches.append(e);
    }

    emit totalsChanged();
    applyFilter();
}

void ManufacturingStatementController::applyFilter() {
    QString q = m_searchQuery.trimmed().toLower();
    QString varFilter = m_selectedVariety.trimmed().toLower();
    bool allVars = (varFilter.isEmpty() || varFilter == "all varieties" || varFilter == "all");

    QVector<ManufacturingBatchEntry> filtered;
    filtered.reserve(m_allBatches.size());

    for (const auto& b : m_allBatches) {
        if (!allVars && b.inputVariety.toLower() != varFilter) {
            continue;
        }

        if (!q.isEmpty()) {
            if (!b.batchNo.toLower().contains(q) &&
                !b.batchDate.toLower().contains(q) &&
                !b.inputVariety.toLower().contains(q) &&
                !b.narration.toLower().contains(q)) {
                continue;
            }
        }

        filtered.append(b);
    }

    m_batchModel.setEntries(filtered);

    if (m_batchModel.rowCount() > 0) {
        selectBatch(0);
    } else {
        selectBatch(-1);
    }
}
