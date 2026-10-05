#include "mandi_migrator.h"
#include "../database_manager.h"
#include "migration_utils.h"
#include <QDebug>
#include <cmath>
#include <algorithm>

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

static std::string toLowerStr(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return std::tolower(c); });
    return out;
}

bool MandiMigrator::migrate_mandi_bahikhata(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& transRows,
    const std::vector<std::map<std::string, std::string>>& millingRows,
    const std::vector<std::map<std::string, std::string>>& customClosingRows,
    const std::vector<std::map<std::string, std::string>>& tdsRows,
    const std::vector<std::map<std::string, std::string>>& bardanaRows,
    const std::vector<std::map<std::string, std::string>>& gatePassRows,
    const std::vector<std::map<std::string, std::string>>& gateRegRows,
    const std::vector<std::map<std::string, std::string>>& saudaRows,
    const std::vector<std::map<std::string, std::string>>& saudaTxRows,
    const std::vector<std::map<std::string, std::string>>& brokerageRows,
    const std::map<int, std::string>& ledgerCodeToName,
    const std::map<int, qint64>& ledgerCodeToId,
    const std::map<int, std::string>& itemCodeToName,
    const std::map<int, qint64>& itemCodeToId,
    const std::map<int, std::string>& stockUnitMap,
    const QMap<QString, int>& fyNameToId
) {
    Q_UNUSED(gatePassRows);
    Q_UNUSED(saudaTxRows);
    Q_UNUSED(itemCodeToId);
    Q_UNUSED(stockUnitMap);

    auto& db = DatabaseManager::instance();
    ctx.updateProgress(90, QString("Migrating Milling Batches, Mandi Forms & Agri Operations..."));

    // 1. Milling Production Batches & Line Items
    std::map<std::pair<std::string, std::string>, std::vector<std::map<std::string, std::string>>> millingGroups;
    for (const auto& mr : millingRows) {
        std::string mvNo = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(mr, "VoucherNumber"))).toStdString();
        std::string mvDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(mr, "VoucherDate"))).toStdString();
        millingGroups[{mvNo, mvDate}].push_back(mr);
    }

    int batchCount = 0;
    for (const auto& pair : millingGroups) {
        std::string mvNo = pair.first.first;
        std::string mvDate = pair.first.second;
        const auto& mRows = pair.second;

        QString batchNoStr = QString("Mill-%1").arg(QString::fromStdString(mvNo));
        QString fyVal = MigrationUtils::resolveFinancialYear(QString::fromStdString(mvDate));
        int fyId = fyNameToId.value(fyVal, 1);

        double paddyIn = 0.0;
        std::string paddyVariety = "Paddy Basmati";
        double headRice = 0.0;
        double brokenRice = 0.0;
        double bran = 0.0;
        double husk = 0.0;
        std::string batchNarration;

        for (const auto& r : mRows) {
            int iCode = parseInteger(getFieldStr(r, "ItemCode"));
            std::string iName = itemCodeToName.count(iCode) ? itemCodeToName.at(iCode) : MigrationUtils::cleanText(QString::fromStdString(getFieldStr(r, "ItemName"))).toStdString();
            if (iName.empty()) iName = "Item #" + std::to_string(iCode);

            std::string iLower = toLowerStr(iName);
            double wt = parseDoubleVal(getFieldStr(r, "Weight"));
            std::string drcr = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(r, "DrCr"))).toStdString();
            std::string narr = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(r, "Narrtn"))).toStdString();
            if (!narr.empty() && batchNarration.empty()) batchNarration = narr;

            if (drcr == "Cr") {
                paddyIn += wt;
                paddyVariety = iName;
            } else {
                if (iLower.find("bran") != std::string::npos && iLower.find("brand") == std::string::npos) {
                    bran += wt;
                } else if (iLower.find("broken") != std::string::npos || iLower.find("nakku") != std::string::npos || iLower.find("tibar") != std::string::npos || iLower.find("dubar") != std::string::npos || iLower.find("mogra") != std::string::npos || iLower.find("kinki") != std::string::npos) {
                    brokenRice += wt;
                } else if (iLower.find("husk") != std::string::npos || iLower.find("phak") != std::string::npos || iLower.find("bhusa") != std::string::npos) {
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
            "batch_number, start_date, end_date, paddy_input_qty, output_head_rice_qty, "
            "output_broken_rice_qty, output_bran_qty, output_husk_qty, wastage_qty, "
            "outturn_ratio_percent, status, notes, financial_year_id"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'COMPLETED', ?, ?);",
            {
                batchNoStr,
                QString::fromStdString(mvDate),
                QString::fromStdString(mvDate),
                paddyIn,
                headRice,
                brokenRice,
                bran,
                husk,
                wastage,
                yieldPct,
                QString::fromStdString(batchNarration),
                fyId
            }
        );
        qint64 batchId = db.lastInsertedId();
        batchCount++;

        for (const auto& r : mRows) {
            int iCode = parseInteger(getFieldStr(r, "ItemCode"));
            std::string iName = itemCodeToName.count(iCode) ? itemCodeToName.at(iCode) : MigrationUtils::cleanText(QString::fromStdString(getFieldStr(r, "ItemName"))).toStdString();
            double bags = parseDoubleVal(getFieldStr(r, "Bags"));
            double wt = parseDoubleVal(getFieldStr(r, "Weight"));
            double rate = parseDoubleVal(getFieldStr(r, "Rate"));
            double amt = parseDoubleVal(getFieldStr(r, "Amount"));
            std::string drcr = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(r, "DrCr"))).toStdString();
            std::string itemType = (drcr == "Cr") ? "CONSUMED" : "PRODUCED";

            db.executeNonQuery(
                "INSERT INTO milling_voucher_items (batch_id, item_name, item_type, bags, weight_qtl, rate, amount) "
                "VALUES (?, ?, ?, ?, ?, ?, ?);",
                {batchId, QString::fromStdString(iName), QString::fromStdString(itemType), bags, wt, rate, amt}
            );
        }
    }
    ctx.stats.totalMillingBatches = batchCount;

    // 2. J-Form & I-Form Procurement
    int jCount = 0;
    int iCount = 0;
    for (const auto& r : transRows) {
        std::string rawType = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(r, "TransType"))).toStdString();
        if (rawType == "JFrm") {
            std::string vNo = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(r, "VoucherNumber"))).toStdString();
            std::string vDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(r, "VoucherDate"))).toStdString();
            QString fyVal = MigrationUtils::resolveFinancialYear(QString::fromStdString(vDate));
            int fyId = fyNameToId.value(fyVal, 1);

            int acCode = parseInteger(getFieldStr(r, "AccountCode"));
            std::string partyName = ledgerCodeToName.count(acCode) ? ledgerCodeToName.at(acCode) : ("Farmer #" + std::to_string(acCode));
            qint64 partyId = ledgerCodeToId.count(acCode) ? ledgerCodeToId.at(acCode) : 0;
            double amt = parseDoubleVal(getFieldStr(r, "Amount"));

            db.executeNonQuery(
                "INSERT INTO jform_vouchers ("
                "jform_no, jform_date, farmer_id, farmer_name, total_amount, financial_year_id"
                ") VALUES (?, ?, ?, ?, ?, ?);",
                {
                    QString::fromStdString(vNo),
                    QString::fromStdString(vDate),
                    partyId,
                    QString::fromStdString(partyName),
                    amt,
                    fyId
                }
            );
            jCount++;
        } else if (rawType == "IFrm") {
            std::string vNo = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(r, "VoucherNumber"))).toStdString();
            std::string vDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(r, "VoucherDate"))).toStdString();
            int acCode = parseInteger(getFieldStr(r, "AccountCode"));
            std::string partyName = ledgerCodeToName.count(acCode) ? ledgerCodeToName.at(acCode) : "";
            double amt = parseDoubleVal(getFieldStr(r, "Amount"));

            db.executeNonQuery(
                "INSERT INTO transport_dispatches ("
                "dispatch_no, dispatch_date, consignee_name, total_freight"
                ") VALUES (?, ?, ?, ?);",
                {
                    QString::fromStdString(vNo),
                    QString::fromStdString(vDate),
                    QString::fromStdString(partyName),
                    amt
                }
            );
            iCount++;
        }
    }
    ctx.stats.totalJForms = jCount;
    ctx.stats.totalIForms = iCount;

    // 3. TDS Vouchers
    int tdsCount = 0;
    for (const auto& t : tdsRows) {
        std::string certNo = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(t, "CertNo", getFieldStr(t, "TDSNo")))).toStdString();
        std::string tDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(t, "Date", getFieldStr(t, "VoucherDate")))).toStdString();
        std::string section = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(t, "Section", "194Q"))).toStdString();
        double gross = parseDoubleVal(getFieldStr(t, "GrossAmount", getFieldStr(t, "Amount")));
        double rate = parseDoubleVal(getFieldStr(t, "TDSRate", getFieldStr(t, "Rate")));
        double tdsAmt = parseDoubleVal(getFieldStr(t, "TDSAmount", getFieldStr(t, "TDS")));
        int acCode = parseInteger(getFieldStr(t, "AccountCode"));
        std::string partyName = ledgerCodeToName.count(acCode) ? ledgerCodeToName.at(acCode) : "";
        qint64 partyId = ledgerCodeToId.count(acCode) ? ledgerCodeToId.at(acCode) : 0;

        db.executeNonQuery(
            "INSERT INTO tds_vouchers ("
            "certificate_no, voucher_date, section_code, party_id, party_name, "
            "gross_amount, tds_rate, tds_amount"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?);",
            {
                QString::fromStdString(certNo),
                QString::fromStdString(tDate),
                QString::fromStdString(section),
                partyId,
                QString::fromStdString(partyName),
                gross,
                rate,
                tdsAmt
            }
        );
        tdsCount++;
    }
    ctx.stats.totalTdsVouchers = tdsCount;

    // 4. Custom Closing Stocks
    for (const auto& c : customClosingRows) {
        std::string iName = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(c, "ItemName"))).toStdString();
        double qty = parseDoubleVal(getFieldStr(c, "ClosingQty", getFieldStr(c, "Qty")));
        double rate = parseDoubleVal(getFieldStr(c, "Rate", getFieldStr(c, "ClosingRate")));
        double val = parseDoubleVal(getFieldStr(c, "Value", getFieldStr(c, "ClosingValue")));
        std::string cDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(c, "Date", getFieldStr(c, "ClosingDate")))).toStdString();

        db.executeNonQuery(
            "INSERT INTO custom_closing_stocks (item_name, closing_date, quantity, rate, valuation) "
            "VALUES (?, ?, ?, ?, ?);",
            {QString::fromStdString(iName), QString::fromStdString(cDate), qty, rate, val}
        );
    }

    // 5. Bardana Transactions
    for (const auto& b : bardanaRows) {
        std::string bDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(b, "Date", getFieldStr(b, "VoucherDate")))).toStdString();
        std::string bType = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(b, "BagType", getFieldStr(b, "BardanaType")))).toStdString();
        double qtyIn = parseDoubleVal(getFieldStr(b, "QtyIn", getFieldStr(b, "InBags")));
        double qtyOut = parseDoubleVal(getFieldStr(b, "QtyOut", getFieldStr(b, "OutBags")));
        double rate = parseDoubleVal(getFieldStr(b, "Rate"));
        int acCode = parseInteger(getFieldStr(b, "AccountCode"));
        std::string partyName = ledgerCodeToName.count(acCode) ? ledgerCodeToName.at(acCode) : "";

        db.executeNonQuery(
            "INSERT INTO bardana_transactions (transaction_date, bag_type, party_name, bags_in, bags_out, rate) "
            "VALUES (?, ?, ?, ?, ?, ?);",
            {QString::fromStdString(bDate), QString::fromStdString(bType), QString::fromStdString(partyName), qtyIn, qtyOut, rate}
        );
    }

    // 6. Gate Register
    for (const auto& gr : gateRegRows) {
        std::string gDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(gr, "Date", getFieldStr(gr, "GateDate")))).toStdString();
        std::string vehicleNo = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(gr, "VehicleNo"))).toStdString();
        std::string party = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(gr, "PartyName"))).toStdString();
        std::string item = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(gr, "ItemName"))).toStdString();
        double gross = parseDoubleVal(getFieldStr(gr, "GrossWeight", getFieldStr(gr, "Gross")));
        double tare = parseDoubleVal(getFieldStr(gr, "TareWeight", getFieldStr(gr, "Tare")));
        double net = parseDoubleVal(getFieldStr(gr, "NetWeight", getFieldStr(gr, "Net")));

        db.executeNonQuery(
            "INSERT INTO gate_register (gate_date, vehicle_no, party_name, item_name, gross_weight, tare_weight, net_weight) "
            "VALUES (?, ?, ?, ?, ?, ?, ?);",
            {QString::fromStdString(gDate), QString::fromStdString(vehicleNo), QString::fromStdString(party), QString::fromStdString(item), gross, tare, net}
        );
    }

    // 7. Sauda Forward Contracts
    for (const auto& s : saudaRows) {
        std::string sNo = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(s, "SaudaNo", getFieldStr(s, "ContractNo")))).toStdString();
        std::string sDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(s, "Date", getFieldStr(s, "SaudaDate")))).toStdString();
        std::string party = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(s, "PartyName"))).toStdString();
        std::string item = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(s, "ItemName"))).toStdString();
        double qty = parseDoubleVal(getFieldStr(s, "Quantity", getFieldStr(s, "Weight")));
        double rate = parseDoubleVal(getFieldStr(s, "Rate", getFieldStr(s, "SaudaRate")));
        std::string broker = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(s, "BrokerName"))).toStdString();

        db.executeNonQuery(
            "INSERT INTO sauda_contracts (contract_no, contract_date, party_name, commodity, quantity_qtl, contract_rate, broker_name) "
            "VALUES (?, ?, ?, ?, ?, ?, ?);",
            {QString::fromStdString(sNo), QString::fromStdString(sDate), QString::fromStdString(party), QString::fromStdString(item), qty, rate, QString::fromStdString(broker)}
        );
    }

    // 8. Brokerage / Dalali Settlements
    for (const auto& br : brokerageRows) {
        std::string bDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(br, "Date", getFieldStr(br, "VoucherDate")))).toStdString();
        std::string broker = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(br, "BrokerName"))).toStdString();
        double amt = parseDoubleVal(getFieldStr(br, "Amount", getFieldStr(br, "BrokerageAmount")));
        std::string narr = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(br, "Narration"))).toStdString();

        db.executeNonQuery(
            "INSERT INTO dalali_settlements (settlement_date, broker_name, amount, narration) "
            "VALUES (?, ?, ?, ?);",
            {QString::fromStdString(bDate), QString::fromStdString(broker), amt, QString::fromStdString(narr)}
        );
    }

    ctx.stats.totalMandiRecords = jCount + iCount + batchCount + tdsCount;
    return true;
}

