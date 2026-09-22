#include "stock_valuation_engine.h"
#include "../database_manager.h"
#include "accounting_engine.h"
#include "fiscal_year_helper.h"
#include <QDebug>
#include <QSet>
#include <QMap>
#include <QHash>
#include <cmath>
#include <algorithm>

bool StockValuationEngine::hasAuditedClosingStock(const QString& asOnDateIso) {
    QString targetDate = FiscalYearHelper::normalizeToIso(asOnDateIso);
    if (targetDate.isEmpty()) return false;

    QVariant countVar = DatabaseManager::instance().executeScalar(
        "SELECT COUNT(*) FROM custom_closing_stocks WHERE closing_date = ? AND amount > 0;",
        {targetDate}
    );
    if (countVar.isValid() && countVar.toInt() > 0) return true;

    FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(targetDate);
    if (fy.isValid() && targetDate >= fy.endDate) {
        QVariant fyCount = DatabaseManager::instance().executeScalar(
            "SELECT COUNT(*) FROM custom_closing_stocks WHERE (financial_year = ? OR financial_year = ?) AND amount > 0;",
            {fy.name, "FY " + fy.name}
        );
        return (fyCount.isValid() && fyCount.toInt() > 0);
    }

    return false;
}

StockValuationReport StockValuationEngine::getAuditedClosingStock(const QString& asOnDateIso) {
    StockValuationReport rep;
    QString targetDate = FiscalYearHelper::normalizeToIso(asOnDateIso);
    if (targetDate.isEmpty()) targetDate = QDate::currentDate().toString("yyyy-MM-dd");

    FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(targetDate);
    rep.asOnDate = targetDate;
    rep.financialYear = fy.name;

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT item_id, item_code, item_name, bags, weight_qtl, rate, amount FROM custom_closing_stocks "
        "WHERE closing_date = ? AND amount > 0 ORDER BY item_name COLLATE NOCASE ASC;",
        {targetDate}
    );

    if (rows.isEmpty() && fy.isValid() && targetDate >= fy.endDate) {
        rows = DatabaseManager::instance().executeQuery(
            "SELECT item_id, item_code, item_name, bags, weight_qtl, rate, amount FROM custom_closing_stocks "
            "WHERE (financial_year = ? OR financial_year = ?) AND amount > 0 ORDER BY item_name COLLATE NOCASE ASC;",
            {fy.name, "FY " + fy.name}
        );
    }

    if (!rows.isEmpty()) {
        rep.isAuditedSnapshot = true;
        for (const auto& rVar : rows) {
            QVariantMap r = rVar.toMap();
            StockValuationItem itm;
            itm.itemId = r.value("item_id").toInt();
            itm.itemCode = r.value("item_code").toString().trimmed();
            itm.itemName = r.value("item_name").toString().trimmed();
            itm.bags = r.value("bags").toInt();
            itm.weightQtl = r.value("weight_qtl").toDouble();
            itm.rate = r.value("rate").toDouble();
            itm.amount = r.value("amount").toDouble();
            itm.amountFmt = AccountingEngine::formatIndianCurrency(itm.amount, true);
            itm.isAudited = true;

            rep.totalBags += itm.bags;
            rep.totalWeightQtl += itm.weightQtl;
            rep.totalValuation += itm.amount;
            rep.items.append(itm);
        }
        rep.totalValuationFmt = AccountingEngine::formatIndianCurrency(rep.totalValuation, true);
    }

    return rep;
}

