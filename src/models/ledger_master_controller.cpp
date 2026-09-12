#include "ledger_master_controller.h"
#include "parties_model.h"
#include "account_groups_model.h"
#include "../services/financial_math_service.h"
#include <QRegularExpression>
#include <cmath>

LedgerMasterController::LedgerMasterController(QObject *parent)
    : QObject(parent) {
}

void LedgerMasterController::setLedgerId(int id) {
    if (m_ledgerId != id) {
        m_ledgerId = id;
        emit ledgerIdChanged();
        emit isEditModeChanged();
        if (id > 0) {
            loadLedger(id);
        }
    }
}

void LedgerMasterController::setGstin(const QString &v) {
    QString upper = v.trimmed().toUpper();
    if (m_gstin != upper) {
        m_gstin = upper;
        emit gstinChanged();

        // Auto extract PAN if 15 chars standard GSTIN
        if (m_pan.isEmpty() && m_gstin.length() == 15) {
            QString extractedPan = extractPanFromGstin(m_gstin);
            if (!extractedPan.isEmpty()) {
                m_pan = extractedPan;
                emit panChanged();
            }
        }
    }
}

QString LedgerMasterController::openingBalanceFmt() const {
    return QString("%1 %2").arg(FinancialMathService::instance().formatInr(m_openingBalance), m_balanceType);
}

bool LedgerMasterController::validateGstin(const QString &gstinStr) const {
    if (gstinStr.trimmed().isEmpty()) return true;
    QRegularExpression gstinRe("^[0-9]{2}[A-Z]{5}[0-9]{4}[A-Z]{1}[1-9A-Z]{1}Z[0-9A-Z]{1}$");
    return gstinRe.match(gstinStr.trimmed().toUpper()).hasMatch();
}

QString LedgerMasterController::extractPanFromGstin(const QString &gstinStr) const {
    QString g = gstinStr.trimmed().toUpper();
    if (g.length() == 15) {
        return g.mid(2, 10);
    }
    return QString();
}

void LedgerMasterController::resetForm() {
    m_ledgerId = 0;
    emit ledgerIdChanged();
    emit isEditModeChanged();

    m_name.clear(); emit nameChanged();
    m_alias.clear(); emit aliasChanged();
    m_prefix.clear(); emit prefixChanged();
    m_groupName = "Sundry Debtors"; emit groupNameChanged();
    m_partyType = "Customer"; emit partyTypeChanged();
    m_specialType = "Regular"; emit specialTypeChanged();

    m_openingBalance = 0.0;
    m_balanceType = "Dr";
    emit openingBalanceChanged();

    m_mailingName.clear(); emit mailingNameChanged();
    m_address.clear(); emit addressChanged();
    m_city = "Raichur"; emit cityChanged();
    m_district = "Raichur"; emit districtChanged();
    m_state = "Karnataka"; emit stateChanged();
    m_pincode = "584101"; emit pincodeChanged();

    m_phone.clear(); emit phoneChanged();
    m_mobile.clear(); emit mobileChanged();
    m_whatsapp.clear(); emit whatsappChanged();
    m_email.clear(); emit emailChanged();
    m_contactPerson.clear(); emit contactPersonChanged();

    m_gstin.clear(); emit gstinChanged();
    m_pan.clear(); emit panChanged();
    m_aadhaar.clear(); emit aadhaarChanged();

    m_creditLimit = 0.0; emit creditLimitChanged();
    m_creditDays = 30; emit creditDaysChanged();

    m_bankName.clear(); emit bankNameChanged();
    m_bankAccount.clear(); emit bankAccountChanged();
    m_ifscCode.clear(); emit ifscCodeChanged();

    m_statusMessage.clear();
    m_isError = false;
    emit statusChanged();
}

bool LedgerMasterController::loadLedger(int id) {
    if (id <= 0) return false;

    PartiesModel pModel;
    auto party = pModel.get_party_by_id(id);
    if (party.isEmpty()) return false;

    m_ledgerId = id;
    emit ledgerIdChanged();
    emit isEditModeChanged();

    m_name = party.value("name").toString(); emit nameChanged();
    m_alias = party.value("alias").toString(); emit aliasChanged();
    m_prefix = party.value("prefix").toString(); emit prefixChanged();
    m_groupName = party.value("group_name", "Sundry Debtors").toString(); emit groupNameChanged();
    m_partyType = party.value("party_type", "Customer").toString(); emit partyTypeChanged();
    m_specialType = party.value("special_type", "Regular").toString(); emit specialTypeChanged();

    m_openingBalance = party.value("opening_balance", 0.0).toDouble();
    m_balanceType = party.value("balance_type", "Dr").toString();
    emit openingBalanceChanged();

    m_mailingName = party.value("mailing_name").toString(); emit mailingNameChanged();
    m_address = party.value("address").toString(); emit addressChanged();
    m_city = party.value("city").toString(); emit cityChanged();
    m_district = party.value("district").toString(); emit districtChanged();
    m_state = party.value("state").toString(); emit stateChanged();
    m_pincode = party.value("pincode").toString(); emit pincodeChanged();

    m_phone = party.value("phone").toString(); emit phoneChanged();
    m_mobile = party.value("mobile").toString(); emit mobileChanged();
    m_whatsapp = party.value("whatsapp").toString(); emit whatsappChanged();
    m_email = party.value("email").toString(); emit emailChanged();
    m_contactPerson = party.value("contact_person").toString(); emit contactPersonChanged();

    m_gstin = party.value("gstin").toString(); emit gstinChanged();
    m_pan = party.value("pan").toString(); emit panChanged();
    m_aadhaar = party.value("aadhaar").toString(); emit aadhaarChanged();

    m_creditLimit = party.value("credit_limit", 0.0).toDouble(); emit creditLimitChanged();
    m_creditDays = party.value("credit_days", 30).toInt(); emit creditDaysChanged();

    m_bankName = party.value("bank_name").toString(); emit bankNameChanged();
    m_bankAccount = party.value("bank_account").toString(); emit bankAccountChanged();
    m_ifscCode = party.value("ifsc_code").toString(); emit ifscCodeChanged();

    m_statusMessage.clear();
    m_isError = false;
    emit statusChanged();
    return true;
}

