#include "ledger_statement_model.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"
#include "../engine/ledger_pipeline.h"
#include <QDate>
#include <algorithm>
#include <cmath>

static std::pair<QString, QString> parseDateIsoAndFmt(const QString& dStr) {
    if (dStr.trimmed().isEmpty()) return {"9999-12-31", ""};
    QString s = dStr.trimmed();
    if (s.contains('-')) {
        QStringList p = s.split('-');
        if (p.size() == 3) {
            if (p[0].length() == 4) {
                int y = p[0].toInt();
                int m = p[1].toInt();
                int d = p[2].toInt();
                return {
                    QString("%1-%2-%3").arg(y, 4, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(d, 2, 10, QChar('0')),
                    QString("%1-%2-%3").arg(d, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(y, 4, 10, QChar('0'))
                };
            } else if (p[2].length() == 4) {
                int d = p[0].toInt();
                int m = p[1].toInt();
                int y = p[2].toInt();
                return {
                    QString("%1-%2-%3").arg(y, 4, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(d, 2, 10, QChar('0')),
                    QString("%1-%2-%3").arg(d, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(y, 4, 10, QChar('0'))
                };
            }
        }
    } else if (s.contains('/')) {
        QStringList p = s.split('/');
        if (p.size() == 3 && p[2].length() == 4) {
            int d = p[0].toInt();
            int m = p[1].toInt();
            int y = p[2].toInt();
            return {
                QString("%1-%2-%3").arg(y, 4, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(d, 2, 10, QChar('0')),
                QString("%1-%2-%3").arg(d, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(y, 4, 10, QChar('0'))
            };
        }
    }
    return {s, s};
}

static QString computeFyForIso(const QString& isoDate, const QString& explicitFy = "") {
    if (!explicitFy.trimmed().isEmpty()) return explicitFy.trimmed();
    FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(isoDate);
    return fy.name;
}

// ======================== LedgerStatementSideModel ========================

LedgerStatementSideModel::LedgerStatementSideModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int LedgerStatementSideModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_entries.size();
}

QVariant LedgerStatementSideModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return QVariant();
    }

    const LedgerStatementEntry& e = m_entries.at(index.row());
    switch (role) {
    case IdRole: return e.id;
    case IsSelectedRole: return e.isSelected;
    case VIsoRole: return e.vIso;
    case VDateRole: return e.vDate;
    case RefNoRole: return e.refNo;
    case VoucherNoRole: return e.voucherNo;
    case InvoiceNoRole: return e.invoiceNo;
    case VoucherTypeRole: return e.voucherType;
    case LegacyTypeRole: return e.legacyType;
    case TransTypeRole: return e.transType;
    case ParticularsRole: return e.particulars;
    case AmountRole: return e.amount;
    case AmountFmtRole: return e.amountFmt;
    case FinancialYearRole: return e.financialYear;
    case SideRole: return e.side;
    default: return QVariant();
    }
}

QHash<int, QByteArray> LedgerStatementSideModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[IsSelectedRole] = "isSelected";
    roles[VIsoRole] = "vIso";
    roles[VDateRole] = "vDate";
    roles[RefNoRole] = "refNo";
    roles[VoucherNoRole] = "voucher_no";
    roles[InvoiceNoRole] = "invoice_no";
    roles[VoucherTypeRole] = "voucher_type";
    roles[LegacyTypeRole] = "legacy_type";
    roles[TransTypeRole] = "trans_type";
    roles[ParticularsRole] = "particulars";
    roles[AmountRole] = "amount";
    roles[AmountFmtRole] = "amountFmt";
    roles[FinancialYearRole] = "financial_year";
    roles[SideRole] = "side";
    return roles;
}

void LedgerStatementSideModel::setEntries(const QVector<LedgerStatementEntry>& entries) {
    beginResetModel();
    m_entries = entries;
    endResetModel();
    emit countChanged();
    recalculateTotals();
}

void LedgerStatementSideModel::clear() {
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
    recalculateTotals();
}

void LedgerStatementSideModel::toggleSelection(int row) {
    if (row < 0 || row >= m_entries.size()) return;
    m_entries[row].isSelected = !m_entries[row].isSelected;
    QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {IsSelectedRole});
    recalculateTotals();
}

void LedgerStatementSideModel::setSelection(int row, bool selected) {
    if (row < 0 || row >= m_entries.size()) return;
    if (m_entries[row].isSelected == selected) return;
    m_entries[row].isSelected = selected;
    QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {IsSelectedRole});
    recalculateTotals();
}

void LedgerStatementSideModel::selectAll(bool selected) {
    if (m_entries.isEmpty()) return;
    for (int i = 0; i < m_entries.size(); ++i) {
        m_entries[i].isSelected = selected;
    }
    emit dataChanged(index(0), index(m_entries.size() - 1), {IsSelectedRole});
    recalculateTotals();
}

