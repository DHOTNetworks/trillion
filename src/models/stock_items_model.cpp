#include "stock_items_model.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include <algorithm>

StockItemsModel::StockItemsModel(QObject* parent)
    : BaseTableModel(
        {"Item Name", "Code", "Type", "Unit", "Packing (kg)", "Purchase Rate", "Sale Rate", "GST Rate %", "HSN"},
        {"name", "code", "item_type", "unit", "packing_kg", "purchase_rate", "sale_rate", "gst_rate", "hsn_code"},
        parent
    )
{
    initActivePeriod();
    reload_data();
}

void StockItemsModel::initActivePeriod() {
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT year_name, start_date, end_date FROM financial_years WHERE is_active = 1 LIMIT 1;"
    );
    if (rows.isEmpty()) {
        rows = DatabaseManager::instance().executeQuery(
            "SELECT year_name, start_date, end_date FROM financial_years ORDER BY start_date DESC LIMIT 1;"
        );
    }
    if (!rows.isEmpty()) {
        QVariantMap r = rows.first().toMap();
        m_currentFinancialYear = r.value("year_name").toString();
        m_currentFromDate = r.value("start_date").toString();
        m_currentToDate = r.value("end_date").toString();
    }
}

void StockItemsModel::reload_data() {
    beginResetModel();
    m_data = DatabaseManager::instance().executeQuery("SELECT * FROM stock_items ORDER BY name COLLATE NOCASE ASC;");
    endResetModel();
    emit dataChangedSignal();
    emit countChanged();
}

QStringList StockItemsModel::get_items_list() const {
    QStringList list;
    for (const QVariant& v : m_data) {
        QString n = v.toMap().value("name").toString();
        if (!n.isEmpty()) list.append(n);
    }
    std::sort(list.begin(), list.end(), [](const QString& a, const QString& b) {
        return a.compare(b, Qt::CaseInsensitive) < 0;
    });
    return list;
}

QVariantMap StockItemsModel::get_item_by_name(const QString& name) const {
    QString cleanName = name.trimmed();
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM stock_items WHERE name = ? OR name LIKE ? LIMIT 1;",
        {cleanName, "%" + cleanName + "%"}
    );
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    return QVariantMap();
}

QStringList StockItemsModel::get_item_types() const {
    return {"By-Product", "Finished Rice", "General Goods", "Packing Material", "Raw Paddy"};
}

QStringList StockItemsModel::get_units() const {
    return {"Bags", "Kg", "Ltr", "MT", "Nos", "Pcs", "Qtl"};
}

QStringList StockItemsModel::get_gst_rates() const {
    return {"0%", "5%", "12%", "18%", "28%"};
}

bool StockItemsModel::add_stock_item(
    const QString& name, const QString& code, const QString& item_type,
    const QString& company_name, const QString& unit, double purchase_rate,
    double sale_rate, double mrp, double discount, const QString& hsn_code,
    double gst_rate, double cess_rate, double packing_kg, int opening_bags,
    double opening_qty, double opening_rate, double opening_value,
    const QString& purchase_ledger, const QString& sale_ledger, const QString& stock_ledger
) {
    bool ok = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO stock_items ("
        "name, code, item_type, goods_type, company_name, category_name, unit, purchase_rate, sale_rate, mrp, discount, "
        "hsn_code, gst_rate, cess_rate, packing_kg, opening_bags, opening_qty, opening_rate, opening_value, "
        "purchase_ledger, sale_ledger, stock_ledger, is_milling_item, include_in_trading, calculate_stock"
        ") VALUES (?, ?, ?, 'Goods', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 1, 1, 1);",
        {
            name, code, item_type, company_name, item_type, unit, purchase_rate, sale_rate, mrp, discount,
            hsn_code, gst_rate, cess_rate, packing_kg, opening_bags, opening_qty, opening_rate, opening_value,
            purchase_ledger, sale_ledger, stock_ledger
        }
    );
    if (ok) reload_data();
    return ok;
}

