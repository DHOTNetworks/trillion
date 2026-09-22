#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QString>

class IFormModel : public QObject {
    Q_OBJECT

public:
    explicit IFormModel(QObject* parent = nullptr);
    ~IFormModel() override = default;

    Q_INVOKABLE QVariantMap get_next_voucher_info(const QString& fy = "");
    Q_INVOKABLE QVariantMap get_buyer_balance(int buyerId);
    Q_INVOKABLE bool save_iform_voucher(const QVariantMap& data, const QVariantList& items);
    Q_INVOKABLE QVariantMap get_iform_voucher(const QVariant& voucherIdOrNo);
    Q_INVOKABLE QVariantMap get_previous_iform_voucher(int currentId = 0, const QString& currentIfOrVchNo = "");
    Q_INVOKABLE QVariantMap get_next_iform_voucher(int currentId = 0, const QString& currentIfOrVchNo = "");
    Q_INVOKABLE QVariantList get_iform_register(const QString& fromDate = "", const QString& toDate = "");
    Q_INVOKABLE bool delete_iform_voucher(int voucherId);

signals:
    void voucherSaved();
    void voucherDeleted();
};
