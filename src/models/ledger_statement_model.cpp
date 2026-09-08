#include "ledger_statement_model.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
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
    if (isoDate >= "2026-04-01" && isoDate <= "2027-03-31") return "FY 2026-27";
    if (isoDate >= "2025-04-01" && isoDate <= "2026-03-31") return "FY 2025-26";
    if (isoDate >= "2024-04-01" && isoDate <= "2025-03-31") return "FY 2024-25";
    if (isoDate >= "2023-04-01" && isoDate <= "2024-03-31") return "FY 2023-24";
    return "FY 2026-27";
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
    m["isSelected"] = e.isSelected;
    m["vIso"] = e.vIso;
    m["vDate"] = e.vDate;
    m["refNo"] = e.refNo;
    m["voucher_no"] = e.voucherNo;
    m["invoice_no"] = e.invoiceNo;
    m["voucher_type"] = e.voucherType;
    m["legacy_type"] = e.legacyType;
    m["trans_type"] = e.transType;
    m["particulars"] = e.particulars;
    m["amount"] = e.amount;
    m["amountFmt"] = e.amountFmt;
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
    if (!fromDate.isEmpty()) m_fromDate = fromDate.trimmed();
    if (!toDate.isEmpty()) m_toDate = toDate.trimmed();
    emit currentPartyNameChanged();
    reloadData();
}

void LedgerStatementController::applyDateFilter(const QString& fromDate, const QString& toDate) {
    auto [fIso, _fFmt] = parseDateIsoAndFmt(fromDate);
    auto [tIso, _tFmt] = parseDateIsoAndFmt(toDate);
    m_fromDate = (fromDate.trimmed().toUpper() == "ALL" || fromDate.trimmed().isEmpty()) ? "" : fIso;
    m_toDate = (toDate.trimmed().toUpper() == "ALL" || toDate.trimmed().isEmpty()) ? "" : tIso;
    applyFilterInternal();
}

void LedgerStatementController::refresh() {
    reloadData();
}

QVariantList LedgerStatementController::searchParties(const QString& query) const {
    QString q = query.trimmed();
    if (q.isEmpty()) {
        return DatabaseManager::instance().executeQuery(
            "SELECT id, legacy_id, name, group_name, party_type, phone, city, gstin, opening_balance, balance_type "
            "FROM parties ORDER BY name COLLATE NOCASE ASC LIMIT 40;"
        );
    }

    QString pattern = q;
    pattern.replace(QChar(0x00A0), '%');
    pattern.replace(' ', '%');
    QString wildcard = "%" + pattern + "%";

    return DatabaseManager::instance().executeQuery(
        "SELECT id, legacy_id, name, group_name, party_type, phone, city, gstin, opening_balance, balance_type "
        "FROM parties WHERE name LIKE ? OR alias LIKE ? OR phone LIKE ? OR city LIKE ? "
        "ORDER BY name COLLATE NOCASE ASC LIMIT 40;",
        {wildcard, wildcard, wildcard, wildcard}
    );
}

