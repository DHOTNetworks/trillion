#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <QHash>
#include <QMutex>

/**
 * @brief Dynamic Voucher Two-Way Bridge & Persistence Pipeline
 * 
 * Provides unified reading, filtering by Financial Year & Voucher Type,
 * mathematical double-entry balance validation (Sum(Dr) == Sum(Cr)),
 * sequential numbering, and transactional persistence.
 */
struct VoucherEntryLine {
    int lineNumber = 1;
    int accountId = 0;
    QString accountName;
    QString drCr; // "Dr" or "Cr"
    double amount = 0.0;
    QString narration;
    int partyId = 0;
    QString partyName;
};

struct VoucherStockLine {
    int lineNumber = 1;
    int itemId = 0;
    QString itemCode;
    QString itemName;
    QString movementType; // "IN", "OUT", "TRANSFER"
    int bags = 0;
    double packingKg = 50.0;
    double weightQtl = 0.0;
    double ratePerQtl = 0.0;
    double grossAmount = 0.0;
    double gstRate = 0.0;
    double cgst = 0.0;
    double sgst = 0.0;
    double igst = 0.0;
    double netAmount = 0.0;
};

struct UnifiedVoucherData {
    int id = 0;
    int voucherNumber = 0;
    QString voucherType; // "Sale", "Purc", "Pymt", "Rcpt", "Jrnl", "Contra", "Milling", "JFrm", "IFrm"
    QString voucherDate;
    int financialYearId = 0;
    QString financialYearName;
    int partyId = 0;
    QString partyName;
    QString invoiceNo;
    QString taxInvoiceNo;
    double totalAmount = 0.0;
    double taxableAmount = 0.0;
    double totalTax = 0.0;
    double roundOff = 0.0;
    QString narration;
    bool isVerified = false;
    bool isCancelled = false;

    // Mandi Surcharges
    double dami = 0.0;
    double marketFee = 0.0;
    double hrdf = 0.0;
    double labour = 0.0;
    double bardana = 0.0;

    // Logistics
    QString vehicleNo;
    QString grNo;
    QString ewayBillNo;
    QString driverName;
    QString station;

    QVector<VoucherEntryLine> entries;
    QVector<VoucherStockLine> stockLines;

    bool isBalanced() const {
        double drSum = 0.0;
        double crSum = 0.0;
        for (const auto& e : entries) {
            if (e.drCr.compare("Dr", Qt::CaseInsensitive) == 0) drSum += e.amount;
            else crSum += e.amount;
        }
        return std::abs(drSum - crSum) < 0.01;
    }
};

class VoucherPipeline : public QObject {
    Q_OBJECT

public:
    static VoucherPipeline& instance();

    // Query & Filtering across Fiscal Year & Types
    QVector<UnifiedVoucherData> queryVouchers(
        const QString& fromDateIso,
        const QString& toDateIso,
        const QStringList& voucherTypes = {},
        int partyId = 0,
        int itemId = 0,
        int limit = 1000
    );

    UnifiedVoucherData getVoucherById(int voucherId);
    UnifiedVoucherData getVoucherByNumberAndType(int voucherNumber, const QString& voucherType, int fyId = 0);
    int getNextVoucherNumber(const QString& voucherType, int fyId = 0);

    // Two-Way Save & Transactional Persistence
    struct SaveResult {
        bool success = false;
        int voucherId = 0;
        int voucherNumber = 0;
        QString errorMessage;
    };

    SaveResult saveVoucher(const UnifiedVoucherData& data);
    bool deleteVoucher(int voucherId, QString* outError = nullptr);

    // UI Feeding
    QStringList getAvailableVoucherTypes();
    QVariantList getVoucherSummaries(const QString& fromDateIso, const QString& toDateIso, const QString& typeFilter = "");

signals:
    void vouchersChanged();

private:
    VoucherPipeline(QObject* parent = nullptr);
    ~VoucherPipeline() override = default;
    VoucherPipeline(const VoucherPipeline&) = delete;
    VoucherPipeline& operator=(const VoucherPipeline&) = delete;

    QMutex m_mutex;
};
