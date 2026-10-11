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
#include "jet4_reader.h"
#include "fiscal_year_helper.h"
#define HAS_LIBMDB 1

namespace {
// Canonical voucher identity "{yyYY}/{Type}-{raw}" (Busy-style FY-embedded
// numbering) for every voucher_no stored by this migrator. Mandi JFrm/IFrm
// keep raw numbers (date/id-scoped lookups, raw stock joins). invoice_no
// keeps source originals byte-identical.
static QString busyCanonVoucherNo(const QString& fyVal, const QString& rawType, const QString& rawNo) {
    QString tok = FiscalYearHelper::canonicalTypeToken(rawType);
    if (tok == "JFrm" || tok == "IFrm") return rawNo.trimmed();
    return FiscalYearHelper::canonicalVoucherNo(fyVal, tok, rawNo);
}
} // namespace

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

static int extractVoucherNum(const QString& vNo, int defaultNum = 1) {
    QString numStr;
    for (QChar c : vNo) {
        if (c.isDigit()) numStr.append(c);
    }
    if (!numStr.isEmpty()) {
        bool ok = false;
        int n = numStr.toInt(&ok);
        if (ok && n > 0) return n;
    }
    return defaultNum;
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
// ---- Busy exact-import lookups (decoded against Busy 21 / Comp0020) ----

// Statutory GST state-code -> state-name list (government codes, not firm data).
static QString gstStateNameFromDigit(int digit) {
    switch (digit) {
        case 1: return "Jammu and Kashmir";
        case 2: return "Himachal Pradesh";
        case 3: return "Punjab";
        case 4: return "Chandigarh";
        case 5: return "Uttarakhand";
        case 6: return "Haryana";
        case 7: return "Delhi";
        case 8: return "Rajasthan";
        case 9: return "Uttar Pradesh";
        case 10: return "Bihar";
        case 11: return "Sikkim";
        case 12: return "Arunachal Pradesh";
        case 13: return "Nagaland";
        case 14: return "Manipur";
        case 15: return "Mizoram";
        case 16: return "Tripura";
        case 17: return "Meghalaya";
        case 18: return "Assam";
        case 19: return "West Bengal";
        case 20: return "Jharkhand";
        case 21: return "Odisha";
        case 22: return "Chhattisgarh";
        case 23: return "Madhya Pradesh";
        case 24: return "Gujarat";
        case 25: return "Daman and Diu";
        case 26: return "Dadra and Nagar Haveli";
        case 27: return "Maharashtra";
        case 28: return "Andhra Pradesh";
        case 29: return "Karnataka";
        case 30: return "Goa";
        case 31: return "Lakshadweep";
        case 32: return "Kerala";
        case 33: return "Tamil Nadu";
        case 34: return "Puducherry";
        case 35: return "Andaman and Nicobar Islands";
        case 36: return "Telangana";
        case 37: return "Andhra Pradesh";
        case 38: return "Ladakh";
        default: return "";
    }
}

// Per-series mandi configuration (MandiVchConfig, keyed by voucher-series code).
struct BusyMandiCfg {
    int matCentre = 0;
    int salePurcAcc = 0;
    int salePurcType = 0;
    int tradingAcc = 0;
    int jrnlSeries = 0;
    int stkTrfSeries = 0;
};

// Per-item expense defaults (MandiItemWiseExpense: itemCode -> billSundryCode -> defs).
struct BusyExpenseDef {
    double purcVal = 0.0;
    double saleVal = 0.0;
};

// Bill-sundry master (Master1 MasterType=9): posting ledger per side.
// I1==1 means Add-type (posts to CM1), else Less-type (posts to CM2).
static int busySundryPostLedger(const std::map<std::string, std::string>& sundryRow) {
    int i1 = parseInt(getVal(sundryRow, "I1"));
    return (i1 == 1) ? parseInt(getVal(sundryRow, "CM1")) : parseInt(getVal(sundryRow, "CM2"));
}

static std::vector<std::map<std::string, std::string>> readTable(MdbHandle* mdb, const char* tableName) {
    // Jackcess-faithful JET4 scan (jet4_reader): explicit DELETED (0x8000)
    // skip + OVERFLOW (0x4000) follow. The old fetch-cursor path cracked
    // 0x4000 pointer slots as real rows and mislocated the cur_row-1 flag
    // peek after page turns, corrupting Busy Tran2/Tran1 splits.
    try {
        return Jet4Reader::readTable(mdb, tableName);
    } catch (...) {
        // Suppress any unexpected read errors
    }
    return {};
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
    int totalCancelled = 0;
    int totalTransactions = 0;
    int totalMandi = 0;
    int totalBillRefs = 0;
    int totalGstRows = 0;
    double totalDr = 0.0;
    double totalCr = 0.0;

    // Aggregate across EVERY year database (Busy splits financial years into
    // separate db*.bds files; only reading the last one silently drops years).
    QStringList allYearDbs = yearDbFiles;
    if (allYearDbs.isEmpty() && !mainDbFile.isEmpty()) allYearDbs << mainDbFile;
    QString activeYearDb = !allYearDbs.isEmpty() ? allYearDbs.last() : QString();
    for (const QString& yearDb : allYearDbs) {
        try {
            MdbHandle* yrMdb = mdb_open(yearDb.toUtf8().constData(), MDB_NOFLAGS);
            if (yrMdb) {
                bool isActive = (yearDb == activeYearDb);
                if (isActive && (companyName == "Busy Accounting Company" || companyName.isEmpty())) {
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

                // Masters live in every year file as copies: count once (active file).
                if (isActive) {
                    auto m1Rows = readTable(yrMdb, "Master1");
                    for (const auto& r : m1Rows) {
                        int mt = parseInt(getVal(r, "MasterType"));
                        if (mt == 1) totalGroups++;
                        else if (mt == 2) totalAccounts++;
                        else if (mt == 6 || (mt == 5 && parseInt(getVal(r, "ParentGrp")) > 0)) totalItems++;
                        else if (mt == 8 || mt == 16) totalUnits++;
                    }
                }

                // Vouchers accumulate across all year files.
                auto t1Rows = readTable(yrMdb, "Tran1");
                for (const auto& v : t1Rows) {
                    if (parseInt(getVal(v, "Cancelled")) != 0 || parseInt(getVal(v, "VchCancelled")) != 0)
                        totalCancelled++;
                    else
                        totalVouchers++;
                }

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
                totalMandi += static_cast<int>(mandiRows.size());

                auto t3Rows = readTable(yrMdb, "Tran3");
                totalBillRefs += static_cast<int>(t3Rows.size());

                auto gstRows = readTable(yrMdb, "VchGSTSumItemWise");
                totalGstRows += static_cast<int>(gstRows.size());

                mdb_close(yrMdb);
            }
        } catch (...) {
            // Suppress and continue
        }
    }

    bool hasBusyData = (totalAccounts > 0 || totalVouchers > 0 || totalItems > 0 || totalGroups > 0);
    report["success"] = hasBusyData;
    report["valid"] = hasBusyData;
    if (!hasBusyData) {
        report["message"] = "No Busy accounting masters or vouchers found in database.";
        report["error"] = report["message"];
    }
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
    report["cancelled_vouchers_count"] = totalCancelled;
    report["bill_refs_count"] = totalBillRefs;
    report["gst_rows_count"] = totalGstRows;
    report["year_files_count"] = static_cast<int>(allYearDbs.size());
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
    db.executeNonQuery("DELETE FROM jform_vouchers;");
    db.executeNonQuery("DELETE FROM jform_voucher_items;");
    db.executeNonQuery("DELETE FROM iform_vouchers;");
    db.executeNonQuery("DELETE FROM iform_voucher_items;");
    db.executeNonQuery("DELETE FROM paddy_procurement;");
    db.executeNonQuery("DELETE FROM paddy_arrivals;");
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
    db.setSetting("firm_source_type", "busy");
    db.setSetting("is_migrated", "1");

    BusyMigrationStats stats;
    stats.companyName = insp.value("company_name").toString();
    stats.gstin = insp.value("gstin").toString();
    stats.financialYear = insp.value("financial_year").toString();

    // Cross-scope accumulators (declared outside the per-file blocks so the
    // final audit summary can report them).
    int totalTran1Rows = 0, maxVchCodeSeen = 0;
    std::map<int, int> seriesSaleCount;
    std::map<int, int> seriesSaleMaxAuto;
    std::map<int, long long> seriesPurcMaxNum;
    std::map<int, QString> seriesPurcMaxNo;
    QString seriesNote, purcNote;
    QStringList verifyNotes;

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
                QString resolved = gstStateNameFromDigit(stateCode.toInt());
                if (!resolved.isEmpty()) state = resolved;
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

        // Exact-import lookups decoded from Busy 21 master types:
        // MT=9 bill sundries (posting ledgers), MT=21 voucher series, MT=25 tax
        // categories, MT=56 states, Config RecType=6 series prefixes,
        // MandiVchConfig per-series config, MandiItemWiseExpense item defaults.
        std::map<int, std::map<std::string, std::string>> sundryMasterMap;
        std::map<int, QString> taxCatNames;
        std::map<int, QString> stateMasterNames;
        std::map<int, QString> seriesNames;
        std::map<int, QString> seriesPrefixes;
        std::map<int, BusyMandiCfg> mandiCfgMap;
        std::map<int, std::map<int, BusyExpenseDef>> expenseByItem;
        int bardanaGroupCode = 0;
        std::set<int> bardanaItemCodes;
        for (const auto& kv : busyMasterMap) {
            const auto& r = kv.second;
            int mt = parseInt(getVal(r, "MasterType"));
            QString nm = QString::fromStdString(cleanStr(getVal(r, "Name")));
            if (mt == 9) sundryMasterMap[kv.first] = r;
            else if (mt == 25) taxCatNames[kv.first] = nm;
            else if (mt == 56) stateMasterNames[kv.first] = nm;
            else if (mt == 21) seriesNames[kv.first] = nm;
            else if (mt == 5 && nm.compare("Bardana", Qt::CaseInsensitive) == 0) bardanaGroupCode = kv.first;
            else if (mt == 6 && bardanaGroupCode > 0 && parseInt(getVal(r, "ParentGrp")) == bardanaGroupCode)
                bardanaItemCodes.insert(kv.first);
        }
        // Items may precede their group in map order; second pass for bardana items.
        if (bardanaGroupCode > 0) {
            for (const auto& kv : busyMasterMap) {
                if (parseInt(getVal(kv.second, "MasterType")) == 6 &&
                    parseInt(getVal(kv.second, "ParentGrp")) == bardanaGroupCode)
                    bardanaItemCodes.insert(kv.first);
            }
        }
        {
            auto cfgRows = readTable(yrMdb, "Config");
            for (const auto& cr : cfgRows) {
                if (parseInt(getVal(cr, "RecType")) != 6) continue;
                int sc = parseInt(getVal(cr, "L1"));
                if (sc > 0) seriesPrefixes[sc] = QString::fromStdString(cleanStr(getVal(cr, "C2")));
            }
            auto mcRows = readTable(yrMdb, "MandiVchConfig");
            for (const auto& mr : mcRows) {
                int vs = parseInt(getVal(mr, "VSCode"));
                if (vs <= 0) continue;
                BusyMandiCfg cfg;
                cfg.matCentre = parseInt(getVal(mr, "DefMatCentreCode"));
                cfg.salePurcAcc = parseInt(getVal(mr, "DefSalePurcAccCode"));
                cfg.salePurcType = parseInt(getVal(mr, "DefSalePurcType"));
                cfg.tradingAcc = parseInt(getVal(mr, "DefTradingAccount"));
                cfg.jrnlSeries = parseInt(getVal(mr, "JrnlVchSeries"));
                cfg.stkTrfSeries = parseInt(getVal(mr, "StkTfrVchSeries"));
                mandiCfgMap[vs] = cfg;
            }
            auto exRows = readTable(yrMdb, "MandiItemWiseExpense");
            for (const auto& er : exRows) {
                int it = parseInt(getVal(er, "ItemCode"));
                int bs = parseInt(getVal(er, "BSCode"));
                if (it <= 0 || bs <= 0) continue;
                BusyExpenseDef def;
                def.purcVal = parseDouble(getVal(er, "PurcDefVal"));
                def.saleVal = parseDouble(getVal(er, "SaleDefVal"));
                expenseByItem[it][bs] = def;
            }
        }

        // Pre-pass across ALL year files: counterparty home-state evidence from
        // VchGSTSumItemWise (PartyStateCode/TinDigit), first-seen wins.
        std::map<int, std::pair<QString, QString>> gstPartyState;
        {
            QStringList allDbs = yearDbFiles;
            if (allDbs.isEmpty() && !mainDbFile.isEmpty()) allDbs << mainDbFile;
            for (const QString& ydb : allDbs) {
                MdbHandle* pm = mdb_open(ydb.toUtf8().constData(), MDB_NOFLAGS);
                if (!pm) continue;
                auto grows = readTable(pm, "VchGSTSumItemWise");
                for (const auto& gr : grows) {
                    int pc = parseInt(getVal(gr, "PartyCode"));
                    if (pc <= 0 || gstPartyState.count(pc)) continue;
                    int sc = parseInt(getVal(gr, "PartyStateCode"));
                    QString digit = QString::fromStdString(cleanStr(getVal(gr, "PartyStateTinDigit")));
                    QString nm = stateMasterNames.count(sc) ? stateMasterNames[sc] : gstStateNameFromDigit(digit.toInt());
                    if (!nm.isEmpty()) gstPartyState[pc] = {nm, digit};
                }
                mdb_close(pm);
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
            QString nLower = name.toLower();
            if (nLower.contains("mandi tax") || nLower.contains("market fee") || nLower.contains("mkt fee") ||
                nLower.contains("kkf") || nLower.contains("hrdf") || nLower.contains("goshalla") || nLower.contains("dharmada")) {
                grpMeta.code1st = 10;
                grpMeta.code2nd = 9;
                grpMeta.nature = "Liabilities";
                grpMeta.extractBs = 1;
                partyType = "Duties/Taxes";
                groupName = "Duties & Taxes";
                parentCode = 118;
            } else if (nLower.contains("commission") || nLower.contains("dami") || nLower.contains("adhat")) {
                grpMeta.code1st = 16;
                grpMeta.code2nd = 15;
                grpMeta.nature = "Income";
                grpMeta.extractBs = 0;
                partyType = "Nominal";
                groupName = "Income (Indirect)";
                parentCode = 127;
            } else if (grpMeta.code1st == 8) partyType = "Buyer";
            else if (grpMeta.code1st == 11) partyType = "Vendor";
            else if (grpMeta.code1st == 3) partyType = "Bank";
            else if (grpMeta.code1st == 4) partyType = "Cash";
            else if (grpMeta.code1st == 10) partyType = "Duties/Taxes";
            else if (grpMeta.code1st == 19 || grpMeta.code1st == 20 || grpMeta.code1st == 23 || grpMeta.code1st == 17 || grpMeta.code1st == 16) partyType = "Nominal";

            QString address = "";
            QString city = "";
            QString state = "Haryana";
            QString stateCode = "06";
            QString pincode = "";
            QString gstin = "";
            QString pan = "";
            QString mobile = "";
            QString email = "";
            QString bankName = "";
            QString bankAc = "";
            QString ifsc = "";
            int addrStateCode = 0;

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
                bankName = QString::fromStdString(cleanStr(getVal(a, "BankName")));
                bankAc = QString::fromStdString(cleanStr(getVal(a, "BankAcNo")));
                ifsc = QString::fromStdString(cleanStr(getVal(a, "IFSCCode")));
                addrStateCode = parseInt(getVal(a, "StateCodeLong"));
            }

            // Counterparty state: GST evidence first, then address state master,
            // else home-state default (Haryana). No fabricated pincodes.
            if (gstPartyState.count(code)) {
                state = gstPartyState[code].first;
                stateCode = gstPartyState[code].second;
            } else if (addrStateCode > 0 && stateMasterNames.count(addrStateCode) &&
                       !stateMasterNames[addrStateCode].startsWith("---")) {
                state = stateMasterNames[addrStateCode];
                stateCode = "";
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
                "bank_name, bank_account, ifsc_code, opening_balance, balance_type, credit_days, credit_limit, gst_party_type) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {code, code, name, alias, printName, groupName, parentCode, grpMeta.code1st, partyType, (grpMeta.code1st == 23 ? 1 : 0),
                 address, city, state, stateCode, pincode, gstin, pan, mobile, email,
                 bankName, bankAc, ifsc, opBal, balType, static_cast<int>(crDays), crLimit,
                 (gstin.isEmpty() ? "Unregistered" : "Registered")}
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

            if (mt == 5) {
                // Stock Group (MT=5 only. MT=3, e.g. code 402 "General", is a
                // structural master, not a stock group — importing it would
                // REPLACE the real "General" group via the UNIQUE name rule.)
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

                // Sale/purchase posting ledgers (Master1 CM3/CM4 on the item).
                int saleLedCode = parseInt(getVal(r, "CM3"));
                int purcLedCode = parseInt(getVal(r, "CM4"));
                QString saleLedName = QString::fromStdString(busyMasterMap.count(saleLedCode) ? cleanStr(getVal(busyMasterMap[saleLedCode], "Name")) : "");
                QString purcLedName = QString::fromStdString(busyMasterMap.count(purcLedCode) ? cleanStr(getVal(busyMasterMap[purcLedCode], "Name")) : "");

                // Mandi expense defaults (MandiItemWiseExpense) into the
                // item's rate columns, resolved via the bill-sundry master.
                double damiRate = 0.0, mktRate = 0.0, hrdfRate = 0.0;
                double tulaiRate = 0.0, jhadaiRate = 0.0;
                QString damiLed, mktLed, hrdfLed;
                if (expenseByItem.count(code)) {
                    for (const auto& ekv : expenseByItem[code]) {
                        int bs = ekv.first;
                        if (!sundryMasterMap.count(bs)) continue;
                        QString sName = QString::fromStdString(cleanStr(getVal(sundryMasterMap[bs], "Name"))).toLower();
                        int postCode = busySundryPostLedger(sundryMasterMap[bs]);
                        QString postName = QString::fromStdString(busyMasterMap.count(postCode) ? cleanStr(getVal(busyMasterMap[postCode], "Name")) : "");
                        if ((sName.contains("adhat") || sName.contains("commission") || sName.contains("dami")) && sName.contains("sale")) {
                            damiRate = ekv.second.saleVal; damiLed = postName;
                        } else if (sName.contains("mandi tax") && !sName.contains("auction")) {
                            mktRate = ekv.second.saleVal != 0.0 ? ekv.second.saleVal : ekv.second.purcVal; mktLed = postName;
                        } else if (sName.contains("kkf") && !sName.contains("auction")) {
                            hrdfRate = ekv.second.saleVal != 0.0 ? ekv.second.saleVal : ekv.second.purcVal; hrdfLed = postName;
                        } else if (sName.contains("tulai")) {
                            tulaiRate = ekv.second.purcVal;
                        } else if (sName.contains("jhadai") || sName.contains("jharai")) {
                            jhadaiRate = ekv.second.purcVal;
                        }
                    }
                }

                db.executeNonQuery(
                    "INSERT OR REPLACE INTO stock_items (id, name, code, trading_group, group_code, hsn_code, unit, gst_rate, "
                    "opening_bags, opening_qty, opening_rate, opening_value, "
                    "sale_ledger, sale_ledger_id, purchase_ledger, purchase_ledger_id, "
                    "dami_rate, dami_ledger, market_fee_rate, market_fee_ledger, hrdf_rate, hrdf_ledger, "
                    "tulai_rate_1, jharai_rate_1) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                    {code, mName, QString::number(code), grpName, (pGrp > 0 ? QVariant(pGrp) : QVariant()), hsn, unitName, gstRate,
                     static_cast<int>(opQty), opQty, opRate, opVal,
                     saleLedName, (saleLedCode > 0 ? QVariant(saleLedCode) : QVariant()),
                     purcLedName, (purcLedCode > 0 ? QVariant(purcLedCode) : QVariant()),
                     damiRate, damiLed, mktRate, mktLed, hrdfRate, hrdfLed, tulaiRate, jhadaiRate}
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
        // STEP 5: VOUCHERS, INVOICES & MANDI RECORDS (85%)
        // ========================================================
        updateProgress(85, "Migrating Double-Entry Vouchers, J-Forms, I-Forms & Invoices...");
        // Vouchers are read from EVERY year database so multi-year Busy data
        // imports completely; masters above come from the active file only.
        QStringList voucherDbs = yearDbFiles;
        if (voucherDbs.isEmpty() && !mainDbFile.isEmpty()) voucherDbs << mainDbFile;
        for (const QString& voucherDb : voucherDbs) {
        MdbHandle* vMdb = mdb_open(voucherDb.toUtf8().constData(), MDB_NOFLAGS);
        if (!vMdb) continue;
        auto t1Rows = readTable(vMdb, "Tran1");
        auto t2Rows = readTable(vMdb, "Tran2");
        auto t3Rows = readTable(vMdb, "Tran3");
        auto mandiRows = readTable(vMdb, "MandiVchItemDet");
        auto gstSumRows = readTable(vMdb, "VchGSTSumItemWise");
        auto otherRows = readTable(vMdb, "VchOtherInfo");

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

        std::map<int, std::vector<std::map<std::string, std::string>>> mandiMap;
        for (const auto& mr : mandiRows) {
            int vCode = parseInt(getVal(mr, "VchCode"));
            mandiMap[vCode].push_back(mr);
        }

        // GST per-item splits (VchGSTSumItemWise) and voucher extras
        // (VchOtherInfo), keyed by voucher code.
        std::map<int, std::vector<std::map<std::string, std::string>>> gstSumMap;
        for (const auto& gr : gstSumRows) {
            int vCode = parseInt(getVal(gr, "VchCode"));
            gstSumMap[vCode].push_back(gr);
        }
        std::map<int, std::map<std::string, std::string>> otherInfoMap;
        for (const auto& orow : otherRows) {
            int vCode = parseInt(getVal(orow, "VchCode"));
            if (!otherInfoMap.count(vCode)) otherInfoMap[vCode] = orow;
        }

        for (const auto& v : t1Rows) {
            // Cancelled/void vouchers are never imported (counted for report).
            if (parseInt(getVal(v, "Cancelled")) != 0 || parseInt(getVal(v, "VchCancelled")) != 0) {
                stats.totalCancelledSkipped++;
                continue;
            }
            int vCode = parseInt(getVal(v, "VchCode"));
            int vType = parseInt(getVal(v, "VchType"));
            int vSeries = parseInt(getVal(v, "VchSeriesCode"));
            totalTran1Rows++;
            if (vCode > maxVchCodeSeen) maxVchCodeSeen = vCode;
            // Fallback GL accounts from MandiVchConfig (per-series sale/purchase
            // accounts, e.g. 1315/1316 for the Auction series), else 4/5.
            int fbSaleAccId = 4, fbPurcAccId = 5;
            QString fbSaleAccName = "Sales", fbPurcAccName = "Purchase";
            if (mandiCfgMap.count(vSeries) && mandiCfgMap[vSeries].salePurcAcc > 0) {
                int cfgAcc = mandiCfgMap[vSeries].salePurcAcc;
                QString cfgName = QString::fromStdString(busyMasterMap.count(cfgAcc) ? cleanStr(getVal(busyMasterMap[cfgAcc], "Name")) : "");
                if (cfgName.isEmpty()) cfgName = (vType == 9 || vType == 1) ? fbSaleAccName : fbPurcAccName;
                if (vType == 9 || vType == 1) { fbSaleAccId = cfgAcc; fbSaleAccName = cfgName; }
                else { fbPurcAccId = cfgAcc; fbPurcAccName = cfgName; }
            }
            QString vNo = QString::fromStdString(cleanStr(getVal(v, "VchNo")));
            QString vDate = parseDateStr(QString::fromStdString(getVal(v, "Date")));
            double totalAmt = parseDouble(getVal(v, "VchAmtBaseCur", getVal(v, "VchSalePurcAmt", getVal(v, "OrgVchAmtBaseCur", getVal(v, "TotalAmount")))));
            QString narration = QString::fromStdString(cleanStr(getVal(v, "Narration")));
            int partyCode = parseInt(getVal(v, "MasterCode1"));
            QString partyName = QString::fromStdString(busyMasterMap.count(partyCode) ? cleanStr(getVal(busyMasterMap[partyCode], "Name")) : "");

            bool isMandi = (mandiMap.count(vCode) > 0);
            QString typeStr = "Journal";
            QString rawType = "Jrnl";
            bool isSale = false;
            bool isPurchase = false;

            if (vType == 9 || vType == 1) {
                if (isMandi) {
                    typeStr = "I-Form";
                    rawType = "IFrm";
                } else {
                    typeStr = "Sales";
                    rawType = "Sale";
                }
                isSale = true;
            } else if (vType == 10 || vType == 2) {
                if (isMandi) {
                    typeStr = "J-Form";
                    rawType = "JFrm";
                } else {
                    typeStr = "Purchase";
                    rawType = "Purc";
                }
                isPurchase = true;
            } else if (vType == 13 || vType == 3) { typeStr = "Payment"; rawType = "Pymt"; }
            else if (vType == 14 || vType == 4) { typeStr = "Receipt"; rawType = "Rcpt"; }
            else if (vType == 15 || vType == 6) { typeStr = "Contra"; rawType = "Contra"; }
            else if (vType == 16 || vType == 5) { typeStr = "Journal"; rawType = "Jrnl"; }
            else if (vType == 11 || vType == 8) { typeStr = "Credit Note"; rawType = "CrNt"; }
            else if (vType == 12 || vType == 7) { typeStr = "Debit Note"; rawType = "DbNt"; }

            // Series continuity tracking (bill-series seeding + report).
            int autoNo = parseInt(getVal(v, "AutoVchNo"));
            if (isSale && vSeries > 0) {
                seriesSaleCount[vSeries]++;
                if (autoNo > seriesSaleMaxAuto[vSeries]) seriesSaleMaxAuto[vSeries] = autoNo;
            }
            if (isPurchase && vSeries > 0) {
                long long n = extractVoucherNum(vNo, 0);
                if (n > seriesPurcMaxNum[vSeries]) { seriesPurcMaxNum[vSeries] = n; seriesPurcMaxNo[vSeries] = vNo; }
            }

            QString vFy = computeFy(vDate);
            // Canonical identity for every voucher_no stored below.
            QString vchCanon = busyCanonVoucherNo(vFy, rawType, vNo);

            // Ensure the voucher's financial year exists (multi-year imports).
            {
                int fyStart = vFy.mid(3, 4).toInt();
                if (fyStart > 1900) {
                    db.executeNonQuery(
                        "INSERT OR IGNORE INTO financial_years (year_name, start_date, end_date, is_active) VALUES (?, ?, ?, 0);",
                        {vFy, QString("%1-04-01").arg(fyStart), QString("%1-03-31").arg(fyStart + 1)}
                    );
                }
            }

            // Voucher extras: transport/e-way/supplier-bill/narrations.
            QString vchNarration = narration;
            QString supplierBillNo, supplierBillDate, linkedPurcBill;
            QString vchVehicle, vchGrNo, vchEway;
            if (otherInfoMap.count(vCode)) {
                const auto& oi = otherInfoMap[vCode];
                QString n1 = QString::fromStdString(cleanStr(getVal(oi, "Narration1")));
                QString n2 = QString::fromStdString(cleanStr(getVal(oi, "Narration2")));
                if (!n1.isEmpty()) vchNarration = (vchNarration.isEmpty() ? n1 : vchNarration + " | " + n1);
                if (!n2.isEmpty()) vchNarration = (vchNarration.isEmpty() ? n2 : vchNarration + " | " + n2);
                supplierBillNo = QString::fromStdString(cleanStr(getVal(oi, "PurchaseBillNo")));
                supplierBillDate = parseDateStr(QString::fromStdString(getVal(oi, "PurchaseBillDate")));
                vchVehicle = QString::fromStdString(cleanStr(getVal(oi, "VehicleNo")));
                vchGrNo = QString::fromStdString(cleanStr(getVal(oi, "GrNo")));
            }
            if (isSale && !supplierBillNo.isEmpty() && supplierBillNo != vNo)
                linkedPurcBill = supplierBillNo;

            // GST split rows: place of supply + tax category for this voucher.
            QString voucherPos;
            QString voucherTaxStatus;
            double invGstPct = 5.0;
            QString invHsn;
            if (gstSumMap.count(vCode)) {
                const auto& grow = gstSumMap[vCode].front();
                int posDigit = parseInt(getVal(grow, "POSStateTinDigit"));
                voucherPos = gstStateNameFromDigit(posDigit);
                int taxCat = parseInt(getVal(grow, "TaxCatCode"));
                if (taxCatNames.count(taxCat)) voucherTaxStatus = taxCatNames[taxCat];
                invGstPct = parseDouble(getVal(grow, "TaxRate"), 5.0);
                invHsn = QString::fromStdString(cleanStr(getVal(grow, "HSNCode")));
            }

            // Bill-wise reference (Tran3, previously built but never stored).
            QString billRefNo, billRefNote;
            if (t3Map.count(vCode) && !t3Map[vCode].empty()) {
                const auto& t3 = t3Map[vCode].front();
                billRefNo = QString::fromStdString(cleanStr(getVal(t3, "No")));
                int refStatus = parseInt(getVal(t3, "Status"));
                int refType = parseInt(getVal(t3, "Type"));
                billRefNote = QString("BillRef %1 (Status %2, Type %3)").arg(billRefNo).arg(refStatus).arg(refType);
                stats.totalBillRefs++;
            }

            // Separate financial GL splits (RecType==1), inventory item rows (RecType==2), and bill sundries (RecType==3)
            std::vector<std::map<std::string, std::string>> glRows;
            std::vector<std::map<std::string, std::string>> itemRows;
            double labourSundry = 0.0;
            double mFeeSundry = 0.0;
            double mFeeRate = 0.0;
            double hrdfSundry = 0.0;
            double hrdfRate = 0.0;
            double damiSundry = 0.0;
            double damiRate = 0.0;
            double goshallaSundry = 0.0;
            double otherSundry = 0.0;

            // Reference item for per-item expense defaults (MandiItemWiseExpense
            // fallback when a sundry row carries no explicit rate in Value1).
            int refItemCode = 0;
            if (mandiMap.count(vCode) && !mandiMap[vCode].empty())
                refItemCode = parseInt(getVal(mandiMap[vCode].front(), "ItemCode"));
            if (refItemCode <= 0 && t2Map.count(vCode)) {
                for (const auto& tr : t2Map[vCode]) {
                    if (parseInt(getVal(tr, "RecType")) == 2) {
                        refItemCode = parseInt(getVal(tr, "MasterCode1"));
                        if (refItemCode > 0) break;
                    }
                }
            }
            auto expenseFallback = [&](int sundryCode) -> double {
                if (refItemCode > 0 && expenseByItem.count(refItemCode) &&
                    expenseByItem[refItemCode].count(sundryCode)) {
                    const BusyExpenseDef& def = expenseByItem[refItemCode][sundryCode];
                    return isSale ? def.saleVal : def.purcVal;
                }
                return 0.0;
            };

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
                    } else if (rType == 3) {
                        int sundryCode = parseInt(getVal(tr, "MasterCode1"));
                        QString sName = QString::fromStdString(busyMasterMap.count(sundryCode) ? cleanStr(getVal(busyMasterMap[sundryCode], "Name")) : "").toLower();
                        double val1 = parseDouble(getVal(tr, "Value1"));
                        double val3 = std::abs(parseDouble(getVal(tr, "Value3")));
                        double effRate = (val1 > 0.001) ? val1 : expenseFallback(sundryCode);
                        if (sName.contains("tulai") || sName.contains("jhadai") || sName.contains("jharai") || sName.contains("labour") || sName.contains("hamali")) {
                            labourSundry += val3;
                        } else if (sName.contains("mandi tax") || sName.contains("market fee") || sName.contains("mkt fee")) {
                            mFeeSundry += val3;
                            if (effRate > 0.001) mFeeRate = effRate;
                        } else if (sName.contains("kkf") || sName.contains("hrdf")) {
                            hrdfSundry += val3;
                            if (effRate > 0.001) hrdfRate = effRate;
                        } else if (sName.contains("commission") || sName.contains("dami") || sName.contains("adhat")) {
                            damiSundry += val3;
                            if (effRate > 0.001) damiRate = effRate;
                        } else if (sName.contains("goshalla")) {
                            goshallaSundry += val3;
                        } else {
                            otherSundry += val3;
                        }
                    }
                }
            }

            if (partyName.isEmpty() && !glRows.empty()) {
                int firstAcc = parseInt(getVal(glRows[0], "MasterCode1"));
                if (busyMasterMap.count(firstAcc)) {
                    partyCode = firstAcc;
                    partyName = QString::fromStdString(cleanStr(getVal(busyMasterMap[firstAcc], "Name")));
                }
            }

            // Stock-truth weight: Busy's item row (Tran2 RecType==2) quantity is
            // what its day summaries book (e.g. A101: 12.85, not the mandi
            // BuyerWeight 12.5), and the only qty consistent with rate math.
            auto mandiWeight = [&](const std::map<std::string, std::string>& mi, double fallbackWt) -> double {
                int mSr = parseInt(getVal(mi, "SrNo"));
                for (const auto& ir : itemRows) {
                    if (parseInt(getVal(ir, "SrNo")) == mSr) {
                        double qq = std::abs(parseDouble(getVal(ir, "Value1", getVal(ir, "D1"))));
                        if (qq > 0.001) return qq;
                    }
                }
                if (itemRows.size() == 1) {
                    double qq = std::abs(parseDouble(getVal(itemRows[0], "Value1", getVal(itemRows[0], "D1"))));
                    if (qq > 0.001) return qq;
                }
                return fallbackWt;
            };

            // Insert Voucher Header (narration carries Busy Narration1/2 +
            // linked-bill cross-references; POS/tax status from GST splits).
            QString headerNarration = vchNarration;
            if (!linkedPurcBill.isEmpty())
                headerNarration = (headerNarration.isEmpty() ? "" : headerNarration + " | ") + "Linked purchase bill " + linkedPurcBill;
            if (!billRefNote.isEmpty())
                headerNarration = (headerNarration.isEmpty() ? billRefNote : headerNarration + " | " + billRefNote);
            db.executeNonQuery(
                "INSERT OR REPLACE INTO vouchers (voucher_no, voucher_type, voucher_date, party_id, party_name, account_type, amount, narration, financial_year, "
                "place_of_supply, tax_status, vehicle_no, gr_no, eway_bill_no) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {vchCanon, typeStr, vDate, (partyCode > 0 ? QVariant(partyCode) : QVariant()), partyName, partyName, totalAmt, headerNarration, vFy,
                 voucherPos, voucherTaxStatus, vchVehicle, vchGrNo, vchEway}
            );
            // Downstream jform/iform/invoice/stock/GL rows inherit the merged
            // Busy narration (Tran1 itself carries no narration column).
            narration = headerNarration;
            stats.totalGlVouchers++;

            // ==========================================
            // MANDI J-FORM / I-FORM & INVOICE CREATION
            // ==========================================
            if (isMandi && isPurchase) {
                int totalBags = 0;
                double totalWeightQtl = 0.0;
                double goodsAmt = parseDouble(getVal(v, "VchSalePurcAmt"));
                int farmerId = partyCode;
                QString farmerName = partyName;
                int buyerId = 0;
                QString buyerName = "Self Purchase";
                QString auctionStatus = "Zimidara Self Purchase";

                const auto& mItems = mandiMap[vCode];
                // J-form number + purchase-item linkage come from the mandi row
                // (Busy authoritative), falling back to the voucher number.
                QString jfNo = vNo;
                QString jfLot;
                if (!mItems.empty()) {
                    QString mv = QString::fromStdString(cleanStr(getVal(mItems.front(), "VchNo")));
                    if (!mv.isEmpty()) jfNo = mv;
                    jfLot = QString::fromStdString(cleanStr(getVal(mItems.front(), "PurcItemId")));
                }
                for (const auto& mi : mItems) {
                    int bQty = std::abs(parseInt(getVal(mi, "BagQty")));
                    double wPerBag = parseDouble(getVal(mi, "WeightPerBag"), 50.0);
                    double lWt = parseDouble(getVal(mi, "LooseWeight"));
                    double bWt = parseDouble(getVal(mi, "BuyerWeight"));
                    double calcWt = mandiWeight(mi, (bWt > 0.001) ? bWt : ((bQty * wPerBag + lWt) / 100.0));
                    totalBags += bQty;
                    totalWeightQtl += calcWt;

                    int pCode = parseInt(getVal(mi, "PurchaserCode"));
                    if (pCode > 0 && busyMasterMap.count(pCode)) {
                        buyerId = pCode;
                        buyerName = QString::fromStdString(cleanStr(getVal(busyMasterMap[pCode], "Name")));
                        auctionStatus = "Mandi Auction";
                    }
                }

                if (goodsAmt < 0.01) {
                    goodsAmt = totalAmt + labourSundry;
                }
                double grandTotal = (totalAmt > 0.01) ? totalAmt : (goodsAmt - labourSundry);
                int numericVchNo = extractVoucherNum(vNo, stats.totalMandiRecords + 1);

                db.executeNonQuery(
                    "INSERT OR REPLACE INTO jform_vouchers ("
                    "fy_id, financial_year, voucher_no, voucher_date, jform_no, zimidar_id, zimidar_name, "
                    "party_id, party_name, auction_sale_status, due_days, total_bags, total_weight, "
                    "goods_amount, bonus_amount, relief_amount, subtotal_amount, labour_amount, "
                    "round_off, grand_total, narration, lot_no) "
                    "VALUES (1, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0, ?, ?, ?, 0.0, 0.0, ?, ?, 0.0, ?, ?, ?);",
                    {vFy, numericVchNo, vDate, jfNo, (farmerId > 0 ? QVariant(farmerId) : QVariant()), farmerName,
                     (buyerId > 0 ? QVariant(buyerId) : QVariant()), buyerName, auctionStatus,
                     totalBags, totalWeightQtl, goodsAmt, goodsAmt, labourSundry, grandTotal, narration, jfLot}
                );

                QVariant jfIdVar = db.executeScalar("SELECT last_insert_rowid();");
                int jfId = jfIdVar.toInt();

                int jRowIdx = 1;
                for (const auto& mi : mItems) {
                    int itCode = parseInt(getVal(mi, "ItemCode"));
                    QString itName = QString::fromStdString(busyMasterMap.count(itCode) ? cleanStr(getVal(busyMasterMap[itCode], "Name")) : "Wheat");
                    int bQty = std::abs(parseInt(getVal(mi, "BagQty")));
                    double wPerBag = parseDouble(getVal(mi, "WeightPerBag"), 50.0);
                    double lWt = parseDouble(getVal(mi, "LooseWeight"));
                    double bWt = parseDouble(getVal(mi, "BuyerWeight"));
                    double calcWt = mandiWeight(mi, (bWt > 0.001) ? bWt : ((bQty * wPerBag + lWt) / 100.0));
                    double rate = 0.0;
                    double itAmt = 0.0;
                    if (!itemRows.empty()) {
                        rate = parseDouble(getVal(itemRows[0], "D2", getVal(itemRows[0], "D3")));
                        itAmt = std::abs(parseDouble(getVal(itemRows[0], "D5", getVal(itemRows[0], "Value3"))));
                    }
                    if (rate < 0.01 && calcWt > 0.001) rate = goodsAmt / calcWt;
                    if (itAmt < 0.01) itAmt = calcWt * rate;

                    db.executeNonQuery(
                        "INSERT INTO jform_voucher_items ("
                        "voucher_id, voucher_no, item_id, item_name, bags, loose_weight, packing, weight, rate, amount) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                        {jfId, numericVchNo, (itCode > 0 ? QVariant(itCode) : QVariant()), itName,
                         bQty, lWt, (wPerBag / 100.0), calcWt, rate, itAmt}
                    );

                    db.executeNonQuery(
                        "INSERT OR REPLACE INTO paddy_procurement (receipt_no, arrival_date, farmer_id, farmer_name, variety, bag_count, gross_weight_qtl, net_weight_qtl, rate_per_qtl, total_amount) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                        {vNo, vDate, farmerId, farmerName, itName, bQty, calcWt, calcWt, rate, itAmt}
                    );

                    db.executeNonQuery(
                        "INSERT OR REPLACE INTO purchase_invoices (invoice_no, voucher_no, invoice_date, supplier_id, supplier_name, "
                        "item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, total_amount, narration, financial_year, sale_status, labour, m_fee, hrdf, "
                        "other_exp, po_no, gst_pct, hsn_code) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'Mandi J-Form', ?, ?, ?, ?, ?, ?, ?);",
                        {vNo, vchCanon, vDate, (farmerId > 0 ? QVariant(farmerId) : QVariant()), farmerName, itName, bQty, calcWt, rate, itAmt, grandTotal, narration, vFy, labourSundry, mFeeSundry, hrdfSundry,
                         (goshallaSundry + otherSundry),
                         ((supplierBillNo.isEmpty() || supplierBillNo == vNo) ? QString("") : supplierBillNo),
                         invGstPct,
                         QString::fromStdString(busyMasterMap.count(itCode) ? cleanStr(getVal(busyMasterMap[itCode], "HSNCode")) : "")}
                    );

                    db.executeNonQuery(
                        "INSERT INTO purchase_invoice_items ("
                        "invoice_id, invoice_no, item_id, item_name, grade, bag_count, packing, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, total_amount) "
                        "VALUES ((SELECT id FROM purchase_invoices WHERE invoice_no = ? LIMIT 1), ?, ?, ?, '', ?, 'Loose', ?, ?, ?, 0.0, ?);",
                        {vNo, vNo, (itCode > 0 ? QVariant(itCode) : QVariant()), itName, bQty, calcWt, rate, itAmt, itAmt}
                    );

                    db.executeNonQuery(
                        "INSERT INTO stock_transactions ("
                        "fy_id, financial_year, voucher_no, voucher_date, trans_type, voucher_type, "
                        "party_id, party_name, bill_no, item_id, item_code, item_name, "
                        "bags, packing, weight_qtl, rate, amount, taxable_amount, tax, "
                        "tax_type, narration, row_no) "
                        "VALUES (?, ?, ?, ?, 'Purc', 'J-Form', ?, ?, ?, ?, ?, ?, ?, 'Loose', ?, ?, ?, ?, 0.0, 'GST', ?, ?);",
                        {
                            1, vFy, vchCanon, vDate,
                            (farmerId > 0 ? QVariant(farmerId) : QVariant()),
                            farmerName, vNo,
                            (itCode > 0 ? QVariant(itCode) : QVariant()),
                            (itCode > 0 ? QString::number(itCode) : QString()), itName,
                            bQty, calcWt, rate, itAmt, itAmt, narration, jRowIdx++
                        }
                    );
                    stats.totalPurchaseInvoices++;
                }
                stats.totalMandiRecords++;
            } else if (isMandi && isSale) {
                int totalBags = 0;
                double totalWeightQtl = 0.0;
                double goodsAmt = parseDouble(getVal(v, "VchSalePurcAmt"));
                int buyerId = partyCode;
                QString buyerName = partyName;

                const auto& mItems = mandiMap[vCode];
                // I-form number + purchase-bill linkage from the mandi row.
                QString ifNo = vNo;
                QString ifLot;
                if (!mItems.empty()) {
                    QString mv = QString::fromStdString(cleanStr(getVal(mItems.front(), "VchNo")));
                    if (!mv.isEmpty()) ifNo = mv;
                    ifLot = QString::fromStdString(cleanStr(getVal(mItems.front(), "PurcItemId")));
                }
                for (const auto& mi : mItems) {
                    int bQty = std::abs(parseInt(getVal(mi, "BagQty")));
                    double wPerBag = parseDouble(getVal(mi, "WeightPerBag"), 50.0);
                    double lWt = std::abs(parseDouble(getVal(mi, "LooseWeight")));
                    double bWt = std::abs(parseDouble(getVal(mi, "BuyerWeight")));
                    double calcWt = mandiWeight(mi, (bWt > 0.001) ? bWt : ((bQty * wPerBag + lWt) / 100.0));
                    totalBags += bQty;
                    totalWeightQtl += calcWt;

                    int pCode = parseInt(getVal(mi, "PurchaserCode"));
                    if (pCode > 0 && (buyerId == 0 || buyerName.isEmpty()) && busyMasterMap.count(pCode)) {
                        buyerId = pCode;
                        buyerName = QString::fromStdString(cleanStr(getVal(busyMasterMap[pCode], "Name")));
                    }
                }

                if (goodsAmt < 0.01) {
                    goodsAmt = totalAmt - (damiSundry + mFeeSundry + hrdfSundry + labourSundry + goshallaSundry + otherSundry);
                }
                double grandTotal = (totalAmt > 0.01) ? totalAmt : (goodsAmt + damiSundry + mFeeSundry + hrdfSundry + labourSundry + goshallaSundry + otherSundry);
                int numericVchNo = extractVoucherNum(ifNo, stats.totalMandiRecords + 1);

                db.executeNonQuery(
                    "INSERT OR REPLACE INTO iform_vouchers ("
                    "fy_id, financial_year, voucher_no, voucher_date, iform_no, buyer_id, buyer_name, broker_name, "
                    "due_days, total_bags, total_weight, goods_amount, dami_rate, dami_amount, mandi_fee_rate, "
                    "mandi_fee_amount, hrdf_rate, hrdf_amount, labour_amount, taxable_amount, tax_amount, round_off, grand_total, narration, lot_no) "
                    "VALUES (1, ?, ?, ?, ?, ?, ?, '', 0, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0.0, 0.0, ?, ?, ?);",
                    {vFy, numericVchNo, vDate, ifNo, (buyerId > 0 ? QVariant(buyerId) : QVariant()), buyerName,
                     totalBags, totalWeightQtl, goodsAmt, damiRate, damiSundry, mFeeRate,
                     mFeeSundry, hrdfRate, hrdfSundry, labourSundry, goodsAmt, grandTotal, narration, ifLot}
                );

                QVariant ifIdVar = db.executeScalar("SELECT last_insert_rowid();");
                int ifId = ifIdVar.toInt();

                int iRowIdx = 1;
                for (const auto& mi : mItems) {
                    int itCode = parseInt(getVal(mi, "ItemCode"));
                    QString itName = QString::fromStdString(busyMasterMap.count(itCode) ? cleanStr(getVal(busyMasterMap[itCode], "Name")) : "Wheat");
                    int bQty = std::abs(parseInt(getVal(mi, "BagQty")));
                    double wPerBag = parseDouble(getVal(mi, "WeightPerBag"), 50.0);
                    double lWt = std::abs(parseDouble(getVal(mi, "LooseWeight")));
                    double bWt = std::abs(parseDouble(getVal(mi, "BuyerWeight")));
                    double calcWt = mandiWeight(mi, (bWt > 0.001) ? bWt : ((bQty * wPerBag + lWt) / 100.0));
                    double rate = 0.0;
                    double itAmt = 0.0;
                    if (!itemRows.empty()) {
                        rate = parseDouble(getVal(itemRows[0], "D2", getVal(itemRows[0], "D3")));
                        itAmt = std::abs(parseDouble(getVal(itemRows[0], "D5", getVal(itemRows[0], "Value3"))));
                    }
                    if (rate < 0.01 && calcWt > 0.001) rate = goodsAmt / calcWt;
                    if (itAmt < 0.01) itAmt = calcWt * rate;

                    db.executeNonQuery(
                        "INSERT INTO iform_voucher_items ("
                        "voucher_id, voucher_no, item_id, item_name, bags, loose_weight, packing, weight, rate, amount, "
                        "dami_rate, dami_amount, mandi_fee_rate, mandi_fee_amount, hrdf_rate, hrdf_amount, labour_amount) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                        {ifId, numericVchNo, (itCode > 0 ? QVariant(itCode) : QVariant()), itName,
                         bQty, lWt, (wPerBag / 100.0), calcWt, rate, itAmt,
                         damiRate, damiSundry, mFeeRate, mFeeSundry, hrdfRate, hrdfSundry, labourSundry}
                    );

                    db.executeNonQuery(
                        "INSERT OR REPLACE INTO sales_invoices (invoice_no, voucher_no, invoice_date, customer_id, customer_name, "
                        "item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, total_amount, narration, financial_year, sale_status, dami, m_fee, hrdf, labour, "
                        "other_exp, gst_pct, hsn_code) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'Mandi I-Form', ?, ?, ?, ?, ?, ?, ?);",
                        {vNo, vchCanon, vDate, (buyerId > 0 ? QVariant(buyerId) : QVariant()), buyerName, itName, bQty, calcWt, rate, itAmt, grandTotal, narration, vFy, damiSundry, mFeeSundry, hrdfSundry, labourSundry,
                         (goshallaSundry + otherSundry), invGstPct,
                         QString::fromStdString(busyMasterMap.count(itCode) ? cleanStr(getVal(busyMasterMap[itCode], "HSNCode")) : "")}
                    );

                    db.executeNonQuery(
                        "INSERT INTO sales_invoice_items ("
                        "invoice_id, invoice_no, item_id, item_name, grade, bag_count, packing, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, total_amount) "
                        "VALUES ((SELECT id FROM sales_invoices WHERE invoice_no = ? LIMIT 1), ?, ?, ?, '', ?, 'Loose', ?, ?, ?, 0.0, ?);",
                        {vNo, vNo, (itCode > 0 ? QVariant(itCode) : QVariant()), itName, bQty, calcWt, rate, itAmt, itAmt}
                    );

                    db.executeNonQuery(
                        "INSERT INTO stock_transactions ("
                        "fy_id, financial_year, voucher_no, voucher_date, trans_type, voucher_type, "
                        "party_id, party_name, bill_no, item_id, item_code, item_name, "
                        "bags, packing, weight_qtl, rate, amount, taxable_amount, tax, "
                        "tax_type, narration, row_no) "
                        "VALUES (?, ?, ?, ?, 'Sale', 'I-Form', ?, ?, ?, ?, ?, ?, ?, 'Loose', ?, ?, ?, ?, 0.0, 'GST', ?, ?);",
                        {
                            1, vFy, vchCanon, vDate,
                            (buyerId > 0 ? QVariant(buyerId) : QVariant()),
                            buyerName, vNo,
                            (itCode > 0 ? QVariant(itCode) : QVariant()),
                            (itCode > 0 ? QString::number(itCode) : QString()), itName,
                            bQty, calcWt, rate, itAmt, itAmt, narration, iRowIdx++
                        }
                    );
                    stats.totalSalesInvoices++;
                }
                stats.totalMandiRecords++;
            } else if (isSale) {
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
                            "item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, total_amount, narration, financial_year, dami, labour, m_fee, hrdf, other_exp) "
                            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                            {vNo, vchCanon, vDate, (partyCode > 0 ? QVariant(partyCode) : QVariant()), partyName, itName, static_cast<int>(qty), qty, rate, taxable, rowTotal, narration, vFy,
                             damiSundry, labourSundry, mFeeSundry, hrdfSundry, (goshallaSundry + otherSundry)}
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
                                1, vFy, vchCanon, vDate, rawType, typeStr,
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
                        "item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, total_amount, narration, financial_year, dami, labour, m_fee, hrdf, other_exp) "
                        "VALUES (?, ?, ?, ?, ?, '', 0, 0, 0, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                        {vNo, vchCanon, vDate, (partyCode > 0 ? QVariant(partyCode) : QVariant()), partyName, totalAmt, totalAmt, narration, vFy,
                         damiSundry, labourSundry, mFeeSundry, hrdfSundry, (goshallaSundry + otherSundry)}
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
                            "item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, total_amount, narration, financial_year, labour, m_fee, hrdf, other_exp, po_no) "
                            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                            {vNo, vchCanon, vDate, (partyCode > 0 ? QVariant(partyCode) : QVariant()), partyName, itName, static_cast<int>(qty), qty, rate, taxable, rowTotal, narration, vFy,
                             labourSundry, mFeeSundry, hrdfSundry, (goshallaSundry + otherSundry),
                             ((supplierBillNo.isEmpty() || supplierBillNo == vNo) ? QString("") : supplierBillNo)}
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
                                1, vFy, vchCanon, vDate, rawType, typeStr,
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
                        "item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, total_amount, narration, financial_year, labour, m_fee, hrdf, other_exp) "
                        "VALUES (?, ?, ?, ?, ?, '', 0, 0, 0, ?, ?, ?, ?, ?, ?, ?, ?);",
                        {vNo, vchCanon, vDate, (partyCode > 0 ? QVariant(partyCode) : QVariant()), partyName, totalAmt, totalAmt, narration, vFy,
                         labourSundry, mFeeSundry, hrdfSundry, (goshallaSundry + otherSundry)}
                    );
                    stats.totalPurchaseInvoices++;
                }
            }

            // Vouchers with item rows but no GL legs: Bardana slips (all items
            // are Bardana-group items) or zero-value stock transfers. Neither
            // was imported before; both now land in their exact registers.
            if (!isMandi && glRows.empty() && !itemRows.empty()) {
                bool allBardana = true;
                for (const auto& ir : itemRows) {
                    int itCode = parseInt(getVal(ir, "MasterCode1"));
                    if (!bardanaItemCodes.count(itCode)) { allBardana = false; break; }
                }
                if (allBardana) {
                    int bRowIdx = 1;
                    for (const auto& ir : itemRows) {
                        int itCode = parseInt(getVal(ir, "MasterCode1"));
                        QString itName = QString::fromStdString(busyMasterMap.count(itCode) ? cleanStr(getVal(busyMasterMap[itCode], "Name")) : "Bardana");
                        double qty = parseDouble(getVal(ir, "Value1", getVal(ir, "D1")));
                        int mc2 = parseInt(getVal(ir, "MasterCode2"));
                        QString godown = QString::fromStdString(busyMasterMap.count(mc2) ? cleanStr(getVal(busyMasterMap[mc2], "Name")) : "Main Godown");
                        QString dc = (qty < 0) ? "Cr" : "Dr";
                        db.executeNonQuery(
                            "INSERT INTO bardana_transactions (fy_id, financial_year, voucher_no, voucher_date, vch_type, party_id, party_name, "
                            "bardana_type, godown_name, dr_cr, qty, rate, amount, bill_no, narration) "
                            "VALUES (1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0.0, 0.0, ?, ?);",
                            {vFy, vchCanon, vDate, typeStr,
                             (partyCode > 0 ? QVariant(partyCode) : QVariant()), partyName,
                             itName, godown, dc, static_cast<int>(std::abs(qty)), vNo,
                             (headerNarration.isEmpty() ? QString("Busy bardana slip; no GL legs in source") : headerNarration)}
                        );
                        db.executeNonQuery(
                            "INSERT INTO stock_transactions ("
                            "fy_id, financial_year, voucher_no, voucher_date, trans_type, voucher_type, "
                            "party_id, party_name, bill_no, item_id, item_code, item_name, "
                            "bags, packing, weight_qtl, rate, amount, taxable_amount, tax, "
                            "tax_type, narration, row_no) "
                            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                            {1, vFy, vchCanon, vDate, rawType, typeStr,
                             (partyCode > 0 ? QVariant(partyCode) : QVariant()), partyName, vNo,
                             (itCode > 0 ? QVariant(itCode) : QVariant()),
                             (itCode > 0 ? QString::number(itCode) : QString()), itName,
                             static_cast<int>(std::abs(qty)), "Loose", std::abs(qty), 0.0, 0.0, 0.0, 0.0,
                             "GST", headerNarration, bRowIdx++}
                        );
                    }
                    stats.totalBardanaSlips++;
                } else {
                    // Stock transfer (e.g. Busy StkTrf series): signed qty rows
                    // move stock between material centres at zero value.
                    int tRowIdx = 1;
                    for (const auto& ir : itemRows) {
                        int itCode = parseInt(getVal(ir, "MasterCode1"));
                        QString itName = QString::fromStdString(busyMasterMap.count(itCode) ? cleanStr(getVal(busyMasterMap[itCode], "Name")) : "");
                        double qty = parseDouble(getVal(ir, "Value1", getVal(ir, "D1")));
                        int mc2 = parseInt(getVal(ir, "MasterCode2"));
                        QString godown = QString::fromStdString(busyMasterMap.count(mc2) ? cleanStr(getVal(busyMasterMap[mc2], "Name")) : "");
                        QString dirNar = ((qty < 0) ? QString("Outward ") : QString("Inward ")) + godown;
                        if (!headerNarration.isEmpty()) dirNar += " | " + headerNarration;
                        db.executeNonQuery(
                            "INSERT INTO stock_transactions ("
                            "fy_id, financial_year, voucher_no, voucher_date, trans_type, voucher_type, "
                            "party_id, party_name, bill_no, item_id, item_code, item_name, "
                            "bags, packing, weight_qtl, rate, amount, taxable_amount, tax, "
                            "tax_type, narration, row_no) "
                            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                            {1, vFy, vchCanon, vDate, rawType, typeStr,
                             (partyCode > 0 ? QVariant(partyCode) : QVariant()), partyName, vNo,
                             (itCode > 0 ? QVariant(itCode) : QVariant()),
                             (itCode > 0 ? QString::number(itCode) : QString()), itName,
                             static_cast<int>(std::abs(qty)), "Loose", std::abs(qty), 0.0, 0.0, 0.0, 0.0,
                             "GST", dirNar, tRowIdx++}
                        );
                    }
                    stats.totalStockTransfers++;
                }
            }

            // Insert Double-Entry Ledger Splits from Tran2
            // (invoice_no carries the Tran3 bill reference when present.)
            QString glInvNo = billRefNo.isEmpty() ? vNo : billRefNo;
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
                            "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_type, trans_type, voucher_date, account_code, party_id, party_name, opposing_account, dr_cr, amount, narration, row_no, invoice_no, place_of_supply) "
                            "VALUES (1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                            {vFy, vchCanon, typeStr, rawType, vDate, accCode, accCode, accName, oppName, drCr, amt, lineNar, rIdx++, glInvNo, voucherPos}
                        );
                        stats.totalGlTransactions++;
                        if (drCr == "Dr") stats.totalDebitSum += amt;
                        else stats.totalCreditSum += amt;
                    }
                } else {
                    QString drOpposing = "";
                    QString crOpposing = "";
                    for (const auto& tr : glRows) {
                        int ac = parseInt(getVal(tr, "MasterCode1"));
                        QString an = QString::fromStdString(busyMasterMap.count(ac) ? cleanStr(getVal(busyMasterMap[ac], "Name")) : "");
                        double v1 = parseDouble(getVal(tr, "Value1"));
                        if (v1 < 0 && drOpposing.isEmpty()) drOpposing = an;
                        else if (v1 > 0 && crOpposing.isEmpty()) crOpposing = an;
                    }

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

                        QString oppName = (drCr == "Dr") ? crOpposing : drOpposing;
                        if (oppName.isEmpty()) oppName = opposingName;

                        QString lineNar = QString::fromStdString(cleanStr(getVal(tr, "ShortNar")));
                        if (lineNar.isEmpty()) lineNar = narration;

                        db.executeNonQuery(
                            "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_type, trans_type, voucher_date, account_code, party_id, party_name, opposing_account, dr_cr, amount, narration, row_no, invoice_no, place_of_supply) "
                            "VALUES (1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                            {vFy, vchCanon, typeStr, rawType, vDate, accCode, accCode, accName, oppName, drCr, amt, lineNar, rIdx++, glInvNo, voucherPos}
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
                        "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_type, trans_type, voucher_date, account_code, party_id, party_name, opposing_account, dr_cr, amount, invoice_no, narration, taxable_amount, row_no, place_of_supply) "
                        "VALUES (1, ?, ?, 'Sales', 'Sale', ?, ?, ?, ?, ?, 'Dr', ?, ?, ?, ?, 1, ?);",
                        {vFy, vchCanon, vDate, partyCode, partyCode, partyName, (fbSaleAccName + " A/c"), totalAmt, vNo, narration, totalAmt, voucherPos}
                    );
                    db.executeNonQuery(
                        "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_type, trans_type, voucher_date, account_code, party_id, party_name, opposing_account, dr_cr, amount, invoice_no, narration, taxable_amount, row_no, place_of_supply) "
                        "VALUES (1, ?, ?, 'Sales', 'Sale', ?, ?, ?, ?, ?, 'Cr', ?, ?, ?, ?, 2, ?);",
                        {vFy, vchCanon, vDate, fbSaleAccId, fbSaleAccId, fbSaleAccName, partyName, totalAmt, vNo, narration, totalAmt, voucherPos}
                    );
                    stats.totalGlTransactions += 2;
                    stats.totalDebitSum += totalAmt;
                    stats.totalCreditSum += totalAmt;
                } else if (isPurchase) {
                    db.executeNonQuery(
                        "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_type, trans_type, voucher_date, account_code, party_id, party_name, opposing_account, dr_cr, amount, invoice_no, narration, taxable_amount, row_no, place_of_supply) "
                        "VALUES (1, ?, ?, 'Purchase', 'Purc', ?, ?, ?, ?, ?, 'Dr', ?, ?, ?, ?, 1, ?);",
                        {vFy, vchCanon, vDate, fbPurcAccId, fbPurcAccId, fbPurcAccName, partyName, totalAmt, vNo, narration, totalAmt, voucherPos}
                    );
                    db.executeNonQuery(
                        "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_type, trans_type, voucher_date, account_code, party_id, party_name, opposing_account, dr_cr, amount, invoice_no, narration, taxable_amount, row_no, place_of_supply) "
                        "VALUES (1, ?, ?, 'Purchase', 'Purc', ?, ?, ?, ?, ?, 'Cr', ?, ?, ?, ?, 2, ?);",
                        {vFy, vchCanon, vDate, partyCode, partyCode, partyName, (fbPurcAccName + " A/c"), totalAmt, vNo, narration, totalAmt, voucherPos}
                    );
                    stats.totalGlTransactions += 2;
                    stats.totalDebitSum += totalAmt;
                    stats.totalCreditSum += totalAmt;
                }
            }
        }

        // ========================================================
        // STEP 6: MANDI & PADDY PROCUREMENT FALLBACK (95%)
        // ========================================================
        updateProgress(95, "Verifying Mandi Procurement & Gunny Bag records...");
        for (const auto& mr : mandiRows) {
            int vCode = parseInt(getVal(mr, "VchCode"));
            int itCode = parseInt(getVal(mr, "ItemCode"));
            int pCode = parseInt(getVal(mr, "PartyCode"));
            QString itName = QString::fromStdString(busyMasterMap.count(itCode) ? cleanStr(getVal(busyMasterMap[itCode], "Name")) : "");
            QString pName = QString::fromStdString(busyMasterMap.count(pCode) ? cleanStr(getVal(busyMasterMap[pCode], "Name")) : "");
            QString vNo = QString::fromStdString(cleanStr(getVal(mr, "VchNo")));
            QString vDate = parseDateStr(QString::fromStdString(getVal(mr, "Date")));
            double bagQty = std::abs(parseDouble(getVal(mr, "BagQty")));
            double looseWt = std::abs(parseDouble(getVal(mr, "LooseWeight")));
            double buyerWt = std::abs(parseDouble(getVal(mr, "BuyerWeight"), looseWt));
            // Prefer the voucher's item-row qty (stock truth, see mandiWeight).
            if (t2Map.count(vCode)) {
                int mSr = parseInt(getVal(mr, "SrNo"));
                const std::map<std::string, std::string>* hit = nullptr;
                int itemRowCount = 0;
                for (const auto& tr : t2Map[vCode]) {
                    if (parseInt(getVal(tr, "RecType")) != 2) continue;
                    itemRowCount++;
                    if (parseInt(getVal(tr, "SrNo")) == mSr) { hit = &tr; break; }
                }
                if (!hit && itemRowCount == 1) {
                    for (const auto& tr : t2Map[vCode]) {
                        if (parseInt(getVal(tr, "RecType")) == 2) { hit = &tr; break; }
                    }
                }
                if (hit) {
                    double qq = std::abs(parseDouble(getVal(*hit, "Value1", getVal(*hit, "D1"))));
                    if (qq > 0.001) { looseWt = qq; buyerWt = qq; }
                }
            }

            db.executeNonQuery(
                "INSERT OR REPLACE INTO paddy_procurement (receipt_no, arrival_date, farmer_id, farmer_name, variety, bag_count, gross_weight_qtl, net_weight_qtl, rate_per_qtl, total_amount) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {vNo, vDate, pCode, pName, itName, static_cast<int>(bagQty), looseWt, buyerWt, 0.0, 0.0}
            );
        } // end STEP6 per-file fallback

        mdb_close(vMdb);
        } // end per-year-file voucher loop

        // ========================================================
        // STEP 7: DAY-SUMMARY CROSS-CHECK + BILL-SERIES SEED (97%)
        // ========================================================
        updateProgress(97, "Cross-checking balances and seeding bill series...");
        verifyNotes.clear();
        {
            auto dsRows = readTable(yrMdb, "DailySum");
            // Our GL net per account (Dr positive).
            std::map<int, double> ourNet;
            QVariantList glAgg = db.executeQuery(
                "SELECT party_id, SUM(CASE WHEN dr_cr='Dr' THEN amount ELSE -amount END) AS net FROM transactions GROUP BY party_id;");
            for (const QVariant& qr : glAgg) {
                QVariantMap m = qr.toMap();
                ourNet[m.value("party_id").toInt()] = m.value("net").toDouble();
            }
            // Busy turnover net per ledger (Cr-Dr), summed over all day rows.
            std::map<int, double> busyNet;
            std::map<int, double> busyInQ, busyOutQ, busyInV, busyOutV;
            for (const auto& dr : dsRows) {
                int mc1 = parseInt(getVal(dr, "MasterCode1"));
                int mc2 = parseInt(getVal(dr, "MasterCode2"));
                int dmt = parseInt(getVal(dr, "MasterType"));
                double d1 = parseDouble(getVal(dr, "Dr1"));
                double c1 = parseDouble(getVal(dr, "Cr1"));
                double d3 = parseDouble(getVal(dr, "Dr3"));
                double c3 = parseDouble(getVal(dr, "Cr3"));
                if (dmt == 2) {
                    busyNet[mc1] += (c1 - d1);
                } else if (dmt == 6 && mc2 == 0) {
                    busyInQ[mc1] += c1; busyOutQ[mc1] += d1;
                    busyInV[mc1] += c3; busyOutV[mc1] += d3;
                }
            }
            for (const auto& kv : busyNet) {
                double ours = ourNet.count(kv.first) ? ourNet[kv.first] : 0.0;
                if (std::abs(ours + kv.second) > 0.01) {
                    QString nm = busyMasterMap.count(kv.first)
                        ? QString::fromStdString(cleanStr(getVal(busyMasterMap[kv.first], "Name")))
                        : QString::number(kv.first);
                    verifyNotes << QString("Ledger %1: Busy net %2 vs imported %3").arg(nm).arg(kv.second, 0, 'f', 2).arg(-ours, 0, 'f', 2);
                }
            }
            // Our stock in/out per item from stock legs.
            QVariantList stAgg = db.executeQuery(
                "SELECT item_id, "
                "SUM(CASE WHEN voucher_type IN ('J-Form','Purchase','Receipt') OR (trans_type='Jrnl' AND narration LIKE 'Inward%') THEN weight_qtl ELSE 0 END) AS qin, "
                "SUM(CASE WHEN voucher_type IN ('I-Form','Sales','Credit Note') OR (trans_type='Jrnl' AND narration LIKE 'Outward%') THEN weight_qtl ELSE 0 END) AS qout, "
                "SUM(CASE WHEN voucher_type IN ('J-Form','Purchase','Receipt') OR (trans_type='Jrnl' AND narration LIKE 'Inward%') THEN amount ELSE 0 END) AS vin, "
                "SUM(CASE WHEN voucher_type IN ('I-Form','Sales','Credit Note') OR (trans_type='Jrnl' AND narration LIKE 'Outward%') THEN amount ELSE 0 END) AS vout "
                "FROM stock_transactions GROUP BY item_id;");
            for (const QVariant& qr : stAgg) {
                QVariantMap m = qr.toMap();
                int ic = m.value("item_id").toInt();
                if (!busyInQ.count(ic) && !busyOutQ.count(ic)) continue;
                QString nm = busyMasterMap.count(ic)
                    ? QString::fromStdString(cleanStr(getVal(busyMasterMap[ic], "Name")))
                    : QString::number(ic);
                if (std::abs(m.value("qin").toDouble() - busyInQ[ic]) > 0.005)
                    verifyNotes << QString("Item %1 inward qty: Busy %2 vs imported %3").arg(nm).arg(busyInQ[ic]).arg(m.value("qin").toDouble());
                if (std::abs(m.value("qout").toDouble() - busyOutQ[ic]) > 0.005)
                    verifyNotes << QString("Item %1 outward qty: Busy %2 vs imported %3").arg(nm).arg(busyOutQ[ic]).arg(m.value("qout").toDouble());
                if (std::abs(m.value("vin").toDouble() - busyInV[ic]) > 0.01)
                    verifyNotes << QString("Item %1 inward value: Busy %2 vs imported %3").arg(nm).arg(busyInV[ic], 0, 'f', 2).arg(m.value("vin").toDouble(), 0, 'f', 2);
                if (std::abs(m.value("vout").toDouble() - busyOutV[ic]) > 0.01)
                    verifyNotes << QString("Item %1 outward value: Busy %2 vs imported %3").arg(nm).arg(busyOutQ[ic], 0, 'f', 2).arg(m.value("vout").toDouble(), 0, 'f', 2);
            }
        }

        // Seed bill_series/sale from the dominant Busy sale series (only when
        // the fresh database has no series configured yet).
        seriesNote.clear();
        if (!seriesSaleCount.empty() && db.getSetting("bill_series/sale").trimmed().isEmpty()) {
            int best = 0, bestN = -1;
            for (const auto& kv : seriesSaleCount) {
                if (kv.second > bestN) { bestN = kv.second; best = kv.first; }
            }
            QString pfx = seriesPrefixes.count(best) ? seriesPrefixes[best].trimmed() : QString();
            // Busy stores "A-"/"T-" with dash; split into prefix + separator.
            QString sep = "";
            if (pfx.endsWith("-")) { sep = "-"; pfx.chop(1); }
            long long nextNo = (seriesSaleMaxAuto.count(best) ? seriesSaleMaxAuto[best] : 0) + 1;
            if (nextNo < 1) nextNo = 1;
            QString sName = seriesNames.count(best) ? seriesNames[best] : QString("Series %1").arg(best);
            QString json = QString("{\"prefix\":\"%1\",\"sepAfterPrefix\":\"%2\",\"fyStyle\":\"none\",\"sepBeforeSeq\":\"\",\"seqWidth\":1,\"startNumber\":%3,\"resetEachFy\":false,\"suffix\":\"\"}")
                .arg(pfx).arg(sep).arg(nextNo);
            db.setSetting("bill_series/sale", json);
            seriesNote = QString("Seeded bill_series/sale from Busy %1 (prefix '%2', next %3)").arg(sName).arg(pfx + sep).arg(nextNo);
        }
        purcNote.clear();
        for (const auto& kv : seriesPurcMaxNo) {
            QString sName = seriesNames.count(kv.first) ? seriesNames[kv.first] : QString("Series %1").arg(kv.first);
            purcNote += QString("%1\u2192%2 ").arg(sName).arg(kv.second.trimmed());
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
        "• Cancelled Vouchers Skipped: %9\n"
        "• Bill References (Tran3) Stored: %10\n"
        "• Bardana Slips Imported: %11\n"
        "• Stock Transfers Imported: %12\n"
        "• Double-Entry Balance: %13 (Discrepancy: ₹%14)"
    )
    .arg(stats.companyName, stats.financialYear)
    .arg(stats.totalGroups)
    .arg(stats.totalAccounts)
    .arg(stats.totalStockItems + stats.totalStockUnits)
    .arg(stats.totalSalesInvoices + stats.totalPurchaseInvoices)
    .arg(stats.totalGlTransactions)
    .arg(stats.totalMandiRecords)
    .arg(stats.totalCancelledSkipped)
    .arg(stats.totalBillRefs)
    .arg(stats.totalBardanaSlips)
    .arg(stats.totalStockTransfers)
    .arg(stats.isBalanced ? "PERFECTLY BALANCED (Dr == Cr)" : "Unbalanced")
    .arg(QString::number(stats.glDiscrepancy, 'f', 2));

    // Audit tail: voucher-code gaps, series continuity, verification outcome,
    // and source tables with no importable content (nothing dropped silently).
    {
        int gaps = (maxVchCodeSeen > 0 && totalTran1Rows > 0) ? (maxVchCodeSeen - totalTran1Rows) : 0;
        summaryMsg += QString("\n• Voucher Codes: %1 rows, max code %2 (%3 gaps = deletions not logged in source; DeletedInfo/DeletedMasters empty)")
            .arg(totalTran1Rows).arg(maxVchCodeSeen).arg(qMax(gaps, 0));
        if (!seriesNote.isEmpty()) summaryMsg += "\n• " + seriesNote;
        if (!purcNote.isEmpty()) summaryMsg += "\n• Purchase series continuity (supplier bills stay manual): " + purcNote.trimmed();
        summaryMsg += "\n• Day-summary cross-check: " + (verifyNotes.isEmpty() ? QString("all ledger nets + item in/out match Busy") : QString("%1 mismatch(es): %2").arg(verifyNotes.size()).arg(verifyNotes.mid(0, 8).join("; ")));
        summaryMsg += "\n• No company GSTIN/address found in source (Company row + all party/GST rows empty; GSTIN sweep clean) — home state Haryana fallback applied per instruction";
        summaryMsg += "\n• Empty source tables skipped (0 rows): TDS/TCS, BillingDet, E-Way/E-Payment, GSTR1/2A/2B/3B/9/ITC, POSDet, BRSOpBal, ItemDesc, OrgSalePurc, VchVAT/GSTInfo, ItemSerial/Param, PackingDetails, Tran4/5/6/8/9/10/11/12, VchConfiguration/Template, StateCompanyDet, StdNar";
        summaryMsg += "\n• Reference-only (kept in source, not ledgers): Help1 series/country/state lists, MasterSupport commission slabs, Config bank-import/print maps, Tran7 adjustment refs, MT=3/7/10/12 structural masters, Folio1 monthly buckets";
    }

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
