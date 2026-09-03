#include "firm_manager.h"
#include "../database_manager.h"
#include "../engine/bahi_khata_migrator.h"
#include <sqlite3.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QDateTime>
#include <QDebug>
#include <iostream>

FirmManager::FirmManager(QObject* parent)
    : QObject(parent), m_activeFolder(QDir::current().filePath("data"))
{
    loadRegistry();
}

QString FirmManager::get_app_data_folder() const {
    return QDir::current().filePath("data");
}

QString FirmManager::registryFilePath() const {
    QDir dataDir(QDir::current().filePath("data"));
    if (!dataDir.exists()) dataDir.mkpath(".");
    return dataDir.filePath("firms_registry.json");
}

QString FirmManager::sanitizeSlug(const QString& name) {
    QString slug = name.toLower();
    slug.remove(QRegularExpression("^m/s\\s*"));
    slug.replace(QRegularExpression("[^a-z0-9]+"), "_");
    slug.remove(QRegularExpression("^_+|_+$"));
    if (slug.isEmpty()) slug = "firm_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    return slug;
}

void FirmManager::loadRegistry() {
    QFile regFile(registryFilePath());
    QVariantList registeredFirms;

    if (regFile.exists() && regFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(regFile.readAll());
        regFile.close();
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            m_activeFirmId = root.value("active_firm_id").toString();
            m_activeFolder = QDir::cleanPath(QDir::current().filePath("data"));
            QJsonArray arr = root.value("firms").toArray();
            for (const auto& v : arr) {
                registeredFirms.append(v.toObject().toVariantMap());
            }
        }
    }

    // If registry is empty or missing Mahadev Rice, initialize defaults
    bool hasMahadev = false;
    for (const auto& f : registeredFirms) {
        if (f.toMap().value("id").toString() == "mahadev_rice") {
            hasMahadev = true;
            break;
        }
    }

    if (!hasMahadev) {
        QVariantMap mahadev;
        mahadev["id"] = "mahadev_rice";
        mahadev["name"] = "M/S MAHADEV RICE INDUSTRY";
        mahadev["db_name"] = "mahadev_rice.db";
        mahadev["db_path"] = "data/mahadev_rice.db";
        mahadev["source_file"] = "Data.004";
        mahadev["folder"] = m_activeFolder;
        mahadev["gstin"] = "06ABKFM5928Q1ZG";
        mahadev["pan"] = "ABKFM5928Q";
        mahadev["city"] = "SIRSA";
        mahadev["state"] = "HARYANA";
        mahadev["firm_type"] = "Partnership Firm";
        mahadev["period"] = "01-04-2023 To 31-03-2027";
        mahadev["is_imported"] = true;
        registeredFirms.prepend(mahadev);
    }

    // Check if Haritage Harvestor exists or register it
    bool hasHaritage = false;
    for (const auto& f : registeredFirms) {
        if (f.toMap().value("id").toString() == "haritage_harvestor") {
            hasHaritage = true;
            break;
        }
    }
    if (!hasHaritage) {
        QVariantMap haritage;
        haritage["id"] = "haritage_harvestor";
        haritage["name"] = "M/S HARITAGE HARVESTOR AGRO PRODUCTS";
        haritage["db_name"] = "haritage_harvestor.db";
        haritage["db_path"] = "data/haritage_harvestor.db";
        haritage["source_file"] = "Data.001";
        haritage["folder"] = m_activeFolder;
        haritage["gstin"] = "06BCUPK4267Q2ZL";
        haritage["pan"] = "BCUPK4267Q";
        haritage["city"] = "SIRSA";
        haritage["state"] = "HARYANA";
        haritage["firm_type"] = "Proprietorship Firm";
        haritage["period"] = "01-04-2024 To 31-03-2027";
        haritage["is_imported"] = QFile::exists("data/haritage_harvestor.db");
        registeredFirms.append(haritage);
    }

    if (m_activeFirmId.isEmpty()) {
        m_activeFirmId = "mahadev_rice";
    }

    saveRegistry(registeredFirms);
}

void FirmManager::saveRegistry(const QVariantList& firms) {
    QFile regFile(registryFilePath());
    if (regFile.open(QIODevice::WriteOnly)) {
        QJsonObject root;
        root["active_firm_id"] = m_activeFirmId;
        root["active_folder"] = m_activeFolder;
        QJsonArray arr;
        for (const auto& f : firms) {
            arr.append(QJsonObject::fromVariantMap(f.toMap()));
        }
        root["firms"] = arr;
        regFile.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        regFile.close();
    }
    emit registryUpdated();
}

