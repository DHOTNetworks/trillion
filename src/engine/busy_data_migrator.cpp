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
    for (const auto& kv : m) {
        if (QString::compare(QString::fromStdString(kv.first), QString::fromStdString(key), Qt::CaseInsensitive) == 0) {
            return kv.second;
        }
    }
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

struct BusyGroupMeta {
    int code1st = 0;
    int code2nd = 0;
    QString nature = "Assets";
    int extractBs = 1;
};

static BusyGroupMeta mapBusyGroup(int code, const std::string& name, int parentCode, const std::map<int, std::string>& groupNameMap, const std::map<int, std::map<std::string, std::string>>& busyMasterMap, int depth = 0) {
    if (depth > 12) {
        return {0, 0, "Assets", 1};
    }
    QString qName = QString::fromStdString(name).trimmed().toLower();

    // Standard Busy MasterType=1 Account Groups
    switch (code) {
        case 101: return {1, 0, "Liabilities", 1};  // Capital Account
        case 102: return {2, 0, "Assets", 1};       // Current Assets
        case 103: return {9, 0, "Liabilities", 1};  // Current Liabilities
        case 104: return {14, 0, "Assets", 1};      // Fixed Assets
        case 105: return {2, 0, "Assets", 1};       // Investments
        case 106: return {13, 0, "Liabilities", 1}; // Loans (Liability)
        case 107: return {17, 15, "Expense", 0};    // Pre-Operative Expenses
        case 108: return {15, 0, "Liabilities", 1}; // Profit & Loss
        case 109: return {18, 0, "Expense", 0};     // Revenue Accounts
        case 110: return {25, 0, "Liabilities", 1}; // Suspense Account
        case 111: return {4, 2, "Assets", 1};       // Cash-in-hand
        case 112: return {3, 2, "Assets", 1};       // Bank Accounts
        case 113: return {26, 2, "Assets", 1};      // Securities & Deposits (Asset)
        case 114: return {6, 2, "Assets", 1};       // Loans & Advances (Asset)
        case 115: return {7, 2, "Assets", 0};       // Stock-in-hand
        case 116: return {8, 2, "Assets", 1};       // Sundry Debtors
        case 117: return {11, 9, "Liabilities", 1}; // Sundry Creditors
        case 118: return {10, 9, "Liabilities", 1}; // Duties & Taxes
        case 119: return {12, 9, "Liabilities", 1}; // Provisions/Expenses Payable
        case 120: return {28, 13, "Liabilities", 1};// Secured Loans
        case 121: return {29, 13, "Liabilities", 1};// Unsecured Loans
        case 122: return {19, 18, "Expense", 0};    // Purchase
        case 123: return {20, 18, "Income", 0};     // Sale
        case 124: return {23, 22, "Expense", 0};    // Expenses (Direct/Mfg.)
        case 125: return {17, 15, "Expense", 0};    // Expenses (Indirect/Admn.)
        case 126: return {16, 15, "Income", 0};     // Income (Direct/Opr.)
        case 127: return {16, 15, "Income", 0};     // Income (Indirect)
        case 128: return {28, 13, "Liabilities", 1};// Bank O/D Account
        case 129: return {1, 0, "Liabilities", 1};  // Reserves & Surplus
    }

    // Name-based classification matching
    if (qName.contains("sale") || qName.contains("selling") || qName.contains("revenue")) {
        return {20, 18, "Income", 0};
    }
    if (qName.contains("purchas") || qName.contains("procurement")) {
        return {19, 18, "Expense", 0};
    }
    if (qName.contains("direct exp") || qName.contains("mfg") || qName.contains("manufacturing") || qName.contains("freight inward") || qName.contains("labour")) {
        return {23, 22, "Expense", 0};
    }
    if (qName.contains("indirect exp") || qName.contains("expens") || qName.contains("expenditure") || qName.contains("administrative") || qName.contains("office")) {
        return {17, 15, "Expense", 0};
    }
    if (qName.contains("indirect inc") || qName.contains("income") || qName.contains("interest received") || qName.contains("discount received")) {
        return {16, 15, "Income", 0};
    }
    if (qName.contains("debtor") || qName.contains("customer") || qName.contains("receivable")) {
        return {8, 2, "Assets", 1};
    }
    if (qName.contains("creditor") || qName.contains("supplier") || qName.contains("vendor") || qName.contains("payable")) {
        return {11, 9, "Liabilities", 1};
    }
    if (qName.contains("bank") || qName.contains("saving") || qName.contains("current ac")) {
        return {3, 2, "Assets", 1};
    }
    if (qName.contains("cash")) {
        return {4, 2, "Assets", 1};
    }
    if (qName.contains("tax") || qName.contains("gst") || qName.contains("duty") || qName.contains("duties") || qName.contains("tds") || qName.contains("tcs")) {
        return {10, 9, "Liabilities", 1};
    }
    if (qName.contains("capital") || qName.contains("partner") || qName.contains("proprietor")) {
        return {1, 0, "Liabilities", 1};
    }
    if (qName.contains("loan") || qName.contains("borrowing")) {
        return {13, 9, "Liabilities", 1};
    }
    if (qName.contains("fixed asset") || qName.contains("machinery") || qName.contains("building") || qName.contains("vehicle") || qName.contains("computer")) {
        return {14, 0, "Assets", 1};
    }

    // Inherit from parent group if available
    if (parentCode > 0 && busyMasterMap.count(parentCode) && parentCode != code) {
        auto itGrp = groupNameMap.find(parentCode);
        std::string pName = (itGrp != groupNameMap.end()) ? itGrp->second : "";
        auto itMaster = busyMasterMap.find(parentCode);
        int grandParent = parseInt(getVal(itMaster->second, "ParentGrp"));
        return mapBusyGroup(parentCode, pName, grandParent, groupNameMap, busyMasterMap, depth + 1);
    }

    return {0, 0, "Assets", 1};
}

