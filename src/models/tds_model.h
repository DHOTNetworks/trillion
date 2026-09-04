#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QString>

class TdsModel : public QObject {
    Q_OBJECT

public:
    explicit TdsModel(QObject* parent = nullptr);
    ~TdsModel() override = default;

    Q_INVOKABLE QVariantMap get_next_voucher_info(const QString& tdsType = "RENT");
    Q_INVOKABLE QVariantMap get_party_info(int partyId, const QString& tdsType = "RENT");
    Q_INVOKABLE QString get_last_narration(int partyId, const QString& tdsType = "");
    Q_INVOKABLE bool save_tds_voucher(const QVariantMap& data);
    Q_INVOKABLE QVariantMap get_tds_voucher(int voucherId);
    Q_INVOKABLE QVariantList get_tds_register(const QString& fromDate = "", const QString& toDate = "");
    Q_INVOKABLE bool delete_tds_voucher(int voucherId);

signals:
    void voucherSaved();
    void voucherDeleted();
};
