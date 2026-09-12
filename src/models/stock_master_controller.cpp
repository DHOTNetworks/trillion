#include "stock_master_controller.h"
#include "stock_items_model.h"
#include "../services/financial_math_service.h"
#include <cmath>

StockMasterController::StockMasterController(QObject *parent)
    : QObject(parent) {
}

void StockMasterController::setItemId(int id) {
    if (m_itemId != id) {
        m_itemId = id;
        emit itemIdChanged();
        emit isEditModeChanged();
        if (id > 0) {
            loadItem(id);
        }
    }
}

void StockMasterController::setOpeningBags(int v) {
    if (m_openingBags != v) {
        m_openingBags = v;
        if (m_openingQty <= 0.0001 && v > 0) {
            double pQtl = m_packingKg > 2.0 ? m_packingKg / 100.0 : (m_packingKg > 0 ? m_packingKg : 0.500);
            m_openingQty = std::round((v * pQtl) * 1000.0) / 1000.0;
        }
        recalculateOpeningValue();
    }
}

void StockMasterController::setOpeningQty(double v) {
    if (std::abs(m_openingQty - v) > 0.001) {
        m_openingQty = v;
        recalculateOpeningValue();
    }
}

void StockMasterController::setOpeningRate(double v) {
    if (std::abs(m_openingRate - v) > 0.001) {
        m_openingRate = v;
        recalculateOpeningValue();
    }
}

QString StockMasterController::openingValueFmt() const {
    return FinancialMathService::instance().formatInr(m_openingValue);
}

void StockMasterController::recalculateOpeningValue() {
    m_openingValue = FinancialMathService::instance().round2(m_openingQty * m_openingRate);
    emit openingStockChanged();
}

void StockMasterController::resetForm() {
    m_itemId = 0;
    emit itemIdChanged();
    emit isEditModeChanged();

    m_name.clear(); emit nameChanged();
    m_code.clear(); emit codeChanged();
    m_itemType = "Paddy"; emit itemTypeChanged();
    m_goodsType = "Goods"; emit goodsTypeChanged();
    m_companyName.clear(); emit companyNameChanged();
    m_stockGroup = "Raw Material"; emit stockGroupChanged();
    m_unit = "QTL"; emit unitChanged();

    m_purchaseRate = 0.0; emit purchaseRateChanged();
    m_saleRate = 0.0; emit saleRateChanged();
    m_mrp = 0.0; emit mrpChanged();
    m_discount = 0.0; emit discountChanged();
    m_hsnCode = "1006"; emit hsnCodeChanged();
    m_gstRate = 5.0; emit gstRateChanged();
    m_cessRate = 0.0; emit cessRateChanged();

    m_packingKg = 50.0; emit packingKgChanged();

    m_openingBags = 0;
    m_openingQty = 0.0;
    m_openingRate = 0.0;
    m_openingValue = 0.0;
    emit openingStockChanged();

    m_purchaseLedger = "Purchase Account"; emit purchaseLedgerChanged();
    m_saleLedger = "Sales Account"; emit saleLedgerChanged();
    m_stockLedger = "Stock In Hand"; emit stockLedgerChanged();

    m_statusMessage.clear();
    m_isError = false;
    emit statusChanged();
}

bool StockMasterController::loadItem(int id) {
    if (id <= 0) return false;

    StockItemsModel sModel;
    auto item = sModel.get_item_by_id(id);
    if (item.isEmpty()) return false;

    m_itemId = id;
    emit itemIdChanged();
    emit isEditModeChanged();

    m_name = item.value("name").toString(); emit nameChanged();
    m_code = item.value("code").toString(); emit codeChanged();
    m_itemType = item.value("item_type", "Paddy").toString(); emit itemTypeChanged();
    m_goodsType = item.value("goods_type", "Goods").toString(); emit goodsTypeChanged();
    m_companyName = item.value("company_name").toString(); emit companyNameChanged();
    m_stockGroup = item.value("stock_group", "Raw Material").toString(); emit stockGroupChanged();
    m_unit = item.value("unit", "QTL").toString(); emit unitChanged();

    m_purchaseRate = item.value("purchase_rate", 0.0).toDouble(); emit purchaseRateChanged();
    m_saleRate = item.value("sale_rate", 0.0).toDouble(); emit saleRateChanged();
    m_mrp = item.value("mrp", 0.0).toDouble(); emit mrpChanged();
    m_discount = item.value("discount", 0.0).toDouble(); emit discountChanged();
    m_hsnCode = item.value("hsn_code", "1006").toString(); emit hsnCodeChanged();
    m_gstRate = item.value("gst_rate", 5.0).toDouble(); emit gstRateChanged();
    m_cessRate = item.value("cess_rate", 0.0).toDouble(); emit cessRateChanged();

    m_packingKg = item.value("packing_kg", 50.0).toDouble(); emit packingKgChanged();

    m_openingBags = item.value("opening_bags", 0).toInt();
    m_openingQty = item.value("opening_qty", 0.0).toDouble();
    m_openingRate = item.value("opening_rate", 0.0).toDouble();
    m_openingValue = item.value("opening_value", 0.0).toDouble();
    emit openingStockChanged();

    m_purchaseLedger = item.value("purchase_ledger", "Purchase Account").toString(); emit purchaseLedgerChanged();
    m_saleLedger = item.value("sale_ledger", "Sales Account").toString(); emit saleLedgerChanged();
    m_stockLedger = item.value("stock_ledger", "Stock In Hand").toString(); emit stockLedgerChanged();

    m_statusMessage.clear();
    m_isError = false;
    emit statusChanged();
    return true;
}

