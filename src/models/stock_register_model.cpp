#include "stock_register_model.h"
#include "stock_items_model.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"
#include <cmath>
#include <QDate>
#include <QMap>

namespace MahadevERP {

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

// ============================================================================
// StockRegisterModel Implementation
// ============================================================================

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
    case GroupValRole: return item.groupVal.isEmpty() ? "General" : item.groupVal;
    case CompanyValRole: return item.companyVal.isEmpty() ? "Self" : item.companyVal;
    case HsnValRole: return item.hsnVal.isEmpty() ? "-" : item.hsnVal;
    case UnitValRole: return item.unitVal.isEmpty() ? "Qtl" : item.unitVal;
    case ItemsCountRole: return item.itemsCount;

    case OpBagsRole: return item.opBags;
    case InBagsRole: return item.inBags;
    case OutBagsRole: return item.outBags;
    case CloseBagsRole: return item.closeBags;

    case OpQtyValRole: return item.opQtyVal;
    case InQtyValRole: return item.inQtyVal;
    case OutQtyValRole: return item.outQtyVal;
    case CloseQtyValRole: return item.closeQtyVal;
    case CloseQtyNumRole: return item.closeQty;

    case OpValValRole: return item.opValVal;
    case InValValRole: return item.inValVal;
    case OutValValRole: return item.outValVal;
    case RateValRole: return item.rateVal;
    case CloseValValRole: return item.closeValVal;
    case CloseValNumRole: return item.closeVal;

    case SalesQtyRole: return item.salesQty;
    case SalesValueValRole: return item.salesValueVal;
    case AvgSaleRateValRole: return item.avgSaleRateVal;
    case CostRateValRole: return item.costRateVal;
    case CogsValRole: return item.cogsVal;
    case GrossProfitValRole: return item.grossProfitVal;
    case GrossProfitNumRole: return item.grossProfit;
    case GpMarginPctValRole: return item.gpMarginPctVal;

    case PeriodDateRole: return item.periodDate;
    case VoucherNoRole: return item.voucherNo;
    case VoucherTypeRole: return item.voucherType;
    case PartyNameRole: return item.partyName;
    case RunningBalanceValRole: return item.runningBalanceVal;

    default: return QVariant();
    }
}

QHash<int, QByteArray> StockRegisterModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdValRole] = "idVal";
    roles[NameValRole] = "nameVal";
    roles[CodeValRole] = "codeVal";
    roles[TypeValRole] = "typeVal";
    roles[GroupValRole] = "groupVal";
    roles[CompanyValRole] = "companyVal";
    roles[HsnValRole] = "hsnVal";
    roles[UnitValRole] = "unitVal";
    roles[ItemsCountRole] = "itemsCount";

    roles[OpBagsRole] = "opBags";
    roles[InBagsRole] = "inBags";
    roles[OutBagsRole] = "outBags";
    roles[CloseBagsRole] = "closeBags";

    roles[OpQtyValRole] = "opQtyVal";
    roles[InQtyValRole] = "inQtyVal";
    roles[OutQtyValRole] = "outQtyVal";
    roles[CloseQtyValRole] = "closeQtyVal";
    roles[CloseQtyNumRole] = "closeQtyNum";

    roles[OpValValRole] = "opValVal";
    roles[InValValRole] = "inValVal";
    roles[OutValValRole] = "outValVal";
    roles[RateValRole] = "rateVal";
    roles[CloseValValRole] = "closeValVal";
    roles[CloseValNumRole] = "closeValNum";

    roles[SalesQtyRole] = "salesQty";
    roles[SalesValueValRole] = "salesValueVal";
    roles[AvgSaleRateValRole] = "avgSaleRateVal";
    roles[CostRateValRole] = "costRateVal";
    roles[CogsValRole] = "cogsVal";
    roles[GrossProfitValRole] = "grossProfitVal";
    roles[GrossProfitNumRole] = "grossProfitNum";
    roles[GpMarginPctValRole] = "gpMarginPctVal";

    roles[PeriodDateRole] = "periodDate";
    roles[VoucherNoRole] = "voucherNo";
    roles[VoucherTypeRole] = "voucherType";
    roles[PartyNameRole] = "partyName";
    roles[RunningBalanceValRole] = "runningBalanceVal";
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
    m["id"] = item.id;
    m["name"] = item.nameVal;
    m["code"] = item.codeVal;
    m["type"] = item.typeVal;
    m["group"] = item.groupVal;
    m["company"] = item.companyVal;
    m["hsn"] = item.hsnVal;
    m["unit"] = item.unitVal;
    m["itemsCount"] = item.itemsCount;

    m["opBags"] = item.opBags;
    m["inBags"] = item.inBags;
    m["outBags"] = item.outBags;
    m["closeBags"] = item.closeBags;

    m["opQty"] = item.opQty;
    m["opQtyVal"] = item.opQtyVal;
    m["inQty"] = item.inQty;
    m["inQtyVal"] = item.inQtyVal;
    m["outQty"] = item.outQty;
    m["outQtyVal"] = item.outQtyVal;
    m["closeQty"] = item.closeQty;
    m["closeQtyVal"] = item.closeQtyVal;

    m["opVal"] = item.opVal;
    m["opValVal"] = item.opValVal;
    m["inVal"] = item.inVal;
    m["inValVal"] = item.inValVal;
    m["outVal"] = item.outVal;
    m["outValVal"] = item.outValVal;
    m["rate"] = item.rate;
    m["rateVal"] = item.rateVal;
    m["closeVal"] = item.closeVal;
    m["closeValVal"] = item.closeValVal;

    m["salesQty"] = item.salesQty;
    m["salesValue"] = item.salesValue;
    m["avgSaleRate"] = item.avgSaleRate;
    m["costRate"] = item.costRate;
    m["cogs"] = item.cogs;
    m["grossProfit"] = item.grossProfit;
    m["gpMarginPct"] = item.gpMarginPct;

    m["periodDate"] = item.periodDate;
    m["voucherNo"] = item.voucherNo;
    m["voucherType"] = item.voucherType;
    m["partyName"] = item.partyName;
    m["runningBalance"] = item.runningBalance;
    return m;
}

