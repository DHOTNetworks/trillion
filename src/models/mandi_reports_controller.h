#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QString>

class MandiReportsController : public QObject {
    Q_OBJECT

public:
    explicit MandiReportsController(QObject* parent = nullptr);
    ~MandiReportsController() override = default;

    // 1. Form M (Market Committee Return - HSAMB)
    Q_INVOKABLE QVariantMap get_form_m_return(const QString& fromDate, const QString& toDate);

    // 2. J-Form Farmer Purchase Register
    Q_INVOKABLE QVariantList get_jform_register(const QString& fromDate, const QString& toDate, int zimidarId = 0);

    // 3. I-Form Buyer Issue Register
    Q_INVOKABLE QVariantList get_iform_register(const QString& fromDate, const QString& toDate, int buyerId = 0);

    // 4. Farmer / Zimidar Khata & Dheri Statement
    Q_INVOKABLE QVariantMap get_farmer_dheri_statement(int zimidarId, const QString& fromDate = "", const QString& toDate = "");

    // 5. Dami & Commission Register
    Q_INVOKABLE QVariantMap get_dami_commission_register(const QString& fromDate, const QString& toDate);

    // Helper: list of all farmers/zimidars
    Q_INVOKABLE QVariantList get_zimidar_list();

    // Helper: list of all buyers
    Q_INVOKABLE QVariantList get_buyer_list();
};
