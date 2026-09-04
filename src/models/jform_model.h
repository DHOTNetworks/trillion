#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QString>

class JFormModel : public QObject {
    Q_OBJECT

public:
    explicit JFormModel(QObject* parent = nullptr);
    ~JFormModel() override = default;

    Q_INVOKABLE QVariantMap get_next_voucher_info();
    Q_INVOKABLE QVariantMap get_zimidar_balance(int zimidarId);
    Q_INVOKABLE bool save_jform_voucher(const QVariantMap& data, const QVariantList& items);
    Q_INVOKABLE QVariantMap get_jform_voucher(int voucherId);
    Q_INVOKABLE QVariantList get_jform_register(const QString& fromDate = "", const QString& toDate = "");
    Q_INVOKABLE bool delete_jform_voucher(int voucherId);

signals:
    void voucherSaved();
    void voucherDeleted();
};
