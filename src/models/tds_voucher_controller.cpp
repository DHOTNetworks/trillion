#include "tds_voucher_controller.h"
#include "tds_model.h"
#include "parties_model.h"
#include "financial_years_model.h"
#include "../services/accounting_date_service.h"
#include "../services/financial_math_service.h"
#include <cmath>

TdsVoucherController::TdsVoucherController(QObject *parent)
    : QObject(parent) {
}

void TdsVoucherController::setEditVoucherId(int id) {
    if (m_editVoucherId != id) {
        m_editVoucherId = id;
        emit editVoucherIdChanged();
        emit isEditModeChanged();
    }
}

void TdsVoucherController::setVoucherDate(const QString &d) {
    QString resolved = AccountingDateService::instance().resolveDate(d);
    if (m_voucherDate != resolved) {
        m_voucherDate = resolved;
        emit voucherDateChanged();
        emit dayOfWeekChanged();
    }
}

QString TdsVoucherController::dayOfWeek() const {
    return AccountingDateService::instance().getDayOfWeek(m_voucherDate);
}

void TdsVoucherController::setTdsType(const QString &t) {
    QString upper = t.trimmed().toUpper();
    if (m_tdsType != upper) {
        m_tdsType = upper;
        emit tdsTypeChanged();

        TdsModel tModel;
        auto info = tModel.get_next_voucher_info(m_tdsType);
        if (info.contains("default_tds_rate")) m_tdsRate = info.value("default_tds_rate").toDouble();
        if (info.contains("default_surcharge_rate")) m_surchargeRate = info.value("default_surcharge_rate").toDouble();
        if (info.contains("default_cess_rate")) m_cessRate = info.value("default_cess_rate").toDouble();

        if (info.contains("default_exp_ledger_name") && !info.value("default_exp_ledger_name").toString().isEmpty()) {
            m_expLedgerName = info.value("default_exp_ledger_name").toString();
            emit expLedgerNameChanged();
        }
        if (info.contains("default_tds_ledger_name") && !info.value("default_tds_ledger_name").toString().isEmpty()) {
            m_tdsLedgerName = info.value("default_tds_ledger_name").toString();
            emit tdsLedgerNameChanged();
        }

        if (!m_selectedPartyName.isEmpty()) {
            updatePartyInfo(m_selectedPartyName);
        }
        recalculateTotals();
    }
}

void TdsVoucherController::setSelectedPartyName(const QString &name) {
    if (m_selectedPartyName != name) {
        m_selectedPartyName = name;
        emit selectedPartyNameChanged();
        updatePartyInfo(name);
    }
}

void TdsVoucherController::updatePartyInfo(const QString &partyName) {
    m_selectedPartyName = partyName.trimmed();
    emit selectedPartyNameChanged();

    PartiesModel pModel;
    auto party = pModel.get_party_by_name(m_selectedPartyName);
    int pId = party.value("id").toInt();
    m_selectedPartyId = pId;
    emit selectedPartyIdChanged();

    TdsModel tModel;
    auto info = tModel.get_party_info(pId, m_tdsType);
    m_partyBalanceText = info.value("formatted_balance", "Date Bal. 0.00").toString();
    emit partyBalanceTextChanged();

    m_partyPan = party.contains("pan") && !party.value("pan").toString().isEmpty()
                     ? party.value("pan").toString()
                     : info.value("pan_no", "").toString();
    emit partyPanChanged();

    m_previousAmount = info.value("previous_amount", 0.0).toDouble();

    if (m_narration.trimmed().isEmpty()) {
        QString lastNarr = info.value("last_narration", "").toString();
        if (!lastNarr.isEmpty()) {
            m_narration = lastNarr;
            emit narrationChanged();
        }
    }

    recalculateTotals();
}

void TdsVoucherController::setIncomeAmount(double amt) {
    if (std::abs(m_incomeAmount - amt) > 0.001) {
        m_incomeAmount = amt;
        recalculateTotals();
    }
}

void TdsVoucherController::setPreviousAmount(double amt) {
    if (std::abs(m_previousAmount - amt) > 0.001) {
        m_previousAmount = amt;
        recalculateTotals();
    }
}

void TdsVoucherController::setTdsRate(double r) {
    if (std::abs(m_tdsRate - r) > 0.001) {
        m_tdsRate = r;
        recalculateTotals();
    }
}

void TdsVoucherController::setSurchargeRate(double r) {
    if (std::abs(m_surchargeRate - r) > 0.001) {
        m_surchargeRate = r;
        recalculateTotals();
    }
}

