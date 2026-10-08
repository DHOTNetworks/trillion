#include "firm_manager.h"
#include "../database_manager.h"
#include "../engine/bahi_khata_migrator.h"
#include "../engine/busy_data_migrator.h"
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

QString FirmManager::formatDateToDisplay(const QString& rawDate) {
    QString s = rawDate.trimmed();
    if (s.isEmpty()) return "";
    if (s.contains(" ")) {
        s = s.section(" ", 0, 0).trimmed();
    }
    s.replace("/", "-").replace(".", "-");

    QStringList parts = s.split('-', Qt::SkipEmptyParts);
    if (parts.size() == 3) {
        int day = 0, month = 0, year = 0;
        if (parts[0].length() == 4 && parts[0].toInt() >= 1900) {
            year = parts[0].toInt();
            month = parts[1].toInt();
            day = parts[2].toInt();
        } else {
            day = parts[0].toInt();
            month = parts[1].toInt();
            year = parts[2].toInt();
            if (parts[2].length() <= 2 || year < 100) {
                year += 2000;
            }
        }

        if (year >= 1900 && year <= 1970) {
            year += 100;
        }

        QDate d(year, month, day);
        if (d.isValid()) {
            return d.toString("dd-MM-yyyy");
        }
    }

    // Fallback QDate parsers
    QDate d = QDate::fromString(s, "dd-MM-yyyy");
    if (!d.isValid()) d = QDate::fromString(s, "yyyy-MM-dd");
    if (!d.isValid()) d = QDate::fromString(s, "d-M-yyyy");
    if (!d.isValid()) d = QDate::fromString(s, "yyyy-M-d");

    if (d.isValid()) {
        if (d.year() < 100) d = d.addYears(2000);
        else if (d.year() >= 1900 && d.year() <= 1970) d = d.addYears(100);
        return d.toString("dd-MM-yyyy");
    }
    return s;
}

