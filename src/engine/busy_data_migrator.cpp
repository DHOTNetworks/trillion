#include "busy_data_migrator.h"
#include "../database_manager.h"
#include "../models/account_classifier.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QFileDialog>
#include <QRegularExpression>
#include <QDebug>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <cmath>
#include <regex>
#include <algorithm>

#include "mdbtools.h"
#define HAS_LIBMDB 1

namespace MahadevERP {

static QString normalizePath(const QString& rawPath) {
    QString path = rawPath.trimmed();
    if (path.startsWith("file://", Qt::CaseInsensitive)) {
        QUrl url(path);
        if (url.isValid() && !url.toLocalFile().isEmpty()) {
            path = url.toLocalFile();
        } else {
            path = path.mid(7);
        }
    }
    if (path.length() >= 3 && path[0] == '/' && path[1].isLetter() && path[2] == ':') {
        path = path.mid(1);
    }
    return QDir::toNativeSeparators(path);
}

static std::string cleanStr(const std::string& s) {
    std::string temp;
    temp.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (static_cast<unsigned char>(s[i]) == 0xC2 && i + 1 < s.size() && static_cast<unsigned char>(s[i+1]) == 0xA0) {
            temp += ' ';
            i++;
        } else if (static_cast<unsigned char>(s[i]) == 0xA0) {
            temp += ' ';
        } else {
            temp += s[i];
        }
    }
    size_t first = temp.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = temp.find_last_not_of(" \t\r\n");
    std::string sub = temp.substr(first, last - first + 1);
    if (sub == "none" || sub == "null" || sub == "None" || sub == "NULL") return "";
    return sub;
}

static std::string getVal(const std::map<std::string, std::string>& m, const std::string& key, const std::string& def = "") {
    auto it = m.find(key);
    if (it != m.end()) return it->second;
    return def;
}

static double parseDouble(const std::string& s, double def = 0.0) {
    std::string clean = cleanStr(s);
    if (clean.empty()) return def;
    std::string num;
    for (char c : clean) {
        if (c != ',' && c != '%') num += c;
    }
    try {
        return std::stod(num);
    } catch (...) {
        return def;
    }
}

static int parseInt(const std::string& s, int def = 0) {
    std::string clean = cleanStr(s);
    if (clean.empty()) return def;
    std::string num;
    for (char c : clean) {
        if (c != ',') num += c;
    }
    try {
        return static_cast<int>(std::stod(num));
    } catch (...) {
        return def;
    }
}