void TdsVoucherController::setCessRate(double r) {
    if (std::abs(m_cessRate - r) > 0.001) {
        m_cessRate = r;
        recalculateTotals();
    }
}

void TdsVoucherController::setUseRoundedTotal(bool b) {
    if (m_useRoundedTotal != b) {
        m_useRoundedTotal = b;
        recalculateTotals();
    }
}

QString TdsVoucherController::incomeAmountFmt() const {
    return FinancialMathService::instance().formatInr(m_incomeAmount);
}

QString TdsVoucherController::totalTaxAmountFmt() const {
    return FinancialMathService::instance().formatInr(m_totalTaxAmount);
}

QString TdsVoucherController::netAmountFmt() const {
    return FinancialMathService::instance().formatInr(m_netAmount);
}

void TdsVoucherController::recalculateTotals() {
    m_totalForTds = m_incomeAmount + m_previousAmount;

    m_tdsTaxAmount = (m_incomeAmount * m_tdsRate) / 100.0;
    m_surchargeTaxAmount = (m_tdsTaxAmount * m_surchargeRate) / 100.0;
    m_cessTaxAmount = ((m_tdsTaxAmount + m_surchargeTaxAmount) * m_cessRate) / 100.0;

    m_totalTaxRate = m_tdsRate + m_surchargeRate + m_cessRate;
    double rawTotalTax = m_tdsTaxAmount + m_surchargeTaxAmount + m_cessTaxAmount;

    if (m_useRoundedTotal) {
        m_totalTaxAmount = std::round(rawTotalTax);
    } else {
        m_totalTaxAmount = FinancialMathService::instance().round2(rawTotalTax);
    }

    m_netAmount = m_incomeAmount - m_totalTaxAmount;
    if (m_netAmount < 0.0) m_netAmount = 0.0;

    emit totalsChanged();
}

void TdsVoucherController::resetForm(const QString &workingDate) {
    m_editVoucherId = 0;
    emit editVoucherIdChanged();
    emit isEditModeChanged();

    FinancialYearsModel fyModel;
    QString wDate = !workingDate.isEmpty() ? workingDate : fyModel.get_working_date();
    if (wDate.isEmpty()) {
        wDate = QDate::currentDate().toString("dd/MM/yyyy");
    }
    setVoucherDate(wDate);

    TdsModel tModel;
    auto info = tModel.get_next_voucher_info(m_tdsType);
    m_voucherNo = info.value("next_voucher_no", 1).toInt();
    emit voucherNoChanged();

    m_tdsRate = info.value("default_tds_rate", 10.0).toDouble();
    m_surchargeRate = info.value("default_surcharge_rate", 0.0).toDouble();
    m_cessRate = info.value("default_cess_rate", 0.0).toDouble();

    m_expLedgerName = info.value("default_exp_ledger_name", "").toString();
    emit expLedgerNameChanged();
    m_tdsLedgerName = info.value("default_tds_ledger_name", "").toString();
    emit tdsLedgerNameChanged();

    m_incomeAmount = 0.0;
    m_previousAmount = 0.0;
    m_narration.clear();
    emit narrationChanged();
    m_nonDeductionReason.clear();
    emit nonDeductionReasonChanged();

    m_statusMessage.clear();
    m_isError = false;
    emit statusChanged();

    if (!m_selectedPartyName.isEmpty()) {
        updatePartyInfo(m_selectedPartyName);
    } else {
        m_partyBalanceText = "Date Bal. 0.00";
        emit partyBalanceTextChanged();
        recalculateTotals();
    }
}

bool TdsVoucherController::loadVoucher(int voucherId) {
    if (voucherId <= 0) return false;

    TdsModel tModel;
    auto vch = tModel.get_tds_voucher_by_id(voucherId);
    if (vch.isEmpty()) return false;

    m_editVoucherId = vch.value("id", 0).toInt();
    emit editVoucherIdChanged();
    emit isEditModeChanged();

    m_voucherNo = vch.value("voucher_no", 1).toInt();
    emit voucherNoChanged();

    QString rawDate = vch.value("voucher_date").toString();
    setVoucherDate(rawDate);

    m_tdsType = vch.value("tds_type", "RENT").toString().toUpper();
    emit tdsTypeChanged();

    m_postInBooks = vch.value("post_in_books", 1).toInt() == 1;
    emit postInBooksChanged();

    m_selectedPartyName = vch.value("ledger_name").toString().trimmed();
    emit selectedPartyNameChanged();
    m_selectedPartyId = vch.value("ledger_id", 0).toInt();
    emit selectedPartyIdChanged();

    m_incomeAmount = vch.value("income_amount", 0.0).toDouble();
    m_previousAmount = vch.value("previous_amount", 0.0).toDouble();

    m_tdsRate = vch.value("rate_tds", 10.0).toDouble();
    m_surchargeRate = vch.value("rate_surcharge", 0.0).toDouble();
    m_cessRate = vch.value("rate_cess", 0.0).toDouble();

    m_expLedgerName = vch.value("exp_ledger_name", "").toString();
    emit expLedgerNameChanged();
    m_tdsLedgerName = vch.value("tds_ledger_name", "").toString();
    emit tdsLedgerNameChanged();

    m_narration = vch.value("narration", "").toString();
    emit narrationChanged();
    m_nonDeductionReason = vch.value("non_deduction_reason", "").toString();
    emit nonDeductionReasonChanged();

    updatePartyInfo(m_selectedPartyName);
    recalculateTotals();
    return true;
}