StockValuationReport StockValuationEngine::calculateLivePhysicalStock(const QString& asOnDateIso) {
    StockValuationReport rep;
    QString targetDate = FiscalYearHelper::normalizeToIso(asOnDateIso);
    if (targetDate.isEmpty()) targetDate = QDate::currentDate().toString("yyyy-MM-dd");

    FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(targetDate);
    rep.asOnDate = targetDate;
    rep.financialYear = fy.name;
    rep.isAuditedSnapshot = false;

    // 1. Fetch all stock items
    QVariantList items = DatabaseManager::instance().executeQuery(
        "SELECT id, code, name, unit, packing_kg, purchase_rate, sale_rate, opening_qty, opening_bags, opening_value "
        "FROM stock_items ORDER BY name COLLATE NOCASE ASC;"
    );
    if (items.isEmpty()) return rep;

    // 2. Find most recent audited closing stock baseline per item before targetDate (if any exists)
    QVariantList customList = DatabaseManager::instance().executeQuery(
        "SELECT item_id, item_name, item_code, closing_date, bags, weight_qtl, rate, amount FROM custom_closing_stocks "
        "WHERE closing_date <= ? ORDER BY closing_date ASC;",
        {targetDate}
    );

    struct BaselineInfo {
        QString date;
        int bags = 0;
        double weight = 0.0;
        double rate = 0.0;
        double amount = 0.0;
    };
    QHash<int, BaselineInfo> baseByItemId;
    QHash<QString, BaselineInfo> baseByCode;
    QHash<QString, BaselineInfo> baseByName;

    for (const auto& cVar : customList) {
        QVariantMap c = cVar.toMap();
        int itemId = c.value("item_id").toInt();
        QString iCode = c.value("item_code").toString().trimmed().toLower();
        QString iName = c.value("item_name").toString().trimmed().toLower();

        BaselineInfo info;
        info.date = c.value("closing_date").toString().trimmed();
        info.bags = c.value("bags").toInt();
        info.weight = c.value("weight_qtl").toDouble();
        info.rate = c.value("rate").toDouble();
        info.amount = c.value("amount").toDouble();

        if (itemId > 0) baseByItemId[itemId] = info;
        if (!iCode.isEmpty()) baseByCode[iCode] = info;
        if (!iName.isEmpty()) baseByName[iName] = info;
    }

    // 3. Batch query stock transactions
    QVariantList transList = DatabaseManager::instance().executeQuery(
        "SELECT item_id, item_code, item_name, trans_type, voucher_date, bags, weight_qtl, rate "
        "FROM stock_transactions WHERE voucher_date <= ?;",
        {targetDate}
    );

    struct TxRow {
        int itemId = 0;
        QString code;
        QString name;
        QString transType;
        QString date;
        int bags = 0;
        double qty = 0.0;
        double rate = 0.0;
    };
    QVector<TxRow> allTx;
    allTx.reserve(transList.size());
    for (const auto& tVar : transList) {
        QVariantMap t = tVar.toMap();
        TxRow tx;
        tx.itemId = t.value("item_id").toInt();
        tx.code = t.value("item_code").toString().trimmed().toLower();
        tx.name = t.value("item_name").toString().trimmed().toLower();
        tx.transType = t.value("trans_type").toString().trimmed();
        tx.date = t.value("voucher_date").toString().trimmed();
        tx.bags = t.value("bags").toInt();
        tx.qty = t.value("weight_qtl").toDouble();
        tx.rate = t.value("rate").toDouble();
        allTx.append(tx);
    }

    // 4. Batch query milling voucher items
    QVariantList millList = DatabaseManager::instance().executeQuery(
        "SELECT item_code, drcr, batch_date, bags, weight_qtl "
        "FROM milling_voucher_items WHERE batch_date <= ?;",
        {targetDate}
    );

    struct MillRow {
        QString code;
        QString drcr;
        QString date;
        int bags = 0;
        double qty = 0.0;
    };
    QVector<MillRow> allMill;
    allMill.reserve(millList.size());
    for (const auto& mVar : millList) {
        QVariantMap m = mVar.toMap();
        MillRow mx;
        mx.code = m.value("item_code").toString().trimmed().toLower();
        mx.drcr = m.value("drcr").toString().trimmed();
        mx.date = m.value("batch_date").toString().trimmed();
        mx.bags = m.value("bags").toInt();
        mx.qty = m.value("weight_qtl").toDouble();
        allMill.append(mx);
    }

    // 5. Compute live physical stock per item
    for (const auto& itemVar : items) {
        QVariantMap item = itemVar.toMap();
        int itemId = item.value("id").toInt();
        QString itemCode = item.value("code").toString().trimmed();
        QString itemName = item.value("name").toString().trimmed();
        QString itemCodeLower = itemCode.toLower();
        QString itemNameLower = itemName.toLower();
        double packingKg = item.value("packing_kg").toDouble();
        if (packingKg <= 0.0) packingKg = 50.0;

        double pRate = item.value("purchase_rate").toDouble();
        double sRate = item.value("sale_rate").toDouble();
        double rate = (pRate > 0.0) ? pRate : ((sRate > 0.0) ? sRate : 3000.0);

        bool hasBaseline = false;
        BaselineInfo bInfo;
        if (itemId > 0 && baseByItemId.contains(itemId)) {
            hasBaseline = true;
            bInfo = baseByItemId[itemId];
        } else if (!itemCodeLower.isEmpty() && baseByCode.contains(itemCodeLower)) {
            hasBaseline = true;
            bInfo = baseByCode[itemCodeLower];
        } else if (!itemNameLower.isEmpty() && baseByName.contains(itemNameLower)) {
            hasBaseline = true;
            bInfo = baseByName[itemNameLower];
        }

        double initQty = 0.0;
        int initBags = 0;
        QString baseDate = "";

        if (hasBaseline) {
            initQty = bInfo.weight;
            initBags = bInfo.bags;
            if (bInfo.rate > 0.0) rate = bInfo.rate;
            baseDate = bInfo.date;
        } else {
            initQty = item.value("opening_qty").toDouble();
            initBags = item.value("opening_bags").toInt();
        }

        double inQty = 0.0, outQty = 0.0;
        int inBags = 0, outBags = 0;
        double latestTxRate = 0.0;
        QString latestTxRateDate = "";

        for (const auto& tx : allTx) {
            bool matches = (tx.itemId == itemId || tx.code == itemCodeLower || tx.name == itemNameLower);
            if (matches && (baseDate.isEmpty() || tx.date > baseDate) && tx.date <= targetDate) {
                if (tx.transType == "Purc" || tx.transType == "Inward" || tx.transType == "P") {
                    inQty += tx.qty;
                    inBags += tx.bags;
                    if (tx.rate > 0.0 && tx.date >= latestTxRateDate) {
                        latestTxRate = tx.rate;
                        latestTxRateDate = tx.date;
                    }
                } else if (tx.transType == "Sale" || tx.transType == "Outward" || tx.transType == "S") {
                    outQty += tx.qty;
                    outBags += tx.bags;
                }
            }
        }

        double millIn = 0.0, millOut = 0.0;
        int millInBags = 0, millOutBags = 0;
        for (const auto& mx : allMill) {
            bool matches = (mx.code == itemCodeLower || mx.code == QString::number(itemId));
            if (matches && (baseDate.isEmpty() || mx.date > baseDate) && mx.date <= targetDate) {
                if (mx.drcr == "Dr") {
                    millIn += mx.qty;
                    millInBags += mx.bags;
                } else if (mx.drcr == "Cr") {
                    millOut += mx.qty;
                    millOutBags += mx.bags;
                }
            }
        }

        double finalQty = initQty + inQty + millIn - outQty - millOut;
        int finalBags = initBags + inBags + millInBags - outBags - millOutBags;

        if (finalBags <= 0 && finalQty > 0.001 && packingKg > 0.0) {
            finalBags = static_cast<int>(std::round((finalQty * 100.0) / packingKg));
        }

        if (latestTxRate > 0.0) {
            rate = latestTxRate;
        }

        if (finalQty > 0.001 || finalBags > 0) {
            StockValuationItem itm;
            itm.itemId = itemId;
            itm.itemCode = itemCode;
            itm.itemName = itemName;
            itm.unit = item.value("unit", "QTL").toString();
            itm.bags = std::max(0, finalBags);
            itm.weightQtl = finalQty;
            itm.rate = rate;
            itm.amount = std::round((finalQty * rate) * 100.0) / 100.0;
            itm.amountFmt = AccountingEngine::formatIndianCurrency(itm.amount, true);
            itm.isAudited = false;

            rep.totalBags += itm.bags;
            rep.totalWeightQtl += itm.weightQtl;
            rep.totalValuation += itm.amount;
            rep.items.append(itm);
        }
    }

    rep.totalValuationFmt = AccountingEngine::formatIndianCurrency(rep.totalValuation, true);
    return rep;
}

