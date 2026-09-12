#include "cheque_voucher_controller.h"
#include "vouchers_model.h"
#include "financial_years_model.h"
#include "../services/accounting_date_service.h"
#include "../services/financial_math_service.h"
#include <cmath>

ChequeRowsModel::ChequeRowsModel(QObject *parent)
    : QAbstractListModel(parent) {
}

int ChequeRowsModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_rows.size();
}

QVariant ChequeRowsModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return QVariant();
    }

    const auto &item = m_rows.at(index.row());
    switch (role) {
    case DrCrRole: return item.drcr;
    case LedgerNameRole: return item.ledgerName;
    case DebitAmtRole: return item.debitAmt > 0.0001 ? QString::number(item.debitAmt, 'f', 2) : "";
    case CreditAmtRole: return item.creditAmt > 0.0001 ? QString::number(item.creditAmt, 'f', 2) : "";
    case RefNoRole: return item.refNo;
    default: return QVariant();
    }
}

bool ChequeRowsModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return false;
    }

    auto &item = m_rows[index.row()];
    switch (role) {
    case DrCrRole:
        item.drcr = value.toString();
        break;
    case LedgerNameRole:
        item.ledgerName = value.toString();
        break;
    case DebitAmtRole:
        item.debitAmt = value.toDouble();
        break;
    case CreditAmtRole:
        item.creditAmt = value.toDouble();
        break;
    case RefNoRole:
        item.refNo = value.toString();
        break;
    default:
        return false;
    }

    emit dataChanged(index, index, {role});
    emit rowsChanged();
    return true;
}

QHash<int, QByteArray> ChequeRowsModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[DrCrRole] = "drcr";
    roles[LedgerNameRole] = "ledgerName";
    roles[DebitAmtRole] = "debitAmt";
    roles[CreditAmtRole] = "creditAmt";
    roles[RefNoRole] = "refNo";
    return roles;
}

void ChequeRowsModel::appendRow(const QString &drcr, const QString &ledgerName,
                                double debitAmt, double creditAmt, const QString &refNo) {
    beginInsertRows(QModelIndex(), m_rows.size(), m_rows.size());
    m_rows.append({drcr, ledgerName, debitAmt, creditAmt, refNo});
    endInsertRows();
    emit rowsChanged();
}

void ChequeRowsModel::removeRowAt(int index) {
    if (index < 0 || index >= m_rows.size()) return;
    beginRemoveRows(QModelIndex(), index, index);
    m_rows.removeAt(index);
    endRemoveRows();
    emit rowsChanged();
}

void ChequeRowsModel::clear() {
    beginResetModel();
    m_rows.clear();
    endResetModel();
    emit rowsChanged();
}

QVariantMap ChequeRowsModel::getRow(int index) const {
    QVariantMap map;
    if (index < 0 || index >= m_rows.size()) return map;
    const auto &item = m_rows.at(index);
    map["drcr"] = item.drcr;
    map["ledgerName"] = item.ledgerName;
    map["debitAmt"] = item.debitAmt;
    map["creditAmt"] = item.creditAmt;
    map["refNo"] = item.refNo;
    return map;
}

void ChequeRowsModel::setRowProperty(int index, const QString &property, const QVariant &value) {
    if (index < 0 || index >= m_rows.size()) return;
    auto &item = m_rows[index];
    int role = -1;

    if (property == "drcr") { item.drcr = value.toString(); role = DrCrRole; }
    else if (property == "ledgerName") { item.ledgerName = value.toString(); role = LedgerNameRole; }
    else if (property == "debitAmt") {
        item.debitAmt = value.typeId() == QMetaType::QString ? value.toString().toDouble() : value.toDouble();
        role = DebitAmtRole;
    }
    else if (property == "creditAmt") {
        item.creditAmt = value.typeId() == QMetaType::QString ? value.toString().toDouble() : value.toDouble();
        role = CreditAmtRole;
    }
    else if (property == "refNo") { item.refNo = value.toString(); role = RefNoRole; }

    if (role != -1) {
        QModelIndex idx = this->index(index, 0);
        emit dataChanged(idx, idx, {role});
        emit rowsChanged();
    }
}

QVariantList ChequeRowsModel::toVariantList() const {
    QVariantList list;
    for (const auto &item : m_rows) {
        QVariantMap map;
        map["drcr"] = item.drcr;
        map["ledgerName"] = item.ledgerName;
        map["debitAmt"] = item.debitAmt;
        map["creditAmt"] = item.creditAmt;
        map["refNo"] = item.refNo;
        list.append(map);
    }
    return list;
}

// -------------------------------------------------------------
// ChequeVoucherController
// -------------------------------------------------------------
ChequeVoucherController::ChequeVoucherController(QObject *parent)
    : QObject(parent) {
    connect(&m_rowsModel, &ChequeRowsModel::rowsChanged, this, &ChequeVoucherController::recalculateTotals);
}

void ChequeVoucherController::setVoucherMode(const QString &m) {
    if (m_voucherMode != m) {
        m_voucherMode = m;
        emit voucherModeChanged();
        resetForm();
    }
}

void ChequeVoucherController::setVoucherDate(const QString &v) {
    QString resolved = AccountingDateService::instance().resolveDate(v);
    if (m_voucherDate != resolved) {
        m_voucherDate = resolved;
        emit voucherDateChanged();
        emit dayOfWeekChanged();
    }
}