QString FirmManager::getFirmPeriod(const QString& dbPath) {
    if (dbPath.isEmpty() || !QFile::exists(dbPath)) return "";
    sqlite3* db = nullptr;
    if (sqlite3_open_v2(dbPath.toUtf8().constData(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        return "";
    }

    QString startStr, endStr;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT MIN(start_date), MAX(end_date) FROM financial_years WHERE is_active >= 0;", -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* s0 = (const char*)sqlite3_column_text(stmt, 0);
            const char* s1 = (const char*)sqlite3_column_text(stmt, 1);
            if (s0 && s1) {
                startStr = QString::fromUtf8(s0).trimmed();
                endStr = QString::fromUtf8(s1).trimmed();
            }
        }
        sqlite3_finalize(stmt);
    }

    // Fallback to transactions / vouchers if financial_years table is empty or missing
    if (startStr.isEmpty() || endStr.isEmpty()) {
        if (sqlite3_prepare_v2(db, "SELECT MIN(voucher_date), MAX(voucher_date) FROM vouchers WHERE is_cancelled = 0;", -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* s0 = (const char*)sqlite3_column_text(stmt, 0);
                const char* s1 = (const char*)sqlite3_column_text(stmt, 1);
                if (s0 && s1) {
                    startStr = QString::fromUtf8(s0).trimmed();
                    endStr = QString::fromUtf8(s1).trimmed();
                }
            }
            sqlite3_finalize(stmt);
        }
    }

    sqlite3_close(db);

    if (!startStr.isEmpty() && !endStr.isEmpty()) {
        QString s_fmt = formatDateToDisplay(startStr.left(10));
        QString e_fmt = formatDateToDisplay(endStr.left(10));
        if (!s_fmt.isEmpty() && !e_fmt.isEmpty()) {
            return s_fmt + " to " + e_fmt;
        }
    }

    return "";
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
            QSet<QString> seenIds;
            for (const auto& v : arr) {
                QVariantMap f = v.toObject().toVariantMap();
                QString id = f.value("id").toString();
                QString dbName = f.value("db_name").toString();
                QString dbPath = f.value("db_path").toString();
                if (dbName == "test.db" || dbName == "mahadev_rice.db") continue;
                if (dbPath.endsWith("test.db") || dbPath.endsWith("mahadev_rice.db")) continue;
                if (!dbPath.isEmpty() && QFile::exists(dbPath) && QFileInfo(dbPath).size() == 0) continue;
                if (!id.isEmpty() && seenIds.contains(id)) continue;
                if (!dbName.isEmpty() && seenIds.contains(dbName)) continue;
                if (!id.isEmpty()) seenIds.insert(id);
                if (!dbName.isEmpty()) seenIds.insert(dbName);
                registeredFirms.append(f);
            }
        }
    }

    if (!settingsFirmId.isEmpty() && settingsFirmId != "mahadev_rice") {
        m_activeFirmId = settingsFirmId;
    } else if (m_activeFirmId == "mahadev_rice") {
        m_activeFirmId.clear();
    }

    // Auto-discover any .db databases present in data/ that are not yet in registry
    QDir dataDir(QDir::current().filePath("data"));
    if (dataDir.exists()) {
        QStringList dbFiles = dataDir.entryList({"*.db"}, QDir::Files, QDir::Name);
        for (const QString& dbName : dbFiles) {
            if (dbName == "test.db" || dbName == "mahadev_rice.db") continue;
            if (dbName.endsWith("-wal") || dbName.endsWith("-shm")) continue;
            if (dbName.startsWith("test_unit_suite")) continue;
            QString fullPath = dataDir.filePath(dbName);
            if (QFileInfo(fullPath).size() == 0) continue;

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
                firm["period"] = getFirmPeriod(fullPath);
                registeredFirms.append(firm);
            }
        }
    }

    // Refresh firm_type, statutory info, and period for all registered firms
    for (auto& f : registeredFirms) {
        QVariantMap m = f.toMap();
        QString fPan = m.value("pan").toString();
        QString fName = m.value("name").toString();
        QString fType = m.value("firm_type").toString();
        m["firm_type"] = BahiKhataMigrator::resolveFirmTypeFromPan(fType, fPan, fName);

        QString dbPath = m.value("db_path").toString();
        if (dbPath.isEmpty()) dbPath = "data/" + m.value("db_name").toString();
        if (!dbPath.startsWith("/") && !dbPath.startsWith("data/")) {
            dbPath = "data/" + dbPath;
        }

        if (QFile::exists(dbPath) && QFileInfo(dbPath).size() > 0) {
            QString period = getFirmPeriod(dbPath);
            if (!period.isEmpty()) {
                m["period"] = period;
            }
        }
        f = m;
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
    if (m_activeFirmId.isEmpty()) return "";
    QVariantMap info = currentFirmInfo();
    QString name = info.value("company_name").toString();
    if (name.isEmpty()) {
        QVariantList firms = const_cast<FirmManager*>(this)->get_registered_firms();
        for (const auto& f : firms) {
            if (f.toMap().value("id").toString() == m_activeFirmId) {
                return f.toMap().value("name").toString();
            }
        }
        return "";
    }
    return name;
}

QVariantMap FirmManager::currentFirmInfo() const {
    if (m_activeFirmId.isEmpty() || DatabaseManager::instance().getConnection() == nullptr) {
        return {};
    }
    QVariantList rows = DatabaseManager::instance().executeQuery("SELECT * FROM company_info LIMIT 1;");
    if (!rows.isEmpty()) {
        return rows.first().toMap();
    }
    return {};
}

QVariantMap FirmManager::get_current_firm_info() {
    return currentFirmInfo();
}

QVariantMap FirmManager::currentFirmRegistryEntry() const {
    if (m_activeFirmId.isEmpty()) return {};
    QVariantList firms = const_cast<FirmManager*>(this)->get_registered_firms();
    for (const auto& f : firms) {
        QVariantMap m = f.toMap();
        if (m.value("id").toString() == m_activeFirmId) {
            m.remove("isActive");
            return m;
        }
    }
    return {};
}

bool FirmManager::setFirmBahiKhataFile(const QString& firmId, const QString& fileName) {
    if (firmId.isEmpty() || fileName.isEmpty()) return false;
    QFile regFile(registryFilePath());
    if (!regFile.exists() || !regFile.open(QIODevice::ReadOnly)) return false;
    QJsonDocument doc = QJsonDocument::fromJson(regFile.readAll());
    regFile.close();
    if (!doc.isObject()) return false;
    QJsonObject root = doc.object();
    QJsonArray arr = root.value("firms").toArray();
    bool found = false;
    for (int i = 0; i < arr.size(); ++i) {
        QJsonObject o = arr[i].toObject();
        if (o.value("id").toString() == firmId) {
            o["bahi_khata_file"] = fileName;
            arr[i] = o;
            found = true;
            break;
        }
    }
    if (!found) return false;
    root["firms"] = arr;
    if (!regFile.open(QIODevice::WriteOnly)) return false;
    regFile.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    regFile.close();
    emit registryUpdated();
    return true;
}

