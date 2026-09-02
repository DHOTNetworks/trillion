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
    }
    reload_data();
}

void StockItemsModel::set_accounting_period(const QString& fromDate, const QString& toDate, const QString& fyLabel) {
    m_currentFromDate = fromDate;
    m_currentToDate = toDate;
    m_currentFinancialYear = !fyLabel.isEmpty() ? fyLabel : QString("%1 To %2").arg(fromDate, toDate);
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
        QVariantMap m;
        m["name"] = yName;
        m["label"] = isActive ? (yName + " (Active)") : yName;
        m["startDate"] = row.value("start_date").toString();
        m["endDate"] = row.value("end_date").toString();
        m["isActive"] = isActive;
        result.append(m);
    }
    QVariantMap allOpt;
    allOpt["name"] = "All";
    allOpt["label"] = "All Financial Years";
    allOpt["startDate"] = "";
    allOpt["endDate"] = "";
    allOpt["isActive"] = false;
    result.append(allOpt);
    return result;
}

QVariantList StockItemsModel::get_stock_register(const QString& param1, const QString& param2) {
    QString fDate = m_currentFromDate;
    QString tDate = m_currentToDate;
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
        double opQty = item.value("opening_qty").toDouble();
        long long opBags = item.value("opening_bags").toLongLong();

        // 1. Opening Stock from custom_closing_stocks if present
        if (!fDate.isEmpty()) {
            QVariantList cRows = DatabaseManager::instance().executeQuery(
                "SELECT closing_date, bags, weight_qtl FROM custom_closing_stocks WHERE (item_id = ? OR item_name = ?) AND closing_date < ? ORDER BY closing_date DESC LIMIT 1;",
                {itemId, name, fDate}
            );
            if (!cRows.isEmpty()) {
                QVariantMap c = cRows.first().toMap();
                opBags = c.value("bags").toLongLong();
                opQty = c.value("weight_qtl").toDouble();
            }
        }

        // 2. Inwards (Purchases)
        double inQty = 0.0;
        long long inBags = 0;
        QVariant inVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM purchase_invoices WHERE item_name = ? AND invoice_date >= ? AND invoice_date <= ?;",
            {name, fDate, tDate}
        );
        if (inVal.isValid()) inQty += inVal.toDouble();

        QVariant inBagsVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(bag_count) FROM purchase_invoices WHERE item_name = ? AND invoice_date >= ? AND invoice_date <= ?;",
            {name, fDate, tDate}
        );
        if (inBagsVal.isValid()) inBags += inBagsVal.toLongLong();

        // 3. Outwards (Sales)
        double outQty = 0.0;
        long long outBags = 0;
        QVariant outVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(weight_qtl) FROM sales_invoices WHERE item_name = ? AND invoice_date >= ? AND invoice_date <= ?;",
            {name, fDate, tDate}
        );
        if (outVal.isValid()) outQty += outVal.toDouble();

        QVariant outBagsVal = DatabaseManager::instance().executeScalar(
            "SELECT SUM(bag_count) FROM sales_invoices WHERE item_name = ? AND invoice_date >= ? AND invoice_date <= ?;",
            {name, fDate, tDate}
        );
        if (outBagsVal.isValid()) outBags += outBagsVal.toLongLong();

        double clQty = opQty + inQty - outQty;
        long long clBags = opBags + inBags - outBags;

        QVariantMap row;
        row["id"] = itemId;
        row["name"] = name;
        row["code"] = item.value("code").toString();
        row["item_type"] = item.value("item_type").toString();
        row["unit"] = item.value("unit").toString();
        row["op_bags"] = opBags;
        row["op_qty"] = opQty;
        row["in_bags"] = inBags;
        row["in_qty"] = inQty;
        row["out_bags"] = outBags;
        row["out_qty"] = outQty;
        row["cl_bags"] = clBags;
        row["cl_qty"] = clQty;

        row["op_qty_fmt"] = AccountingEngine::formatIndianNumber(opQty, 2, "Qtl");
        row["in_qty_fmt"] = AccountingEngine::formatIndianNumber(inQty, 2, "Qtl");
        row["out_qty_fmt"] = AccountingEngine::formatIndianNumber(outQty, 2, "Qtl");
        row["cl_qty_fmt"] = AccountingEngine::formatIndianNumber(clQty, 2, "Qtl");

        result.append(row);
    }

    return result;
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
