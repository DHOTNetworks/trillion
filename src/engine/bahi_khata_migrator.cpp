#include "bahi_khata_migrator.h"
#include "../database_manager.h"
#include "../config/feature_flags.h"
#include "migration/migration_utils.h"
#include "migration/migration_types.h"
#include "migration/group_migrator.h"
#include "migration/ledger_migrator.h"
#include "migration/stock_migrator.h"
#include "migration/voucher_migrator.h"
#include "migration/mandi_migrator.h"
#include "data_preprocessor.h"
#include "../models/account_classifier.h"
#include "fiscal_year_helper.h"
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
        else if (y >= 1900 && y <= 1970) y += 100;

        // In Jet DB libmdb date strings, format is MM/DD/YYYY
        // If m > 12 and d <= 12, then it was DD/MM/YYYY
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

static QString computeFinancialYear(const QString& isoDate) {
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
    // Jackcess-faithful JET4 scan (jet4_reader): brute-force owned data pages
    // with explicit DELETED (0x8000) skip + OVERFLOW (0x4000) follow.
    // Replaces the fetch-cursor path whose cur_row-1 flag peek misclassified
    // rows once a deleted entry existed in the table.
    return Jet4Reader::readTable(mdb, tableName);
}

#endif

BahiKhataMigrator::BahiKhataMigrator(QObject* parent) : QObject(parent) {}
BahiKhataMigrator::~BahiKhataMigrator() = default;

QString BahiKhataMigrator::choose_mdb_file(const QString& startDir) {
    QString filter = "Bahi Khata Databases (Data.* *.0* *.mdb *.accdb);;All Files (*.*)";
    return QFileDialog::getOpenFileName(nullptr, "Select Bahi Khata Database File (Data.* or *.mdb)", startDir, filter);
}