QString FirmManager::deriveBahiKhataFileName(const QString& dbName, const QString& firmId) {
    // "..._data_004.db" / "test_migration_018" -> "Data.004" / "Data.018".
    static const QRegularExpression dataRe("(?:^|_)data_(\\d{1,3})(?:\\.db)?$",
                                           QRegularExpression::CaseInsensitiveOption);
    auto matchDigits = [&](const QString& s) -> QString {
        QRegularExpressionMatch m = dataRe.match(s);
        if (m.hasMatch()) {
            bool ok = false;
            int n = m.captured(1).toInt(&ok);
            if (ok && n >= 0 && n <= 999)
                return QString("Data.%1").arg(n, 3, 10, QChar('0'));
        }
        return "";
    };
    QString hit = matchDigits(dbName);
    if (!hit.isEmpty()) return hit;
    // Fallback: any standalone 3-digit run in the firm id.
    static const QRegularExpression anyRe("(\\d{3})");
    QRegularExpressionMatch m = anyRe.match(firmId);
    if (m.hasMatch()) return QString("Data.%1").arg(m.captured(1));
    return "";
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
    QStringList mdbCandidates = dir.entryList({"Data.*", "data.*", "DATA.*", "*.mdb", "*.accdb", "*.0*"}, QDir::Files, QDir::Name);
    QStringList mdbFiles;
    for (const QString& f : mdbCandidates) {
        if (f.endsWith(".ldb", Qt::CaseInsensitive) ||
            f.endsWith(".db", Qt::CaseInsensitive) ||
            f.endsWith(".db-wal", Qt::CaseInsensitive) ||
            f.endsWith(".db-shm", Qt::CaseInsensitive) ||
            f.endsWith(".json", Qt::CaseInsensitive) ||
            f.endsWith(".txt", Qt::CaseInsensitive) ||
            f.endsWith(".log", Qt::CaseInsensitive) ||
            f.endsWith(".bak", Qt::CaseInsensitive)) {
            continue;
        }
        if (!mdbFiles.contains(f)) {
            mdbFiles.append(f);
        }
    }

    // If scanning the App's Working Folder's data/ directory or any folder primarily with .db files:
    bool isAppData = (targetCanonical == appDataCanonical) || (!dbFiles.isEmpty() && mdbFiles.isEmpty());

    if (isAppData) {
        QVariantList regFirms = get_registered_firms();
        QMap<QString, QVariantMap> regMap;
        for (const auto& rf : regFirms) {
            QVariantMap m = rf.toMap();
            regMap[m.value("id").toString()] = m;
            regMap[m.value("db_name").toString()] = m;
        }

        for (const QString& dbName : dbFiles) {
            if (dbName == "test.db" || dbName == "mahadev_rice.db") continue; // skip legacy/mock templates
            if (dbName.endsWith("-wal") || dbName.endsWith("-shm")) continue;
            if (dbName.startsWith("test_unit_suite") || dbName.startsWith("test_migration_")) continue;

            QString fullPath = dir.filePath(dbName);
            if (QFileInfo(fullPath).size() == 0) continue;
            QString slug = dbName;
            slug.remove(".db");

            if (regMap.contains(slug) || regMap.contains(dbName)) {
                QVariantMap firm = regMap.value(slug, regMap.value(dbName));
                firm["isActive"] = (firm.value("id").toString() == m_activeFirmId);
                QString p = firm.value("period").toString();
                if (p.isEmpty() || p == "All Fiscal Years" || p == "Active" || !p.contains("to", Qt::CaseInsensitive)) {
                    QString calculatedPeriod = getFirmPeriod(fullPath);
                    if (!calculatedPeriod.isEmpty()) {
                        firm["period"] = calculatedPeriod;
                    }
                }
                results.append(firm);
                continue;
            }

            QVariantMap firm;
            firm["id"] = slug;
            firm["db_name"] = dbName;
            firm["db_path"] = fullPath;
            firm["source_file"] = dbName;
            firm["folder"] = targetFolder;
            firm["file_size"] = QFileInfo(fullPath).size();
            firm["is_imported"] = true;
            firm["isActive"] = (slug == m_activeFirmId);

            // Read statutory and firm details directly from SQLite for new database
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
                        if (c_gst && strlen(c_gst) > 0) firm["gstin"] = QString::fromUtf8(c_gst);
                        if (c_pan && strlen(c_pan) > 0) firm["pan"] = QString::fromUtf8(c_pan);
                        if (c_city && strlen(c_city) > 0) firm["city"] = QString::fromUtf8(c_city);
                        if (c_state && strlen(c_state) > 0) firm["state"] = QString::fromUtf8(c_state);
                        if (c_type && strlen(c_type) > 0) firm["firm_type"] = QString::fromUtf8(c_type);
                        if (c_biz && strlen(c_biz) > 0) firm["business"] = QString::fromUtf8(c_biz);
                    }
                    sqlite3_finalize(stmt);
                }
                sqlite3_close(db);
            }

            firm["period"] = getFirmPeriod(fullPath);

            if (!firm.contains("name") || firm["name"].toString().isEmpty()) {
                firm["name"] = slug.replace("_", " ").toUpper();
            }
            if (!firm.contains("city")) firm["city"] = "";
            firm["firm_type"] = BahiKhataMigrator::resolveFirmTypeFromPan(
                firm.value("firm_type").toString(),
                firm.value("pan").toString(),
                firm.value("name").toString()
            );

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
        firm["city"] = insp.value("station").toString();
        firm["state"] = insp.value("state").toString();
        firm["firm_type"] = insp.value("firmType").toString();
        firm["business"] = insp.value("business").toString();

        QString fyF = insp.value("fyFrom").toString();
        QString fyT = insp.value("fyTo").toString();
        if (!fyF.isEmpty() && !fyT.isEmpty()) {
            QString s_fmt = formatDateToDisplay(fyF);
            QString e_fmt = formatDateToDisplay(fyT);
            if (!s_fmt.isEmpty() && !e_fmt.isEmpty()) {
                firm["period"] = s_fmt + " to " + e_fmt;
            } else {
                firm["period"] = fyF + " to " + fyT;
            }
        } else {
            firm["period"] = "";
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

    // Check if Busy dataset first
    MahadevERP::BusyDataMigrator busyMig;
    QVariantMap insp = busyMig.inspect_busy_data(mdbFilePath);
    bool isBusy = insp.value("valid", false).toBool();

    if (!isBusy) {
        BahiKhataMigrator migrator;
        insp = migrator.inspect_mdb_file(mdbFilePath);
    }

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
        if (!fileStem.isEmpty() && !slug.endsWith(fileStem) && !isBusy) {
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
    firm["city"] = isBusy ? insp.value("city").toString() : insp.value("station").toString();
    firm["state"] = insp.value("state").toString();
    firm["firm_type"] = isBusy ? "Busy Accounting" : insp.value("firmType").toString();
    firm["business"] = isBusy ? "Rice Trading & Processing" : insp.value("business").toString();
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
    QDate cur = QDate::currentDate();
    int currentFyStartYear = (cur.month() >= 4) ? cur.year() : (cur.year() - 1);
    QString rawBooksFrom = firmInfo.value("books_from").toString().trimmed();
    if (rawBooksFrom.isEmpty()) rawBooksFrom = QString("%1-04-01").arg(currentFyStartYear);

    int startYear = currentFyStartYear;
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
    int targetEndYear = std::max(currentFyStartYear, fyStartYear);

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
            firmInfo.value("business_type").toString(),
            firmInfo.value("address").toString(),
            firmInfo.value("city").toString(),
            firmInfo.value("state").toString(),
            firmInfo.value("state_code").toString(),
            firmInfo.value("pincode").toString(),
            firmInfo.value("phone").toString(),
            firmInfo.value("mobile").toString(),
            firmInfo.value("email").toString(),
            gstin,
            pan,
            firmInfo.value("fssai_no").toString(),
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
        QString fyName = QString("FY %1-%2").arg(y).arg(QString::number((y + 1) % 100).rightJustified(2, '0'));
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
    regItem["city"] = firmInfo.value("city").toString();
    regItem["state"] = firmInfo.value("state").toString();
    regItem["firm_type"] = resolvedFirmType;
    regItem["is_imported"] = true;
    regItem["period"] = QString("%1-04-01 To %2-03-31").arg(fyStartYear).arg(targetEndYear + 1);

    QVariantList firms = get_registered_firms();
    firms.append(regItem);
    saveRegistry(firms);

    emit firmSwitched(m_activeFirmId, compName);
    return true;
}
