#pragma once

#include <QObject>
#include <QVariantMap>
#include <QVariantList>
#include <QString>

class InterestModel : public QObject {
    Q_OBJECT

public:
    explicit InterestModel(QObject* parent = nullptr);
    ~InterestModel() override = default;

    Q_INVOKABLE QVariantMap get_interest_data(
        const QString& partyName,
        const QString& fromDate = "",
        const QString& toDate = "",
        double crRate = 12.0,
        double drRate = 12.0,
        bool dayMethod360 = true,
        int yearDivisor = 365,
        bool includeOpBal = true
    );

    Q_INVOKABLE bool post_interest_voucher(
        const QString& partyName,
        const QString& vchDate,
        double interestAmount,
        bool isReceivable,
        const QString& narration = ""
    );

signals:
    void interestVoucherPosted();
};