QVariantMap LedgerStatementSideModel::get(int row) const {
    if (row < 0 || row >= m_entries.size()) return QVariantMap();
    const LedgerStatementEntry& e = m_entries.at(row);
    QVariantMap m;
    m["id"] = e.id;
    m["partyId"] = e.partyId;
    m["party_id"] = e.partyId;
    m["partyName"] = e.partyName;
    m["party_name"] = e.partyName;
    m["isSelected"] = e.isSelected;
    m["vIso"] = e.vIso;
    m["vDate"] = e.vDate;
    m["date"] = e.vIso;
    m["voucher_date"] = e.vIso;
    m["refNo"] = e.refNo;
    m["voucherNo"] = e.voucherNo;
    m["voucher_no"] = e.voucherNo;
    m["invoiceNo"] = e.invoiceNo;
    m["invoice_no"] = e.invoiceNo;
    m["voucherType"] = e.voucherType;
    m["voucher_type"] = e.voucherType;
    m["legacyType"] = e.legacyType;
    m["legacy_type"] = e.legacyType;
    m["transType"] = e.transType;
    m["trans_type"] = e.transType;
    m["particulars"] = e.particulars;
    m["amount"] = e.amount;
    m["amountFmt"] = e.amountFmt;
    m["financialYear"] = e.financialYear;
    m["financial_year"] = e.financialYear;
    m["side"] = e.side;
    return m;
}

void LedgerStatementSideModel::recalculateTotals() {
    double tot = 0.0;
    double selTot = 0.0;
    for (const auto& e : m_entries) {
        tot += e.amount;
        if (e.isSelected) selTot += e.amount;
    }
    m_totalAmount = tot;
    m_totalAmountFmt = AccountingEngine::formatIndianCurrency(tot, true);
    m_selectedTotal = selTot;
    m_selectedTotalFmt = AccountingEngine::formatIndianCurrency(selTot, true);
    emit totalsChanged();
}

// ======================== LedgerStatementController ========================

LedgerStatementController::LedgerStatementController(QObject* parent)
    : QObject(parent)
{
    connect(&m_drModel, &LedgerStatementSideModel::totalsChanged, this, &LedgerStatementController::onSideTotalsChanged);
    connect(&m_crModel, &LedgerStatementSideModel::totalsChanged, this, &LedgerStatementController::onSideTotalsChanged);
}

void LedgerStatementController::setCurrentPartyName(const QString& name) {
    if (m_currentPartyName == name) return;
    m_currentPartyName = name.trimmed();
    emit currentPartyNameChanged();
    reloadData();
}

double LedgerStatementController::netBalance() const {
    return std::abs(m_drModel.totalAmount() - m_crModel.totalAmount());
}

QString LedgerStatementController::netBalanceFmt() const {
    return AccountingEngine::formatIndianCurrency(netBalance(), true);
}

QString LedgerStatementController::netBalanceType() const {
    double dr = m_drModel.totalAmount();
    double cr = m_crModel.totalAmount();
    if (std::abs(dr - cr) < 0.001) return "Nil Balance";
    if (dr > cr) return "Dr (Debit Balance)";
    return "Cr (Credit Balance)";
}

void LedgerStatementController::onSideTotalsChanged() {
    emit statementTotalsChanged();
}

void LedgerStatementController::loadPartyStatement(const QString& partyName, const QString& fromDate, const QString& toDate) {
    m_currentPartyName = partyName.trimmed();
    FiscalYearInfo fy = FiscalYearHelper::getActiveFiscalYear();
    QString fIso = FiscalYearHelper::normalizeToIso(fromDate);
    QString tIso = FiscalYearHelper::normalizeToIso(toDate);
    FiscalYearHelper::clampDateRangeToFiscalYear(fIso, tIso, fy);
    m_fromDate = fIso;
    m_toDate = tIso;
    emit currentPartyNameChanged();
    reloadData();
}

void LedgerStatementController::applyDateFilter(const QString& fromDate, const QString& toDate) {
    FiscalYearInfo fy = FiscalYearHelper::getActiveFiscalYear();
    QString fIso = FiscalYearHelper::normalizeToIso(fromDate);
    QString tIso = FiscalYearHelper::normalizeToIso(toDate);
    FiscalYearHelper::clampDateRangeToFiscalYear(fIso, tIso, fy);
    m_fromDate = fIso;
    m_toDate = tIso;
    reloadData();
}

void LedgerStatementController::refresh() {
    reloadData();
}

QVariantList LedgerStatementController::searchParties(const QString& query) const {
    return LedgerPipeline::instance().searchLedgers(query, "", 50);
}

void LedgerStatementController::reloadData() {
    m_rawDrEntries.clear();
    m_rawCrEntries.clear();

    QString cleanName = m_currentPartyName.trimmed();
    if (cleanName.isEmpty()) {
        m_drModel.clear();
        m_crModel.clear();
        emit statementTotalsChanged();
        emit statementLoaded();
        return;
    }

    PartitionedLedgerData partData = FiscalYearHelper::partitionPartyTransactions(cleanName, m_fromDate, m_toDate);
    m_fromDate = partData.effectiveFromDate;
    m_toDate = partData.effectiveToDate;
    m_rawDrEntries = partData.drEntries;
    m_rawCrEntries = partData.crEntries;

    m_drModel.setEntries(m_rawDrEntries);
    m_crModel.setEntries(m_rawCrEntries);
    emit statementTotalsChanged();
    emit statementLoaded();
}

void LedgerStatementController::applyFilterInternal() {
    reloadData();
}
