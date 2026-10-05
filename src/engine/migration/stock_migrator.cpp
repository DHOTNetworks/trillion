#include "stock_migrator.h"
#include "../database_manager.h"
#include "migration_utils.h"
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

bool StockMigrator::migrate_stocks_bahikhata(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& stockGroupRows,
    const std::vector<std::map<std::string, std::string>>& stockUnitRows,
    const std::vector<std::map<std::string, std::string>>& itemRows,
    std::map<int, std::string>& outStockGroupMap,
    std::map<int, std::string>& outStockUnitMap,
    std::map<int, std::string>& outItemCodeToName,
    std::map<int, qint64>& outItemCodeToId
) {
    auto& db = DatabaseManager::instance();
    ctx.updateProgress(60, QString("Migrating %1 Stock Items from Bahi-Khata...").arg(itemRows.size()));

    outStockGroupMap.clear();
    outStockUnitMap.clear();
    outItemCodeToName.clear();
    outItemCodeToId.clear();

    // 1. Stock Groups
    for (const auto& sg : stockGroupRows) {
        int code = parseInteger(getFieldStr(sg, "Code1st"));
        std::string name = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(sg, "GroupName"))).toStdString();
        if (name.empty()) continue;
        int qtyNotShow = parseInteger(getFieldStr(sg, "QtyNotShowInTrading"));
        double clRate = parseDoubleVal(getFieldStr(sg, "ClStockRate"));
        db.executeNonQuery(
            "INSERT INTO stock_groups (group_name, legacy_code, qty_not_show_in_trading, cl_stock_rate) "
            "VALUES (?, ?, ?, ?);",
            {QString::fromStdString(name), code, qtyNotShow, clRate}
        );
        outStockGroupMap[code] = name;
    }

    // 2. Stock Units
    for (const auto& su : stockUnitRows) {
        int code = parseInteger(getFieldStr(su, "Code1st"));
        std::string name = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(su, "UnitName"))).toStdString();
        if (name.empty()) continue;
        int decimals = parseInteger(getFieldStr(su, "DecimalPlaces", "2"), 2);
        db.executeNonQuery(
            "INSERT INTO stock_units (unit_name, legacy_code, decimal_places) "
            "VALUES (?, ?, ?);",
            {QString::fromStdString(name), code, decimals}
        );
        outStockUnitMap[code] = name;
    }

    // 3. Stock Items
    for (const auto& it : itemRows) {
        std::string iName = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(it, "ItemName"))).toStdString();
        if (iName.empty()) continue;

        int legacyCode = parseInteger(getFieldStr(it, "Code1st"));
        int gCode = parseInteger(getFieldStr(it, "GroupCode"));
        std::string groupName = outStockGroupMap.count(gCode) ? outStockGroupMap[gCode] : "General";

        int uCode = parseInteger(getFieldStr(it, "UnitCode", "1"));
        std::string unitName = outStockUnitMap.count(uCode) ? outStockUnitMap[uCode] : "QTL";

        double opQty = parseDoubleVal(getFieldStr(it, "OpeningQty"));
        double opRate = parseDoubleVal(getFieldStr(it, "OpeningRate"));
        double opVal = parseDoubleVal(getFieldStr(it, "OpeningValue", getFieldStr(it, "OpeningVal")));
        if (opVal == 0.0 && opQty > 0 && opRate > 0) {
            opVal = opQty * opRate;
        }

        std::string hsn = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(it, "HSNCode", getFieldStr(it, "TariffCode")))).toStdString();
        double gstRate = parseDoubleVal(getFieldStr(it, "GSTRate", getFieldStr(it, "TaxRate")));

        db.executeNonQuery(
            "INSERT INTO stock_items ("
            "name, stock_group, unit, opening_quantity, opening_rate, opening_value, "
            "hsn_code, gst_rate_percent, legacy_code, is_active"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, 1);",
            {
                QString::fromStdString(iName),
                QString::fromStdString(groupName),
                QString::fromStdString(unitName),
                opQty,
                opRate,
                opVal,
                QString::fromStdString(hsn),
                gstRate,
                legacyCode
            }
        );
        qint64 itemId = db.lastInsertedId();
        outItemCodeToName[legacyCode] = iName;
        outItemCodeToId[legacyCode] = itemId;

        // Inserter for initial inventory ledger row if opening quantity exists
        if (opQty != 0.0) {
            db.executeNonQuery(
                "INSERT INTO inventory (item_name, item_id, unit, opening_qty, current_qty, valuation_rate) "
                "VALUES (?, ?, ?, ?, ?, ?);",
                {
                    QString::fromStdString(iName),
                    itemId,
                    QString::fromStdString(unitName),
                    opQty,
                    opQty,
                    opRate
                }
            );
        }
    }

    ctx.stats.totalStockUnits = static_cast<int>(outStockUnitMap.size());
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
