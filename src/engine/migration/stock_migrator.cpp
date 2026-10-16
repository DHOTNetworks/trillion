#include "stock_migrator.h"
#include "../database_manager.h"
#include "../data_preprocessor.h"
#include "migration_utils.h"
#include "migration_columns.h"
#include <QDebug>
#include <cmath>

namespace MahadevERP {

static std::string getFieldStr(const std::map<std::string, std::string>& m, const std::string& key, const std::string& def = "") {
    auto it = m.find(key);
    if (it != m.end()) return it->second;
    for (const auto& kv : m) {
        if (QString::compare(QString::fromStdString(kv.first), QString::fromStdString(key), Qt::CaseInsensitive) == 0) {
            return kv.second;
        }
    }
    return def;
}

static double parseDoubleVal(const std::string& s, double def = 0.0) {
    std::string clean;
    for (char c : s) {
        if (c != ',' && c != '%' && c != ' ') clean += c;
    }
    if (clean.empty()) return def;
    try {
        return std::stod(clean);
    } catch (...) {
        return def;
    }
}

static int parseInteger(const std::string& s, int def = 0) {
    std::string clean;
    for (char c : s) {
        if (c != ',' && c != ' ') clean += c;
    }
    if (clean.empty()) return def;
    try {
        return std::stoi(clean);
    } catch (...) {
        return def;
    }
}

bool StockMigrator::migrate_stock_masters_bahikhata(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& stockGroupRows,
    const std::vector<std::map<std::string, std::string>>& stockUnitRows,
    std::map<int, std::string>& outStockGroupMap,
    std::map<int, std::string>& outStockUnitMap
) {
    // C3 strangler slice S (masters half): transplanted verbatim from the
    // retired monolith PASS 1.5. Field keys via MigrationColumns = JSON-driven.
    auto& db = DatabaseManager::instance();
    auto GF = [](const char* t) { return MigrationColumns::stockGroupField(QString::fromLatin1(t)).toStdString(); };
    auto UF = [](const char* t) { return MigrationColumns::stockUnitField(QString::fromLatin1(t)).toStdString(); };
    outStockGroupMap.clear();
    outStockUnitMap.clear();
    for (const auto& sg : stockGroupRows) {
        int code = DataPreprocessor::parseInt(getFieldStr(sg, GF("legacy_code")));
        std::string name = DataPreprocessor::cleanText(getFieldStr(sg, GF("group_name")));
        if (name.empty()) continue;
        int qtyNotShow = DataPreprocessor::parseInt(getFieldStr(sg, GF("qty_not_show_in_trading")));
        double clRate = DataPreprocessor::parseDouble(getFieldStr(sg, GF("cl_stock_rate")));
        db.executeNonQuery(
            "INSERT INTO stock_groups (group_name, legacy_code, qty_not_show_in_trading, cl_stock_rate) "
            "VALUES (?, ?, ?, ?);",
            {QString::fromStdString(name), code, qtyNotShow, clRate}
        );
        outStockGroupMap[code] = name;
    }

    for (const auto& su : stockUnitRows) {
        int code = DataPreprocessor::parseInt(getFieldStr(su, UF("legacy_code")));
        std::string name = DataPreprocessor::cleanText(getFieldStr(su, UF("unit_name")));
        if (name.empty()) continue;
        int decimals = DataPreprocessor::parseInt(getFieldStr(su, UF("decimal_places"), "2"), 2);
        db.executeNonQuery(
            "INSERT INTO stock_units (unit_name, legacy_code, decimal_places) "
            "VALUES (?, ?, ?);",
            {QString::fromStdString(name), code, decimals}
        );
        outStockUnitMap[code] = name;
    }

    ctx.stats.totalStockUnits = static_cast<int>(outStockUnitMap.size());
    return true;
}

bool StockMigrator::migrate_stock_items_bahikhata(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& itemRows,
    const std::map<int, std::string>& ledgerCodeMap,
    const std::map<int, int>& legacyIdToPartyId,
    const std::map<int, std::string>& inStockGroupMap,
    const std::map<int, std::string>& inStockUnitMap,
    std::map<int, ItemDetail>& outItemCodeMap,
    std::map<int, std::string>& outItemCodeToName,
    std::map<int, qint64>& outItemCodeToId
) {
    // C3 strangler slice S (items half): transplanted verbatim from the retired
    // monolith PASS 3 (stock items + inventory). Field keys via MigrationColumns.
    // Helpers: DataPreprocessor mirrors (verbatim-identical bodies).
    // Data-driven (user directive): no hardcoded 50 kg packing / "Packing" unit /
    // 5% gst fallbacks — absent source values migrate as 0/empty. Parity-safe:
    // live fixtures carry every one of these fields on all rows (verified).
    auto& db = DatabaseManager::instance();
    auto SF = [](const char* t) { return MigrationColumns::stockField(QString::fromLatin1(t)).toStdString(); };
    outItemCodeMap.clear();
    outItemCodeToName.clear();
    outItemCodeToId.clear();
    ctx.updateProgress(40, QString("Migrating %1 Stock Items & Inventory...").arg(itemRows.size()));
    db.executeNonQuery("DELETE FROM stock_items;");
    db.executeNonQuery("DELETE FROM inventory;");

    for (const auto& item : itemRows) {
        std::string iName = DataPreprocessor::cleanText(getFieldStr(item, SF("name")));
        if (iName.empty()) continue;

        int legacyCode = DataPreprocessor::parseInt(getFieldStr(item, SF("legacy_code")));
        std::string code = DataPreprocessor::cleanText(getFieldStr(item, SF("legacy_code")));

        // 1. Item Types
        std::string rawItemType = DataPreprocessor::cleanText(getFieldStr(item, SF("item_type"), "Both"));
        std::string itemType = "Both";
        if (DataPreprocessor::toLowerStr(rawItemType).find("mandi") != std::string::npos) itemType = "Mandi";
        else if (DataPreprocessor::toLowerStr(rawItemType).find("market") != std::string::npos) itemType = "Market";

        std::string goodsType = DataPreprocessor::cleanText(getFieldStr(item, SF("goods_type"), "Goods"));
        if (goodsType.empty()) goodsType = "Goods";

        // 2. Trading Group & Unit
        int groupCode = DataPreprocessor::parseInt(getFieldStr(item, SF("group_code")));
        std::string tradingGroup = "";
        auto itGrp = inStockGroupMap.find(groupCode);
        if (itGrp != inStockGroupMap.end()) tradingGroup = itGrp->second;
        else tradingGroup = "Primary";

        int unitCode = DataPreprocessor::parseInt(getFieldStr(item, SF("unit_code")));
        std::string unit = "Qtl.";
        auto itUnit = inStockUnitMap.find(unitCode);
        if (itUnit != inStockUnitMap.end()) unit = itUnit->second;

        std::string rateCalcOn = DataPreprocessor::cleanText(getFieldStr(item, SF("rate_calc_on"), "N/A"));
        if (rateCalcOn.empty()) rateCalcOn = "N/A";

        int autoAdjustName = DataPreprocessor::parseInt(getFieldStr(item, SF("auto_adjust_name"), "1"), 1);
        int calculateStock = DataPreprocessor::parseInt(getFieldStr(item, SF("calculate_stock"), "1"), 1);
        int calculateInTrading = DataPreprocessor::parseInt(getFieldStr(item, SF("include_in_trading"), "1"), 1);
        std::string itemNarration = DataPreprocessor::cleanText(getFieldStr(item, SF("item_narration")));
        int capitalGoods = DataPreprocessor::parseInt(getFieldStr(item, SF("capital_goods"), "0"), 0);

        // 3. Taxes: VAT, CST, GST, HSN
        double vatRate = DataPreprocessor::parseDouble(getFieldStr(item, SF("vat_rate")));
        int vatLedgerCode = DataPreprocessor::parseInt(getFieldStr(item, SF("vat_ledger_id")));
        std::string vatLedger = "VAT A/c";
        { auto it = ledgerCodeMap.find(vatLedgerCode); if (it != ledgerCodeMap.end()) vatLedger = it->second; }

        double surchargeOnVat = DataPreprocessor::parseDouble(getFieldStr(item, SF("surcharge_on_vat")));
        double vatAgainstD1 = DataPreprocessor::parseDouble(getFieldStr(item, SF("vat_against_d1")));

        double cstRate = DataPreprocessor::parseDouble(getFieldStr(item, SF("cst_rate")));
        int cstLedgerCode = DataPreprocessor::parseInt(getFieldStr(item, SF("cst_ledger_id")));
        std::string cstLedger = "CST A/c";
        { auto it = ledgerCodeMap.find(cstLedgerCode); if (it != ledgerCodeMap.end()) cstLedger = it->second; }

        double cstWithoutCForm = DataPreprocessor::parseDouble(getFieldStr(item, SF("cst_without_cform")));

        std::string hsn = DataPreprocessor::cleanText(getFieldStr(item, SF("hsn_code"), ""));

        std::string gstSlab = DataPreprocessor::cleanText(getFieldStr(item, SF("gst_rate")));
        std::string gstSlabLower = DataPreprocessor::toLowerStr(gstSlab);
        double gstRate = 0.0;
        if (gstSlabLower.find("exempt") != std::string::npos || gstSlabLower == "0" || gstSlabLower == "0%") {
            gstRate = 0.0;
        } else if (!gstSlab.empty()) {
            gstRate = DataPreprocessor::parseDouble(gstSlab);
        } else if (vatRate > 0.01) {
            gstRate = vatRate;
        }
        double cessRate = DataPreprocessor::parseDouble(getFieldStr(item, SF("cess_rate")));

        // 4. Mandi Rates & Ledgers
        double damiRate = DataPreprocessor::parseDouble(getFieldStr(item, SF("dami_rate")));
        int damiLedgerCode = DataPreprocessor::parseInt(getFieldStr(item, SF("dami_ledger_id")));
        std::string damiLedger = "Dami A/c";
        { auto it = ledgerCodeMap.find(damiLedgerCode); if (it != ledgerCodeMap.end()) damiLedger = it->second; }
        int damiLedgerId = 0;
        { auto it = legacyIdToPartyId.find(damiLedgerCode); if (it != legacyIdToPartyId.end()) damiLedgerId = it->second; }

        double marketFeeRate = DataPreprocessor::parseDouble(getFieldStr(item, SF("market_fee_rate")));
        int mFeeLedgerCode = DataPreprocessor::parseInt(getFieldStr(item, SF("market_fee_ledger_id")));
        std::string mFeeLedger = "Market Fee A/c";
        { auto it = ledgerCodeMap.find(mFeeLedgerCode); if (it != ledgerCodeMap.end()) mFeeLedger = it->second; }
        int mFeeLedgerId = 0;
        { auto it = legacyIdToPartyId.find(mFeeLedgerCode); if (it != legacyIdToPartyId.end()) mFeeLedgerId = it->second; }

        double hrdfRate = DataPreprocessor::parseDouble(getFieldStr(item, SF("hrdf_rate")));
        int hrdfLedgerCode = DataPreprocessor::parseInt(getFieldStr(item, SF("hrdf_ledger_id")));
        std::string hrdfLedger = "H.R.D.F. A/c";
        { auto it = ledgerCodeMap.find(hrdfLedgerCode); if (it != ledgerCodeMap.end()) hrdfLedger = it->second; }
        int hrdfLedgerId = 0;
        { auto it = legacyIdToPartyId.find(hrdfLedgerCode); if (it != legacyIdToPartyId.end()) hrdfLedgerId = it->second; }

        int mktCommttFormApply = DataPreprocessor::parseInt(getFieldStr(item, SF("market_commtt_form_apply")));
        int mktCommttCouponApply = DataPreprocessor::parseInt(getFieldStr(item, SF("market_commtt_coupon_apply")));
        int damiCalcOnWeight = DataPreprocessor::parseInt(getFieldStr(item, SF("dami_calc_on_weight")));
        int taxOnQty = DataPreprocessor::parseInt(getFieldStr(item, SF("tax_on_qty"), getFieldStr(item, "CessCalcOnQty")));

        // 5. Trading Posting Accounts
        int purcLedgerCode = DataPreprocessor::parseInt(getFieldStr(item, SF("purchase_ledger_id")));
        std::string purcLedger = "";
        { auto it = ledgerCodeMap.find(purcLedgerCode); if (it != ledgerCodeMap.end()) purcLedger = it->second; }
        int purcLedgerId = 0;
        { auto it = legacyIdToPartyId.find(purcLedgerCode); if (it != legacyIdToPartyId.end()) purcLedgerId = it->second; }

        int purcRetLedgerCode = DataPreprocessor::parseInt(getFieldStr(item, SF("purchase_return_ledger_id")));
        std::string purcRetLedger = purcLedger;
        { auto it = ledgerCodeMap.find(purcRetLedgerCode); if (it != ledgerCodeMap.end()) purcRetLedger = it->second; }
        int purcRetLedgerId = purcLedgerId;
        { auto it = legacyIdToPartyId.find(purcRetLedgerCode); if (it != legacyIdToPartyId.end()) purcRetLedgerId = it->second; }

        int saleLedgerCode = DataPreprocessor::parseInt(getFieldStr(item, SF("sale_ledger_id")));
        std::string saleLedger = "";
        { auto it = ledgerCodeMap.find(saleLedgerCode); if (it != ledgerCodeMap.end()) saleLedger = it->second; }
        int saleLedgerId = 0;
        { auto it = legacyIdToPartyId.find(saleLedgerCode); if (it != legacyIdToPartyId.end()) saleLedgerId = it->second; }

        int saleRetLedgerCode = DataPreprocessor::parseInt(getFieldStr(item, SF("sale_return_ledger_id")));
        std::string saleRetLedger = saleLedger;
        { auto it = ledgerCodeMap.find(saleRetLedgerCode); if (it != ledgerCodeMap.end()) saleRetLedger = it->second; }
        int saleRetLedgerId = saleLedgerId;
        { auto it = legacyIdToPartyId.find(saleRetLedgerCode); if (it != legacyIdToPartyId.end()) saleRetLedgerId = it->second; }

        int stockLedgerCode = DataPreprocessor::parseInt(getFieldStr(item, SF("stock_ledger_id")));
        std::string stockLedger = "";
        { auto it = ledgerCodeMap.find(stockLedgerCode); if (it != ledgerCodeMap.end()) stockLedger = it->second; }
        int stockLedgerId = 0;
        { auto it = legacyIdToPartyId.find(stockLedgerCode); if (it != legacyIdToPartyId.end()) stockLedgerId = it->second; }

        int vatLedgerId = 0;
        { auto it = legacyIdToPartyId.find(vatLedgerCode); if (it != legacyIdToPartyId.end()) vatLedgerId = it->second; }
        int cstLedgerId = 0;
        { auto it = legacyIdToPartyId.find(cstLedgerCode); if (it != legacyIdToPartyId.end()) cstLedgerId = it->second; }

        // 6. Pricing & Packing
        // Data-driven (no hardcoded 50 kg fallback): absent Packing migrates as 0, never a fake 50.
        double packingKg = DataPreprocessor::parseDouble(getFieldStr(item, SF("packing_kg")));
        double purRate = DataPreprocessor::parseDouble(getFieldStr(item, SF("purchase_rate")));
        double saleRate = DataPreprocessor::parseDouble(getFieldStr(item, SF("sale_rate")));
        double bonusApproved = DataPreprocessor::parseDouble(getFieldStr(item, SF("bonus_approved")));
        double mrp = DataPreprocessor::parseDouble(getFieldStr(item, SF("mrp")));
        double discount = DataPreprocessor::parseDouble(getFieldStr(item, SF("discount")));

        // 7. Opening stock
        int opBags = DataPreprocessor::parseInt(getFieldStr(item, SF("opening_bags")));
        double opQty = DataPreprocessor::parseDouble(getFieldStr(item, SF("opening_qty")));
        double opRate = DataPreprocessor::parseDouble(getFieldStr(item, SF("opening_rate")));
        if (opRate <= 0.0) opRate = purRate;
        double opVal = DataPreprocessor::parseDouble(getFieldStr(item, SF("opening_value")));
        if (opVal <= 0.0) opVal = std::round(opQty * opRate * 100.0) / 100.0;

        std::string millStr = DataPreprocessor::toLowerStr(getFieldStr(item, SF("is_manufacturing_item")));
        int isManufacturingItem = (millStr == "true" || millStr == "1" || millStr == "yes") ? 1 : 0;

        // 8. Labour Rates (3 slabs x 7 operations)
        // Data-driven (no hardcoded "Packing" unit): absent LabourRateUnit migrates as empty.
        std::string labourRateUnit = DataPreprocessor::cleanText(getFieldStr(item, SF("labour_rate_unit")));

        double utrai1 = DataPreprocessor::parseDouble(getFieldStr(item, SF("utrai_rate_1")));
        double jharai1 = DataPreprocessor::parseDouble(getFieldStr(item, SF("jharai_rate_1")));
        double bharai1 = DataPreprocessor::parseDouble(getFieldStr(item, SF("bharai_rate_1")));
        double tulai1 = DataPreprocessor::parseDouble(getFieldStr(item, SF("tulai_rate_1")));
        double khichai1 = DataPreprocessor::parseDouble(getFieldStr(item, SF("khichai_rate_1")));
        double silai1 = DataPreprocessor::parseDouble(getFieldStr(item, SF("silai_rate_1")));
        double loading1 = DataPreprocessor::parseDouble(getFieldStr(item, SF("loading_rate_1")));

        double utrai2 = DataPreprocessor::parseDouble(getFieldStr(item, SF("utrai_rate_2")));
        double jharai2 = DataPreprocessor::parseDouble(getFieldStr(item, SF("jharai_rate_2")));
        double bharai2 = DataPreprocessor::parseDouble(getFieldStr(item, SF("bharai_rate_2")));
        double tulai2 = DataPreprocessor::parseDouble(getFieldStr(item, SF("tulai_rate_2")));
        double khichai2 = DataPreprocessor::parseDouble(getFieldStr(item, SF("khichai_rate_2")));
        double silai2 = DataPreprocessor::parseDouble(getFieldStr(item, SF("silai_rate_2")));
        double loading2 = DataPreprocessor::parseDouble(getFieldStr(item, SF("loading_rate_2")));

        double utrai3 = DataPreprocessor::parseDouble(getFieldStr(item, SF("utrai_rate_3")));
        double jharai3 = DataPreprocessor::parseDouble(getFieldStr(item, SF("jharai_rate_3")));
        double bharai3 = DataPreprocessor::parseDouble(getFieldStr(item, SF("bharai_rate_3")));
        double tulai3 = DataPreprocessor::parseDouble(getFieldStr(item, SF("tulai_rate_3")));
        double khichai3 = DataPreprocessor::parseDouble(getFieldStr(item, SF("khichai_rate_3")));
        double silai3 = DataPreprocessor::parseDouble(getFieldStr(item, SF("silai_rate_3")));
        double loading3 = DataPreprocessor::parseDouble(getFieldStr(item, SF("loading_rate_3")));

        std::string companyName = DataPreprocessor::cleanText(getFieldStr(item, SF("company_name")));

        db.executeNonQuery(
            "INSERT INTO stock_items ("
            "name, code, item_type, goods_type, trading_group, group_code, company_name, category_name, unit, unit_code, "
            "rate_calc_on, auto_adjust_name, item_narration, capital_goods, hsn_code, gst_rate, cess_rate, "
            "vat_rate, vat_ledger, vat_ledger_id, surcharge_on_vat, vat_against_d1, cst_rate, cst_ledger, cst_ledger_id, cst_without_cform, "
            "dami_rate, dami_ledger, dami_ledger_id, market_fee_rate, market_fee_ledger, market_fee_ledger_id, hrdf_rate, hrdf_ledger, hrdf_ledger_id, "
            "market_commtt_form_apply, market_commtt_coupon_apply, dami_calc_on_weight, tax_on_qty, "
            "purchase_rate, sale_rate, bonus_approved, mrp, discount, packing_kg, "
            "opening_bags, opening_qty, opening_rate, opening_value, "
            "purchase_ledger, purchase_ledger_id, purchase_return_ledger, purchase_return_ledger_id, "
            "sale_ledger, sale_ledger_id, sale_return_ledger, sale_return_ledger_id, stock_ledger, stock_ledger_id, "
            "is_manufacturing_item, include_in_trading, calculate_stock, labour_rate_unit, "
            "utrai_rate_1, jharai_rate_1, bharai_rate_1, tulai_rate_1, khichai_rate_1, silai_rate_1, loading_rate_1, "
            "utrai_rate_2, jharai_rate_2, bharai_rate_2, tulai_rate_2, khichai_rate_2, silai_rate_2, loading_rate_2, "
            "utrai_rate_3, jharai_rate_3, bharai_rate_3, tulai_rate_3, khichai_rate_3, silai_rate_3, loading_rate_3, "
            "legacy_code) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                QString::fromStdString(iName),
                QString::fromStdString(code),
                QString::fromStdString(itemType),
                QString::fromStdString(goodsType),
                QString::fromStdString(tradingGroup),
                groupCode,
                QString::fromStdString(companyName),
                QString::fromStdString(tradingGroup),
                QString::fromStdString(unit),
                unitCode,
                QString::fromStdString(rateCalcOn),
                autoAdjustName,
                QString::fromStdString(itemNarration),
                capitalGoods,
                QString::fromStdString(hsn),
                gstRate,
                cessRate,
                vatRate,
                QString::fromStdString(vatLedger),
                vatLedgerId,
                surchargeOnVat,
                vatAgainstD1,
                cstRate,
                QString::fromStdString(cstLedger),
                cstLedgerId,
                cstWithoutCForm,
                damiRate,
                QString::fromStdString(damiLedger),
                damiLedgerId,
                marketFeeRate,
                QString::fromStdString(mFeeLedger),
                mFeeLedgerId,
                hrdfRate,
                QString::fromStdString(hrdfLedger),
                hrdfLedgerId,
                mktCommttFormApply,
                mktCommttCouponApply,
                damiCalcOnWeight,
                taxOnQty,
                purRate,
                saleRate,
                bonusApproved,
                mrp,
                discount,
                packingKg,
                opBags,
                opQty,
                opRate,
                opVal,
                QString::fromStdString(purcLedger),
                purcLedgerId,
                QString::fromStdString(purcRetLedger),
                purcRetLedgerId,
                QString::fromStdString(saleLedger),
                saleLedgerId,
                QString::fromStdString(saleRetLedger),
                saleRetLedgerId,
                QString::fromStdString(stockLedger),
                stockLedgerId,
                isManufacturingItem,
                calculateInTrading,
                calculateStock,
                QString::fromStdString(labourRateUnit),
                utrai1, jharai1, bharai1, tulai1, khichai1, silai1, loading1,
                utrai2, jharai2, bharai2, tulai2, khichai2, silai2, loading2,
                utrai3, jharai3, bharai3, tulai3, khichai3, silai3, loading3,
                legacyCode
            }
        );

        int itemDbId = static_cast<int>(db.lastInsertedId());
        outItemCodeMap[legacyCode] = {itemDbId, iName, hsn, gstRate, packingKg};

        db.executeNonQuery(
            "INSERT INTO inventory (item_code, item_name, category, current_stock_qtl, sale_rate, gst_rate, packing_kg) "
            "VALUES (?, ?, ?, ?, ?, ?, ?);",
            {
                QString("ITEM-%1").arg(legacyCode),
                QString::fromStdString(iName),
                QString::fromStdString(itemType),
                opQty,
                saleRate,
                QString("%1%").arg(gstRate),
                static_cast<int>(packingKg)
            }
        );
    }
    for (const auto& kv : outItemCodeMap) {
        outItemCodeToName[kv.first] = kv.second.name;
        outItemCodeToId[kv.first] = static_cast<qint64>(kv.second.id);
    }

