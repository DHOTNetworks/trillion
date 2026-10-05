#include "voucher_migrator.h"
#include "../database_manager.h"
#include "migration_utils.h"
#include <QDebug>
#include <cmath>

namespace MahadevERP {

static std::string getFieldStr(const std::map<std::string, std::string>& m, const std::string& key, const std::string& def = "") {
    auto it = m.find(key);
    if (it != m.end()) return it->second;
    for (const auto& kv : m) {
        if (QString::compare(QString::fromStdString(kv.first), QString::fromStdString(key), Qt::CaseInsensitive) == 0) {
            return kv.second;
        }
    }
    return def;
}

static double parseDoubleVal(const std::string& s, double def = 0.0) {
    std::string clean;
    for (char c : s) {
        if (c != ',' && c != '%' && c != ' ') clean += c;
    }
    if (clean.empty()) return def;
    try {
        return std::stod(clean);
    } catch (...) {
        return def;
    }
}

static int parseInteger(const std::string& s, int def = 0) {
    std::string clean;
    for (char c : s) {
        if (c != ',' && c != ' ') clean += c;
    }
    if (clean.empty()) return def;
    try {
        return std::stoi(clean);
    } catch (...) {
        return def;
    }
}

QString VoucherMigrator::mapVoucherTypeStr(const QString& rawType) {
    QString t = rawType.trimmed().toLower();
    if (t == "sale" || t == "sales") return "Sales";
    if (t == "purc" || t == "purch" || t == "purchase" || t == "purchases") return "Purchase";
    if (t == "jrnl" || t == "journal" || t == "jv") return "Journal";
    if (t == "paym" || t == "payment" || t == "pay") return "Payment";
    if (t == "recp" || t == "receipt" || t == "rcpt") return "Receipt";
    if (t == "cntra" || t == "contra") return "Contra";
    if (t == "dbnt" || t == "debit note" || t == "debitnote") return "Debit Note";
    if (t == "crnt" || t == "credit note" || t == "creditnote") return "Credit Note";
    if (t == "jfrm" || t == "jform" || t == "j-form") return "J-Form";
    if (t == "ifrm" || t == "iform" || t == "i-form") return "I-Form";
    if (t == "mill" || t == "milling") return "Milling";
    if (t == "bank") return "Bank";
    if (t == "tds") return "TDS";
    return rawType.trimmed().isEmpty() ? "Journal" : rawType.trimmed();
}

bool VoucherMigrator::migrate_vouchers_bahikhata(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& transRows,
    const std::vector<std::map<std::string, std::string>>& stockTransRows,
    const std::vector<std::map<std::string, std::string>>& transportRows,
    const std::map<int, std::string>& ledgerCodeToName,
    const std::map<int, qint64>& ledgerCodeToId,
    const std::map<int, std::string>& itemCodeToName,
    const std::map<int, qint64>& itemCodeToId,
    const std::map<int, std::string>& stockUnitMap,
    const QMap<QString, int>& fyNameToId
) {
    Q_UNUSED(itemCodeToName);
    Q_UNUSED(itemCodeToId);
    Q_UNUSED(stockUnitMap);
    auto& db = DatabaseManager::instance();
    ctx.updateProgress(85, QString("Migrating %1 Transactions & Invoices from Bahi-Khata...").arg(transRows.size()));

    // Cache transport details by (vType, vNo, vDate)
    struct TransportInfo {
        std::string lrNo;
        std::string vehicleNo;
        std::string transportName;
        std::string station;
        std::string ewayBill;
        double weight = 0.0;
    };
    std::map<std::string, TransportInfo> transportMap;
    for (const auto& tr : transportRows) {
        std::string vNo = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(tr, "VoucherNumber", getFieldStr(tr, "VNo")))).toStdString();
        std::string rawType = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(tr, "TransType", getFieldStr(tr, "VType")))).toStdString();
        QString vType = mapVoucherTypeStr(QString::fromStdString(rawType));
        QString vDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(tr, "VoucherDate", getFieldStr(tr, "VDate"))));
        std::string key = QString("%1-%2-%3").arg(vType).arg(QString::fromStdString(vNo)).arg(vDate).toStdString();

        TransportInfo info;
        info.lrNo = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(tr, "GRNo", getFieldStr(tr, "LRNo")))).toStdString();
        info.vehicleNo = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(tr, "VehicleNo"))).toStdString();
        info.transportName = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(tr, "TransportName"))).toStdString();
        info.station = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(tr, "Station"))).toStdString();
        info.ewayBill = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(tr, "EWayBillNo", getFieldStr(tr, "EwayBill")))).toStdString();
        info.weight = parseDoubleVal(getFieldStr(tr, "Weight", getFieldStr(tr, "TotalWeight")));
        transportMap[key] = info;
    }

    // Group transactions by (vType, vNo, vDate)
    std::map<std::string, std::vector<std::map<std::string, std::string>>> voucherGroups;
    for (const auto& r : transRows) {
        std::string vNo = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(r, "VoucherNumber"))).toStdString();
        if (vNo.empty()) continue;
        std::string rawType = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(r, "TransType"))).toStdString();
        QString vType = mapVoucherTypeStr(QString::fromStdString(rawType));
        QString vDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(r, "VoucherDate")));
        std::string key = QString("%1-%2-%3").arg(vType).arg(QString::fromStdString(vNo)).arg(vDate).toStdString();
        voucherGroups[key].push_back(r);
    }

    // Cache stock transactions by key
    struct StockLineInfo {
        int itemCode = 0;
        std::string itemName;
        double bags = 0.0;
        double weight = 0.0;
        double rate = 0.0;
        double amount = 0.0;
        double packing = 0.0;
        std::string brand;
    };
    std::map<std::string, std::vector<StockLineInfo>> stockTransMap;
    for (const auto& st : stockTransRows) {
        std::string vNo = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(st, "VoucherNumber", getFieldStr(st, "VNo")))).toStdString();
        std::string rawType = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(st, "TransType", getFieldStr(st, "VType")))).toStdString();
        QString vType = mapVoucherTypeStr(QString::fromStdString(rawType));
        QString vDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(st, "VoucherDate", getFieldStr(st, "VDate"))));
        std::string key = QString("%1-%2-%3").arg(vType).arg(QString::fromStdString(vNo)).arg(vDate).toStdString();

        StockLineInfo sli;
        sli.itemCode = parseInteger(getFieldStr(st, "ItemCode"));
        sli.itemName = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(st, "ItemName"))).toStdString();
        sli.bags = parseDoubleVal(getFieldStr(st, "Bags", getFieldStr(st, "Qty")));
        sli.weight = parseDoubleVal(getFieldStr(st, "Weight", getFieldStr(st, "TotalWeight")));
        sli.rate = parseDoubleVal(getFieldStr(st, "Rate"));
        sli.amount = parseDoubleVal(getFieldStr(st, "Amount"));
        sli.packing = parseDoubleVal(getFieldStr(st, "Packing"));
        sli.brand = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(st, "Brand"))).toStdString();
        stockTransMap[key].push_back(sli);
    }

    double totalDr = 0.0;
    double totalCr = 0.0;
    int salesInvCount = 0;
    int purcInvCount = 0;
    int glTxCount = 0;
    int vchCount = 0;

    for (const auto& kv : voucherGroups) {
        const auto& lines = kv.second;
        if (lines.empty()) continue;

        const auto& first = lines.front();
        std::string vNo = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(first, "VoucherNumber"))).toStdString();
        std::string rawType = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(first, "TransType"))).toStdString();
        QString vType = mapVoucherTypeStr(QString::fromStdString(rawType));
        QString vDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(first, "VoucherDate")));
        QString fyVal = MigrationUtils::resolveFinancialYear(vDate);
        int fyId = fyNameToId.value(fyVal, 1);
        QString formattedVchNo = MigrationUtils::formatFyVoucherNo(vDate, vType, QString::fromStdString(vNo));

        double groupDr = 0.0;
        double groupCr = 0.0;
        std::string masterNarr = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(first, "Narration", getFieldStr(first, "Narrtn")))).toStdString();

        for (const auto& l : lines) {
            std::string drCr = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "DrCr"))).toStdString();
            double amt = parseDoubleVal(getFieldStr(l, "Amount"));
            if (drCr == "Dr" || drCr == "D") {
                groupDr += amt;
            } else {
                groupCr += amt;
            }
        }

        double vchAmt = std::max(groupDr, groupCr);
        bool isBalanced = (std::abs(groupDr - groupCr) < 0.01);

        db.executeNonQuery(
            "INSERT INTO vouchers (voucher_number, voucher_type, voucher_date, financial_year_id, financial_year, narration, amount, is_balanced) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?);",
            {
                formattedVchNo,
                vType,
                vDate,
                fyId,
                fyVal,
                QString::fromStdString(masterNarr),
                vchAmt,
                (isBalanced ? 1 : 0)
            }
        );
        qint64 vchId = db.lastInsertedId();
        vchCount++;

        // Insert double-entry legs into transactions
        for (const auto& l : lines) {
            int acCode = parseInteger(getFieldStr(l, "AccountCode"));
            std::string partyName = ledgerCodeToName.count(acCode) ? ledgerCodeToName.at(acCode) : ("Party #" + std::to_string(acCode));
            qint64 partyId = ledgerCodeToId.count(acCode) ? ledgerCodeToId.at(acCode) : 0;

            std::string drCr = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "DrCr"))).toStdString();
            double amt = parseDoubleVal(getFieldStr(l, "Amount"));
            double drAmt = (drCr == "Dr" || drCr == "D") ? amt : 0.0;
            double crAmt = (drCr == "Cr" || drCr == "C") ? amt : 0.0;

            totalDr += drAmt;
            totalCr += crAmt;

            std::string lineNarr = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "Narration", getFieldStr(l, "Narrtn")))).toStdString();
            if (lineNarr.empty()) lineNarr = masterNarr;

            db.executeNonQuery(
                "INSERT INTO transactions ("
                "voucher_id, party_id, account_name, debit_amount, credit_amount, "
                "narration, transaction_date, financial_year_id, financial_year"
                ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {
                    vchId,
                    partyId,
                    QString::fromStdString(partyName),
                    drAmt,
                    crAmt,
                    QString::fromStdString(lineNarr),
                    vDate,
                    fyId,
                    fyVal
                }
            );
            glTxCount++;
        }

        // Check if this voucher corresponds to a Sales or Purchase Invoice
        if (vType == "Sales" || vType == "Sale") {
            salesInvCount++;
            std::string partyName = "Cash Customer";
            qint64 partyId = 0;
            for (const auto& l : lines) {
                std::string drCr = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "DrCr"))).toStdString();
                if (drCr == "Dr" || drCr == "D") {
                    int acCode = parseInteger(getFieldStr(l, "AccountCode"));
                    if (ledgerCodeToName.count(acCode)) {
                        partyName = ledgerCodeToName.at(acCode);
                        partyId = ledgerCodeToId.at(acCode);
                        break;
                    }
                }
            }

            TransportInfo tInfo;
            if (transportMap.count(kv.first)) {
                tInfo = transportMap[kv.first];
            }

            db.executeNonQuery(
                "INSERT INTO sales_invoices ("
                "invoice_number, invoice_date, customer_id, customer_name, total_amount, "
                "lr_no, vehicle_no, transport_name, station, eway_bill_no, voucher_id, financial_year_id"
                ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {
                    formattedVchNo,
                    vDate,
                    partyId,
                    QString::fromStdString(partyName),
                    vchAmt,
                    QString::fromStdString(tInfo.lrNo),
                    QString::fromStdString(tInfo.vehicleNo),
                    QString::fromStdString(tInfo.transportName),
                    QString::fromStdString(tInfo.station),
                    QString::fromStdString(tInfo.ewayBill),
                    vchId,
                    fyId
                }
            );
            qint64 sinvId = db.lastInsertedId();

            if (stockTransMap.count(kv.first)) {
                for (const auto& sli : stockTransMap[kv.first]) {
                    db.executeNonQuery(
                        "INSERT INTO sales_invoice_items (invoice_id, item_name, quantity, weight, rate, total_amount) "
                        "VALUES (?, ?, ?, ?, ?, ?);",
                        {
                            sinvId,
                            QString::fromStdString(sli.itemName),
                            sli.bags,
                            sli.weight,
                            sli.rate,
                            sli.amount
                        }
                    );
                }
            }
        } else if (vType == "Purchase" || vType == "Purc") {
            purcInvCount++;
            std::string supplierName = "Cash Supplier";
            qint64 supplierId = 0;
            for (const auto& l : lines) {
                std::string drCr = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(l, "DrCr"))).toStdString();
                if (drCr == "Cr" || drCr == "C") {
                    int acCode = parseInteger(getFieldStr(l, "AccountCode"));
                    if (ledgerCodeToName.count(acCode)) {
                        supplierName = ledgerCodeToName.at(acCode);
                        supplierId = ledgerCodeToId.at(acCode);
                        break;
                    }
                }
            }

            db.executeNonQuery(
                "INSERT INTO purchase_invoices ("
                "invoice_number, invoice_date, supplier_id, supplier_name, total_amount, voucher_id, financial_year_id"
                ") VALUES (?, ?, ?, ?, ?, ?, ?);",
                {
                    formattedVchNo,
                    vDate,
                    supplierId,
                    QString::fromStdString(supplierName),
                    vchAmt,
                    vchId,
                    fyId
                }
            );
            qint64 pinvId = db.lastInsertedId();

            if (stockTransMap.count(kv.first)) {
                for (const auto& sli : stockTransMap[kv.first]) {
                    db.executeNonQuery(
                        "INSERT INTO purchase_invoice_items (invoice_id, item_name, quantity, weight, rate, total_amount) "
                        "VALUES (?, ?, ?, ?, ?, ?);",
                        {
                            pinvId,
                            QString::fromStdString(sli.itemName),
                            sli.bags,
                            sli.weight,
                            sli.rate,
                            sli.amount
                        }
                    );
                }
            }
        }
    }

    ctx.stats.totalGlVouchers = vchCount;
    ctx.stats.totalGlTransactions = glTxCount;
    ctx.stats.totalSalesInvoices = salesInvCount;
    ctx.stats.totalPurchaseInvoices = purcInvCount;
    ctx.stats.totalDebitSum = totalDr;
    ctx.stats.totalCreditSum = totalCr;
    ctx.stats.glDiscrepancy = std::abs(totalDr - totalCr);
    ctx.stats.isBalanced = (ctx.stats.glDiscrepancy < 0.01);

    return true;
}