bool StockItemsModel::update_stock_item(
    int item_id, const QString& name, const QString& code, const QString& item_type,
    const QString& company_name, const QString& unit, double purchase_rate,
    double sale_rate, double mrp, double discount, const QString& hsn_code,
    double gst_rate, double cess_rate, double packing_kg, int opening_bags,
    double opening_qty, double opening_rate, double opening_value,
    const QString& purchase_ledger, const QString& sale_ledger, const QString& stock_ledger
) {
    bool ok = DatabaseManager::instance().executeNonQuery(
        "UPDATE stock_items SET name = ?, code = ?, item_type = ?, company_name = ?, category_name = ?, unit = ?, "
        "purchase_rate = ?, sale_rate = ?, mrp = ?, discount = ?, hsn_code = ?, gst_rate = ?, cess_rate = ?, "
        "packing_kg = ?, opening_bags = ?, opening_qty = ?, opening_rate = ?, opening_value = ?, "
        "purchase_ledger = ?, sale_ledger = ?, stock_ledger = ? WHERE id = ?;",
        {
            name, code, item_type, company_name, item_type, unit, purchase_rate, sale_rate, mrp, discount,
            hsn_code, gst_rate, cess_rate, packing_kg, opening_bags, opening_qty, opening_rate, opening_value,
            purchase_ledger, sale_ledger, stock_ledger, item_id
        }
    );
    if (ok) reload_data();
    return ok;
}

bool StockItemsModel::delete_stock_item(int item_id) {
    bool ok = DatabaseManager::instance().executeNonQuery(
        "DELETE FROM stock_items WHERE id = ?;",
        {item_id}
    );
    if (ok) reload_data();
    return ok;
}

QString StockItemsModel::get_financial_year() const {
    return m_currentFinancialYear;
}

QString StockItemsModel::get_from_date() const {
    return m_currentFromDate;
}

QString StockItemsModel::get_to_date() const {
    return m_currentToDate;
}

void StockItemsModel::set_financial_year(const QString& fy) {
    m_currentFinancialYear = fy;
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT start_date, end_date FROM financial_years WHERE year_name = ? LIMIT 1;",
        {fy}
    );
    if (!rows.isEmpty()) {
        m_currentFromDate = rows.first().toMap().value("start_date").toString();
        m_currentToDate = rows.first().toMap().value("end_date").toString();
        DatabaseManager::instance().executeNonQuery("UPDATE financial_years SET is_active = 0;");
        DatabaseManager::instance().executeNonQuery("UPDATE financial_years SET is_active = 1 WHERE year_name = ?;", {fy});
    }
    AccountingEngine::setActivePeriod(m_currentFromDate, m_currentToDate, m_currentFinancialYear);
    reload_data();
}

void StockItemsModel::set_accounting_period(const QString& fromDate, const QString& toDate, const QString& fyLabel) {
    m_currentFromDate = fromDate;
    m_currentToDate = toDate;
    m_currentFinancialYear = !fyLabel.isEmpty() ? fyLabel : QString("%1 To %2").arg(fromDate, toDate);

    DatabaseManager::instance().executeNonQuery("UPDATE financial_years SET is_active = 0;");
    if (!fromDate.isEmpty() && !toDate.isEmpty()) {
        QVariantList match = DatabaseManager::instance().executeQuery(
            "SELECT id, year_name FROM financial_years WHERE year_name = ? OR (start_date = ? AND end_date = ?) LIMIT 1;",
            {fyLabel, fromDate, toDate}
        );
        if (!match.isEmpty()) {
            int id = match.first().toMap().value("id").toInt();
            QString matchedName = match.first().toMap().value("year_name").toString();
            m_currentFinancialYear = matchedName;
            DatabaseManager::instance().executeNonQuery("UPDATE financial_years SET is_active = 1 WHERE id = ?;", {id});
        }
    }
    AccountingEngine::setActivePeriod(m_currentFromDate, m_currentToDate, m_currentFinancialYear);
    reload_data();
}

QVariantList StockItemsModel::get_available_financial_years() const {
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT year_name, start_date, end_date, is_active FROM financial_years ORDER BY start_date ASC;"
    );
    QVariantList result;
    for (const QVariant& r : rows) {
        QVariantMap row = r.toMap();
        QString yName = row.value("year_name").toString();
        bool isActive = row.value("is_active").toBool();
        QString sd = row.value("start_date").toString();
        QString ed = row.value("end_date").toString();

        QString sFmt = sd;
        QString eFmt = ed;
        QStringList sParts = sd.split('-');
        if (sParts.size() == 3) sFmt = QString("%1-%2-%3").arg(sParts[2], sParts[1], sParts[0]);
        QStringList eParts = ed.split('-');
        if (eParts.size() == 3) eFmt = QString("%1-%2-%3").arg(eParts[2], eParts[1], eParts[0]);

        QVariantMap m;
        m["name"] = yName;
        m["label"] = isActive ? (yName + " (Active)") : yName;
        m["startDate"] = sd;
        m["endDate"] = ed;
        m["startFormatted"] = sFmt;
        m["endFormatted"] = eFmt;
        m["isActive"] = isActive;
        result.append(m);
    }
    QVariantMap allOpt;
    allOpt["name"] = "All";
    allOpt["label"] = "All Financial Years";
    allOpt["startDate"] = "";
    allOpt["endDate"] = "";
    allOpt["startFormatted"] = "Beginning";
    allOpt["endFormatted"] = "Latest";
    allOpt["isActive"] = false;
    result.append(allOpt);
    return result;
}