static QString parseDateStr(const QString& raw) {
    QString s = raw.trimmed();
    if (s.isEmpty()) return "";
    QString firstPart = s.split(' ').first();
    QStringList parts = firstPart.split('/');
    if (parts.size() == 3) {
        int m = parts[0].toInt();
        int d = parts[1].toInt();
        int y = parts[2].toInt();
        if (y < 100) y += 2000;
        else if (y >= 1900 && y <= 1970) y += 100;

        if (m > 12 && d <= 12) {
            std::swap(m, d);
        }

        return QString("%1-%2-%3")
            .arg(y, 4, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(d, 2, 10, QChar('0'));
    }
    parts = firstPart.split('-');
    if (parts.size() == 3) {
        if (parts[0].length() == 4) {
            int y = parts[0].toInt();
            if (y >= 1900 && y <= 1970) y += 100;
            return QString("%1-%2-%3").arg(y, 4, 10, QChar('0')).arg(parts[1]).arg(parts[2]);
        }
        int d = parts[0].toInt();
        int m = parts[1].toInt();
        int y = parts[2].toInt();
        if (y < 100) y += 2000;
        else if (y >= 1900 && y <= 1970) y += 100;
        return QString("%1-%2-%3")
            .arg(y, 4, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(d, 2, 10, QChar('0'));
    }
    return firstPart;
}

static QString computeFy(const QString& isoDate) {
    if (isoDate.length() < 7) {
        QDate cur = QDate::currentDate();
        int curYr = (cur.month() >= 4) ? cur.year() : (cur.year() - 1);
        return QString("FY %1-%2").arg(curYr).arg(QString::number((curYr + 1) % 100).rightJustified(2, '0'));
    }
    int year = isoDate.left(4).toInt();
    int month = isoDate.mid(5, 2).toInt();
    if (month >= 4) {
        return QString("FY %1-%2").arg(year).arg(QString::number((year + 1) % 100).rightJustified(2, '0'));
    } else {
        return QString("FY %1-%2").arg(year - 1).arg(QString::number(year % 100).rightJustified(2, '0'));
    }
}

#if HAS_LIBMDB
static std::vector<std::map<std::string, std::string>> readTable(MdbHandle* mdb, const char* tableName) {
    std::vector<std::map<std::string, std::string>> result;
    if (!mdb || !tableName) return result;

    MdbTableDef* table = mdb_read_table_by_name(mdb, const_cast<char*>(tableName), MDB_TABLE);
    if (!table) return result;

    mdb_read_columns(table);
    if (!table->columns || table->num_cols == 0) {
        mdb_free_tabledef(table);
        return result;
    }

    unsigned int numCols = table->num_cols;
    std::vector<std::string> colNames(numCols);
    const size_t bufSize = MDB_BIND_SIZE + 512;
    std::vector<std::vector<char>> colBuffers(numCols, std::vector<char>(bufSize, 0));

    for (unsigned int j = 0; j < numCols; j++) {
        MdbColumn* col = static_cast<MdbColumn*>(g_ptr_array_index(table->columns, j));
        if (col && col->name[0] != '\0') {
            colNames[j] = col->name;
            mdb_bind_column(table, j + 1, colBuffers[j].data(), nullptr);
        } else {
            colNames[j] = "";
        }
    }

    mdb_rewind_table(table);
    while (mdb_fetch_row(table)) {
        std::map<std::string, std::string> row;
        for (unsigned int j = 0; j < numCols; j++) {
            if (!colNames[j].empty()) {
                row[colNames[j]] = colBuffers[j].data();
            }
        }
        result.push_back(std::move(row));
    }

    mdb_free_tabledef(table);
    return result;
}
#endif

BusyDataMigrator::BusyDataMigrator(QObject* parent) : QObject(parent) {}
BusyDataMigrator::~BusyDataMigrator() = default;

QString BusyDataMigrator::choose_busy_directory(const QString& startDir) {
    QString dir = QFileDialog::getExistingDirectory(nullptr, "Select Busy Data Directory (Contains DATA/ or db.bds)", startDir);
    if (dir.isEmpty()) {
        QString filter = "Busy Database Files (db*.bds *.bds *.mdb *.DB);;All Files (*.*)";
        return QFileDialog::getOpenFileName(nullptr, "Select Busy Database File (db.bds or db*.bds)", startDir, filter);
    }
    return dir;
}

void BusyDataMigrator::updateProgress(int percent, const QString& status) {
    m_progressPercent = percent;
    m_statusText = status;
    emit progressChanged(percent);
    emit statusChanged(status);
    emit migrationProgress(percent, status);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

QString BusyDataMigrator::parseMdbDate(const QString& rawDate) {
    return parseDateStr(rawDate);
}

QString BusyDataMigrator::determineFinancialYear(const QString& isoDate) {
    return computeFy(isoDate);
}

QVariantMap BusyDataMigrator::inspect_busy_data(const QString& busyPath) {
    QVariantMap report;
    report["success"] = false;
    report["valid"] = false;

    QString cleanP = normalizePath(busyPath);
    QFileInfo fi(cleanP);
    if (!fi.exists()) {
        fi.setFile(QDir::current().filePath(cleanP));
        cleanP = fi.absoluteFilePath();
    }

    QString dataDir;
    QString mainDbFile;
    QStringList yearDbFiles;

    if (fi.isDir()) {
        if (QDir(cleanP).exists("DATA")) {
            dataDir = QDir(cleanP).filePath("DATA");
        } else if (QDir(cleanP).exists("data")) {
            dataDir = QDir(cleanP).filePath("data");
        } else {
            dataDir = cleanP;
        }
    } else if (fi.isFile()) {
        dataDir = fi.absolutePath();
        if (fi.fileName().compare("db.bds", Qt::CaseInsensitive) == 0) {
            mainDbFile = fi.absoluteFilePath();
        } else if (fi.fileName().endsWith(".bds", Qt::CaseInsensitive)) {
            yearDbFiles << fi.absoluteFilePath();
        }
    }

    QDir dir(dataDir);
    if (mainDbFile.isEmpty()) {
        if (dir.exists("db.bds")) {
            mainDbFile = dir.filePath("db.bds");
        } else if (dir.exists("DB.BDS")) {
            mainDbFile = dir.filePath("DB.BDS");
        }
    }

    QStringList bdsFiles = dir.entryList(QStringList() << "db*.bds" << "DB*.BDS", QDir::Files, QDir::Name);
    for (const auto& bf : bdsFiles) {
        if (!bf.startsWith("db.bds", Qt::CaseInsensitive)) {
            QString fullYp = dir.filePath(bf);
            if (!yearDbFiles.contains(fullYp)) {
                yearDbFiles << fullYp;
            }
        }
    }

    if (mainDbFile.isEmpty() && yearDbFiles.isEmpty()) {
        report["message"] = "No Busy database files (db.bds / db*.bds) found in path: " + cleanP;
        report["error"] = report["message"];
        return report;
    }

#if HAS_LIBMDB
    MdbHandle* mainMdb = nullptr;
    if (!mainDbFile.isEmpty()) {
        mainMdb = mdb_open(mainDbFile.toUtf8().constData(), MDB_NOFLAGS);
    }

    QString companyName = "Busy Accounting Company";
    QString gstin = "";
    QString pan = "";
    QString address = "";
    QString begFy = "";

    if (mainMdb) {
        auto compRows = readTable(mainMdb, "Company");
        if (!compRows.empty()) {
            const auto& c = compRows.front();
            companyName = QString::fromStdString(cleanStr(c.at("Name")));
            gstin = QString::fromStdString(cleanStr(c.at("GSTNo")));
            pan = QString::fromStdString(cleanStr(c.at("ITPAN")));
            QString a1 = QString::fromStdString(cleanStr(c.at("Address1")));
            QString a2 = QString::fromStdString(cleanStr(c.at("Address2")));
            QString a3 = QString::fromStdString(cleanStr(c.at("Address3")));
            QStringList addrs;
            if (!a1.isEmpty()) addrs << a1;
            if (!a2.isEmpty()) addrs << a2;
            if (!a3.isEmpty()) addrs << a3;
            address = addrs.join(", ");
            begFy = parseDateStr(QString::fromStdString(c.at("BegFY")));
        }
        mdb_close(mainMdb);
    }

    int totalGroups = 0;
    int totalAccounts = 0;
    int totalItems = 0;
    int totalUnits = 0;
    int totalVouchers = 0;
    int totalTransactions = 0;
    int totalMandi = 0;
    double totalDr = 0.0;
    double totalCr = 0.0;

    QString activeYearDb = !yearDbFiles.isEmpty() ? yearDbFiles.last() : mainDbFile;
    if (!activeYearDb.isEmpty()) {
        MdbHandle* yrMdb = mdb_open(activeYearDb.toUtf8().constData(), MDB_NOFLAGS);
        if (yrMdb) {
            auto m1Rows = readTable(yrMdb, "Master1");
            for (const auto& r : m1Rows) {
                int mt = parseInt(r.at("MasterType"));
                if (mt == 1) totalGroups++;
                else if (mt == 2) totalAccounts++;
                else if (mt == 3 || mt == 5) totalItems++;
                else if (mt == 8 || mt == 16) totalUnits++;
            }

            auto t1Rows = readTable(yrMdb, "Tran1");
            totalVouchers = static_cast<int>(t1Rows.size());

            auto t2Rows = readTable(yrMdb, "Tran2");
            totalTransactions = static_cast<int>(t2Rows.size());
            for (const auto& r : t2Rows) {
                double amt = parseDouble(r.at("Amount"));
                int drcr = parseInt(r.at("Type"));
                if (drcr == 1) totalDr += amt;
                else if (drcr == 2) totalCr += amt;
            }

            auto mandiRows = readTable(yrMdb, "MandiVchItemDet");
            totalMandi = static_cast<int>(mandiRows.size());

            mdb_close(yrMdb);
        }
    }

    report["success"] = true;
    report["valid"] = true;
    report["sourceType"] = "Busy";
    report["company_name"] = companyName;
    report["companyName"] = companyName;
    report["gstin"] = gstin;
    report["pan"] = pan;
    report["address"] = address;
    report["financial_year"] = !begFy.isEmpty() ? computeFy(begFy) : "FY 2026-27";
    report["financialYear"] = report["financial_year"];
    report["account_groups_count"] = totalGroups;
    report["groupsCount"] = totalGroups;
    report["accounts_count"] = totalAccounts;
    report["accountsCount"] = totalAccounts;
    report["stock_items_count"] = totalItems;
    report["itemsCount"] = totalItems;
    report["stock_units_count"] = totalUnits;
    report["unitsCount"] = totalUnits;
    report["vouchers_count"] = totalVouchers;
    report["totalVouchersCount"] = totalVouchers;
    report["transactions_count"] = totalTransactions;
    report["glTransactionsCount"] = totalTransactions;
    report["mandi_records_count"] = totalMandi;
    report["debitSum"] = totalDr;
    report["creditSum"] = totalCr;
    report["glDiscrepancy"] = std::abs(totalDr - totalCr);
    report["isBalanced"] = (std::abs(totalDr - totalCr) < 0.01);
    report["data_dir"] = dataDir;
    report["main_db"] = mainDbFile;
    report["year_dbs"] = yearDbFiles;
#endif

    return report;
}

bool BusyDataMigrator::migrate_busy_data(const QString& busyPath) {
    m_isMigrating = true;
    emit migratingChanged();
    updateProgress(5, "Scanning Busy directory and database files...");

    QVariantMap insp = inspect_busy_data(busyPath);
    if (!insp.value("success").toBool()) {
        m_isMigrating = false;
        emit migratingChanged();
        emit migrationFinished(false, insp.value("message").toString());
        return false;
    }

    QString mainDbFile = insp.value("main_db").toString();
    QStringList yearDbFiles = insp.value("year_dbs").toStringList();
    if (mainDbFile.isEmpty() && yearDbFiles.isEmpty()) {
        m_isMigrating = false;
        emit migratingChanged();
        emit migrationFinished(false, "No valid Busy database files found to migrate.");
        return false;
    }

#if HAS_LIBMDB
    DatabaseManager& db = DatabaseManager::instance();
    db.beginTransaction();

    BusyMigrationStats stats;
    stats.companyName = insp.value("company_name").toString();
    stats.gstin = insp.value("gstin").toString();
    stats.financialYear = insp.value("financial_year").toString();

    // ========================================================
    // STEP 1: COMPANY PROFILE & FINANCIAL YEARS (15%)
    // ========================================================
    updateProgress(15, "Migrating Company Profile & Financial Years...");
    MdbHandle* mainMdb = !mainDbFile.isEmpty() ? mdb_open(mainDbFile.toUtf8().constData(), MDB_NOFLAGS) : nullptr;
    if (mainMdb) {
        auto compRows = readTable(mainMdb, "Company");
        if (!compRows.empty()) {
            const auto& c = compRows.front();
            QString name = QString::fromStdString(cleanStr(getVal(c, "Name")));
            QString printName = QString::fromStdString(cleanStr(getVal(c, "PrintName", name.toStdString())));
            if (printName.isEmpty()) printName = name;
            QString gstin = QString::fromStdString(cleanStr(getVal(c, "GSTNo")));
            QString pan = QString::fromStdString(cleanStr(getVal(c, "ITPAN")));
            if (pan.isEmpty() && gstin.length() == 15) pan = gstin.mid(2, 10);
            QString phone = QString::fromStdString(cleanStr(getVal(c, "TelNo")));
            QString email = QString::fromStdString(cleanStr(getVal(c, "Email")));

            QString a1 = QString::fromStdString(cleanStr(getVal(c, "Address1")));
            QString a2 = QString::fromStdString(cleanStr(getVal(c, "Address2")));
            QString a3 = QString::fromStdString(cleanStr(getVal(c, "Address3")));
            QStringList addrs;
            if (!a1.isEmpty()) addrs << a1;
            if (!a2.isEmpty()) addrs << a2;
            if (!a3.isEmpty()) addrs << a3;
            QString fullAddr = addrs.join(", ");

            QString state = "Haryana";
            QString stateCode = "06";
            if (gstin.length() >= 2 && gstin.left(2).toInt() > 0) {
                stateCode = gstin.left(2);
                if (stateCode == "06") state = "Haryana";
                else if (stateCode == "03") state = "Punjab";
                else if (stateCode == "07") state = "Delhi";
                else if (stateCode == "08") state = "Rajasthan";
                else if (stateCode == "09") state = "Uttar Pradesh";
            }

            db.executeNonQuery(
                "INSERT OR REPLACE INTO company_info (id, company_name, address, gstin, pan_no, phone, email, state, state_code) "
                "VALUES (1, ?, ?, ?, ?, ?, ?, ?, ?);",
                {name, fullAddr, gstin, pan, phone, email, state, stateCode}
            );

            QString begFy = parseDateStr(QString::fromStdString(getVal(c, "BegFY")));
            if (!begFy.isEmpty()) {
                QString fyName = computeFy(begFy);
                int sYr = begFy.left(4).toInt();
                QString sDate = QString("%1-04-01").arg(sYr);
                QString eDate = QString("%1-03-31").arg(sYr + 1);
                db.executeNonQuery(
                    "INSERT OR IGNORE INTO financial_years (year_name, start_date, end_date, is_active) VALUES (?, ?, ?, 1);",
                    {fyName, sDate, eDate}
                );
            }
        }
        mdb_close(mainMdb);
    }

    // ========================================================
    // STEP 2: ACCOUNT GROUPS HIERARCHY (30%)
    // ========================================================
    updateProgress(30, "Migrating Account Groups Hierarchy...");
    QString activeYearDb = !yearDbFiles.isEmpty() ? yearDbFiles.last() : mainDbFile;
    MdbHandle* yrMdb = !activeYearDb.isEmpty() ? mdb_open(activeYearDb.toUtf8().constData(), MDB_NOFLAGS) : nullptr;

    std::map<int, std::map<std::string, std::string>> busyMasterMap;
    std::map<int, std::string> groupNameMap;

    if (yrMdb) {
        auto m1Rows = readTable(yrMdb, "Master1");
        for (const auto& r : m1Rows) {
            int code = parseInt(getVal(r, "Code"));
            busyMasterMap[code] = r;
            if (parseInt(getVal(r, "MasterType")) == 1) {
                groupNameMap[code] = cleanStr(getVal(r, "Name"));
            }
        }

        // Insert standard groups
        for (const auto& kv : groupNameMap) {
            int code = kv.first;
            std::string name = kv.second;
            int parentCode = parseInt(getVal(busyMasterMap[code], "ParentGrp"));
            std::string parentName = groupNameMap.count(parentCode) ? groupNameMap[parentCode] : "Primary";

            db.executeNonQuery(
                "INSERT OR REPLACE INTO account_groups (id, name, parent_group_name, nature, is_system) "
                "VALUES (?, ?, ?, ?, ?);",
                {code, QString::fromStdString(name), QString::fromStdString(parentName), "Assets", parentCode == 0 ? 1 : 0}
            );
            stats.totalGroups++;
        }

        // ========================================================
        // STEP 3: PARTIES & LEDGERS (50%)
        // ========================================================
        updateProgress(50, "Migrating Parties & General Ledgers with Addresses...");
        auto addrRows = readTable(yrMdb, "MasterAddressInfo");
        std::map<int, std::map<std::string, std::string>> addrMap;
        for (const auto& ar : addrRows) {
            int mCode = parseInt(getVal(ar, "MasterCode"));
            addrMap[mCode] = ar;
        }

        auto folioRows = readTable(yrMdb, "Folio1");
        std::map<int, std::pair<double, std::string>> folioBalMap;
        for (const auto& fr : folioRows) {
            int mCode = parseInt(getVal(fr, "MasterCode"));
            double opBal = parseDouble(getVal(fr, "D1"));
            int bType = parseInt(getVal(fr, "B1"));
            folioBalMap[mCode] = {opBal, bType == 1 ? "Dr" : "Cr"};
        }

        for (const auto& kv : busyMasterMap) {
            const auto& r = kv.second;
            int mt = parseInt(getVal(r, "MasterType"));
            if (mt != 2) continue; // Only Ledgers / Accounts

            int code = kv.first;
            QString name = QString::fromStdString(cleanStr(getVal(r, "Name")));
            if (name.isEmpty()) continue;

            QString alias = QString::fromStdString(cleanStr(getVal(r, "Alias")));
            QString printName = QString::fromStdString(cleanStr(getVal(r, "PrintName", name.toStdString())));
            int parentCode = parseInt(getVal(r, "ParentGrp"));
            QString groupName = QString::fromStdString(groupNameMap.count(parentCode) ? groupNameMap[parentCode] : "Sundry Debtors");

            QString address = "";
            QString city = "";
            QString state = "Haryana";
            QString stateCode = "06";
            QString pincode = "125055";
            QString gstin = "";
            QString pan = "";
            QString mobile = "";
            QString email = "";
            QString bankName = "";
            QString bankAc = "";
            QString ifsc = "";

            if (addrMap.count(code)) {
                const auto& a = addrMap[code];
                QString a1 = QString::fromStdString(cleanStr(getVal(a, "Address1")));
                QString a2 = QString::fromStdString(cleanStr(getVal(a, "Address2")));
                QString a3 = QString::fromStdString(cleanStr(getVal(a, "Address3")));
                QString a4 = QString::fromStdString(cleanStr(getVal(a, "Address4")));
                QStringList al;
                if (!a1.isEmpty()) al << a1;
                if (!a2.isEmpty()) al << a2;
                if (!a3.isEmpty()) al << a3;
                if (!a4.isEmpty()) al << a4;
                address = al.join(", ");
                city = QString::fromStdString(cleanStr(getVal(a, "City")));
                gstin = QString::fromStdString(cleanStr(getVal(a, "GSTNo"))).toUpper();
                pan = QString::fromStdString(cleanStr(getVal(a, "ITPAN"))).toUpper();
                if (pan.isEmpty() && gstin.length() == 15) pan = gstin.mid(2, 10);
                mobile = QString::fromStdString(cleanStr(getVal(a, "MobileNo")));
                email = QString::fromStdString(cleanStr(getVal(a, "Email")));
                pincode = QString::fromStdString(cleanStr(getVal(a, "PinCode")));
                if (pincode.isEmpty()) pincode = "125055";
                bankName = QString::fromStdString(cleanStr(getVal(a, "BankName")));
                bankAc = QString::fromStdString(cleanStr(getVal(a, "BankAcNo")));
                ifsc = QString::fromStdString(cleanStr(getVal(a, "IFSCCode")));
            }

            double opBal = 0.0;
            QString balType = "Dr";
            if (folioBalMap.count(code)) {
                opBal = folioBalMap[code].first;
                balType = QString::fromStdString(folioBalMap[code].second);
            } else {
                opBal = parseDouble(getVal(r, "D1"));
                balType = (parseInt(getVal(r, "I2")) == 1) ? "Cr" : "Dr";
            }

            double crDays = parseDouble(getVal(r, "D6"), 30.0);
            double crLimit = parseDouble(getVal(r, "D7"), 0.0);

            db.executeNonQuery(
                "INSERT OR REPLACE INTO parties (id, legacy_id, name, alias, mailing_name, group_name, group_id, "
                "address, city, state, state_code, pincode, gstin, pan, mobile, email, "
                "bank_name, bank_account, ifsc_code, opening_balance, balance_type, credit_days, credit_limit) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {code, code, name, alias, printName, groupName, parentCode,
                 address, city, state, stateCode, pincode, gstin, pan, mobile, email,
                 bankName, bankAc, ifsc, opBal, balType, static_cast<int>(crDays), crLimit}
            );
            stats.totalAccounts++;
        }

        // ========================================================
        // STEP 4: STOCK GROUPS, UNITS & ITEMS (70%)
        // ========================================================
        updateProgress(70, "Migrating Stock Groups, Units and Commodities...");
        for (const auto& kv : busyMasterMap) {
            const auto& r = kv.second;
            int mt = parseInt(getVal(r, "MasterType"));
            int code = kv.first;
            QString name = QString::fromStdString(cleanStr(getVal(r, "Name")));
            if (name.isEmpty()) continue;

            if (mt == 3) {
                // Item Group
                db.executeNonQuery(
                    "INSERT OR REPLACE INTO stock_groups (id, group_name, legacy_code) VALUES (?, ?, ?);",
                    {code, name, code}
                );
            } else if (mt == 8 || mt == 16) {
                // Stock Units
                db.executeNonQuery(
                    "INSERT OR REPLACE INTO stock_units (id, unit_name, legacy_code) VALUES (?, ?, ?);",
                    {code, name, code}
                );
                stats.totalStockUnits++;
            } else if (mt == 5 || mt == 6) {
                // Stock Item
                QString hsn = QString::fromStdString(cleanStr(getVal(r, "HSNCode", "1006")));
                int pGrp = parseInt(getVal(r, "ParentGrp"));
                QString grpName = QString::fromStdString(busyMasterMap.count(pGrp) ? cleanStr(getVal(busyMasterMap[pGrp], "Name")) : "Rice Commodities");
                double opQty = parseDouble(getVal(r, "D1"));
                double opRate = parseDouble(getVal(r, "D2"));
                double opVal = parseDouble(getVal(r, "D3"), opQty * opRate);
                double gstRate = parseDouble(getVal(r, "D4"), 5.0);

                db.executeNonQuery(
                    "INSERT OR REPLACE INTO stock_items (id, name, code, trading_group, group_code, hsn_code, unit, gst_rate, "
                    "opening_bags, opening_qty, opening_rate, opening_value) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                    {code, name, QString::number(code), grpName, pGrp, hsn, "Qtl.", gstRate,
                     static_cast<int>(opQty), opQty, opRate, opVal}
                );
                stats.totalStockItems++;
            }
        }

        // ========================================================
        // STEP 5: VOUCHERS, INVOICES & TRANSACTIONS (85%)
        // ========================================================
        updateProgress(85, "Migrating Double-Entry Vouchers, Sales & Purchase Invoices...");
        auto t1Rows = readTable(yrMdb, "Tran1");
        auto t2Rows = readTable(yrMdb, "Tran2");
        auto t3Rows = readTable(yrMdb, "Tran3");
        auto t4Rows = readTable(yrMdb, "Tran4");
        auto mandiRows = readTable(yrMdb, "MandiVchItemDet");

        std::map<int, std::vector<std::map<std::string, std::string>>> t2Map;
        for (const auto& tr : t2Rows) {
            int vCode = parseInt(getVal(tr, "VchCode"));
            t2Map[vCode].push_back(tr);
        }

        std::map<int, std::vector<std::map<std::string, std::string>>> t3Map;
        for (const auto& tr : t3Rows) {
            int vCode = parseInt(getVal(tr, "VchCode"));
            t3Map[vCode].push_back(tr);
        }

        for (const auto& v : t1Rows) {
            int vCode = parseInt(getVal(v, "VchCode"));
            int vType = parseInt(getVal(v, "VchType"));
            QString vNo = QString::fromStdString(cleanStr(getVal(v, "VchNo")));
            QString vDate = parseDateStr(QString::fromStdString(getVal(v, "Date")));
            double totalAmt = parseDouble(getVal(v, "TotalAmount"));
            QString narration = QString::fromStdString(cleanStr(getVal(v, "Narration")));
            int partyCode = parseInt(getVal(v, "MasterCode1"));
            QString partyName = QString::fromStdString(busyMasterMap.count(partyCode) ? cleanStr(getVal(busyMasterMap[partyCode], "Name")) : "General Account");

            QString typeStr = "Journal";
            if (vType == 1) typeStr = "Sales";
            else if (vType == 2) typeStr = "Purchase";
            else if (vType == 3) typeStr = "Payment";
            else if (vType == 4) typeStr = "Receipt";
            else if (vType == 6) typeStr = "Contra";
            else if (vType == 7) typeStr = "Debit Note";
            else if (vType == 8) typeStr = "Credit Note";

            db.executeNonQuery(
                "INSERT OR REPLACE INTO vouchers (id, voucher_no, voucher_type, voucher_date, amount, narration) "
                "VALUES (?, ?, ?, ?, ?, ?);",
                {vCode, vNo, typeStr, vDate, totalAmt, narration}
            );
            stats.totalGlVouchers++;

            // Insert Sales Invoice
            if (vType == 1) {
                QString itemName = "Rice Basmati";
                double bags = 0, weight = 0, rate = 0, taxAmt = 0;
                if (t3Map.count(vCode) && !t3Map[vCode].empty()) {
                    const auto& itemRow = t3Map[vCode].front();
                    int itCode = parseInt(getVal(itemRow, "MasterCode1"));
                    itemName = QString::fromStdString(busyMasterMap.count(itCode) ? cleanStr(getVal(busyMasterMap[itCode], "Name")) : "Rice Basmati");
                    weight = parseDouble(getVal(itemRow, "Value1"));
                    rate = parseDouble(getVal(itemRow, "Value2"));
                    taxAmt = parseDouble(getVal(itemRow, "Value3"));
                } else {
                    taxAmt = totalAmt;
                }

                db.executeNonQuery(
                    "INSERT OR REPLACE INTO sales_invoices (id, invoice_no, voucher_no, invoice_date, customer_id, customer_name, "
                    "item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, total_amount, narration) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                    {vCode, vNo, vNo, vDate, partyCode, partyName, itemName, static_cast<int>(bags), weight, rate, taxAmt, totalAmt, narration}
                );
                stats.totalSalesInvoices++;
            } else if (vType == 2) {
                // Purchase Invoice
                QString itemName = "Paddy 1509";
                double bags = 0, weight = 0, rate = 0, taxAmt = 0;
                if (t3Map.count(vCode) && !t3Map[vCode].empty()) {
                    const auto& itemRow = t3Map[vCode].front();
                    int itCode = parseInt(getVal(itemRow, "MasterCode1"));
                    itemName = QString::fromStdString(busyMasterMap.count(itCode) ? cleanStr(getVal(busyMasterMap[itCode], "Name")) : "Paddy 1509");
                    weight = parseDouble(getVal(itemRow, "Value1"));
                    rate = parseDouble(getVal(itemRow, "Value2"));
                    taxAmt = parseDouble(getVal(itemRow, "Value3"));
                } else {
                    taxAmt = totalAmt;
                }

                db.executeNonQuery(
                    "INSERT OR REPLACE INTO purchase_invoices (id, invoice_no, voucher_no, invoice_date, supplier_id, supplier_name, "
                    "item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, total_amount, narration) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                    {vCode, vNo, vNo, vDate, partyCode, partyName, itemName, static_cast<int>(bags), weight, rate, taxAmt, totalAmt, narration}
                );
                stats.totalPurchaseInvoices++;
            }

            // Insert Double-Entry Ledger Splits from Tran2
            if (t2Map.count(vCode)) {
                for (const auto& tr : t2Map[vCode]) {
                    int accCode = parseInt(getVal(tr, "MasterCode1"));
                    QString accName = QString::fromStdString(busyMasterMap.count(accCode) ? cleanStr(getVal(busyMasterMap[accCode], "Name")) : "General Ledger");
                    int recType = parseInt(getVal(tr, "RecType"));
                    double d1 = parseDouble(getVal(tr, "D1"));
                    double d2 = parseDouble(getVal(tr, "D2"));
                    QString drCr = (recType == 1 || d1 > 0.001) ? "Dr" : "Cr";
                    double amt = (drCr == "Dr") ? (d1 > 0.001 ? d1 : d2) : (d2 > 0.001 ? d2 : d1);

                    db.executeNonQuery(
                        "INSERT INTO transactions (voucher_no, voucher_type, voucher_date, party_id, party_name, dr_cr, amount, narration) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?);",
                        {vNo, typeStr, vDate, accCode, accName, drCr, amt, narration}
                    );
                    stats.totalGlTransactions++;
                    if (drCr == "Dr") stats.totalDebitSum += amt;
                    else stats.totalCreditSum += amt;
                }
            }
        }

        // ========================================================
        // STEP 6: MANDI & PADDY PROCUREMENT (95%)
        // ========================================================
        updateProgress(95, "Migrating Mandi Procurement & Gunny Bag records...");
        for (const auto& mr : mandiRows) {
            int vCode = parseInt(getVal(mr, "VchCode"));
            int itCode = parseInt(getVal(mr, "ItemCode"));
            int pCode = parseInt(getVal(mr, "PartyCode"));
            QString itName = QString::fromStdString(busyMasterMap.count(itCode) ? cleanStr(getVal(busyMasterMap[itCode], "Name")) : "Paddy Mandi");
            QString pName = QString::fromStdString(busyMasterMap.count(pCode) ? cleanStr(getVal(busyMasterMap[pCode], "Name")) : "Farmer / Commission Agent");
            QString vNo = QString::fromStdString(cleanStr(getVal(mr, "VchNo")));
            QString vDate = parseDateStr(QString::fromStdString(getVal(mr, "Date")));
            double bagQty = parseDouble(getVal(mr, "BagQty"));
            double looseWt = parseDouble(getVal(mr, "LooseWeight"));
            double buyerWt = parseDouble(getVal(mr, "BuyerWeight"), looseWt);

            db.executeNonQuery(
                "INSERT OR REPLACE INTO paddy_procurement (receipt_no, arrival_date, farmer_id, farmer_name, variety, bag_count, gross_weight_qtl, net_weight_qtl, rate_per_qtl, total_amount) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {vNo, vDate, pCode, pName, itName, static_cast<int>(bagQty), looseWt, buyerWt, 0.0, 0.0}
            );
            stats.totalMandiRecords++;
        }

        mdb_close(yrMdb);
    }

    db.commit();

    stats.glDiscrepancy = std::abs(stats.totalDebitSum - stats.totalCreditSum);
    stats.isBalanced = (stats.glDiscrepancy < 0.01);

    updateProgress(100, "Migration Completed Successfully!");
    m_isMigrating = false;
    emit migratingChanged();

    QString summaryMsg = QString(
        "Successfully migrated Busy Accounting data for '%1' (%2):\n"
        "• Account Groups: %3\n"
        "• Parties & Ledgers: %4\n"
        "• Stock Commodities & Units: %5\n"
        "• Sales & Purchase Invoices: %6\n"
        "• GL Vouchers & Transactions: %7\n"
        "• Mandi Procurement Records: %8\n"
        "• Double-Entry Balance: %9 (Discrepancy: ₹%10)"
    )
    .arg(stats.companyName, stats.financialYear)
    .arg(stats.totalGroups)
    .arg(stats.totalAccounts)
    .arg(stats.totalStockItems + stats.totalStockUnits)
    .arg(stats.totalSalesInvoices + stats.totalPurchaseInvoices)
    .arg(stats.totalGlTransactions)
    .arg(stats.totalMandiRecords)
    .arg(stats.isBalanced ? "PERFECTLY BALANCED (Dr == Cr)" : "Unbalanced")
    .arg(QString::number(stats.glDiscrepancy, 'f', 2));

    emit migrationFinished(true, summaryMsg);
    return true;
#else
    m_isMigrating = false;
    emit migratingChanged();
    emit migrationFinished(false, "libmdb is not enabled in this build.");
    return false;
#endif
}

} // namespace MahadevERP