// ============================================================================
// StockRegisterController Implementation
// ============================================================================

StockRegisterController::StockRegisterController(QObject* parent)
    : QObject(parent)
{
}

void StockRegisterController::setViewMode(StockViewMode mode) {
    if (m_viewMode != mode) {
        m_viewMode = mode;
        emit configChanged();
        reload(m_currentFromDate, m_currentToDate);
    }
}

void StockRegisterController::setGrouping(StockGrouping grouping) {
    if (m_grouping != grouping) {
        m_grouping = grouping;
        emit configChanged();
        reload(m_currentFromDate, m_currentToDate);
    }
}

void StockRegisterController::setSelectedItemId(int itemId) {
    if (m_selectedItemId != itemId) {
        m_selectedItemId = itemId;
        emit configChanged();
        if (m_viewMode == StockViewMode::MonthlyDaily) {
            reload(m_currentFromDate, m_currentToDate);
        }
    }
}

void StockRegisterController::setIsDailyDetail(bool b) {
    if (m_isDailyDetail != b) {
        m_isDailyDetail = b;
        emit configChanged();
        if (m_viewMode == StockViewMode::MonthlyDaily) {
            reload(m_currentFromDate, m_currentToDate);
        }
    }
}

void StockRegisterController::setSearchQuery(const QString& query) {
    if (m_searchQuery != query) {
        m_searchQuery = query;
        emit searchQueryChanged();
        applyFilter();
    }
}

void StockRegisterController::reload(const QString& fromDate, const QString& toDate) {
    m_currentFromDate = fromDate.trimmed();
    m_currentToDate = toDate.trimmed();

    if (m_currentFromDate.isEmpty() && m_currentToDate.isEmpty()) {
        FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
        m_currentFromDate = activeFy.startDate;
        m_currentToDate = activeFy.endDate.isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : activeFy.endDate;
    }

    fetchRawData(m_currentFromDate, m_currentToDate);
    applyFilter();
}