bool VoucherMigrator::migrate_vouchers_busy(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& tran1Rows,
    const std::vector<std::map<std::string, std::string>>& tran2Rows,
    const std::vector<std::map<std::string, std::string>>& tran3Rows,
    const std::map<int, std::string>& accountCodeToName,
    const std::map<int, qint64>& accountCodeToId,
    const std::map<int, std::string>& itemCodeToName,
    const std::map<int, qint64>& itemCodeToId,
    const QMap<QString, int>& fyNameToId
) {
    Q_UNUSED(itemCodeToName);
    Q_UNUSED(itemCodeToId);
    auto& db = DatabaseManager::instance();
    ctx.updateProgress(85, QString("Migrating Double-Entry Vouchers & Invoices from Busy..."));

    // Group Tran2 by VchCode
    std::map<int, std::vector<std::map<std::string, std::string>>> lineGroups;
    for (const auto& l : tran2Rows) {
        int vchCode = parseInteger(getFieldStr(l, "VchCode", getFieldStr(l, "Code")));
        lineGroups[vchCode].push_back(l);
    }

    // Group Tran3 by VchCode
    std::map<int, std::vector<std::map<std::string, std::string>>> itemGroups;
    for (const auto& it : tran3Rows) {
        int vchCode = parseInteger(getFieldStr(it, "VchCode", getFieldStr(it, "Code")));
        itemGroups[vchCode].push_back(it);
    }

    double totalDr = 0.0;
    double totalCr = 0.0;
    int vchCount = 0;
    int glTxCount = 0;
    int salesCount = 0;
    int purcCount = 0;

    for (const auto& h : tran1Rows) {
        int vchCode = parseInteger(getFieldStr(h, "VchCode", getFieldStr(h, "Code")));
        std::string rawVchNo = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(h, "VchNo", getFieldStr(h, "DocNo")))).toStdString();
        if (rawVchNo.empty()) rawVchNo = std::to_string(vchCode);

        std::string rawType = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(h, "VchType", getFieldStr(h, "Type")))).toStdString();
        QString vType = mapVoucherTypeStr(QString::fromStdString(rawType));
        QString vDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(h, "Date", getFieldStr(h, "VchDate"))));
        QString fyVal = MigrationUtils::resolveFinancialYear(vDate);
        int fyId = fyNameToId.value(fyVal, 1);
        QString formattedVchNo = MigrationUtils::formatFyVoucherNo(vDate, vType, QString::fromStdString(rawVchNo));

        std::string narr = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(h, "Narration", getFieldStr(h, "Narr1")))).toStdString();
        double masterAmt = parseDoubleVal(getFieldStr(h, "TotalAmt", getFieldStr(h, "Amount")));

        db.executeNonQuery(
            "INSERT INTO vouchers (voucher_number, voucher_type, voucher_date, financial_year_id, financial_year, narration, amount, is_balanced) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, 1);",
            {
                formattedVchNo,
                vType,
                vDate,
                fyId,
                fyVal,
                QString::fromStdString(narr),
                masterAmt
            }
        );
        qint64 vchId = db.lastInsertedId();
        vchCount++;

        // Process lines in Tran2
        if (lineGroups.count(vchCode)) {
            for (const auto& l : lineGroups[vchCode]) {
                int acCode = parseInteger(getFieldStr(l, "MasterCode1", getFieldStr(l, "AccountCode")));
                std::string partyName = accountCodeToName.count(acCode) ? accountCodeToName.at(acCode) : ("Account #" + std::to_string(acCode));
                qint64 partyId = accountCodeToId.count(acCode) ? accountCodeToId.at(acCode) : 0;

                double amt = parseDoubleVal(getFieldStr(l, "Amount", getFieldStr(l, "Value")));
                int dType = parseInteger(getFieldStr(l, "DType", "1"));
                double drAmt = (dType == 1 ? amt : 0.0);
                double crAmt = (dType == 2 ? amt : 0.0);

                totalDr += drAmt;
                totalCr += crAmt;

                db.executeNonQuery(
                    "INSERT INTO transactions ("
                    "voucher_id, party_id, account_name, debit_amount, credit_amount, "
                    "narration, transaction_date, financial_year_id, financial_year"
                    ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);",
                    {
                        vchId,
                        partyId,
                        QString::fromStdString(partyName),
                        drAmt,
                        crAmt,
                        QString::fromStdString(narr),
                        vDate,
                        fyId,
                        fyVal
                    }
                );
                glTxCount++;
            }
        }

        if (vType == "Sales" || vType == "Sale") {
            salesCount++;
            db.executeNonQuery(
                "INSERT INTO sales_invoices (invoice_number, invoice_date, total_amount, voucher_id, financial_year_id) "
                "VALUES (?, ?, ?, ?, ?);",
                {formattedVchNo, vDate, masterAmt, vchId, fyId}
            );
        } else if (vType == "Purchase" || vType == "Purc") {
            purcCount++;
            db.executeNonQuery(
                "INSERT INTO purchase_invoices (invoice_number, invoice_date, total_amount, voucher_id, financial_year_id) "
                "VALUES (?, ?, ?, ?, ?);",
                {formattedVchNo, vDate, masterAmt, vchId, fyId}
            );
        }
    }

    ctx.stats.totalGlVouchers = vchCount;
    ctx.stats.totalGlTransactions = glTxCount;
    ctx.stats.totalSalesInvoices = salesCount;
    ctx.stats.totalPurchaseInvoices = purcCount;
    ctx.stats.totalDebitSum = totalDr;
    ctx.stats.totalCreditSum = totalCr;
    ctx.stats.glDiscrepancy = std::abs(totalDr - totalCr);
    ctx.stats.isBalanced = (ctx.stats.glDiscrepancy < 0.01);

    return true;
}