bool LedgerMasterController::loadLedgerByName(const QString &name) {
    if (name.trimmed().isEmpty()) return false;
    PartiesModel pModel;
    auto party = pModel.get_party_by_name(name);
    if (!party.isEmpty() && party.contains("id")) {
        return loadLedger(party.value("id").toInt());
    }
    return false;
}

bool LedgerMasterController::saveLedger() {
    m_statusMessage.clear();
    m_isError = false;

    if (m_name.trimmed().isEmpty()) {
        m_statusMessage = "Please enter a valid Ledger Account Name.";
        m_isError = true;
        emit statusChanged();
        return false;
    }

    if (!m_gstin.trimmed().isEmpty() && !validateGstin(m_gstin)) {
        m_statusMessage = "Warning: GSTIN format appears invalid (expected 15 alphanumeric characters).";
    }

    QString mailing = m_mailingName.trimmed().isEmpty() ? m_name.trimmed() : m_mailingName.trimmed();

    PartiesModel pModel;
    bool ok = false;
    if (m_ledgerId > 0) {
        ok = pModel.update_ledger_full(
            m_ledgerId, m_name.trimmed(), m_alias.trimmed(), m_prefix.trimmed(),
            m_groupName.trimmed(), m_partyType.trimmed(), m_specialType.trimmed(),
            m_openingBalance, m_balanceType.trimmed(), mailing,
            m_address.trimmed(), m_city.trimmed(), m_district.trimmed(),
            m_state.trimmed(), m_pincode.trimmed(), m_phone.trimmed(),
            m_mobile.trimmed(), m_whatsapp.trimmed(), m_email.trimmed(),
            m_contactPerson.trimmed(), m_gstin.trimmed(), m_pan.trimmed(),
            m_aadhaar.trimmed(), m_creditLimit, m_creditDays,
            m_bankName.trimmed(), m_bankAccount.trimmed(), m_ifscCode.trimmed()
        );
        if (ok) {
            m_statusMessage = QString("Ledger Account '%1' updated successfully!").arg(m_name.trimmed());
        }
    } else {
        ok = pModel.add_ledger_full(
            m_name.trimmed(), m_alias.trimmed(), m_prefix.trimmed(),
            m_groupName.trimmed(), m_partyType.trimmed(), m_specialType.trimmed(),
            m_openingBalance, m_balanceType.trimmed(), mailing,
            m_address.trimmed(), m_city.trimmed(), m_district.trimmed(),
            m_state.trimmed(), m_pincode.trimmed(), m_phone.trimmed(),
            m_mobile.trimmed(), m_whatsapp.trimmed(), m_email.trimmed(),
            m_contactPerson.trimmed(), m_gstin.trimmed(), m_pan.trimmed(),
            m_aadhaar.trimmed(), m_creditLimit, m_creditDays,
            m_bankName.trimmed(), m_bankAccount.trimmed(), m_ifscCode.trimmed()
        );
        if (ok) {
            m_statusMessage = QString("Ledger Account '%1' created successfully!").arg(m_name.trimmed());
        }
    }

    if (ok) {
        m_isError = false;
        emit statusChanged();
        emit ledgerSaved();
        return true;
    } else {
        m_statusMessage = "Failed to save Ledger Account in database.";
        m_isError = true;
        emit statusChanged();
        return false;
    }
}

bool LedgerMasterController::saveGroup(const QString &name, const QString &parentGroup, const QString &nature, const QString &desc, bool balanceSheet) {
    if (name.trimmed().isEmpty()) {
        m_statusMessage = "Please enter a valid Account Group Name.";
        m_isError = true;
        emit statusChanged();
        return false;
    }

    AccountGroupsModel gModel;
    bool ok = gModel.add_group(name.trimmed(), parentGroup.trimmed(), nature.trimmed(), desc.trimmed(), balanceSheet);
    if (ok) {
        m_statusMessage = QString("Account Group '%1' created successfully!").arg(name.trimmed());
        m_isError = false;
        emit statusChanged();
        emit groupSaved();
        return true;
    } else {
        m_statusMessage = "Failed to create Account Group in database.";
        m_isError = true;
        emit statusChanged();
        return false;
    }
}

bool LedgerMasterController::updateGroup(int groupId, const QString &name, const QString &parentGroup, const QString &nature, const QString &desc, bool balanceSheet) {
    if (groupId <= 0 || name.trimmed().isEmpty()) {
        m_statusMessage = "Please select a valid Account Group to update.";
        m_isError = true;
        emit statusChanged();
        return false;
    }

    AccountGroupsModel gModel;
    bool ok = gModel.update_group(groupId, name.trimmed(), parentGroup.trimmed(), nature.trimmed(), desc.trimmed(), balanceSheet);
    if (ok) {
        m_statusMessage = QString("Account Group '%1' updated successfully!").arg(name.trimmed());
        m_isError = false;
        emit statusChanged();
        emit groupSaved();
        return true;
    } else {
        m_statusMessage = "Failed to update Account Group in database.";
        m_isError = true;
        emit statusChanged();
        return false;
    }
}
