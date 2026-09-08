#include "milling_statement_model.h"
#include "milling_model.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include <cmath>

MillingBatchListModel::MillingBatchListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int MillingBatchListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_entries.size();
}

QVariant MillingBatchListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return QVariant();

    const auto& item = m_entries.at(index.row());
    switch (role) {
    case IdRole: return item.id;
    case BatchNoRole: return item.batchNo;
    case BatchDateRole: return item.batchDate;
    case PaddyVarietyRole: return item.paddyVariety;
    case PaddyInputRole: return item.paddyInput;
    case HeadRiceRole: return item.headRice;
    case BrokenRiceRole: return item.brokenRice;
    case BranRole: return item.bran;
    case HuskRole: return item.husk;
    case WastageRole: return item.wastage;
    case YieldPctRole: return item.yieldPct;
    case NarrationRole: return item.narration;
    default: return QVariant();
    }
}

QHash<int, QByteArray> MillingBatchListModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[BatchNoRole] = "batchNo";
    roles[BatchDateRole] = "batchDate";
    roles[PaddyVarietyRole] = "paddyVariety";
    roles[PaddyInputRole] = "paddyInput";
    roles[HeadRiceRole] = "headRice";
    roles[BrokenRiceRole] = "brokenRice";
    roles[BranRole] = "bran";
    roles[HuskRole] = "husk";
    roles[WastageRole] = "wastage";
    roles[YieldPctRole] = "yieldPct";
    roles[NarrationRole] = "narration";
    return roles;
}

void MillingBatchListModel::setEntries(const QVector<MillingBatchEntry>& entries) {
    beginResetModel();
    m_entries = entries;
    endResetModel();
    emit countChanged();
}

void MillingBatchListModel::clear() {
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}

QVariantMap MillingBatchListModel::get(int row) const {
    if (row < 0 || row >= m_entries.size()) return QVariantMap();
    const auto& item = m_entries.at(row);
    QVariantMap m;
    m["id"] = item.id;
    m["batchNo"] = item.batchNo;
    m["batchDate"] = item.batchDate;
    m["paddyVariety"] = item.paddyVariety;
    m["paddyInput"] = item.paddyInput;
    m["headRice"] = item.headRice;
    m["brokenRice"] = item.brokenRice;
    m["bran"] = item.bran;
    m["husk"] = item.husk;
    m["wastage"] = item.wastage;
    m["yieldPct"] = item.yieldPct;
    m["narration"] = item.narration;
    return m;
}

MillingItemListModel::MillingItemListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int MillingItemListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_entries.size();
}