bool MandiMigrator::migrate_mandi_busy(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& mandiRows,
    const std::map<int, std::string>& accountCodeToName,
    const std::map<int, qint64>& accountCodeToId,
    const QMap<QString, int>& fyNameToId
) {
    auto& db = DatabaseManager::instance();
    ctx.updateProgress(95, QString("Migrating Busy Mandi & Procurement Records..."));

    int mCount = 0;
    for (const auto& r : mandiRows) {
        std::string formNo = MigrationUtils::cleanText(QString::fromStdString(getFieldStr(r, "FormNo", getFieldStr(r, "VchNo")))).toStdString();
        std::string fDate = MigrationUtils::parseNormalizedDate(QString::fromStdString(getFieldStr(r, "Date", getFieldStr(r, "VchDate")))).toStdString();
        QString fyVal = MigrationUtils::resolveFinancialYear(QString::fromStdString(fDate));
        int fyId = fyNameToId.value(fyVal, 1);

        int acCode = parseInteger(getFieldStr(r, "AccountCode", getFieldStr(r, "FarmerCode")));
        std::string partyName = accountCodeToName.count(acCode) ? accountCodeToName.at(acCode) : ("Farmer #" + std::to_string(acCode));
        qint64 partyId = accountCodeToId.count(acCode) ? accountCodeToId.at(acCode) : 0;
        double amt = parseDoubleVal(getFieldStr(r, "Amount", getFieldStr(r, "TotalAmt")));

        db.executeNonQuery(
            "INSERT INTO jform_vouchers (jform_no, jform_date, farmer_id, farmer_name, total_amount, financial_year_id) "
            "VALUES (?, ?, ?, ?, ?, ?);",
            {
                QString::fromStdString(formNo),
                QString::fromStdString(fDate),
                partyId,
                QString::fromStdString(partyName),
                amt,
                fyId
            }
        );
        mCount++;
    }

    ctx.stats.totalMandiRecords = mCount;
    return true;
}

} // namespace MahadevERP
