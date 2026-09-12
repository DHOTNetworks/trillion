#include "paddy_procurement_controller.h"
#include "paddy_arrivals_model.h"
#include "financial_years_model.h"
#include "../services/accounting_date_service.h"
#include "../services/financial_math_service.h"
#include <cmath>

PaddyProcurementController::PaddyProcurementController(QObject *parent)
    : QObject(parent) {
}

void PaddyProcurementController::setArrivalDate(const QString &v) {
    QString resolved = AccountingDateService::instance().resolveDate(v);
    if (m_arrivalDate != resolved) {
        m_arrivalDate = resolved;
        emit arrivalDateChanged();
        emit dayOfWeekChanged();
    }
}

QString PaddyProcurementController::dayOfWeek() const {
    return AccountingDateService::instance().getDayOfWeek(m_arrivalDate);
}

void PaddyProcurementController::setBagCount(int b) {
    if (m_bagCount != b) {
        m_bagCount = b;
        recalculateDeductions();
    }
}

void PaddyProcurementController::setGrossWeightQtl(double w) {
    if (std::abs(m_grossWeightQtl - w) > 0.001) {
        m_grossWeightQtl = w;
        recalculateDeductions();
    }
}

void PaddyProcurementController::setMoisturePct(double m) {
    if (std::abs(m_moisturePct - m) > 0.001) {
        m_moisturePct = m;
        recalculateDeductions();
    }
}

void PaddyProcurementController::setRatePerQtl(double r) {
    if (std::abs(m_ratePerQtl - r) > 0.001) {
        m_ratePerQtl = r;
        recalculateDeductions();
    }
}

void PaddyProcurementController::setHamaliPerBag(double h) {
    if (std::abs(m_hamaliPerBag - h) > 0.001) {
        m_hamaliPerBag = h;
        recalculateDeductions();
    }
}

QString PaddyProcurementController::netAmountFmt() const {
    return FinancialMathService::instance().formatInr(m_netAmount);
}

void PaddyProcurementController::recalculateDeductions() {
    // Moisture deduction standard: benchmark 17.0%
    double moistDed = (m_grossWeightQtl * (m_moisturePct - 17.0) * 0.01);
    if (moistDed < 0.0) moistDed = 0.0;
    m_moistureDeductionQtl = std::round(moistDed * 1000.0) / 1000.0;

    m_netWeightQtl = std::max(0.0, std::round((m_grossWeightQtl - m_moistureDeductionQtl) * 1000.0) / 1000.0);
    m_hamaliTotal = FinancialMathService::instance().round2(m_bagCount * m_hamaliPerBag);

    double grossAmt = m_netWeightQtl * m_ratePerQtl;
    m_netAmount = std::max(0.0, FinancialMathService::instance().round2(grossAmt - m_hamaliTotal));

    emit calculationsChanged();
}

void PaddyProcurementController::resetForm(const QString &workingDate) {
    FinancialYearsModel fyModel;
    QString wDate = !workingDate.isEmpty() ? workingDate : fyModel.get_working_date();
    if (wDate.isEmpty()) {
        wDate = QDate::currentDate().toString("dd/MM/yyyy");
    }
    setArrivalDate(wDate);

    m_farmerName.clear();
    emit farmerNameChanged();

    m_paddyVariety = "Sona Masoori";
    emit paddyVarietyChanged();

    m_bagCount = 0;
    m_grossWeightQtl = 0.0;
    m_moisturePct = 14.0;
    m_ratePerQtl = 0.0;
    m_hamaliPerBag = 0.0;
    m_paymentStatus = "Pending";
    emit paymentStatusChanged();

    m_statusMessage.clear();
    m_isError = false;
    emit statusChanged();

    recalculateDeductions();
}

bool PaddyProcurementController::saveArrivalSlip() {
    m_statusMessage.clear();
    m_isError = false;

    if (m_farmerName.trimmed().isEmpty()) {
        m_statusMessage = "Please enter Farmer / Supplier Name.";
        m_isError = true;
        emit statusChanged();
        return false;
    }

    if (m_bagCount <= 0 && m_grossWeightQtl <= 0) {
        m_statusMessage = "Please enter valid Bags and Gross Weight.";
        m_isError = true;
        emit statusChanged();
        return false;
    }

    FinancialYearsModel fyModel;
    auto activeFy = fyModel.get_active_year();
    int fyStartYear = QDate::fromString(activeFy.value("start_date").toString(), "yyyy-MM-dd").year();
    if (fyStartYear <= 0) fyStartYear = 2026;

    if (!AccountingDateService::instance().validateDateInFy(m_arrivalDate, fyStartYear)) {
        m_statusMessage = "Arrival Date is outside the active Financial Year.";
        m_isError = true;
        emit statusChanged();
        return false;
    }

    QString isoDate = AccountingDateService::instance().toIso(m_arrivalDate);

    PaddyArrivalsModel pModel;
    bool ok = pModel.add_arrival(
        m_farmerName.trimmed(),
        m_paddyVariety,
        isoDate,
        m_bagCount,
        m_grossWeightQtl,
        m_moisturePct,
        m_ratePerQtl,
        m_hamaliPerBag,
        m_paymentStatus
    );

    if (ok) {
        m_statusMessage = "Paddy Arrival Entry recorded successfully!";
        m_isError = false;
        emit statusChanged();
        emit arrivalSaved();
        resetForm();
        return true;
    } else {
        m_statusMessage = "Failed to record Paddy Arrival Entry in database.";
        m_isError = true;
        emit statusChanged();
        return false;
    }
}
