#include "stock_pipeline.h"
#include "../database_manager.h"
#include <QMutexLocker>
#include <cmath>
#include <QDebug>

StockPipeline::StockPipeline(QObject* parent)
    : QObject(parent)
{
}

StockPipeline& StockPipeline::instance() {
    static StockPipeline s_instance;
    return s_instance;
}

void StockPipeline::invalidateCache() {
    QMutexLocker locker(&m_mutex);
    m_groupsById.clear();
    m_itemsById.clear();
    m_itemsByName.clear();
    m_itemsByCode.clear();
    m_valid = false;
    emit stockChanged();
}

void StockPipeline::ensureLoaded() {
    if (m_valid) return;

    m_groupsById.clear();
    m_itemsById.clear();
    m_itemsByName.clear();
    m_itemsByCode.clear();

    // 1. Fetch Stock Groups
    QVariantList groupRows = DatabaseManager::instance().executeQuery(
        "SELECT id, legacy_code, group_name, qty_not_show_in_trading, cl_stock_rate FROM stock_groups;"
    );
    for (const auto& gVar : groupRows) {
        QVariantMap g = gVar.toMap();
        StockGroupRecord gr;
        gr.id = g.value("id").toInt();
        gr.legacyCode = g.value("legacy_code").toInt();
        gr.groupName = g.value("group_name").toString().trimmed();
        gr.qtyNotShowInTrading = (g.value("qty_not_show_in_trading").toInt() == 1);
        gr.clStockRate = g.value("cl_stock_rate").toDouble();
        m_groupsById.insert(gr.id, gr);
    }

    // 2. Fetch Stock Items
    QVariantList itemRows = DatabaseManager::instance().executeQuery(
        "SELECT id, legacy_code, code, name, print_name, item_type, group_code, trading_group, "
        "       unit, alt_unit, conversion_factor, packing_kg, hsn_code, gst_rate, "
        "       purchase_rate, sale_rate, opening_bags, opening_qty, opening_rate, opening_value, "
        "       is_milling_item, include_in_trading, calculate_stock "
        "FROM stock_items;"
    );

    for (const auto& iVar : itemRows) {
        QVariantMap i = iVar.toMap();
        StockItemRecord ir;
        ir.id = i.value("id").toInt();
        ir.legacyCode = i.value("legacy_code").toInt();
        ir.code = i.value("code").toString().trimmed();
        ir.name = i.value("name").toString().trimmed();
        ir.printName = i.value("print_name").toString().trimmed();
        ir.itemType = i.value("item_type").toString().trimmed();
        ir.groupId = i.value("group_code").toInt();
        ir.groupName = i.value("trading_group").toString().trimmed();
        ir.unit = i.value("unit").toString().trimmed();
        ir.altUnit = i.value("alt_unit").toString().trimmed();
        ir.conversionFactor = i.value("conversion_factor").toDouble();
        ir.packingKg = i.value("packing_kg").toDouble();
        ir.hsnCode = i.value("hsn_code").toString().trimmed();
        ir.gstRate = i.value("gst_rate").toDouble();
        ir.purchaseRate = i.value("purchase_rate").toDouble();
        ir.saleRate = i.value("sale_rate").toDouble();
        ir.openingBags = i.value("opening_bags").toDouble();
        ir.openingQty = i.value("opening_qty").toDouble();
        ir.openingRate = i.value("opening_rate").toDouble();
        ir.openingValue = i.value("opening_value").toDouble();
        ir.isMillingItem = (i.value("is_milling_item").toInt() == 1);
        ir.includeInTrading = (i.value("include_in_trading").toInt() == 1);
        ir.calculateStock = (i.value("calculate_stock").toInt() == 1);

        m_itemsById.insert(ir.id, ir);
        m_itemsByName.insert(ir.name.toLower(), ir);
        if (!ir.code.isEmpty()) {
            m_itemsByCode.insert(ir.code.toLower(), ir);
        }
    }

    m_valid = true;
}

QVector<StockGroupRecord> StockPipeline::getAllStockGroups() {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();
    QVector<StockGroupRecord> res;
    for (const auto& g : m_groupsById) res.append(g);
    return res;
}

QVector<StockItemRecord> StockPipeline::getAllStockItems() {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();
    QVector<StockItemRecord> res;
    for (const auto& item : m_itemsById) res.append(item);
    return res;
}

StockItemRecord StockPipeline::getItemById(int itemId) {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();
    return m_itemsById.value(itemId);
}

