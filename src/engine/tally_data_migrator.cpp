#include "tally_data_migrator.h"
#include "../database_manager.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QXmlStreamReader>
#include <QFileDialog>
#include <QCoreApplication>
#include <QDebug>
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

    // 1. Tally compact standard YYYYMMDD (e.g. 20240220)
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
    if (!d.isValid()) return "2026-2027";

    int y = d.year();
    if (d.month() >= 4) {
        return QString("%1-%2").arg(y).arg(y + 1);
    } else {
        return QString("%1-%2").arg(y - 1).arg(y);
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
    QString filter = "Tally Data Files (*.xml Master.xml DayBook.xml *.xlsx);;XML Files (*.xml);;All Files (*.*)";
    QString file = QFileDialog::getOpenFileName(nullptr, "Select Tally XML or Excel File", startDir, filter);
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
        QStringList entries = dir.entryList(QStringList() << "*.xml" << "*.XML", QDir::Files);
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

    bool foundAnyTallyEnvelope = false;

    for (const QString& xmlFile : xmlFilesToInspect) {
        QFile file(xmlFile);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        QXmlStreamReader xml(&file);
        while (!xml.atEnd() && !xml.hasError()) {
            QXmlStreamReader::TokenType token = xml.readNext();
            if (token == QXmlStreamReader::StartElement) {
                QString name = xml.name().toString().toUpper();
                if (name == "ENVELOPE" || name == "TALLYMESSAGE") {
                    foundAnyTallyEnvelope = true;
                } else if (name == "UNIT") {
                    unitsCount++;
                } else if (name == "GROUP") {
                    groupsCount++;
                } else if (name == "LEDGER") {
                    accountsCount++;
                } else if (name == "STOCKITEM") {
                    itemsCount++;
                } else if (name == "VOUCHER") {
                    vouchersCount++;
                    // Inspect internal ledger entries for double entry calculations
                    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString().toUpper() == "VOUCHER") && !xml.atEnd()) {
                        xml.readNext();
                        if (xml.isStartElement() && xml.name().toString().toUpper() == "ALLLEDGERENTRIES.LIST") {
                            glTransactionsCount++;
                            while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString().toUpper() == "ALLLEDGERENTRIES.LIST") && !xml.atEnd()) {
                                xml.readNext();
                                if (xml.isStartElement() && xml.name().toString().toUpper() == "AMOUNT") {
                                    double amt = xml.readElementText().toDouble();
                                    if (amt < 0) {
                                        totalDr += std::abs(amt);
                                    } else {
                                        totalCr += amt;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        file.close();
    }

    if (!foundAnyTallyEnvelope && accountsCount == 0 && vouchersCount == 0) {
        report["message"] = "File is not a valid Tally XML data file.";
        report["error"] = report["message"];
        return report;
    }

    report["valid"] = true;
    report["success"] = true;
    report["companyName"] = companyName;
    report["unitsCount"] = unitsCount;
    report["groupsCount"] = groupsCount;
    report["accountsCount"] = accountsCount;
    report["ledgersCount"] = accountsCount;
    report["itemsCount"] = itemsCount;
    report["stockItemsCount"] = itemsCount;
    report["totalVouchersCount"] = vouchersCount;
    report["vouchersCount"] = vouchersCount;
    report["glTransactionsCount"] = glTransactionsCount;
    report["stockTxCount"] = vouchersCount;
    report["millingCount"] = 0;
    report["totalDr"] = totalDr;
    report["totalCr"] = totalCr;
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
        QStringList entries = dir.entryList(QStringList() << "*.xml" << "*.XML", QDir::Files);
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

    updateProgress(10, "Scanning Tally XML files...");
    db.beginTransaction();

    TallyMigrationStats stats;

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
        QString unit = "Qtl.";
        QString hsn = "1006";
        double gstRate = 5.0;
        double openingQty = 0.0;
        double openingRate = 0.0;
        double openingValue = 0.0;
    };

    QList<TallyLedger> ledgers;
    QList<TallyStockItem> stockItems;

    int fileIdx = 0;
    for (const QString& xmlFile : xmlFiles) {
        fileIdx++;
        updateProgress(15 + (fileIdx * 10 / xmlFiles.size()), QString("Parsing %1...").arg(QFileInfo(xmlFile).fileName()));

        QFile file(xmlFile);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        QXmlStreamReader xml(&file);
        while (!xml.atEnd() && !xml.hasError()) {
            QXmlStreamReader::TokenType token = xml.readNext();
            if (token == QXmlStreamReader::StartElement) {
                QString tag = xml.name().toString().toUpper();

                if (tag == "UNIT") {
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
                else if (tag == "GROUP") {
                    QString name = xml.attributes().value("NAME").toString();
                    QString parent = "Primary";
                    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString().toUpper() == "GROUP") && !xml.atEnd()) {
                        xml.readNext();
                        if (xml.isStartElement()) {
                            QString sub = xml.name().toString().toUpper();
                            if (sub == "NAME" && name.isEmpty()) name = xml.readElementText();
                            else if (sub == "PARENT") parent = xml.readElementText();
                        }
                    }
                    if (!name.isEmpty()) {
                        db.executeNonQuery(
                            "INSERT OR REPLACE INTO account_groups (name, parent_group_name) VALUES (?, ?);",
                            {name, parent.isEmpty() ? "Primary" : parent}
                        );
                        stats.totalGroups++;
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
                            else if (sub == "INCOMETAXNUMBER") l.pan = xml.readElementText().trimmed().toUpper();
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
                                QString dp = xml.readElementText().trimmed().toLower();
                                if (dp == "yes") l.balanceType = "Dr";
                            }
                            else if (sub == "BILLCREDITPERIOD") l.creditDays = xml.readElementText().toInt();
                            else if (sub == "CREDITLIMIT") l.creditLimit = xml.readElementText().toDouble();
                        }
                    }
                    if (!l.name.isEmpty()) {
                        ledgers.append(l);
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
                            else if (sub == "OPENINGBALANCE") itm.openingQty = xml.readElementText().toDouble();
                            else if (sub == "OPENINGRATE") itm.openingRate = xml.readElementText().toDouble();
                            else if (sub == "OPENINGVALUE") itm.openingValue = std::abs(xml.readElementText().toDouble());
                        }
                    }
                    if (!itm.name.isEmpty()) {
                        stockItems.append(itm);
                    }
                }
                else if (tag == "VOUCHER") {
                    QString vchType = xml.attributes().value("VCHTYPE").toString();
                    if (vchType.isEmpty()) vchType = "Journal";
                    QString vchNo;
                    QString dateStr;
                    QString party;
                    QString narration;

                    struct LedgerEntry {
                        QString ledger;
                        double amount = 0.0;
                        QString drCr = "Dr";
                    };

                    struct InventoryEntry {
                        QString item;
                        double qty = 0.0;
                        double rate = 0.0;
                        double amount = 0.0;
                        QString unit = "Qtl.";
                    };

                    QList<LedgerEntry> vchLedgers;
                    QList<InventoryEntry> vchInventory;

                    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString().toUpper() == "VOUCHER") && !xml.atEnd()) {
                        xml.readNext();
                        if (xml.isStartElement()) {
                            QString sub = xml.name().toString().toUpper();
                            if (sub == "VOUCHERTYPENAME" && vchType.isEmpty()) vchType = xml.readElementText();
                            else if (sub == "VOUCHERNUMBER") vchNo = xml.readElementText();
                            else if (sub == "DATE") dateStr = parseDateStr(xml.readElementText());
                            else if (sub == "PARTYLEDGERNAME" || sub == "PARTYNAME") party = xml.readElementText();
                            else if (sub == "NARRATION") narration = xml.readElementText();
                            else if (sub == "ALLLEDGERENTRIES.LIST") {
                                LedgerEntry le;
                                while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString().toUpper() == "ALLLEDGERENTRIES.LIST") && !xml.atEnd()) {
                                    xml.readNext();
                                    if (xml.isStartElement()) {
                                        QString lsub = xml.name().toString().toUpper();
                                        if (lsub == "LEDGERNAME") le.ledger = xml.readElementText();
                                        else if (lsub == "AMOUNT") {
                                            double a = xml.readElementText().toDouble();
                                            if (a < 0) {
                                                le.amount = std::abs(a);
                                                le.drCr = "Dr";
                                            } else {
                                                le.amount = a;
                                                le.drCr = "Cr";
                                            }
                                        }
                                        else if (lsub == "ISDEEMEDPOSITIVE") {
                                            if (xml.readElementText().trimmed().toLower() == "yes") le.drCr = "Dr";
                                        }
                                    }
                                }
                                if (!le.ledger.isEmpty() && le.amount > 0) {
                                    vchLedgers.append(le);
                                }
                            }
                            else if (sub == "ALLINVENTORYENTRIES.LIST") {
                                InventoryEntry ie;
                                while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name().toString().toUpper() == "ALLINVENTORYENTRIES.LIST") && !xml.atEnd()) {
                                    xml.readNext();
                                    if (xml.isStartElement()) {
                                        QString isub = xml.name().toString().toUpper();
                                        if (isub == "STOCKITEMNAME") ie.item = xml.readElementText();
                                        else if (isub == "RATE") ie.rate = xml.readElementText().toDouble();
                                        else if (isub == "AMOUNT") ie.amount = std::abs(xml.readElementText().toDouble());
                                        else if (isub == "ACTUALQTY" || isub == "BILLEDQTY") {
                                            QString qstr = xml.readElementText();
                                            ie.qty = qstr.split(" ").first().toDouble();
                                        }
                                    }
                                }
                                if (!ie.item.isEmpty()) {
                                    vchInventory.append(ie);
                                }
                            }
                        }
                    }

                    if (vchNo.isEmpty()) {
                        vchNo = QString("TLY-%1").arg(stats.totalVouchers + 1);
                    }
                    if (dateStr.isEmpty()) {
                        dateStr = "2026-04-01";
                    }
                    QString fy = computeFy(dateStr);

                    double vchTotal = 0.0;
                    for (const auto& le : vchLedgers) {
                        if (le.drCr == "Dr") vchTotal += le.amount;
                    }
                    if (vchTotal == 0.0 && !vchLedgers.isEmpty()) {
                        vchTotal = vchLedgers.first().amount;
                    }
                    if (party.isEmpty() && !vchLedgers.isEmpty()) {
                        party = vchLedgers.first().ledger;
                    }

                    // Insert voucher header into vouchers table
                    db.executeNonQuery(
                        "INSERT OR REPLACE INTO vouchers (voucher_no, voucher_type, voucher_date, party_name, account_type, amount, narration, financial_year) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?);",
                        {vchNo, vchType, dateStr, party, vchType, vchTotal, narration, fy}
                    );
                    stats.totalVouchers++;

                    // Insert transaction splits into transactions table
                    for (const auto& le : vchLedgers) {
                        db.executeNonQuery(
                            "INSERT INTO transactions (voucher_no, voucher_type, voucher_date, party_name, amount, dr_cr, narration, financial_year) "
                            "VALUES (?, ?, ?, ?, ?, ?, ?, ?);",
                            {vchNo, vchType, dateStr, le.ledger, le.amount, le.drCr, narration, fy}
                        );
                        stats.totalTransactions++;
                        if (le.drCr == "Dr") stats.totalDr += le.amount;
                        else stats.totalCr += le.amount;
                    }
                }
            }
        }
        file.close();
    }

    // Step 2: Insert master ledgers and parties into SQLite
    updateProgress(60, "Writing master ledgers and accounts...");
    for (const auto& l : ledgers) {
        QString st = resolveState(l.state);
        QString stCode = getStateCode(st);
        QString pan = l.pan;
        if (pan.isEmpty() && l.gstin.length() == 15) {
            pan = l.gstin.mid(2, 10);
        }

        db.executeNonQuery(
            "INSERT OR REPLACE INTO parties (name, mailing_name, group_name, address, state, state_code, pincode, gstin, pan, "
            "bank_name, bank_account, ifsc_code, opening_balance, balance_type, credit_days, credit_limit) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {l.name, l.name, l.parent, l.address, st, stCode, l.pincode, l.gstin, pan,
             l.bankName, l.bankAccount, l.ifsc, l.openingBalance, l.balanceType, l.creditDays, l.creditLimit}
        );
        stats.totalAccounts++;
    }

    // Step 3: Insert stock items into SQLite
    updateProgress(80, "Writing stock items and inventory masters...");
    int itmIdx = 1;
    for (const auto& itm : stockItems) {
        QString codeStr = QString("ITEM-%1").arg(itmIdx++);
        db.executeNonQuery(
            "INSERT OR REPLACE INTO stock_items (name, code, trading_group, hsn_code, unit, gst_rate, opening_qty, opening_rate, opening_value) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {itm.name, codeStr, itm.parent, itm.hsn, itm.unit, itm.gstRate, itm.openingQty, itm.openingRate, itm.openingValue}
        );
        stats.totalStockItems++;
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