void StockRegisterController::fetchRawData(const QString& fromDate, const QString& toDate) {
    m_allEntries.clear();
    auto& db = DatabaseManager::instance();

    if (m_viewMode == StockViewMode::MonthlyDaily) {
        // ====================================================================
        // Mode 4: Item Monthly / Daily Timeline Breakdown
        // ====================================================================
        int itmId = m_selectedItemId;
        if (itmId <= 0) {
            QVariant activeItem = db.executeScalar("SELECT item_id FROM stock_transactions WHERE item_id > 0 ORDER BY voucher_date DESC LIMIT 1;");
            if (activeItem.isValid() && activeItem.toInt() > 0) {
                itmId = activeItem.toInt();
            } else {
                QVariant fst = db.executeScalar("SELECT id FROM stock_items ORDER BY name COLLATE NOCASE ASC LIMIT 1;");
                if (fst.isValid()) itmId = fst.toInt();
            }
            m_selectedItemId = itmId;
        }
        if (itmId <= 0) return;

        QVariantMap itmMeta = db.executeScalar("SELECT name, code, unit, opening_qty, opening_bags, opening_rate FROM stock_items WHERE id = ?;", {itmId}).toMap();
        QVariantList itmRows = db.executeQuery("SELECT name, code, unit, opening_qty, opening_bags, opening_rate FROM stock_items WHERE id = ?;", {itmId});
        if (itmRows.isEmpty()) return;
        itmMeta = itmRows.first().toMap();

        QString itmName = itmMeta.value("name").toString();
        QString itmCode = itmMeta.value("code").toString();
        QString itmUnit = itmMeta.value("unit", "Qtl").toString();
        double opMasterQty = itmMeta.value("opening_qty").toDouble();
        long long opMasterBags = itmMeta.value("opening_bags").toLongLong();
        double opMasterRate = itmMeta.value("opening_rate").toDouble();
        if (opMasterRate <= 0.0 && opMasterQty > 0.0) {
            opMasterRate = itmMeta.value("opening_value").toDouble() / opMasterQty;
        }
        if (opMasterRate <= 0.0) opMasterRate = itmMeta.value("purchase_rate", 0.0).toDouble();

        double opQty = opMasterQty;
        long long opBags = opMasterBags;
        double opRate = opMasterRate;

        // 1. Opening Stock from custom_closing_stocks or master opening
        if (!fromDate.isEmpty()) {
            QVariantList customRows = db.executeQuery(
                "SELECT closing_date, bags, weight_qtl, rate, amount FROM custom_closing_stocks WHERE (item_id = ? OR item_name = ? OR item_code = ?) AND closing_date < ? ORDER BY closing_date DESC LIMIT 1;",
                {itmId, itmName, itmCode, fromDate}
            );
            if (!customRows.isEmpty()) {
                QVariantMap customRow = customRows.first().toMap();
                QString cDate = customRow.value("closing_date").toString();
                opBags = customRow.value("bags").toLongLong();
                opQty = customRow.value("weight_qtl").toDouble();
                if (customRow.value("rate").toDouble() > 0.0) {
                    opRate = customRow.value("rate").toDouble();
                }

                // Add intermediate stock transactions between audited closing date and fromDate
                QVariantList rInt = db.executeQuery(
                    "SELECT "
                    "SUM(CASE WHEN trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M') THEN weight_qtl ELSE 0 END) AS in_wt, "
                    "SUM(CASE WHEN trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M') THEN bags ELSE 0 END) AS in_bg, "
                    "SUM(CASE WHEN trans_type IN ('Sale', 'PrRn', 'Outward', 'S') THEN weight_qtl ELSE 0 END) AS out_wt, "
                    "SUM(CASE WHEN trans_type IN ('Sale', 'PrRn', 'Outward', 'S') THEN bags ELSE 0 END) AS out_bg "
                    "FROM stock_transactions "
                    "WHERE (item_id = ? OR item_name = ? OR item_code = ?) AND voucher_date > ? AND voucher_date < ?;",
                    {itmId, itmName, itmCode, cDate, fromDate}
                );
                if (!rInt.isEmpty()) {
                    QVariantMap ri = rInt.first().toMap();
                    opQty += (ri.value("in_wt").toDouble() - ri.value("out_wt").toDouble());
                    opBags += (ri.value("in_bg").toLongLong() - ri.value("out_bg").toLongLong());
                }
            } else {
                // If no custom_closing_stocks before fromDate, roll forward master opening with prior transactions
                QVariantList rPrior = db.executeQuery(
                    "SELECT "
                    "SUM(CASE WHEN trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M') THEN weight_qtl ELSE 0 END) AS in_wt, "
                    "SUM(CASE WHEN trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M') THEN bags ELSE 0 END) AS in_bg, "
                    "SUM(CASE WHEN trans_type IN ('Sale', 'PrRn', 'Outward', 'S') THEN weight_qtl ELSE 0 END) AS out_wt, "
                    "SUM(CASE WHEN trans_type IN ('Sale', 'PrRn', 'Outward', 'S') THEN bags ELSE 0 END) AS out_bg "
                    "FROM stock_transactions "
                    "WHERE (item_id = ? OR item_name = ? OR item_code = ?) AND voucher_date < ?;",
                    {itmId, itmName, itmCode, fromDate}
                );
                if (!rPrior.isEmpty()) {
                    QVariantMap rp = rPrior.first().toMap();
                    opQty += (rp.value("in_wt").toDouble() - rp.value("out_wt").toDouble());
                    opBags += (rp.value("in_bg").toLongLong() - rp.value("out_bg").toLongLong());
                }
            }
        }

        if (!m_isDailyDetail) {
            // Monthly Timeline (Apr to Mar)
            QDate sDate = QDate::fromString(fromDate, "yyyy-MM-dd");
            QDate eDate = QDate::fromString(toDate, "yyyy-MM-dd");
            if (!sDate.isValid()) sDate = QDate(QDate::currentDate().year(), 4, 1);
            if (!eDate.isValid()) eDate = QDate::currentDate();

            double runningQty = opQty;
            long long runningBags = opBags;

            // Iterate 12 months or range months
            QDate curMonthStart(sDate.year(), sDate.month(), 1);
            while (curMonthStart <= eDate) {
                QDate curMonthEnd = curMonthStart.addMonths(1).addDays(-1);
                QString mFrom = curMonthStart.toString("yyyy-MM-dd");
                QString mTo = curMonthEnd.toString("yyyy-MM-dd");

                QVariantList mTx = db.executeQuery(
                    "SELECT "
                    "SUM(CASE WHEN trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M') THEN weight_qtl ELSE 0 END) AS in_wt, "
                    "SUM(CASE WHEN trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M') THEN bags ELSE 0 END) AS in_bg, "
                    "SUM(CASE WHEN trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M') THEN amount ELSE 0 END) AS in_val, "
                    "SUM(CASE WHEN trans_type IN ('Sale', 'PrRn', 'Outward', 'S') THEN weight_qtl ELSE 0 END) AS out_wt, "
                    "SUM(CASE WHEN trans_type IN ('Sale', 'PrRn', 'Outward', 'S') THEN bags ELSE 0 END) AS out_bg, "
                    "SUM(CASE WHEN trans_type IN ('Sale', 'PrRn', 'Outward', 'S') THEN amount ELSE 0 END) AS out_val "
                    "FROM stock_transactions "
                    "WHERE (item_id = ? OR item_name = ? OR item_code = ?) AND voucher_date >= ? AND voucher_date <= ?;",
                    {itmId, itmName, itmCode, mFrom, mTo}
                );

                double inWt = 0.0, outWt = 0.0, inVal = 0.0, outVal = 0.0;
                long long inBg = 0, outBg = 0;
                if (!mTx.isEmpty()) {
                    QVariantMap m = mTx.first().toMap();
                    inWt = m.value("in_wt").toDouble();
                    inBg = m.value("in_bg").toLongLong();
                    inVal = m.value("in_val").toDouble();
                    outWt = m.value("out_wt").toDouble();
                    outBg = m.value("out_bg").toLongLong();
                    outVal = m.value("out_val").toDouble();
                }

                double opMonthQty = runningQty;
                long long opMonthBags = runningBags;
                runningQty += (inWt - outWt);
                runningBags += (inBg - outBg);

                double vRate = opRate;
                if (inWt > 0.001 && inVal > 0.0) {
                    vRate = inVal / inWt;
                }
                double cVal = runningQty * vRate;

                StockRegisterEntry e;
                e.id = itmId;
                e.nameVal = itmName;
                e.codeVal = itmCode;
                e.unitVal = itmUnit;
                e.periodDate = curMonthStart.toString("MMMM yyyy");

                e.opBags = opMonthBags;
                e.opQty = opMonthQty;
                e.opQtyVal = QString::number(opMonthQty, 'f', 2);

                e.inBags = inBg;
                e.inQty = inWt;
                e.inQtyVal = QString::number(inWt, 'f', 2);
                e.inVal = inVal;
                e.inValVal = formatINR(inVal);

                e.outBags = outBg;
                e.outQty = outWt;
                e.outQtyVal = QString::number(outWt, 'f', 2);
                e.outVal = outVal;
                e.outValVal = formatINR(outVal);

                e.closeBags = runningBags;
                e.closeQty = runningQty;
                e.closeQtyVal = QString::number(runningQty, 'f', 2);
                e.runningBalance = runningQty;
                e.runningBalanceVal = QString::number(runningQty, 'f', 2);

                e.rate = vRate;
                e.rateVal = formatINR(vRate);
                e.closeVal = cVal;
                e.closeValVal = formatINR(cVal);

                m_allEntries.append(e);
                curMonthStart = curMonthStart.addMonths(1);
            }
        } else {
            // Daily Detail Movements
            double runningQty = opQty;
            long long runningBags = opBags;

            // Opening Balance Row
            StockRegisterEntry opEntry;
            opEntry.id = itmId;
            opEntry.nameVal = itmName;
            opEntry.codeVal = itmCode;
            opEntry.unitVal = itmUnit;
            opEntry.periodDate = fromDate;
            opEntry.voucherType = "OPENING";
            opEntry.partyName = "Opening Stock Balance";
            opEntry.closeBags = runningBags;
            opEntry.closeQty = runningQty;
            opEntry.closeQtyVal = QString::number(runningQty, 'f', 2);
            opEntry.runningBalance = runningQty;
            opEntry.runningBalanceVal = QString::number(runningQty, 'f', 2);
            opEntry.rate = opRate;
            opEntry.rateVal = formatINR(opRate);
            opEntry.closeVal = runningQty * opRate;
            opEntry.closeValVal = formatINR(opEntry.closeVal);
            m_allEntries.append(opEntry);

            QVariantList txRows = db.executeQuery(
                "SELECT voucher_date, voucher_no, trans_type, voucher_type, party_name, bags, weight_qtl, rate, amount "
                "FROM stock_transactions "
                "WHERE (item_id = ? OR item_name = ? OR item_code = ?) AND voucher_date >= ? AND voucher_date <= ? "
                "ORDER BY voucher_date ASC, id ASC;",
                {itmId, itmName, itmCode, fromDate, toDate}
            );

            for (const auto& r : txRows) {
                QVariantMap m = r.toMap();
                QString tType = m.value("trans_type").toString();
                bool isInward = (tType == "Purc" || tType == "SlRn" || tType == "Inward" || tType == "P" || tType == "M");

                double wt = m.value("weight_qtl").toDouble();
                long long bg = m.value("bags").toLongLong();
                double amt = m.value("amount").toDouble();
                double rt = m.value("rate").toDouble();

                double inWt = isInward ? wt : 0.0;
                long long inBg = isInward ? bg : 0;
                double outWt = !isInward ? wt : 0.0;
                long long outBg = !isInward ? bg : 0;

                runningQty += (inWt - outWt);
                runningBags += (inBg - outBg);

                StockRegisterEntry e;
                e.id = itmId;
                e.nameVal = itmName;
                e.codeVal = itmCode;
                e.unitVal = itmUnit;

                QDate dt = QDate::fromString(m.value("voucher_date").toString(), "yyyy-MM-dd");
                e.periodDate = dt.isValid() ? dt.toString("dd-MM-yyyy") : m.value("voucher_date").toString();
                e.voucherNo = m.value("voucher_no").toString();
                e.voucherType = m.value("voucher_type", tType).toString();
                e.partyName = m.value("party_name").toString();

                e.inBags = inBg;
                e.inQty = inWt;
                e.inQtyVal = inWt > 0.0 ? QString::number(inWt, 'f', 2) : "-";
                e.inVal = isInward ? amt : 0.0;
                e.inValVal = isInward ? formatINR(amt) : "-";

                e.outBags = outBg;
                e.outQty = outWt;
                e.outQtyVal = outWt > 0.0 ? QString::number(outWt, 'f', 2) : "-";
                e.outVal = !isInward ? amt : 0.0;
                e.outValVal = !isInward ? formatINR(amt) : "-";

                e.closeBags = runningBags;
                e.closeQty = runningQty;
                e.closeQtyVal = QString::number(runningQty, 'f', 2);
                e.runningBalance = runningQty;
                e.runningBalanceVal = QString::number(runningQty, 'f', 2);

                e.rate = rt > 0.0 ? rt : opMasterRate;
                e.rateVal = formatINR(e.rate);
                e.closeVal = runningQty * e.rate;
                e.closeValVal = formatINR(e.closeVal);

                m_allEntries.append(e);
            }
        }
        return;
    }

    // ========================================================================
    // Modes 1, 2, 3: Only Stock, Stock With Amount, and Item Wise Profit & Loss
    // ========================================================================
    StockItemsModel tempModel;
    QVariantList rawItems = tempModel.get_stock_register(fromDate, toDate);

    // Fetch Master Item metadata for rich categorization (group, company, HSN, purchase rate, sale rate)
    QVariantList itemMasters = db.executeQuery(
        "SELECT id, name, code, item_type, trading_group, company_name, hsn_code, unit, purchase_rate, sale_rate, opening_rate, opening_value "
        "FROM stock_items;"
    );
    QMap<int, QVariantMap> masterMap;
    for (const auto& im : itemMasters) {
        masterMap[im.toMap().value("id").toInt()] = im.toMap();
    }

    // Fetch Outward / Sales Realizations and Cost from transactions to compute true Gross Profit per item
    QVariantList salesRows = db.executeQuery(
        "SELECT item_id, item_name, "
        "SUM(CASE WHEN trans_type IN ('Sale', 'S') THEN weight_qtl ELSE 0 END) AS sale_wt, "
        "SUM(CASE WHEN trans_type IN ('Sale', 'S') THEN amount ELSE 0 END) AS sale_val, "
        "SUM(CASE WHEN trans_type IN ('Purc', 'P', 'Inward') THEN weight_qtl ELSE 0 END) AS purc_wt, "
        "SUM(CASE WHEN trans_type IN ('Purc', 'P', 'Inward') THEN amount ELSE 0 END) AS purc_val "
        "FROM stock_transactions "
        "WHERE voucher_date >= ? AND voucher_date <= ? "
        "GROUP BY item_id, item_name;",
        {fromDate, toDate}
    );
    QMap<int, QVariantMap> salesMap;
    for (const auto& sr : salesRows) {
        salesMap[sr.toMap().value("item_id").toInt()] = sr.toMap();
    }

    // Convert rawItems to rich StockRegisterEntry list
    QVector<StockRegisterEntry> itemEntries;
    itemEntries.reserve(rawItems.size());

    for (const auto& itmVar : rawItems) {
        QVariantMap itm = itmVar.toMap();
        int itemId = itm.value("id").toInt();
        QVariantMap meta = masterMap.value(itemId);

        StockRegisterEntry e;
        e.id = itemId;
        e.nameVal = itm.value("name").toString();
        e.codeVal = itm.value("code").toString();
        e.typeVal = itm.value("item_type").toString();
        e.groupVal = meta.value("trading_group").toString().trimmed();
        if (e.groupVal.isEmpty()) e.groupVal = "General Commodities";
        e.companyVal = meta.value("company_name").toString().trimmed();
        if (e.companyVal.isEmpty()) e.companyVal = "Self Production";
        e.hsnVal = meta.value("hsn_code").toString().trimmed();
        if (e.hsnVal.isEmpty()) e.hsnVal = "1006";
        e.unitVal = itm.value("unit", "Qtl").toString();

        e.opBags = itm.value("opening_bags").toLongLong();
        e.opQty = itm.value("opening_qty").toDouble();
        e.opQtyVal = itm.value("opening_qty_fmt", QString::number(e.opQty, 'f', 2)).toString();

        e.inBags = itm.value("inward_bags").toLongLong();
        e.inQty = itm.value("inward_qty").toDouble();
        e.inQtyVal = itm.value("inward_qty_fmt", QString::number(e.inQty, 'f', 2)).toString();

        e.outBags = itm.value("outward_bags").toLongLong();
        e.outQty = itm.value("outward_qty").toDouble();
        e.outQtyVal = itm.value("outward_qty_fmt", QString::number(e.outQty, 'f', 2)).toString();

        e.closeBags = itm.value("closing_bags").toLongLong();
        e.closeQty = itm.value("closing_qty").toDouble();
        e.closeQtyVal = itm.value("closing_qty_fmt", QString::number(e.closeQty, 'f', 2)).toString();

        double opRate = meta.value("opening_rate", 0.0).toDouble();
        if (opRate <= 0.0 && e.opQty > 0.0) {
            opRate = meta.value("opening_value", 0.0).toDouble() / e.opQty;
        }
        if (opRate <= 0.0) opRate = meta.value("purchase_rate", 0.0).toDouble();
        if (opRate <= 0.0) opRate = itm.value("rate", 0.0).toDouble();

        e.opVal = e.opQty * opRate;
        e.opValVal = formatINR(e.opVal);

        QVariantMap sInfo = salesMap.value(itemId);
        double purcVal = sInfo.value("purc_val").toDouble();
        double purcWt = sInfo.value("purc_wt").toDouble();
        double inRate = (purcWt > 0.001) ? (purcVal / purcWt) : opRate;

        e.inVal = (purcVal > 0.0) ? purcVal : (e.inQty * inRate);
        e.inValVal = formatINR(e.inVal);

        double saleVal = sInfo.value("sale_val").toDouble();
        double saleWt = sInfo.value("sale_wt").toDouble();
        double outRate = (saleWt > 0.001) ? (saleVal / saleWt) : meta.value("sale_rate", opRate).toDouble();

        e.outVal = (saleVal > 0.0) ? saleVal : (e.outQty * outRate);
        e.outValVal = formatINR(e.outVal);

        e.rate = itm.value("rate").toDouble();
        if (e.rate <= 0.0) e.rate = opRate;
        e.rateVal = formatINR(e.rate);

        e.closeVal = itm.value("closing_value").toDouble();
        if (e.closeVal <= 0.0 && e.closeQty > 0.0) e.closeVal = e.closeQty * e.rate;
        e.closeValVal = formatINR(e.closeVal);

        // Profit & Loss Metrics
        e.salesQty = (saleWt > 0.0) ? saleWt : e.outQty;
        e.salesValue = e.outVal;
        e.salesValueVal = formatINR(e.salesValue);

        e.avgSaleRate = (e.salesQty > 0.001) ? (e.salesValue / e.salesQty) : outRate;
        e.avgSaleRateVal = formatINR(e.avgSaleRate);

        e.costRate = (e.inQty > 0.001 && e.inVal > 0.0) ? (e.inVal / e.inQty) : opRate;
        e.costRateVal = formatINR(e.costRate);

        e.cogs = e.salesQty * e.costRate;
        e.cogsVal = formatINR(e.cogs);

        e.grossProfit = e.salesValue - e.cogs;
        e.grossProfitVal = formatINR(e.grossProfit);

        e.gpMarginPct = (e.salesValue > 0.001) ? ((e.grossProfit / e.salesValue) * 100.0) : 0.0;
        e.gpMarginPctVal = QString::number(e.gpMarginPct, 'f', 2) + "%";

        itemEntries.append(e);
    }

    // Apply Grouping if requested
    if (m_grouping == StockGrouping::ItemWise) {
        m_allEntries = itemEntries;
    } else if (m_grouping == StockGrouping::GroupWise) {
        // Group by groupVal
        QMap<QString, StockRegisterEntry> gMap;
        for (const auto& e : itemEntries) {
            QString gKey = e.groupVal.trimmed().isEmpty() ? "General Commodities" : e.groupVal.trimmed();
            if (!gMap.contains(gKey)) {
                StockRegisterEntry g;
                g.nameVal = gKey;
                g.codeVal = QString("GRP-%1").arg(gMap.size() + 1);
                g.typeVal = "Group Summary";
                g.groupVal = gKey;
                g.unitVal = "Qtl";
                g.itemsCount = 0;
                gMap[gKey] = g;
            }
            auto& g = gMap[gKey];
            g.itemsCount++;
            g.opBags += e.opBags;
            g.opQty += e.opQty;
            g.opVal += e.opVal;
            g.inBags += e.inBags;
            g.inQty += e.inQty;
            g.inVal += e.inVal;
            g.outBags += e.outBags;
            g.outQty += e.outQty;
            g.outVal += e.outVal;
            g.closeBags += e.closeBags;
            g.closeQty += e.closeQty;
            g.closeVal += e.closeVal;
            g.salesQty += e.salesQty;
            g.salesValue += e.salesValue;
            g.cogs += e.cogs;
            g.grossProfit += e.grossProfit;
        }
        for (auto& g : gMap) {
            g.opQtyVal = QString::number(g.opQty, 'f', 2);
            g.opValVal = formatINR(g.opVal);
            g.inQtyVal = QString::number(g.inQty, 'f', 2);
            g.inValVal = formatINR(g.inVal);
            g.outQtyVal = QString::number(g.outQty, 'f', 2);
            g.outValVal = formatINR(g.outVal);
            g.closeQtyVal = QString::number(g.closeQty, 'f', 2);
            g.closeValVal = formatINR(g.closeVal);
            g.rate = (g.closeQty > 0.001) ? (g.closeVal / g.closeQty) : 0.0;
            g.rateVal = formatINR(g.rate);

            g.salesValueVal = formatINR(g.salesValue);
            g.avgSaleRate = (g.salesQty > 0.001) ? (g.salesValue / g.salesQty) : 0.0;
            g.avgSaleRateVal = formatINR(g.avgSaleRate);
            g.costRate = (g.salesQty > 0.001) ? (g.cogs / g.salesQty) : 0.0;
            g.costRateVal = formatINR(g.costRate);
            g.cogsVal = formatINR(g.cogs);
            g.grossProfitVal = formatINR(g.grossProfit);
            g.gpMarginPct = (g.salesValue > 0.001) ? ((g.grossProfit / g.salesValue) * 100.0) : 0.0;
            g.gpMarginPctVal = QString::number(g.gpMarginPct, 'f', 2) + "%";

            m_allEntries.append(g);
        }
    } else if (m_grouping == StockGrouping::CompanyWise) {
        // Group by companyVal
        QMap<QString, StockRegisterEntry> cMap;
        for (const auto& e : itemEntries) {
            QString cKey = e.companyVal.trimmed().isEmpty() ? "Self Production" : e.companyVal.trimmed();
            if (!cMap.contains(cKey)) {
                StockRegisterEntry c;
                c.nameVal = cKey;
                c.codeVal = QString("CMP-%1").arg(cMap.size() + 1);
                c.typeVal = "Company Summary";
                c.companyVal = cKey;
                c.unitVal = "Qtl";
                c.itemsCount = 0;
                cMap[cKey] = c;
            }
            auto& c = cMap[cKey];
            c.itemsCount++;
            c.opBags += e.opBags;
            c.opQty += e.opQty;
            c.opVal += e.opVal;
            c.inBags += e.inBags;
            c.inQty += e.inQty;
            c.inVal += e.inVal;
            c.outBags += e.outBags;
            c.outQty += e.outQty;
            c.outVal += e.outVal;
            c.closeBags += e.closeBags;
            c.closeQty += e.closeQty;
            c.closeVal += e.closeVal;
            c.salesQty += e.salesQty;
            c.salesValue += e.salesValue;
            c.cogs += e.cogs;
            c.grossProfit += e.grossProfit;
        }
        for (auto& c : cMap) {
            c.opQtyVal = QString::number(c.opQty, 'f', 2);
            c.opValVal = formatINR(c.opVal);
            c.inQtyVal = QString::number(c.inQty, 'f', 2);
            c.inValVal = formatINR(c.inVal);
            c.outQtyVal = QString::number(c.outQty, 'f', 2);
            c.outValVal = formatINR(c.outVal);
            c.closeQtyVal = QString::number(c.closeQty, 'f', 2);
            c.closeValVal = formatINR(c.closeVal);
            c.rate = (c.closeQty > 0.001) ? (c.closeVal / c.closeQty) : 0.0;
            c.rateVal = formatINR(c.rate);

            c.salesValueVal = formatINR(c.salesValue);
            c.avgSaleRate = (c.salesQty > 0.001) ? (c.salesValue / c.salesQty) : 0.0;
            c.avgSaleRateVal = formatINR(c.avgSaleRate);
            c.costRate = (c.salesQty > 0.001) ? (c.cogs / c.salesQty) : 0.0;
            c.costRateVal = formatINR(c.costRate);
            c.cogsVal = formatINR(c.cogs);
            c.grossProfitVal = formatINR(c.grossProfit);
            c.gpMarginPct = (c.salesValue > 0.001) ? ((c.grossProfit / c.salesValue) * 100.0) : 0.0;
            c.gpMarginPctVal = QString::number(c.gpMarginPct, 'f', 2) + "%";

            m_allEntries.append(c);
        }
    } else if (m_grouping == StockGrouping::HsnWise) {
        // Group by hsnVal
        QMap<QString, StockRegisterEntry> hMap;
        for (const auto& e : itemEntries) {
            QString hKey = e.hsnVal.trimmed().isEmpty() ? "1006" : e.hsnVal.trimmed();
            if (!hMap.contains(hKey)) {
                StockRegisterEntry h;
                h.nameVal = QString("HSN Code: %1").arg(hKey);
                h.codeVal = hKey;
                h.typeVal = "HSN Summary";
                h.hsnVal = hKey;
                h.unitVal = "Qtl";
                h.itemsCount = 0;
                hMap[hKey] = h;
            }
            auto& h = hMap[hKey];
            h.itemsCount++;
            h.opBags += e.opBags;
            h.opQty += e.opQty;
            h.opVal += e.opVal;
            h.inBags += e.inBags;
            h.inQty += e.inQty;
            h.inVal += e.inVal;
            h.outBags += e.outBags;
            h.outQty += e.outQty;
            h.outVal += e.outVal;
            h.closeBags += e.closeBags;
            h.closeQty += e.closeQty;
            h.closeVal += e.closeVal;
            h.salesQty += e.salesQty;
            h.salesValue += e.salesValue;
            h.cogs += e.cogs;
            h.grossProfit += e.grossProfit;
        }
        for (auto& h : hMap) {
            h.opQtyVal = QString::number(h.opQty, 'f', 2);
            h.opValVal = formatINR(h.opVal);
            h.inQtyVal = QString::number(h.inQty, 'f', 2);
            h.inValVal = formatINR(h.inVal);
            h.outQtyVal = QString::number(h.outQty, 'f', 2);
            h.outValVal = formatINR(h.outVal);
            h.closeQtyVal = QString::number(h.closeQty, 'f', 2);
            h.closeValVal = formatINR(h.closeVal);
            h.rate = (h.closeQty > 0.001) ? (h.closeVal / h.closeQty) : 0.0;
            h.rateVal = formatINR(h.rate);

            h.salesValueVal = formatINR(h.salesValue);
            h.avgSaleRate = (h.salesQty > 0.001) ? (h.salesValue / h.salesQty) : 0.0;
            h.avgSaleRateVal = formatINR(h.avgSaleRate);
            h.costRate = (h.salesQty > 0.001) ? (h.cogs / h.salesQty) : 0.0;
            h.costRateVal = formatINR(h.costRate);
            h.cogsVal = formatINR(h.cogs);
            h.grossProfitVal = formatINR(h.grossProfit);
            h.gpMarginPct = (h.salesValue > 0.001) ? ((h.grossProfit / h.salesValue) * 100.0) : 0.0;
            h.gpMarginPctVal = QString::number(h.gpMarginPct, 'f', 2) + "%";

            m_allEntries.append(h);
        }
    } else if (m_grouping == StockGrouping::TotalSummary) {
        // Consolidated Total Row
        StockRegisterEntry t;
        t.nameVal = "CONSOLIDATED INVENTORY TOTAL";
        t.codeVal = "ALL";
        t.typeVal = "Grand Total";
        t.unitVal = "Qtl";
        t.itemsCount = itemEntries.size();

        for (const auto& e : itemEntries) {
            t.opBags += e.opBags;
            t.opQty += e.opQty;
            t.opVal += e.opVal;
            t.inBags += e.inBags;
            t.inQty += e.inQty;
            t.inVal += e.inVal;
            t.outBags += e.outBags;
            t.outQty += e.outQty;
            t.outVal += e.outVal;
            t.closeBags += e.closeBags;
            t.closeQty += e.closeQty;
            t.closeVal += e.closeVal;
            t.salesQty += e.salesQty;
            t.salesValue += e.salesValue;
            t.cogs += e.cogs;
            t.grossProfit += e.grossProfit;
        }
        t.opQtyVal = QString::number(t.opQty, 'f', 2);
        t.opValVal = formatINR(t.opVal);
        t.inQtyVal = QString::number(t.inQty, 'f', 2);
        t.inValVal = formatINR(t.inVal);
        t.outQtyVal = QString::number(t.outQty, 'f', 2);
        t.outValVal = formatINR(t.outVal);
        t.closeQtyVal = QString::number(t.closeQty, 'f', 2);
        t.closeValVal = formatINR(t.closeVal);
        t.rate = (t.closeQty > 0.001) ? (t.closeVal / t.closeQty) : 0.0;
        t.rateVal = formatINR(t.rate);

        t.salesValueVal = formatINR(t.salesValue);
        t.avgSaleRate = (t.salesQty > 0.001) ? (t.salesValue / t.salesQty) : 0.0;
        t.avgSaleRateVal = formatINR(t.avgSaleRate);
        t.costRate = (t.salesQty > 0.001) ? (t.cogs / t.salesQty) : 0.0;
        t.costRateVal = formatINR(t.costRate);
        t.cogsVal = formatINR(t.cogs);
        t.grossProfitVal = formatINR(t.grossProfit);
        t.gpMarginPct = (t.salesValue > 0.001) ? ((t.grossProfit / t.salesValue) * 100.0) : 0.0;
        t.gpMarginPctVal = QString::number(t.gpMarginPct, 'f', 2) + "%";

        m_allEntries.append(t);
    }
}

