#include "voucher_migrator.h"
#include "../database_manager.h"
#include "../data_preprocessor.h"
#include "../fiscal_year_helper.h"
#include "migration_columns.h"
#include "migration_utils.h"
#include <QDate>
#include <QDebug>
#include <algorithm>
#include <cmath>
#include <tuple>
#include <utility>

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

// Ordered-fallback read for pipe-compound JSON sources
// ("InvoiceNo|TaxInvoiceNo"): first non-empty (post-clean) wins — mirrors
// the monolith's explicit if-empty chains exactly (clean-then-check per
// candidate, so "null"/"none" values fall through the same way).
static std::string getFieldFirstClean(
    const std::map<std::string, std::string>& m, const std::string& pipeKeys, const std::string& def = "") {
    size_t start = 0;
    while (start <= pipeKeys.size()) {
        const size_t bar = pipeKeys.find('|', start);
        std::string key = (bar == std::string::npos) ? pipeKeys.substr(start) : pipeKeys.substr(start, bar - start);
        const size_t f = key.find_first_not_of(" \t");
        const size_t l = key.find_last_not_of(" \t");
        if (f != std::string::npos) {
            std::string v = DataPreprocessor::cleanText(getFieldStr(m, key.substr(f, l - f + 1)));
            if (!v.empty()) return v;
        }
        if (bar == std::string::npos) break;
        start = bar + 1;
    }
    return def;
}

// Verbatim monolith helpers (kept file-local for parity: DataPreprocessor's
// parseDateFormatted lacks the 1900-1970 century window, so the transplant
// carries the exact Jet-date body instead of the near-miss mirror).
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

static QString canonVoucherNo(const QString& fyVal, const std::string& rawTransType, const std::string& rawNo) {
    QString tok = FiscalYearHelper::canonicalTypeToken(QString::fromStdString(rawTransType));
    if (tok == "JFrm" || tok == "IFrm") return QString::fromStdString(DataPreprocessor::cleanText(rawNo));
    return FiscalYearHelper::canonicalVoucherNo(fyVal, tok, QString::fromStdString(rawNo));
}

static std::string lookupLedgerName(
    int acCode,
    const std::map<int, PartyDetail>& ledgerDetailMap,
    const std::map<int, std::string>& ledgerCodeMap,
    const std::map<int, std::string>& ledgerDeletedNameMap
) {
    auto it = ledgerDetailMap.find(acCode);
    if (it != ledgerDetailMap.end() && !it->second.name.empty()) return it->second.name;
    auto jt = ledgerCodeMap.find(acCode);
    if (jt != ledgerCodeMap.end() && !jt->second.empty()) return jt->second;
    auto kt = ledgerDeletedNameMap.find(acCode);
    if (kt != ledgerDeletedNameMap.end() && !kt->second.empty()) return kt->second;
    return "";
}