QString FirmManager::currentFirmName() const {
    QVariantMap info = currentFirmInfo();
    QString name = info.value("company_name").toString();
    if (name.isEmpty()) {
        QVariantList firms = const_cast<FirmManager*>(this)->get_registered_firms();
        for (const auto& f : firms) {
            if (f.toMap().value("id").toString() == m_activeFirmId) {
                return f.toMap().value("name").toString();
            }
        }
        return "M/S MAHADEV RICE INDUSTRY";
    }
    return name;
}

QVariantMap FirmManager::currentFirmInfo() const {
    QVariantList rows = DatabaseManager::instance().executeQuery("SELECT * FROM company_info LIMIT 1;");
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    return {};
}

QVariantMap FirmManager::get_current_firm_info() {
    return currentFirmInfo();
}

QVariantList FirmManager::get_registered_firms() {
    QFile regFile(registryFilePath());
    if (regFile.exists() && regFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(regFile.readAll());
        regFile.close();
        if (doc.isObject()) {
            QJsonArray arr = doc.object().value("firms").toArray();
            QVariantList res;
            for (const auto& v : arr) {
                QVariantMap m = v.toObject().toVariantMap();
                m["isActive"] = (m.value("id").toString() == m_activeFirmId);
                res.append(m);
            }
            return res;
        }
    }
    return {};
}

QString FirmManager::get_active_firm_folder() {
    return m_activeFolder;
}

void FirmManager::set_active_firm_folder(const QString& folderPath) {
    if (!folderPath.isEmpty() && m_activeFolder != folderPath) {
        m_activeFolder = folderPath;
        QVariantList firms = get_registered_firms();
        saveRegistry(firms);
        emit activeFolderChanged();
    }
}

void FirmManager::setActiveFolder(const QString& folder) {
    set_active_firm_folder(folder);
}

void FirmManager::refresh_registry() {
    loadRegistry();
}