    ctx.stats.totalStockUnits = static_cast<int>(inStockUnitMap.size());
    ctx.stats.totalStockItems = static_cast<int>(outItemCodeToId.size());
    return true;
}

bool StockMigrator::migrate_stocks_busy(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& itemRows,
    const std::vector<std::map<std::string, std::string>>& unitRows,
    const std::vector<std::map<std::string, std::string>>& groupRows,
    std::map<int, std::string>& outItemCodeToName,
    std::map<int, qint64>& outItemCodeToId
) {
    auto& db = DatabaseManager::instance();
    ctx.updateProgress(70, QString("Migrating %1 Commodities & Stock Items from Busy...").arg(itemRows.size()));

    outItemCodeToName.clear();
    outItemCodeToId.clear();

    std::map<int, std::string> unitMap;
    for (const auto& u : unitRows) {
        int code = parseInteger(getFieldStr(u, "Code"));
        std::string name = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(u, "Name", getFieldStr(u, "Symbol")))).toStdString();
        if (name.empty()) continue;
        db.executeNonQuery(
            "INSERT INTO stock_units (unit_name, legacy_code, decimal_places) "
            "VALUES (?, ?, 2);",
            {QString::fromStdString(name), code}
        );
        unitMap[code] = name;
    }

    std::map<int, std::string> grpMap;
    for (const auto& g : groupRows) {
        int code = parseInteger(getFieldStr(g, "Code"));
        std::string name = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(g, "Name"))).toStdString();
        if (name.empty()) continue;
        db.executeNonQuery(
            "INSERT INTO stock_groups (group_name, legacy_code) "
            "VALUES (?, ?);",
            {QString::fromStdString(name), code}
        );
        grpMap[code] = name;
    }

    for (const auto& i : itemRows) {
        std::string name = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(i, "Name"))).toStdString();
        if (name.empty()) continue;

        int code = parseInteger(getFieldStr(i, "Code"));
        int uCode = parseInteger(getFieldStr(i, "UnitCode", getFieldStr(i, "Unit", "1")));
        std::string unit = unitMap.count(uCode) ? unitMap[uCode] : "QTL";

        int gCode = parseInteger(getFieldStr(i, "GroupCode", "0"));
        std::string group = grpMap.count(gCode) ? grpMap[gCode] : "General";

        double opQty = parseDoubleVal(getFieldStr(i, "OpQty", getFieldStr(i, "OpeningQty")));
        double opVal = parseDoubleVal(getFieldStr(i, "OpVal", getFieldStr(i, "OpeningVal")));
        double opRate = (opQty > 0 ? opVal / opQty : 0.0);

        std::string hsn = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(i, "HSNCode", getFieldStr(i, "HSN")))).toStdString();
        double gstRate = parseDoubleVal(getFieldStr(i, "TaxRate", getFieldStr(i, "GSTRate")));

        db.executeNonQuery(
            "INSERT INTO stock_items ("
            "name, stock_group, unit, opening_quantity, opening_rate, opening_value, "
            "hsn_code, gst_rate_percent, legacy_code, is_active"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, 1);",
            {
                QString::fromStdString(name),
                QString::fromStdString(group),
                QString::fromStdString(unit),
                opQty,
                opRate,
                opVal,
                QString::fromStdString(hsn),
                gstRate,
                code
            }
        );
        qint64 itemId = db.lastInsertedId();
        outItemCodeToName[code] = name;
        outItemCodeToId[code] = itemId;

        if (opQty != 0.0) {
            db.executeNonQuery(
                "INSERT INTO inventory (item_name, item_id, unit, opening_qty, current_qty, valuation_rate) "
                "VALUES (?, ?, ?, ?, ?, ?);",
                {
                    QString::fromStdString(name),
                    itemId,
                    QString::fromStdString(unit),
                    opQty,
                    opQty,
                    opRate
                }
            );
        }
    }

    ctx.stats.totalStockUnits = static_cast<int>(unitMap.size());
    ctx.stats.totalStockItems = static_cast<int>(outItemCodeToId.size());
    return true;
}

