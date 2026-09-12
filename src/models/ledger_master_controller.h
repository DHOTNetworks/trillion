#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class LedgerMasterController : public QObject {
    Q_OBJECT

    Q_PROPERTY(int ledgerId READ ledgerId WRITE setLedgerId NOTIFY ledgerIdChanged)
    Q_PROPERTY(bool isEditMode READ isEditMode NOTIFY isEditModeChanged)

    // Identity & Group
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString alias READ alias WRITE setAlias NOTIFY aliasChanged)
    Q_PROPERTY(QString prefix READ prefix WRITE setPrefix NOTIFY prefixChanged)
    Q_PROPERTY(QString groupName READ groupName WRITE setGroupName NOTIFY groupNameChanged)
    Q_PROPERTY(QString partyType READ partyType WRITE setPartyType NOTIFY partyTypeChanged)
    Q_PROPERTY(QString specialType READ specialType WRITE setSpecialType NOTIFY specialTypeChanged)

    // Opening Balance
    Q_PROPERTY(double openingBalance READ openingBalance WRITE setOpeningBalance NOTIFY openingBalanceChanged)
    Q_PROPERTY(QString balanceType READ balanceType WRITE setBalanceType NOTIFY balanceTypeChanged)
    Q_PROPERTY(QString openingBalanceFmt READ openingBalanceFmt NOTIFY openingBalanceChanged)

    // Address & Location
    Q_PROPERTY(QString mailingName READ mailingName WRITE setMailingName NOTIFY mailingNameChanged)
    Q_PROPERTY(QString address READ address WRITE setAddress NOTIFY addressChanged)
    Q_PROPERTY(QString city READ city WRITE setCity NOTIFY cityChanged)
    Q_PROPERTY(QString district READ district WRITE setDistrict NOTIFY districtChanged)
    Q_PROPERTY(QString state READ state WRITE setState NOTIFY stateChanged)
    Q_PROPERTY(QString pincode READ pincode WRITE setPincode NOTIFY pincodeChanged)

    // Contact
    Q_PROPERTY(QString phone READ phone WRITE setPhone NOTIFY phoneChanged)
    Q_PROPERTY(QString mobile READ mobile WRITE setMobile NOTIFY mobileChanged)
    Q_PROPERTY(QString whatsapp READ whatsapp WRITE setWhatsapp NOTIFY whatsappChanged)
    Q_PROPERTY(QString email READ email WRITE setEmail NOTIFY emailChanged)
    Q_PROPERTY(QString contactPerson READ contactPerson WRITE setContactPerson NOTIFY contactPersonChanged)

    // Tax & Regulatory
    Q_PROPERTY(QString gstin READ gstin WRITE setGstin NOTIFY gstinChanged)
    Q_PROPERTY(QString pan READ pan WRITE setPan NOTIFY panChanged)
    Q_PROPERTY(QString aadhaar READ aadhaar WRITE setAadhaar NOTIFY aadhaarChanged)

    // Credit & Terms
    Q_PROPERTY(double creditLimit READ creditLimit WRITE setCreditLimit NOTIFY creditLimitChanged)
    Q_PROPERTY(int creditDays READ creditDays WRITE setCreditDays NOTIFY creditDaysChanged)

    // Bank Details
    Q_PROPERTY(QString bankName READ bankName WRITE setBankName NOTIFY bankNameChanged)
    Q_PROPERTY(QString bankAccount READ bankAccount WRITE setBankAccount NOTIFY bankAccountChanged)
    Q_PROPERTY(QString ifscCode READ ifscCode WRITE setIfscCode NOTIFY ifscCodeChanged)

    // Status
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(bool isError READ isError NOTIFY statusChanged)

