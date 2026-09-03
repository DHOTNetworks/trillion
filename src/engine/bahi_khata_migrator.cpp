#include "bahi_khata_migrator.h"
#include "../database_manager.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
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

static QString normalizeMdbPath(const QString& rawPath) {
    QString path = rawPath.trimmed();
    if (path.startsWith("file://", Qt::CaseInsensitive)) {
        QUrl url(path);
        if (url.isValid() && !url.toLocalFile().isEmpty()) {
            path = url.toLocalFile();
        } else {
            path = path.mid(7);
        }
    }
    // Remove leading slash for Windows drive letters like "/Z:/..." or "/C:/..."
    if (path.length() >= 3 && path[0] == '/' && path[1].isLetter() && path[2] == ':') {
        path = path.mid(1);
    }
    return QDir::toNativeSeparators(path);
}

static std::string cleanText(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = s.find_last_not_of(" \t\r\n");
    std::string sub = s.substr(first, last - first + 1);
    if (sub == "none" || sub == "null" || sub == "None" || sub == "NULL") return "";
    return sub;
}

static std::string toLowerStr(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return std::tolower(c); });
    return out;
}

static double parseDoubleVal(const std::string& s, double def = 0.0) {
    std::string clean = cleanText(s);
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

static int parseIntVal(const std::string& s, int def = 0) {
    std::string clean = cleanText(s);
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

static QString parseDateFormatted(const QString& raw) {
    QString s = raw.trimmed();
    if (s.isEmpty()) return "";
    QString firstPart = s.split(' ').first();
    QStringList parts = firstPart.split('/');
    if (parts.size() == 3) {
        int m = parts[0].toInt();
        int d = parts[1].toInt();
        int y = parts[2].toInt();
        if (y < 100) y += 2000;
        return QString("%1-%2-%3")
            .arg(y, 4, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(d, 2, 10, QChar('0'));
    }
    parts = firstPart.split('-');
    if (parts.size() == 3) {
        if (parts[0].length() == 4) return firstPart;
        int d = parts[0].toInt();
        int m = parts[1].toInt();
        int y = parts[2].toInt();
        if (y < 100) y += 2000;
        return QString("%1-%2-%3")
            .arg(y, 4, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(d, 2, 10, QChar('0'));
    }
    return firstPart;
}

static QString computeFinancialYear(const QString& isoDate) {
    if (isoDate.length() < 7) return "FY 2025-26";
    int year = isoDate.left(4).toInt();
    int month = isoDate.mid(5, 2).toInt();
    if (month >= 4) {
        return QString("FY %1-%2").arg(year).arg(QString::number(year + 1).right(2));
    } else {
        return QString("FY %1-%2").arg(year - 1).arg(QString::number(year).right(2));
    }
}

static QString mapVoucherTypeStr(const std::string& raw) {
    QString t = QString::fromStdString(raw).trimmed().toUpper();
    if (t == "SL" || t == "SALE" || t == "SALES" || t == "SALEVOUCHER" || t == "SLRN") return "Sales";
    if (t == "PU" || t == "PURC" || t == "PURCHASE" || t == "PURCHASEVOUCHER" || t == "PR" || t == "PRRN") return "Purchase";
    if (t == "PY" || t == "PAYMENT" || t == "PYMT" || t == "P" || t == "CHPT" || t == "CHQPYMT") return "Payment";
    if (t == "RC" || t == "RECEIPT" || t == "RCPT" || t == "R" || t == "CHRT" || t == "CHQRCPT") return "Receipt";
    if (t == "CN" || t == "CONTRA" || t == "CNTR" || t == "C") return "Contra";
    if (t == "JV" || t == "JOURNAL" || t == "JRNL" || t == "J") return "Journal";
    return "Journal";
}

static std::string classifyStockItemStr(const std::string& name) {
    std::string n = toLowerStr(name);
    static const std::regex rPaddy(R"(\bpaddy\b)", std::regex::icase);
    static const std::regex rHusk(R"(\bhusk\b)", std::regex::icase);
    static const std::regex rByProd(R"(\b(bran|husk|broken|nakku)\b)", std::regex::icase);
    static const std::regex rRice(R"(\brice\b)", std::regex::icase);
    static const std::regex rPack(R"(\b(bardana|bag|bags|thread|packing)\b)", std::regex::icase);
    static const std::regex rGen(R"(\b(machinery|dryer|parts|motor)\b)", std::regex::icase);

    if (std::regex_search(n, rPaddy)) {
        if (std::regex_search(n, rHusk)) return "By-Product";
        return "Raw Paddy";
    }
    if (std::regex_search(n, rByProd)) return "By-Product";
    if (std::regex_search(n, rRice)) return "Finished Rice";
    if (std::regex_search(n, rPack)) return "Packing Material";
    if (std::regex_search(n, rGen)) return "General Goods";
    return "General Goods";
}

#if HAS_LIBMDB
static std::string getField(const std::map<std::string, std::string>& m, const std::string& key, const std::string& def = "") {
    auto it = m.find(key);
    if (it != m.end()) {
        return it->second;
    }
    for (const auto& kv : m) {
        if (kv.first.size() == key.size()) {
            bool eq = true;
            for (size_t i = 0; i < key.size(); ++i) {
                if (std::tolower(static_cast<unsigned char>(kv.first[i])) != std::tolower(static_cast<unsigned char>(key[i]))) {
                    eq = false;
                    break;
                }
            }
            if (eq) return kv.second;
        }
    }
    return def;
}

static std::vector<std::map<std::string, std::string>> readTableRows(MdbHandle* mdb, const char* tableName) {
    std::vector<std::map<std::string, std::string>> result;
    MdbTableDef* table = mdb_read_table_by_name(mdb, const_cast<char*>(tableName), MDB_TABLE);
    if (!table) return result;

    mdb_read_columns(table);
    unsigned int numCols = table->num_cols;
    if (numCols == 0) {
        mdb_free_tabledef(table);
        return result;
    }

    std::vector<std::string> colNames(numCols);
    std::vector<std::vector<char>> colBuffers(numCols, std::vector<char>(4096, 0));

    for (unsigned int j = 0; j < numCols; j++) {
        MdbColumn* col = static_cast<MdbColumn*>(g_ptr_array_index(table->columns, j));
        colNames[j] = col->name;
        mdb_bind_column(table, j + 1, colBuffers[j].data(), nullptr);
    }

    mdb_rewind_table(table);
    while (mdb_fetch_row(table)) {
        std::map<std::string, std::string> row;
        for (unsigned int j = 0; j < numCols; j++) {
            row[colNames[j]] = colBuffers[j].data();
        }
        result.push_back(std::move(row));
    }

    mdb_free_tabledef(table);
    return result;
}
#endif

BahiKhataMigrator::BahiKhataMigrator(QObject* parent) : QObject(parent) {}
BahiKhataMigrator::~BahiKhataMigrator() = default;

void BahiKhataMigrator::updateProgress(int percent, const QString& status) {
    m_progressPercent = percent;
    m_statusText = status;
    emit progressChanged(percent);
    emit statusChanged(status);
    emit migrationProgress(percent, status);
    QCoreApplication::processEvents();
}

QString BahiKhataMigrator::parseMdbDate(const QString& rawDate) {
    return parseDateFormatted(rawDate);
}

QString BahiKhataMigrator::determineFinancialYear(const QString& isoDate) {
    return computeFinancialYear(isoDate);
}

QVariantMap BahiKhataMigrator::inspect_mdb_file(const QString& mdbFilePath) {
    QVariantMap result;
    result["valid"] = false;

    QString cleanPath = normalizeMdbPath(mdbFilePath);
    QFileInfo fi(cleanPath);
    if (!fi.exists()) {
        result["error"] = "File not found: " + cleanPath;
        return result;
    }

#if HAS_LIBMDB
    MdbHandle* mdb = mdb_open(cleanPath.toUtf8().constData(), MDB_NOFLAGS);
    if (!mdb) {
        result["error"] = "Unable to open Jet database file. Ensure it is a valid .mdb / .004 file.";
        return result;
    }

    GPtrArray* tables = mdb_read_catalog(mdb, MDB_TABLE);
    QVariantList tableNames;
    if (tables) {
        for (unsigned int i = 0; i < tables->len; i++) {
            MdbCatalogEntry* entry = static_cast<MdbCatalogEntry*>(g_ptr_array_index(tables, i));
            if (entry && entry->object_type == MDB_TABLE) {
                tableNames.append(QString::fromUtf8(entry->object_name));
            }
        }
    }

    result["valid"] = true;
    result["filePath"] = cleanPath;
    result["fileName"] = fi.fileName();
    result["fileSize"] = fi.size();
    result["tables"] = tableNames;
    result["tableCount"] = tableNames.size();

    auto stockItems = readTableRows(mdb, "StockItems");
    auto ledgers = readTableRows(mdb, "Ledgers");
    auto stockTx = readTableRows(mdb, "StockTransactions");
    auto milling = readTableRows(mdb, "MillingVouchers");

    result["stockItemsCount"] = static_cast<int>(stockItems.size());
    result["ledgersCount"] = static_cast<int>(ledgers.size());
    result["stockTxCount"] = static_cast<int>(stockTx.size());
    result["millingCount"] = static_cast<int>(milling.size());

    auto compRows = readTableRows(mdb, "CompanyInfo");
    if (!compRows.empty()) {
        const auto& c = compRows.back();
        result["companyName"] = QString::fromStdString(getField(c, "CompanyName"));
        result["firmType"] = QString::fromStdString(getField(c, "FirmType"));
        result["gstin"] = QString::fromStdString(getField(c, "GSTIN"));
        result["pan"] = QString::fromStdString(getField(c, "PAN_No"));
        result["station"] = QString::fromStdString(getField(c, "MyStation"));
        result["state"] = QString::fromStdString(getField(c, "MySTATE"));
        result["address"] = QString::fromStdString(getField(c, "Address"));
        result["phone"] = QString::fromStdString(getField(c, "Phone_O"));
        result["business"] = QString::fromStdString(getField(c, "Business"));
        result["fyFrom"] = QString::fromStdString(getField(c, "AccYearFrom"));
        result["fyTo"] = QString::fromStdString(getField(c, "AccYearTo"));
    }

    mdb_close(mdb);
#else
    result["error"] = "libmdb is not compiled into this build.";
#endif
    return result;
}

struct PartyDetail {
    int id = 0;
    std::string name;
    std::string group_name;
    std::string party_type;
    int legacy_id = 0;
};

struct ItemDetail {
    int id = 0;
    std::string name;
    std::string hsn;
    double gst_rate = 5.0;
    double packing_kg = 50.0;
};

struct StockMovementEntry {
    int item_id = 0;
    std::string item_name;
    std::string hsn_code;
    int bags = 0;
    double weight_qtl = 0.0;
    double rate = 0.0;
    double amount = 0.0;
    double taxable_amount = 0.0;
    double gst_pct = 5.0;
    double cgst = 0.0;
    double sgst = 0.0;
    double igst = 0.0;
    double cess = 0.0;
};

struct LogisticsEntry {
    std::string vehicle_no;
    std::string gr_no;
    std::string driver;
    std::string eway_bill_no;
    std::string shipping_address;
    std::string po_no;
    int distance = 0;
    std::string irn_no;
    std::string bill_time;
};

struct ResolvedVoucherParty {
    std::string primary_party;
    int primary_party_id = 0;
    int primary_party_legacy_code = 0;
    std::string opposing_account = "Cash";
};

static ResolvedVoucherParty resolveVoucherParty(
    const QString& vType,
    const std::vector<std::map<std::string, std::string>>& rows,
    const std::map<int, PartyDetail>& ledgerDetailMap,
    const std::map<int, std::string>& ledgerCodeMap
) {
    ResolvedVoucherParty res;
    std::string bankOrCash;
    int bankOrCashId = 0;
    std::string tradingOrExp;
    int tradingOrExpId = 0;

    for (const auto& r : rows) {
        int acCode = parseIntVal(getField(r, "AccountCode"));
        auto it = ledgerDetailMap.find(acCode);
        std::string lName = (it != ledgerDetailMap.end()) ? it->second.name : ("Party #" + std::to_string(acCode));
        int lId = (it != ledgerDetailMap.end()) ? it->second.id : 0;
        std::string grpName = (it != ledgerDetailMap.end()) ? it->second.group_name : "Sundry Debtors";
        std::string gLower = toLowerStr(grpName);

        bool isParty = false;
        const char* partyKeywords[] = {
            "debtor", "debitor", "creditor", "party", "parties", "buyer",
            "seller", "vendor", "farmer", "customer", "supplier", "broker",
            "zimidar", "thekdar", "mandi", "employee"
        };
        for (const char* kw : partyKeywords) {
            if (gLower.find(kw) != std::string::npos) {
                isParty = true;
                break;
            }
        }

        std::string lLower = toLowerStr(lName);
        bool isBankOrCash = (gLower.find("bank") != std::string::npos || gLower.find("cash") != std::string::npos ||
                             lLower.find("bank") != std::string::npos || lLower.find("cash") != std::string::npos ||
                             lLower.find("c/c") != std::string::npos || lLower.find(" cc ") != std::string::npos ||
                             lLower.find(" cc") != std::string::npos || lLower.find("c/a") != std::string::npos ||
                             lLower.find("overdraft") != std::string::npos);

        if (isParty) {
            if (res.primary_party.empty()) {
                res.primary_party = lName;
                res.primary_party_id = lId;
                res.primary_party_legacy_code = acCode;
            }
        } else if (isBankOrCash) {
            bankOrCash = lName;
            bankOrCashId = lId;
        } else if (gLower.find("trading") != std::string::npos || gLower.find("purchase") != std::string::npos || gLower.find("sale") != std::string::npos) {
            tradingOrExp = lName;
            tradingOrExpId = lId;
        }
    }

    if (res.primary_party.empty()) {
        if (!bankOrCash.empty()) {
            res.primary_party = bankOrCash;
            res.primary_party_id = bankOrCashId;
        } else if (!tradingOrExp.empty()) {
            res.primary_party = tradingOrExp;
            res.primary_party_id = tradingOrExpId;
        } else if (!rows.empty()) {
            int firstCode = parseIntVal(getField(rows[0], "AccountCode"));
            auto itCode = ledgerCodeMap.find(firstCode);
            res.primary_party = (itCode != ledgerCodeMap.end()) ? itCode->second : "Trade Account";
            auto itDetail = ledgerDetailMap.find(firstCode);
            res.primary_party_id = (itDetail != ledgerDetailMap.end()) ? itDetail->second.id : 0;
            res.primary_party_legacy_code = firstCode;
        }
    }

    if (vType == "Sales") {
        res.opposing_account = bankOrCash.empty() ? "Sales Accounts" : bankOrCash;
    } else if (vType == "Purchase") {
        res.opposing_account = bankOrCash.empty() ? "Purchase Accounts" : bankOrCash;
    } else {
        res.opposing_account = bankOrCash.empty() ? "Cash" : bankOrCash;
    }

    return res;
}

bool BahiKhataMigrator::migrate_mdb_file(const QString& mdbFilePath) {
    QString cleanPath = normalizeMdbPath(mdbFilePath);
    QFileInfo fi(cleanPath);
    if (!fi.exists()) {
        emit migrationFinished(false, "MDB file does not exist: " + cleanPath);
        return false;
    }

#if HAS_LIBMDB
    m_isMigrating = true;
    emit migratingChanged();

    updateProgress(5, "Opening Bahi Khata Database: " + fi.fileName());
    MdbHandle* mdb = mdb_open(cleanPath.toUtf8().constData(), MDB_NOFLAGS);
    if (!mdb) {
        m_isMigrating = false;
        emit migratingChanged();
        emit migrationFinished(false, "Failed to open MDB database.");
        return false;
    }

    updateProgress(10, "Extracting Raw JetDB Tables...");
    auto compRows = readTableRows(mdb, "CompanyInfo");
    auto groupRows = readTableRows(mdb, "Groups");
    auto ledgerRows = readTableRows(mdb, "Ledgers");
    auto itemRows = readTableRows(mdb, "StockItems");
    auto stockTransRows = readTableRows(mdb, "StockTransactions");
    auto transportRows = readTableRows(mdb, "SaleTransportationDetail");
    auto transRows = readTableRows(mdb, "Transactions");
    auto millingRows = readTableRows(mdb, "MillingVouchers");
    auto customRows = readTableRows(mdb, "CustomClosingStocks");

    auto& db = DatabaseManager::instance();
    db.executeNonQuery("PRAGMA foreign_keys = OFF;");
    db.beginTransaction();

    db.executeNonQuery("DELETE FROM sales_invoices;");
    db.executeNonQuery("DELETE FROM purchase_invoices;");
    db.executeNonQuery("DELETE FROM paddy_arrivals;");
    db.executeNonQuery("DELETE FROM milling_voucher_items;");
    db.executeNonQuery("DELETE FROM milling_batches;");
    db.executeNonQuery("DELETE FROM stock_transactions;");
    db.executeNonQuery("DELETE FROM custom_closing_stocks;");
    db.executeNonQuery("DELETE FROM vouchers;");
    db.executeNonQuery("DELETE FROM inventory;");
    db.executeNonQuery("DELETE FROM stock_items;");
    db.executeNonQuery("DELETE FROM parties;");
    db.executeNonQuery("DELETE FROM account_groups;");
    db.executeNonQuery("DELETE FROM financial_years;");
    db.executeNonQuery("DELETE FROM company_info;");

    // =========================================================
    // PASS 0.1: COMPANY INFO (CompanyInfo)
    // =========================================================
    updateProgress(12, "Migrating Company Info...");
    if (!compRows.empty()) {
        const auto& c = compRows.back();
        db.executeNonQuery(
            "INSERT INTO company_info ("
            "company_name, firm_type, business_type, address, city, state, state_code, pincode, "
            "phone, mobile, email, gstin, pan_no, fssai_no, ml_no, "
            "bank_name, bank_account, ifsc_code, books_from, acc_year_from, acc_year_to, data_file_source"
            ") VALUES (?, ?, ?, ?, ?, ?, '06', '125055', ?, ?, '', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                QString::fromStdString(getField(c, "CompanyName")),
                QString::fromStdString(getField(c, "FirmType")),
                QString::fromStdString(getField(c, "Business")),
                QString::fromStdString(getField(c, "Address")),
                QString::fromStdString(getField(c, "MyStation")),
                QString::fromStdString(getField(c, "MySTATE")),
                QString::fromStdString(getField(c, "Phone_O")),
                QString::fromStdString(getField(c, "Phone_F", getField(c, "Mobile1"))),
                QString::fromStdString(getField(c, "GSTIN")),
                QString::fromStdString(getField(c, "PAN_No")),
                "10822019000152",
                QString::fromStdString(getField(c, "ML_No")),
                QString::fromStdString(getField(c, "Mobile2")),
                QString::fromStdString(getField(c, "Bank2")),
                QString::fromStdString(getField(c, "Bank3")),
                QString::fromStdString(getField(c, "BooksBeginingFrom")),
                parseDateFormatted(QString::fromStdString(getField(c, "AccYearFrom"))),
                parseDateFormatted(QString::fromStdString(getField(c, "AccYearTo"))),
                fi.fileName()
            }
        );
    }

    // =========================================================
    // PASS 0.5: FINANCIAL YEARS (CompanyInfo)
    // =========================================================
    updateProgress(15, "Migrating Financial Years...");
    std::set<QString> seenFys;
    std::map<QString, int> fyNameToId;
    int latestYear = 0;
    QString latestFyName;

    for (const auto& comp : compRows) {
        QString fromD = parseDateFormatted(QString::fromStdString(getField(comp, "AccYearFrom")));
        QString toD = parseDateFormatted(QString::fromStdString(getField(comp, "AccYearTo")));
        if (fromD.isEmpty() || toD.isEmpty()) continue;

        QString fyName = computeFinancialYear(fromD);
        if (seenFys.find(fyName) == seenFys.end()) {
            seenFys.insert(fyName);
            int startYear = fromD.left(4).toInt();
            if (startYear >= latestYear) {
                latestYear = startYear;
                latestFyName = fyName;
            }
            db.executeNonQuery(
                "INSERT INTO financial_years (year_name, start_date, end_date, is_active, is_locked) "
                "VALUES (?, ?, ?, 0, 0);",
                {fyName, fromD, toD}
            );
            fyNameToId[fyName] = static_cast<int>(db.lastInsertedId());
        }
    }

    // Scan transactions for any additional fiscal years
    for (const auto& t : transRows) {
        QString vDate = parseDateFormatted(QString::fromStdString(getField(t, "VoucherDate")));
        if (vDate.length() >= 7) {
            QString fyName = computeFinancialYear(vDate);
            if (seenFys.find(fyName) == seenFys.end()) {
                seenFys.insert(fyName);
                int y = vDate.left(4).toInt();
                int m = vDate.mid(5, 2).toInt();
                int sYear = (m >= 4) ? y : y - 1;
                QString fromD = QString("%1-04-01").arg(sYear);
                QString toD = QString("%1-03-31").arg(sYear + 1);
                db.executeNonQuery(
                    "INSERT INTO financial_years (year_name, start_date, end_date, is_active, is_locked) "
                    "VALUES (?, ?, ?, 0, 0);",
                    {fyName, fromD, toD}
                );
                fyNameToId[fyName] = static_cast<int>(db.lastInsertedId());
                if (sYear >= latestYear) {
                    latestYear = sYear;
                    latestFyName = fyName;
                }
            }
        }
    }

    if (!latestFyName.isEmpty()) {
        db.executeNonQuery("UPDATE financial_years SET is_active = 1 WHERE year_name = ?;", {latestFyName});
    }

    // Refresh FY IDs from DB
    QVariantList fyList = db.executeQuery("SELECT id, year_name FROM financial_years;");
    for (const auto& rowVar : fyList) {
        QVariantMap r = rowVar.toMap();
        fyNameToId[r.value("year_name").toString()] = r.value("id").toInt();
    }

    // =========================================================
    // PASS 1: ACCOUNT GROUPS
    // =========================================================
    updateProgress(22, QString("Migrating %1 Account Groups...").arg(groupRows.size()));
    db.executeNonQuery("DELETE FROM account_groups;");
    std::map<int, std::string> groupCodeMap;

    for (const auto& g : groupRows) {
        std::string gName = cleanText(getField(g, "GroupName"));
        if (gName.empty()) continue;
        int code1 = parseIntVal(getField(g, "Code1st"));
        int extractBs = parseIntVal(getField(g, "ExtractInBalanceSheet", "1"), 1);

        std::string nature = "Assets";
        std::string gLower = toLowerStr(gName);
        if (gLower.find("debtor") != std::string::npos || gLower.find("bank") != std::string::npos ||
            gLower.find("cash") != std::string::npos || gLower.find("asset") != std::string::npos ||
            gLower.find("receivable") != std::string::npos || gLower.find("deposit") != std::string::npos) {
            nature = "Assets";
        } else if (gLower.find("creditor") != std::string::npos || gLower.find("loan") != std::string::npos ||
                   gLower.find("liability") != std::string::npos || gLower.find("payable") != std::string::npos ||
                   gLower.find("capital") != std::string::npos || gLower.find("duty") != std::string::npos ||
                   gLower.find("tax") != std::string::npos || gLower.find("gst") != std::string::npos) {
            nature = "Liabilities";
        } else if (gLower.find("sale") != std::string::npos || gLower.find("revenue") != std::string::npos ||
                   gLower.find("income") != std::string::npos || gLower.find("direct income") != std::string::npos) {
            nature = "Income";
        } else if (gLower.find("purchase") != std::string::npos || gLower.find("expense") != std::string::npos ||
                   gLower.find("hamali") != std::string::npos || gLower.find("freight") != std::string::npos ||
                   gLower.find("labour") != std::string::npos || gLower.find("exp") != std::string::npos) {
            nature = "Expense";
        }

        db.executeNonQuery(
            "INSERT INTO account_groups (name, parent_group_name, nature, description, extract_in_balance_sheet, is_system) "
            "VALUES (?, 'Primary / Root Group', ?, ?, ?, 0);",
            {
                QString::fromStdString(gName),
                QString::fromStdString(nature),
                QString("Legacy Group Code #%1").arg(code1),
                extractBs
            }
        );
        groupCodeMap[code1] = gName;
    }

    // =========================================================
    // PASS 2: LEDGERS (PARTIES & ACCOUNTS)
    // =========================================================
    updateProgress(30, QString("Migrating %1 Ledgers & Parties...").arg(ledgerRows.size()));
    db.executeNonQuery("DELETE FROM parties;");
    std::map<int, std::string> ledgerCodeMap;
    std::map<int, PartyDetail> ledgerDetailMap;
    std::map<int, int> legacyIdToPartyId;

    for (const auto& l : ledgerRows) {
        std::string lName = cleanText(getField(l, "LedgerName"));
        if (lName.empty()) continue;

        int legacyId = parseIntVal(getField(l, "Code1st"));
        std::string alias = cleanText(getField(l, "LedgerAlais"));
        std::string prefix = cleanText(getField(l, "Prefix", "M/s"));
        if (prefix.empty()) prefix = "M/s";

        int gCode = parseIntVal(getField(l, "GroupCode"));
        std::string groupName = groupCodeMap.count(gCode) ? groupCodeMap[gCode] : "Sundry Debtors";

        double opBal = parseDoubleVal(getField(l, "OpeningBal"));
        std::string balType = cleanText(getField(l, "OpeningType", "Dr"));
        if (balType.empty()) balType = "Dr";

        std::string mailingName = cleanText(getField(l, "MailingName"));
        if (mailingName.empty()) mailingName = lName;
        std::string address = cleanText(getField(l, "MailingAdd"));
        std::string city = cleanText(getField(l, "Station"));
        std::string district = cleanText(getField(l, "Distt"));
        std::string state = cleanText(getField(l, "STATE", "Haryana"));
        if (state.empty()) state = "Haryana";
        std::string stateCode = cleanText(getField(l, "PartyState"));
        std::string pincode = cleanText(getField(l, "PartyPINcode"));
        std::string route = cleanText(getField(l, "Route"));

        std::string phone = cleanText(getField(l, "Phone_O"));
        std::string mobile = cleanText(getField(l, "MobNoForSMS"));
        if (mobile.empty()) mobile = phone;
        std::string whatsapp = cleanText(getField(l, "WhatsappNo"));
        if (whatsapp.empty()) whatsapp = mobile;
        std::string email = cleanText(getField(l, "Email"));
        std::string contactPerson = cleanText(getField(l, "ConcernedPerson"));
        if (contactPerson.empty()) contactPerson = lName;

        std::string gstin = cleanText(getField(l, "GSTIN"));
        std::string pan = cleanText(getField(l, "IncomeTaxNo"));
        std::string tan = cleanText(getField(l, "PartyTAN"));
        std::string aadhaar = cleanText(getField(l, "AadharNo"));
        std::string gstPartyType = cleanText(getField(l, "GSTPartyType"));
        if (gstPartyType.empty()) gstPartyType = gstin.empty() ? "Unregistered" : "Registered";

        std::string bankName = cleanText(getField(l, "BankName"));
        std::string bankAccount = cleanText(getField(l, "BankAccount"));
        std::string ifscCode = cleanText(getField(l, "IFSCCode"));

        double creditLimit = parseDoubleVal(getField(l, "CreditLimit"));
        double interestRate = parseDoubleVal(getField(l, "InterestRate"));
        double commissionRate = parseDoubleVal(getField(l, "Percentage"));
        std::string commissionOn = cleanText(getField(l, "CommnCalcOn"));

        std::string applyTcsStr = toLowerStr(getField(l, "ApplyTCSForParty"));
        int applyTcs = (applyTcsStr == "true" || applyTcsStr == "1" || applyTcsStr == "yes") ? 1 : 0;
        std::string tcsExemptStr = toLowerStr(getField(l, "TCSNotApplyInSale"));
        int tcsExempt = (tcsExemptStr == "true" || tcsExemptStr == "1" || tcsExemptStr == "yes") ? 1 : 0;

        std::string partyType = cleanText(getField(l, "PartyType"));
        if (partyType.empty()) {
            partyType = (groupName.find("Debtor") != std::string::npos) ? "Buyer" :
                        ((groupName.find("Creditor") != std::string::npos) ? "Vendor" : "Merchant");
        }
        std::string specialType = cleanText(getField(l, "SpecialPartyType"));
        if (specialType.empty()) {
            specialType = (partyType.find("Buyer") != std::string::npos) ? "Rice Buyer" : "Paddy Seller";
        }

        db.executeNonQuery(
            "INSERT INTO parties ("
            "name, alias, prefix, group_name, party_type, special_type, "
            "opening_balance, balance_type, mailing_name, address, city, district, state, state_code, pincode, route, "
            "mobile, whatsapp, phone, email, contact_person, pan, aadhaar, tan, gstin, gst_party_type, "
            "bank_name, bank_account, ifsc_code, credit_limit, credit_days, interest_rate, commission_rate, commission_on, "
            "apply_tcs, tcs_exempt, legacy_id) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 30, ?, ?, ?, ?, ?, ?);",
            {
                QString::fromStdString(lName),
                QString::fromStdString(alias),
                QString::fromStdString(prefix),
                QString::fromStdString(groupName),
                QString::fromStdString(partyType),
                QString::fromStdString(specialType),
                opBal,
                QString::fromStdString(balType),
                QString::fromStdString(mailingName),
                QString::fromStdString(address),
                QString::fromStdString(city),
                QString::fromStdString(district),
                QString::fromStdString(state),
                QString::fromStdString(stateCode),
                QString::fromStdString(pincode),
                QString::fromStdString(route),
                QString::fromStdString(mobile),
                QString::fromStdString(whatsapp),
                QString::fromStdString(phone),
                QString::fromStdString(email),
                QString::fromStdString(contactPerson),
                QString::fromStdString(pan),
                QString::fromStdString(aadhaar),
                QString::fromStdString(tan),
                QString::fromStdString(gstin),
                QString::fromStdString(gstPartyType),
                QString::fromStdString(bankName),
                QString::fromStdString(bankAccount),
                QString::fromStdString(ifscCode),
                creditLimit,
                interestRate,
                commissionRate,
                QString::fromStdString(commissionOn),
                applyTcs,
                tcsExempt,
                legacyId
            }
        );

        int partyDbId = static_cast<int>(db.lastInsertedId());
        legacyIdToPartyId[legacyId] = partyDbId;
        ledgerCodeMap[legacyId] = lName;
        ledgerDetailMap[legacyId] = {partyDbId, lName, groupName, partyType, legacyId};
    }

    // =========================================================
    // PASS 3: STOCK ITEMS & INVENTORY
    // =========================================================
    updateProgress(40, QString("Migrating %1 Stock Items & Inventory...").arg(itemRows.size()));
    db.executeNonQuery("DELETE FROM stock_items;");
    db.executeNonQuery("DELETE FROM inventory;");
    std::map<int, ItemDetail> itemCodeMap;

    for (const auto& item : itemRows) {
        std::string iName = cleanText(getField(item, "ItemName"));
        if (iName.empty()) continue;

        int legacyCode = parseIntVal(getField(item, "Code1st"));
        std::string code = cleanText(getField(item, "Code1st"));
        std::string hsn = cleanText(getField(item, "HSNCode", "1006"));
        if (hsn.empty()) hsn = "1006";

        double purRate = parseDoubleVal(getField(item, "PurcRate"));
        double saleRate = parseDoubleVal(getField(item, "SaleRate"));
        double mrp = parseDoubleVal(getField(item, "MRP"));
        double discount = parseDoubleVal(getField(item, "Discount"));

        std::string gstSlab = cleanText(getField(item, "GSTRateSlab"));
        double gstRate = !gstSlab.empty() ? parseDoubleVal(gstSlab) : parseDoubleVal(getField(item, "VAT", "5.0"), 5.0);
        double cessRate = parseDoubleVal(getField(item, "CessRate"));
        double packingKg = parseDoubleVal(getField(item, "Packing", "50.0"), 50.0);

        int opBags = parseIntVal(getField(item, "OpeningBags"));
        double opQty = parseDoubleVal(getField(item, "OpeningQty"));
        double opRate = parseDoubleVal(getField(item, "OpeningRate"));
        if (opRate <= 0.0) opRate = purRate;
        double opVal = parseDoubleVal(getField(item, "OpeningValue"));
        if (opVal <= 0.0) opVal = std::round(opQty * opRate * 100.0) / 100.0;

        double damiRate = parseDoubleVal(getField(item, "DamiRate"));
        double marketFeeRate = parseDoubleVal(getField(item, "MarketFeeRate"));
        double hrdfRate = parseDoubleVal(getField(item, "HRDFRate"));

        std::string millStr = toLowerStr(getField(item, "MillingItem"));
        int isMillingItem = (millStr == "true" || millStr == "1" || millStr == "yes") ? 1 : 0;

        std::string itemType = classifyStockItemStr(iName);

        db.executeNonQuery(
            "INSERT INTO stock_items ("
            "name, code, item_type, goods_type, company_name, category_name, unit, "
            "purchase_rate, sale_rate, mrp, discount, hsn_code, gst_rate, cess_rate, packing_kg, "
            "opening_bags, opening_qty, opening_rate, opening_value, "
            "purchase_ledger, sale_ledger, stock_ledger, is_milling_item, include_in_trading, calculate_stock, "
            "dami_rate, market_fee_rate, hrdf_rate, legacy_code) "
            "VALUES (?, ?, ?, 'Goods', 'Mill Master', ?, 'Qtl', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'Purchase Accounts', 'Sales Accounts', 'Stock-in-Hand', ?, 1, 1, ?, ?, ?, ?);",
            {
                QString::fromStdString(iName),
                QString::fromStdString(code),
                QString::fromStdString(itemType),
                QString::fromStdString(itemType),
                purRate,
                saleRate,
                mrp,
                discount,
                QString::fromStdString(hsn),
                gstRate,
                cessRate,
                packingKg,
                opBags,
                opQty,
                opRate,
                opVal,
                isMillingItem,
                damiRate,
                marketFeeRate,
                hrdfRate,
                legacyCode
            }
        );

        int itemDbId = static_cast<int>(db.lastInsertedId());
        itemCodeMap[legacyCode] = {itemDbId, iName, hsn, gstRate, packingKg};

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

    // =========================================================
    // PASS 4: STOCK MOVEMENTS & LOGISTICS CACHING
    // =========================================================
    updateProgress(50, "Caching Stock Movements & Logistics...");
    std::map<std::string, std::vector<StockMovementEntry>> stockTransMap;

    for (const auto& st : stockTransRows) {
        std::string vNo = cleanText(getField(st, "VoucherNumber"));
        QString vType = mapVoucherTypeStr(getField(st, "TransType"));
        QString vDate = parseDateFormatted(QString::fromStdString(getField(st, "VoucherDate")));
        std::string key = QString("%1-%2-%3").arg(vType).arg(QString::fromStdString(vNo)).arg(vDate).toStdString();

        int iCode = parseIntVal(getField(st, "ItemCode"));
        auto itItem = itemCodeMap.find(iCode);
        int itemId = (itItem != itemCodeMap.end()) ? itItem->second.id : 1;
        std::string itemName = (itItem != itemCodeMap.end()) ? itItem->second.name : "Rice Basmati(Non Branded)";
        std::string hsn = (itItem != itemCodeMap.end()) ? itItem->second.hsn : "1006";
        double defaultGst = (itItem != itemCodeMap.end()) ? itItem->second.gst_rate : 5.0;

        int bags = parseIntVal(getField(st, "Bags"));
        double wt = parseDoubleVal(getField(st, "Weight"));
        double rate = parseDoubleVal(getField(st, "Rate"));
        double amt = parseDoubleVal(getField(st, "Amount"));
        double taxable = parseDoubleVal(getField(st, "TaxableAmount"));
        if (taxable <= 0.0) taxable = amt;

        double vatcst = parseDoubleVal(getField(st, "VATCST"));
        double gstPct = (vatcst > 0.0) ? vatcst : defaultGst;
        double cgst = parseDoubleVal(getField(st, "CGSTTax"));
        double sgst = parseDoubleVal(getField(st, "SGSTTax"));
        double igst = parseDoubleVal(getField(st, "IGSTTax"));
        double cess = parseDoubleVal(getField(st, "CESSTax"));

        StockMovementEntry entry;
        entry.item_id = itemId;
        entry.item_name = itemName;
        entry.hsn_code = hsn;
        entry.bags = bags;
        entry.weight_qtl = wt;
        entry.rate = rate;
        entry.amount = amt;
        entry.taxable_amount = taxable;
        entry.gst_pct = gstPct;
        entry.cgst = cgst;
        entry.sgst = sgst;
        entry.igst = igst;
        entry.cess = cess;

        stockTransMap[key].push_back(entry);
    }

    std::map<std::string, LogisticsEntry> transportMap;
    for (const auto& tr : transportRows) {
        std::string vNo = cleanText(getField(tr, "VoucherNumber"));
        QString vType = mapVoucherTypeStr(getField(tr, "TransType"));
        QString vDate = parseDateFormatted(QString::fromStdString(getField(tr, "VoucherDate")));
        std::string key = QString("%1-%2-%3").arg(vType).arg(QString::fromStdString(vNo)).arg(vDate).toStdString();

        LogisticsEntry log;
        log.vehicle_no = cleanText(getField(tr, "VehicleNo"));
        log.gr_no = cleanText(getField(tr, "GRNo"));
        log.driver = cleanText(getField(tr, "DriverName"));
        log.eway_bill_no = cleanText(getField(tr, "EWayBillNo"));
        log.shipping_address = cleanText(getField(tr, "ShippingAddress"));
        log.po_no = cleanText(getField(tr, "PurchaseOrderNo"));
        log.distance = parseIntVal(getField(tr, "Distance"));
        log.irn_no = cleanText(getField(tr, "IRNNo"));
        log.bill_time = cleanText(getField(tr, "DispatchTime"));

        transportMap[key] = log;
    }

    // =========================================================
    // PASS 5: TRANSACTIONS & VOUCHERS (INTELLIGENT PARTY RESOLUTION)
    // =========================================================
    updateProgress(60, "Migrating Transactions & Reconstructing Double-Entry Invoices...");
    std::map<std::string, std::vector<std::map<std::string, std::string>>> voucherGroups;
    for (const auto& r : transRows) {
        std::string vNo = cleanText(getField(r, "VoucherNumber"));
        if (vNo.empty()) continue;
        QString vType = mapVoucherTypeStr(getField(r, "TransType"));
        QString vDate = parseDateFormatted(QString::fromStdString(getField(r, "VoucherDate")));
        std::string key = QString("%1-%2-%3").arg(vType).arg(QString::fromStdString(vNo)).arg(vDate).toStdString();
        voucherGroups[key].push_back(r);
    }

    db.executeNonQuery("DELETE FROM vouchers;");
    db.executeNonQuery("DELETE FROM sales_invoices;");
    db.executeNonQuery("DELETE FROM purchase_invoices;");
    db.executeNonQuery("DELETE FROM paddy_arrivals;");

    int salesInvoiceCount = 0;
    int purchaseInvoiceCount = 0;

    for (const auto& pair : voucherGroups) {
        const std::string& key = pair.first;
        const auto& rows = pair.second;
        if (rows.empty()) continue;

        const auto& firstR = rows[0];
        std::string vNo = cleanText(getField(firstR, "VoucherNumber"));
        std::string rawType = cleanText(getField(firstR, "TransType"));
        QString vType = mapVoucherTypeStr(rawType);
        QString vDate = parseDateFormatted(QString::fromStdString(getField(firstR, "VoucherDate")));
        QString fyVal = computeFinancialYear(vDate);
        int fyId = fyNameToId.count(fyVal) ? fyNameToId[fyVal] : 1;

        std::string narration = cleanText(getField(firstR, "Narration"));
        if (narration.empty()) narration = cleanText(getField(firstR, "Narrtn"));

        std::string invNo = cleanText(getField(firstR, "InvoiceNo"));
        if (invNo.empty()) invNo = cleanText(getField(firstR, "TaxInvoiceNo"));
        if (invNo.empty()) invNo = QString("VCH-%1").arg(QString::fromStdString(vNo)).toStdString();

        ResolvedVoucherParty partyRes = resolveVoucherParty(vType, rows, ledgerDetailMap, ledgerCodeMap);

        double totalAmount = 0.0;
        double partyAmount = 0.0;
        for (const auto& itemRow : rows) {
            double amt = parseDoubleVal(getField(itemRow, "Amount"));
            int acCode = parseIntVal(getField(itemRow, "AccountCode"));
            totalAmount = std::max(totalAmount, amt);
            if (partyRes.primary_party_legacy_code != 0 && acCode == partyRes.primary_party_legacy_code) {
                partyAmount = amt;
            }
        }
        if (partyAmount <= 0.0) partyAmount = totalAmount;

        auto itLog = transportMap.find(key);
        std::string vehicleNo = (itLog != transportMap.end() && !itLog->second.vehicle_no.empty()) ? itLog->second.vehicle_no : cleanText(getField(firstR, "VehicleNo"));
        std::string grNo = (itLog != transportMap.end() && !itLog->second.gr_no.empty()) ? itLog->second.gr_no : cleanText(getField(firstR, "GRNo"));
        std::string driverName = (itLog != transportMap.end() && !itLog->second.driver.empty()) ? itLog->second.driver : cleanText(getField(firstR, "DriverName"));
        std::string ewayBill = (itLog != transportMap.end() && !itLog->second.eway_bill_no.empty()) ? itLog->second.eway_bill_no : cleanText(getField(firstR, "EWayBillNo"));
        std::string brokerName = cleanText(getField(firstR, "BrokerName"));
        std::string farmerName = cleanText(getField(firstR, "ZimidarName"));

        QString vchNumStr = !rawType.empty() ? QString("%1-%2").arg(QString::fromStdString(rawType)).arg(QString::fromStdString(vNo)) :
                                               QString("%1-%2").arg(vType).arg(QString::fromStdString(vNo));

        // 1. Insert Consolidated Double-Entry Voucher
        db.executeNonQuery(
            "INSERT INTO vouchers ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, legacy_type, "
            "ledger_id, party_name, account_type, amount, narration, "
            "vehicle_no, gr_no, driver_name, eway_bill_no, broker_name, farmer_name) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                fyId,
                fyVal,
                vchNumStr,
                vDate,
                vType,
                QString::fromStdString(rawType),
                partyRes.primary_party_id,
                QString::fromStdString(partyRes.primary_party),
                QString::fromStdString(partyRes.opposing_account),
                partyAmount,
                QString::fromStdString(narration),
                QString::fromStdString(vehicleNo),
                QString::fromStdString(grNo),
                QString::fromStdString(driverName),
                QString::fromStdString(ewayBill),
                QString::fromStdString(brokerName),
                QString::fromStdString(farmerName)
            }
        );

        // 2. Reconstruct Invoices with Line Items
        auto itStockLines = stockTransMap.find(key);
        int itemId = 1;
        std::string itemName = (vType == "Sales") ? "Rice Basmati(Non Branded)" : "Paddy Basmati";
        std::string hsnCode = "1006";
        int bagCount = 0;
        double weightQtl = (partyAmount > 0.0) ? (partyAmount / 6500.0) : 0.0;
        double ratePerQtl = 6500.0;
        double gstPct = 5.0;
        double cgstAmt = 0.0, sgstAmt = 0.0, igstAmt = 0.0;
        double taxableAmt = partyAmount;

        if (itStockLines != stockTransMap.end() && !itStockLines->second.empty()) {
            const auto& lines = itStockLines->second;
            itemId = lines[0].item_id;
            itemName = lines[0].item_name;
            hsnCode = lines[0].hsn_code;
            gstPct = lines[0].gst_pct;
            ratePerQtl = lines[0].rate;

            weightQtl = 0.0;
            taxableAmt = 0.0;
            for (const auto& line : lines) {
                bagCount += line.bags;
                weightQtl += line.weight_qtl;
                taxableAmt += line.taxable_amount;
                cgstAmt += line.cgst;
                sgstAmt += line.sgst;
                igstAmt += line.igst;
            }
            if (taxableAmt <= 0.0) taxableAmt = partyAmount;
            if (ratePerQtl <= 0.0 && weightQtl > 0.0) {
                ratePerQtl = std::round((taxableAmt / weightQtl) * 100.0) / 100.0;
            }
        }

        if (vType == "Sales") {
            db.executeNonQuery(
                "INSERT INTO sales_invoices ("
                "fy_id, financial_year, invoice_no, invoice_date, customer_id, customer_name, "
                "item_id, item_name, hsn_code, "
                "bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, "
                "cgst_amount, sgst_amount, igst_amount, "
                "total_amount, payment_mode, vehicle_no, eway_bill_no, narration, gr_no, driver, broker_name) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'Credit', ?, ?, ?, ?, ?, ?);",
                {
                    fyId,
                    fyVal,
                    QString::fromStdString(invNo),
                    vDate,
                    partyRes.primary_party_id,
                    QString::fromStdString(partyRes.primary_party),
                    itemId,
                    QString::fromStdString(itemName),
                    QString::fromStdString(hsnCode),
                    bagCount,
                    weightQtl,
                    ratePerQtl,
                    taxableAmt,
                    gstPct,
                    cgstAmt,
                    sgstAmt,
                    igstAmt,
                    partyAmount,
                    QString::fromStdString(vehicleNo),
                    QString::fromStdString(ewayBill),
                    QString::fromStdString(narration),
                    QString::fromStdString(grNo),
                    QString::fromStdString(driverName),
                    QString::fromStdString(brokerName)
                }
            );
            salesInvoiceCount++;
        } else if (vType == "Purchase") {
            db.executeNonQuery(
                "INSERT INTO purchase_invoices ("
                "fy_id, financial_year, invoice_no, invoice_date, supplier_id, supplier_name, "
                "item_id, item_name, hsn_code, "
                "bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, "
                "cgst_amount, sgst_amount, igst_amount, "
                "total_amount, payment_mode, vehicle_no, eway_bill_no, narration, gr_no, driver, broker_name) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'Credit', ?, ?, ?, ?, ?, ?);",
                {
                    fyId,
                    fyVal,
                    QString::fromStdString(invNo),
                    vDate,
                    partyRes.primary_party_id,
                    QString::fromStdString(partyRes.primary_party),
                    itemId,
                    QString::fromStdString(itemName),
                    QString::fromStdString(hsnCode),
                    bagCount,
                    weightQtl,
                    ratePerQtl,
                    taxableAmt,
                    gstPct,
                    cgstAmt,
                    sgstAmt,
                    igstAmt,
                    partyAmount,
                    QString::fromStdString(vehicleNo),
                    QString::fromStdString(ewayBill),
                    QString::fromStdString(narration),
                    QString::fromStdString(grNo),
                    QString::fromStdString(driverName),
                    QString::fromStdString(brokerName)
                }
            );
            purchaseInvoiceCount++;

            // Register in paddy arrivals if purchase item is Paddy
            std::string itemLower = toLowerStr(itemName);
            if (itemLower.find("paddy") != std::string::npos || itemLower.find("dheri") != std::string::npos) {
                db.executeNonQuery(
                    "INSERT INTO paddy_arrivals ("
                    "slip_no, arrival_date, farmer_id, farmer_name, paddy_variety, "
                    "bag_count, gross_weight_qtl, moisture_pct, moisture_deduction_qtl, net_weight_qtl, "
                    "rate_per_qtl, hamali_charges, net_amount, payment_status) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, 17.0, 0.0, ?, ?, 0.0, ?, 'Unpaid');",
                    {
                        QString::fromStdString(invNo),
                        vDate,
                        partyRes.primary_party_id,
                        QString::fromStdString(partyRes.primary_party),
                        QString::fromStdString(itemName),
                        bagCount,
                        weightQtl,
                        weightQtl,
                        ratePerQtl,
                        partyAmount
                    }
                );
            }
        }
    }

    // =========================================================
    // PASS 5.2: MILLING PRODUCTION BATCHES & LINE ITEMS
    // =========================================================
    updateProgress(75, QString("Migrating Milling Production Batches & Line Items..."));
    std::map<std::pair<std::string, std::string>, std::vector<std::map<std::string, std::string>>> millingGroups;
    for (const auto& mr : millingRows) {
        std::string mvNo = cleanText(getField(mr, "VoucherNumber"));
        std::string mvDate = parseDateFormatted(QString::fromStdString(getField(mr, "VoucherDate"))).toStdString();
        millingGroups[{mvNo, mvDate}].push_back(mr);
    }

    db.executeNonQuery("DELETE FROM milling_batches;");
    db.executeNonQuery("DELETE FROM milling_voucher_items;");

    for (const auto& pair : millingGroups) {
        std::string mvNo = pair.first.first;
        std::string mvDate = pair.first.second;
        const auto& mRows = pair.second;

        QString batchNoStr = QString("Mill-%1").arg(QString::fromStdString(mvNo));
        QString fyVal = computeFinancialYear(QString::fromStdString(mvDate));
        int fyId = fyNameToId.count(fyVal) ? fyNameToId[fyVal] : 1;

        double paddyIn = 0.0;
        std::string paddyVariety = "Paddy Basmati";
        double headRice = 0.0;
        double brokenRice = 0.0;
        double bran = 0.0;
        double husk = 0.0;
        std::string batchNarration;

        for (const auto& r : mRows) {
            int iCode = parseIntVal(getField(r, "ItemCode"));
            auto itItem = itemCodeMap.find(iCode);
            std::string iName = (itItem != itemCodeMap.end()) ? itItem->second.name : cleanText(getField(r, "ItemName"));
            if (iName.empty()) iName = "Item #" + std::to_string(iCode);

            std::string iLower = toLowerStr(iName);
            double wt = parseDoubleVal(getField(r, "Weight"));
            std::string drcr = cleanText(getField(r, "DrCr"));
            std::string narr = cleanText(getField(r, "Narrtn"));
            if (!narr.empty() && batchNarration.empty()) batchNarration = narr;

            if (drcr == "Cr") {
                paddyIn += wt;
                paddyVariety = iName;
            } else {
                if (iLower.find("bran") != std::string::npos) {
                    bran += wt;
                } else if (iLower.find("broken") != std::string::npos || iLower.find("nakku") != std::string::npos) {
                    brokenRice += wt;
                } else if (iLower.find("husk") != std::string::npos || iLower.find("phak") != std::string::npos) {
                    husk += wt;
                } else {
                    headRice += wt;
                }
            }
        }

        double totalOut = headRice + brokenRice + bran + husk;
        double wastage = std::max(0.0, std::round((paddyIn - totalOut) * 1000.0) / 1000.0);
        double yieldPct = (paddyIn > 0.0) ? std::round((headRice / paddyIn * 100.0) * 100.0) / 100.0 : 0.0;

        db.executeNonQuery(
            "INSERT INTO milling_batches ("
            "fy_id, financial_year, batch_no, batch_date, paddy_variety, paddy_input_qtl, "
            "head_rice_qtl, broken_rice_qtl, bran_qtl, husk_qtl, "
            "wastage_qtl, yield_pct, narration) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                fyId,
                fyVal,
                batchNoStr,
                QString::fromStdString(mvDate),
                QString::fromStdString(paddyVariety),
                paddyIn,
                headRice,
                brokenRice,
                bran,
                husk,
                wastage,
                yieldPct,
                QString::fromStdString(batchNarration)
            }
        );
        int batchDbId = static_cast<int>(db.lastInsertedId());

        for (const auto& r : mRows) {
            int rowNo = parseIntVal(getField(r, "RowNo", "1"), 1);
            std::string drcr = cleanText(getField(r, "DrCr"));
            int iCode = parseIntVal(getField(r, "ItemCode"));
            auto itItem = itemCodeMap.find(iCode);
            int iId = (itItem != itemCodeMap.end()) ? itItem->second.id : 1;
            std::string iName = (itItem != itemCodeMap.end()) ? itItem->second.name : cleanText(getField(r, "ItemName"));
            if (iName.empty()) iName = "Item #" + std::to_string(iCode);

            double pct = parseDoubleVal(getField(r, "Percentage"));
            double wt = parseDoubleVal(getField(r, "Weight"));
            int bags = parseIntVal(getField(r, "Bags"));
            double amt = parseDoubleVal(getField(r, "Amount"));
            double rate = (wt > 0.0) ? std::round((amt / wt) * 100.0) / 100.0 : 0.0;
            std::string narr = cleanText(getField(r, "Narrtn"));

            db.executeNonQuery(
                "INSERT INTO milling_voucher_items ("
                "batch_id, batch_no, batch_date, row_no, drcr, "
                "item_id, item_code, item_name, percentage, "
                "weight_qtl, bags, rate, amount, narration) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {
                    batchDbId,
                    batchNoStr,
                    QString::fromStdString(mvDate),
                    rowNo,
                    QString::fromStdString(drcr),
                    iId,
                    QString::number(iCode),
                    QString::fromStdString(iName),
                    pct,
                    wt,
                    bags,
                    rate,
                    amt,
                    QString::fromStdString(narr)
                }
            );
        }

        QString vchNarr = QString("Milling Batch %1: Input %2 Qtl %3 -> Produced %4 Qtl (%5% Yield)")
            .arg(batchNoStr)
            .arg(paddyIn, 0, 'f', 2)
            .arg(QString::fromStdString(paddyVariety))
            .arg(totalOut, 0, 'f', 2)
            .arg(yieldPct, 0, 'f', 2);
        if (!batchNarration.empty()) {
            vchNarr += " | " + QString::fromStdString(batchNarration);
        }

        db.executeNonQuery(
            "INSERT INTO vouchers ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, legacy_type, "
            "party_name, account_type, amount, narration) "
            "VALUES (?, ?, ?, ?, 'Milling', 'Mill', 'Milling / Production Account', 'Production Account', 0.0, ?);",
            {fyId, fyVal, batchNoStr, QString::fromStdString(mvDate), vchNarr}
        );
    }

    // =========================================================
    // PASS 5.3: STOCK TRANSACTIONS (INVENTORY LEDGER)
    // =========================================================
    updateProgress(85, QString("Migrating %1 Stock Movements (Inventory Ledger)...").arg(stockTransRows.size()));
    db.executeNonQuery("DELETE FROM stock_transactions;");

    // Key by {VoucherNumber, TransType, VoucherDate} and strictly filter EntryType == "Ledger"
    std::map<std::tuple<std::string, std::string, std::string>, std::tuple<std::string, int, std::string>> txLookup;
    std::map<std::pair<std::string, std::string>, std::tuple<std::string, int, std::string>> txLookupFallback;

    for (const auto& tr : transRows) {
        std::string entryType = cleanText(getField(tr, "EntryType"));
        if (entryType != "Ledger") continue; // Counter-party ledger ONLY (prevents Goods Amount / TDS overwrite)

        std::string vNo = cleanText(getField(tr, "VoucherNumber"));
        std::string tType = cleanText(getField(tr, "TransType"));
        QString vDate = parseDateFormatted(QString::fromStdString(getField(tr, "VoucherDate")));
        int pCode = parseIntVal(getField(tr, "PartyCode"));
        if (pCode == 0) pCode = parseIntVal(getField(tr, "AccountCode"));
        std::string invNo = cleanText(getField(tr, "InvoiceNo"));
        if (invNo.empty()) invNo = cleanText(getField(tr, "TaxInvoiceNo"));
        if (invNo.empty()) invNo = vNo;

        auto itParty = ledgerDetailMap.find(pCode);
        if (itParty != ledgerDetailMap.end()) {
            txLookup[{vNo, tType, vDate.toStdString()}] = {invNo, itParty->second.id, itParty->second.name};
            txLookupFallback[{vNo, tType}] = {invNo, itParty->second.id, itParty->second.name};
        }
    }

    for (const auto& st : stockTransRows) {
        QString stDate = parseDateFormatted(QString::fromStdString(getField(st, "VoucherDate")));
        if (stDate.isEmpty()) continue;

        std::string stVno = cleanText(getField(st, "VoucherNumber"));
        std::string stTt = cleanText(getField(st, "TransType"));
        std::string stVt = cleanText(getField(st, "VoucherType"));
        std::string stIcode = cleanText(getField(st, "ItemCode"));
        int iCode = parseIntVal(stIcode);

        auto itItem = itemCodeMap.find(iCode);
        if (itItem == itemCodeMap.end()) continue;
        int stItemId = itItem->second.id;
        std::string stItemName = itItem->second.name;

        int stBags = parseIntVal(getField(st, "Bags"));
        double stPack = parseDoubleVal(getField(st, "Packing"));
        double stWt = parseDoubleVal(getField(st, "Weight"));
        double stRate = parseDoubleVal(getField(st, "Rate"));
        double stAmt = parseDoubleVal(getField(st, "Amount"));
        double stTaxable = parseDoubleVal(getField(st, "TaxableAmount"));
        double stTax = parseDoubleVal(getField(st, "Tax"));
        std::string stTaxtype = cleanText(getField(st, "TaxType"));
        std::string stNarr = cleanText(getField(st, "Narration"));
        int stRow = parseIntVal(getField(st, "RowNo", "1"), 1);

        std::string stBill = stVno;
        int stPid = 0;
        std::string stPname;

        auto itTx = txLookup.find({stVno, stTt, stDate.toStdString()});
        if (itTx != txLookup.end()) {
            stBill = std::get<0>(itTx->second);
            stPid = std::get<1>(itTx->second);
            stPname = std::get<2>(itTx->second);
        } else {
            auto itFb = txLookupFallback.find({stVno, stTt});
            if (itFb != txLookupFallback.end()) {
                stBill = std::get<0>(itFb->second);
                stPid = std::get<1>(itFb->second);
                stPname = std::get<2>(itFb->second);
            } else {
                int cpCode = parseIntVal(getField(st, "CommissionPartyCode"));
                if (cpCode == 0) cpCode = parseIntVal(getField(st, "DheriPurchaseFrom"));
                auto itCp = ledgerDetailMap.find(cpCode);
                if (itCp != ledgerDetailMap.end()) {
                    stPid = itCp->second.id;
                    stPname = itCp->second.name;
                }
                std::string dheriBill = cleanText(getField(st, "DheriBillNo"));
                if (!dheriBill.empty()) stBill = dheriBill;
            }
        }

        QString stFy = computeFinancialYear(stDate);
        int stFyId = fyNameToId.count(stFy) ? fyNameToId[stFy] : 1;

        db.executeNonQuery(
            "INSERT INTO stock_transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, trans_type, voucher_type, "
            "party_id, party_name, bill_no, item_id, item_code, item_name, "
            "bags, packing, weight_qtl, rate, amount, taxable_amount, tax, "
            "tax_type, narration, row_no) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                stFyId,
                stFy,
                QString::fromStdString(stVno),
                stDate,
                QString::fromStdString(stTt),
                QString::fromStdString(stVt),
                stPid,
                QString::fromStdString(stPname),
                QString::fromStdString(stBill),
                stItemId,
                QString::fromStdString(stIcode),
                QString::fromStdString(stItemName),
                stBags,
                stPack,
                stWt,
                stRate,
                stAmt,
                stTaxable,
                stTax,
                QString::fromStdString(stTaxtype),
                QString::fromStdString(stNarr),
                stRow
            }
        );
    }

    // =========================================================
    // PASS 5.5: CUSTOM CLOSING STOCKS
    // =========================================================
    updateProgress(95, QString("Migrating %1 Audited Custom Closing Stock Records...").arg(customRows.size()));
    db.executeNonQuery("DELETE FROM custom_closing_stocks;");

    for (const auto& cr : customRows) {
        QString cDate = parseDateFormatted(QString::fromStdString(getField(cr, "ClosingStockDate")));
        if (cDate.isEmpty()) continue;

        QString cFy = computeFinancialYear(cDate);
        int cFyId = fyNameToId.count(cFy) ? fyNameToId[cFy] : 1;

        std::string cItemCode = cleanText(getField(cr, "ItemCode"));
        int iCode = parseIntVal(cItemCode);
        auto itItem = itemCodeMap.find(iCode);
        int cItemId = (itItem != itemCodeMap.end()) ? itItem->second.id : 1;
        std::string cItemName = (itItem != itemCodeMap.end()) ? itItem->second.name : ("Item #" + cItemCode);

        int cBags = parseIntVal(getField(cr, "Bags"));
        double cWt = parseDoubleVal(getField(cr, "Weight"));
        double cAmt = parseDoubleVal(getField(cr, "Amount"));
        double cRate = (cWt > 0.0) ? std::round((cAmt / cWt) * 100.0) / 100.0 : 0.0;

        db.executeNonQuery(
            "INSERT INTO custom_closing_stocks ("
            "fy_id, financial_year, closing_date, item_id, item_code, item_name, bags, weight_qtl, rate, amount) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                cFyId,
                cFy,
                cDate,
                cItemId,
                QString::fromStdString(cItemCode),
                QString::fromStdString(cItemName),
                cBags,
                cWt,
                cRate,
                cAmt
            }
        );
    }

    db.commit();
    db.executeNonQuery("PRAGMA foreign_keys = ON;");
    mdb_close(mdb);

    updateProgress(100, "Migration Completed Successfully!");
    m_isMigrating = false;
    emit migratingChanged();

    QString summary = QString("Successfully imported %1 Financial Years, %2 Groups, %3 Parties, %4 Items, %5 Vouchers (%6 Sales, %7 Purchases), %8 Milling Batches, and %9 Audited Closing Stocks from %10")
        .arg(seenFys.size())
        .arg(groupRows.size())
        .arg(ledgerRows.size())
        .arg(itemRows.size())
        .arg(voucherGroups.size())
        .arg(salesInvoiceCount)
        .arg(purchaseInvoiceCount)
        .arg(millingGroups.size())
        .arg(customRows.size())
        .arg(fi.fileName());

    emit migrationFinished(true, summary);
    return true;
#else
    emit migrationFinished(false, "libmdb not available in this build.");
    return false;
#endif
}
