#include "master_data_provider.h"
#include "../database_manager.h"
#include "../models/account_classifier.h"
#include "../engine/data_preprocessor.h"
#include <QMutexLocker>
#include <QDebug>

MasterDataProvider::MasterDataProvider(QObject* parent)
    : QObject(parent) {
}

MasterDataProvider& MasterDataProvider::instance() {
    static MasterDataProvider s_instance;
    return s_instance;
}

void MasterDataProvider::invalidateAllCaches() {
    QMutexLocker locker(&m_cacheMutex);
    AccountClassifier::invalidateCache();
    emit masterDataChanged();
}

void MasterDataProvider::invalidateAccountCache() {
    QMutexLocker locker(&m_cacheMutex);
    AccountClassifier::invalidateCache();
    emit masterDataChanged();
}

void MasterDataProvider::invalidateStockCache() {
    QMutexLocker locker(&m_cacheMutex);
    emit masterDataChanged();
}

// 1. Groups Data Feeding
QVariantList MasterDataProvider::getAllGroups(bool hierarchical) {
    Q_UNUSED(hierarchical);
    QString sql = "SELECT id, name, parent_group_name, nature, is_system, code1st, code2nd, code3rd, code4th "
                  "FROM account_groups ORDER BY name ASC;";
    return DatabaseManager::instance().executeQuery(sql);
}

QStringList MasterDataProvider::getGroupNames(const QString& filterNature) {
    QString sql = "SELECT name FROM account_groups";
    QVariantList params;
    if (!filterNature.trimmed().isEmpty()) {
        sql += " WHERE nature = ?";
        params << filterNature.trimmed();
    }
    sql += " ORDER BY name ASC;";

    QVariantList rows = DatabaseManager::instance().executeQuery(sql, params);
    QStringList names;
    for (const auto& r : rows) {
        names << r.toMap().value("name").toString();
    }
    return names;
}

QVariantMap MasterDataProvider::getGroupDetails(int groupId) {
    QString sql = "SELECT id, name, parent_group_name, nature, is_system, code1st, code2nd, code3rd, code4th "
                  "FROM account_groups WHERE id = ?;";
    QVariantList rows = DatabaseManager::instance().executeQuery(sql, { groupId });
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    return QVariantMap();
}

QVariantMap MasterDataProvider::getGroupDetailsByCode(int code1st) {
    QString sql = "SELECT id, name, parent_group_name, nature, is_system, code1st, code2nd, code3rd, code4th "
                  "FROM account_groups WHERE code1st = ?;";
    QVariantList rows = DatabaseManager::instance().executeQuery(sql, { code1st });
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    return QVariantMap();
}

int MasterDataProvider::getGroupIdByName(const QString& groupName) {
    QString sql = "SELECT id FROM account_groups WHERE LOWER(name) = LOWER(?);";
    QVariant val = DatabaseManager::instance().executeScalar(sql, { groupName.trimmed() });
    return val.isValid() ? val.toInt() : 0;
}

// 2. Ledgers & Parties Data Feeding
QVariantList MasterDataProvider::searchAccounts(const QString& query, const QString& filterGroup, int limit) {
    QString q = query.trimmed();
    QString sql = "SELECT p.id, p.name, p.group_name, p.group_code, p.city, p.district, p.state, "
                  "p.mobile, p.gstin, p.pan, p.bank_name, p.bank_account, p.ifsc_code, "
                  "p.opening_balance, p.balance_type, p.credit_limit, p.credit_days, p.interest_rate "
                  "FROM parties p ";
    
    QStringList conditions;
    QVariantList params;

    if (!filterGroup.trimmed().isEmpty()) {
        conditions << "LOWER(p.group_name) = LOWER(?)";
        params << filterGroup.trimmed();
    }

    if (!q.isEmpty()) {
        conditions << "(p.name LIKE ? OR p.city LIKE ? OR p.mobile LIKE ? OR p.gstin LIKE ?)";
        QString pattern = "%" + q + "%";
        params << pattern << pattern << pattern << pattern;
    }

    if (!conditions.isEmpty()) {
        sql += " WHERE " + conditions.join(" AND ");
    }

    sql += QString(" ORDER BY p.name ASC LIMIT %1;").arg(limit > 0 ? limit : 50);

    return DatabaseManager::instance().executeQuery(sql, params);
}

QVariantList MasterDataProvider::getAccountsByGroup(const QString& groupName) {
    QString sql = "SELECT id, name, group_name, group_code, city, mobile, gstin, opening_balance, balance_type "
                  "FROM parties WHERE LOWER(group_name) = LOWER(?) ORDER BY name ASC;";
    return DatabaseManager::instance().executeQuery(sql, { groupName.trimmed() });
}