StockValuationReport StockValuationEngine::getEffectiveClosingStock(const QString& asOnDateIso) {
    if (hasAuditedClosingStock(asOnDateIso)) {
        return getAuditedClosingStock(asOnDateIso);
    }
    return calculateLivePhysicalStock(asOnDateIso);
}

bool StockValuationEngine::saveAuditedClosingStock(const QString& closingDateIso, const QVector<StockValuationItem>& items, QString& errorOut) {
    QString targetDate = FiscalYearHelper::normalizeToIso(closingDateIso);
    if (targetDate.isEmpty()) {
        errorOut = "Invalid closing date specified.";
        return false;
    }

    FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(targetDate);
    QString fyName = fy.name.isEmpty() ? "FY " + QString::number(QDate::fromString(targetDate, "yyyy-MM-dd").year()) : fy.name;
    QVariant fyIdVar = DatabaseManager::instance().executeScalar(
        "SELECT id FROM financial_years WHERE year_name = ?;",
        {fyName}
    );
    int fyId = fyIdVar.isValid() ? fyIdVar.toInt() : 1;

    auto& db = DatabaseManager::instance();
    db.beginTransaction();

    // Remove existing custom closing stock for this exact date
    db.executeNonQuery(
        "DELETE FROM custom_closing_stocks WHERE closing_date = ?;",
        {targetDate}
    );

    for (const auto& itm : items) {
        if (itm.weightQtl <= 0.001 && itm.amount <= 0.01) continue;

        double rate = itm.rate;
        double amt = itm.amount;
        if (amt <= 0.01 && itm.weightQtl > 0.001 && rate > 0.0) {
            amt = std::round((itm.weightQtl * rate) * 100.0) / 100.0;
        } else if (rate <= 0.01 && itm.weightQtl > 0.001 && amt > 0.01) {
            rate = std::round((amt / itm.weightQtl) * 100.0) / 100.0;
        }

        db.executeNonQuery(
            "INSERT INTO custom_closing_stocks (fy_id, financial_year, closing_date, item_id, item_code, item_name, bags, weight_qtl, rate, amount) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                (fyId > 0 ? QVariant(fyId) : QVariant()),
                fyName,
                targetDate,
                (itm.itemId > 0 ? QVariant(itm.itemId) : QVariant()),
                itm.itemCode,
                itm.itemName,
                itm.bags,
                itm.weightQtl,
                rate,
                amt
            }
        );
    }

    db.commit();
    return true;
}

bool StockValuationEngine::deleteAuditedClosingStock(const QString& closingDateIso, QString& errorOut) {
    QString targetDate = FiscalYearHelper::normalizeToIso(closingDateIso);
    if (targetDate.isEmpty()) {
        errorOut = "Invalid closing date specified.";
        return false;
    }

    DatabaseManager::instance().executeNonQuery(
        "DELETE FROM custom_closing_stocks WHERE closing_date = ?;",
        {targetDate}
    );
    return true;
}

bool StockValuationEngine::autoLockYearEndClosingStock(const QString& asOnDateIso, QString& errorOut) {
    StockValuationReport live = calculateLivePhysicalStock(asOnDateIso);
    if (live.items.isEmpty()) {
        errorOut = "No active physical stock found to lock.";
        return false;
    }
    return saveAuditedClosingStock(asOnDateIso, live.items, errorOut);
}