void LedgerStatementController::reloadData() {
    m_rawDrEntries.clear();
    m_rawCrEntries.clear();

    QString cleanName = m_currentPartyName.trimmed();

    QString activeFromDate = m_fromDate.isEmpty() ? AccountingEngine::getActiveFromDate() : m_fromDate;
    QString activeToDate = m_toDate.isEmpty() ? AccountingEngine::getActiveToDate() : m_toDate;
    QString activeFyName = AccountingEngine::getActiveFyLabel();

    if (activeFromDate.isEmpty() && activeToDate.isEmpty()) {
        QVariantList fyActiveRows = DatabaseManager::instance().executeQuery(
            "SELECT year_name, start_date, end_date FROM financial_years WHERE is_active = 1 LIMIT 1;"
        );
        if (!fyActiveRows.isEmpty()) {
            QVariantMap act = fyActiveRows.first().toMap();
            activeFyName = act.value("year_name").toString();
            activeFromDate = act.value("start_date").toString();
            activeToDate = act.value("end_date").toString();
        }
    }

    if (!cleanName.isEmpty()) {
        QString namePattern = cleanName;
        namePattern.replace(QChar(0x00A0), '%');
        namePattern.replace(' ', '%');
        QString wildcard = "%" + namePattern + "%";

        // Find party info from master
        QVariantList pRows = DatabaseManager::instance().executeQuery(
            "SELECT id, legacy_id, opening_balance, balance_type FROM parties WHERE name = ? OR name LIKE ? OR name LIKE ? LIMIT 1;",
            {cleanName, "%" + cleanName + "%", wildcard}
        );
        int partyId = 0;
        int legacyCode = 0;
        double initialOp = 0.0;
        QString initialOpType = "Cr";
        if (!pRows.isEmpty()) {
            partyId = pRows.first().toMap().value("id").toInt();
            legacyCode = pRows.first().toMap().value("legacy_id").toInt();
            initialOp = pRows.first().toMap().value("opening_balance").toDouble();
            initialOpType = pRows.first().toMap().value("balance_type").toString();
        }

        // 1. Calculate Opening Balance from prior transactions
        double priorDr = 0.0;
        double priorCr = 0.0;
        if (initialOpType.compare("Dr", Qt::CaseInsensitive) == 0) priorDr += initialOp;
        else priorCr += initialOp;

        if (!activeFromDate.isEmpty()) {
            QVariantList priorRows = DatabaseManager::instance().executeQuery(
                "SELECT dr_cr, SUM(amount) as total_amt FROM transactions "
                "WHERE (party_name = ? OR party_name LIKE ? OR party_name LIKE ? OR party_id = ? OR (account_code > 0 AND account_code = ?)) "
                "AND voucher_date < ? GROUP BY dr_cr;",
                {cleanName, "%" + cleanName + "%", wildcard, partyId, legacyCode, activeFromDate}
            );
            for (const auto& pr : priorRows) {
                QVariantMap m = pr.toMap();
                if (m.value("dr_cr").toString().compare("Dr", Qt::CaseInsensitive) == 0) {
                    priorDr += m.value("total_amt").toDouble();
                } else {
                    priorCr += m.value("total_amt").toDouble();
                }
            }
        }

        double netOp = priorDr - priorCr;
        if (std::abs(netOp) > 0.001) {
            auto [opIso, opFmt] = parseDateIsoAndFmt(!activeFromDate.isEmpty() ? activeFromDate : "2024-04-01");
            LedgerStatementEntry opEntry;
            opEntry.id = -1;
            opEntry.isSelected = false;
            opEntry.vIso = opIso;
            opEntry.vDate = opFmt;
            opEntry.refNo = "OP-BAL";
            opEntry.voucherNo = "OP";
            opEntry.invoiceNo = "";
            opEntry.voucherType = "OBal";
            opEntry.legacyType = "OBal";
            opEntry.transType = "OBal";
            opEntry.particulars = QString("Opening Balance (%1)").arg(netOp >= 0 ? "Dr" : "Cr");
            opEntry.amount = std::abs(netOp);
            opEntry.amountFmt = AccountingEngine::formatIndianCurrency(opEntry.amount, true);
            opEntry.financialYear = !activeFyName.isEmpty() ? activeFyName : "Opening";
            opEntry.side = (netOp >= 0) ? "Dr" : "Cr";

            if (netOp >= 0) m_rawDrEntries.append(opEntry);
            else m_rawCrEntries.append(opEntry);
        }

        // 2. Fetch all transactions
        QString sql = "SELECT id, voucher_no, voucher_date, voucher_type, trans_type, opposing_account, dr_cr, amount, invoice_no, narration, financial_year, broker_name, vehicle_no, gr_no, taxable_amount, tds_amount FROM transactions WHERE (party_name = ? OR party_name LIKE ? OR party_name LIKE ? OR party_id = ? OR (account_code > 0 AND account_code = ?))";
        QVariantList params = {cleanName, "%" + cleanName + "%", wildcard, partyId, legacyCode};
        if (!activeFromDate.isEmpty() && !activeToDate.isEmpty()) {
            sql += " AND voucher_date >= ? AND voucher_date <= ?";
            params << activeFromDate << activeToDate;
        }
        sql += " ORDER BY voucher_date ASC, CAST(voucher_no AS INTEGER) ASC, id ASC;";

        QVariantList rows = DatabaseManager::instance().executeQuery(sql, params);
        for (const auto& r : rows) {
            QVariantMap t = r.toMap();
            int vId = t.value("id").toInt();
            QString vNo = t.value("voucher_no").toString();
            QString rawType = t.value("trans_type").toString();
            QString vType = t.value("voucher_type").toString();
            QString opposing = t.value("opposing_account").toString();
            QString drCr = t.value("dr_cr").toString();
            double amt = t.value("amount").toDouble();
            QString invNo = t.value("invoice_no").toString();
            QString narr = t.value("narration").toString().trimmed();
            QString veh = t.value("vehicle_no").toString().trimmed();
            QString broker = t.value("broker_name").toString().trimmed();
            auto [isoD, fmtD] = parseDateIsoAndFmt(t.value("voucher_date").toString());
            QString fyStr = computeFyForIso(isoD, t.value("financial_year").toString());

            if (rawType == "TDS" || vType == "TDS") {
                rawType = "TDS";
                vType = "TDS";
            }
            QString displayRef = !rawType.isEmpty() ? QString("%1 %2").arg(rawType, vNo).trimmed() : QString("%1 %2").arg(vType, vNo).trimmed();
            if (displayRef.isEmpty()) displayRef = vNo;

            QString desc;
            if (rawType == "Sale" || vType == "Sales") {
                desc = QString("Sales Invoice: %1").arg(!invNo.isEmpty() ? invNo : vNo);
            } else if (rawType == "Purc" || vType == "Purchase") {
                desc = QString("Purchase Bill: %1").arg(!invNo.isEmpty() ? invNo : vNo);
            } else if (rawType == "ChRt" || rawType == "Rcpt" || vType == "Receipt") {
                desc = QString("Receipt via %1").arg(!opposing.isEmpty() ? opposing : "Bank/Cash");
            } else if (rawType == "ChPt" || rawType == "Pymt" || vType == "Payment") {
                desc = QString("Payment to %1").arg(!opposing.isEmpty() ? opposing : "Bank/Cash");
            } else if (rawType == "TDS" || vType == "TDS") {
                desc = !narr.isEmpty() ? narr : QString("TDS: %1").arg(!opposing.isEmpty() ? opposing : "TDS");
            } else if (rawType == "Jrnl" || vType == "Journal") {
                desc = QString("Journal: %1").arg(!opposing.isEmpty() ? opposing : "A/c");
            } else if (rawType == "JFrm" || vType == "J-Form") {
                desc = QString("J-Form: %1").arg(!opposing.isEmpty() ? opposing : "Paddy Purchase");
            } else {
                desc = QString("%1: %2").arg(vType, opposing);
            }

            if (!veh.isEmpty()) desc += " | Veh: " + veh;
            if (!broker.isEmpty()) desc += " | Broker: " + broker;
            if (!narr.isEmpty() && rawType != "TDS" && vType != "TDS" && !desc.contains(narr)) desc += " | " + narr;

            double tdsAmt = t.value("tds_amount").toDouble();
            if ((rawType == "Purc" || vType == "Purchase") && tdsAmt > 0.0) {
                // Gross up purchase bill on Cr side
                LedgerStatementEntry item;
                item.id = vId;
                item.isSelected = false;
                item.vIso = isoD;
                item.vDate = fmtD;
                item.refNo = displayRef;
                item.voucherNo = vNo;
                item.invoiceNo = invNo;
                item.voucherType = vType;
                item.legacyType = rawType;
                item.transType = rawType;
                item.particulars = QString("B.No. %1").arg(!invNo.isEmpty() ? invNo : vNo);
                if (!veh.isEmpty()) item.particulars += " | Veh: " + veh;
                if (!broker.isEmpty()) item.particulars += " | Broker: " + broker;
                if (!narr.isEmpty()) item.particulars += " | " + narr;
                item.amount = amt + tdsAmt;
                item.amountFmt = AccountingEngine::formatIndianCurrency(item.amount, true);
                item.financialYear = fyStr;
                item.side = "Cr";
                m_rawCrEntries.append(item);

                // Add separate TDS deduction on Dr side matching Bahi Khata
                LedgerStatementEntry tdsItem;
                tdsItem.id = vId;
                tdsItem.isSelected = false;
                tdsItem.vIso = isoD;
                tdsItem.vDate = fmtD;
                tdsItem.refNo = displayRef;
                tdsItem.voucherNo = vNo;
                tdsItem.invoiceNo = invNo;
                tdsItem.voucherType = "TDS";
                tdsItem.legacyType = "TDS";
                tdsItem.transType = "TDS";
                tdsItem.particulars = QString("T.D.S. U/S 194Q (B.No. %1)").arg(!invNo.isEmpty() ? invNo : vNo);
                tdsItem.amount = tdsAmt;
                tdsItem.amountFmt = AccountingEngine::formatIndianCurrency(tdsItem.amount, true);
                tdsItem.financialYear = fyStr;
                tdsItem.side = "Dr";
                m_rawDrEntries.append(tdsItem);
            } else {
                LedgerStatementEntry item;
                item.id = vId;
                item.isSelected = false;
                item.vIso = isoD;
                item.vDate = fmtD;
                item.refNo = displayRef;
                item.voucherNo = vNo;
                item.invoiceNo = invNo;
                item.voucherType = vType;
                item.legacyType = rawType;
                item.transType = rawType;
                item.particulars = desc;
                item.amount = amt;
                item.amountFmt = AccountingEngine::formatIndianCurrency(item.amount, true);
                item.financialYear = fyStr;
                item.side = (drCr.compare("Dr", Qt::CaseInsensitive) == 0) ? "Dr" : "Cr";

                if (drCr.compare("Dr", Qt::CaseInsensitive) == 0) {
                    m_rawDrEntries.append(item);
                } else {
                    m_rawCrEntries.append(item);
                }
            }
        }
    } else {
        // Load all transactions across all parties
        QString sql = "SELECT id, voucher_no, voucher_date, voucher_type, trans_type, party_name, opposing_account, dr_cr, amount, invoice_no, narration, financial_year, broker_name, vehicle_no, taxable_amount, tds_amount FROM transactions";
        QVariantList params;
        if (!activeFromDate.isEmpty() && !activeToDate.isEmpty()) {
            sql += " WHERE voucher_date >= ? AND voucher_date <= ?";
            params << activeFromDate << activeToDate;
        }
        sql += " ORDER BY voucher_date ASC, CAST(voucher_no AS INTEGER) ASC, id ASC;";

        QVariantList rows = DatabaseManager::instance().executeQuery(sql, params);
        for (const auto& r : rows) {
            QVariantMap t = r.toMap();
            int vId = t.value("id").toInt();
            QString vNo = t.value("voucher_no").toString();
            QString rawType = t.value("trans_type").toString();
            QString vType = t.value("voucher_type").toString();
            QString pName = t.value("party_name").toString();
            QString opposing = t.value("opposing_account").toString();
            QString drCr = t.value("dr_cr").toString();
            double amt = t.value("amount").toDouble();
            QString invNo = t.value("invoice_no").toString();
            QString narr = t.value("narration").toString().trimmed();
            QString veh = t.value("vehicle_no").toString().trimmed();
            QString broker = t.value("broker_name").toString().trimmed();
            double tdsAmt = t.value("tds_amount").toDouble();
            auto [isoD, fmtD] = parseDateIsoAndFmt(t.value("voucher_date").toString());
            QString fyStr = computeFyForIso(isoD, t.value("financial_year").toString());

            QString displayRef = !rawType.isEmpty() ? QString("%1 %2").arg(rawType, vNo).trimmed() : QString("%1 %2").arg(vType, vNo).trimmed();

            if ((rawType == "Purc" || vType == "Purchase") && tdsAmt > 0.0) {
                QString desc = QString("[%1] B.No. %2").arg(pName, !invNo.isEmpty() ? invNo : vNo);
                if (!veh.isEmpty()) desc += " | Veh: " + veh;
                if (!broker.isEmpty()) desc += " | Broker: " + broker;
                if (!narr.isEmpty()) desc += " | " + narr;

                LedgerStatementEntry item;
                item.id = vId; item.isSelected = false; item.vIso = isoD; item.vDate = fmtD;
                item.refNo = displayRef; item.voucherNo = vNo; item.invoiceNo = invNo;
                item.voucherType = vType; item.legacyType = rawType; item.transType = rawType;
                item.particulars = desc; item.amount = amt + tdsAmt;
                item.amountFmt = AccountingEngine::formatIndianCurrency(item.amount, true);
                item.financialYear = fyStr; item.side = "Cr";
                m_rawCrEntries.append(item);

                LedgerStatementEntry tdsItem;
                tdsItem.id = vId; tdsItem.isSelected = false; tdsItem.vIso = isoD; tdsItem.vDate = fmtD;
                tdsItem.refNo = displayRef; tdsItem.voucherNo = vNo; tdsItem.invoiceNo = invNo;
                tdsItem.voucherType = "TDS"; tdsItem.legacyType = "TDS"; tdsItem.transType = "TDS";
                tdsItem.particulars = QString("[%1] T.D.S. U/S 194Q (B.No. %2)").arg(pName, !invNo.isEmpty() ? invNo : vNo);
                tdsItem.amount = tdsAmt;
                tdsItem.amountFmt = AccountingEngine::formatIndianCurrency(tdsItem.amount, true);
                tdsItem.financialYear = fyStr; tdsItem.side = "Dr";
                m_rawDrEntries.append(tdsItem);
            } else {
                QString desc = QString("[%1] %2").arg(pName, !opposing.isEmpty() ? opposing : vType);
                if (!veh.isEmpty()) desc += " | Veh: " + veh;
                if (!broker.isEmpty()) desc += " | Broker: " + broker;
                if (!narr.isEmpty()) desc += " | " + narr;

                LedgerStatementEntry item;
                item.id = vId; item.isSelected = false; item.vIso = isoD; item.vDate = fmtD;
                item.refNo = displayRef; item.voucherNo = vNo; item.invoiceNo = invNo;
                item.voucherType = vType; item.legacyType = rawType; item.transType = rawType;
                item.particulars = desc; item.amount = amt;
                item.amountFmt = AccountingEngine::formatIndianCurrency(item.amount, true);
                item.financialYear = fyStr; item.side = (drCr.compare("Dr", Qt::CaseInsensitive) == 0) ? "Dr" : "Cr";

                if (drCr.compare("Dr", Qt::CaseInsensitive) == 0) {
                    m_rawDrEntries.append(item);
                } else {
                    m_rawCrEntries.append(item);
                }
            }
        }
    }

    auto sortFn = [](const LedgerStatementEntry& a, const LedgerStatementEntry& b) {
        if (a.vIso != b.vIso) return a.vIso < b.vIso;
        int numA = a.voucherNo.toInt();
        int numB = b.voucherNo.toInt();
        if (numA != numB) return numA < numB;
        return a.id < b.id;
    };

    std::sort(m_rawDrEntries.begin(), m_rawDrEntries.end(), sortFn);
    std::sort(m_rawCrEntries.begin(), m_rawCrEntries.end(), sortFn);

    applyFilterInternal();
    emit statementLoaded();
}

void LedgerStatementController::applyFilterInternal() {
    QVector<LedgerStatementEntry> drFiltered;
    QVector<LedgerStatementEntry> crFiltered;

    for (const auto& e : m_rawDrEntries) {
        if (!m_fromDate.isEmpty() && e.vIso < m_fromDate) continue;
        if (!m_toDate.isEmpty() && e.vIso > m_toDate) continue;
        drFiltered.append(e);
    }

    for (const auto& e : m_rawCrEntries) {
        if (!m_fromDate.isEmpty() && e.vIso < m_fromDate) continue;
        if (!m_toDate.isEmpty() && e.vIso > m_toDate) continue;
        crFiltered.append(e);
    }

    m_drModel.setEntries(drFiltered);
    m_crModel.setEntries(crFiltered);
    emit statementTotalsChanged();
}
