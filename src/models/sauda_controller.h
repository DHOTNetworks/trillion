#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class SaudaController : public QObject {
    Q_OBJECT
public:
    explicit SaudaController(QObject* parent = nullptr);

    // Create forward contract booking
    bool createSaudaContract(
        const QString& saudaNo,
        const QString& saudaDate,
        const QString& saudaType, // "SALE" or "BUY"
        int partyId,
        const QString& partyName,
        int brokerId,
        const QString& brokerName,
        int itemId,
        const QString& itemName,
        const QString& grade,
        int contractedBags,
        double contractedWeightQtl,
        double ratePerQtl,
        double dalaliRatePerQtl,
        double dalaliPct,
        const QString& deliveryFrom = "",
        const QString& deliveryTo = "",
        const QString& paymentTerms = "",
        const QString& conditionNotes = ""
    );

    // Record fulfillment on contract
    bool fulfillSauda(int saudaId, int bags, double weightQtl);

    // Dalali Settlement
    bool createDalaliSettlement(
        int brokerId,
        const QString& brokerName,
        int saudaId,
        const QString& saudaNo,
        const QString& settlementDate,
        const QString& voucherType,
        const QString& voucherNo,
        const QString& invoiceNo,
        const QString& partyName,
        const QString& itemName,
        double weightQtl,
        double ratePerQtl,
        double dalaliRate,
        double tdsPct = 5.0, // Section 194H TDS
        const QString& narration = ""
    );

    // Post Dalali to Double-Entry Journal Voucher (Dr Dalali Expense, Cr TDS Payable, Cr Broker Ledger)
    bool postDalaliToJournalVoucher(int settlementId);

    // Queries
    QVariantList getSaudaContracts(const QString& saudaType = "ALL", const QString& status = "ALL");
    QVariantList getDalaliSettlements(int brokerId = 0, const QString& fromDate = "", const QString& toDate = "");
    QVariantMap getSaudaById(int id);
    QString generateNextSaudaNo(const QString& prefix = "SD-");

signals:
    void saudaDataChanged();
    void dalaliDataChanged();
};