QVariantList FirmManager::scan_folder_for_firms(const QString& folderPath) {
    QString targetFolder = folderPath.isEmpty() ? m_activeFolder : folderPath;
    QDir dir(targetFolder);
    QVariantList results;

    if (!dir.exists()) return results;

    QString appDataCanonical = QDir(QDir::current().filePath("data")).canonicalPath();
    QString targetCanonical = dir.canonicalPath();

    QStringList dbFiles = dir.entryList({"*.db"}, QDir::Files, QDir::Name);
    QStringList mdbFiles = dir.entryList({"Data.*"}, QDir::Files, QDir::Name);

    // If scanning the App's Working Folder's data/ directory or any folder primarily with .db files:
    bool isAppData = (targetCanonical == appDataCanonical) || (!dbFiles.isEmpty() && mdbFiles.isEmpty());

    if (isAppData) {
        for (const QString& dbName : dbFiles) {
            if (dbName == "mahadev_accounting.db") continue; // skip legacy/backup template
            if (dbName.endsWith("-wal") || dbName.endsWith("-shm")) continue;

            QString fullPath = dir.filePath(dbName);
            QString slug = dbName;
            slug.remove(".db");

            QVariantMap firm;
            firm["id"] = slug;
            firm["db_name"] = dbName;
            firm["db_path"] = fullPath;
            firm["source_file"] = dbName;
            firm["folder"] = targetFolder;
            firm["file_size"] = QFileInfo(fullPath).size();
            firm["is_imported"] = true;
            firm["isActive"] = (slug == m_activeFirmId);

            // Read statutory and firm details directly from SQLite
            sqlite3* db = nullptr;
            if (sqlite3_open_v2(fullPath.toUtf8().constData(), &db, SQLITE_OPEN_READONLY, nullptr) == SQLITE_OK) {
                sqlite3_stmt* stmt = nullptr;
                if (sqlite3_prepare_v2(db, "SELECT company_name, gstin, pan_no, city, state, firm_type, business_type FROM company_info LIMIT 1;", -1, &stmt, nullptr) == SQLITE_OK) {
                    if (sqlite3_step(stmt) == SQLITE_ROW) {
                        const char* c_name = (const char*)sqlite3_column_text(stmt, 0);
                        const char* c_gst = (const char*)sqlite3_column_text(stmt, 1);
                        const char* c_pan = (const char*)sqlite3_column_text(stmt, 2);
                        const char* c_city = (const char*)sqlite3_column_text(stmt, 3);
                        const char* c_state = (const char*)sqlite3_column_text(stmt, 4);
                        const char* c_type = (const char*)sqlite3_column_text(stmt, 5);
                        const char* c_biz = (const char*)sqlite3_column_text(stmt, 6);

                        if (c_name && strlen(c_name) > 0) firm["name"] = QString::fromUtf8(c_name);
                        if (c_gst) firm["gstin"] = QString::fromUtf8(c_gst);
                        if (c_pan) firm["pan"] = QString::fromUtf8(c_pan);
                        if (c_city) firm["city"] = QString::fromUtf8(c_city);
                        if (c_state) firm["state"] = QString::fromUtf8(c_state);
                        if (c_type) firm["firm_type"] = QString::fromUtf8(c_type);
                        if (c_biz) firm["business"] = QString::fromUtf8(c_biz);
                    }
                    sqlite3_finalize(stmt);
                }

                if (sqlite3_prepare_v2(db, "SELECT MIN(start_date), MAX(end_date) FROM financial_years;", -1, &stmt, nullptr) == SQLITE_OK) {
                    if (sqlite3_step(stmt) == SQLITE_ROW) {
                        const char* s_date = (const char*)sqlite3_column_text(stmt, 0);
                        const char* e_date = (const char*)sqlite3_column_text(stmt, 1);
                        if (s_date && e_date) {
                            firm["period"] = QString::fromUtf8(s_date) + " To " + QString::fromUtf8(e_date);
                        }
                    }
                    sqlite3_finalize(stmt);
                }
                sqlite3_close(db);
            }

            if (!firm.contains("name") || firm["name"].toString().isEmpty()) {
                if (slug == "mahadev_rice") firm["name"] = "M/S MAHADEV RICE INDUSTRY";
                else if (slug == "sushil_trading") firm["name"] = "M/S SUSHIL TRADING COMPANY";
                else if (slug == "haritage_harvestor") firm["name"] = "M/S HARITAGE HARVESTOR AGRO PRODUCTS";
                else firm["name"] = slug.replace("_", " ").toUpper();
            }
            if (!firm.contains("city") || firm["city"].toString().isEmpty()) firm["city"] = "Sirsa";
            if (!firm.contains("firm_type") || firm["firm_type"].toString().isEmpty()) firm["firm_type"] = "Partnership Firm";
            if (!firm.contains("period") || firm["period"].toString().isEmpty()) firm["period"] = "Active";

            results.append(firm);
        }
        return results;
    }

    // Otherwise, external Bahi-Khata folder selected by user for import!
    BahiKhataMigrator migrator;
    for (const QString& fName : mdbFiles) {
        if (fName.endsWith(".ldb", Qt::CaseInsensitive)) continue;

        QString fullPath = dir.filePath(fName);
        QVariantMap insp = migrator.inspect_mdb_file(fullPath);

        QVariantMap firm;
        firm["source_file"] = fName;
        firm["full_path"] = fullPath;
        firm["folder"] = targetFolder;
        firm["file_size"] = QFileInfo(fullPath).size();

        QString compName = insp.value("companyName").toString();
        if (compName.isEmpty()) {
            compName = "Firm (" + fName + ")";
        }
        firm["name"] = compName;

        QString slug = sanitizeSlug(compName);
        if (fName == "Data.004" || slug.contains("mahadev")) slug = "mahadev_rice";
        else if (fName == "Data.001" || slug.contains("haritage")) slug = "haritage_harvestor";
        else if (fName == "Data.018" || slug.contains("sushil")) slug = "sushil_trading";

        firm["id"] = slug;
        firm["db_name"] = slug + ".db";
        firm["db_path"] = "data/" + slug + ".db";
        firm["gstin"] = insp.value("gstin").toString();
        firm["pan"] = insp.value("pan").toString();
        firm["city"] = insp.value("station").toString().isEmpty() ? "Sirsa" : insp.value("station").toString();
        firm["state"] = insp.value("state").toString().isEmpty() ? "Haryana" : insp.value("state").toString();
        firm["firm_type"] = insp.value("firmType").toString();
        firm["business"] = insp.value("business").toString();

        QString fyF = insp.value("fyFrom").toString();
        QString fyT = insp.value("fyTo").toString();
        if (!fyF.isEmpty() && !fyT.isEmpty()) {
            QString s_fmt = fyF.left(8).trimmed().replace("/", "-");
            QString e_fmt = fyT.left(8).trimmed().replace("/", "-");
            firm["period"] = s_fmt + " To " + e_fmt;
        } else {
            firm["period"] = "Active";
        }

        bool isImported = QFile::exists(firm["db_path"].toString());
        firm["is_imported"] = isImported;
        firm["isActive"] = (firm["id"].toString() == m_activeFirmId);

        results.append(firm);
    }

    return results;
}

