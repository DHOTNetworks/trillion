#include "stock_items_model.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"

StockItemsModel::StockItemsModel(QObject* parent)
    : BaseTableModel(
        {"Item Name", "Category", "Bags", "Weight (Qtl)", "Sale Rate", "Purc Rate", "GST", "HSN"},
        {"item_name", "category", "current_bags", "current_weight_qtl", "sale_rate", "purchase_rate", "gst_rate", "hsn_code"},
        parent
    )
{
    reload_data();
}

void StockItemsModel::reload_data() {
    beginResetModel();
    m_data = DatabaseManager::instance().executeQuery("SELECT * FROM stock_items ORDER BY item_name ASC;");
    endResetModel();
    emit dataChangedSignal();
    emit countChanged();
}

QVariantMap StockItemsModel::get_item_by_name(const QString& name) const {
    QString cleanName = name.trimmed();
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM stock_items WHERE item_name = ? LIMIT 1;",
        {cleanName}
    );
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    return QVariantMap();
}

QVariantMap StockItemsModel::get_item_movement(const QString& itemName, const QString& fy) {
    QVariantMap result;
    QString targetFy = fy;
    if (targetFy.isEmpty()) {
        QVariant fyVal = DatabaseManager::instance().executeScalar("SELECT year_name FROM financial_years WHERE is_active = 1 LIMIT 1;");
        targetFy = fyVal.isValid() ? fyVal.toString() : "FY 2026-27";
    }

    QVariantMap item = get_item_by_name(itemName);
    double opBags = item.value("opening_bags").toDouble();
    double opWeight = item.value("opening_weight_qtl").toDouble();
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

bool StockItemsModel::add_item(const QString& name, const QString& category, double opening_bags, double opening_weight, double sale_rate, double purchase_rate, const QString& gst_rate, const QString& hsn_code) {
    bool ok = DatabaseManager::instance().executeNonQuery(
        "INSERT INTO stock_items (item_name, category, opening_bags, opening_weight_qtl, current_bags, current_weight_qtl, sale_rate, purchase_rate, gst_rate, hsn_code) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        {name, category, opening_bags, opening_weight, opening_bags, opening_weight, sale_rate, purchase_rate, gst_rate, hsn_code}
    );
    if (ok) reload_data();
    return ok;
}