void BahiKhataMigrator::updateProgress(int percent, const QString& status) {
    m_progressPercent = percent;
    m_statusText = status;
    emit progressChanged(percent);
    emit statusChanged(status);
    emit migrationProgress(percent, status);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

QString BahiKhataMigrator::parseMdbDate(const QString& rawDate) {
    return parseDateFormatted(rawDate);
}

QString BahiKhataMigrator::determineFinancialYear(const QString& isoDate) {
    return computeFinancialYear(isoDate);
}

QString BahiKhataMigrator::resolveFirmTypeFromPan(const QString& rawType, const QString& pan, const QString& companyName) {
    QString cleanPan = pan.trimmed().toUpper();
    if (cleanPan.length() == 15) {
        cleanPan = cleanPan.mid(2, 10);
    }
    if (cleanPan.length() >= 4) {
        QChar c = cleanPan.at(3);
        if (c == 'P') return "Proprietorship Firm";
        if (c == 'F') return "Partnership Firm";
        if (c == 'C') return "Private Limited Company";
        if (c == 'H') return "Hindu Undivided Family (HUF)";
        if (c == 'T') return "Trust";
        if (c == 'A' || c == 'B') return "Association of Persons (AOP/BOI)";
        if (c == 'G') return "Government Agency";
        if (c == 'J') return "Artificial Juridical Person";
        if (c == 'L') return "Local Authority";
    }

    QString lowerName = companyName.trimmed().toLower();
    if (lowerName.contains("pvt ltd") || lowerName.contains("private limited")) return "Private Limited Company";
    if (lowerName.contains("llp")) return "Limited Liability Partnership (LLP)";
    if (lowerName.contains("ltd") || lowerName.contains("limited")) return "Public Limited Company";

    QString lowerType = rawType.trimmed().toLower();
    if (lowerType.contains("prop") || lowerType.contains("proprietor")) return "Proprietorship Firm";
    if (lowerType.contains("partner")) return "Partnership Firm";
    if (lowerType.contains("pvt") || lowerType.contains("private") || lowerType.contains("ltd") || lowerType.contains("limited")) return "Private Limited Company";
    if (lowerType.contains("huf")) return "Hindu Undivided Family (HUF)";
    if (lowerType.contains("trust") || lowerType.contains("society")) return "Trust";
    if (lowerType.contains("individual")) return "Individual";

    if (!rawType.trimmed().isEmpty() && rawType.trimmed() != "N/A") return rawType.trimmed();
    return "Proprietorship Firm";
}

void BahiKhataMigrator::cleanBankingDetails(const QString& m2, const QString& b2, const QString& b3,
                                            QString& outBankName, QString& outAccountNo, QString& outIfsc) {
    QStringList candidates = { m2.trimmed(), b2.trimmed(), b3.trimmed() };
    outBankName.clear();
    outAccountNo.clear();
    outIfsc.clear();

    QRegularExpression ifscRegex(R"(\b[A-Z]{4}0[A-Z0-9]{6}\b)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression ifscPrefixRegex(R"(^IFS[CE][\.\:\s\-]*)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression accPrefixRegex(R"(^(A\/C|A\/c|ACC|ACCOUNT)[\s\.\:\#\-NnoO]*)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression bankPrefixRegex(R"(^(BANK\s*(&|AND)?\s*BRANCH|BANK\s*NAME|BRANCH)[\s\.\:\-]*)", QRegularExpression::CaseInsensitiveOption);

    // 1. Identify IFSC
    for (int i = 0; i < candidates.size(); ++i) {
        QString s = candidates[i];
        if (s.isEmpty()) continue;
        auto match = ifscRegex.match(s);
        if (match.hasMatch()) {
            outIfsc = match.captured(0).toUpper();
            candidates[i].clear();
            break;
        } else if (s.contains("IFSC", Qt::CaseInsensitive) || s.contains("IFSE", Qt::CaseInsensitive)) {
            QString clean = s;
            clean.remove(ifscPrefixRegex);
            outIfsc = clean.trimmed().toUpper();
            candidates[i].clear();
            break;
        }
    }

    // 2. Identify Account Number vs Bank Name
    for (int i = 0; i < candidates.size(); ++i) {
        QString s = candidates[i];
        if (s.isEmpty()) continue;
        if (s.contains("A/C", Qt::CaseInsensitive) || s.contains("A/c", Qt::CaseInsensitive) || s.contains("Acc", Qt::CaseInsensitive) || s.contains("Account", Qt::CaseInsensitive)) {
            QString clean = s;
            clean.remove(accPrefixRegex);
            outAccountNo = clean.trimmed();
            candidates[i].clear();
            break;
        }
    }

    // 3. Identify Bank Name
    for (int i = 0; i < candidates.size(); ++i) {
        QString s = candidates[i];
        if (s.isEmpty()) continue;
        if (s.contains("BANK", Qt::CaseInsensitive) || s.contains("BRANCH", Qt::CaseInsensitive) ||
            s.contains("AXIS", Qt::CaseInsensitive) || s.contains("CANARA", Qt::CaseInsensitive) ||
            s.contains("SBI", Qt::CaseInsensitive) || s.contains("HDFC", Qt::CaseInsensitive) ||
            s.contains("ICICI", Qt::CaseInsensitive) || s.contains("PUNJAB", Qt::CaseInsensitive) ||
            s.contains("PNB", Qt::CaseInsensitive) || s.contains("YES", Qt::CaseInsensitive) ||
            s.contains("KOTAK", Qt::CaseInsensitive) || s.contains("UNION", Qt::CaseInsensitive) ||
            s.contains("BARODA", Qt::CaseInsensitive) || s.contains("OBC", Qt::CaseInsensitive)) {
            QString clean = s;
            clean.remove(bankPrefixRegex);
            outBankName = clean.trimmed();
            candidates[i].clear();
            break;
        }
    }

    // 4. Assign remaining candidates to account or bank name
    for (const QString &s : candidates) {
        if (s.isEmpty()) continue;
        int digitCount = 0;
        for (QChar ch : s) {
            if (ch.isDigit()) digitCount++;
        }
        if (digitCount >= 6 && outAccountNo.isEmpty()) {
            QString clean = s;
            clean.remove(accPrefixRegex);
            outAccountNo = clean.trimmed();
        } else if (outBankName.isEmpty()) {
            QString clean = s;
            clean.remove(bankPrefixRegex);
            outBankName = clean.trimmed();
        } else if (outAccountNo.isEmpty()) {
            outAccountNo = s.trimmed();
        }
    }
}

QString BahiKhataMigrator::extractFssai(const QString& business, const QString& address) {
    QRegularExpression fssaiRegex(R"(\b(1\d{13})\b)");
    auto match = fssaiRegex.match(business);
    if (match.hasMatch()) {
        return match.captured(1);
    }
    match = fssaiRegex.match(address);
    if (match.hasMatch()) {
        return match.captured(1);
    }
    return "10822019000152";
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
    MdbHandle* mdb = mdb_open(QFile::encodeName(cleanPath).constData(), MDB_NOFLAGS);
    if (!mdb) {
        // Fallback with UTF-8 data
        mdb = mdb_open(cleanPath.toUtf8().constData(), MDB_NOFLAGS);
    }
    if (!mdb) {
        result["error"] = "Unable to open Jet database file. Ensure it is a valid .mdb / Data.*** file.";
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
    auto manufacturing = readTableRows(mdb, "MillingVouchers");
    auto transRows = readTableRows(mdb, "Transactions");

    result["stockItemsCount"] = static_cast<int>(stockItems.size());
    result["ledgersCount"] = static_cast<int>(ledgers.size());
    result["stockTxCount"] = static_cast<int>(stockTx.size());
    result["manufacturingCount"] = static_cast<int>(manufacturing.size());
    result["transactionsCount"] = static_cast<int>(transRows.size());

    auto compRows = readTableRows(mdb, "CompanyInfo");
    if (!compRows.empty()) {
        const auto& c = compRows.back();
        QString compName = QString::fromStdString(getField(c, "CompanyName"));
        QString gstin = QString::fromStdString(getField(c, "GSTIN"));
        QString pan = QString::fromStdString(getField(c, "PAN_No"));
        if (pan.isEmpty() && gstin.length() == 15) {
            pan = gstin.mid(2, 10);
        }
        QString rawFirmType = QString::fromStdString(getField(c, "FirmType"));
        QString firmType = resolveFirmTypeFromPan(rawFirmType, pan, compName);
        QString address = QString::fromStdString(getField(c, "Address"));
        QString business = QString::fromStdString(getField(c, "Business"));
        QString station = QString::fromStdString(getField(c, "MyStation"));
        QString state = QString::fromStdString(getField(c, "MySTATE"));
        QString phone = QString::fromStdString(getField(c, "Phone_O"));
        QString mobile = QString::fromStdString(getField(c, "Phone_F", getField(c, "Mobile1")));

        QString bankName, bankAcc, ifsc;
        cleanBankingDetails(
            QString::fromStdString(getField(c, "Mobile2")),
            QString::fromStdString(getField(c, "Bank2")),
            QString::fromStdString(getField(c, "Bank3")),
            bankName, bankAcc, ifsc
        );

        result["companyName"] = compName;
        result["firmType"] = firmType;
        result["gstin"] = gstin;
        result["pan"] = pan;
        result["station"] = station;
        result["state"] = state;
        result["stateCode"] = DataPreprocessor::extractStateCode(gstin, state);
        result["pincode"] = DataPreprocessor::extractPincode(address);
        result["address"] = address;
        result["phone"] = phone;
        result["mobile"] = mobile;
        result["business"] = business;
        result["fssaiNo"] = extractFssai(business, address);
        result["mlNo"] = QString::fromStdString(getField(c, "ML_No"));
        result["bankName"] = bankName;
        result["bankAccount"] = bankAcc;
        result["ifscCode"] = ifsc;
        result["fyFrom"] = QString::fromStdString(getField(c, "AccYearFrom"));
        result["fyTo"] = QString::fromStdString(getField(c, "AccYearTo"));
        result["booksFrom"] = QString::fromStdString(getField(c, "BooksBeginingFrom"));
    }

    mdb_close(mdb);
#else
    result["error"] = "libmdb is not compiled into this build.";
#endif
    return result;
}

using MahadevERP::PartyDetail;
using MahadevERP::ItemDetail;

struct StockMovementEntry {
    int item_id = 0;
    std::string item_name;
    std::string grade;
    std::string hsn_code;
    int bags = 0;
    double packing = 0.500;
    double weight_qtl = 0.0;
    double rate = 0.0;
    double amount = 0.0;
    double taxable_amount = 0.0;
    double gst_pct = 5.0;
    double cgst = 0.0;
    double sgst = 0.0;
    double igst = 0.0;
    double cess = 0.0;
    std::string dheri_bill_no;
    std::string voucher_type;
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
    std::string grade;
    std::string challan_no;
    std::string kanda_weight;
    std::string transport;
    std::string broker_name;
};

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
    MdbHandle* mdb = mdb_open(QFile::encodeName(cleanPath).constData(), MDB_NOFLAGS);
    if (!mdb) {
        // Fallback with UTF-8 data
        mdb = mdb_open(cleanPath.toUtf8().constData(), MDB_NOFLAGS);
    }
    if (!mdb) {
        m_isMigrating = false;
        emit migratingChanged();
        emit migrationFinished(false, "Failed to open MDB database.");
        return false;
    }

    updateProgress(10, "Extracting Raw JetDB Tables...");
    auto compRows = readTableRows(mdb, "CompanyInfo");
    auto groupRows = readTableRows(mdb, "Groups");
    auto stockGroupRows = readTableRows(mdb, "StockGroups");
    auto stockUnitRows = readTableRows(mdb, "StockUnits");
    auto ledgerRows = readTableRows(mdb, "Ledgers");
    auto itemRows = readTableRows(mdb, "StockItems");
    auto stockTransRows = readTableRows(mdb, "StockTransactions");
    auto transportRows = readTableRows(mdb, "SaleTransportationDetail");
    auto transRows = readTableRows(mdb, "Transactions");
    auto manufacturingRows = readTableRows(mdb, "MillingVouchers");
    auto customRows = readTableRows(mdb, "CustomClosingStocks");
    auto tdsRows = readTableRows(mdb, "TDSDeductions");
    auto bardanaRows = readTableRows(mdb, "BardanaTransactions");
    auto gatePassRows = readTableRows(mdb, "GatePassVouchers");
    auto gateRegRows = readTableRows(mdb, "GateRegister");
    auto saudaVchRows = readTableRows(mdb, "SaudaVouchers");
    auto saudaTxRows = readTableRows(mdb, "SaudaTransactions");
    auto brokerageRows = readTableRows(mdb, "BrokerageVouchers");

    // Flag-primed: prime domain flags from SOURCE data before
    // ensureTablesExist creates the gated domain tables (same rules as the
    // new pipeline: JFrm/IFrm TransTypes, sauda/brokerage rows, or
    // paddy/dheri items evidence mandi business; ON-only, package-aligned).
    {
        auto hasTransType = [&](const char* tok) {
            for (const auto& r : transRows) {
                auto it = r.find("TransType");
                if (it != r.end() && QString::fromStdString(it->second).trimmed().compare(tok, Qt::CaseInsensitive) == 0) return true;
            }
            return false;
        };
        auto hasItemNamed = [&](const char* needle) {
            const QString n = QString::fromUtf8(needle);
            for (const auto& r : itemRows) {
                for (const auto& kv : r) {
                    if (QString::fromStdString(kv.second).contains(n, Qt::CaseInsensitive)) return true;
                }
            }
            for (const auto& r : ledgerRows) {
                for (const auto& kv : r) {
                    if (QString::fromStdString(kv.second).contains(n, Qt::CaseInsensitive)) return true;
                }
            }
            return false;
        };
        const bool mandi = hasTransType("JFrm") || hasTransType("IFrm")
            || !saudaVchRows.empty() || !saudaTxRows.empty() || !brokerageRows.empty()
            || hasItemNamed("paddy") || hasItemNamed("dheri");
        const bool manufacturer = !manufacturingRows.empty();
        FeatureFlags::primeFromSource(mandi, manufacturer,
            mandi || manufacturer || !bardanaRows.empty(),
            mandi || !transportRows.empty() || !gatePassRows.empty() || !gateRegRows.empty());
    }

    auto& db = DatabaseManager::instance();
    db.ensureTablesExist();
    db.executeNonQuery("PRAGMA foreign_keys = OFF;");
    db.beginTransaction();

    db.executeNonQuery("DELETE FROM bardana_transactions;");
    db.executeNonQuery("DELETE FROM gate_register;");
    db.executeNonQuery("DELETE FROM sauda_contracts;");
    db.executeNonQuery("DELETE FROM dalali_settlements;");
    db.executeNonQuery("DELETE FROM transport_dispatches;");
    db.executeNonQuery("DELETE FROM debit_credit_notes;");
    db.executeNonQuery("DELETE FROM transactions;");
    db.executeNonQuery("DELETE FROM sales_invoices;");
    db.executeNonQuery("DELETE FROM purchase_invoices;");
    db.executeNonQuery("DELETE FROM paddy_slips;");
    db.executeNonQuery("DELETE FROM manufacturing_voucher_items;");
    db.executeNonQuery("DELETE FROM manufacturing_batches;");
    db.executeNonQuery("DELETE FROM stock_transactions;");
    db.executeNonQuery("DELETE FROM jform_vouchers;");
    db.executeNonQuery("DELETE FROM jform_voucher_items;");
    db.executeNonQuery("DELETE FROM tds_vouchers;");
    db.executeNonQuery("DELETE FROM sales_invoice_items;");
    db.executeNonQuery("DELETE FROM purchase_invoice_items;");
    db.executeNonQuery("DELETE FROM custom_closing_stocks;");
    db.executeNonQuery("DELETE FROM vouchers;");
    db.executeNonQuery("DELETE FROM inventory;");
    db.executeNonQuery("DELETE FROM stock_items;");
    db.executeNonQuery("DELETE FROM stock_groups;");
    db.executeNonQuery("DELETE FROM stock_units;");
    db.executeNonQuery("DELETE FROM parties;");
    db.executeNonQuery("DELETE FROM account_groups;");
    db.executeNonQuery("DELETE FROM financial_years;");
    db.executeNonQuery("DELETE FROM company_info;");

    // =========================================================
    // PASS 0.1: COMPANY INFO (CompanyInfo)
    // =========================================================
    updateProgress(12, "Migrating Company Info...");
    int booksStartYear = 0;
    QString globalBooksFrom = "";
    if (!compRows.empty()) {
        const auto& c = compRows.back();
        QString compName = QString::fromStdString(getField(c, "CompanyName"));
        QString gstin = QString::fromStdString(getField(c, "GSTIN"));
        QString pan = QString::fromStdString(getField(c, "PAN_No"));
        if (pan.isEmpty() && gstin.length() == 15) {
            pan = gstin.mid(2, 10);
        }
        QString rawFirmType = QString::fromStdString(getField(c, "FirmType"));
        QString firmType = resolveFirmTypeFromPan(rawFirmType, pan, compName);
        QString address = QString::fromStdString(getField(c, "Address"));
        QString business = QString::fromStdString(getField(c, "Business"));
        QString station = QString::fromStdString(getField(c, "MyStation"));
        QString state = QString::fromStdString(getField(c, "MySTATE"));
        QString stateCode = DataPreprocessor::extractStateCode(gstin, state);
        QString pincode = DataPreprocessor::extractPincode(address);
        QString phone = QString::fromStdString(getField(c, "Phone_O"));
        QString mobile = QString::fromStdString(getField(c, "Phone_F", getField(c, "Mobile1")));
        QString mlNo = QString::fromStdString(getField(c, "ML_No"));
        QString fssaiNo = extractFssai(business, address);
        QString booksFrom = parseDateFormatted(QString::fromStdString(getField(c, "BooksBeginingFrom")));
        if (booksFrom.isEmpty()) {
            booksFrom = parseDateFormatted(QString::fromStdString(getField(c, "AccYearFrom")));
        }
        if (!booksFrom.isEmpty()) {
            globalBooksFrom = booksFrom;
        }
        if (booksFrom.length() >= 4) {
            booksStartYear = booksFrom.left(4).toInt();
        }
        QString accFrom = parseDateFormatted(QString::fromStdString(getField(c, "AccYearFrom")));
        QString accTo = parseDateFormatted(QString::fromStdString(getField(c, "AccYearTo")));

        QString bankName, bankAcc, ifsc;
        cleanBankingDetails(
            QString::fromStdString(getField(c, "Mobile2")),
            QString::fromStdString(getField(c, "Bank2")),
            QString::fromStdString(getField(c, "Bank3")),
            bankName, bankAcc, ifsc
        );

        db.executeNonQuery(
            "INSERT INTO company_info ("
            "company_name, firm_type, business_type, address, city, state, state_code, pincode, "
            "phone, mobile, email, gstin, pan_no, fssai_no, ml_no, "
            "bank_name, bank_account, ifsc_code, books_from, acc_year_from, acc_year_to, data_file_source"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, '', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                compName,
                firmType,
                business,
                address,
                station,
                state,
                stateCode,
                pincode,
                phone,
                mobile,
                gstin,
                pan,
                fssaiNo,
                mlNo,
                bankName,
                bankAcc,
                ifsc,
                booksFrom,
                accFrom,
                accTo,
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
    int earliestYear = 9999;
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
            if (startYear < earliestYear) earliestYear = startYear;
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
                if (sYear < earliestYear) earliestYear = sYear;
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

    // Ensure all financial years between startY and latestYear exist (never invent future years)
    int startY = (earliestYear != 9999 && earliestYear > 1990) ? earliestYear : (booksStartYear > 1990 ? booksStartYear : 0);
    int targetMaxYear = (latestYear > 1990) ? latestYear : 0;
    if (startY == 0 || targetMaxYear == 0) {
        QDate cur = QDate::currentDate();
        int currentFyStartYear = (cur.month() >= 4) ? cur.year() : (cur.year() - 1);
        startY = currentFyStartYear;
        targetMaxYear = currentFyStartYear;
    }
    if (booksStartYear > 1990 && booksStartYear < startY) {
        startY = booksStartYear;
    }

    for (int y = startY; y <= targetMaxYear; ++y) {
        QString fyName = QString("FY %1-%2").arg(y).arg(QString::number((y + 1) % 100).rightJustified(2, '0'));
        if (seenFys.find(fyName) == seenFys.end()) {
            seenFys.insert(fyName);
            QString fromD = QString("%1-04-01").arg(y);
            QString toD = QString("%1-03-31").arg(y + 1);
            db.executeNonQuery(
                "INSERT INTO financial_years (year_name, start_date, end_date, is_active, is_locked) "
                "VALUES (?, ?, ?, 0, 0);",
                {fyName, fromD, toD}
            );
            fyNameToId[fyName] = static_cast<int>(db.lastInsertedId());
            if (y >= latestYear) {
                latestYear = y;
                latestFyName = fyName;
            }
        }
    }

    // Activate the latest operational FY from the imported database
    QString activeFy = latestFyName;
    if (activeFy.isEmpty()) {
        activeFy = QString("FY %1-%2").arg(targetMaxYear).arg(QString::number((targetMaxYear + 1) % 100).rightJustified(2, '0'));
    }
    if (!activeFy.isEmpty()) {
        db.executeNonQuery("UPDATE financial_years SET is_active = 0;");
        db.executeNonQuery("UPDATE financial_years SET is_active = 1 WHERE year_name = ?;", {activeFy});
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
    std::map<int, qint64> groupCodeToIdMap;
    std::map<int, std::vector<int>> groupLineageMap;
    // C3 strangler slice G: PASS 1 lives in GroupMigrator now (JSON-driven
    // field keys via MigrationColumns). This stays a thin wrapper — no new
    // logic here (file frozen read-only for new logic from this slice on).
    {
        MahadevERP::MigrationContext gctx;
        gctx.provider = MahadevERP::MigrationProvider::BahiKhata;
        gctx.sourcePath = mdbFilePath;
        gctx.progressCallback = [this](int p, const QString& s) { updateProgress(p, s); };
        if (!MahadevERP::GroupMigrator::migrate_groups_bahikhata(
                gctx, groupRows, groupCodeMap, groupCodeToIdMap, &groupLineageMap)) {
            qWarning() << "[Migration] group pass failed for" << mdbFilePath;
            return false;
        }
    }

    // =========================================================
    // PASS 1.5: STOCK GROUPS & STOCK UNITS (C3 strangler slice S —
    // lives in StockMigrator now, JSON-driven keys via MigrationColumns).
    // Thin wrapper — no new logic here.
    std::map<int, std::string> stockGroupMap;
    std::map<int, std::string> stockUnitMap;
    {
        MahadevERP::MigrationContext sctx;
        sctx.provider = MahadevERP::MigrationProvider::BahiKhata;
        sctx.sourcePath = mdbFilePath;
        sctx.progressCallback = [this](int p, const QString& s) { updateProgress(p, s); };
        if (!MahadevERP::StockMigrator::migrate_stock_masters_bahikhata(
                sctx, stockGroupRows, stockUnitRows,
                stockGroupMap, stockUnitMap)) {
            qWarning() << "[Migration] stock masters pass failed for" << mdbFilePath;
            return false;
        }
    }

    // =========================================================
    // PASS 2: LEDGERS (PARTIES & ACCOUNTS)
    // =========================================================
    updateProgress(30, QString("Migrating %1 Ledgers & Parties...").arg(ledgerRows.size()));
    // C3 strangler slice L: PASS 2 lives in LedgerMigrator now (JSON-driven
    // field keys via MigrationColumns). This stays a thin wrapper — no new
    // logic here (file frozen read-only for new logic from slice G on).
    std::map<int, std::string> ledgerCodeMap;
    std::map<int, MahadevERP::PartyDetail> ledgerDetailMap;
    std::map<int, int> legacyIdToPartyId;
    std::map<int, std::string> ledgerDeletedNameMap;
    {
        MahadevERP::MigrationContext lctx;
        lctx.provider = MahadevERP::MigrationProvider::BahiKhata;
        lctx.sourcePath = mdbFilePath;
        lctx.progressCallback = [this](int p, const QString& s) { updateProgress(p, s); };
        if (!MahadevERP::LedgerMigrator::migrate_ledgers_bahikhata(
                lctx, ledgerRows, globalBooksFrom, groupCodeMap, groupLineageMap,
                groupCodeToIdMap, mdb, ledgerCodeMap, ledgerDetailMap,
                legacyIdToPartyId, ledgerDeletedNameMap)) {
            qWarning() << "[Migration] ledger pass failed for" << mdbFilePath;
            return false;
        }
    }

    // =========================================================
    // PASS 3: STOCK ITEMS & INVENTORY (C3 strangler slice S — lives in
    // StockMigrator now, JSON-driven keys via MigrationColumns).
    // Thin wrapper — no new logic here.
    updateProgress(40, QString("Migrating %1 Stock Items & Inventory...").arg(itemRows.size()));
    std::map<int, MahadevERP::ItemDetail> itemCodeMap;
    {
        MahadevERP::MigrationContext sctx;
        sctx.provider = MahadevERP::MigrationProvider::BahiKhata;
        sctx.sourcePath = mdbFilePath;
        sctx.progressCallback = [this](int p, const QString& s) { updateProgress(p, s); };
        std::map<int, std::string> itemCodeToName;
        std::map<int, qint64> itemCodeToId;
        if (!MahadevERP::StockMigrator::migrate_stock_items_bahikhata(
                sctx, itemRows, ledgerCodeMap, legacyIdToPartyId,
                stockGroupMap, stockUnitMap,
                itemCodeMap, itemCodeToName, itemCodeToId)) {
            qWarning() << "[Migration] stock items pass failed for" << mdbFilePath;
            return false;
        }
    }

    // =========================================================
    // PASS 4: STOCK MOVEMENTS & LOGISTICS CACHING
    // =========================================================
    // C3 strangler slice V: PASS 4 lives in VoucherMigrator now
    // (JSON-driven field keys via MigrationColumns). Thin wrapper — no
    // new logic here. MahadevERP::-qualified maps: later passes only
    // read through identical field names.
    std::map<std::string, std::vector<MahadevERP::StockMovementEntry>> stockTransMap;
    std::map<std::string, MahadevERP::LogisticsEntry> transportMap;
    {
        MahadevERP::MigrationContext vctx;
        vctx.provider = MahadevERP::MigrationProvider::BahiKhata;
        vctx.sourcePath = mdbFilePath;
        vctx.progressCallback = [this](int p, const QString& s) { updateProgress(p, s); };
        if (!MahadevERP::VoucherMigrator::migrate_movements_bahikhata(
                vctx, stockTransRows, transportRows, itemCodeMap,
                stockTransMap, transportMap)) {
            qWarning() << "[Migration] movements pass failed for" << mdbFilePath;
            return false;
        }
    }

    // =========================================================
    // PASS 5: TRANSACTIONS & VOUCHERS (100% DOUBLE-ENTRY PARITY)
    // =========================================================
    // C3 strangler slice V: PASS 5 sections 1-2 live in VoucherMigrator
    // now (JSON-driven field keys via MigrationColumns). Thin wrapper —
    // no new logic here. voucherGroups + invoice counts flow back out
    // for the summary + later mandi passes.
    std::map<std::string, std::vector<std::map<std::string, std::string>>> voucherGroups;
    int salesInvoiceCount = 0;
    int purchaseInvoiceCount = 0;
    {
        MahadevERP::MigrationContext vctx;
        vctx.provider = MahadevERP::MigrationProvider::BahiKhata;
        vctx.sourcePath = mdbFilePath;
        vctx.progressCallback = [this](int p, const QString& s) { updateProgress(p, s); };
        if (!MahadevERP::VoucherMigrator::migrate_transactions_bahikhata(
                vctx, transRows, stockTransMap, transportMap,
                ledgerCodeMap, ledgerDetailMap, ledgerDeletedNameMap, fyNameToId,
                voucherGroups, salesInvoiceCount, purchaseInvoiceCount)) {
            qWarning() << "[Migration] transactions pass failed for" << mdbFilePath;
            return false;
        }
    }

    // (§2 invoice/voucher reconstruction moved into
    // VoucherMigrator::migrate_transactions_bahikhata — PASS 5 wrapper above.)

    // =========================================================
    // PASS 5.2: manufacturING PRODUCTION BATCHES & LINE ITEMS
    // =========================================================
    // C3 slice M1: PASS 5.2 lives in MandiMigrator now (JSON-driven
    // field keys via MigrationColumns). Thin wrapper — no new logic
    // here. The group count flows back out for the summary
    // (value-identical: skips never incremented either counter).
    size_t manufacturingGroupCount = 0;
    {
        MahadevERP::MigrationContext mctx;
        mctx.provider = MahadevERP::MigrationProvider::BahiKhata;
        mctx.sourcePath = mdbFilePath;
        mctx.progressCallback = [this](int p, const QString& s) { updateProgress(p, s); };
        if (!MahadevERP::MandiMigrator::migrate_manufacturing_bahikhata(
                mctx, manufacturingRows, itemCodeMap, fyNameToId,
                manufacturingGroupCount)) {
            qWarning() << "[Migration] manufacturing pass failed for" << mdbFilePath;
            return false;
        }
    }

    // =========================================================
    // PASS 5.3: STOCK TRANSACTIONS (INVENTORY LEDGER)
    // =========================================================
    // C3 strangler slice V: PASS 5.3 lives in VoucherMigrator now
    // (JSON-driven field keys via MigrationColumns). Thin wrapper — no
    // new logic here.
    {
        MahadevERP::MigrationContext vctx;
        vctx.provider = MahadevERP::MigrationProvider::BahiKhata;
        vctx.sourcePath = mdbFilePath;
        vctx.progressCallback = [this](int p, const QString& s) { updateProgress(p, s); };
        if (!MahadevERP::VoucherMigrator::migrate_stock_ledger_bahikhata(
                vctx, transRows, stockTransRows,
                ledgerDetailMap, itemCodeMap, fyNameToId)) {
            qWarning() << "[Migration] stock-ledger pass failed for" << mdbFilePath;
            return false;
        }
    }

    // =========================================================
    // PASS 3.5: J-FORM PROCUREMENT VOUCHERS (TransType == 'JFrm')
    // =========================================================
    // C3 slice M1: PASS 3.5 lives in MandiMigrator now (JSON-driven
    // field keys via MigrationColumns). Thin wrapper — no new logic here.
    {
        MahadevERP::MigrationContext mctx;
        mctx.provider = MahadevERP::MigrationProvider::BahiKhata;
        mctx.sourcePath = mdbFilePath;
        mctx.progressCallback = [this](int p, const QString& s) { updateProgress(p, s); };
        if (!MahadevERP::MandiMigrator::migrate_jform_bahikhata(
                mctx, transRows, stockTransRows,
                ledgerDetailMap, itemCodeMap, fyNameToId)) {
            qWarning() << "[Migration] J-Form pass failed for" << mdbFilePath;
            return false;
        }
    }


    // =========================================================
    // PASS 3.6: I-FORM MANDI ISSUE VOUCHERS (TransType == 'IFrm')
    // =========================================================
    // C3 slice M1: PASS 3.6 lives in MandiMigrator now (JSON-driven
    // field keys via MigrationColumns). Thin wrapper — no new logic here.
    {
        MahadevERP::MigrationContext mctx;
        mctx.provider = MahadevERP::MigrationProvider::BahiKhata;
        mctx.sourcePath = mdbFilePath;
        mctx.progressCallback = [this](int p, const QString& s) { updateProgress(p, s); };
        if (!MahadevERP::MandiMigrator::migrate_iform_bahikhata(
                mctx, transRows, stockTransRows,
                ledgerDetailMap, itemCodeMap, fyNameToId)) {
            qWarning() << "[Migration] I-Form pass failed for" << mdbFilePath;
            return false;
        }
    }


    // =========================================================
    // PASS 5.4: TDS VOUCHERS (TDSDeductions)
    // =========================================================
    // C3 slice M2: PASS 5.4 lives in MandiMigrator now (JSON-driven
    // field keys via MigrationColumns). Thin wrapper — no new logic here.
    {
        MahadevERP::MigrationContext mctx;
        mctx.provider = MahadevERP::MigrationProvider::BahiKhata;
        mctx.sourcePath = mdbFilePath;
        mctx.progressCallback = [this](int p, const QString& s) { updateProgress(p, s); };
        if (!MahadevERP::MandiMigrator::migrate_tds_bahikhata(
                mctx, transRows, tdsRows, ledgerDetailMap, fyNameToId)) {
            qWarning() << "[Migration] TDS pass failed for" << mdbFilePath;
            return false;
        }
    }


    // =========================================================
    // PASS 5.5: CUSTOM CLOSING STOCKS
    // =========================================================
    // C3 slice M2: PASS 5.5 lives in MandiMigrator now (JSON-driven
    // field keys via MigrationColumns). Thin wrapper — no new logic here.
    {
        MahadevERP::MigrationContext mctx;
        mctx.provider = MahadevERP::MigrationProvider::BahiKhata;
        mctx.sourcePath = mdbFilePath;
        mctx.progressCallback = [this](int p, const QString& s) { updateProgress(p, s); };
        if (!MahadevERP::MandiMigrator::migrate_closing_bahikhata(
                mctx, customRows, itemCodeMap, fyNameToId)) {
            qWarning() << "[Migration] closing-stock pass failed for" << mdbFilePath;
            return false;
        }
    }

    // =========================================================
    // PASS 5.6: BARDANA TRANSACTIONS (BardanaTransactions)
    // =========================================================
    // C3 slice M2: PASS 5.6 lives in MandiMigrator now (JSON-driven
    // field keys via MigrationColumns). Thin wrapper — no new logic here.
    {
        MahadevERP::MigrationContext mctx;
        mctx.provider = MahadevERP::MigrationProvider::BahiKhata;
        mctx.sourcePath = mdbFilePath;
        mctx.progressCallback = [this](int p, const QString& s) { updateProgress(p, s); };
        if (!MahadevERP::MandiMigrator::migrate_bardana_bahikhata(
                mctx, bardanaRows, ledgerCodeMap, ledgerDetailMap,
                ledgerDeletedNameMap, fyNameToId)) {
            qWarning() << "[Migration] bardana pass failed for" << mdbFilePath;
            return false;
        }
    }

    // =========================================================
    // PASS 5.7: GATE PASS & GATE REGISTER (GatePassVouchers & GateRegister)
    // =========================================================
    // C3 slice M2: PASS 5.7 lives in MandiMigrator now (JSON-driven
    // field keys via MigrationColumns). Thin wrapper — no new logic here.
    // GateRegister source rows stay unread (verify-only JSON coverage).
    {
        MahadevERP::MigrationContext mctx;
        mctx.provider = MahadevERP::MigrationProvider::BahiKhata;
        mctx.sourcePath = mdbFilePath;
        mctx.progressCallback = [this](int p, const QString& s) { updateProgress(p, s); };
        if (!MahadevERP::MandiMigrator::migrate_gate_bahikhata(
                mctx, gatePassRows, ledgerDetailMap, itemCodeMap)) {
            qWarning() << "[Migration] gate pass failed for" << mdbFilePath;
            return false;
        }
    }

    // =========================================================
    // PASS 5.8: SAUDA FORWARD CONTRACTS (SaudaVouchers & SaudaTransactions)
    // =========================================================
    // C3 slice M2: PASS 5.8 lives in MandiMigrator now (JSON-driven
    // field keys via MigrationColumns). Thin wrapper — no new logic here.
    // SaudaTransactions rows stay unread (verify-only JSON coverage).
    {
        MahadevERP::MigrationContext mctx;
        mctx.provider = MahadevERP::MigrationProvider::BahiKhata;
        mctx.sourcePath = mdbFilePath;
        mctx.progressCallback = [this](int p, const QString& s) { updateProgress(p, s); };
        if (!MahadevERP::MandiMigrator::migrate_sauda_bahikhata(
                mctx, saudaVchRows, ledgerDetailMap, itemCodeMap, fyNameToId)) {
            qWarning() << "[Migration] sauda pass failed for" << mdbFilePath;
            return false;
        }
    }

    // =========================================================
    // PASS 5.9: BROKERAGE / DALALI SETTLEMENTS (BrokerageVouchers)
    // =========================================================
    // C3 slice M2: PASS 5.9 lives in MandiMigrator now (JSON-driven
    // field keys via MigrationColumns). Thin wrapper — no new logic here.
    {
        MahadevERP::MigrationContext mctx;
        mctx.provider = MahadevERP::MigrationProvider::BahiKhata;
        mctx.sourcePath = mdbFilePath;
        mctx.progressCallback = [this](int p, const QString& s) { updateProgress(p, s); };
        if (!MahadevERP::MandiMigrator::migrate_dalali_bahikhata(
                mctx, brokerageRows, ledgerDetailMap, fyNameToId)) {
            qWarning() << "[Migration] dalali pass failed for" << mdbFilePath;
            return false;
        }
    }

    db.commit();
    db.executeNonQuery("PRAGMA foreign_keys = ON;");
    db.ensureDomainMasters();
    mdb_close(mdb);

    updateProgress(100, "Migration Completed Successfully!");
    m_isMigrating = false;
    emit migratingChanged();

    QString summary = QString("Successfully imported %1 Financial Years, %2 Groups, %3 Parties, %4 Items, %5 Vouchers (%6 Sales, %7 Purchases), %8 Manufacturing Batches, and %9 Audited Closing Stocks from %10")
        .arg(seenFys.size())
        .arg(groupRows.size())
        .arg(ledgerRows.size())
        .arg(itemRows.size())
        .arg(voucherGroups.size())
        .arg(salesInvoiceCount)
        .arg(purchaseInvoiceCount)
        .arg(manufacturingGroupCount)
        .arg(customRows.size())
        .arg(fi.fileName());

    emit migrationFinished(true, summary);
    return true;
#else
    emit migrationFinished(false, "libmdb not available in this build.");
    return false;
#endif
}