bool FirmManager::switch_to_firm(const QString& firmId) {
    if (firmId.isEmpty()) return false;

    QVariantList firms = get_registered_firms();
    QVariantMap targetFirm;
    for (const auto& f : firms) {
        if (f.toMap().value("id").toString() == firmId) {
            targetFirm = f.toMap();
            break;
        }
    }

    // If not in registry, try to find in folder scan
    if (targetFirm.isEmpty()) {
        QVariantList scanned = scan_folder_for_firms();
        for (const auto& s : scanned) {
            if (s.toMap().value("id").toString() == firmId) {
                targetFirm = s.toMap();
                firms.append(targetFirm);
                break;
            }
        }
    }

    if (targetFirm.isEmpty()) {
        qWarning() << "Firm not found:" << firmId;
        return false;
    }

    QString dbPath = targetFirm.value("db_path").toString();
    if (dbPath.isEmpty()) {
        dbPath = "data/" + firmId + ".db";
    }

    // If db file doesn't exist yet, but source_file exists, auto import it!
    if (!QFile::exists(dbPath) && targetFirm.contains("full_path")) {
        QString srcMdb = targetFirm.value("full_path").toString();
        if (QFile::exists(srcMdb)) {
            import_bahi_khata_firm(srcMdb, targetFirm.value("name").toString());
        }
    }

    std::cout << "[INFO] Switching to firm: " << firmId.toStdString() << " Database: " << dbPath.toStdString() << std::endl;
    bool ok = DatabaseManager::instance().switchDatabase(dbPath);
    if (ok) {
        m_activeFirmId = firmId;
        saveRegistry(firms);
        emit firmSwitched(m_activeFirmId, targetFirm.value("name").toString());
        return true;
    }

    return false;
}

bool FirmManager::prepare_firm_for_import(const QString& mdbFilePath, const QString& customFirmName, const QString& explicitFirmId) {
    QFileInfo fi(mdbFilePath);
    if (!fi.exists()) return false;

    BahiKhataMigrator migrator;
    QVariantMap insp = migrator.inspect_mdb_file(mdbFilePath);

    QString firmName = customFirmName;
    if (firmName.isEmpty()) {
        firmName = insp.value("companyName").toString();
    }
    if (firmName.isEmpty()) {
        firmName = "Firm " + fi.fileName();
    }

    QString slug = explicitFirmId;
    if (slug.isEmpty()) {
        slug = sanitizeSlug(firmName);
        if (fi.fileName() == "Data.004" || slug.contains("mahadev")) slug = "mahadev_rice";
        else if (fi.fileName() == "Data.001" || slug.contains("haritage")) slug = "haritage_harvestor";
        else if (fi.fileName() == "Data.018" || slug.contains("sushil")) slug = "sushil_trading";
    }

    QString targetDbPath = "data/" + slug + ".db";

    std::cout << "[INFO] Preparing firm database for import: " << firmName.toStdString() << " (" << slug.toStdString() << ") -> " << targetDbPath.toStdString() << std::endl;

    // Switch DB to target (will initialize tables if needed)
    DatabaseManager::instance().switchDatabase(targetDbPath);
    m_activeFirmId = slug;

    QVariantMap firm;
    firm["id"] = slug;
    firm["name"] = firmName;
    firm["db_name"] = slug + ".db";
    firm["db_path"] = targetDbPath;
    firm["source_file"] = fi.fileName();
    firm["folder"] = fi.absolutePath();
    firm["full_path"] = fi.absoluteFilePath();
    firm["gstin"] = insp.value("gstin").toString();
    firm["pan"] = insp.value("pan").toString();
    firm["city"] = insp.value("station").toString().isEmpty() ? "Sirsa" : insp.value("station").toString();
    firm["state"] = insp.value("state").toString().isEmpty() ? "Haryana" : insp.value("state").toString();
    firm["firm_type"] = insp.value("firmType").toString();
    firm["business"] = insp.value("business").toString();
    firm["is_imported"] = true;

    QVariantList firms = get_registered_firms();
    bool found = false;
    for (int i = 0; i < firms.size(); ++i) {
        if (firms[i].toMap().value("id").toString() == slug) {
            firms[i] = firm;
            found = true;
            break;
        }
    }
    if (!found) firms.append(firm);

    saveRegistry(firms);
    return true;
}

