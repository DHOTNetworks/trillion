#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class GateRegisterController : public QObject {
    Q_OBJECT
public:
    explicit GateRegisterController(QObject* parent = nullptr);

    // Create a new gate pass entry
    bool createGateEntry(
        const QString& gatePassNo,
        const QString& entryDate,
        const QString& entryTime,
        const QString& direction,
        const QString& purpose,
        const QString& vehicleNo,
        const QString& driverName,
        const QString& driverPhone,
        const QString& transporterName,
        int partyId,
        const QString& partyName,
        const QString& commodity,
        int bagCount,
        double grossWeightQtl,
        const QString& remarks = ""
    );

    // Update status / Weighbridge linkage
    bool updateWeighbridgeWeights(
        const QString& gatePassNo,
        const QString& weighbridgeSlipNo,
        double grossWeightQtl,
        double tareWeightQtl,
        double netWeightQtl
    );

    bool updateGateStatus(int id, const QString& newStatus, const QString& exitTime = "");
    bool linkVoucher(const QString& gatePassNo, const QString& voucherNo);

    // Queries
    QVariantList getLiveGateEntries(const QString& filterDirection = "ALL", const QString& filterStatus = "ALL");
    QVariantList getGateRegisterHistory(const QString& fromDate = "", const QString& toDate = "", const QString& direction = "ALL");
    QVariantMap getGateEntryByPassNo(const QString& gatePassNo);

    QString generateNextGatePassNo(const QString& prefix = "GP-");

signals:
    void gateDataChanged();
};