QVariantList MasterDataProvider::getAccountsByGroupCodes(const QVector<int>& groupCodes) {
    if (groupCodes.isEmpty()) return QVariantList();
    QString clause = AccountClassifier::generateHierarchySqlClause(groupCodes, "g");
    QString sql = QString(
        "SELECT p.id, p.name, p.group_name, p.group_code, p.city, p.mobile, p.gstin, p.opening_balance, p.balance_type "
        "FROM parties p "
        "LEFT JOIN account_groups g ON p.group_code = g.code1st OR LOWER(p.group_name) = LOWER(g.name) "
        "WHERE %1 "
        "ORDER BY p.name ASC;"
    ).arg(clause);
    return DatabaseManager::instance().executeQuery(sql);
}

QVariantMap MasterDataProvider::getAccountDetails(int accountId) {
    QString sql = "SELECT * FROM parties WHERE id = ?;";
    QVariantList rows = DatabaseManager::instance().executeQuery(sql, { accountId });
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    return QVariantMap();
}

QVariantMap MasterDataProvider::getAccountDetailsByName(const QString& accountName) {
    QString sql = "SELECT * FROM parties WHERE LOWER(name) = LOWER(?);";
    QVariantList rows = DatabaseManager::instance().executeQuery(sql, { accountName.trimmed() });
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    return QVariantMap();
}

QStringList MasterDataProvider::getAccountNames(const QString& filterGroup) {
    QString sql = "SELECT name FROM parties";
    QVariantList params;
    if (!filterGroup.trimmed().isEmpty()) {
        sql += " WHERE LOWER(group_name) = LOWER(?)";
        params << filterGroup.trimmed();
    }
    sql += " ORDER BY name ASC;";
    QVariantList rows = DatabaseManager::instance().executeQuery(sql, params);
    QStringList names;
    for (const auto& r : rows) {
        names << r.toMap().value("name").toString();
    }
    return names;
}

// 3. Stock Items & Categories Data Feeding
QVariantList MasterDataProvider::searchStockItems(const QString& query, int limit) {
    QString q = query.trimmed();
    QString sql = "SELECT id, name, code, item_type, unit, alt_unit, conversion_factor, "
                  "hsn_code, gst_rate, purchase_rate, sale_rate, packing_kg, opening_qty, opening_bags "
                  "FROM stock_items ";
    QVariantList params;
    if (!q.isEmpty()) {
        sql += "WHERE name LIKE ? OR code LIKE ? OR hsn_code LIKE ? ";
        QString pat = "%" + q + "%";
        params << pat << pat << pat;
    }
    sql += QString("ORDER BY name ASC LIMIT %1;").arg(limit > 0 ? limit : 50);
    return DatabaseManager::instance().executeQuery(sql, params);
}

QVariantList MasterDataProvider::getAllStockItems() {
    QString sql = "SELECT * FROM stock_items ORDER BY name ASC;";
    return DatabaseManager::instance().executeQuery(sql);
}

QVariantMap MasterDataProvider::getStockItemDetails(int itemId) {
    QString sql = "SELECT * FROM stock_items WHERE id = ?;";
    QVariantList rows = DatabaseManager::instance().executeQuery(sql, { itemId });
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    return QVariantMap();
}

QVariantMap MasterDataProvider::getStockItemDetailsByName(const QString& itemName) {
    QString sql = "SELECT * FROM stock_items WHERE LOWER(name) = LOWER(?);";
    QVariantList rows = DatabaseManager::instance().executeQuery(sql, { itemName.trimmed() });
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    return QVariantMap();
}

QVariantMap MasterDataProvider::getStockItemDetailsByCode(const QString& itemCode) {
    QString sql = "SELECT * FROM stock_items WHERE LOWER(code) = LOWER(?);";
    QVariantList rows = DatabaseManager::instance().executeQuery(sql, { itemCode.trimmed() });
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    return QVariantMap();
}

QStringList MasterDataProvider::getStockItemNames() {
    QString sql = "SELECT name FROM stock_items ORDER BY name ASC;";
    QVariantList rows = DatabaseManager::instance().executeQuery(sql);
    QStringList names;
    for (const auto& r : rows) {
        names << r.toMap().value("name").toString();
    }
    return names;
}

