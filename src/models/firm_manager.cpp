#include "firm_manager.h"
#include "../database_manager.h"
#include "../engine/bahi_khata_migrator.h"
#include <sqlite3.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QDateTime>
#include <QSettings>
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

QString FirmManager::choose_firm_folder(const QString& currentFolder) {
    QString start = currentFolder.isEmpty() ? get_app_data_folder() : currentFolder;
    return QFileDialog::getExistingDirectory(nullptr, "Select Bahi-Khata Firm Data Folder", start);
}

QString FirmManager::registryFilePath() const {
    QDir dataDir(QDir::current().filePath("data"));
    if (!dataDir.exists()) dataDir.mkpath(".");
    return dataDir.filePath("firms_registry.json");
}

QString FirmManager::sanitizeSlug(const QString& name) {
    QString slug = name.toLower();
    slug.remove(QRegularExpression("^m/s\\s*"));
    slug.remove(QRegularExpression("^ms\\s*"));
    slug.replace(QRegularExpression("[^a-z0-9]+"), "_");
    slug.remove(QRegularExpression("^_+|_+$"));
    if (slug.isEmpty()) slug = "firm_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    return slug;
}

void FirmManager::loadRegistry() {
    QFile regFile(registryFilePath());
    QVariantList registeredFirms;

    QSettings settings("MahadevAgro", "Mahadev Rice Mill ERP");
    QString settingsFirmId = settings.value("active_firm_id").toString();

    if (regFile.exists() && regFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(regFile.readAll());
        regFile.close();
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            if (settingsFirmId.isEmpty()) {
                m_activeFirmId = root.value("active_firm_id").toString();
            }
            m_activeFolder = QDir::cleanPath(QDir::current().filePath("data"));
            QJsonArray arr = root.value("firms").toArray();
            for (const auto& v : arr) {
                registeredFirms.append(v.toObject().toVariantMap());
            }
        }
    }

    if (!settingsFirmId.isEmpty()) {
        m_activeFirmId = settingsFirmId;
    }

    // Auto-discover any .db databases present in data/ that are not yet in registry
    QDir dataDir(QDir::current().filePath("data"));
    if (dataDir.exists()) {
        QStringList dbFiles = dataDir.entryList({"*.db"}, QDir::Files, QDir::Name);
        for (const QString& dbName : dbFiles) {
            if (dbName == "mahadev_accounting.db") continue;
            if (dbName.endsWith("-wal") || dbName.endsWith("-shm")) continue;
            QString slug = dbName;
            slug.remove(".db");

            bool existsInReg = false;
            for (const auto& f : registeredFirms) {
                if (f.toMap().value("id").toString() == slug || f.toMap().value("db_name").toString() == dbName) {
                    existsInReg = true;
                    break;
                }
            }

            if (!existsInReg) {
                QString fullPath = dataDir.filePath(dbName);
                QVariantMap firm;
                firm["id"] = slug;
                firm["db_name"] = dbName;
                firm["db_path"] = "data/" + dbName;
                firm["source_file"] = dbName;
                firm["folder"] = dataDir.absolutePath();
                firm["is_imported"] = true;

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
                    sqlite3_close(db);
                }

                if (!firm.contains("name") || firm["name"].toString().isEmpty()) {
                    firm["name"] = slug.replace("_", " ").toUpper();
                }
                firm["firm_type"] = BahiKhataMigrator::resolveFirmTypeFromPan(
                    firm.value("firm_type").toString(),
                    firm.value("pan").toString(),
                    firm.value("name").toString()
                );
                registeredFirms.append(firm);
            }
        }
    }

    // Refresh firm_type and statutory info for already registered firms
    for (auto& f : registeredFirms) {
        QVariantMap m = f.toMap();
        QString fPan = m.value("pan").toString();
        QString fName = m.value("name").toString();
        QString fType = m.value("firm_type").toString();
        m["firm_type"] = BahiKhataMigrator::resolveFirmTypeFromPan(fType, fPan, fName);
        f = m;
    }

    if (m_activeFirmId.isEmpty() && !registeredFirms.isEmpty()) {
        m_activeFirmId = registeredFirms.first().toMap().value("id").toString();
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

    QSettings settings("MahadevAgro", "Mahadev Rice Mill ERP");
    if (!m_activeFirmId.isEmpty()) {
        settings.setValue("active_firm_id", m_activeFirmId);
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
        return "Company";
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
                firm["name"] = slug.replace("_", " ").toUpper();
            }
            if (!firm.contains("city") || firm["city"].toString().isEmpty()) firm["city"] = "Sirsa";
            firm["firm_type"] = BahiKhataMigrator::resolveFirmTypeFromPan(
                firm.value("firm_type").toString(),
                firm.value("pan").toString(),
                firm.value("name").toString()
            );
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

        QString fileStem = QFileInfo(fName).fileName().toLower().replace(".", "_");
        QString compSlug = sanitizeSlug(compName);
        QString slug = compSlug;
        if (!fileStem.isEmpty() && !slug.endsWith(fileStem)) {
            slug += "_" + fileStem;
        }

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
        firm["isActive"] = false; // Never auto-activate external files before user explicitly opens or imports

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
        QString fileStem = fi.fileName().toLower().replace(".", "_");
        QString compSlug = sanitizeSlug(firmName);
        slug = compSlug;
        if (!fileStem.isEmpty() && !slug.endsWith(fileStem)) {
            slug += "_" + fileStem;
        }
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

    QString gstin = firmInfo.value("gstin").toString().trimmed();
    QString pan = firmInfo.value("pan_no").toString().trimmed();
    if (pan.isEmpty() && gstin.length() == 15) {
        pan = gstin.mid(2, 10);
    }

    QString rawType = firmInfo.value("firm_type", "Partnership Firm").toString();
    QString resolvedFirmType = BahiKhataMigrator::resolveFirmTypeFromPan(rawType, pan, compName);

    QString slug = sanitizeSlug(compName);
    QString targetDbPath = "data/" + slug + ".db";

    DatabaseManager::instance().switchDatabase(targetDbPath);

    // Flexible date parsing for books_from (e.g., "1.4.24", "01/04/2024", "2024-04-01", "1-4-24", "01.04.2024")
    QString rawBooksFrom = firmInfo.value("books_from").toString().trimmed();
    if (rawBooksFrom.isEmpty()) rawBooksFrom = "2026-04-01";

    int startYear = 2026;
    int startMonth = 4;
    int startDay = 1;

    QRegularExpression dateDmy(R"(^(\d{1,2})[\.\/\-](\d{1,2})[\.\/\-](\d{2,4})$)");
    QRegularExpression dateYmd(R"(^(\d{4})[\.\/\-](\d{1,2})[\.\/\-](\d{1,2})$)");
    auto mDmy = dateDmy.match(rawBooksFrom);
    auto mYmd = dateYmd.match(rawBooksFrom);

    if (mYmd.hasMatch()) {
        startYear = mYmd.captured(1).toInt();
        startMonth = mYmd.captured(2).toInt();
        startDay = mYmd.captured(3).toInt();
    } else if (mDmy.hasMatch()) {
        startDay = mDmy.captured(1).toInt();
        startMonth = mDmy.captured(2).toInt();
        int y = mDmy.captured(3).toInt();
        if (y < 100) y += 2000;
        startYear = y;
    }

    // Determine the starting FY year
    int fyStartYear = (startMonth >= 4) ? startYear : (startYear - 1);
    int currentYear = 2026; // Base active year
    int targetEndYear = std::max(currentYear, fyStartYear);

    QString formattedBooksFrom = QString("%1-%2-%3")
        .arg(startYear, 4, 10, QChar('0'))
        .arg(startMonth, 2, 10, QChar('0'))
        .arg(startDay, 2, 10, QChar('0'));

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
            resolvedFirmType,
            firmInfo.value("business_type", "Rice Mill & Grain Processing").toString(),
            firmInfo.value("address").toString(),
            firmInfo.value("city", "Sirsa").toString(),
            firmInfo.value("state", "Haryana").toString(),
            firmInfo.value("state_code", "06").toString(),
            firmInfo.value("pincode", "125055").toString(),
            firmInfo.value("phone").toString(),
            firmInfo.value("mobile").toString(),
            firmInfo.value("email").toString(),
            gstin,
            pan,
            firmInfo.value("fssai_no", "10822019000152").toString(),
            firmInfo.value("ml_no").toString(),
            firmInfo.value("bank_name").toString(),
            firmInfo.value("bank_account").toString(),
            firmInfo.value("ifsc_code").toString(),
            formattedBooksFrom,
            QString("%1-04-01").arg(targetEndYear),
            QString("%1-03-31").arg(targetEndYear + 1)
        }
    );

    // Create all Financial Years from fyStartYear to targetEndYear (e.g. 2024 -> FY 2024-25, FY 2025-26, FY 2026-27, FY 2027-28)
    DatabaseManager::instance().executeNonQuery("DELETE FROM financial_years;");
    for (int y = fyStartYear; y <= targetEndYear; ++y) {
        QString fyName = QString("FY %1-%2").arg(y).arg(QString::number(y + 1).right(2));
        QString fyStart = QString("%1-04-01").arg(y);
        QString fyEnd = QString("%1-03-31").arg(y + 1);
        int isActive = (y == targetEndYear) ? 1 : 0;

        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO financial_years (year_name, start_date, end_date, is_active, is_locked) "
            "VALUES (?, ?, ?, ?, 0);",
            {fyName, fyStart, fyEnd, isActive}
        );
    }

    m_activeFirmId = slug;

    QVariantMap regItem;
    regItem["id"] = slug;
    regItem["name"] = compName;
    regItem["db_name"] = slug + ".db";
    regItem["db_path"] = targetDbPath;
    regItem["folder"] = QDir::current().filePath("data");
    regItem["gstin"] = gstin;
    regItem["pan"] = pan;
    regItem["city"] = firmInfo.value("city", "Sirsa").toString();
    regItem["state"] = firmInfo.value("state", "Haryana").toString();
    regItem["firm_type"] = resolvedFirmType;
    regItem["is_imported"] = true;
    regItem["period"] = QString("%1-04-01 To %2-03-31").arg(fyStartYear).arg(targetEndYear + 1);

    QVariantList firms = get_registered_firms();
    firms.append(regItem);
    saveRegistry(firms);

    emit firmSwitched(m_activeFirmId, compName);
    return true;
}
