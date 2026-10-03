#include "tally_data_migrator.h"
#include "../database_manager.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QXmlStreamReader>
#include <QFileDialog>
#include <QCoreApplication>
#include <QDebug>
#include <QRegularExpression>
#include <QMap>
#include <QSet>
#include <cmath>

namespace MahadevERP {

static const QMap<QString, QString> s_stateCodes = {
    {"JAMMU AND KASHMIR", "01"}, {"HIMACHAL PRADESH", "02"}, {"PUNJAB", "03"},
    {"CHANDIGARH", "04"}, {"UTTARAKHAND", "05"}, {"HARYANA", "06"},
    {"DELHI", "07"}, {"RAJASTHAN", "08"}, {"UTTAR PRADESH", "09"},
    {"BIHAR", "10"}, {"SIKKIM", "11"}, {"ARUNACHAL PRADESH", "12"},
    {"NAGALAND", "13"}, {"MANIPUR", "14"}, {"MIZORAM", "15"},
    {"TRIPURA", "16"}, {"MEGHALAYA", "17"}, {"ASSAM", "18"},
    {"WEST BENGAL", "19"}, {"JHARKHAND", "20"}, {"ODISHA", "21"},
    {"CHHATTISGARH", "22"}, {"MADHYA PRADESH", "23"}, {"GUJARAT", "24"},
    {"MAHARASHTRA", "27"}, {"ANDHRA PRADESH", "37"}, {"LADAKH", "38"},
    {"KARNATAKA", "29"}, {"GOA", "30"}, {"KERALA", "32"},
    {"TAMIL NADU", "33"}, {"PUDUCHERRY", "34"}, {"TELANGANA", "36"}
};

struct TallyGroupMeta {
    int code1st = 0;
    int code2nd = 0;
    QString nature = "Assets";
    int extractBs = 1;
};

static TallyGroupMeta mapTallyGroup(const QString& name, const QString& parentName, const QMap<QString, QString>& parentMap, int depth = 0) {
    if (depth > 15) {
        return {0, 0, "Assets", 1};
    }
    QString qName = name.trimmed().toLower();

    // Standard Primary & Sub-Groups in Tally
    if (qName == "capital account" || qName == "capital accounts" || qName == "reserves & surplus") {
        return {1, 0, "Liabilities", 1};
    }
    if (qName == "current assets") {
        return {2, 0, "Assets", 1};
    }
    if (qName == "bank accounts" || qName == "bank occ a/c" || qName == "bank od a/c") {
        return {3, 2, "Assets", 1};
    }
    if (qName == "cash-in-hand" || qName == "cash in hand" || qName == "cash") {
        return {4, 2, "Assets", 1};
    }
    if (qName == "deposits (asset)" || qName == "securities & deposits (asset)") {
        return {26, 2, "Assets", 1};
    }
    if (qName == "loans & advances (asset)") {
        return {6, 2, "Assets", 1};
    }
    if (qName == "stock-in-hand" || qName == "stock in hand") {
        return {7, 2, "Assets", 0};
    }
    if (qName == "sundry debtors") {
        return {8, 2, "Assets", 1};
    }
    if (qName == "current liabilities") {
        return {9, 0, "Liabilities", 1};
    }
    if (qName == "duties & taxes" || qName == "duties and taxes") {
        return {10, 9, "Liabilities", 1};
    }
    if (qName == "provisions") {
        return {12, 9, "Liabilities", 1};
    }
    if (qName == "sundry creditors") {
        return {11, 9, "Liabilities", 1};
    }
    if (qName == "fixed assets") {
        return {14, 0, "Assets", 1};
    }
    if (qName == "investments") {
        return {2, 0, "Assets", 1};
    }
    if (qName == "loans (liability)" || qName == "secured loans" || qName == "unsecured loans") {
        return {13, 0, "Liabilities", 1};
    }
    if (qName == "suspense a/c" || qName == "suspense account") {
        return {25, 0, "Liabilities", 1};
    }
    if (qName == "misc. expenses (asset)") {
        return {17, 15, "Expense", 0};
    }
    if (qName == "branch / divisions" || qName == "branches / divisions") {
        return {13, 0, "Liabilities", 1};
    }
    if (qName == "sales accounts" || qName == "sales account" || qName == "sale" || qName == "sales") {
        return {20, 18, "Income", 0};
    }
    if (qName == "purchase accounts" || qName == "purchase account" || qName == "purchase" || qName == "purchases") {
        return {19, 18, "Expense", 0};
    }
    if (qName == "direct expenses" || qName == "expenses (direct)" || qName == "manufacturing expenses") {
        return {23, 22, "Expense", 0};
    }
    if (qName == "direct incomes" || qName == "income (direct)") {
        return {16, 15, "Income", 0};
    }
    if (qName == "indirect expenses" || qName == "expenses (indirect)" || qName == "administrative expenses") {
        return {17, 15, "Expense", 0};
    }
    if (qName == "indirect incomes" || qName == "income (indirect)") {
        return {16, 15, "Income", 0};
    }

    // Name-based keywords matching
    if (qName.contains("debtor") || qName.contains("customer")) return {8, 2, "Assets", 1};
    if (qName.contains("creditor") || qName.contains("supplier") || qName.contains("vendor")) return {11, 9, "Liabilities", 1};
    if (qName.contains("sale") || qName.contains("revenue")) return {20, 18, "Income", 0};
    if (qName.contains("purchas") || qName.contains("procurement")) return {19, 18, "Expense", 0};
    if (qName.contains("direct exp") || qName.contains("mfg") || qName.contains("freight inward") || qName.contains("labour")) return {23, 22, "Expense", 0};
    if (qName.contains("indirect exp") || qName.contains("expense") || qName.contains("expenditure")) return {17, 15, "Expense", 0};
    if (qName.contains("indirect inc") || qName.contains("income")) return {16, 15, "Income", 0};
    if (qName.contains("bank")) return {3, 2, "Assets", 1};
    if (qName.contains("cash")) return {4, 2, "Assets", 1};
    if (qName.contains("tax") || qName.contains("duty") || qName.contains("gst") || qName.contains("tds") || qName.contains("tcs")) return {10, 9, "Liabilities", 1};
    if (qName.contains("fixed asset") || qName.contains("machinery") || qName.contains("building") || qName.contains("vehicle")) return {14, 0, "Assets", 1};
    if (qName.contains("capital") || qName.contains("partner") || qName.contains("proprietor")) return {1, 0, "Liabilities", 1};
    if (qName.contains("loan") || qName.contains("borrowing")) return {13, 0, "Liabilities", 1};

    // Recursive inheritance from parent group
    if (!parentName.isEmpty() && parentName != "Primary" && parentMap.contains(parentName)) {
        return mapTallyGroup(parentName, parentMap.value(parentName), parentMap, depth + 1);
    }

    return {0, 0, "Assets", 1};
}

static QString readAndSanitizeXmlFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    QByteArray bytes = file.readAll();
    file.close();