bool VoucherMigrator::migrate_vouchers_tally(
    MigrationContext& ctx,
    const QList<QVariantMap>& voucherRecords,
    const std::map<std::string, qint64>& ledgerNameToId,
    const std::map<std::string, qint64>& itemNameToId,
    const QMap<QString, int>& fyNameToId
) {
    Q_UNUSED(itemNameToId);
    auto& db = DatabaseManager::instance();
    ctx.updateProgress(90, QString("Writing %1 Vouchers from Tally...").arg(voucherRecords.size()));

    double totalDr = 0.0;
    double totalCr = 0.0;
    int vchCount = 0;
    int glTxCount = 0;
    int salesCount = 0;
    int purcCount = 0;

    for (const auto& v : voucherRecords) {
        QString rawVchNo = MigrationUtils::cleanText(v.value("voucher_number").toString());
        QString rawType = MigrationUtils::cleanText(v.value("voucher_type").toString());
        QString vType = mapVoucherTypeStr(rawType);
        QString vDate = MigrationUtils::parseNormalizedDate(v.value("voucher_date").toString());
        QString fyVal = MigrationUtils::resolveFinancialYear(vDate);
        int fyId = fyNameToId.value(fyVal, 1);
        QString formattedVchNo = MigrationUtils::formatFyVoucherNo(vDate, vType, rawVchNo);

        QString narr = v.value("narration").toString();
        double amount = v.value("amount", 0.0).toDouble();

        db.executeNonQuery(
            "INSERT INTO vouchers (voucher_number, voucher_type, voucher_date, financial_year_id, financial_year, narration, amount, is_balanced) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, 1);",
            {
                formattedVchNo,
                vType,
                vDate,
                fyId,
                fyVal,
                narr,
                amount
            }
        );
        qint64 vchId = db.lastInsertedId();
        vchCount++;

        QVariantList ledgers = v.value("ledger_entries").toList();
        for (const auto& lVar : ledgers) {
            QVariantMap l = lVar.toMap();
            QString partyName = MigrationUtils::cleanText(l.value("ledger_name").toString());
            qint64 partyId = ledgerNameToId.count(partyName.toStdString()) ? ledgerNameToId.at(partyName.toStdString()) : 0;
            double amt = l.value("amount", 0.0).toDouble();
            double drAmt = 0.0;
            double crAmt = 0.0;

            if (amt < 0) {
                drAmt = -amt;
            } else {
                crAmt = amt;
            }

            totalDr += drAmt;
            totalCr += crAmt;

            db.executeNonQuery(
                "INSERT INTO transactions ("
                "voucher_id, party_id, account_name, debit_amount, credit_amount, "
                "narration, transaction_date, financial_year_id, financial_year"
                ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {
                    vchId,
                    partyId,
                    partyName,
                    drAmt,
                    crAmt,
                    narr,
                    vDate,
                    fyId,
                    fyVal
                }
            );
            glTxCount++;
        }

        if (vType == "Sales") {
            salesCount++;
            db.executeNonQuery(
                "INSERT INTO sales_invoices (invoice_number, invoice_date, total_amount, voucher_id, financial_year_id) "
                "VALUES (?, ?, ?, ?, ?);",
                {formattedVchNo, vDate, amount, vchId, fyId}
            );
        } else if (vType == "Purchase") {
            purcCount++;
            db.executeNonQuery(
                "INSERT INTO purchase_invoices (invoice_number, invoice_date, total_amount, voucher_id, financial_year_id) "
                "VALUES (?, ?, ?, ?, ?);",
                {formattedVchNo, vDate, amount, vchId, fyId}
            );
        }
    }

    ctx.stats.totalGlVouchers = vchCount;
    ctx.stats.totalGlTransactions = glTxCount;
    ctx.stats.totalSalesInvoices = salesCount;
    ctx.stats.totalPurchaseInvoices = purcCount;
    ctx.stats.totalDebitSum = totalDr;
    ctx.stats.totalCreditSum = totalCr;
    ctx.stats.glDiscrepancy = std::abs(totalDr - totalCr);
    ctx.stats.isBalanced = (ctx.stats.glDiscrepancy < 0.01);

    return true;
}

} // namespace MahadevERP