#if HAS_LIBMDB
static std::vector<std::map<std::string, std::string>> readTable(MdbHandle* mdb, const char* tableName) {
    std::vector<std::map<std::string, std::string>> result;
    if (!mdb || !tableName) return result;

    try {
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
    } catch (...) {
        // Suppress any unexpected read errors
    }
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
        } else {
            mainDbFile = fi.absoluteFilePath();
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
    QString companyName = "Busy Accounting Company";
    QString gstin = "";
    QString pan = "";
    QString address = "";
    QString begFy = "";

    try {
        MdbHandle* mainMdb = nullptr;
        if (!mainDbFile.isEmpty()) {
            mainMdb = mdb_open(mainDbFile.toUtf8().constData(), MDB_NOFLAGS);
        }

        if (mainMdb) {
            auto compRows = readTable(mainMdb, "Company");
            if (!compRows.empty()) {
                const auto& c = compRows.front();
                companyName = QString::fromStdString(cleanStr(getVal(c, "Name")));
                gstin = QString::fromStdString(cleanStr(getVal(c, "GSTNo", getVal(c, "GSTIN"))));
                pan = QString::fromStdString(cleanStr(getVal(c, "ITPAN", getVal(c, "PAN"))));
                QString a1 = QString::fromStdString(cleanStr(getVal(c, "Address1")));
                QString a2 = QString::fromStdString(cleanStr(getVal(c, "Address2")));
                QString a3 = QString::fromStdString(cleanStr(getVal(c, "Address3")));
                QStringList addrs;
                if (!a1.isEmpty()) addrs << a1;
                if (!a2.isEmpty()) addrs << a2;
                if (!a3.isEmpty()) addrs << a3;
                address = addrs.join(", ");
                begFy = parseDateStr(QString::fromStdString(getVal(c, "BegFY")));
            }
            mdb_close(mainMdb);
        }
    } catch (...) {
        // Suppress and continue
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
        try {
            MdbHandle* yrMdb = mdb_open(activeYearDb.toUtf8().constData(), MDB_NOFLAGS);
            if (yrMdb) {
                if (companyName == "Busy Accounting Company" || companyName.isEmpty()) {
                    auto compRows = readTable(yrMdb, "Company");
                    if (!compRows.empty()) {
                        const auto& c = compRows.front();
                        QString cn = QString::fromStdString(cleanStr(getVal(c, "Name")));
                        if (!cn.isEmpty()) companyName = cn;
                        if (gstin.isEmpty()) gstin = QString::fromStdString(cleanStr(getVal(c, "GSTNo", getVal(c, "GSTIN"))));
                        if (pan.isEmpty()) pan = QString::fromStdString(cleanStr(getVal(c, "ITPAN", getVal(c, "PAN"))));
                        if (begFy.isEmpty()) begFy = parseDateStr(QString::fromStdString(getVal(c, "BegFY")));
                    }
                }

                auto m1Rows = readTable(yrMdb, "Master1");
                for (const auto& r : m1Rows) {
                    int mt = parseInt(getVal(r, "MasterType"));
                    if (mt == 1) totalGroups++;
                    else if (mt == 2) totalAccounts++;
                    else if (mt == 6 || (mt == 5 && parseInt(getVal(r, "ParentGrp")) > 0)) totalItems++;
                    else if (mt == 8 || mt == 16) totalUnits++;
                }

                auto t1Rows = readTable(yrMdb, "Tran1");
                totalVouchers = static_cast<int>(t1Rows.size());

                auto t2Rows = readTable(yrMdb, "Tran2");
                for (const auto& r : t2Rows) {
                    int rType = parseInt(getVal(r, "RecType"));
                    if (rType == 1) {
                        totalTransactions++;
                        double val1 = parseDouble(getVal(r, "Value1"));
                        double d1 = parseDouble(getVal(r, "D1"));
                        double d2 = parseDouble(getVal(r, "D2"));
                        if (std::abs(val1) > 0.001) {
                            double a = std::abs(val1);
                            if (val1 < 0) totalDr += a;
                            else totalCr += a;
                        } else {
                            if (d1 > 0.001) totalDr += d1;
                            else if (d2 > 0.001) totalCr += d2;
                            else {
                                double amt = std::abs(parseDouble(getVal(r, "Amount")));
                                if (parseInt(getVal(r, "Type")) == 1) totalDr += amt;
                                else totalCr += amt;
                            }
                        }
                    }
                }

                auto mandiRows = readTable(yrMdb, "MandiVchItemDet");
                totalMandi = static_cast<int>(mandiRows.size());

                mdb_close(yrMdb);
            }
        } catch (...) {
            // Suppress and continue
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

    // Clear initial database tables before migrating so default seed placeholders do not duplicate
    db.executeNonQuery("DELETE FROM transactions;");
    db.executeNonQuery("DELETE FROM vouchers;");
    db.executeNonQuery("DELETE FROM sales_invoices;");
    db.executeNonQuery("DELETE FROM sales_invoice_items;");
    db.executeNonQuery("DELETE FROM purchase_invoices;");
    db.executeNonQuery("DELETE FROM purchase_invoice_items;");
    db.executeNonQuery("DELETE FROM stock_transactions;");
    db.executeNonQuery("DELETE FROM inventory;");
    db.executeNonQuery("DELETE FROM stock_items;");
    db.executeNonQuery("DELETE FROM stock_groups;");
    db.executeNonQuery("DELETE FROM stock_units;");
    db.executeNonQuery("DELETE FROM parties;");
    db.executeNonQuery("DELETE FROM account_groups;");
    db.executeNonQuery("DELETE FROM financial_years;");
    db.executeNonQuery("DELETE FROM company_info;");
    AccountClassifier::invalidateCache();

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
            BusyGroupMeta meta = mapBusyGroup(code, name, parentCode, groupNameMap, busyMasterMap);

            db.executeNonQuery(
                "INSERT OR REPLACE INTO account_groups (id, name, parent_group_name, nature, extract_in_balance_sheet, code1st, code2nd, is_system) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?);",
                {code, QString::fromStdString(name), QString::fromStdString(parentName), meta.nature, meta.extractBs, meta.code1st, meta.code2nd, parentCode == 0 ? 1 : 0}
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
            std::string pGroupName = groupNameMap.count(parentCode) ? groupNameMap[parentCode] : "Sundry Debtors";
            QString groupName = QString::fromStdString(pGroupName);
            BusyGroupMeta grpMeta = mapBusyGroup(parentCode, pGroupName, 0, groupNameMap, busyMasterMap);

            QString partyType = "General";
            if (grpMeta.code1st == 8) partyType = "Buyer";
            else if (grpMeta.code1st == 11) partyType = "Vendor";
            else if (grpMeta.code1st == 3) partyType = "Bank";
            else if (grpMeta.code1st == 4) partyType = "Cash";
            else if (grpMeta.code1st == 10) partyType = "Duties/Taxes";
            else if (grpMeta.code1st == 19 || grpMeta.code1st == 20 || grpMeta.code1st == 23 || grpMeta.code1st == 17 || grpMeta.code1st == 16) partyType = "Nominal";

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
                "INSERT OR REPLACE INTO parties (id, legacy_id, name, alias, mailing_name, group_name, group_id, group_code, party_type, calc_direct_expense, "
                "address, city, state, state_code, pincode, gstin, pan, mobile, email, "
                "bank_name, bank_account, ifsc_code, opening_balance, balance_type, credit_days, credit_limit) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {code, code, name, alias, printName, groupName, parentCode, grpMeta.code1st, partyType, (grpMeta.code1st == 23 ? 1 : 0),
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
            QString mName = QString::fromStdString(cleanStr(getVal(r, "Name")));
            if (mName.isEmpty()) continue;

            if (mt == 5 || (mt == 3 && parseInt(getVal(r, "ParentGrp")) == 0)) {
                // Stock Group
                db.executeNonQuery(
                    "INSERT OR REPLACE INTO stock_groups (id, group_name, legacy_code) VALUES (?, ?, ?);",
                    {code, mName, code}
                );
            } else if (mt == 8 || mt == 16) {
                // Stock Units
                db.executeNonQuery(
                    "INSERT OR REPLACE INTO stock_units (id, unit_name, legacy_code) VALUES (?, ?, ?);",
                    {code, mName, code}
                );
                stats.totalStockUnits++;
            } else if (mt == 6 || (mt == 5 && parseInt(getVal(r, "ParentGrp")) > 0)) {
                // Stock Item
                QString hsn = QString::fromStdString(cleanStr(getVal(r, "HSNCode")));
                int pGrp = parseInt(getVal(r, "ParentGrp"));
                QString grpName;
                if (pGrp > 0 && busyMasterMap.count(pGrp)) {
                    grpName = QString::fromStdString(cleanStr(getVal(busyMasterMap[pGrp], "Name")));
                }

                int uCode = parseInt(getVal(r, "CM2", getVal(r, "MasterSupport")));
                QString unitName;
                if (uCode > 0 && busyMasterMap.count(uCode)) {
                    unitName = QString::fromStdString(cleanStr(getVal(busyMasterMap[uCode], "Name")));
                }

                double opQty = parseDouble(getVal(r, "D1"));
                double opRate = parseDouble(getVal(r, "D2"));
                double opVal = parseDouble(getVal(r, "D3"), opQty * opRate);
                double gstRate = parseDouble(getVal(r, "D4"), 0.0);

                db.executeNonQuery(
                    "INSERT OR REPLACE INTO stock_items (id, name, code, trading_group, group_code, hsn_code, unit, gst_rate, "
                    "opening_bags, opening_qty, opening_rate, opening_value) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                    {code, mName, QString::number(code), grpName, (pGrp > 0 ? QVariant(pGrp) : QVariant()), hsn, unitName, gstRate,
                     static_cast<int>(opQty), opQty, opRate, opVal}
                );

                db.executeNonQuery(
                    "INSERT OR REPLACE INTO inventory (item_code, item_name, category, current_stock_qtl, sale_rate, gst_rate, packing_kg) "
                    "VALUES (?, ?, ?, ?, ?, ?, 0.0);",
                    {QString::number(code), mName, grpName, opQty, opRate, gstRate}
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
            double totalAmt = parseDouble(getVal(v, "VchAmtBaseCur", getVal(v, "VchSalePurcAmt", getVal(v, "OrgVchAmtBaseCur", getVal(v, "TotalAmount")))));
            QString narration = QString::fromStdString(cleanStr(getVal(v, "Narration")));
            int partyCode = parseInt(getVal(v, "MasterCode1"));
            QString partyName = QString::fromStdString(busyMasterMap.count(partyCode) ? cleanStr(getVal(busyMasterMap[partyCode], "Name")) : "");

            QString typeStr = "Journal";
            QString rawType = "Jrnl";
            bool isSale = false;
            bool isPurchase = false;

            if (vType == 9 || vType == 1) { typeStr = "Sales"; rawType = "Sale"; isSale = true; }
            else if (vType == 10 || vType == 2) { typeStr = "Purchase"; rawType = "Purc"; isPurchase = true; }
            else if (vType == 13 || vType == 3) { typeStr = "Payment"; rawType = "Pymt"; }
            else if (vType == 14 || vType == 4) { typeStr = "Receipt"; rawType = "Rcpt"; }
            else if (vType == 15 || vType == 6) { typeStr = "Contra"; rawType = "Contra"; }
            else if (vType == 16 || vType == 5) { typeStr = "Journal"; rawType = "Jrnl"; }
            else if (vType == 11 || vType == 8) { typeStr = "Credit Note"; rawType = "CrNt"; }
            else if (vType == 12 || vType == 7) { typeStr = "Debit Note"; rawType = "DbNt"; }

            QString vFy = computeFy(vDate);

            // Separate financial GL splits (RecType==1) and inventory item rows (RecType==2)
            std::vector<std::map<std::string, std::string>> glRows;
            std::vector<std::map<std::string, std::string>> itemRows;
            if (t2Map.count(vCode)) {
                for (const auto& tr : t2Map[vCode]) {
                    int rType = parseInt(getVal(tr, "RecType"));
                    if (rType == 1) {
                        glRows.push_back(tr);
                    } else if (rType == 2) {
                        double q = std::abs(parseDouble(getVal(tr, "Value1", getVal(tr, "D1"))));
                        double val = std::abs(parseDouble(getVal(tr, "Value3", getVal(tr, "D5"))));
                        if (q > 0.0001 || val > 0.0001) {
                            itemRows.push_back(tr);
                        }
                    }
                }
            }

            // Insert Voucher Header
            db.executeNonQuery(
                "INSERT OR REPLACE INTO vouchers (voucher_no, voucher_type, voucher_date, party_id, party_name, account_type, amount, narration, financial_year) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {vNo, typeStr, vDate, (partyCode > 0 ? QVariant(partyCode) : QVariant()), partyName, partyName, totalAmt, narration, vFy}
            );
            stats.totalGlVouchers++;

            // Insert Sales Invoice & Stock Transactions
            if (isSale) {
                if (!itemRows.empty()) {
                    int sRowIdx = 1;
                    for (const auto& ir : itemRows) {
                        int itCode = parseInt(getVal(ir, "MasterCode1"));
                        QString itName = QString::fromStdString(busyMasterMap.count(itCode) ? cleanStr(getVal(busyMasterMap[itCode], "Name")) : "");
                        double qty = std::abs(parseDouble(getVal(ir, "Value1", getVal(ir, "D1"))));
                        double rate = parseDouble(getVal(ir, "D2", getVal(ir, "D3")));
                        double taxable = std::abs(parseDouble(getVal(ir, "D5", getVal(ir, "Value3"))));
                        if (taxable < 0.01 && qty > 0.001 && rate > 0.001) taxable = qty * rate;
                        double rowTotal = (taxable > 0.01) ? taxable : totalAmt;

                        db.executeNonQuery(
                            "INSERT OR REPLACE INTO sales_invoices (invoice_no, voucher_no, invoice_date, customer_id, customer_name, "
                            "item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, total_amount, narration, financial_year) "
                            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                            {vNo, vNo, vDate, (partyCode > 0 ? QVariant(partyCode) : QVariant()), partyName, itName, static_cast<int>(qty), qty, rate, taxable, rowTotal, narration, vFy}
                        );

                        db.executeNonQuery(
                            "INSERT INTO sales_invoice_items ("
                            "invoice_id, invoice_no, item_id, item_name, grade, bag_count, packing, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, total_amount) "
                            "VALUES ((SELECT id FROM sales_invoices WHERE invoice_no = ? LIMIT 1), ?, ?, ?, '', ?, 'Loose', ?, ?, ?, 0.0, ?);",
                            {vNo, vNo, (itCode > 0 ? QVariant(itCode) : QVariant()), itName, static_cast<int>(qty), qty, rate, taxable, rowTotal}
                        );

                        db.executeNonQuery(
                            "INSERT INTO stock_transactions ("
                            "fy_id, financial_year, voucher_no, voucher_date, trans_type, voucher_type, "
                            "party_id, party_name, bill_no, item_id, item_code, item_name, "
                            "bags, packing, weight_qtl, rate, amount, taxable_amount, tax, "
                            "tax_type, narration, row_no) "
                            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                            {
                                1, vFy, vNo, vDate, rawType, typeStr,
                                (partyCode > 0 ? QVariant(partyCode) : QVariant()),
                                partyName, vNo,
                                (itCode > 0 ? QVariant(itCode) : QVariant()),
                                (itCode > 0 ? QString::number(itCode) : QString()), itName,
                                static_cast<int>(qty), "Loose", qty, rate, rowTotal, taxable, 0.0,
                                "GST", narration, sRowIdx++
                            }
                        );
                        stats.totalSalesInvoices++;
                    }
                } else {
                    db.executeNonQuery(
                        "INSERT OR REPLACE INTO sales_invoices (invoice_no, voucher_no, invoice_date, customer_id, customer_name, "
                        "item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, total_amount, narration, financial_year) "
                        "VALUES (?, ?, ?, ?, ?, '', 0, 0, 0, ?, ?, ?, ?);",
                        {vNo, vNo, vDate, (partyCode > 0 ? QVariant(partyCode) : QVariant()), partyName, totalAmt, totalAmt, narration, vFy}
                    );
                    stats.totalSalesInvoices++;
                }
            } else if (isPurchase) {
                if (!itemRows.empty()) {
                    int pRowIdx = 1;
                    for (const auto& ir : itemRows) {
                        int itCode = parseInt(getVal(ir, "MasterCode1"));
                        QString itName = QString::fromStdString(busyMasterMap.count(itCode) ? cleanStr(getVal(busyMasterMap[itCode], "Name")) : "");
                        double qty = std::abs(parseDouble(getVal(ir, "Value1", getVal(ir, "D1"))));
                        double rate = parseDouble(getVal(ir, "D2", getVal(ir, "D3")));
                        double taxable = std::abs(parseDouble(getVal(ir, "D5", getVal(ir, "Value3"))));
                        if (taxable < 0.01 && qty > 0.001 && rate > 0.001) taxable = qty * rate;
                        double rowTotal = (taxable > 0.01) ? taxable : totalAmt;

                        db.executeNonQuery(
                            "INSERT OR REPLACE INTO purchase_invoices (invoice_no, voucher_no, invoice_date, supplier_id, supplier_name, "
                            "item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, total_amount, narration, financial_year) "
                            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                            {vNo, vNo, vDate, (partyCode > 0 ? QVariant(partyCode) : QVariant()), partyName, itName, static_cast<int>(qty), qty, rate, taxable, rowTotal, narration, vFy}
                        );

                        db.executeNonQuery(
                            "INSERT INTO purchase_invoice_items ("
                            "invoice_id, invoice_no, item_id, item_name, grade, bag_count, packing, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, total_amount) "
                            "VALUES ((SELECT id FROM purchase_invoices WHERE invoice_no = ? LIMIT 1), ?, ?, ?, '', ?, 'Loose', ?, ?, ?, 0.0, ?);",
                            {vNo, vNo, (itCode > 0 ? QVariant(itCode) : QVariant()), itName, static_cast<int>(qty), qty, rate, taxable, rowTotal}
                        );

                        db.executeNonQuery(
                            "INSERT INTO stock_transactions ("
                            "fy_id, financial_year, voucher_no, voucher_date, trans_type, voucher_type, "
                            "party_id, party_name, bill_no, item_id, item_code, item_name, "
                            "bags, packing, weight_qtl, rate, amount, taxable_amount, tax, "
                            "tax_type, narration, row_no) "
                            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                            {
                                1, vFy, vNo, vDate, rawType, typeStr,
                                (partyCode > 0 ? QVariant(partyCode) : QVariant()),
                                partyName, vNo,
                                (itCode > 0 ? QVariant(itCode) : QVariant()),
                                (itCode > 0 ? QString::number(itCode) : QString()), itName,
                                static_cast<int>(qty), "Loose", qty, rate, rowTotal, taxable, 0.0,
                                "GST", narration, pRowIdx++
                            }
                        );
                        stats.totalPurchaseInvoices++;
                    }
                } else {
                    db.executeNonQuery(
                        "INSERT OR REPLACE INTO purchase_invoices (invoice_no, voucher_no, invoice_date, supplier_id, supplier_name, "
                        "item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, total_amount, narration, financial_year) "
                        "VALUES (?, ?, ?, ?, ?, '', 0, 0, 0, ?, ?, ?, ?);",
                        {vNo, vNo, vDate, (partyCode > 0 ? QVariant(partyCode) : QVariant()), partyName, totalAmt, totalAmt, narration, vFy}
                    );
                    stats.totalPurchaseInvoices++;
                }
            }

            // Insert Double-Entry Ledger Splits from Tran2
            if (!glRows.empty()) {
                int rIdx = 1;
                QString opposingName = partyName;
                if (glRows.size() == 2) {
                    int a1 = parseInt(getVal(glRows[0], "MasterCode1"));
                    int a2 = parseInt(getVal(glRows[1], "MasterCode1"));
                    QString n1 = QString::fromStdString(busyMasterMap.count(a1) ? cleanStr(getVal(busyMasterMap[a1], "Name")) : "");
                    QString n2 = QString::fromStdString(busyMasterMap.count(a2) ? cleanStr(getVal(busyMasterMap[a2], "Name")) : "");

                    for (size_t i = 0; i < glRows.size(); ++i) {
                        const auto& tr = glRows[i];
                        int accCode = parseInt(getVal(tr, "MasterCode1"));
                        QString accName = (i == 0) ? n1 : n2;
                        QString oppName = (i == 0) ? n2 : n1;

                        double val1 = parseDouble(getVal(tr, "Value1"));
                        double d1 = parseDouble(getVal(tr, "D1"));
                        double d2 = parseDouble(getVal(tr, "D2"));
                        QString drCr;
                        double amt = 0.0;
                        if (std::abs(val1) > 0.001) {
                            amt = std::abs(val1);
                            drCr = (val1 < 0) ? "Dr" : "Cr";
                        } else {
                            if (d1 > 0.001) { amt = d1; drCr = "Dr"; }
                            else if (d2 > 0.001) { amt = d2; drCr = "Cr"; }
                            else {
                                amt = std::abs(parseDouble(getVal(tr, "Amount")));
                                drCr = (parseInt(getVal(tr, "Type")) == 1) ? "Dr" : "Cr";
                            }
                        }

                        QString lineNar = QString::fromStdString(cleanStr(getVal(tr, "ShortNar")));
                        if (lineNar.isEmpty()) lineNar = narration;

                        db.executeNonQuery(
                            "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_type, trans_type, voucher_date, account_code, party_id, party_name, opposing_account, dr_cr, amount, narration, row_no) "
                            "VALUES (1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                            {vFy, vNo, typeStr, rawType, vDate, accCode, accCode, accName, oppName, drCr, amt, lineNar, rIdx++}
                        );
                        stats.totalGlTransactions++;
                        if (drCr == "Dr") stats.totalDebitSum += amt;
                        else stats.totalCreditSum += amt;
                    }
                } else {
                    for (const auto& tr : glRows) {
                        int accCode = parseInt(getVal(tr, "MasterCode1"));
                        QString accName = QString::fromStdString(busyMasterMap.count(accCode) ? cleanStr(getVal(busyMasterMap[accCode], "Name")) : "");

                        double val1 = parseDouble(getVal(tr, "Value1"));
                        double d1 = parseDouble(getVal(tr, "D1"));
                        double d2 = parseDouble(getVal(tr, "D2"));
                        QString drCr;
                        double amt = 0.0;
                        if (std::abs(val1) > 0.001) {
                            amt = std::abs(val1);
                            drCr = (val1 < 0) ? "Dr" : "Cr";
                        } else {
                            if (d1 > 0.001) { amt = d1; drCr = "Dr"; }
                            else if (d2 > 0.001) { amt = d2; drCr = "Cr"; }
                            else {
                                amt = std::abs(parseDouble(getVal(tr, "Amount")));
                                drCr = (parseInt(getVal(tr, "Type")) == 1) ? "Dr" : "Cr";
                            }
                        }

                        QString lineNar = QString::fromStdString(cleanStr(getVal(tr, "ShortNar")));
                        if (lineNar.isEmpty()) lineNar = narration;

                        db.executeNonQuery(
                            "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_type, trans_type, voucher_date, account_code, party_id, party_name, opposing_account, dr_cr, amount, narration, row_no) "
                            "VALUES (1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                            {vFy, vNo, typeStr, rawType, vDate, accCode, accCode, accName, opposingName, drCr, amt, lineNar, rIdx++}
                        );
                        stats.totalGlTransactions++;
                        if (drCr == "Dr") stats.totalDebitSum += amt;
                        else stats.totalCreditSum += amt;
                    }
                }
            } else if (totalAmt > 0.0001) {
                // Fallback double-entry generation
                if (isSale) {
                    db.executeNonQuery(
                        "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_type, trans_type, voucher_date, account_code, party_id, party_name, opposing_account, dr_cr, amount, invoice_no, narration, taxable_amount, row_no) "
                        "VALUES (1, ?, ?, 'Sales', 'Sale', ?, ?, ?, ?, 'Sale A/c', 'Dr', ?, ?, ?, ?, 1);",
                        {vFy, vNo, vDate, partyCode, partyCode, partyName, totalAmt, vNo, narration, totalAmt}
                    );
                    db.executeNonQuery(
                        "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_type, trans_type, voucher_date, account_code, party_id, party_name, opposing_account, dr_cr, amount, invoice_no, narration, taxable_amount, row_no) "
                        "VALUES (1, ?, ?, 'Sales', 'Sale', ?, 4, 4, 'Sales', ?, 'Cr', ?, ?, ?, ?, 2);",
                        {vFy, vNo, vDate, partyName, totalAmt, vNo, narration, totalAmt}
                    );
                    stats.totalGlTransactions += 2;
                    stats.totalDebitSum += totalAmt;
                    stats.totalCreditSum += totalAmt;
                } else if (isPurchase) {
                    db.executeNonQuery(
                        "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_type, trans_type, voucher_date, account_code, party_id, party_name, opposing_account, dr_cr, amount, invoice_no, narration, taxable_amount, row_no) "
                        "VALUES (1, ?, ?, 'Purchase', 'Purc', ?, 5, 5, 'Purchase', ?, 'Dr', ?, ?, ?, ?, 1);",
                        {vFy, vNo, vDate, partyName, totalAmt, vNo, narration, totalAmt}
                    );
                    db.executeNonQuery(
                        "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_type, trans_type, voucher_date, account_code, party_id, party_name, opposing_account, dr_cr, amount, invoice_no, narration, taxable_amount, row_no) "
                        "VALUES (1, ?, ?, 'Purchase', 'Purc', ?, ?, ?, ?, 'Purchase A/c', 'Cr', ?, ?, ?, ?, 2);",
                        {vFy, vNo, vDate, partyCode, partyCode, partyName, totalAmt, vNo, narration, totalAmt}
                    );
                    stats.totalGlTransactions += 2;
                    stats.totalDebitSum += totalAmt;
                    stats.totalCreditSum += totalAmt;
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
            QString itName = QString::fromStdString(busyMasterMap.count(itCode) ? cleanStr(getVal(busyMasterMap[itCode], "Name")) : "");
            QString pName = QString::fromStdString(busyMasterMap.count(pCode) ? cleanStr(getVal(busyMasterMap[pCode], "Name")) : "");
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