QVariantList StockItemsModel::get_stock_register(const QString& param1, const QString& param2) {
    QString fDate = AccountingEngine::getActiveFromDate();
    QString tDate = AccountingEngine::getActiveToDate();
    if (fDate.isEmpty() && tDate.isEmpty()) {
        fDate = m_currentFromDate;
        tDate = m_currentToDate;
    }
    if (!param1.isEmpty() && !param2.isEmpty()) {
        fDate = param1;
        tDate = param2;
    } else if (!param1.isEmpty() && param1 != "All") {
        QVariantList fyRow = DatabaseManager::instance().executeQuery(
            "SELECT start_date, end_date FROM financial_years WHERE year_name = ? LIMIT 1;",
            {param1}
        );
        if (!fyRow.isEmpty()) {
            fDate = fyRow.first().toMap().value("start_date").toString();
            tDate = fyRow.first().toMap().value("end_date").toString();
        }
    }

    QVariantList items = DatabaseManager::instance().executeQuery("SELECT * FROM stock_items ORDER BY name COLLATE NOCASE ASC;");
    QVariantList result;

    for (const QVariant& itmVar : items) {
        QVariantMap item = itmVar.toMap();
        QString name = item.value("name").toString();
        int itemId = item.value("id").toInt();
        QString code = item.value("code").toString();
        QString itemType = item.value("item_type").toString();
        if (itemType.isEmpty()) itemType = "General Goods";
        QString unit = item.value("unit").toString();
        if (unit.isEmpty()) unit = "Qtl";

        double opQty = item.value("opening_qty").toDouble();
        long long opBags = item.value("opening_bags").toLongLong();

        // 1. Opening Stock from custom_closing_stocks or master opening
        if (!fDate.isEmpty()) {
            QVariantList customRows = DatabaseManager::instance().executeQuery(
                "SELECT closing_date, bags, weight_qtl FROM custom_closing_stocks WHERE (item_id = ? OR item_name = ? OR item_code = ?) AND closing_date < ? ORDER BY closing_date DESC LIMIT 1;",
                {itemId, name, code, fDate}
            );
            if (!customRows.isEmpty()) {
                QVariantMap customRow = customRows.first().toMap();
                QString cDate = customRow.value("closing_date").toString();
                opBags = customRow.value("bags").toLongLong();
                opQty = customRow.value("weight_qtl").toDouble();

                // Add intermediate stock transactions between audited closing date and fDate
                QVariantList rInt = DatabaseManager::instance().executeQuery(
                    "SELECT "
                    "SUM(CASE WHEN trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M') THEN weight_qtl ELSE 0 END) AS in_wt, "
                    "SUM(CASE WHEN trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M') THEN bags ELSE 0 END) AS in_bg, "
                    "SUM(CASE WHEN trans_type IN ('Sale', 'PrRn', 'Outward', 'S') THEN weight_qtl ELSE 0 END) AS out_wt, "
                    "SUM(CASE WHEN trans_type IN ('Sale', 'PrRn', 'Outward', 'S') THEN bags ELSE 0 END) AS out_bg "
                    "FROM stock_transactions "
                    "WHERE (item_id = ? OR item_name = ? OR item_code = ?) AND voucher_date > ? AND voucher_date < ?;",
                    {itemId, name, code, cDate, fDate}
                );
                if (!rInt.isEmpty()) {
                    QVariantMap ri = rInt.first().toMap();
                    opQty += (ri.value("in_wt").toDouble() - ri.value("out_wt").toDouble());
                    opBags += (ri.value("in_bg").toLongLong() - ri.value("out_bg").toLongLong());
                }
            } else {
                // If no custom_closing_stocks before fDate, roll forward master opening with prior transactions
                QVariantList rPrior = DatabaseManager::instance().executeQuery(
                    "SELECT "
                    "SUM(CASE WHEN trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M') THEN weight_qtl ELSE 0 END) AS in_wt, "
                    "SUM(CASE WHEN trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M') THEN bags ELSE 0 END) AS in_bg, "
                    "SUM(CASE WHEN trans_type IN ('Sale', 'PrRn', 'Outward', 'S') THEN weight_qtl ELSE 0 END) AS out_wt, "
                    "SUM(CASE WHEN trans_type IN ('Sale', 'PrRn', 'Outward', 'S') THEN bags ELSE 0 END) AS out_bg "
                    "FROM stock_transactions "
                    "WHERE (item_id = ? OR item_name = ? OR item_code = ?) AND voucher_date < ?;",
                    {itemId, name, code, fDate}
                );
                if (!rPrior.isEmpty()) {
                    QVariantMap rp = rPrior.first().toMap();
                    opQty += (rp.value("in_wt").toDouble() - rp.value("out_wt").toDouble());
                    opBags += (rp.value("in_bg").toLongLong() - rp.value("out_bg").toLongLong());
                }
            }
        }

        // 2. Inwards from Stock Transactions (Purchases & Sales Returns)
        double inwardPur = 0.0;
        long long inwardPurBags = 0;
        QString stInSql = "SELECT "
                          "SUM(CASE WHEN trans_type IN ('Purc', 'Inward', 'P', 'M', 'SlRn') THEN weight_qtl ELSE 0 END) AS in_wt, "
                          "SUM(CASE WHEN trans_type IN ('Purc', 'Inward', 'P', 'M', 'SlRn') THEN bags ELSE 0 END) AS in_bg "
                          "FROM stock_transactions WHERE (item_id = ? OR item_name = ? OR item_code = ?) AND trans_type IN ('Purc', 'Inward', 'P', 'M', 'SlRn')";
        QVariantList stInParams = {itemId, name, code};
        if (!fDate.isEmpty() && !tDate.isEmpty()) {
            stInSql += " AND voucher_date >= ? AND voucher_date <= ?";
            stInParams << fDate << tDate;
        }
        QVariantList stInRows = DatabaseManager::instance().executeQuery(stInSql, stInParams);
        if (!stInRows.isEmpty()) {
            inwardPur = stInRows.first().toMap().value("in_wt").toDouble();
            inwardPurBags = stInRows.first().toMap().value("in_bg").toLongLong();
        }

        // Inward from Milling Production & Arrivals
        double inwardMilling = 0.0;
        long long inwardMillingBags = 0;
        QString mbSql = "SELECT SUM(weight_qtl) FROM milling_voucher_items WHERE (item_id = ? OR item_name = ? OR item_code = ?) AND drcr = 'Dr'";
        QVariantList mbParams = {itemId, name, code};
        if (!fDate.isEmpty() && !tDate.isEmpty()) {
            mbSql += " AND batch_date >= ? AND batch_date <= ?";
            mbParams << fDate << tDate;
        }
        QVariant mbVal = DatabaseManager::instance().executeScalar(mbSql, mbParams);
        if (mbVal.isValid()) {
            inwardMilling = mbVal.toDouble();
            inwardMillingBags = 0; // Milling output is loose weight (0 bags)
        }

        // Inward from Paddy Procurement / Arrivals (for Raw Paddy items)
        double inwardPaddy = 0.0;
        long long inwardPaddyBags = 0;
        if (name.contains("Paddy", Qt::CaseInsensitive)) {
            QString padSql = "SELECT SUM(net_weight_qtl) AS in_wt, SUM(bag_count) AS in_bg FROM paddy_procurement WHERE variety = ?";
            QVariantList padParams = {name};
            if (!fDate.isEmpty() && !tDate.isEmpty()) {
                padSql += " AND arrival_date >= ? AND arrival_date <= ?";
                padParams << fDate << tDate;
            }
            QVariantList padRows = DatabaseManager::instance().executeQuery(padSql, padParams);
            if (!padRows.isEmpty()) {
                inwardPaddy = padRows.first().toMap().value("in_wt").toDouble();
                inwardPaddyBags = padRows.first().toMap().value("in_bg").toLongLong();
            }
        }

        // Outwards from Stock Transactions (Sales & Purchase Returns)
        double outwardSales = 0.0;
        long long outwardSalesBags = 0;
        QString stOutSql = "SELECT "
                           "SUM(CASE WHEN trans_type IN ('Sale', 'Outward', 'S', 'PrRn') THEN weight_qtl ELSE 0 END) AS out_wt, "
                           "SUM(CASE WHEN trans_type IN ('Sale', 'Outward', 'S', 'PrRn') THEN bags ELSE 0 END) AS out_bg "
                           "FROM stock_transactions WHERE (item_id = ? OR item_name = ? OR item_code = ?) AND trans_type IN ('Sale', 'Outward', 'S', 'PrRn')";
        QVariantList stOutParams = {itemId, name, code};
        if (!fDate.isEmpty() && !tDate.isEmpty()) {
            stOutSql += " AND voucher_date >= ? AND voucher_date <= ?";
            stOutParams << fDate << tDate;
        }
        QVariantList stOutRows = DatabaseManager::instance().executeQuery(stOutSql, stOutParams);
        if (!stOutRows.isEmpty()) {
            outwardSales = stOutRows.first().toMap().value("out_wt").toDouble();
            outwardSalesBags = stOutRows.first().toMap().value("out_bg").toLongLong();
        }

        // Outward for Paddy Milled (Consumption from milling_voucher_items Cr)
        double outwardMillingPaddy = 0.0;
        long long outwardMillingPaddyBags = 0;
        QString padMillSql = "SELECT SUM(weight_qtl) FROM milling_voucher_items WHERE (item_id = ? OR item_name = ? OR item_code = ?) AND drcr = 'Cr'";
        QVariantList padMillParams = {itemId, name, code};
        if (!fDate.isEmpty() && !tDate.isEmpty()) {
            padMillSql += " AND batch_date >= ? AND batch_date <= ?";
            padMillParams << fDate << tDate;
        }
        QVariant padMillVal = DatabaseManager::instance().executeScalar(padMillSql, padMillParams);
        if (padMillVal.isValid()) {
            outwardMillingPaddy = padMillVal.toDouble();
            outwardMillingPaddyBags = 0; // Paddy milled from silo/bulk is loose
        }

        double inwardTotal = inwardPur + inwardPaddy + inwardMilling;
        long long inwardBagsTotal = inwardPurBags + inwardPaddyBags + inwardMillingBags;
        double outwardTotal = outwardSales + outwardMillingPaddy;
        long long outwardBagsTotal = outwardSalesBags + outwardMillingPaddyBags;

        double closingQty = opQty + inwardTotal - outwardTotal;
        long long closingBags = opBags + inwardBagsTotal - outwardBagsTotal;

        double rate = item.value("sale_rate").toDouble();
        if (rate <= 0.0) rate = item.value("purchase_rate").toDouble();

        if (rate <= 0.0) {
            QVariant avgR = DatabaseManager::instance().executeScalar(
                "SELECT AVG(rate) FROM stock_transactions WHERE (item_id = ? OR item_name = ? OR item_code = ?) AND rate > 0;",
                {itemId, name, code}
            );
            if (avgR.isValid() && avgR.toDouble() > 0.0) rate = std::round(avgR.toDouble() * 100.0) / 100.0;
            else rate = 2500.0;
        }

        double closingVal = closingQty * rate;

        QVariantMap row;
        row["id"] = itemId;
        row["name"] = name;
        row["code"] = code;
        row["item_type"] = itemType;
        row["unit"] = unit;
        row["opening_bags"] = opBags;
        row["opening_qty"] = opQty;
        row["inward_bags"] = inwardBagsTotal;
        row["inward_qty"] = inwardTotal;
        row["outward_bags"] = outwardBagsTotal;
        row["outward_qty"] = outwardTotal;
        row["closing_bags"] = closingBags;
        row["closing_qty"] = closingQty;
        row["rate"] = rate;
        row["closing_value"] = closingVal;

        row["opening_qty_fmt"] = AccountingEngine::formatIndianNumber(opQty, 2, unit);
        row["inward_qty_fmt"] = AccountingEngine::formatIndianNumber(inwardTotal, 2, unit);
        row["outward_qty_fmt"] = AccountingEngine::formatIndianNumber(outwardTotal, 2, unit);
        row["closing_qty_fmt"] = AccountingEngine::formatIndianNumber(closingQty, 2, unit);
        row["opening_bags_fmt"] = AccountingEngine::formatIndianNumber(opBags, 0);
        row["inward_bags_fmt"] = AccountingEngine::formatIndianNumber(inwardBagsTotal, 0);
        row["outward_bags_fmt"] = AccountingEngine::formatIndianNumber(outwardBagsTotal, 0);
        row["closing_bags_fmt"] = AccountingEngine::formatIndianNumber(closingBags, 0);
        row["rate_fmt"] = AccountingEngine::formatIndianCurrency(rate);
        row["closing_value_fmt"] = AccountingEngine::formatIndianCurrency(closingVal);

        result.append(row);
    }

    return result;
}

