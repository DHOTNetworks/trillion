#pragma once

#include <QString>
#include <QVariantMap>
#include <QVariantList>
#include <functional>

namespace MahadevERP {

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
    int totalMillingBatches = 0;
    int totalJForms = 0;
    int totalIForms = 0;
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
