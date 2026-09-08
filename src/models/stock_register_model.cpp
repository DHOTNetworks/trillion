#include "stock_register_model.h"
#include "stock_items_model.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include <cmath>

static QString formatINR(double val) {
    if (std::abs(val) < 0.001) return "₹0.00";
    bool neg = val < 0;
    double absVal = std::abs(val);
    long long integerPart = static_cast<long long>(absVal);
    int decimalPart = static_cast<int>(std::round((absVal - integerPart) * 100.0));
    if (decimalPart >= 100) {
        integerPart += 1;
        decimalPart = 0;
    }

    QString s = QString::number(integerPart);
    QString res = "";
    int n = s.length();
    if (n <= 3) {
        res = s;
    } else {
        res = s.right(3);
        int rem = n - 3;
        while (rem > 0) {
            int take = std::min(2, rem);
            res = s.mid(rem - take, take) + "," + res;
            rem -= take;
        }
    }
    QString decStr = QString::number(decimalPart);
    if (decStr.length() == 1) decStr = "0" + decStr;
    return (neg ? "-₹" : "₹") + res + "." + decStr;
}

StockRegisterModel::StockRegisterModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int StockRegisterModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_entries.size();
}

QVariant StockRegisterModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return QVariant();

    const auto& item = m_entries.at(index.row());
    switch (role) {
    case IdValRole: return item.id;
    case NameValRole: return item.nameVal;
    case CodeValRole: return item.codeVal.isEmpty() ? "-" : item.codeVal;
    case TypeValRole: return item.typeVal.isEmpty() ? "-" : item.typeVal;
    case UnitValRole: return item.unitVal.isEmpty() ? "Qtl" : item.unitVal;
    case OpBagsRole: return item.opBags;
    case InBagsRole: return item.inBags;
    case OutBagsRole: return item.outBags;
    case CloseBagsRole: return item.closeBags;
    case OpQtyValRole: return item.opQtyVal;
    case InQtyValRole: return item.inQtyVal;
    case OutQtyValRole: return item.outQtyVal;
    case CloseQtyValRole: return item.closeQtyVal;
    case CloseQtyNumRole: return item.closeQty;
    case RateValRole: return item.rateVal;
    case CloseValValRole: return item.closeValVal;
    case CloseValNumRole: return item.closeVal;
    default: return QVariant();
    }
}

QHash<int, QByteArray> StockRegisterModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdValRole] = "idVal";
    roles[NameValRole] = "nameVal";
    roles[CodeValRole] = "codeVal";
    roles[TypeValRole] = "typeVal";
    roles[UnitValRole] = "unitVal";
    roles[OpBagsRole] = "opBags";
    roles[InBagsRole] = "inBags";
    roles[OutBagsRole] = "outBags";
    roles[CloseBagsRole] = "closeBags";
    roles[OpQtyValRole] = "opQtyVal";
    roles[InQtyValRole] = "inQtyVal";
    roles[OutQtyValRole] = "outQtyVal";
    roles[CloseQtyValRole] = "closeQtyVal";
    roles[CloseQtyNumRole] = "closeQtyNum";
    roles[RateValRole] = "rateVal";
    roles[CloseValValRole] = "closeValVal";
    roles[CloseValNumRole] = "closeValNum";
    return roles;
}

void StockRegisterModel::setEntries(const QVector<StockRegisterEntry>& entries) {
    beginResetModel();
    m_entries = entries;
    endResetModel();
    emit countChanged();
}

void StockRegisterModel::clear() {
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}

QVariantMap StockRegisterModel::get(int row) const {
    if (row < 0 || row >= m_entries.size()) return QVariantMap();
    const auto& item = m_entries.at(row);
    QVariantMap m;
    m["idVal"] = item.id;
    m["nameVal"] = item.nameVal;
    m["codeVal"] = item.codeVal;
    m["typeVal"] = item.typeVal;
    m["unitVal"] = item.unitVal;
    m["opBags"] = item.opBags;
    m["inBags"] = item.inBags;
    m["outBags"] = item.outBags;
    m["closeBags"] = item.closeBags;
    m["opQtyVal"] = item.opQtyVal;
    m["inQtyVal"] = item.inQtyVal;
    m["outQtyVal"] = item.outQtyVal;
    m["closeQtyVal"] = item.closeQtyVal;
    m["closeQtyNum"] = item.closeQty;
    m["rateVal"] = item.rateVal;
    m["closeValVal"] = item.closeValVal;
    m["closeValNum"] = item.closeVal;
    return m;
}