QString ChequeVoucherController::dayOfWeek() const {
    return AccountingDateService::instance().getDayOfWeek(m_voucherDate);
}

QString ChequeVoucherController::totalDebitFmt() const {
    return FinancialMathService::instance().formatInr(m_totalDebit);
}

QString ChequeVoucherController::totalCreditFmt() const {
    return FinancialMathService::instance().formatInr(m_totalCredit);
}

void ChequeVoucherController::resetForm(const QString &workingDate) {
    VouchersModel vModel;
    QString prefix = (m_voucherMode == "Receipt" || m_voucherMode == "ChRt") ? "ChRt" : "ChPt";
    m_voucherNo = vModel.get_next_voucher_no(prefix);
    emit voucherNoChanged();

    FinancialYearsModel fyModel;
    QString wDate = !workingDate.isEmpty() ? workingDate : fyModel.get_working_date();
    if (wDate.isEmpty()) {
        wDate = QDate::currentDate().toString("dd/MM/yyyy");
    }
    setVoucherDate(wDate);

    m_narration.clear();
    emit narrationChanged();

    m_rowsModel.clear();
    m_rowsModel.appendRow("Dr", "", 0.0, 0.0, "");
    m_rowsModel.appendRow("Cr", "", 0.0, 0.0, "");

    m_statusMessage.clear();
    m_isError = false;
    emit statusChanged();

    recalculateTotals();
}

void ChequeVoucherController::addNewRow() {
    QString defaultType = "Cr";
    if (m_rowsModel.count() > 0) {
        auto lastRow = m_rowsModel.getRow(m_rowsModel.count() - 1);
        defaultType = lastRow.value("drcr").toString() == "Dr" ? "Cr" : "Dr";
    }
    m_rowsModel.appendRow(defaultType, "", 0.0, 0.0, "");
    recalculateTotals();
}

void ChequeVoucherController::removeRow(int idx) {
    if (m_rowsModel.count() > 2) {
        m_rowsModel.removeRowAt(idx);
        recalculateTotals();
    }
}

void ChequeVoucherController::recalculateTotals() {
    double sumDr = 0.0;
    double sumCr = 0.0;

    for (int i = 0; i < m_rowsModel.count(); ++i) {
        auto r = m_rowsModel.getRow(i);
        if (r.value("drcr").toString() == "Dr") {
            sumDr += r.value("debitAmt").toDouble();
        } else {
            sumCr += r.value("creditAmt").toDouble();
        }
    }

    m_totalDebit = FinancialMathService::instance().round2(sumDr);
    m_totalCredit = FinancialMathService::instance().round2(sumCr);
    emit totalsChanged();
}

bool ChequeVoucherController::saveVoucher() {
    m_statusMessage.clear();
    m_isError = false;

    if (m_rowsModel.count() < 2) {
        m_statusMessage = "Please enter at least two transaction rows.";
        m_isError = true;
        emit statusChanged();
        return false;
    }

    if (std::abs(m_totalDebit - m_totalCredit) > 0.01) {
        m_statusMessage = QString("Total Debit (%1) does not equal Total Credit (%2).")
                              .arg(totalDebitFmt(), totalCreditFmt());
        m_isError = true;
        emit statusChanged();
        return false;
    }

    if (m_totalDebit <= 0.001) {
        m_statusMessage = "Please enter a valid Voucher Amount.";
        m_isError = true;
        emit statusChanged();
        return false;
    }

    for (int i = 0; i < m_rowsModel.count(); ++i) {
        auto r = m_rowsModel.getRow(i);
        if (r.value("ledgerName").toString().trimmed().isEmpty()) {
            m_statusMessage = QString("Please select a Ledger Account for Row %1.").arg(i + 1);
            m_isError = true;
            emit statusChanged();
            return false;
        }
    }

    // Validate date against FY
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

    QString vchType = (m_voucherMode == "Receipt" || m_voucherMode == "ChRt") ? "Cheque Receipt" : "Cheque Payment";
    QString drParty;
    QString crParty;
    QString chqNo;

    for (int i = 0; i < m_rowsModel.count(); ++i) {
        auto r = m_rowsModel.getRow(i);
        if (r.value("drcr").toString() == "Dr" && drParty.isEmpty()) {
            drParty = r.value("ledgerName").toString().trimmed();
        }
        if (r.value("drcr").toString() == "Cr" && crParty.isEmpty()) {
            crParty = r.value("ledgerName").toString().trimmed();
        }
        if (!r.value("refNo").toString().trimmed().isEmpty() && chqNo.isEmpty()) {
            chqNo = r.value("refNo").toString().trimmed();
        }
    }

    VouchersModel vModel;
    bool ok = vModel.add_cheque_voucher(vchType, drParty, crParty, m_totalDebit, chqNo, m_narration.trimmed(), m_voucherDate);

    if (ok) {
        m_statusMessage = QString("%1 %2 saved & posted successfully!").arg(vchType, m_voucherNo);
        m_isError = false;
        emit statusChanged();
        emit voucherSaved();
        resetForm();
        return true;
    } else {
        m_statusMessage = "Failed to save Cheque Voucher in database.";
        m_isError = true;
        emit statusChanged();
        return false;
    }
}