    if (bytes.isEmpty()) return QString();

    QString content;
    if (bytes.size() >= 2 && static_cast<unsigned char>(bytes[0]) == 0xFF && static_cast<unsigned char>(bytes[1]) == 0xFE) {
        // UTF-16LE with BOM
        content = QString::fromUtf16(reinterpret_cast<const char16_t*>(bytes.constData() + 2), (bytes.size() - 2) / 2);
    } else if (bytes.size() >= 2 && static_cast<unsigned char>(bytes[0]) == 0xFE && static_cast<unsigned char>(bytes[1]) == 0xFF) {
        // UTF-16BE with BOM
        QByteArray leBytes(bytes.size() - 2, 0);
        for (int i = 2; i + 1 < bytes.size(); i += 2) {
            leBytes[i - 2] = bytes[i + 1];
            leBytes[i - 1] = bytes[i];
        }
        content = QString::fromUtf16(reinterpret_cast<const char16_t*>(leBytes.constData()), leBytes.size() / 2);
    } else if (bytes.size() >= 4 && bytes[1] == 0 && bytes[3] == 0) {
        // UTF-16LE without BOM
        content = QString::fromUtf16(reinterpret_cast<const char16_t*>(bytes.constData()), bytes.size() / 2);
    } else {
        // UTF-8
        content = QString::fromUtf8(bytes);
    }

    // Sanitize illegal XML 1.0 numeric character references (&#1; - &#31;)
    static const QRegularExpression ctrlRefRx("&#([0-9]+);");
    QString result;
    result.reserve(content.size());
    qsizetype lastPos = 0;
    auto it = ctrlRefRx.globalMatch(content);
    while (it.hasNext()) {
        auto match = it.next();
        result.append(content.sliced(lastPos, match.capturedStart() - lastPos));
        int val = match.captured(1).toInt();
        if (val == 9 || val == 10 || val == 13 || (val >= 32 && val <= 126)) {
            result.append(QChar(val));
        }
        lastPos = match.capturedEnd();
    }
    result.append(content.sliced(lastPos));
    return result;
}

TallyDataMigrator::TallyDataMigrator(QObject* parent)
    : QObject(parent)
{
}

QString TallyDataMigrator::normalizePath(const QString& p) {
    return QDir::cleanPath(p.trimmed());
}

QString TallyDataMigrator::cleanStr(const QString& s) {
    return s.trimmed();
}

QString TallyDataMigrator::parseDateStr(const QString& raw) {
    QString s = raw.trimmed();
    if (s.isEmpty()) return "";

    // 1. Tally compact standard YYYYMMDD (e.g. 20260401)
    if (s.length() == 8 && s.toInt() > 19000000) {
        return QString("%1-%2-%3").arg(s.left(4), s.mid(4, 2), s.right(2));
    }

    // 2. ISO YYYY-MM-DD
    if (s.contains("-") && s.length() >= 10) {
        QDate d = QDate::fromString(s.left(10), "yyyy-MM-dd");
        if (d.isValid()) return d.toString("yyyy-MM-dd");
    }

    // 3. DD-MM-YYYY or DD/MM/YYYY
    if (s.contains("/")) {
        QDate d = QDate::fromString(s, "dd/MM/yyyy");
        if (d.isValid()) return d.toString("yyyy-MM-dd");
    }
    if (s.contains("-")) {
        QDate d = QDate::fromString(s, "dd-MM-yyyy");
        if (d.isValid()) return d.toString("yyyy-MM-dd");
    }

    return s;
}

QString TallyDataMigrator::computeFy(const QString& isoDate) {
    QDate d = QDate::fromString(isoDate, "yyyy-MM-dd");
    if (!d.isValid()) return "FY 2026-27";

    int y = d.year();
    if (d.month() >= 4) {
        return QString("FY %1-%2").arg(y).arg(QString::number((y + 1) % 100).rightJustified(2, '0'));
    } else {
        return QString("FY %1-%2").arg(y - 1).arg(QString::number(y % 100).rightJustified(2, '0'));
    }
}

QString TallyDataMigrator::resolveState(const QString& rawState) {
    QString st = rawState.trimmed().toUpper();
    st.replace("&", "AND");
    for (auto it = s_stateCodes.constBegin(); it != s_stateCodes.constEnd(); ++it) {
        if (st.contains(it.key()) || it.key().contains(st)) {
            return it.key();
        }
    }
    return rawState.trimmed();
}

QString TallyDataMigrator::getStateCode(const QString& stateName) {
    QString st = stateName.trimmed().toUpper();
    st.replace("&", "AND");
    for (auto it = s_stateCodes.constBegin(); it != s_stateCodes.constEnd(); ++it) {
        if (st.contains(it.key()) || it.key().contains(st)) {
            return it.value();
        }
    }
    return "06";
}

