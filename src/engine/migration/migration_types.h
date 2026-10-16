#pragma once

#include <QString>
#include <QVariantMap>
#include <QVariantList>
#include <functional>
#include <string>

namespace MahadevERP {

struct PartyDetail {
    int id = 0;
    std::string name;
    std::string group_name;
    std::string party_type;
    int legacy_id = 0;
    std::string gstin;
    std::string address;
    std::string city;
    std::string state;
    std::string phone;
};

struct ItemDetail {
    int id = 0;
    std::string name;
    std::string hsn;
    // Data-driven (no hardcoded 5% / 50 kg): absent values migrate as 0.
    double gst_rate = 0.0;
    double packing_kg = 0.0;
};

// C3 strangler slice V: voucher-pass shared rows. Transplanted verbatim
// from bahi_khata_migrator.cpp (PASS 4/5) — defaults preserved exactly
// (packing 0.500, gst 5.0): they are monolith semantics, NOT new
// fallbacks; changing them would break migrated-DB parity. The monolith
// keeps its own file-scope copies until the mandi slice retires them.
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

struct ResolvedVoucherParty {
    std::string primary_party;
    int primary_party_id = 0;
    int primary_party_legacy_code = 0;
    std::string primary_party_gstin;
    std::string opposing_account = "Cash";
};

enum class MigrationProvider {
    BahiKhata,
    Busy,
    Tally,
    Custom
};

struct MigrationStats {
    int totalAccounts = 0;
    int totalGroups = 0;
    int totalStockItems = 0;
    int totalStockUnits = 0;
    int totalSalesInvoices = 0;
    int totalPurchaseInvoices = 0;
    int totalGlVouchers = 0;
    int totalGlTransactions = 0;
    int totalMandiRecords = 0;
    int totalTdsVouchers = 0;
    int totalManufacturingBatches = 0;
    int totalJForms = 0;
    int totalIForms = 0;
    int unmappedFyRows = 0; // F5: rows skipped for unparseable FY labels (fail-loud, never default-1)
    double totalDebitSum = 0.0;
    double totalCreditSum = 0.0;
    double glDiscrepancy = 0.0;
    bool isBalanced = true;
    QString companyName;
    QString gstin;
    QString financialYear;
    QString pan;
    QString firmType;
    QString stateName;
    QString stateCode;
    QString address;
};

struct MigrationContext {
    MigrationProvider provider = MigrationProvider::BahiKhata;
    QString sourcePath;
    QString targetDbPath;
    MigrationStats stats;
    bool isCancelled = false;
    std::function<void(int percent, const QString& step)> progressCallback;

    void updateProgress(int percent, const QString& step) {
        if (progressCallback) {
            progressCallback(percent, step);
        }
    }
};

} // namespace MahadevERP