QVariant MillingItemListModel::data(const QModelIndex& index, int role) const {
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

QHash<int, QByteArray> MillingItemListModel::roleNames() const {
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

void MillingItemListModel::setEntries(const QVector<MillingItemEntry>& entries) {
    beginResetModel();
    m_entries = entries;
    endResetModel();
    emit countChanged();
}

void MillingItemListModel::clear() {
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}

QVariantMap MillingItemListModel::get(int row) const {
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

MillingStatementController::MillingStatementController(QObject* parent)
    : QObject(parent)
{
}

void MillingStatementController::setSearchQuery(const QString& query) {
    if (m_searchQuery != query) {
        m_searchQuery = query;
        emit searchQueryChanged();
        applyFilter();
    }
}

void MillingStatementController::setSelectedVariety(const QString& variety) {
    if (m_selectedVariety != variety) {
        m_selectedVariety = variety;
        emit selectedVarietyChanged();
        applyFilter();
    }
}

void MillingStatementController::setSelectedBatchIndex(int idx) {
    if (m_selectedBatchIndex != idx) {
        m_selectedBatchIndex = idx;
        emit selectedBatchIndexChanged();
    }
}

void MillingStatementController::selectBatch(int index) {
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

void MillingStatementController::loadBatchItems(int batchId, const QString& batchNo) {
    MillingModel tempModel;
    QVariantList items = tempModel.get_batch_items(batchId, batchNo);

    QVector<MillingItemEntry> itemEntries;
    itemEntries.reserve(items.size());

    for (int i = 0; i < items.size(); ++i) {
        QVariantMap it = items[i].toMap();
        MillingItemEntry e;
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

void MillingStatementController::reload(const QString& fromDate, const QString& toDate, const QString& variety) {
    QString fDate = fromDate.trimmed();
    QString tDate = toDate.trimmed();

    if (fDate.isEmpty() && tDate.isEmpty()) {
        fDate = AccountingEngine::getActiveFromDate();
        tDate = AccountingEngine::getActiveToDate();
    }

    MillingModel tempModel;
    QVariantList rawList = tempModel.get_milling_statement(fDate, tDate, variety);
    QVariantMap totals = tempModel.get_milling_totals(fDate, tDate);

    m_totalBatchesCount = totals.value("total_batches", rawList.size()).toInt();
    m_totalPaddyMilledFmt = totals.value("total_paddy_fmt", "0.00").toString();
    m_totalHeadRiceProducedFmt = totals.value("total_head_rice_fmt", "0.00").toString();
    m_totalBranProducedFmt = totals.value("total_bran_fmt", "0.00").toString();
    m_totalBrokenProducedFmt = totals.value("total_broken_fmt", "0.00").toString();
    m_totalHuskProducedFmt = totals.value("total_husk_fmt", "0.00").toString();
    m_totalWastageMilledFmt = totals.value("total_wastage_fmt", "0.00").toString();
    m_avgYieldPctFmt = totals.value("avg_yield_fmt", "0.00%").toString();

    m_allBatches.clear();
    m_allBatches.reserve(rawList.size());

    for (int i = 0; i < rawList.size(); ++i) {
        QVariantMap b = rawList[i].toMap();
        MillingBatchEntry e;
        e.id = b.value("id", i + 1).toInt();
        e.batchNo = b.value("batch_no").toString();
        e.batchDate = b.value("batch_date").toString();
        e.paddyVariety = b.value("paddy_variety", "Paddy Basmati").toString();
        e.paddyInput = b.value("paddy_input_fmt", QString::number(b.value("paddy_input_qtl").toDouble(), 'f', 3)).toString();
        e.headRice = b.value("head_rice_fmt", QString::number(b.value("head_rice_qtl").toDouble(), 'f', 3)).toString();
        e.brokenRice = b.value("broken_rice_fmt", QString::number(b.value("broken_rice_qtl").toDouble(), 'f', 3)).toString();
        e.bran = b.value("bran_fmt", QString::number(b.value("bran_qtl").toDouble(), 'f', 3)).toString();
        e.husk = b.value("husk_fmt", QString::number(b.value("husk_qtl").toDouble(), 'f', 3)).toString();
        e.wastage = b.value("wastage_fmt", QString::number(b.value("wastage_qtl").toDouble(), 'f', 3)).toString();

        double yPct = b.value("yield_pct").toDouble();
        e.yieldPct = b.value("yield_pct_fmt", QString::number(yPct, 'f', 2) + "%").toString();
        e.narration = b.value("narration").toString();

        m_allBatches.append(e);
    }

    emit totalsChanged();
    applyFilter();
}

void MillingStatementController::applyFilter() {
    QString q = m_searchQuery.trimmed().toLower();
    QString varFilter = m_selectedVariety.trimmed().toLower();
    bool allVars = (varFilter.isEmpty() || varFilter == "all varieties" || varFilter == "all");

    QVector<MillingBatchEntry> filtered;
    filtered.reserve(m_allBatches.size());

    for (const auto& b : m_allBatches) {
        if (!allVars && b.paddyVariety.toLower() != varFilter) {
            continue;
        }

        if (!q.isEmpty()) {
            if (!b.batchNo.toLower().contains(q) &&
                !b.batchDate.toLower().contains(q) &&
                !b.paddyVariety.toLower().contains(q) &&
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