void TallyDataMigrator::updateProgress(int percent, const QString& status) {
    m_progressPercent = percent;
    m_statusText = status;
    emit progressChanged(percent);
    emit statusChanged(status);
    emit migrationProgress(percent, status);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

QString TallyDataMigrator::choose_tally_path(const QString& startDir) {
    QString filter = "Tally Data Files (*.xml Master.xml DayBook.xml Transactions.xml);;XML Files (*.xml);;All Files (*.*)";
    QString file = QFileDialog::getOpenFileName(nullptr, "Select Tally XML File", startDir, filter);
    if (file.isEmpty()) {
        return QFileDialog::getExistingDirectory(nullptr, "Select Tally Data Directory", startDir);
    }
    return file;
}

QVariantMap TallyDataMigrator::inspect_tally_data(const QString& tallyPath) {
    QVariantMap report;
    report["success"] = false;
    report["valid"] = false;
    report["sourceType"] = "Tally";

    QString cleanP = normalizePath(tallyPath);
    QFileInfo fi(cleanP);
    if (!fi.exists()) {
        fi.setFile(QDir::current().filePath(cleanP));
        cleanP = fi.absoluteFilePath();
    }

    QStringList xmlFilesToInspect;
    if (fi.isDir()) {
        QDir dir(cleanP);
        QStringList entries = dir.entryList(QStringList() << "*.xml" << "*.XML", QDir::Files, QDir::Name);
        for (const QString& e : entries) {
            xmlFilesToInspect << dir.filePath(e);
        }
    } else if (fi.isFile()) {
        xmlFilesToInspect << cleanP;
    }

    if (xmlFilesToInspect.isEmpty()) {
        report["message"] = "No Tally XML files found in path: " + cleanP;
        report["error"] = report["message"];
        return report;
    }

    int unitsCount = 0;
    int groupsCount = 0;
    int accountsCount = 0;
    int itemsCount = 0;
    int vouchersCount = 0;
    int glTransactionsCount = 0;
    double totalDr = 0.0;
    double totalCr = 0.0;
    QString companyName = "Tally Prime Company";
    QString gstin = "";
    QString financialYear = "FY 2026-27";

    bool foundAnyTallyEnvelope = false;

    for (const QString& xmlFile : xmlFilesToInspect) {
        QString content = readAndSanitizeXmlFile(xmlFile);
        if (content.isEmpty()) continue;

        QXmlStreamReader xml(content);
        while (!xml.atEnd() && !xml.hasError()) {
            QXmlStreamReader::TokenType token = xml.readNext();
            if (token == QXmlStreamReader::StartElement) {
                QString name = xml.name().toString().toUpper();
                if (name == "ENVELOPE" || name == "TALLYMESSAGE") {
                    foundAnyTallyEnvelope = true;
                } else if (name == "SVCURRENTCOMPANY" || name == "CURRENTCOMPANY") {
                    QString comp = xml.readElementText().trimmed();
                    if (!comp.isEmpty()) companyName = comp;
                } else if (name == "UNIT") {
                    unitsCount++;
                } else if (name == "GROUP" || name == "STOCKGROUP") {
                    groupsCount++;
                } else if (name == "LEDGER") {
                    accountsCount++;
                } else if (name == "STOCKITEM") {
                    itemsCount++;
                } else if (name == "VOUCHER") {
                    vouchersCount++;
                    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString().toUpper() == "VOUCHER") && !xml.atEnd()) {
                        xml.readNext();
                        if (xml.isStartElement()) {
                            QString sub = xml.name().toString().toUpper();
                            if (sub == "DATE" && !xml.readElementText().trimmed().isEmpty()) {
                                financialYear = computeFy(parseDateStr(xml.readElementText()));
                            } else if (sub == "PARTYGSTIN" && gstin.isEmpty()) {
                                gstin = xml.readElementText().trimmed().toUpper();
                            } else if (sub == "ALLLEDGERENTRIES.LIST" || sub == "LEDGERENTRIES.LIST" || sub == "ACCOUNTINGALLOCATIONS.LIST") {
                                glTransactionsCount++;
                                while (!(xml.tokenType() == QXmlStreamReader::EndElement && (xml.name().toString().toUpper() == sub)) && !xml.atEnd()) {
                                    xml.readNext();
                                    if (xml.isStartElement()) {
                                        QString lsub = xml.name().toString().toUpper();
                                        if (lsub == "AMOUNT") {
                                            double amt = xml.readElementText().toDouble();
                                            if (amt < 0) totalDr += std::abs(amt);
                                            else totalCr += amt;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (!foundAnyTallyEnvelope && accountsCount == 0 && vouchersCount == 0 && itemsCount == 0) {
        report["message"] = "File is not a valid Tally XML data file.";
        report["error"] = report["message"];
        return report;
    }

    report["valid"] = true;
    report["success"] = true;
    report["company_name"] = companyName;
    report["companyName"] = companyName;
    report["gstin"] = gstin;
    report["financial_year"] = financialYear;
    report["financialYear"] = financialYear;
    report["unitsCount"] = unitsCount;
    report["groupsCount"] = groupsCount;
    report["accountsCount"] = accountsCount;
    report["ledgersCount"] = accountsCount;
    report["itemsCount"] = itemsCount;
    report["stockItemsCount"] = itemsCount;
    report["totalVouchersCount"] = vouchersCount;
    report["vouchersCount"] = vouchersCount;
    report["glTransactionsCount"] = glTransactionsCount;
    report["totalDr"] = totalDr;
    report["totalCr"] = totalCr;
    report["debitSum"] = totalDr;
    report["creditSum"] = totalCr;
    double glDiff = std::abs(totalDr - totalCr);
    report["glDiscrepancy"] = glDiff;
    report["isBalanced"] = (glDiff < 0.01);

    return report;
}

bool TallyDataMigrator::migrate_tally_data(const QString& tallyPath, const QString& targetDbPath) {
    DatabaseManager& db = DatabaseManager::instance();
    if (!targetDbPath.isEmpty()) {
        db.switchDatabase(targetDbPath);
    }
    db.ensureTablesExist();

    QString cleanP = normalizePath(tallyPath);
    QFileInfo fi(cleanP);
    if (!fi.exists()) {
        fi.setFile(QDir::current().filePath(cleanP));
        cleanP = fi.absoluteFilePath();
    }

    QStringList xmlFiles;
    if (fi.isDir()) {
        QDir dir(cleanP);
        QStringList entries = dir.entryList(QStringList() << "*.xml" << "*.XML", QDir::Files, QDir::Name);
        for (const QString& e : entries) {
            xmlFiles << dir.filePath(e);
        }
    } else if (fi.isFile()) {
        xmlFiles << cleanP;
    }

    if (xmlFiles.isEmpty()) {
        emit migrationFinished(false, "No Tally XML files found to import.");
        return false;
    }

    updateProgress(5, "Scanning Tally XML files and decoding encodings...");
    db.beginTransaction();

    TallyMigrationStats stats;
    QString detectedCompanyName = "Tally Prime Company";
    QString detectedGstin = "";
    QSet<QString> encounteredFiscalYears;

    struct TallyGroup {
        QString name;
        QString parent = "Primary";
        bool isRevenue = false;
        bool affectsGrossProfit = false;
        bool isDeemedPositive = false;
    };

    struct TallyStockGroup {
        QString name;
        QString parent = "Primary";
    };

    struct TallyLedger {
        QString name;
        QString parent;
        QString gstin;
        QString pan;
        QString state;
        QString address;
        QString pincode;
        QString bankAccount;
        QString ifsc;
        QString bankName;
        double openingBalance = 0.0;
        QString balanceType = "Dr";
        int creditDays = 30;
        double creditLimit = 0.0;
    };

    struct TallyStockItem {
        QString name;
        QString parent;
        QString unit;
        QString hsn;
        double gstRate = 0.0;
        double openingQty = 0.0;
        double openingRate = 0.0;
        double openingValue = 0.0;
    };

    struct TallyVoucherSplit {
        QString ledger;
        double amount = 0.0;
        QString drCr = "Dr";
        QString narration;
    };

    struct TallyVoucherItem {
        QString item;
        double qty = 0.0;
        double rate = 0.0;
        double amount = 0.0;
        QString unit = "Qtl.";
        QString batch;
        QString godown;
    };

    struct TallyVoucher {
        QString vchType;
        QString vchNo;
        QString dateStr;
        QString partyName;
        QString partyGstin;
        QString partyState;
        QString partyPincode;
        QString narration;
        QList<TallyVoucherSplit> splits;
        QList<TallyVoucherItem> items;
    };

    QList<TallyGroup> groups;
    QList<TallyStockGroup> stockGroups;
    QList<TallyLedger> ledgers;
    QList<TallyStockItem> stockItems;
    QList<TallyVoucher> vouchers;
    QMap<QString, QString> groupParentMap;

    int fileIdx = 0;
    for (const QString& xmlFile : xmlFiles) {
        fileIdx++;
        updateProgress(10 + (fileIdx * 20 / xmlFiles.size()), QString("Parsing %1...").arg(QFileInfo(xmlFile).fileName()));

        QString content = readAndSanitizeXmlFile(xmlFile);
        if (content.isEmpty()) continue;

        QXmlStreamReader xml(content);
        while (!xml.atEnd() && !xml.hasError()) {
            QXmlStreamReader::TokenType token = xml.readNext();
            if (token == QXmlStreamReader::StartElement) {
                QString tag = xml.name().toString().toUpper();

                if (tag == "SVCURRENTCOMPANY" || tag == "CURRENTCOMPANY") {
                    QString comp = xml.readElementText().trimmed();
                    if (!comp.isEmpty()) detectedCompanyName = comp;
                }
                else if (tag == "UNIT") {
                    QString name = xml.attributes().value("NAME").toString();
                    int decPlaces = 0;

                    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString().toUpper() == "UNIT") && !xml.atEnd()) {
                        xml.readNext();
                        if (xml.isStartElement()) {
                            QString sub = xml.name().toString().toUpper();
                            if (sub == "NAME" && name.isEmpty()) name = xml.readElementText();
                            else if (sub == "DECIMALPLACES") decPlaces = xml.readElementText().toInt();
                        }
                    }
                    if (!name.isEmpty()) {
                        db.executeNonQuery(
                            "INSERT OR REPLACE INTO stock_units (unit_name, decimal_places) VALUES (?, ?);",
                            {name, decPlaces}
                        );
                        stats.totalUnits++;
                    }
                }
                else if (tag == "STOCKGROUP") {
                    TallyStockGroup sg;
                    sg.name = xml.attributes().value("NAME").toString();
                    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString().toUpper() == "STOCKGROUP") && !xml.atEnd()) {
                        xml.readNext();
                        if (xml.isStartElement()) {
                            QString sub = xml.name().toString().toUpper();
                            if (sub == "NAME" && sg.name.isEmpty()) sg.name = xml.readElementText();
                            else if (sub == "PARENT") sg.parent = xml.readElementText();
                        }
                    }
                    if (!sg.name.isEmpty()) {
                        stockGroups.append(sg);
                    }
                }
                else if (tag == "GROUP") {
                    TallyGroup g;
                    g.name = xml.attributes().value("NAME").toString();
                    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString().toUpper() == "GROUP") && !xml.atEnd()) {
                        xml.readNext();
                        if (xml.isStartElement()) {
                            QString sub = xml.name().toString().toUpper();
                            if (sub == "NAME" && g.name.isEmpty()) g.name = xml.readElementText();
                            else if (sub == "PARENT") g.parent = xml.readElementText();
                            else if (sub == "ISREVENUE") g.isRevenue = (xml.readElementText().trimmed().toLower() == "yes");
                            else if (sub == "AFFECTSGROSSPROFIT") g.affectsGrossProfit = (xml.readElementText().trimmed().toLower() == "yes");
                            else if (sub == "ISDEEMEDPOSITIVE") g.isDeemedPositive = (xml.readElementText().trimmed().toLower() == "yes");
                        }
                    }
                    if (!g.name.isEmpty()) {
                        if (g.parent.isEmpty()) g.parent = "Primary";
                        groups.append(g);
                        groupParentMap[g.name] = g.parent;
                    }
                }
                else if (tag == "LEDGER") {
                    TallyLedger l;
                    l.name = xml.attributes().value("NAME").toString();
                    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString().toUpper() == "LEDGER") && !xml.atEnd()) {
                        xml.readNext();
                        if (xml.isStartElement()) {
                            QString sub = xml.name().toString().toUpper();
                            if (sub == "NAME" && l.name.isEmpty()) l.name = xml.readElementText();
                            else if (sub == "PARENT") l.parent = xml.readElementText();
                            else if (sub == "PARTYGSTIN" || sub == "GSTIN" || sub == "GSTREGISTRATIONNUMBER") l.gstin = xml.readElementText().trimmed().toUpper();
                            else if (sub == "INCOMETAXNUMBER" || sub == "PAN") l.pan = xml.readElementText().trimmed().toUpper();
                            else if (sub == "PARTYSTATE" || sub == "STATENAME" || sub == "LEDSTATENAME") l.state = xml.readElementText();
                            else if (sub == "PINCODE") l.pincode = xml.readElementText();
                            else if (sub == "ADDRESS") {
                                QString a = xml.readElementText().trimmed();
                                if (!a.isEmpty()) {
                                    l.address = l.address.isEmpty() ? a : (l.address + ", " + a);
                                }
                            }
                            else if (sub == "ACCOUNTNUMBER" || sub == "BANKDETAILS") l.bankAccount = xml.readElementText();
                            else if (sub == "IFSCODE") l.ifsc = xml.readElementText();
                            else if (sub == "BANKNAME") l.bankName = xml.readElementText();
                            else if (sub == "OPENINGBALANCE") {
                                double bal = xml.readElementText().toDouble();
                                if (bal < 0) {
                                    l.openingBalance = std::abs(bal);
                                    l.balanceType = "Dr";
                                } else {
                                    l.openingBalance = bal;
                                    l.balanceType = "Cr";
                                }
                            }
                            else if (sub == "ISDEEMEDPOSITIVE") {
                                if (xml.readElementText().trimmed().toLower() == "yes") l.balanceType = "Dr";
                            }
                            else if (sub == "BILLCREDITPERIOD") l.creditDays = xml.readElementText().toInt();
                            else if (sub == "CREDITLIMIT") l.creditLimit = xml.readElementText().toDouble();
                        }
                    }
                    if (!l.name.isEmpty()) {
                        ledgers.append(l);
                        if (!l.gstin.isEmpty() && detectedGstin.isEmpty()) {
                            detectedGstin = l.gstin;
                        }
                    }
                }
                else if (tag == "STOCKITEM") {
                    TallyStockItem itm;
                    itm.name = xml.attributes().value("NAME").toString();
                    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString().toUpper() == "STOCKITEM") && !xml.atEnd()) {
                        xml.readNext();
                        if (xml.isStartElement()) {
                            QString sub = xml.name().toString().toUpper();
                            if (sub == "NAME" && itm.name.isEmpty()) itm.name = xml.readElementText();
                            else if (sub == "PARENT") itm.parent = xml.readElementText();
                            else if (sub == "BASEUNITS") itm.unit = xml.readElementText();
                            else if (sub == "HSNCODE") itm.hsn = xml.readElementText();
                            else if (sub == "GSTRATE") itm.gstRate = xml.readElementText().toDouble();
                            else if (sub == "OPENINGBALANCE") {
                                QString rawOp = xml.readElementText().trimmed();
                                itm.openingQty = rawOp.split(" ").first().toDouble();
                            }
                            else if (sub == "OPENINGRATE") itm.openingRate = xml.readElementText().toDouble();
                            else if (sub == "OPENINGVALUE") itm.openingValue = std::abs(xml.readElementText().toDouble());
                        }
                    }
                    if (!itm.name.isEmpty()) {
                        if (itm.openingValue == 0.0 && itm.openingQty > 0.0 && itm.openingRate > 0.0) {
                            itm.openingValue = itm.openingQty * itm.openingRate;
                        }
                        stockItems.append(itm);
                    }
                }
                else if (tag == "VOUCHER") {
                    TallyVoucher v;
                    v.vchType = xml.attributes().value("VCHTYPE").toString();
                    if (v.vchType.isEmpty()) v.vchType = "Journal";

                    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString().toUpper() == "VOUCHER") && !xml.atEnd()) {
                        xml.readNext();
                        if (xml.isStartElement()) {
                            QString sub = xml.name().toString().toUpper();
                            if (sub == "VOUCHERTYPENAME" && (v.vchType.isEmpty() || v.vchType == "Journal")) v.vchType = xml.readElementText();
                            else if (sub == "VOUCHERNUMBER") v.vchNo = xml.readElementText();
                            else if (sub == "DATE") v.dateStr = parseDateStr(xml.readElementText());
                            else if (sub == "PARTYLEDGERNAME" || (sub == "PARTYNAME" && v.partyName.isEmpty())) v.partyName = xml.readElementText();
                            else if (sub == "PARTYGSTIN") v.partyGstin = xml.readElementText().trimmed().toUpper();
                            else if (sub == "PARTYPINCODE") v.partyPincode = xml.readElementText().trimmed();
                            else if (sub == "STATENAME" || sub == "PARTYSTATE") v.partyState = xml.readElementText().trimmed();
                            else if (sub == "NARRATION") v.narration = xml.readElementText();
                            else if (sub == "ALLINVENTORYENTRIES.LIST") {
                                TallyVoucherItem vi;
                                while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString().toUpper() == "ALLINVENTORYENTRIES.LIST") && !xml.atEnd()) {
                                    xml.readNext();
                                    if (xml.isStartElement()) {
                                        QString isub = xml.name().toString().toUpper();
                                        if (isub == "STOCKITEMNAME") vi.item = xml.readElementText();
                                        else if (isub == "RATE") vi.rate = xml.readElementText().split("/").first().toDouble();
                                        else if (isub == "AMOUNT") vi.amount = std::abs(xml.readElementText().toDouble());
                                        else if (isub == "ACTUALQTY" || isub == "BILLEDQTY") {
                                            QString qstr = xml.readElementText().trimmed();
                                            vi.qty = qstr.split(" ").first().toDouble();
                                            if (qstr.contains(" ")) vi.unit = qstr.split(" ").last();
                                        }
                                        else if (isub == "BATCHNAME") vi.batch = xml.readElementText();
                                        else if (isub == "GODOWNNAME") vi.godown = xml.readElementText();
                                        else if (isub == "ACCOUNTINGALLOCATIONS.LIST") {
                                            TallyVoucherSplit vs;
                                            while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString().toUpper() == "ACCOUNTINGALLOCATIONS.LIST") && !xml.atEnd()) {
                                                xml.readNext();
                                                if (xml.isStartElement()) {
                                                    QString asub = xml.name().toString().toUpper();
                                                    if (asub == "LEDGERNAME") vs.ledger = xml.readElementText();
                                                    else if (asub == "AMOUNT") {
                                                        double a = xml.readElementText().toDouble();
                                                        vs.drCr = (a < 0) ? "Dr" : "Cr";
                                                        vs.amount = std::abs(a);
                                                    }
                                                    else if (asub == "ISDEEMEDPOSITIVE") {
                                                        if (xml.readElementText().trimmed().toLower() == "yes") vs.drCr = "Dr";
                                                    }
                                                }
                                            }
                                            if (!vs.ledger.isEmpty() && vs.amount > 0.0) {
                                                v.splits.append(vs);
                                            }
                                        }
                                    }
                                }
                                if (!vi.item.isEmpty()) {
                                    v.items.append(vi);
                                }
                            }
                            else if (sub == "ALLLEDGERENTRIES.LIST" || sub == "LEDGERENTRIES.LIST") {
                                TallyVoucherSplit vs;
                                while (!(xml.tokenType() == QXmlStreamReader::EndElement && (xml.name().toString().toUpper() == sub)) && !xml.atEnd()) {
                                    xml.readNext();
                                    if (xml.isStartElement()) {
                                        QString lsub = xml.name().toString().toUpper();
                                        if (lsub == "LEDGERNAME") vs.ledger = xml.readElementText();
                                        else if (lsub == "AMOUNT") {
                                            double a = xml.readElementText().toDouble();
                                            vs.drCr = (a < 0) ? "Dr" : "Cr";
                                            vs.amount = std::abs(a);
                                        }
                                        else if (lsub == "ISDEEMEDPOSITIVE") {
                                            if (xml.readElementText().trimmed().toLower() == "yes") vs.drCr = "Dr";
                                        }
                                    }
                                }
                                if (!vs.ledger.isEmpty() && vs.amount > 0.0) {
                                    v.splits.append(vs);
                                }
                            }
                        }
                    }

                    if (!v.dateStr.isEmpty()) {
                        encounteredFiscalYears.insert(computeFy(v.dateStr));
                    }
                    vouchers.append(v);
                }
            }
        }
    }

    // STEP 1: Insert Company Profile & Financial Years (35%)
    updateProgress(35, "Writing Company Profile & Fiscal Years...");
    db.executeNonQuery(
        "INSERT OR REPLACE INTO company_info (id, company_name, gstin, state, state_code) VALUES (1, ?, ?, ?, ?);",
        {detectedCompanyName, detectedGstin, "Haryana", "06"}
    );

    if (encounteredFiscalYears.isEmpty()) {
        encounteredFiscalYears.insert("FY 2026-27");
    }
    for (const QString& fy : encounteredFiscalYears) {
        // e.g. FY 2026-27 -> 2026-04-01 to 2027-03-31
        QString sYrStr = fy.mid(3, 4);
        int sYr = sYrStr.toInt();
        if (sYr >= 2000) {
            QString sDate = QString("%1-04-01").arg(sYr);
            QString eDate = QString("%1-03-31").arg(sYr + 1);
            db.executeNonQuery(
                "INSERT OR IGNORE INTO financial_years (year_name, start_date, end_date, is_active) VALUES (?, ?, ?, 1);",
                {fy, sDate, eDate}
            );
        }
    }

    // STEP 2: Stock Groups & Units (45%)
    updateProgress(45, "Writing Stock Groups and Units...");
    for (const auto& sg : stockGroups) {
        db.executeNonQuery(
            "INSERT OR IGNORE INTO stock_groups (group_name) VALUES (?);",
            {sg.name}
        );
    }

    // STEP 3: Account Groups with 4-Code Hierarchy & Nature (55%)
    updateProgress(55, "Classifying and writing Account Groups hierarchy...");
    for (const auto& g : groups) {
        TallyGroupMeta meta = mapTallyGroup(g.name, g.parent, groupParentMap);
        db.executeNonQuery(
            "INSERT OR REPLACE INTO account_groups (name, parent_group_name, nature, code1st, code2nd, extract_in_balance_sheet) "
            "VALUES (?, ?, ?, ?, ?, ?);",
            {g.name, g.parent, meta.nature, meta.code1st, meta.code2nd, meta.extractBs}
        );
        stats.totalGroups++;
    }

    // STEP 4: Master Ledgers / Parties (70%)
    updateProgress(70, "Writing Master Ledgers and Parties...");
    QMap<QString, int> partyNameToId;
    QMap<QString, QString> partyNameToGroup;
    QMap<QString, int> partyNameToGroupCode;

    int partyIdCounter = 1;
    for (const auto& l : ledgers) {
        int pId = partyIdCounter++;
        QString st = resolveState(l.state);
        QString stCode = getStateCode(st);
        QString pan = l.pan;
        if (pan.isEmpty() && l.gstin.length() == 15) {
            pan = l.gstin.mid(2, 10);
        }
        if (st.isEmpty() && l.gstin.length() >= 2) {
            stCode = l.gstin.left(2);
            for (auto it = s_stateCodes.constBegin(); it != s_stateCodes.constEnd(); ++it) {
                if (it.value() == stCode) {
                    st = it.key();
                    break;
                }
            }
        }

        TallyGroupMeta gMeta = mapTallyGroup(l.parent, groupParentMap.value(l.parent), groupParentMap);

        QString partyType = "General";
        if (gMeta.code1st == 8) partyType = "Buyer";
        else if (gMeta.code1st == 11) partyType = "Vendor";
        else if (gMeta.code1st == 3) partyType = "Bank";
        else if (gMeta.code1st == 4) partyType = "Cash";
        else if (gMeta.code1st >= 16 && gMeta.code1st <= 24) partyType = "Nominal";

        db.executeNonQuery(
            "INSERT OR REPLACE INTO parties (id, legacy_id, name, mailing_name, group_name, group_id, group_code, party_type, calc_direct_expense, "
            "address, state, state_code, pincode, gstin, pan, bank_name, bank_account, ifsc_code, opening_balance, balance_type, credit_days, credit_limit) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {pId, pId, l.name, l.name, l.parent, gMeta.code1st, gMeta.code1st, partyType, (gMeta.code1st == 23 ? 1 : 0),
             l.address, st, stCode, l.pincode, l.gstin, pan, l.bankName, l.bankAccount, l.ifsc, l.openingBalance, l.balanceType, l.creditDays, l.creditLimit}
        );

        partyNameToId[l.name] = pId;
        partyNameToGroup[l.name] = l.parent;
        partyNameToGroupCode[l.name] = gMeta.code1st;
        stats.totalAccounts++;
    }

    // STEP 5: Stock Items (80%)
    updateProgress(80, "Writing Stock Items...");
    int itmIdx = 1;
    for (const auto& itm : stockItems) {
        QString codeStr = QString("ITEM-%1").arg(itmIdx++);
        db.executeNonQuery(
            "INSERT OR REPLACE INTO stock_items (name, code, trading_group, hsn_code, unit, gst_rate, opening_qty, opening_rate, opening_value) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {itm.name, codeStr, itm.parent, itm.hsn, itm.unit.isEmpty() ? "Qtl." : itm.unit, itm.gstRate, itm.openingQty, itm.openingRate, itm.openingValue}
        );

        db.executeNonQuery(
            "INSERT OR REPLACE INTO inventory (item_code, item_name, category, current_stock_qtl, sale_rate, gst_rate, packing_kg) "
            "VALUES (?, ?, ?, ?, ?, ?, 0.0);",
            {codeStr, itm.name, itm.parent, itm.openingQty, itm.openingRate, itm.gstRate}
        );
        stats.totalStockItems++;
    }

    // STEP 6: Vouchers, Invoices & Double-Entry Transactions (90%)
    updateProgress(90, "Writing Double-Entry Transactions and Invoices...");
    for (const auto& v : vouchers) {
        QString vNo = v.vchNo;
        if (vNo.isEmpty()) vNo = QString("TLY-%1").arg(stats.totalVouchers + 1);
        QString dateStr = v.dateStr.isEmpty() ? "2026-04-01" : v.dateStr;
        QString fy = computeFy(dateStr);

        QVariant fyIdVar = db.executeScalar("SELECT id FROM financial_years WHERE year_name = ?;", {fy});
        int fyId = fyIdVar.toInt();
        if (fyId <= 0) {
            QString sYrStr = fy.mid(3, 4);
            int sYr = sYrStr.toInt();
            if (sYr >= 2000) {
                QString sDate = QString("%1-04-01").arg(sYr);
                QString eDate = QString("%1-03-31").arg(sYr + 1);
                db.executeNonQuery(
                    "INSERT OR IGNORE INTO financial_years (year_name, start_date, end_date, is_active) VALUES (?, ?, ?, 1);",
                    {fy, sDate, eDate}
                );
                fyIdVar = db.executeScalar("SELECT id FROM financial_years WHERE year_name = ?;", {fy});
                fyId = fyIdVar.toInt();
            }
            if (fyId <= 0) fyId = 1;
        }

        double totalAmt = 0.0;
        for (const auto& s : v.splits) {
            if (s.drCr == "Dr") totalAmt += s.amount;
        }
        if (totalAmt == 0.0 && !v.splits.isEmpty()) {
            totalAmt = v.splits.first().amount;
        }

        QString headerParty = v.partyName;
        if (headerParty.isEmpty() && !v.splits.isEmpty()) {
            headerParty = v.splits.first().ledger;
        }

        // Auto-create header party if not already created
        if (!partyNameToId.contains(headerParty) && !headerParty.trimmed().isEmpty()) {
            int newId = partyIdCounter++;
            TallyGroupMeta gMeta = mapTallyGroup(headerParty, "", groupParentMap);
            QString pType = (gMeta.code1st == 8) ? "Buyer" : ((gMeta.code1st == 11) ? "Vendor" : "General");
            QString grpName = (gMeta.code1st == 8) ? "Sundry Debtors" : ((gMeta.code1st == 11) ? "Sundry Creditors" : "Current Liabilities");
            db.executeNonQuery(
                "INSERT OR REPLACE INTO parties (id, legacy_id, name, mailing_name, group_name, group_id, group_code, party_type, calc_direct_expense) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, 0);",
                {newId, newId, headerParty, headerParty, grpName, gMeta.code1st, gMeta.code1st, pType}
            );
            partyNameToId[headerParty] = newId;
            partyNameToGroup[headerParty] = grpName;
            partyNameToGroupCode[headerParty] = gMeta.code1st;
            stats.totalAccounts++;
        }

        int partyId = partyNameToId.value(headerParty, 0);
        QVariant partyIdParam = (partyId > 0) ? QVariant(partyId) : QVariant();

        // 1. Insert into vouchers table
        db.executeNonQuery(
            "INSERT OR REPLACE INTO vouchers (voucher_no, voucher_type, voucher_date, party_id, party_name, account_type, amount, narration, financial_year) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {vNo, v.vchType, dateStr, partyIdParam, headerParty, v.vchType, totalAmt, v.narration, fy}
        );
        stats.totalVouchers++;

        // 2. Insert into sales_invoices or purchase_invoices and stock_transactions
        if (v.vchType.compare("Sales", Qt::CaseInsensitive) == 0) {
            if (!v.items.isEmpty()) {
                int sRowIdx = 1;
                for (const auto& vi : v.items) {
                    QString itemName = vi.item;
                    double qty = vi.qty;
                    double rate = vi.rate;
                    double taxAmt = (vi.amount > 0.0) ? vi.amount : (qty * rate > 0.0 ? qty * rate : totalAmt);

                    db.executeNonQuery(
                        "INSERT OR REPLACE INTO sales_invoices (invoice_no, voucher_no, invoice_date, customer_id, customer_name, "
                        "item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, total_amount, narration, financial_year) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                        {vNo, vNo, dateStr, partyIdParam, headerParty, itemName, static_cast<int>(qty), qty, rate, taxAmt, totalAmt, v.narration, fy}
                    );

                    db.executeNonQuery(
                        "INSERT INTO sales_invoice_items ("
                        "invoice_id, invoice_no, item_id, item_name, grade, bag_count, packing, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, total_amount) "
                        "VALUES ((SELECT id FROM sales_invoices WHERE invoice_no = ? LIMIT 1), ?, (SELECT id FROM stock_items WHERE name = ? LIMIT 1), ?, '', ?, 'Loose', ?, ?, ?, 0.0, ?);",
                        {vNo, vNo, itemName, itemName, static_cast<int>(qty), qty, rate, taxAmt, totalAmt}
                    );

                    db.executeNonQuery(
                        "INSERT INTO stock_transactions ("
                        "fy_id, financial_year, voucher_no, voucher_date, trans_type, voucher_type, "
                        "party_id, party_name, bill_no, item_id, item_code, item_name, "
                        "bags, packing, weight_qtl, rate, amount, taxable_amount, tax, "
                        "tax_type, narration, row_no) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, (SELECT id FROM stock_items WHERE name = ? LIMIT 1), (SELECT code FROM stock_items WHERE name = ? LIMIT 1), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                        {
                            fyId, fy, vNo, dateStr, "Sale", "Sales",
                            partyIdParam, headerParty, vNo,
                            itemName, itemName, itemName,
                            static_cast<int>(qty), "Loose", qty, rate, taxAmt, taxAmt, 0.0,
                            "GST", v.narration, sRowIdx++
                        }
                    );
                }
            } else {
                db.executeNonQuery(
                    "INSERT OR REPLACE INTO sales_invoices (invoice_no, voucher_no, invoice_date, customer_id, customer_name, "
                    "item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, total_amount, narration, financial_year) "
                    "VALUES (?, ?, ?, ?, ?, '', 0, 0, 0, ?, ?, ?, ?);",
                    {vNo, vNo, dateStr, partyIdParam, headerParty, totalAmt, totalAmt, v.narration, fy}
                );
            }
        } else if (v.vchType.compare("Purchase", Qt::CaseInsensitive) == 0) {
            if (!v.items.isEmpty()) {
                int pRowIdx = 1;
                for (const auto& vi : v.items) {
                    QString itemName = vi.item;
                    double qty = vi.qty;
                    double rate = vi.rate;
                    double taxAmt = (vi.amount > 0.0) ? vi.amount : (qty * rate > 0.0 ? qty * rate : totalAmt);

                    db.executeNonQuery(
                        "INSERT OR REPLACE INTO purchase_invoices (invoice_no, voucher_no, invoice_date, supplier_id, supplier_name, "
                        "item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, total_amount, narration, financial_year) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                        {vNo, vNo, dateStr, partyIdParam, headerParty, itemName, static_cast<int>(qty), qty, rate, taxAmt, totalAmt, v.narration, fy}
                    );

                    db.executeNonQuery(
                        "INSERT INTO purchase_invoice_items ("
                        "invoice_id, invoice_no, item_id, item_name, grade, bag_count, packing, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, total_amount) "
                        "VALUES ((SELECT id FROM purchase_invoices WHERE invoice_no = ? LIMIT 1), ?, (SELECT id FROM stock_items WHERE name = ? LIMIT 1), ?, '', ?, 'Loose', ?, ?, ?, 0.0, ?);",
                        {vNo, vNo, itemName, itemName, static_cast<int>(qty), qty, rate, taxAmt, totalAmt}
                    );

                    db.executeNonQuery(
                        "INSERT INTO stock_transactions ("
                        "fy_id, financial_year, voucher_no, voucher_date, trans_type, voucher_type, "
                        "party_id, party_name, bill_no, item_id, item_code, item_name, "
                        "bags, packing, weight_qtl, rate, amount, taxable_amount, tax, "
                        "tax_type, narration, row_no) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, (SELECT id FROM stock_items WHERE name = ? LIMIT 1), (SELECT code FROM stock_items WHERE name = ? LIMIT 1), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                        {
                            fyId, fy, vNo, dateStr, "Purc", "Purchase",
                            partyIdParam, headerParty, vNo,
                            itemName, itemName, itemName,
                            static_cast<int>(qty), "Loose", qty, rate, taxAmt, taxAmt, 0.0,
                            "GST", v.narration, pRowIdx++
                        }
                    );
                }
            } else {
                db.executeNonQuery(
                    "INSERT OR REPLACE INTO purchase_invoices (invoice_no, voucher_no, invoice_date, supplier_id, supplier_name, "
                    "item_name, bag_count, weight_qtl, rate_per_qtl, taxable_amount, total_amount, narration, financial_year) "
                    "VALUES (?, ?, ?, ?, ?, '', 0, 0, 0, ?, ?, ?, ?);",
                    {vNo, vNo, dateStr, partyIdParam, headerParty, totalAmt, totalAmt, v.narration, fy}
                );
            }
        }

        // 3. Insert Double-Entry Ledger Splits into transactions table
        int rowIdx = 1;
        for (const auto& s : v.splits) {
            // Auto-create split party if not already created
            if (!partyNameToId.contains(s.ledger) && !s.ledger.trimmed().isEmpty()) {
                int newId = partyIdCounter++;
                TallyGroupMeta gMeta = mapTallyGroup(s.ledger, "", groupParentMap);
                QString pType = (gMeta.code1st == 10) ? "Nominal" : "General";
                QString grpName = (gMeta.code1st == 10) ? "Duties & Taxes" : "Current Liabilities";
                db.executeNonQuery(
                    "INSERT OR REPLACE INTO parties (id, legacy_id, name, mailing_name, group_name, group_id, group_code, party_type, calc_direct_expense) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, 0);",
                    {newId, newId, s.ledger, s.ledger, grpName, gMeta.code1st, gMeta.code1st, pType}
                );
                partyNameToId[s.ledger] = newId;
                partyNameToGroup[s.ledger] = grpName;
                partyNameToGroupCode[s.ledger] = gMeta.code1st;
                stats.totalAccounts++;
            }

            int splitPartyId = partyNameToId.value(s.ledger, 0);
            QVariant splitPartyIdParam = (splitPartyId > 0) ? QVariant(splitPartyId) : QVariant();
            QVariant accCodeParam = (splitPartyId > 0) ? QVariant(splitPartyId) : QVariant();

            QString oppAcc = headerParty;
            if (s.ledger == headerParty && v.splits.size() > 1) {
                for (const auto& os : v.splits) {
                    if (os.ledger != s.ledger) {
                        oppAcc = os.ledger;
                        break;
                    }
                }
            }

            db.executeNonQuery(
                "INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_type, trans_type, voucher_date, "
                "account_code, party_id, party_name, opposing_account, dr_cr, amount, taxable_amount, narration, row_no) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {fyId, fy, vNo, v.vchType, v.vchType, dateStr, accCodeParam, splitPartyIdParam, s.ledger, oppAcc, s.drCr, s.amount, s.amount, s.narration.isEmpty() ? v.narration : s.narration, rowIdx++}
            );
            stats.totalTransactions++;
            if (s.drCr == "Dr") stats.totalDr += s.amount;
            else stats.totalCr += s.amount;
        }
    }

    db.commit();
    updateProgress(100, "Tally migration completed successfully!");

    QString summaryMsg = QString("Successfully imported %1 vouchers, %2 accounts, %3 items and %4 units from Tally Prime!")
                             .arg(stats.totalVouchers)
                             .arg(stats.totalAccounts)
                             .arg(stats.totalStockItems)
                             .arg(stats.totalUnits);

    emit migrationFinished(true, summaryMsg);
    return true;
}

} // namespace MahadevERP