bool StockMasterController::loadItemByName(const QString &name) {
    if (name.trimmed().isEmpty()) return false;
    StockItemsModel sModel;
    auto item = sModel.get_item_by_name(name);
    if (!item.isEmpty() && item.contains("id")) {
        return loadItem(item.value("id").toInt());
    }
    return false;
}

bool StockMasterController::saveItem() {
    m_statusMessage.clear();
    m_isError = false;

    if (m_name.trimmed().isEmpty()) {
        m_statusMessage = "Please enter a valid Stock Item Name.";
        m_isError = true;
        emit statusChanged();
        return false;
    }

    QVariantMap data;
    data["name"] = m_name.trimmed();
    data["code"] = m_code.trimmed();
    data["item_type"] = m_itemType.trimmed();
    data["goods_type"] = m_goodsType.trimmed();
    data["company_name"] = m_companyName.trimmed();
    data["stock_group"] = m_stockGroup.trimmed();
    data["unit"] = m_unit.trimmed();
    data["purchase_rate"] = m_purchaseRate;
    data["sale_rate"] = m_saleRate;
    data["mrp"] = m_mrp;
    data["discount"] = m_discount;
    data["hsn_code"] = m_hsnCode.trimmed();
    data["gst_rate"] = m_gstRate;
    data["cess_rate"] = m_cessRate;
    data["packing_kg"] = m_packingKg;
    data["opening_bags"] = m_openingBags;
    data["opening_qty"] = m_openingQty;
    data["opening_rate"] = m_openingRate;
    data["opening_value"] = m_openingValue;
    data["purchase_ledger"] = m_purchaseLedger.trimmed();
    data["sale_ledger"] = m_saleLedger.trimmed();
    data["stock_ledger"] = m_stockLedger.trimmed();

    StockItemsModel sModel;
    bool ok = false;
    if (m_itemId > 0) {
        ok = sModel.update_stock_item_full(m_itemId, data);
        if (ok) {
            m_statusMessage = QString("Stock Item '%1' updated successfully!").arg(m_name.trimmed());
        }
    } else {
        ok = sModel.save_stock_item_full(data);
        if (ok) {
            m_statusMessage = QString("Stock Item '%1' created successfully!").arg(m_name.trimmed());
        }
    }

    if (ok) {
        m_isError = false;
        emit statusChanged();
        emit itemSaved();
        return true;
    } else {
        m_statusMessage = "Failed to save Stock Item in database.";
        m_isError = true;
        emit statusChanged();
        return false;
    }
}

bool StockMasterController::deleteItem(int id) {
    if (id <= 0) return false;
    StockItemsModel sModel;
    bool ok = sModel.delete_stock_item(id);
    if (ok) {
        m_statusMessage = "Stock Item deleted successfully.";
        m_isError = false;
        emit statusChanged();
        emit itemDeleted();
        resetForm();
        return true;
    } else {
        m_statusMessage = "Failed to delete Stock Item.";
        m_isError = true;
        emit statusChanged();
        return false;
    }
}

bool StockMasterController::addStockGroup(const QString &groupName) {
    if (groupName.trimmed().isEmpty()) return false;
    StockItemsModel sModel;
    return sModel.add_stock_group(groupName.trimmed());
}