bool StockMigrator::migrate_stocks_tally(
    MigrationContext& ctx,
    const QList<QVariantMap>& unitRecords,
    const QList<QVariantMap>& itemRecords,
    std::map<std::string, qint64>& outItemNameToId
) {
    auto& db = DatabaseManager::instance();
    ctx.updateProgress(80, QString("Writing %1 Stock Items from Tally...").arg(itemRecords.size()));

    outItemNameToId.clear();

    for (const auto& u : unitRecords) {
        QString name = MigrationUtils::cleanText(u.value("name").toString());
        if (name.isEmpty()) continue;
        int decimals = u.value("decimal_places", 2).toInt();
        db.executeNonQuery(
            "INSERT INTO stock_units (unit_name, decimal_places) "
            "VALUES (?, ?);",
            {name, decimals}
        );
    }

    for (const auto& i : itemRecords) {
        QString name = MigrationUtils::cleanText(i.value("name").toString());
        if (name.isEmpty()) continue;

        QString parent = MigrationUtils::cleanText(i.value("parent", "General").toString());
        QString unit = MigrationUtils::cleanText(i.value("base_units", "QTL").toString());
        double opQty = i.value("opening_balance", 0.0).toDouble();
        double opVal = i.value("opening_value", 0.0).toDouble();
        double opRate = (opQty > 0 ? opVal / opQty : 0.0);
        QString hsn = i.value("hsn_code").toString();
        double gstRate = i.value("gst_rate", 0.0).toDouble();

        db.executeNonQuery(
            "INSERT INTO stock_items ("
            "name, stock_group, unit, opening_quantity, opening_rate, opening_value, "
            "hsn_code, gst_rate_percent, is_active"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, 1);",
            {
                name,
                parent,
                unit,
                opQty,
                opRate,
                opVal,
                hsn,
                gstRate
            }
        );
        qint64 itemId = db.lastInsertedId();
        outItemNameToId[name.toStdString()] = itemId;

        if (opQty != 0.0) {
            db.executeNonQuery(
                "INSERT INTO inventory (item_name, item_id, unit, opening_qty, current_qty, valuation_rate) "
                "VALUES (?, ?, ?, ?, ?, ?);",
                {
                    name,
                    itemId,
                    unit,
                    opQty,
                    opQty,
                    opRate
                }
            );
        }
    }

    ctx.stats.totalStockUnits = static_cast<int>(unitRecords.size());
    ctx.stats.totalStockItems = static_cast<int>(outItemNameToId.size());
    return true;
}

} // namespace MahadevERP