StockItemRecord StockPipeline::getItemByName(const QString& name) {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();
    return m_itemsByName.value(name.trimmed().toLower());
}

StockItemRecord StockPipeline::getItemByCode(const QString& code) {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();
    return m_itemsByCode.value(code.trimmed().toLower());
}

QVector<StockItemBalanceResult> StockPipeline::calculateStockBalances(
    const QString& fromDateIso,
    const QString& toDateIso,
    int filterGroupId
) {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();

    QVector<StockItemBalanceResult> results;

    // Movement aggregator
    struct ItemMovAgg {
        double inwardBags = 0.0;
        double inwardQtl = 0.0;
        double inwardVal = 0.0;
        double outwardBags = 0.0;
        double outwardQtl = 0.0;
        double outwardVal = 0.0;
    };
    QHash<int, ItemMovAgg> aggByItem;

    // 1. Inward movements: Purchases
    QString purcSql = "SELECT item_id, SUM(bag_count) as tot_bags, SUM(weight_qtl) as tot_qtl, SUM(taxable_amount) as tot_val "
                      "FROM purchase_invoices WHERE 1=1 ";
    QVariantList purcParams;
    if (!fromDateIso.isEmpty()) { purcSql += " AND invoice_date >= ?"; purcParams << fromDateIso; }
    if (!toDateIso.isEmpty()) { purcSql += " AND invoice_date <= ?"; purcParams << toDateIso; }
    purcSql += " AND item_id IS NOT NULL AND item_id > 0 GROUP BY item_id;";

    QVariantList purcRows = DatabaseManager::instance().executeQuery(purcSql, purcParams);
    for (const auto& rVar : purcRows) {
        QVariantMap r = rVar.toMap();
        int itId = r.value("item_id").toInt();
        aggByItem[itId].inwardBags += r.value("tot_bags").toDouble();
        aggByItem[itId].inwardQtl += r.value("tot_qtl").toDouble();
        aggByItem[itId].inwardVal += r.value("tot_val").toDouble();
    }

    // 2. Outward movements: Sales
    QString saleSql = "SELECT item_id, SUM(bag_count) as tot_bags, SUM(weight_qtl) as tot_qtl, SUM(taxable_amount) as tot_val "
                      "FROM sales_invoices WHERE 1=1 ";
    QVariantList saleParams;
    if (!fromDateIso.isEmpty()) { saleSql += " AND invoice_date >= ?"; saleParams << fromDateIso; }
    if (!toDateIso.isEmpty()) { saleSql += " AND invoice_date <= ?"; saleParams << toDateIso; }
    saleSql += " AND item_id IS NOT NULL AND item_id > 0 GROUP BY item_id;";

    QVariantList saleRows = DatabaseManager::instance().executeQuery(saleSql, saleParams);
    for (const auto& rVar : saleRows) {
        QVariantMap r = rVar.toMap();
        int itId = r.value("item_id").toInt();
        aggByItem[itId].outwardBags += r.value("tot_bags").toDouble();
        aggByItem[itId].outwardQtl += r.value("tot_qtl").toDouble();
        aggByItem[itId].outwardVal += r.value("tot_val").toDouble();
    }

    // 3. Milling Movements
    QString millSql = "SELECT item_id, drcr, SUM(bags) as tot_bags, SUM(weight_qtl) as tot_qtl, SUM(amount) as tot_val "
                      "FROM milling_voucher_items WHERE 1=1 ";
    QVariantList millParams;
    if (!fromDateIso.isEmpty()) { millSql += " AND batch_date >= ?"; millParams << fromDateIso; }
    if (!toDateIso.isEmpty()) { millSql += " AND batch_date <= ?"; millParams << toDateIso; }
    millSql += " AND item_id IS NOT NULL AND item_id > 0 GROUP BY item_id, drcr;";

    QVariantList millRows = DatabaseManager::instance().executeQuery(millSql, millParams);
    for (const auto& rVar : millRows) {
        QVariantMap r = rVar.toMap();
        int itId = r.value("item_id").toInt();
        QString drcr = r.value("drcr").toString().trimmed();
        if (drcr.compare("Cr", Qt::CaseInsensitive) == 0) {
            // Output from mill = Inward finished product
            aggByItem[itId].inwardBags += r.value("tot_bags").toDouble();
            aggByItem[itId].inwardQtl += r.value("tot_qtl").toDouble();
            aggByItem[itId].inwardVal += r.value("tot_val").toDouble();
        } else {
            // Input to mill = Outward raw paddy consumed
            aggByItem[itId].outwardBags += r.value("tot_bags").toDouble();
            aggByItem[itId].outwardQtl += r.value("tot_qtl").toDouble();
            aggByItem[itId].outwardVal += r.value("tot_val").toDouble();
        }
    }

    // 4. Compute closing stock for each item
    for (const auto& item : m_itemsById) {
        if (filterGroupId > 0 && item.groupId != filterGroupId) continue;

        StockItemBalanceResult res;
        res.itemId = item.id;
        res.itemCode = item.code;
        res.itemName = item.name;
        res.groupName = item.groupName;
        res.unit = item.unit;
        res.packingKg = item.packingKg;
        res.includeInTrading = item.includeInTrading;

        // Check group flags
        StockGroupRecord grp = m_groupsById.value(item.groupId);
        res.qtyNotShowInTrading = grp.qtyNotShowInTrading;

        res.openingBags = item.openingBags;
        res.openingWeightQtl = item.openingQty;
        res.openingRate = item.openingRate;
        res.openingValue = item.openingValue;
        if (res.openingValue <= 0.0 && res.openingWeightQtl > 0.0 && res.openingRate > 0.0) {
            res.openingValue = res.openingWeightQtl * res.openingRate;
        }

        ItemMovAgg mov = aggByItem.value(item.id);
        res.inwardBags = mov.inwardBags;
        res.inwardWeightQtl = mov.inwardQtl;
        res.inwardValue = mov.inwardVal;

        res.outwardBags = mov.outwardBags;
        res.outwardWeightQtl = mov.outwardQtl;
        res.outwardValue = mov.outwardVal;

        res.closingBags = res.openingBags + res.inwardBags - res.outwardBags;
        res.closingWeightQtl = res.openingWeightQtl + res.inwardWeightQtl - res.outwardWeightQtl;

        // Valuation Rate
        double valRate = item.purchaseRate;
        if (valRate <= 0.0 && res.inwardWeightQtl > 0.0 && res.inwardValue > 0.0) {
            valRate = res.inwardValue / res.inwardWeightQtl;
        }
        if (valRate <= 0.0) {
            valRate = item.openingRate;
        }
        res.closingRate = valRate;
        res.closingValue = (res.closingWeightQtl > 0.0) ? (res.closingWeightQtl * res.closingRate) : 0.0;

        results.append(res);
    }

    return results;
}