void StockRegisterController::applyFilter() {
    QString q = m_searchQuery.trimmed().toLower();
    QVector<StockRegisterEntry> filtered;
    filtered.reserve(m_allEntries.size());

    m_totalItemsCount = 0;
    m_totalOpeningQty = 0.0;
    m_totalInwardQty = 0.0;
    m_totalOutwardQty = 0.0;
    m_totalClosingQty = 0.0;
    m_totalClosingVal = 0.0;

    m_totalSalesTurnover = 0.0;
    m_totalCogs = 0.0;
    m_totalGrossProfit = 0.0;

    for (const auto& e : m_allEntries) {
        if (!q.isEmpty()) {
            QString haystack = (e.nameVal + " " + e.codeVal + " " + e.typeVal + " " + e.groupVal + " " + e.companyVal + " " + e.hsnVal + " " + e.periodDate).toLower();
            if (!haystack.contains(q)) {
                continue;
            }
        }

        filtered.append(e);

        m_totalItemsCount++;
        m_totalOpeningQty += e.opQty;
        m_totalInwardQty += e.inQty;
        m_totalOutwardQty += e.outQty;

        if (m_viewMode != StockViewMode::MonthlyDaily) {
            m_totalClosingQty += e.closeQty;
            m_totalClosingVal += e.closeVal;
        }

        m_totalSalesTurnover += e.salesValue;
        m_totalCogs += e.cogs;
        m_totalGrossProfit += e.grossProfit;
    }

    if (m_viewMode == StockViewMode::MonthlyDaily) {
        if (!filtered.isEmpty()) {
            m_totalClosingQty = filtered.last().closeQty;
            m_totalClosingVal = filtered.last().closeVal;
        } else {
            m_totalClosingQty = 0.0;
            m_totalClosingVal = 0.0;
        }
    }

    m_overallGpMargin = (m_totalSalesTurnover > 0.001) ? ((m_totalGrossProfit / m_totalSalesTurnover) * 100.0) : 0.0;

    m_totalClosingQtyFmt = QString::number(m_totalClosingQty, 'f', 2) + " Qtl";
    m_totalClosingValFmt = formatINR(m_totalClosingVal);
    m_totalInwardQtyFmt = QString::number(m_totalInwardQty, 'f', 2) + " Qtl";
    m_totalOutwardQtyFmt = QString::number(m_totalOutwardQty, 'f', 2) + " Qtl";

    m_totalSalesTurnoverFmt = formatINR(m_totalSalesTurnover);
    m_totalCogsFmt = formatINR(m_totalCogs);
    m_totalGrossProfitFmt = formatINR(m_totalGrossProfit);
    m_overallGpMarginFmt = QString::number(m_overallGpMargin, 'f', 2) + "%";

    m_model.setEntries(filtered);
    emit totalsChanged();
}

} // namespace MahadevERP
