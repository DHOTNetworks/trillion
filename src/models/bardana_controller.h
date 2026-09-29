#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class BardanaController : public QObject {
    Q_OBJECT
public:
    explicit BardanaController(QObject* parent = nullptr);

    // Create a new Bardana transaction (Issue, Receive, Sale, Purchase, Opening)
    bool createBardanaTransaction(
        const QString& vchType,
        const QString& voucherDate,
        const QString& voucherNo,
        int partyId,
        const QString& partyName,
        const QString& bardanaType,
        const QString& godownName,
        const QString& drCr,
        int qty,
        double rate,
        const QString& vehicleNo = "",
        const QString& billNo = "",
        const QString& narration = ""
    );

    // Party-wise net running bag balance
    int getPartyBagBalance(int partyId, const QString& bardanaType = "");

    // Godown-wise physical bag stock
    int getGodownBagStock(const QString& godownName, const QString& bardanaType = "");

    // History and Ledger Queries
    QVariantList getBardanaLedger(int partyId, const QString& fromDate = "", const QString& toDate = "");
    QVariantList getBardanaRegister(const QString& fromDate = "", const QString& toDate = "", const QString& vchType = "");
    QVariantList getGodownStockSummary();
    QVariantList getPartyBalanceSummary();

    bool deleteBardanaTransaction(int id);

signals:
    void bardanaDataChanged();
};