QVariantList StockItemsModel::get_item_movements(const QString& itemName, const QString& param1, const QString& param2) {
    QVariantList movements;
    QString fromDate = "";
    QString toDate = "";

    if (!param1.isEmpty() && !param2.isEmpty()) {
        fromDate = param1;
        toDate = param2;
    } else if (!param1.isEmpty() && param1 != "All") {
        QVariantList fyRow = DatabaseManager::instance().executeQuery("SELECT start_date, end_date FROM financial_years WHERE year_name = ? LIMIT 1;", {param1});
        if (!fyRow.isEmpty()) {
            fromDate = fyRow.first().toMap().value("start_date").toString();
            toDate = fyRow.first().toMap().value("end_date").toString();
        } else {
            fromDate = AccountingEngine::getActiveFromDate();
            toDate = AccountingEngine::getActiveToDate();
        }
    } else {
        fromDate = AccountingEngine::getActiveFromDate();
        toDate = AccountingEngine::getActiveToDate();
    }
    if (fromDate.isEmpty() && toDate.isEmpty()) {
        fromDate = m_currentFromDate;
        toDate = m_currentToDate;
    }

    // 1. Calculate Opening Balance prior to fromDate
    if (!fromDate.isEmpty()) {
        double opWt = 0.0;
        long long opBags = 0;
        double opRate = 0.0;
        double opVal = 0.0;

        QVariantList customRows = DatabaseManager::instance().executeQuery(
            "SELECT closing_date, bags, weight_qtl, rate, amount FROM custom_closing_stocks WHERE item_name = ? AND closing_date < ? ORDER BY closing_date DESC LIMIT 1;",
            {itemName, fromDate}
        );
        if (!customRows.isEmpty()) {
            QVariantMap c = customRows.first().toMap();
            QString cDate = c.value("closing_date").toString();
            opBags = c.value("bags").toLongLong();
            opWt = c.value("weight_qtl").toDouble();
            opRate = c.value("rate").toDouble();
            opVal = c.value("amount").toDouble();

            QVariantList rInt = DatabaseManager::instance().executeQuery(
                "SELECT "
                "SUM(CASE WHEN trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M') THEN weight_qtl ELSE 0 END) AS in_wt, "
                "SUM(CASE WHEN trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M') THEN bags ELSE 0 END) AS in_bg, "
                "SUM(CASE WHEN trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M') THEN amount ELSE 0 END) AS in_amt, "
                "SUM(CASE WHEN trans_type IN ('Sale', 'PrRn', 'Outward', 'S') THEN weight_qtl ELSE 0 END) AS out_wt, "
                "SUM(CASE WHEN trans_type IN ('Sale', 'PrRn', 'Outward', 'S') THEN bags ELSE 0 END) AS out_bg, "
                "SUM(CASE WHEN trans_type IN ('Sale', 'PrRn', 'Outward', 'S') THEN amount ELSE 0 END) AS out_amt "
                "FROM stock_transactions "
                "WHERE item_name = ? AND voucher_date > ? AND voucher_date < ?;",
                {itemName, cDate, fromDate}
            );
            if (!rInt.isEmpty()) {
                QVariantMap ri = rInt.first().toMap();
                opWt += (ri.value("in_wt").toDouble() - ri.value("out_wt").toDouble());
                opBags += (ri.value("in_bg").toLongLong() - ri.value("out_bg").toLongLong());
                opVal += (ri.value("in_amt").toDouble() - ri.value("out_amt").toDouble());
            }
        } else {
            QVariantList itmRows = DatabaseManager::instance().executeQuery(
                "SELECT opening_qty, opening_bags, opening_rate, opening_value FROM stock_items WHERE name = ? LIMIT 1;",
                {itemName}
            );
            if (!itmRows.isEmpty()) {
                QVariantMap itm = itmRows.first().toMap();
                opWt = itm.value("opening_qty").toDouble();
                opBags = itm.value("opening_bags").toLongLong();
                opRate = itm.value("opening_rate").toDouble();
                opVal = itm.value("opening_value").toDouble();
            }
            QVariantList rPrior = DatabaseManager::instance().executeQuery(
                "SELECT "
                "SUM(CASE WHEN trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M') THEN weight_qtl ELSE 0 END) AS in_wt, "
                "SUM(CASE WHEN trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M') THEN bags ELSE 0 END) AS in_bg, "
                "SUM(CASE WHEN trans_type IN ('Purc', 'SlRn', 'Inward', 'P', 'M') THEN amount ELSE 0 END) AS in_amt, "
                "SUM(CASE WHEN trans_type IN ('Sale', 'PrRn', 'Outward', 'S') THEN weight_qtl ELSE 0 END) AS out_wt, "
                "SUM(CASE WHEN trans_type IN ('Sale', 'PrRn', 'Outward', 'S') THEN bags ELSE 0 END) AS out_bg, "
                "SUM(CASE WHEN trans_type IN ('Sale', 'PrRn', 'Outward', 'S') THEN amount ELSE 0 END) AS out_amt "
                "FROM stock_transactions "
                "WHERE item_name = ? AND voucher_date < ?;",
                {itemName, fromDate}
            );
            if (!rPrior.isEmpty()) {
                QVariantMap rp = rPrior.first().toMap();
                opWt += (rp.value("in_wt").toDouble() - rp.value("out_wt").toDouble());
                opBags += (rp.value("in_bg").toLongLong() - rp.value("out_bg").toLongLong());
                opVal += (rp.value("in_amt").toDouble() - rp.value("out_amt").toDouble());
            }
        }

        if (std::abs(opWt) > 0.001 || std::abs(opBags) > 0) {
            QVariantMap opItem;
            opItem["isInward"] = (opWt >= 0);
            opItem["vDate"] = fromDate;
            opItem["refNo"] = "OP-BAL";
            opItem["type"] = "Opening Balance (B/F)";
            opItem["party"] = "Opening Stock (B/F)";
            opItem["bags"] = std::abs(opBags);
            opItem["qty"] = std::abs(opWt);
            opItem["rate"] = opRate;
            opItem["amount"] = std::abs(opVal);
            opItem["financial_year"] = "Opening";
            movements.append(opItem);
        }
    }

    // 2. Milling movements strictly for this item from milling_voucher_items
    QString mviSql = "SELECT mvi.batch_no, mvi.batch_date, mvi.drcr, mvi.weight_qtl, mb.financial_year "
                     "FROM milling_voucher_items mvi "
                     "LEFT JOIN milling_batches mb ON mb.id = mvi.batch_id "
                     "WHERE (mvi.item_name = ? OR mvi.item_code = (SELECT code FROM stock_items WHERE name = ? LIMIT 1))";
    QVariantList mviParams = {itemName, itemName};
    if (!fromDate.isEmpty() && !toDate.isEmpty()) {
        mviSql += " AND mvi.batch_date >= ? AND mvi.batch_date <= ?";
        mviParams << fromDate << toDate;
    }
    mviSql += " ORDER BY mvi.batch_date ASC;";
    QVariantList mviRows = DatabaseManager::instance().executeQuery(mviSql, mviParams);
    for (const QVariant& r : mviRows) {
        QVariantMap mv = r.toMap();
        bool isInward = (mv.value("drcr").toString() == "Dr");
        QVariantMap m;
        m["isInward"] = isInward;
        m["vDate"] = mv.value("batch_date").toString();
        m["refNo"] = "-";
        m["type"] = isInward ? "Milling Inwards" : "Milling Outwards";
        m["party"] = isInward ? "Milling Inwards" : "Milling Outwards";
        m["bags"] = 0; // Loose production / consumption
        m["qty"] = mv.value("weight_qtl").toDouble();
        m["rate"] = 0.0;
        m["amount"] = 0.0;
        m["financial_year"] = mv.value("financial_year").toString();
        movements.append(m);
    }

    // 3. Inwards & Outwards from stock_transactions
    QString stSql = "SELECT voucher_date, COALESCE(bill_no, voucher_no, '') AS ref_no, trans_type, party_name, bags, weight_qtl, rate, amount, financial_year FROM stock_transactions WHERE (item_name = ? OR item_id = (SELECT id FROM stock_items WHERE name = ? LIMIT 1))";
    QVariantList stParams = {itemName, itemName};
    if (!fromDate.isEmpty() && !toDate.isEmpty()) {
        stSql += " AND voucher_date >= ? AND voucher_date <= ?";
        stParams << fromDate << toDate;
    }
    stSql += " ORDER BY voucher_date ASC, id ASC;";
    QVariantList stRows = DatabaseManager::instance().executeQuery(stSql, stParams);
    for (const QVariant& r : stRows) {
        QVariantMap st = r.toMap();
        QString tType = st.value("trans_type").toString();
        bool isInward = (tType == "Purc" || tType == "Inward" || tType == "P" || tType == "M" || tType == "SlRn");

        QVariantMap m;
        m["isInward"] = isInward;
        m["vDate"] = st.value("voucher_date").toString();
        m["refNo"] = st.value("ref_no").toString();
        m["type"] = isInward ? QString("Purchase / Inward (%1)").arg(tType) : QString("Sale / Outward (%1)").arg(tType);
        m["party"] = st.value("party_name").toString();
        m["bags"] = st.value("bags").toInt();
        m["qty"] = st.value("weight_qtl").toDouble();
        m["rate"] = st.value("rate").toDouble();
        m["amount"] = st.value("amount").toDouble();
        m["financial_year"] = st.value("financial_year").toString();
        movements.append(m);
    }

    // Sort movements chronologically with OP-BAL always first
    std::sort(movements.begin(), movements.end(), [](const QVariant& a, const QVariant& b) {
        QVariantMap ma = a.toMap();
        QVariantMap mb = b.toMap();
        if (ma.value("refNo").toString() == "OP-BAL") return true;
        if (mb.value("refNo").toString() == "OP-BAL") return false;
        return ma.value("vDate").toString() < mb.value("vDate").toString();
    });

    return movements;
}

