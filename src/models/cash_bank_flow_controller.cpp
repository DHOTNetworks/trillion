#include "cash_bank_flow_controller.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"
#include "account_classifier.h"
#include <QDate>
#include <QRegularExpression>
#include <cmath>
#include <algorithm>
#include <map>
#include <set>

namespace MahadevERP {

CashBankFlowController::CashBankFlowController(QObject* parent)
    : QObject(parent)
{
    FiscalYearInfo fy = FiscalYearHelper::getActiveFiscalYear();
    m_fromDate = fy.startDate;
    m_toDate = fy.endDate;
}

void CashBankFlowController::setStatementType(FlowStatementType type) {
    if (m_statementType != type) {
        m_statementType = type;
        emit statementTypeChanged(type);
        recalculate();
    }
}

QString CashBankFlowController::statementTitle() const {
    switch (m_statementType) {
        case FlowStatementType::CashFlow:
            return "Cash Flow Statement";
        case FlowStatementType::BankFlow:
            return "Bank Flow Statement";
        case FlowStatementType::JointFlow:
            return "Cash & Bank Flow Statement";
    }
    return "Cash & Bank Flow Statement";
}

bool CashBankFlowController::isCashLedger(const QString& name, const QString& groupName) const {
    QString n = name.trimmed().toLower();
    QString g = groupName.trimmed().toLower();
    if (g == "cash-in-hand" || g == "cash in hand" || g == "cash") return true;
    if (n == "cash" || n == "cash a/c" || n == "cash in hand" || n == "cash-in-hand" || n == "petty cash") return true;
    return false;
}

bool CashBankFlowController::isBankLedger(const QString& name, const QString& groupName) const {
    if (isCashLedger(name, groupName)) return false;
    QString g = groupName.trimmed().toLower();
    if (g == "bank(s) a/c" || g == "bank accounts" || g == "bank account" || g == "secured loans") return true;
    return false;
}

void CashBankFlowController::loadData(const QString& fromDate, const QString& toDate, const QString& filterText) {
    m_fromDate = fromDate.trimmed().isEmpty() ? FiscalYearHelper::getActiveFiscalYear().startDate : fromDate.trimmed();
    m_toDate = toDate.trimmed().isEmpty() ? FiscalYearHelper::getActiveFiscalYear().endDate : toDate.trimmed();
    m_filterText = filterText.trimmed();
    recalculate();
}

void CashBankFlowController::recalculate() {
    m_partyRows.clear();
    m_bankCashRows.clear();
    m_summary = FlowStatementSummary();

    auto& db = DatabaseManager::instance();

    // 1. Fetch all parties and identify Cash / Bank accounts deterministically via 4-tier lineage
    QVariantList partyList = db.executeQuery(
        "SELECT p.id, p.name, p.group_name, p.group_code, p.opening_balance, p.balance_type, "
        "       COALESCE(g.code1st, 0) AS c1, COALESCE(g.code2nd, 0) AS c2, COALESCE(g.code3rd, 0) AS c3, COALESCE(g.code4th, 0) AS c4 "
        "FROM parties p "
        "LEFT JOIN account_groups g ON (p.group_id = g.id OR (p.group_code > 0 AND p.group_code = g.code1st) OR p.group_name = g.name) "
        "ORDER BY p.name ASC;"
    );
    std::map<QString, int> partyNameToId;
    std::map<QString, QString> partyNameToGroup;
    std::map<QString, double> partyOpBalance;
    std::map<QString, QString> partyOpType;
    std::set<QString> cashAccountNames;
    std::set<QString> bankAccountNames;

    for (const auto& pVar : partyList) {
        QVariantMap p = pVar.toMap();
        int pId = p.value("id").toInt();
        QString pName = p.value("name").toString().trimmed();
        QString gName = p.value("group_name").toString().trimmed();
        double opBal = p.value("opening_balance").toDouble();
        QString opT = p.value("balance_type").toString().trimmed();
        if (opT.isEmpty()) opT = "Dr";

        partyNameToId[pName] = pId;
        partyNameToGroup[pName] = gName;
        partyOpBalance[pName] = opBal;
        partyOpType[pName] = opT;

        int c1 = p.value("c1").toInt();
        int c2 = p.value("c2").toInt();
        int c3 = p.value("c3").toInt();
        int c4 = p.value("c4").toInt();

        bool isCash = AccountClassifier::isDescendantOf(c1, c2, c3, c4, StandardGroupCode::CashInHand) || isCashLedger(pName, gName);
        bool isBank = !isCash && (AccountClassifier::isDescendantOf(c1, c2, c3, c4, StandardGroupCode::BankAccounts) ||
                                  AccountClassifier::isDescendantOf(c1, c2, c3, c4, StandardGroupCode::SecuredLoansCC) ||
                                  isBankLedger(pName, gName));

        if (isCash) {
            cashAccountNames.insert(pName);
        } else if (isBank) {
            bankAccountNames.insert(pName);
        }
    }

    // Default "Cash" if not in master
    if (cashAccountNames.empty()) {
        cashAccountNames.insert("Cash");
    }

    // 2. Fetch all transactions matching statement filters
    QStringList whereConditions;
    QVariantList params;

    // Filter by Voucher Types and Trans Types
    if (m_statementType == FlowStatementType::CashFlow) {
        // Cash Flow: trans_type in ('Pymt', 'Rcpt') OR trans_type in ('Payment', 'Receipt') with cash
        whereConditions << "(trans_type IN ('Pymt', 'Rcpt', 'PYMT', 'RCPT', 'PY', 'RC') OR opposing_account IN (" +
                           [&]() {
                               QStringList l;
                               for (const auto& c : cashAccountNames) {
                                   l << "?";
                                   params << c;
                               }
                               return l.join(",");
                           }() +
                           ") OR party_name IN (" +
                           [&]() {
                               QStringList l;
                               for (const auto& c : cashAccountNames) {
                                   l << "?";
                                   params << c;
                               }
                               return l.join(",");
                           }() +
                           "))";
    } else if (m_statementType == FlowStatementType::BankFlow) {
        // Bank Flow: trans_type in ('ChPt', 'ChRt', 'CHPT', 'CHRT') OR bank accounts involved
        whereConditions << "(trans_type IN ('ChPt', 'ChRt', 'CHPT', 'CHRT') OR opposing_account IN (" +
                           [&]() {
                               QStringList l;
                               for (const auto& b : bankAccountNames) {
                                   l << "?";
                                   params << b;
                               }
                               if (l.isEmpty()) { l << "'__NONE__'"; }
                               return l.join(",");
                           }() +
                           ") OR party_name IN (" +
                           [&]() {
                               QStringList l;
                               for (const auto& b : bankAccountNames) {
                                   l << "?";
                                   params << b;
                               }
                               if (l.isEmpty()) { l << "'__NONE__'"; }
                               return l.join(",");
                           }() +
                           "))";
    } else {
        // Joint Flow: All Cash + Bank transactions
        whereConditions << "(trans_type IN ('Pymt', 'Rcpt', 'PYMT', 'RCPT', 'PY', 'RC', 'ChPt', 'ChRt', 'CHPT', 'CHRT') OR "
                           "opposing_account IN (" +
                           [&]() {
                               QStringList l;
                               for (const auto& c : cashAccountNames) { l << "?"; params << c; }
                               for (const auto& b : bankAccountNames) { l << "?"; params << b; }
                               if (l.isEmpty()) { l << "'__NONE__'"; }
                               return l.join(",");
                           }() +
                           ") OR party_name IN (" +
                           [&]() {
                               QStringList l;
                               for (const auto& c : cashAccountNames) { l << "?"; params << c; }
                               for (const auto& b : bankAccountNames) { l << "?"; params << b; }
                               if (l.isEmpty()) { l << "'__NONE__'"; }
                               return l.join(",");
                           }() +
                           "))";
    }

    // Date range
    whereConditions << "voucher_date >= ? AND voucher_date <= ?";
    params << m_fromDate << m_toDate;

    QString sql = "SELECT voucher_no, voucher_date, voucher_type, trans_type, party_id, party_name, opposing_account, dr_cr, amount "
                  "FROM transactions WHERE " + whereConditions.join(" AND ") + " ORDER BY party_name ASC, voucher_date ASC;";

    QVariantList txRows = db.executeQuery(sql, params);

    // 3. Aggregate Receipts and Payments per Party
    struct PartyFlowAccumulator {
        double receipts = 0.0;
        double payments = 0.0;
        int partyId = 0;
    };
    std::map<QString, PartyFlowAccumulator> partyFlowMap;

    // Track total Cash and Bank inflows and outflows separately
    double totalCashReceipts = 0.0;
    double totalCashPayments = 0.0;
    std::map<QString, double> bankReceiptsMap;
    std::map<QString, double> bankPaymentsMap;

    for (const auto& txVar : txRows) {
        QVariantMap tx = txVar.toMap();
        QString partyName = tx.value("party_name").toString().trimmed();
        QString oppName = tx.value("opposing_account").toString().trimmed();
        QString tType = tx.value("trans_type").toString().trimmed();
        QString drCr = tx.value("dr_cr").toString().trimmed();
        double amt = tx.value("amount").toDouble();
        int pId = tx.value("party_id").toInt();

        bool isPartyCash = cashAccountNames.count(partyName) > 0;
        bool isPartyBank = bankAccountNames.count(partyName) > 0;
        bool isOppCash = cashAccountNames.count(oppName) > 0;
        bool isOppBank = bankAccountNames.count(oppName) > 0;

        // Skip direct Bank-Cash contra double count if needed, or track properly
        if (isPartyCash) {
            if (drCr == "Dr") totalCashReceipts += amt;
            else totalCashPayments += amt;
            continue;
        }
        if (isPartyBank) {
            if (drCr == "Dr") bankReceiptsMap[partyName] += amt;
            else bankPaymentsMap[partyName] += amt;
            continue;
        }

        // For ordinary party/ledger:
        // In Payment voucher (Pymt / ChPt): Party is Dr (Payment received by party from business) -> Payments column
        // In Receipt voucher (Rcpt / ChRt): Party is Cr (Receipt from party into business) -> Receipts column
        if (tType.compare("Pymt", Qt::CaseInsensitive) == 0 || tType.compare("ChPt", Qt::CaseInsensitive) == 0 ||
            tType.compare("Payment", Qt::CaseInsensitive) == 0 || (drCr == "Dr" && (isOppCash || isOppBank))) {
            partyFlowMap[partyName].payments += amt;
            if (partyFlowMap[partyName].partyId == 0) partyFlowMap[partyName].partyId = pId;
        } else if (tType.compare("Rcpt", Qt::CaseInsensitive) == 0 || tType.compare("ChRt", Qt::CaseInsensitive) == 0 ||
                   tType.compare("Receipt", Qt::CaseInsensitive) == 0 || (drCr == "Cr" && (isOppCash || isOppBank))) {
            partyFlowMap[partyName].receipts += amt;
            if (partyFlowMap[partyName].partyId == 0) partyFlowMap[partyName].partyId = pId;
        } else {
            // Default handling by Dr / Cr
            if (drCr == "Dr") {
                partyFlowMap[partyName].payments += amt;
            } else {
                partyFlowMap[partyName].receipts += amt;
            }
            if (partyFlowMap[partyName].partyId == 0) partyFlowMap[partyName].partyId = pId;
        }
    }

    // 4. Calculate Opening Balances as of fromDate for each party
    auto calculatePartyOpBal = [&](const QString& pName) -> double {
        double baseOp = partyOpBalance.count(pName) ? partyOpBalance[pName] : 0.0;
        QString baseType = partyOpType.count(pName) ? partyOpType[pName] : "Dr";
        double netOp = (baseType == "Cr") ? -baseOp : baseOp;

        // Prior transactions before m_fromDate
        QVariant priorDrVar = db.executeScalar(
            "SELECT COALESCE(SUM(amount), 0.0) FROM transactions WHERE party_name = ? AND voucher_date < ? AND dr_cr = 'Dr';",
            {pName, m_fromDate}
        );
        QVariant priorCrVar = db.executeScalar(
            "SELECT COALESCE(SUM(amount), 0.0) FROM transactions WHERE party_name = ? AND voucher_date < ? AND dr_cr = 'Cr';",
            {pName, m_fromDate}
        );
        double priorDr = priorDrVar.isValid() ? priorDrVar.toDouble() : 0.0;
        double priorCr = priorCrVar.isValid() ? priorCrVar.toDouble() : 0.0;

        return netOp + (priorDr - priorCr);
    };

    // Calculate Cash Account overall balance
    double cashOpBal = 0.0;
    for (const auto& cName : cashAccountNames) {
        cashOpBal += calculatePartyOpBal(cName);
    }
    double cashClBal = cashOpBal + (totalCashReceipts - totalCashPayments);

    // Calculate Bank Account balances
    std::map<QString, double> bankOpBalMap;
    std::map<QString, double> bankClBalMap;
    double totalBankClBal = 0.0;

    for (const auto& bName : bankAccountNames) {
        double bOp = calculatePartyOpBal(bName);
        double bRec = bankReceiptsMap[bName];
        double bPay = bankPaymentsMap[bName];
        double bCl = bOp + (bRec - bPay);
        bankOpBalMap[bName] = bOp;
        bankClBalMap[bName] = bCl;
        totalBankClBal += bCl;
    }

    // 5. Build FlowLedgerRow records
    double sumOp = 0.0;
    double sumReceipts = 0.0;
    double sumPayments = 0.0;
    double sumCl = 0.0;

    QString fQuery = m_filterText.toLower();

    for (const auto& pair : partyFlowMap) {
        const QString& pName = pair.first;
        const auto& flow = pair.second;

        if (!fQuery.isEmpty() && !pName.toLower().contains(fQuery)) {
            continue;
        }

        FlowLedgerRow row;
        row.partyId = flow.partyId > 0 ? flow.partyId : (partyNameToId.count(pName) ? partyNameToId[pName] : 0);
        row.ledgerName = pName;
        row.receipts = flow.receipts;
        row.payments = flow.payments;
        row.opBalance = calculatePartyOpBal(pName);
        row.opType = (row.opBalance >= 0.0) ? "Dr" : "Cr";
        row.clBalance = row.opBalance + (row.receipts - row.payments);
        row.clType = (row.clBalance >= 0.0) ? "Dr" : "Cr";
        row.isBankOrCashAccount = false;

        sumOp += row.opBalance;
        sumReceipts += row.receipts;
        sumPayments += row.payments;

        m_partyRows.append(row);
    }

    // 6. Build Bottom Rows for Cash / Bank
    if (m_statementType == FlowStatementType::CashFlow || m_statementType == FlowStatementType::JointFlow) {
        FlowLedgerRow cashRow;
        cashRow.partyId = partyNameToId.count("Cash") ? partyNameToId["Cash"] : 0;
        cashRow.ledgerName = "Cash";
        cashRow.opBalance = cashOpBal;
        cashRow.receipts = totalCashReceipts;
        cashRow.payments = totalCashPayments;
        cashRow.clBalance = (cashClBal >= 0.0) ? cashClBal : -cashClBal;
        cashRow.clType = (cashClBal >= 0.0) ? "Dr" : "Cr";
        cashRow.isBankOrCashAccount = true;
        m_bankCashRows.append(cashRow);
    }

    if (m_statementType == FlowStatementType::BankFlow || m_statementType == FlowStatementType::JointFlow) {
        for (const auto& bName : bankAccountNames) {
            double bRec = bankReceiptsMap[bName];
            double bPay = bankPaymentsMap[bName];
            double bCl = bankClBalMap[bName];

            // Only show bank accounts that had transactions or non-zero balance
            if (bRec > 0.0 || bPay > 0.0 || std::abs(bCl) > 0.01) {
                FlowLedgerRow bankRow;
                bankRow.partyId = partyNameToId.count(bName) ? partyNameToId[bName] : 0;
                bankRow.ledgerName = bName;
                bankRow.opBalance = bankOpBalMap[bName];
                bankRow.receipts = bRec;
                bankRow.payments = bPay;
                bankRow.clBalance = (bCl >= 0.0) ? bCl : -bCl;
                bankRow.clType = (bCl >= 0.0) ? "Dr" : "Cr";
                bankRow.isBankOrCashAccount = true;
                m_bankCashRows.append(bankRow);
            }
        }
    }

    // Calculate Grand Total closing balance
    if (m_statementType == FlowStatementType::CashFlow) {
        sumCl = cashClBal;
    } else if (m_statementType == FlowStatementType::BankFlow) {
        sumCl = totalBankClBal;
    } else {
        sumCl = cashClBal + totalBankClBal;
    }

    m_summary.totalOpBalance = sumOp;
    m_summary.totalReceipts = sumReceipts;
    m_summary.totalPayments = sumPayments;
    m_summary.totalClBalance = sumCl;
    m_summary.cashClosingBalance = cashClBal;
    m_summary.bankClosingBalance = totalBankClBal;
    m_summary.rowCount = m_partyRows.size();

    emit dataChanged();
}

QVariantList CashBankFlowController::getPartyRowsAsVariantList() const {
    QVariantList list;
    for (const auto& r : m_partyRows) {
        QVariantMap m;
        m["partyId"] = r.partyId;
        m["ledgerName"] = r.ledgerName;
        m["opBalance"] = r.opBalance;
        m["opType"] = r.opType;
        m["receipts"] = r.receipts;
        m["payments"] = r.payments;
        m["clBalance"] = r.clBalance;
        m["clType"] = r.clType;
        list.append(m);
    }
    return list;
}

QVariantList CashBankFlowController::getBankCashRowsAsVariantList() const {
    QVariantList list;
    for (const auto& r : m_bankCashRows) {
        QVariantMap m;
        m["partyId"] = r.partyId;
        m["ledgerName"] = r.ledgerName;
        m["opBalance"] = r.opBalance;
        m["receipts"] = r.receipts;
        m["payments"] = r.payments;
        m["clBalance"] = r.clBalance;
        m["clType"] = r.clType;
        list.append(m);
    }
    return list;
}

QVariantMap CashBankFlowController::getSummaryAsVariantMap() const {
    QVariantMap m;
    m["totalOpBalance"] = m_summary.totalOpBalance;
    m["totalReceipts"] = m_summary.totalReceipts;
    m["totalPayments"] = m_summary.totalPayments;
    m["totalClBalance"] = m_summary.totalClBalance;
    m["cashClosingBalance"] = m_summary.cashClosingBalance;
    m["bankClosingBalance"] = m_summary.bankClosingBalance;
    m["rowCount"] = m_summary.rowCount;
    return m;
}

QString CashBankFlowController::generateCsvReport() const {
    QString csv;
    csv += QString("%1 (%2 To %3)\n").arg(statementTitle(), FiscalYearHelper::formatDisplayDate(m_fromDate), FiscalYearHelper::formatDisplayDate(m_toDate));
    csv += "Ledger Name,Op.Balance,Receipts,Payments,Cl.Balance\n";

    for (const auto& r : m_partyRows) {
        QString recStr = r.receipts > 0.0 ? QString::number(r.receipts, 'f', 2) : "--";
        QString payStr = r.payments > 0.0 ? QString::number(r.payments, 'f', 2) : "--";
        csv += QString("\"%1\",%2,%3,%4,%5\n")
            .arg(r.ledgerName)
            .arg(QString::number(r.opBalance, 'f', 2))
            .arg(recStr)
            .arg(payStr)
            .arg(QString::number(r.clBalance, 'f', 2));
    }

    for (const auto& r : m_bankCashRows) {
        csv += QString("\"%1\",,,,\"%2\"\n")
            .arg(r.ledgerName)
            .arg(QString::number(r.clBalance, 'f', 2));
    }

    csv += QString("Grand Total,%1,%2,%3,%4\n")
        .arg(QString::number(m_summary.totalOpBalance, 'f', 2))
        .arg(QString::number(m_summary.totalReceipts, 'f', 2))
        .arg(QString::number(m_summary.totalPayments, 'f', 2))
        .arg(QString::number(m_summary.totalClBalance, 'f', 2));

    return csv;
}

} // namespace MahadevERP