static ResolvedVoucherParty resolveVoucherParty(
    const QString& vType,
    const std::vector<std::map<std::string, std::string>>& rows,
    const std::map<int, PartyDetail>& ledgerDetailMap,
    const std::map<int, std::string>& ledgerCodeMap,
    const std::map<int, std::string>& ledgerDeletedNameMap
) {
    ResolvedVoucherParty res;
    std::string bankOrCash;
    int bankOrCashId = 0;
    std::string tradingOrExp;
    int tradingOrExpId = 0;

    for (const auto& r : rows) {
        int acCode = DataPreprocessor::parseInt(getFieldStr(r, "AccountCode"));
        auto it = ledgerDetailMap.find(acCode);
        std::string lName = lookupLedgerName(acCode, ledgerDetailMap, ledgerCodeMap, ledgerDeletedNameMap);
        if (lName.empty()) lName = "Party #" + std::to_string(acCode);
        int lId = (it != ledgerDetailMap.end()) ? it->second.id : 0;
        std::string grpName = (it != ledgerDetailMap.end()) ? it->second.group_name : "Sundry Debtors";
        std::string gLower = DataPreprocessor::toLowerStr(grpName);

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

        std::string lLower = DataPreprocessor::toLowerStr(lName);
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
                if (it != ledgerDetailMap.end()) res.primary_party_gstin = it->second.gstin;
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
            int firstCode = DataPreprocessor::parseInt(getFieldStr(rows[0], "AccountCode"));
            res.primary_party = lookupLedgerName(firstCode, ledgerDetailMap, ledgerCodeMap, ledgerDeletedNameMap);
            if (res.primary_party.empty()) res.primary_party = "Trade Account";
            auto itDetail = ledgerDetailMap.find(firstCode);
            res.primary_party_id = (itDetail != ledgerDetailMap.end()) ? itDetail->second.id : 0;
            res.primary_party_legacy_code = firstCode;
            if (itDetail != ledgerDetailMap.end()) res.primary_party_gstin = itDetail->second.gstin;
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
    // Keeper-based: alias normalization lives in FiscalYearHelper::canonicalTypeToken.
    // Display mapping below matches the proven BahiKhataMigrator::mapVoucherTypeStr
    // (src/engine/bahi_khata_migrator.cpp) that produced the current migrated DB
    // (vouchers: Payment/Purchase/Sales/Receipt/Journal/...) — verified against
    // live Data.002 TransType values {ChPt,ChRt,Jrnl,PrRn,Purc,Pymt,Rcpt,Sale,SlRn}.
    // Cash/bank families merge for STORAGE only; voucher numbering keeps raw
    // tokens distinct via canonVoucherNo/canonicalVoucherNo — never merge there.
    QString tok = FiscalYearHelper::canonicalTypeToken(rawType);
    if (tok == "Sale") return "Sales";
    if (tok == "Purc") return "Purchase";
    if (tok == "Pymt" || tok == "ChPt") return "Payment";
    if (tok == "Rcpt" || tok == "ChRt") return "Receipt";
    if (tok == "Jrnl") return "Journal";
    if (tok == "Contra") return "Contra";
    if (tok == "JFrm") return "J-Form";
    if (tok == "IFrm") return "I-Form";
    if (tok == "Mill") return "Manufacturing";
    if (tok == "Manu") return "Manufacturing";
    if (tok == "Bank") return "Bank";
    if (tok == "TDS") return "TDS";
    if (tok == "TCS") return "TCS";
    if (tok == "DbNt") return "Debit Note";
    if (tok == "CrNt") return "Credit Note";
    return rawType.trimmed().isEmpty() ? "Journal" : rawType.trimmed();
}

bool VoucherMigrator::migrate_movements_bahikhata(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& stockTransRows,
    const std::vector<std::map<std::string, std::string>>& transportRows,
    const std::map<int, ItemDetail>& itemCodeMap,
    std::map<std::string, std::vector<StockMovementEntry>>& outStockTransMap,
    std::map<std::string, LogisticsEntry>& outTransportMap
) {
    // C3 strangler slice V (monolith PASS 4: stock movements + logistics
    // caching): transplanted verbatim. Source keys go through
    // MigrationColumns where the StockTransactions JSON entry covers them
    // (STF); tax-split / grade-chain / logistics reads keep documented
    // JSON-gap literals — the SaleTransportationDetail JSON entry targets
    // the mandi-slice transport_dispatches table write, not this cache.
    auto STF = [](const char* t) { return MigrationColumns::stockTransField(QString::fromLatin1(t)).toStdString(); };
    ctx.updateProgress(50, "Caching Stock Movements & Logistics...");
    outStockTransMap.clear();
    outTransportMap.clear();

    for (const auto& st : stockTransRows) {
        std::string vNo = DataPreprocessor::cleanText(getFieldStr(st, STF("voucher_no")));
        QString vType = VoucherMigrator::mapVoucherTypeStr(QString::fromStdString(getFieldStr(st, STF("trans_type"))));
        QString vDate = parseDateFormatted(QString::fromStdString(getFieldStr(st, STF("voucher_date"))));
        std::string key = QString("%1-%2-%3").arg(vType).arg(QString::fromStdString(vNo)).arg(vDate).toStdString();

        int iCode = DataPreprocessor::parseInt(getFieldStr(st, STF("item_code")));
        auto itItem = itemCodeMap.find(iCode);
        int itemId = (itItem != itemCodeMap.end()) ? itItem->second.id : 1;
        std::string itemName = (itItem != itemCodeMap.end()) ? itItem->second.name : "Commodity";
        std::string hsn = (itItem != itemCodeMap.end()) ? itItem->second.hsn : "";
        double defaultGst = (itItem != itemCodeMap.end()) ? itItem->second.gst_rate : 5.0;

        int bags = DataPreprocessor::parseInt(getFieldStr(st, STF("bags")));
        double packing = DataPreprocessor::parseDouble(getFieldStr(st, STF("packing")), 0.500);
        double wt = DataPreprocessor::parseDouble(getFieldStr(st, STF("weight_qtl")));
        double rate = DataPreprocessor::parseDouble(getFieldStr(st, STF("rate")));
        double amt = DataPreprocessor::parseDouble(getFieldStr(st, STF("amount")));
        double taxable = DataPreprocessor::parseDouble(getFieldStr(st, STF("taxable_amount")));
        if (taxable <= 0.0) taxable = amt;

        // JSON-gap(STF): tax-split + grade fields absent from the
        // StockTransactions JSON entry — literals preserved verbatim.
        double vatcst = DataPreprocessor::parseDouble(getFieldStr(st, "VATCST"));
        double gstPct = (vatcst > 0.0) ? vatcst : defaultGst;
        double cgst = DataPreprocessor::parseDouble(getFieldStr(st, "CGSTTax"));
        double sgst = DataPreprocessor::parseDouble(getFieldStr(st, "SGSTTax"));
        double igst = DataPreprocessor::parseDouble(getFieldStr(st, "IGSTTax"));
        double cess = DataPreprocessor::parseDouble(getFieldStr(st, "CESSTax"));
        std::string dheriBill = DataPreprocessor::cleanText(getFieldStr(st, "DheriBillNo"));
        std::string itemGrade = dheriBill;
        if (itemGrade.empty()) itemGrade = DataPreprocessor::cleanText(getFieldStr(st, "Grade"));
        if (itemGrade.empty()) itemGrade = DataPreprocessor::cleanText(getFieldStr(st, "Quality"));
        if (itemGrade.empty()) itemGrade = DataPreprocessor::cleanText(getFieldStr(st, "ItemQuality"));
        if (itemGrade.empty()) itemGrade = DataPreprocessor::cleanText(getFieldStr(st, "Variety"));
        std::string vchTypeStr = DataPreprocessor::cleanText(getFieldStr(st, STF("voucher_type")));

        StockMovementEntry entry;
        entry.item_id = itemId;
        entry.item_name = itemName;
        entry.grade = itemGrade;
        entry.hsn_code = hsn;
        entry.bags = bags;
        entry.packing = packing;
        entry.weight_qtl = wt;
        entry.rate = rate;
        entry.amount = amt;
        entry.taxable_amount = taxable;
        entry.gst_pct = gstPct;
        entry.cgst = cgst;
        entry.sgst = sgst;
        entry.igst = igst;
        entry.cess = cess;
        entry.dheri_bill_no = dheriBill;
        entry.voucher_type = vchTypeStr;

        outStockTransMap[key].push_back(entry);
    }

    for (const auto& tr : transportRows) {
        // JSON-gap(transport): the SaleTransportationDetail JSON entry maps
        // the mandi-slice transport_dispatches write, not this cache —
        // every read below stays a verbatim literal.
        std::string vNo = DataPreprocessor::cleanText(getFieldStr(tr, "VoucherNumber"));
        QString vType = VoucherMigrator::mapVoucherTypeStr(QString::fromStdString(getFieldStr(tr, "TransType")));
        QString vDate = parseDateFormatted(QString::fromStdString(getFieldStr(tr, "VoucherDate")));
        std::string key = QString("%1-%2-%3").arg(vType).arg(QString::fromStdString(vNo)).arg(vDate).toStdString();

        LogisticsEntry log;
        log.vehicle_no = DataPreprocessor::cleanText(getFieldStr(tr, "VehicleNo"));
        log.gr_no = DataPreprocessor::cleanText(getFieldStr(tr, "GRNo"));
        log.driver = DataPreprocessor::cleanText(getFieldStr(tr, "DriverName"));
        log.eway_bill_no = DataPreprocessor::cleanText(getFieldStr(tr, "EWayBillNo"));
        if (log.eway_bill_no.empty()) log.eway_bill_no = DataPreprocessor::cleanText(getFieldStr(tr, "ST38No"));
        log.shipping_address = DataPreprocessor::cleanText(getFieldStr(tr, "ShippingAddress"));
        log.po_no = DataPreprocessor::cleanText(getFieldStr(tr, "PurchaseOrderNo"));
        log.distance = DataPreprocessor::parseInt(getFieldStr(tr, "Distance"));
        log.irn_no = DataPreprocessor::cleanText(getFieldStr(tr, "IRNNo"));
        log.bill_time = DataPreprocessor::cleanText(getFieldStr(tr, "DispatchTime"));
        log.grade = DataPreprocessor::cleanText(getFieldStr(tr, "Grade"));
        log.challan_no = DataPreprocessor::cleanText(getFieldStr(tr, "ChallanNo"));
        log.kanda_weight = DataPreprocessor::cleanText(getFieldStr(tr, "KandaWeight"));
        log.transport = DataPreprocessor::cleanText(getFieldStr(tr, "BillItemName"));
        if (log.transport.empty()) log.transport = DataPreprocessor::cleanText(getFieldStr(tr, "TransporterName"));
        log.broker_name = DataPreprocessor::cleanText(getFieldStr(tr, "OtherInfo"));

        outTransportMap[key] = log;
    }
    return true;
}

bool VoucherMigrator::migrate_transactions_bahikhata(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& transRows,
    const std::map<std::string, std::vector<StockMovementEntry>>& stockTransMap,
    const std::map<std::string, LogisticsEntry>& transportMap,
    const std::map<int, std::string>& ledgerCodeMap,
    const std::map<int, PartyDetail>& ledgerDetailMap,
    const std::map<int, std::string>& ledgerDeletedNameMap,
    std::map<QString, int>& fyNameToId,
    std::map<std::string, std::vector<std::map<std::string, std::string>>>& outVoucherGroups,
    int& outSalesInvoiceCount,
    int& outPurchaseInvoiceCount
) {
    // C3 strangler slice V (monolith PASS 5 sections 1-2: direct
    // transactions + invoice/voucher reconstruction): transplanted
    // verbatim. VF keys come from the Transactions JSON entry; the one
    // collapsed-header casualty (amount: legs read the real "Amount"
    // field while the header block wins derived:leg-sum) plus absent
    // fields (PurchaseType/EntryType + expense/TCS/logistics extras) stay
    // documented JSON-gap literals. Silent fy skips gained the F5
    // warn+count (row output unchanged).
    auto VF = [](const char* t) { return MigrationColumns::voucherField(QString::fromLatin1(t)).toStdString(); };
    auto& db = DatabaseManager::instance();
    ctx.updateProgress(60, "Migrating All Transactions & Reconstructing Double-Entry Invoices...");
    db.executeNonQuery("DELETE FROM transactions;");
    db.executeNonQuery("DELETE FROM vouchers;");
    db.executeNonQuery("DELETE FROM sales_invoices;");
    db.executeNonQuery("DELETE FROM sales_invoice_items;");
    db.executeNonQuery("DELETE FROM purchase_invoices;");
    db.executeNonQuery("DELETE FROM purchase_invoice_items;");
    db.executeNonQuery("DELETE FROM paddy_slips;");
    db.executeNonQuery("DELETE FROM jform_vouchers;");
    db.executeNonQuery("DELETE FROM jform_voucher_items;");

    int salesInvoiceCount = 0;
    int purchaseInvoiceCount = 0;

    // Group transactions by (rawType, vNo, vDate)
    std::map<std::string, std::vector<std::map<std::string, std::string>>> voucherGroups;
    for (const auto& r : transRows) {
        std::string vNo = DataPreprocessor::cleanText(getFieldStr(r, VF("voucher_no")));
        if (vNo.empty()) continue;
        std::string rawType = DataPreprocessor::cleanText(getFieldStr(r, VF("trans_type")));
        std::string pType = DataPreprocessor::cleanText(getFieldStr(r, "PurchaseType")); // JSON-gap(VF)
        std::string eType = DataPreprocessor::cleanText(getFieldStr(r, "EntryType")); // JSON-gap(VF)
        std::string narr = DataPreprocessor::cleanText(getFieldStr(r, "Narration")); // JSON-gap(VF): TDS sniff reads Narration only (compound would widen it)
        if (rawType == "Jrnl" && (pType.rfind("TDS", 0) == 0 || eType == "TDS" || eType == "PartyTDS" || narr.find("T.D.S.") != std::string::npos)) {
            rawType = "TDS";
        }
        QString vType = (rawType == "TDS") ? "TDS" : VoucherMigrator::mapVoucherTypeStr(QString::fromStdString(rawType));
        QString vDate = parseDateFormatted(QString::fromStdString(getFieldStr(r, VF("voucher_date"))));
        std::string key = QString("%1-%2-%3").arg(vType).arg(QString::fromStdString(vNo)).arg(vDate).toStdString();
        voucherGroups[key].push_back(r);
    }

    // 1. Direct 1-to-1 Migration of every Transaction row into 'transactions' table
    for (const auto& r : transRows) {
        std::string vNo = DataPreprocessor::cleanText(getFieldStr(r, VF("voucher_no")));
        if (vNo.empty()) continue;
        std::string rawType = DataPreprocessor::cleanText(getFieldStr(r, VF("trans_type")));
        std::string pType = DataPreprocessor::cleanText(getFieldStr(r, "PurchaseType")); // JSON-gap(VF)
        std::string eType = DataPreprocessor::cleanText(getFieldStr(r, "EntryType")); // JSON-gap(VF)
        std::string narr = DataPreprocessor::cleanText(getFieldStr(r, "Narration")); // JSON-gap(VF): TDS sniff reads Narration only (the compound would widen it)
        if (rawType == "Jrnl" && (pType.rfind("TDS", 0) == 0 || eType == "TDS" || eType == "PartyTDS" || narr.find("T.D.S.") != std::string::npos)) {
            rawType = "TDS";
        }
        QString vType = (rawType == "TDS") ? "TDS" : VoucherMigrator::mapVoucherTypeStr(QString::fromStdString(rawType));
        QString vDate = parseDateFormatted(QString::fromStdString(getFieldStr(r, VF("voucher_date"))));
        QString fyVal = computeFinancialYear(vDate);
        int fyId = MigrationUtils::fyIdForStd(fyNameToId, fyVal);
        // F5: fail loud with row context (was a silent skip).
        if (fyId <= 0) { ++ctx.stats.unmappedFyRows; qWarning() << "[Migration] voucher row skipped (unresolvable FY):" << QString::fromStdString(vNo) << fyVal; continue; }

        int acCode = DataPreprocessor::parseInt(getFieldStr(r, VF("account_code")));
        auto itDetail = ledgerDetailMap.find(acCode);
        std::string partyName = lookupLedgerName(acCode, ledgerDetailMap, ledgerCodeMap, ledgerDeletedNameMap);
        if (partyName.empty()) partyName = "Party #" + std::to_string(acCode);
        int partyId = (itDetail != ledgerDetailMap.end()) ? itDetail->second.id : 0;
        QString vchCanon = canonVoucherNo(fyVal, rawType, vNo);
        if (vchCanon.isEmpty()) {
            // Fail loud (§2/F6): no fabricated numbers, no active-FY borrow.
            qWarning() << "[Migration] unnumbered voucher row skipped (FY token unresolvable):"
                       << QString::fromStdString(vNo) << fyVal;
            continue;
        }

        std::string drCr = DataPreprocessor::cleanText(getFieldStr(r, VF("dr_cr")));
        // JSON-gap(VF): the collapsed header block wins derived:leg-sum for
        // "amount", but these legs read the real Amount field.
        double amt = DataPreprocessor::parseDouble(getFieldStr(r, "Amount"));
        std::string invNo = getFieldFirstClean(r, VF("invoice_no"));
        std::string narration = getFieldFirstClean(r, VF("narration"));

        std::string saudaDate = DataPreprocessor::cleanText(getFieldStr(r, VF("sauda_date")));
        std::string bankDate = parseDateFormatted(QString::fromStdString(getFieldStr(r, VF("bank_date")))).toStdString();
        std::string brokerName = DataPreprocessor::cleanText(getFieldStr(r, VF("broker_name")));
        std::string vehicleNo = DataPreprocessor::cleanText(getFieldStr(r, VF("vehicle_no")));
        std::string grNo = DataPreprocessor::cleanText(getFieldStr(r, VF("gr_no")));
        int rowNo = DataPreprocessor::parseInt(getFieldStr(r, VF("row_no")), 1);

        std::string key = QString("%1-%2-%3").arg(vType).arg(QString::fromStdString(vNo)).arg(vDate).toStdString();
        double taxableAmt = DataPreprocessor::parseDouble(getFieldStr(r, VF("taxable_amount")));
        double tdsRate = DataPreprocessor::parseDouble(getFieldStr(r, VF("tds_rate")));
        double tdsAmt = 0.0;
        if (tdsRate > 0.0 || taxableAmt > 0.0) {
            auto itSt = stockTransMap.find(key);
            if (itSt != stockTransMap.end() && !itSt->second.empty()) {
                double stockTotal = 0.0;
                for (const auto& sl : itSt->second) {
                    stockTotal += sl.amount;
                }
                if (stockTotal > amt) {
                    tdsAmt = std::round((stockTotal - amt) * 100.0) / 100.0;
                }
            }
            if (tdsAmt <= 0.0 && tdsRate > 0.0 && taxableAmt > 0.0) {
                tdsAmt = std::round(taxableAmt * tdsRate / 100.0);
            }
        }
        const auto& group = voucherGroups[key];
        std::vector<std::string> opposingList;
        for (const auto& other : group) {
            int otherCode = DataPreprocessor::parseInt(getFieldStr(other, VF("account_code")));
            if (otherCode != acCode) {
                auto itO = ledgerDetailMap.find(otherCode);
                std::string oName = (itO != ledgerDetailMap.end()) ? itO->second.name : ("Account #" + std::to_string(otherCode));
                opposingList.push_back(oName);
            }
        }
        std::string opposingStr;
        if (!opposingList.empty()) {
            opposingStr = opposingList[0];
            if (opposingList.size() > 1) opposingStr += " & Others";
        } else {
            if (rawType == "Sale" || rawType == "SLRN") opposingStr = "Sales Accounts";
            else if (rawType == "Purc" || rawType == "PRRN") opposingStr = "Purchase Accounts";
            else if (rawType == "JFrm") opposingStr = "Paddy Purchase";
            else opposingStr = "Bank/Cash";
        }

        db.executeNonQuery(
            "INSERT INTO transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, trans_type, "
            "account_code, party_id, party_name, opposing_account, dr_cr, amount, "
            "invoice_no, narration, sauda_date, bank_date, broker_name, vehicle_no, gr_no, "
            "taxable_amount, tds_amount, tds_rate, row_no) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                (fyId > 0 ? QVariant(fyId) : QVariant()),
                fyVal,
                vchCanon,
                vDate,
                vType,
                QString::fromStdString(rawType),
                acCode,
                (partyId > 0 ? QVariant(partyId) : QVariant()),
                QString::fromStdString(partyName),
                QString::fromStdString(opposingStr),
                QString::fromStdString(drCr),
                amt,
                QString::fromStdString(invNo),
                QString::fromStdString(narration),
                QString::fromStdString(saudaDate),
                QString::fromStdString(bankDate),
                QString::fromStdString(brokerName),
                QString::fromStdString(vehicleNo),
                QString::fromStdString(grNo),
                taxableAmt,
                tdsAmt,
                tdsRate,
                rowNo
            }
        );
    }

    // 2. Reconstruct Invoices & Vouchers for Edit Entry Pages
    for (const auto& pair : voucherGroups) {
        const std::string& key = pair.first;
        const auto& rows = pair.second;
        const auto& firstR = rows[0];
        std::string vNo = DataPreprocessor::cleanText(getFieldStr(firstR, VF("voucher_no")));
        std::string rawType = DataPreprocessor::cleanText(getFieldStr(firstR, VF("trans_type")));
        std::string pType = DataPreprocessor::cleanText(getFieldStr(firstR, "PurchaseType")); // JSON-gap(VF)
        std::string eType = DataPreprocessor::cleanText(getFieldStr(firstR, "EntryType")); // JSON-gap(VF)
        std::string narr = DataPreprocessor::cleanText(getFieldStr(firstR, "Narration")); // JSON-gap(VF): TDS sniff reads Narration only
        if (rawType == "Jrnl" && (pType.rfind("TDS", 0) == 0 || eType == "TDS" || eType == "PartyTDS" || narr.find("T.D.S.") != std::string::npos)) {
            rawType = "TDS";
        }
        QString vType = (rawType == "TDS") ? "TDS" : VoucherMigrator::mapVoucherTypeStr(QString::fromStdString(rawType));
        QString vDate = parseDateFormatted(QString::fromStdString(getFieldStr(firstR, VF("voucher_date"))));
        QString fyVal = computeFinancialYear(vDate);
        int fyId = MigrationUtils::fyIdForStd(fyNameToId, fyVal);
        // F5: fail loud with row context (was a silent skip).
        if (fyId <= 0) { ++ctx.stats.unmappedFyRows; qWarning() << "[Migration] voucher skipped (unresolvable FY):" << QString::fromStdString(vNo) << fyVal; continue; }

        QString vchNumStr = canonVoucherNo(fyVal, rawType, vNo);
        if (vchNumStr.isEmpty()) {
            // Fail loud (§2/F6): no fabricated numbers, no active-FY borrow.
            qWarning() << "[Migration] unnumbered voucher skipped (FY token unresolvable):"
                       << QString::fromStdString(vNo) << fyVal;
            continue;
        }
        std::string narration = getFieldFirstClean(firstR, VF("narration"));

        std::string invNo = getFieldFirstClean(firstR, VF("invoice_no"));
        if (invNo.empty()) invNo = vNo;

        ResolvedVoucherParty partyRes = resolveVoucherParty(vType, rows, ledgerDetailMap, ledgerCodeMap, ledgerDeletedNameMap);

        double totalAmount = 0.0;
        double partyAmount = 0.0;
        for (const auto& itemRow : rows) {
            double amt = DataPreprocessor::parseDouble(getFieldStr(itemRow, "Amount")); // JSON-gap(VF): see legs note above
            int acCode = DataPreprocessor::parseInt(getFieldStr(itemRow, VF("account_code")));
            totalAmount = std::max(totalAmount, amt);
            if (partyRes.primary_party_legacy_code != 0 && acCode == partyRes.primary_party_legacy_code) {
                partyAmount = amt;
            }
        }
        if (partyAmount <= 0.0) partyAmount = totalAmount;

        auto itStockLines = stockTransMap.find(key);
        auto itLog = transportMap.find(key);
        std::string vehicleNo = (itLog != transportMap.end() && !itLog->second.vehicle_no.empty()) ? itLog->second.vehicle_no : DataPreprocessor::cleanText(getFieldStr(firstR, VF("vehicle_no")));
        std::string grNo = (itLog != transportMap.end() && !itLog->second.gr_no.empty()) ? itLog->second.gr_no : DataPreprocessor::cleanText(getFieldStr(firstR, VF("gr_no")));
        std::string driverName = (itLog != transportMap.end() && !itLog->second.driver.empty()) ? itLog->second.driver : DataPreprocessor::cleanText(getFieldStr(firstR, VF("driver_name")));
        std::string ewayBill = (itLog != transportMap.end() && !itLog->second.eway_bill_no.empty()) ? itLog->second.eway_bill_no : DataPreprocessor::cleanText(getFieldStr(firstR, VF("eway_bill_no")));
        if (ewayBill.empty()) ewayBill = DataPreprocessor::cleanText(getFieldStr(firstR, "ST38No")); // JSON-gap(VF)
        std::string brokerName = (itLog != transportMap.end() && !itLog->second.broker_name.empty()) ? itLog->second.broker_name : DataPreprocessor::cleanText(getFieldStr(firstR, VF("broker_name")));
        std::string farmerName = DataPreprocessor::cleanText(getFieldStr(firstR, VF("farmer_name")));
        std::string bankDate = parseDateFormatted(QString::fromStdString(getFieldStr(firstR, VF("bank_date")))).toStdString();
        std::string saudaDate = DataPreprocessor::cleanText(getFieldStr(firstR, VF("sauda_date")));
        std::string billTime = (itLog != transportMap.end() && !itLog->second.bill_time.empty()) ? itLog->second.bill_time : DataPreprocessor::cleanText(getFieldStr(firstR, "DispatchTime")); // JSON-gap(VF)
        std::string shippingAddress = (itLog != transportMap.end() && !itLog->second.shipping_address.empty()) ? itLog->second.shipping_address : DataPreprocessor::cleanText(getFieldStr(firstR, "ShippingAddress")); // JSON-gap(VF)
        std::string poNo = (itLog != transportMap.end() && !itLog->second.po_no.empty()) ? itLog->second.po_no : DataPreprocessor::cleanText(getFieldStr(firstR, "PurchaseOrderNo")); // JSON-gap(VF)
        std::string grade = (itLog != transportMap.end() && !itLog->second.grade.empty()) ? itLog->second.grade : (itStockLines != stockTransMap.end() && !itStockLines->second.empty() ? itStockLines->second[0].dheri_bill_no : "");
        std::string transport = (itLog != transportMap.end() && !itLog->second.transport.empty()) ? itLog->second.transport : DataPreprocessor::cleanText(getFieldStr(firstR, "BillItemName")); // JSON-gap(VF)
        std::string challanNo = (itLog != transportMap.end() && !itLog->second.challan_no.empty()) ? itLog->second.challan_no : DataPreprocessor::cleanText(getFieldStr(firstR, VF("challan_no")));
        std::string kandaWeight = (itLog != transportMap.end() && !itLog->second.kanda_weight.empty()) ? itLog->second.kanda_weight : DataPreprocessor::cleanText(getFieldStr(firstR, "KandaWeight")); // JSON-gap(VF)

        // 1. Identify Market Type across all 3 types
        std::string rawPurchaseType = DataPreprocessor::cleanText(getFieldStr(firstR, "PurchaseType")); // JSON-gap(VF)
        for (const auto& rItem : rows) {
            std::string pt = DataPreprocessor::cleanText(getFieldStr(rItem, "PurchaseType")); // JSON-gap(VF)
            if (!pt.empty()) { rawPurchaseType = pt; break; }
        }
        std::string marketType = "Market Type (With Stock)";
        if (rawPurchaseType.find("Without Stock") != std::string::npos) {
            marketType = "Market Type (Without Stock)";
        } else if (rawPurchaseType.find("Mandi") != std::string::npos || rawType == "JFrm") {
            marketType = "Mandi Type";
        } else if (rawPurchaseType.find("With Stock") != std::string::npos) {
            marketType = "Market Type (With Stock)";
        } else if (itStockLines == stockTransMap.end() || itStockLines->second.empty()) {
            marketType = "Market Type (Without Stock)";
        }

        // 2. Identify Due Days, Tax Status & GSTIN
        int dueDays = DataPreprocessor::parseInt(getFieldStr(firstR, VF("due_days")), 0);
        std::string taxStatus = DataPreprocessor::cleanText(getFieldStr(firstR, VF("tax_status")));
        if (taxStatus.empty()) taxStatus = (vType == "Sales") ? "GST / Exempt" : "GST";
        std::string placeOfSupply = DataPreprocessor::cleanText(getFieldStr(firstR, VF("place_of_supply")));
        std::string gstinVal = DataPreprocessor::cleanText(getFieldStr(firstR, "ECommGSTIN")); // JSON-gap(VF)
        if (gstinVal.empty() || gstinVal == "0") gstinVal = partyRes.primary_party_gstin;

        // 3. Aggregated Expenses & Calculations
        double freightCharges = 0.0;
        double otherExpenses = 0.0;
        double discountAmount = 0.0;
        double tcsAmount = 0.0;
        double tcsRate = DataPreprocessor::parseDouble(getFieldStr(firstR, "TCSRate"), 0.0); // JSON-gap(VF)
        double roundOff = 0.0;
        double cgstTax = 0.0, sgstTax = 0.0, igstTax = 0.0, cessTax = 0.0;
        double damiAmt = 0.0, labourAmt = 0.0, auctionAmt = 0.0, mFeeAmt = 0.0, hrdfAmt = 0.0, welfareAmt = 0.0, dhrmdAmt = 0.0, sutliAmt = 0.0;

        for (const auto& rItem : rows) {
            std::string entryT = DataPreprocessor::cleanText(getFieldStr(rItem, "EntryType")); // JSON-gap(VF): EntryType VALUES drive the ledger split
            double rAmt = DataPreprocessor::parseDouble(getFieldStr(rItem, "Amount")); // JSON-gap(VF): see legs note above
            std::string rDrCr = DataPreprocessor::cleanText(getFieldStr(rItem, VF("dr_cr")));
            if (entryT == "Freight" || entryT == "FreightBeforeVAT") {
                freightCharges += rAmt;
            } else if (entryT == "Discount") {
                discountAmount += rAmt;
            } else if (entryT == "TCS") {
                tcsAmount += rAmt;
            } else if (entryT == "RoundOff" || entryT == "ROUND" || entryT == "Round Off") {
                roundOff += (rDrCr == "Cr" && vType == "Sales") ? -rAmt : rAmt;
            } else if (entryT == "CGST") {
                cgstTax += rAmt;
            } else if (entryT == "SGST") {
                sgstTax += rAmt;
            } else if (entryT == "IGST") {
                igstTax += rAmt;
            } else if (entryT == "CessTax" || entryT == "Cess") {
                cessTax += rAmt;
            } else if (entryT == "Dami") {
                damiAmt += rAmt;
            } else if (entryT == "Labour" || entryT == "Loading") {
                labourAmt += rAmt;
            } else if (entryT == "Auction") {
                auctionAmt += rAmt;
            } else if (entryT == "MFee" || entryT == "MktFee" || entryT == "MFees") {
                mFeeAmt += rAmt;
            } else if (entryT == "CessHRDF") {
                hrdfAmt += rAmt;
            } else if (entryT == "Welfare") {
                welfareAmt += rAmt;
            } else if (entryT == "Dharmada" || entryT == "Gaushala") {
                dhrmdAmt += rAmt;
            } else if (entryT == "Sutli" || entryT == "Bardana") {
                sutliAmt += rAmt;
            } else if (entryT == "OtherExp" || entryT == "Exp." || entryT == "QualityClaim") {
                otherExpenses += rAmt;
            }
        }
        if (tcsAmount <= 0.0 && tcsRate > 0.0) {
            double tcsTaxable = DataPreprocessor::parseDouble(getFieldStr(firstR, "TCSTaxable")); // JSON-gap(VF)
            if (tcsTaxable > 0.0) tcsAmount = std::round((tcsTaxable * tcsRate / 100.0) * 100.0) / 100.0;
        }

        // Insert Header Voucher
        db.executeNonQuery(
            "INSERT INTO vouchers ("
            "fy_id, financial_year, voucher_no, voucher_date, voucher_type, legacy_type, "
            "ledger_id, party_name, account_type, amount, narration, "
            "vehicle_no, gr_no, driver_name, eway_bill_no, broker_name, farmer_name, bank_date, sauda_date, "
            "due_days, market_type, tax_status, place_of_supply, challan_no) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
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
                QString::fromStdString(farmerName),
                QString::fromStdString(bankDate),
                QString::fromStdString(saudaDate),
                dueDays,
                QString::fromStdString(marketType),
                QString::fromStdString(taxStatus),
                QString::fromStdString(placeOfSupply),
                QString::fromStdString(challanNo)
            }
        );

        // Line Items from StockTransactions
        int itemId = 0;
        std::string itemName = "";
        std::string hsnCode = "";
        int bagCount = 0;
        double weightQtl = 0.0;
        double ratePerQtl = 0.0;
        double gstPct = 0.0;
        double cgstAmt = cgstTax, sgstAmt = sgstTax, igstAmt = igstTax;
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
            double stCgst = 0.0, stSgst = 0.0, stIgst = 0.0;
            for (const auto& line : lines) {
                bagCount += line.bags;
                weightQtl += line.weight_qtl;
                taxableAmt += line.taxable_amount;
                stCgst += line.cgst;
                stSgst += line.sgst;
                stIgst += line.igst;
            }
            if (stCgst > 0.0) cgstAmt = stCgst;
            if (stSgst > 0.0) sgstAmt = stSgst;
            if (stIgst > 0.0) igstAmt = stIgst;

            if (taxableAmt <= 0.0) taxableAmt = partyAmount;
            if (ratePerQtl <= 0.0 && weightQtl > 0.0) {
                ratePerQtl = std::round((taxableAmt / weightQtl) * 100.0) / 100.0;
            }
        } else if (marketType == "Market Type (Without Stock)") {
            bagCount = 0;
            weightQtl = 0.0;
            ratePerQtl = partyAmount;
            taxableAmt = partyAmount;
        }

        if (vType == "Sales") {
            db.executeNonQuery(
                "INSERT INTO sales_invoices ("
                "fy_id, financial_year, voucher_no, invoice_no, invoice_date, customer_id, customer_name, gstin, "
                "item_id, item_name, hsn_code, "
                "bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, "
                "cgst_amount, sgst_amount, igst_amount, round_off, gst_amount, "
                "total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status, market_fee_status, "
                "dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, "
                "gr_no, driver_name, broker_name, sauda_date, bill_time, shipping_address, po_no, grade, "
                "kanda_weight, transport, market_type, due_days, tax_status, challan_no, freight_charges, tcs_amount, tcs_rate, place_of_supply) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'Credit', ?, ?, ?, 'Self Sale', 'Paid', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {
                    (fyId > 0 ? QVariant(fyId) : QVariant()),
                    fyVal,
                    vchNumStr,
                    QString::fromStdString(invNo),
                    vDate,
                    (partyRes.primary_party_id > 0 ? QVariant(partyRes.primary_party_id) : QVariant()),
                    QString::fromStdString(partyRes.primary_party),
                    QString::fromStdString(gstinVal),
                    (itemId > 0 ? QVariant(itemId) : QVariant()),
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
                    roundOff,
                    (cgstAmt + sgstAmt + igstAmt),
                    partyAmount,
                    QString::fromStdString(vehicleNo),
                    QString::fromStdString(ewayBill),
                    QString::fromStdString(narration),
                    damiAmt,
                    labourAmt,
                    auctionAmt,
                    mFeeAmt,
                    hrdfAmt,
                    otherExpenses,
                    welfareAmt,
                    dhrmdAmt,
                    sutliAmt,
                    discountAmount,
                    QString::fromStdString(grNo),
                    QString::fromStdString(driverName),
                    QString::fromStdString(brokerName),
                    QString::fromStdString(saudaDate),
                    QString::fromStdString(billTime),
                    QString::fromStdString(shippingAddress),
                    QString::fromStdString(poNo),
                    QString::fromStdString(grade),
                    QString::fromStdString(kandaWeight),
                    QString::fromStdString(transport),
                    QString::fromStdString(marketType),
                    dueDays,
                    QString::fromStdString(taxStatus),
                    QString::fromStdString(challanNo),
                    freightCharges,
                    tcsAmount,
                    tcsRate,
                    QString::fromStdString(placeOfSupply)
                }
            );
            long long salesInvId = db.lastInsertedId();

            // Insert line items
            if (itStockLines != stockTransMap.end() && !itStockLines->second.empty()) {
                for (const auto& sLine : itStockLines->second) {
                    std::string lineGrade = sLine.grade;
                    if (lineGrade.empty()) lineGrade = grade;
                    db.executeNonQuery(
                        "INSERT INTO sales_invoice_items ("
                        "invoice_id, invoice_no, item_id, item_name, grade, bag_count, packing, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, total_amount) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                        {
                            salesInvId,
                            QString::fromStdString(invNo),
                            (sLine.item_id > 0 ? QVariant(sLine.item_id) : QVariant()),
                            QString::fromStdString(sLine.item_name),
                            QString::fromStdString(lineGrade),
                            sLine.bags,
                            QString::number(sLine.packing, 'f', 3),
                            sLine.weight_qtl,
                            sLine.rate,
                            sLine.taxable_amount,
                            sLine.gst_pct,
                            sLine.amount
                        }
                    );
                }
            } else {
                db.executeNonQuery(
                    "INSERT INTO sales_invoice_items ("
                    "invoice_id, invoice_no, item_id, item_name, grade, bag_count, packing, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, total_amount) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                    {
                        salesInvId,
                        QString::fromStdString(invNo),
                        (itemId > 0 ? QVariant(itemId) : QVariant()),
                        QString::fromStdString(itemName),
                        QString::fromStdString(grade),
                        bagCount,
                        (marketType == "Market Type (Without Stock)" ? "" : "0.500"),
                        weightQtl,
                        ratePerQtl,
                        taxableAmt,
                        gstPct,
                        partyAmount
                    }
                );
            }
            salesInvoiceCount++;
        } else if (vType == "Purchase") {
            purchaseInvoiceCount++;
            db.executeNonQuery(
                "INSERT INTO purchase_invoices ("
                "fy_id, financial_year, voucher_no, invoice_no, invoice_date, supplier_id, supplier_name, gstin, "
                "item_id, item_name, hsn_code, "
                "bag_count, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, "
                "cgst_amount, sgst_amount, igst_amount, round_off, gst_amount, "
                "total_amount, payment_mode, vehicle_no, eway_bill_no, narration, sale_status, market_fee_status, "
                "dami, labour, auction, m_fee, hrdf, other_exp, welfare, dhrmd, sutli, less_amount, "
                "gr_no, driver_name, broker_name, sauda_date, bill_time, shipping_address, po_no, grade, "
                "kanda_weight, transport, market_type, due_days, tax_status, challan_no, freight_charges, tcs_amount, tcs_rate, place_of_supply) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'Credit', ?, ?, ?, 'Self Purchase', 'Paid', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                {
                    (fyId > 0 ? QVariant(fyId) : QVariant()),
                    fyVal,
                    vchNumStr,
                    QString::fromStdString(invNo),
                    vDate,
                    (partyRes.primary_party_id > 0 ? QVariant(partyRes.primary_party_id) : QVariant()),
                    QString::fromStdString(partyRes.primary_party),
                    QString::fromStdString(gstinVal),
                    (itemId > 0 ? QVariant(itemId) : QVariant()),
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
                    roundOff,
                    (cgstAmt + sgstAmt + igstAmt),
                    partyAmount,
                    QString::fromStdString(vehicleNo),
                    QString::fromStdString(ewayBill),
                    QString::fromStdString(narration),
                    damiAmt,
                    labourAmt,
                    auctionAmt,
                    mFeeAmt,
                    hrdfAmt,
                    otherExpenses,
                    welfareAmt,
                    dhrmdAmt,
                    sutliAmt,
                    discountAmount,
                    QString::fromStdString(grNo),
                    QString::fromStdString(driverName),
                    QString::fromStdString(brokerName),
                    QString::fromStdString(saudaDate),
                    QString::fromStdString(billTime),
                    QString::fromStdString(shippingAddress),
                    QString::fromStdString(poNo),
                    QString::fromStdString(grade),
                    QString::fromStdString(kandaWeight),
                    QString::fromStdString(transport),
                    QString::fromStdString(marketType),
                    dueDays,
                    QString::fromStdString(taxStatus),
                    QString::fromStdString(challanNo),
                    freightCharges,
                    tcsAmount,
                    tcsRate,
                    QString::fromStdString(placeOfSupply)
                }
            );
            long long purcInvId = db.lastInsertedId();

            // Insert line items
            if (itStockLines != stockTransMap.end() && !itStockLines->second.empty()) {
                for (const auto& pLine : itStockLines->second) {
                    std::string lineGrade = pLine.grade;
                    if (lineGrade.empty()) lineGrade = grade;
                    db.executeNonQuery(
                        "INSERT INTO purchase_invoice_items ("
                        "invoice_id, invoice_no, item_id, item_name, grade, bag_count, packing, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, total_amount) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                        {
                            purcInvId,
                            QString::fromStdString(invNo),
                            (pLine.item_id > 0 ? QVariant(pLine.item_id) : QVariant()),
                            QString::fromStdString(pLine.item_name),
                            QString::fromStdString(lineGrade),
                            pLine.bags,
                            QString::number(pLine.packing, 'f', 3),
                            pLine.weight_qtl,
                            pLine.rate,
                            pLine.taxable_amount,
                            pLine.gst_pct,
                            pLine.amount
                        }
                    );
                }
            } else {
                db.executeNonQuery(
                    "INSERT INTO purchase_invoice_items ("
                    "invoice_id, invoice_no, item_id, item_name, grade, bag_count, packing, weight_qtl, rate_per_qtl, taxable_amount, gst_pct, total_amount) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                    {
                        purcInvId,
                        QString::fromStdString(invNo),
                        (itemId > 0 ? QVariant(itemId) : QVariant()),
                        QString::fromStdString(itemName),
                        QString::fromStdString(grade),
                        bagCount,
                        (marketType == "Market Type (Without Stock)" ? "" : "0.500"),
                        weightQtl,
                        ratePerQtl,
                        taxableAmt,
                        gstPct,
                        partyAmount
                    }
                );
            }

            // Register in paddy arrivals if purchase item is Paddy
            std::string itemLower = DataPreprocessor::toLowerStr(itemName);
            if (itemLower.find("paddy") != std::string::npos || itemLower.find("dheri") != std::string::npos) {
                db.executeNonQuery(
                    "INSERT INTO paddy_slips (slip_type, "
                    "slip_no, arrival_date, farmer_id, farmer_name, variety, "
                    "bag_count, gross_weight_qtl, moisture_pct, moisture_deduction_qtl, net_weight_qtl, "
                    "rate_per_qtl, hamali_charges, net_amount, payment_status) "
                    "VALUES ('arrival', ?, ?, ?, ?, ?, ?, ?, 17.0, 0.0, ?, ?, 0.0, ?, 'Unpaid');",
                    {
                        QString::fromStdString(invNo),
                        vDate,
                        (partyRes.primary_party_id > 0 ? QVariant(partyRes.primary_party_id) : QVariant()),
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

    outVoucherGroups = std::move(voucherGroups);
    outSalesInvoiceCount = salesInvoiceCount;
    outPurchaseInvoiceCount = purchaseInvoiceCount;
    return true;
}

bool VoucherMigrator::migrate_stock_ledger_bahikhata(
    MigrationContext& ctx,
    const std::vector<std::map<std::string, std::string>>& transRows,
    const std::vector<std::map<std::string, std::string>>& stockTransRows,
    const std::map<int, PartyDetail>& ledgerDetailMap,
    const std::map<int, ItemDetail>& itemCodeMap,
    std::map<QString, int>& fyNameToId
) {
    // C3 strangler slice V (monolith PASS 5.3: stock transactions inventory
    // ledger): transplanted verbatim. txLookup keys on EntryType=="Ledger"
    // counter-party rows (the "Ledger" VALUE filter is logic, not a field
    // lookup). STF serves the stock-row reads; the PartyCode-first /
    // AccountCode-fallback party resolution keeps its verbatim two-step.
    auto VF = [](const char* t) { return MigrationColumns::voucherField(QString::fromLatin1(t)).toStdString(); };
    auto STF = [](const char* t) { return MigrationColumns::stockTransField(QString::fromLatin1(t)).toStdString(); };
    auto& db = DatabaseManager::instance();
    ctx.updateProgress(85, QString("Migrating %1 Stock Movements (Inventory Ledger)...").arg(stockTransRows.size()));
    db.executeNonQuery("DELETE FROM stock_transactions;");

    // Key by {VoucherNumber, TransType, VoucherDate} and strictly filter EntryType == "Ledger"
    std::map<std::tuple<std::string, std::string, std::string>, std::tuple<std::string, int, std::string>> txLookup;
    std::map<std::pair<std::string, std::string>, std::tuple<std::string, int, std::string>> txLookupFallback;

    for (const auto& tr : transRows) {
        std::string entryType = DataPreprocessor::cleanText(getFieldStr(tr, "EntryType")); // JSON-gap(VF): filter VALUE, not a column read
        if (entryType != "Ledger") continue; // Counter-party ledger ONLY (prevents Goods Amount / TDS overwrite)

        std::string vNo = DataPreprocessor::cleanText(getFieldStr(tr, VF("voucher_no")));
        std::string tType = DataPreprocessor::cleanText(getFieldStr(tr, VF("trans_type")));
        QString vDate = parseDateFormatted(QString::fromStdString(getFieldStr(tr, VF("voucher_date"))));
        int pCode = DataPreprocessor::parseInt(getFieldStr(tr, STF("party_id")));
        if (pCode == 0) pCode = DataPreprocessor::parseInt(getFieldStr(tr, VF("account_code"))); // JSON-gap: PartyCode-first, AccountCode fallback
        std::string invNo = getFieldFirstClean(tr, VF("invoice_no"));
        if (invNo.empty()) invNo = vNo;

        auto itParty = ledgerDetailMap.find(pCode);
        if (itParty != ledgerDetailMap.end()) {
            txLookup[{vNo, tType, vDate.toStdString()}] = {invNo, itParty->second.id, itParty->second.name};
            txLookupFallback[{vNo, tType}] = {invNo, itParty->second.id, itParty->second.name};
        }
    }

    for (const auto& st : stockTransRows) {
        QString stDate = parseDateFormatted(QString::fromStdString(getFieldStr(st, STF("voucher_date"))));
        if (stDate.isEmpty()) continue;

        std::string stVno = DataPreprocessor::cleanText(getFieldStr(st, STF("voucher_no")));
        std::string stTt = DataPreprocessor::cleanText(getFieldStr(st, STF("trans_type")));
        std::string stVt = DataPreprocessor::cleanText(getFieldStr(st, STF("voucher_type")));
        std::string stIcode = DataPreprocessor::cleanText(getFieldStr(st, STF("item_code")));
        int iCode = DataPreprocessor::parseInt(stIcode);

        auto itItem = itemCodeMap.find(iCode);
        if (itItem == itemCodeMap.end()) continue;
        int stItemId = itItem->second.id;
        std::string stItemName = itItem->second.name;

        int stBags = DataPreprocessor::parseInt(getFieldStr(st, STF("bags")));
        double stPack = DataPreprocessor::parseDouble(getFieldStr(st, STF("packing")));
        double stWt = DataPreprocessor::parseDouble(getFieldStr(st, STF("weight_qtl")));
        double stRate = DataPreprocessor::parseDouble(getFieldStr(st, STF("rate")));
        double stAmt = DataPreprocessor::parseDouble(getFieldStr(st, STF("amount")));
        double stTaxable = DataPreprocessor::parseDouble(getFieldStr(st, STF("taxable_amount")));
        double stTax = DataPreprocessor::parseDouble(getFieldStr(st, STF("tax")));
        std::string stTaxtype = DataPreprocessor::cleanText(getFieldStr(st, STF("tax_type")));
        // NOTE: single Narration read, no Narrtn fallback — verbatim monolith
        // semantics (unlike the Transactions legs/header compound above).
        std::string stNarr = DataPreprocessor::cleanText(getFieldStr(st, "Narration")); // JSON-gap(STF)
        int stRow = DataPreprocessor::parseInt(getFieldStr(st, STF("row_no")), 1);

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
            }
        }

        QString stFy = computeFinancialYear(stDate);
        int stFyId = MigrationUtils::fyIdForStd(fyNameToId, stFy);
        // F5: fail loud with row context (was a silent skip).
        if (stFyId <= 0) { ++ctx.stats.unmappedFyRows; qWarning() << "[Migration] stock row skipped (unresolvable FY):" << QString::fromStdString(stVno) << stFy; continue; }

        // Fail loud (§2/F6): the unified number is minted once here and
        // must exist before the insert — never store an empty voucher_no.
        QString stVchNo = canonVoucherNo(stFy, stTt, stVno);
        if (stVchNo.isEmpty()) {
            qWarning() << "[Migration] unnumbered stock row skipped (FY token unresolvable):"
                       << QString::fromStdString(stVno) << stFy;
            continue;
        }

        db.executeNonQuery(
            "INSERT INTO stock_transactions ("
            "fy_id, financial_year, voucher_no, voucher_date, trans_type, voucher_type, "
            "party_id, party_name, bill_no, item_id, item_code, item_name, "
            "bags, packing, weight_qtl, rate, amount, taxable_amount, tax, "
            "tax_type, narration, row_no) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            {
                (stFyId > 0 ? QVariant(stFyId) : QVariant()),
                stFy,
                stVchNo,
                stDate,
                QString::fromStdString(stTt),
                QString::fromStdString(stVt),
                (stPid > 0 ? QVariant(stPid) : QVariant()),
                QString::fromStdString(stPname),
                QString::fromStdString(stBill),
                (stItemId > 0 ? QVariant(stItemId) : QVariant()),
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
    std::map<QString, int>& fyNameToId
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
        int fyId = MigrationUtils::fyIdForStd(fyNameToId, fyVal);
        if (fyId <= 0) { ++ctx.stats.unmappedFyRows; continue; }
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
    std::map<QString, int>& fyNameToId
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
        int fyId = MigrationUtils::fyIdForStd(fyNameToId, fyVal);
        if (fyId <= 0) { ++ctx.stats.unmappedFyRows; continue; }
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