QVariantMap StockItemsModel::get_item_movement(const QString& itemName, const QString& fy) {
    QVariantMap result;
    QString targetFy = !fy.isEmpty() ? fy : m_currentFinancialYear;

    QVariantMap item = get_item_by_name(itemName);
    double opBags = item.value("opening_bags").toDouble();
    double opWeight = item.value("opening_qty").toDouble();
    double opValue = item.value("opening_value").toDouble();

    QVariantList inRows = DatabaseManager::instance().executeQuery(
        "SELECT invoice_no AS ref_no, invoice_date AS vch_date, supplier_name AS party_name, bag_count AS bags, weight_qtl AS weight, taxable_amount AS amount "
        "FROM purchase_invoices WHERE item_name = ? AND financial_year = ? ORDER BY invoice_date ASC, id ASC;",
        {itemName, targetFy}
    );

    QVariantList outRows = DatabaseManager::instance().executeQuery(
        "SELECT invoice_no AS ref_no, invoice_date AS vch_date, customer_name AS party_name, bag_count AS bags, weight_qtl AS weight, taxable_amount AS amount "
        "FROM sales_invoices WHERE item_name = ? AND financial_year = ? ORDER BY invoice_date ASC, id ASC;",
        {itemName, targetFy}
    );

    double inBags = 0, inWeight = 0, inValue = 0;
    for (const QVariant& r : inRows) {
        QVariantMap row = r.toMap();
        inBags += row.value("bags").toDouble();
        inWeight += row.value("weight").toDouble();
        inValue += row.value("amount").toDouble();
    }

    double outBags = 0, outWeight = 0, outValue = 0;
    for (const QVariant& r : outRows) {
        QVariantMap row = r.toMap();
        outBags += row.value("bags").toDouble();
        outWeight += row.value("weight").toDouble();
        outValue += row.value("amount").toDouble();
    }

    double clBags = opBags + inBags - outBags;
    double clWeight = opWeight + inWeight - outWeight;

    result["item_name"] = itemName;
    result["financial_year"] = targetFy;
    result["opening_bags"] = opBags;
    result["opening_weight"] = opWeight;
    result["opening_value"] = opValue;
    result["inward_bags"] = inBags;
    result["inward_weight"] = inWeight;
    result["inward_value"] = inValue;
    result["outward_bags"] = outBags;
    result["outward_weight"] = outWeight;
    result["outward_value"] = outValue;
    result["closing_bags"] = clBags;
    result["closing_weight"] = clWeight;
    result["inward_rows"] = inRows;
    result["outward_rows"] = outRows;
    return result;
}