QStringList MasterDataProvider::getStockCategories() {
    QString sql = "SELECT group_name FROM stock_groups ORDER BY group_name ASC;";
    QVariantList rows = DatabaseManager::instance().executeQuery(sql);
    QStringList cats;
    for (const auto& r : rows) {
        cats << r.toMap().value("group_name").toString();
    }
    return cats;
}

// 4. Units of Measurement
QVariantList MasterDataProvider::getAllStockUnits() {
    QString sql = "SELECT id, unit_name, legacy_code, decimal_places FROM stock_units ORDER BY unit_name ASC;";
    return DatabaseManager::instance().executeQuery(sql);
}

QStringList MasterDataProvider::getUnitNames() {
    QString sql = "SELECT unit_name FROM stock_units ORDER BY unit_name ASC;";
    QVariantList rows = DatabaseManager::instance().executeQuery(sql);
    QStringList names;
    for (const auto& r : rows) {
        names << r.toMap().value("unit_name").toString();
    }
    if (names.isEmpty()) {
        names << "Qtl." << "Bags" << "Kg." << "Nos" << "Mtr.";
    }
    return names;
}

QVariantMap MasterDataProvider::getUnitDetails(const QString& unitName) {
    QString sql = "SELECT id, unit_name, legacy_code, decimal_places FROM stock_units WHERE LOWER(unit_name) = LOWER(?);";
    QVariantList rows = DatabaseManager::instance().executeQuery(sql, { unitName.trimmed() });
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    return QVariantMap();
}

double MasterDataProvider::getUnitConversionFactor(const QString& fromUnit, const QString& toUnit) {
    QString f = fromUnit.trimmed().toLower();
    QString t = toUnit.trimmed().toLower();
    if (f == t) return 1.0;
    if ((f == "qtl" || f == "qtl.") && (t == "kg" || t == "kg.")) return 100.0;
    if ((f == "kg" || f == "kg.") && (t == "qtl" || t == "qtl.")) return 0.01;
    if ((f == "ton" || f == "mt") && (t == "qtl" || t == "qtl.")) return 10.0;
    return 1.0;
}

// 5. Dynamic Voucher Types & Configurations
QVariantList MasterDataProvider::getAvailableVoucherTypes() {
    QString sql = "SELECT * FROM voucher_settings;";
    QVariantList rows = DatabaseManager::instance().executeQuery(sql);
    if (rows.isEmpty()) {
        // Fallback default list
        QVariantList defaults;
        QVariantMap vSale; vSale["voucher_type"] = "Sale"; defaults.append(vSale);
        QVariantMap vPurc; vPurc["voucher_type"] = "Purc"; defaults.append(vPurc);
        QVariantMap vPymt; vPymt["voucher_type"] = "Pymt"; defaults.append(vPymt);
        QVariantMap vRcpt; vRcpt["voucher_type"] = "Rcpt"; defaults.append(vRcpt);
        QVariantMap vJrnl; vJrnl["voucher_type"] = "Jrnl"; defaults.append(vJrnl);
        return defaults;
    }
    return rows;
}

QVariantMap MasterDataProvider::getVoucherTypeByCode(const QString& code) {
    QString sql = "SELECT * FROM voucher_settings WHERE LOWER(voucher_type) = LOWER(?);";
    QVariantList rows = DatabaseManager::instance().executeQuery(sql, { code.trimmed() });
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    return QVariantMap();
}

QStringList MasterDataProvider::getVoucherTypeCodes() {
    QString sql = "SELECT voucher_type FROM voucher_settings ORDER BY voucher_type ASC;";
    QVariantList rows = DatabaseManager::instance().executeQuery(sql);
    QStringList codes;
    for (const auto& r : rows) {
        codes << r.toMap().value("voucher_type").toString();
    }
    if (codes.isEmpty()) {
        codes << "Sale" << "Purc" << "Pymt" << "Rcpt" << "Jrnl" << "ChPt" << "ChRt" << "JFrm" << "IFrm";
    }
    return codes;
}

QVariantMap MasterDataProvider::getVoucherSettings(const QString& voucherType) {
    return getVoucherTypeByCode(voucherType);
}

// 6. Tax Rates & Statutory Charges
QVariantList MasterDataProvider::getActiveTaxRates() {
    QString sql = "SELECT DISTINCT gst_rate FROM stock_items WHERE gst_rate IS NOT NULL ORDER BY gst_rate ASC;";
    QVariantList rows = DatabaseManager::instance().executeQuery(sql);
    if (rows.isEmpty()) {
        QVariantList defs;
        QVariantMap r0; r0["gst_rate"] = 0.0; defs.append(r0);
        QVariantMap r5; r5["gst_rate"] = 5.0; defs.append(r5);
        QVariantMap r12; r12["gst_rate"] = 12.0; defs.append(r12);
        QVariantMap r18; r18["gst_rate"] = 18.0; defs.append(r18);
        return defs;
    }
    return rows;
}