bool FirmManager::import_bahi_khata_firm(const QString& mdbFilePath, const QString& customFirmName) {
    if (!prepare_firm_for_import(mdbFilePath, customFirmName)) return false;

    std::cout << "[INFO] Migrating MDB " << mdbFilePath.toStdString() << " into active DB" << std::endl;
    BahiKhataMigrator migrator;
    bool ok = migrator.migrate_mdb_file(mdbFilePath);
    if (ok) {
        emit firmSwitched(m_activeFirmId, currentFirmName());
        return true;
    }
    return false;
}

bool FirmManager::create_new_firm(const QVariantMap& firmInfo) {
    QString compName = firmInfo.value("company_name").toString().trimmed();
    if (compName.isEmpty()) return false;

    QString slug = sanitizeSlug(compName);
    QString targetDbPath = "data/" + slug + ".db";

    DatabaseManager::instance().switchDatabase(targetDbPath);

    // Populate company_info
    DatabaseManager::instance().executeNonQuery("DELETE FROM company_info;");
    DatabaseManager::instance().executeNonQuery(
        "INSERT INTO company_info ("
        "company_name, firm_type, business_type, address, city, state, state_code, pincode, "
        "phone, mobile, email, gstin, pan_no, fssai_no, ml_no, "
        "bank_name, bank_account, ifsc_code, books_from, acc_year_from, acc_year_to, data_file_source"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'Native');",
        {
            compName,
            firmInfo.value("firm_type", "Partnership Firm").toString(),
            firmInfo.value("business_type", "Rice Mill").toString(),
            firmInfo.value("address").toString(),
            firmInfo.value("city", "Sirsa").toString(),
            firmInfo.value("state", "Haryana").toString(),
            firmInfo.value("state_code", "06").toString(),
            firmInfo.value("pincode", "125055").toString(),
            firmInfo.value("phone").toString(),
            firmInfo.value("mobile").toString(),
            firmInfo.value("email").toString(),
            firmInfo.value("gstin").toString(),
            firmInfo.value("pan_no").toString(),
            firmInfo.value("fssai_no").toString(),
            firmInfo.value("ml_no").toString(),
            firmInfo.value("bank_name").toString(),
            firmInfo.value("bank_account").toString(),
            firmInfo.value("ifsc_code").toString(),
            firmInfo.value("books_from", "2026-04-01").toString(),
            firmInfo.value("acc_year_from", "2026-04-01").toString(),
            firmInfo.value("acc_year_to", "2027-03-31").toString()
        }
    );

    // Create default Financial Year
    QString fyName = firmInfo.value("fy_name", "FY 2026-27").toString();
    QString fyStart = firmInfo.value("acc_year_from", "2026-04-01").toString();
    QString fyEnd = firmInfo.value("acc_year_to", "2027-03-31").toString();

    DatabaseManager::instance().executeNonQuery(
        "INSERT OR IGNORE INTO financial_years (year_name, start_date, end_date, is_active, is_locked) "
        "VALUES (?, ?, ?, 1, 0);",
        {fyName, fyStart, fyEnd}
    );

    m_activeFirmId = slug;

    QVariantMap regItem;
    regItem["id"] = slug;
    regItem["name"] = compName;
    regItem["db_name"] = slug + ".db";
    regItem["db_path"] = targetDbPath;
    regItem["folder"] = QDir::current().filePath("data");
    regItem["gstin"] = firmInfo.value("gstin").toString();
    regItem["pan"] = firmInfo.value("pan_no").toString();
    regItem["city"] = firmInfo.value("city", "Sirsa").toString();
    regItem["state"] = firmInfo.value("state", "Haryana").toString();
    regItem["firm_type"] = firmInfo.value("firm_type", "Partnership Firm").toString();
    regItem["is_imported"] = true;
    regItem["period"] = fyStart + " To " + fyEnd;

    QVariantList firms = get_registered_firms();
    firms.append(regItem);
    saveRegistry(firms);

    emit firmSwitched(m_activeFirmId, compName);
    return true;
}