StockPipeline::TradingAccountSection StockPipeline::generateTradingAccount(
    const QString& fromDateIso,
    const QString& toDateIso
) {
    QVector<StockItemBalanceResult> balances = calculateStockBalances(fromDateIso, toDateIso);
    TradingAccountSection section;

    for (const auto& b : balances) {
        if (!b.includeInTrading) continue;

        section.openingStockValue += b.openingValue;
        section.purchasesValue += b.inwardValue;
        section.salesValue += b.outwardValue;
        section.closingStockValue += b.closingValue;
        section.tradingItems.append(b);
    }

    // Direct Expenses (Mandi Fee, Dami, Labour, Freight Inward)
    QVariant directExpVal = DatabaseManager::instance().executeScalar(
        "SELECT SUM(amount) FROM transactions t "
        "JOIN parties p ON t.party_id = p.id "
        "JOIN account_groups g ON p.group_id = g.id OR LOWER(p.group_name) = LOWER(g.name) "
        "WHERE t.voucher_date >= ? AND t.voucher_date <= ? AND LOWER(g.nature) = 'direct expense' AND t.dr_cr = 'Dr';",
        { fromDateIso, toDateIso }
    );
    section.directExpensesValue = directExpVal.toDouble();

    double totalCredit = section.salesValue + section.closingStockValue;
    double totalDebit = section.openingStockValue + section.purchasesValue + section.directExpensesValue;

    if (totalCredit >= totalDebit) {
        section.grossProfit = totalCredit - totalDebit;
        section.grossLoss = 0.0;
    } else {
        section.grossProfit = 0.0;
        section.grossLoss = totalDebit - totalCredit;
    }

    return section;
}