QVariantList MasterDataProvider::getMandiChargeDefinitions() {
    // Dynamic charge heads
    QVariantList charges;
    QVariantMap c1; c1["name"] = "Dami"; c1["default_rate"] = 2.0; c1["calc_on"] = "Goods"; charges.append(c1);
    QVariantMap c2; c2["name"] = "Market Fee"; c2["default_rate"] = 2.0; c2["calc_on"] = "Goods"; charges.append(c2);
    QVariantMap c3; c3["name"] = "H.R.D.F."; c3["default_rate"] = 2.0; c3["calc_on"] = "Goods"; charges.append(c3);
    QVariantMap c4; c4["name"] = "Labour"; c4["default_rate"] = 0.0; c4["calc_on"] = "Bags"; charges.append(c4);
    QVariantMap c5; c5["name"] = "Bardana"; c5["default_rate"] = 0.0; c5["calc_on"] = "Bags"; charges.append(c5);
    return charges;
}

double MasterDataProvider::getStandardTaxRate(const QString& hsnCode) {
    QString sql = "SELECT gst_rate FROM stock_items WHERE hsn_code = ? LIMIT 1;";
    QVariant val = DatabaseManager::instance().executeScalar(sql, { hsnCode.trimmed() });
    if (val.isValid() && val.toDouble() >= 0.0) {
        return val.toDouble();
    }
    return 0.0;
}

double MasterDataProvider::getItemGstRate(const QString& itemName, int itemId) {
    if (itemId > 0) {
        QVariant val = DatabaseManager::instance().executeScalar("SELECT gst_rate FROM stock_items WHERE id = ? LIMIT 1;", { itemId });
        if (val.isValid() && !val.isNull()) return val.toDouble();
    }
    if (!itemName.trimmed().isEmpty()) {
        QVariant val = DatabaseManager::instance().executeScalar("SELECT gst_rate FROM stock_items WHERE LOWER(TRIM(name)) = LOWER(TRIM(?)) LIMIT 1;", { itemName.trimmed() });
        if (val.isValid() && !val.isNull()) return val.toDouble();
    }
    return 0.0;
}

QString MasterDataProvider::getItemHsnCode(const QString& itemName, int itemId) {
    if (itemId > 0) {
        QVariant val = DatabaseManager::instance().executeScalar("SELECT hsn_code FROM stock_items WHERE id = ? LIMIT 1;", { itemId });
        if (val.isValid() && !val.toString().trimmed().isEmpty()) return val.toString().trimmed();
    }
    if (!itemName.trimmed().isEmpty()) {
        QVariant val = DatabaseManager::instance().executeScalar("SELECT hsn_code FROM stock_items WHERE LOWER(TRIM(name)) = LOWER(TRIM(?)) LIMIT 1;", { itemName.trimmed() });
        if (val.isValid() && !val.toString().trimmed().isEmpty()) return val.toString().trimmed();
    }
    return "1006";
}

double MasterDataProvider::getTcsRate(bool hasPan) {
    return hasPan ? 0.1 : 1.0;
}

double MasterDataProvider::getTds194QRate() {
    return 0.1;
}

// 7. Company & Financial Year Info
QVariantMap MasterDataProvider::getActiveCompanyInfo() {
    QString sql = "SELECT * FROM company_info LIMIT 1;";
    QVariantList rows = DatabaseManager::instance().executeQuery(sql);
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    return QVariantMap();
}

QVariantList MasterDataProvider::getFinancialYearsList() {
    QString sql = "SELECT id, year_name, start_date, end_date, is_active, is_locked FROM financial_years ORDER BY start_date DESC;";
    return DatabaseManager::instance().executeQuery(sql);
}

QVariantMap MasterDataProvider::getActiveFinancialYear() {
    QString sql = "SELECT id, year_name, start_date, end_date, is_active, is_locked FROM financial_years WHERE is_active = 1 LIMIT 1;";
    QVariantList rows = DatabaseManager::instance().executeQuery(sql);
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    // If none active, return latest
    sql = "SELECT id, year_name, start_date, end_date, is_active, is_locked FROM financial_years ORDER BY start_date DESC LIMIT 1;";
    rows = DatabaseManager::instance().executeQuery(sql);
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    return QVariantMap();
}