public:
    explicit LedgerMasterController(QObject *parent = nullptr);

    int ledgerId() const { return m_ledgerId; }
    void setLedgerId(int id);
    bool isEditMode() const { return m_ledgerId > 0; }

    QString name() const { return m_name; }
    void setName(const QString &v) { if (m_name != v) { m_name = v; emit nameChanged(); } }

    QString alias() const { return m_alias; }
    void setAlias(const QString &v) { if (m_alias != v) { m_alias = v; emit aliasChanged(); } }

    QString prefix() const { return m_prefix; }
    void setPrefix(const QString &v) { if (m_prefix != v) { m_prefix = v; emit prefixChanged(); } }

    QString groupName() const { return m_groupName; }
    void setGroupName(const QString &v) { if (m_groupName != v) { m_groupName = v; emit groupNameChanged(); } }

    QString partyType() const { return m_partyType; }
    void setPartyType(const QString &v) { if (m_partyType != v) { m_partyType = v; emit partyTypeChanged(); } }

    QString specialType() const { return m_specialType; }
    void setSpecialType(const QString &v) { if (m_specialType != v) { m_specialType = v; emit specialTypeChanged(); } }

    double openingBalance() const { return m_openingBalance; }
    void setOpeningBalance(double v) { if (std::abs(m_openingBalance - v) > 0.001) { m_openingBalance = v; emit openingBalanceChanged(); } }

    QString balanceType() const { return m_balanceType; }
    void setBalanceType(const QString &v) { if (m_balanceType != v) { m_balanceType = v; emit balanceTypeChanged(); } }

    QString openingBalanceFmt() const;

    QString mailingName() const { return m_mailingName; }
    void setMailingName(const QString &v) { if (m_mailingName != v) { m_mailingName = v; emit mailingNameChanged(); } }

    QString address() const { return m_address; }
    void setAddress(const QString &v) { if (m_address != v) { m_address = v; emit addressChanged(); } }

    QString city() const { return m_city; }
    void setCity(const QString &v) { if (m_city != v) { m_city = v; emit cityChanged(); } }

    QString district() const { return m_district; }
    void setDistrict(const QString &v) { if (m_district != v) { m_district = v; emit districtChanged(); } }

    QString state() const { return m_state; }
    void setState(const QString &v) { if (m_state != v) { m_state = v; emit stateChanged(); } }

    QString pincode() const { return m_pincode; }
    void setPincode(const QString &v) { if (m_pincode != v) { m_pincode = v; emit pincodeChanged(); } }

    QString phone() const { return m_phone; }
    void setPhone(const QString &v) { if (m_phone != v) { m_phone = v; emit phoneChanged(); } }

    QString mobile() const { return m_mobile; }
    void setMobile(const QString &v) { if (m_mobile != v) { m_mobile = v; emit mobileChanged(); } }

    QString whatsapp() const { return m_whatsapp; }
    void setWhatsapp(const QString &v) { if (m_whatsapp != v) { m_whatsapp = v; emit whatsappChanged(); } }

    QString email() const { return m_email; }
    void setEmail(const QString &v) { if (m_email != v) { m_email = v; emit emailChanged(); } }

    QString contactPerson() const { return m_contactPerson; }
    void setContactPerson(const QString &v) { if (m_contactPerson != v) { m_contactPerson = v; emit contactPersonChanged(); } }

    QString gstin() const { return m_gstin; }
    void setGstin(const QString &v);

    QString pan() const { return m_pan; }
    void setPan(const QString &v) { if (m_pan != v) { m_pan = v; emit panChanged(); } }

    QString aadhaar() const { return m_aadhaar; }
    void setAadhaar(const QString &v) { if (m_aadhaar != v) { m_aadhaar = v; emit aadhaarChanged(); } }

    double creditLimit() const { return m_creditLimit; }
    void setCreditLimit(double v) { if (std::abs(m_creditLimit - v) > 0.001) { m_creditLimit = v; emit creditLimitChanged(); } }

    int creditDays() const { return m_creditDays; }
    void setCreditDays(int v) { if (m_creditDays != v) { m_creditDays = v; emit creditDaysChanged(); } }

    QString bankName() const { return m_bankName; }
    void setBankName(const QString &v) { if (m_bankName != v) { m_bankName = v; emit bankNameChanged(); } }

    QString bankAccount() const { return m_bankAccount; }
    void setBankAccount(const QString &v) { if (m_bankAccount != v) { m_bankAccount = v; emit bankAccountChanged(); } }

    QString ifscCode() const { return m_ifscCode; }
    void setIfscCode(const QString &v) { if (m_ifscCode != v) { m_ifscCode = v; emit ifscCodeChanged(); } }

    QString statusMessage() const { return m_statusMessage; }
    bool isError() const { return m_isError; }

    // Invokables
    Q_INVOKABLE void resetForm();
    Q_INVOKABLE bool loadLedger(int id);
    Q_INVOKABLE bool loadLedgerByName(const QString &name);
    Q_INVOKABLE bool validateGstin(const QString &gstinStr) const;
    Q_INVOKABLE QString extractPanFromGstin(const QString &gstinStr) const;
    Q_INVOKABLE bool saveLedger();
    Q_INVOKABLE bool saveGroup(const QString &name, const QString &parentGroup, const QString &nature, const QString &desc = "", bool balanceSheet = false);
    Q_INVOKABLE bool updateGroup(int groupId, const QString &name, const QString &parentGroup, const QString &nature, const QString &desc = "", bool balanceSheet = false);

signals:
    void ledgerIdChanged();
    void isEditModeChanged();
    void nameChanged();
    void aliasChanged();
    void prefixChanged();
    void groupNameChanged();
    void partyTypeChanged();
    void specialTypeChanged();
    void openingBalanceChanged();
    void balanceTypeChanged();
    void mailingNameChanged();
    void addressChanged();
    void cityChanged();
    void districtChanged();
    void stateChanged();
    void pincodeChanged();
    void phoneChanged();
    void mobileChanged();
    void whatsappChanged();
    void emailChanged();
    void contactPersonChanged();
    void gstinChanged();
    void panChanged();
    void aadhaarChanged();
    void creditLimitChanged();
    void creditDaysChanged();
    void bankNameChanged();
    void bankAccountChanged();
    void ifscCodeChanged();
    void statusChanged();
    void ledgerSaved();
    void groupSaved();

private:
    int m_ledgerId = 0;

    QString m_name;
    QString m_alias;
    QString m_prefix;
    QString m_groupName = "Sundry Debtors";
    QString m_partyType = "Customer";
    QString m_specialType = "Regular";

    double m_openingBalance = 0.0;
    QString m_balanceType = "Dr";

    QString m_mailingName;
    QString m_address;
    QString m_city = "Raichur";
    QString m_district = "Raichur";
    QString m_state = "Karnataka";
    QString m_pincode = "584101";

    QString m_phone;
    QString m_mobile;
    QString m_whatsapp;
    QString m_email;
    QString m_contactPerson;

    QString m_gstin;
    QString m_pan;
    QString m_aadhaar;

    double m_creditLimit = 0.0;
    int m_creditDays = 30;

    QString m_bankName;
    QString m_bankAccount;
    QString m_ifscCode;

    QString m_statusMessage;
    bool m_isError = false;
};