QVariantList StockPipeline::searchItems(const QString& query, int limit) {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();

    QString q = query.trimmed().toLower();
    QVariantList list;

    for (const auto& item : m_itemsById) {
        if (!q.isEmpty()) {
            bool matches = item.name.toLower().contains(q) ||
                            item.code.toLower().contains(q) ||
                            item.groupName.toLower().contains(q) ||
                            item.hsnCode.contains(q);
            if (!matches) continue;
        }

        QVariantMap m;
        m["id"] = item.id;
        m["code"] = item.code;
        m["name"] = item.name;
        m["group_name"] = item.groupName;
        m["unit"] = item.unit;
        m["packing_kg"] = item.packingKg;
        m["purchase_rate"] = item.purchaseRate;
        m["sale_rate"] = item.saleRate;
        m["gst_rate"] = item.gstRate;
        m["hsn_code"] = item.hsnCode;
        list.append(m);

        if (limit > 0 && list.size() >= limit) break;
    }
    return list;
}

QStringList StockPipeline::getItemNames() {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();
    QStringList names;
    for (const auto& item : m_itemsById) names << item.name;
    names.sort(Qt::CaseInsensitive);
    return names;
}

QStringList StockPipeline::getStockGroupNames() {
    QMutexLocker locker(&m_mutex);
    ensureLoaded();
    QStringList names;
    for (const auto& g : m_groupsById) names << g.groupName;
    names.sort(Qt::CaseInsensitive);
    return names;
}

QStringList StockPipeline::getStockUnitNames() {
    QVariantList rows = DatabaseManager::instance().executeQuery("SELECT unit_name FROM stock_units;");
    QStringList units;
    for (const auto& r : rows) units << r.toMap().value("unit_name").toString();
    if (units.isEmpty()) units << "Qtl." << "Bags" << "Kg." << "Nos" << "Mtr.";
    return units;
}

bool StockPipeline::saveStockItem(const StockItemRecord& item, QString* outError) {
    QString trimmedName = item.name.trimmed();
    if (trimmedName.isEmpty()) {
        if (outError) *outError = "Item name cannot be empty.";
        return false;
    }

    bool success = false;
    if (item.id > 0) {
        success = DatabaseManager::instance().executeNonQuery(
            "UPDATE stock_items SET name = ?, code = ?, print_name = ?, item_type = ?, trading_group = ?, "
            "unit = ?, alt_unit = ?, conversion_factor = ?, packing_kg = ?, hsn_code = ?, gst_rate = ?, "
            "purchase_rate = ?, sale_rate = ?, is_milling_item = ?, include_in_trading = ?, calculate_stock = ? "
            "WHERE id = ?;",
            { trimmedName, item.code, item.printName, item.itemType, item.groupName,
              item.unit, item.altUnit, item.conversionFactor, item.packingKg, item.hsnCode, item.gstRate,
              item.purchaseRate, item.saleRate, item.isMillingItem ? 1 : 0, item.includeInTrading ? 1 : 0,
              item.calculateStock ? 1 : 0, item.id }
        );
    } else {
        success = DatabaseManager::instance().executeNonQuery(
            "INSERT INTO stock_items (name, code, print_name, item_type, trading_group, unit, alt_unit, "
            "conversion_factor, packing_kg, hsn_code, gst_rate, purchase_rate, sale_rate, is_milling_item, "
            "include_in_trading, calculate_stock) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            { trimmedName, item.code, item.printName, item.itemType, item.groupName,
              item.unit, item.altUnit, item.conversionFactor, item.packingKg, item.hsnCode, item.gstRate,
              item.purchaseRate, item.saleRate, item.isMillingItem ? 1 : 0, item.includeInTrading ? 1 : 0,
              item.calculateStock ? 1 : 0 }
        );
    }

    if (success) {
        invalidateCache();
    } else if (outError) {
        *outError = "Database error while saving stock item.";
    }
    return success;
}

bool StockPipeline::deleteStockItem(int itemId, QString* outError) {
    if (itemId <= 0) return false;

    // Check usage in sales/purchases
    QVariant saleCnt = DatabaseManager::instance().executeScalar(
        "SELECT COUNT(*) FROM sales_invoices WHERE item_id = ?;", { itemId }
    );
    if (saleCnt.toInt() > 0) {
        if (outError) *outError = "Cannot delete item used in sales invoices.";
        return false;
    }

    bool success = DatabaseManager::instance().executeNonQuery(
        "DELETE FROM stock_items WHERE id = ?;", { itemId }
    );
    if (success) {
        invalidateCache();
    } else if (outError) {
        *outError = "Database error while deleting stock item.";
    }
    return success;
}