bool TdsVoucherController::saveVoucher() {
    m_statusMessage.clear();
    m_isError = false;

    if (m_selectedPartyName.isEmpty()) {
        m_statusMessage = "Please select a Deductee / Party Ledger!";
        m_isError = true;
        emit statusChanged();
        return false;
    }

    if (m_incomeAmount <= 0.001) {
        m_statusMessage = "Please enter a valid Income Amount greater than 0!";
        m_isError = true;
        emit statusChanged();
        return false;
    }

    FinancialYearsModel fyModel;
    auto activeFy = fyModel.get_active_year();
    int fyStartYear = QDate::fromString(activeFy.value("start_date").toString(), "yyyy-MM-dd").year();
    if (fyStartYear <= 0) fyStartYear = 2026;

    if (!AccountingDateService::instance().validateDateInFy(m_voucherDate, fyStartYear)) {
        m_statusMessage = "Voucher Date is outside the active Financial Year.";
        m_isError = true;
        emit statusChanged();
        return false;
    }

    PartiesModel pModel;
    int expId = 0;
    if (!m_expLedgerName.isEmpty()) {
        auto expP = pModel.get_party_by_name(m_expLedgerName);
        expId = expP.value("id", 0).toInt();
    }

    int tdsId = 0;
    if (!m_tdsLedgerName.isEmpty()) {
        auto tdsP = pModel.get_party_by_name(m_tdsLedgerName);
        tdsId = tdsP.value("id", 0).toInt();
    }

    QVariantMap payload;
    payload["id"] = m_editVoucherId;
    payload["voucher_no"] = m_voucherNo;
    payload["voucher_date"] = m_voucherDate;
    payload["day_of_week"] = dayOfWeek();
    payload["post_in_books"] = m_postInBooks ? 1 : 0;
    payload["tds_type"] = m_tdsType;
    payload["ledger_id"] = m_selectedPartyId;
    payload["ledger_name"] = m_selectedPartyName;
    payload["income_amount"] = m_incomeAmount;
    payload["previous_amount"] = m_previousAmount;
    payload["total_for_tds"] = m_totalForTds;
    payload["narration"] = m_narration.trimmed();
    payload["rate_tds"] = m_tdsRate;
    payload["tax_amount_tds"] = m_tdsTaxAmount;
    payload["rate_surcharge"] = m_surchargeRate;
    payload["tax_amount_surcharge"] = m_surchargeTaxAmount;
    payload["rate_cess"] = m_cessRate;
    payload["tax_amount_cess"] = m_cessTaxAmount;
    payload["use_rounded_total"] = m_useRoundedTotal ? 1 : 0;
    payload["total_tax_rate"] = m_totalTaxRate;
    payload["total_tax_amount"] = m_totalTaxAmount;
    payload["net_amount"] = m_netAmount;
    payload["non_deduction_reason"] = m_nonDeductionReason.trimmed();
    payload["exp_ledger_id"] = expId;
    payload["exp_ledger_name"] = m_expLedgerName;
    payload["tds_ledger_id"] = tdsId;
    payload["tds_ledger_name"] = m_tdsLedgerName;

    TdsModel tModel;
    bool ok = tModel.save_tds_voucher(payload);

    if (ok) {
        m_statusMessage = QString("TDS Voucher #%1 saved & posted successfully!").arg(m_voucherNo);
        m_isError = false;
        emit statusChanged();
        emit voucherSaved();
        resetForm();
        return true;
    } else {
        m_statusMessage = "Failed to save TDS Voucher in database.";
        m_isError = true;
        emit statusChanged();
        return false;
    }
}