StockRegisterController::StockRegisterController(QObject* parent)
    : QObject(parent)
{
}

void StockRegisterController::setSearchQuery(const QString& query) {
    if (m_searchQuery != query) {
        m_searchQuery = query;
        emit searchQueryChanged();
        applyFilter();
    }
}

void StockRegisterController::reload(const QString& fromDate, const QString& toDate) {
    QString fDate = fromDate.trimmed();
    QString tDate = toDate.trimmed();

    if (fDate.isEmpty() && tDate.isEmpty()) {
        fDate = AccountingEngine::getActiveFromDate();
        tDate = AccountingEngine::getActiveToDate();
    }

    StockItemsModel tempModel;
    QVariantList rawList = tempModel.get_stock_register(fDate, tDate);

    m_allEntries.clear();
    m_allEntries.reserve(rawList.size());

    for (const auto& var : rawList) {
        QVariantMap it = var.toMap();
        StockRegisterEntry e;
        e.id = it.value("id").toInt();
        e.nameVal = it.value("name").toString();
        e.codeVal = it.value("code").toString();
        e.typeVal = it.value("item_type").toString();
        e.unitVal = it.value("unit", "Qtl").toString();
        e.opBags = it.value("opening_bags").toLongLong();
        e.inBags = it.value("inward_bags").toLongLong();
        e.outBags = it.value("outward_bags").toLongLong();
        e.closeBags = it.value("closing_bags").toLongLong();

        e.opQty = it.value("opening_qty").toDouble();
        e.opQtyVal = it.value("opening_qty_fmt", QString::number(e.opQty, 'f', 2)).toString();

        e.inQty = it.value("inward_qty").toDouble();
        e.inQtyVal = it.value("inward_qty_fmt", QString::number(e.inQty, 'f', 2)).toString();

        e.outQty = it.value("outward_qty").toDouble();
        e.outQtyVal = it.value("outward_qty_fmt", QString::number(e.outQty, 'f', 2)).toString();

        e.closeQty = it.value("closing_qty").toDouble();
        e.closeQtyVal = it.value("closing_qty_fmt", QString::number(e.closeQty, 'f', 2)).toString();

        e.rate = it.value("rate").toDouble();
        e.rateVal = it.value("rate_fmt", formatINR(e.rate)).toString();

        e.closeVal = it.value("closing_value").toDouble();
        e.closeValVal = it.value("closing_value_fmt", formatINR(e.closeVal)).toString();

        m_allEntries.append(e);
    }

    applyFilter();
}

void StockRegisterController::applyFilter() {
    QString q = m_searchQuery.trimmed().toLower();
    QVector<StockRegisterEntry> filtered;
    filtered.reserve(m_allEntries.size());

    m_totalItemsCount = 0;
    m_totalClosingQty = 0.0;
    m_totalClosingVal = 0.0;

    for (const auto& e : m_allEntries) {
        if (!q.isEmpty()) {
            if (!e.nameVal.toLower().contains(q) &&
                !e.codeVal.toLower().contains(q) &&
                !e.typeVal.toLower().contains(q)) {
                continue;
            }
        }

        filtered.append(e);

        m_totalItemsCount++;
        m_totalClosingQty += e.closeQty;
        m_totalClosingVal += e.closeVal;
    }

    m_totalClosingQtyFmt = QString::number(m_totalClosingQty, 'f', 2) + " Qtl";
    m_totalClosingValFmt = formatINR(m_totalClosingVal);

    m_model.setEntries(filtered);
    emit totalsChanged();
}
