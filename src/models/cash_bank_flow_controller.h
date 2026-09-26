#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QVariantMap>
#include <QVariantList>

namespace MahadevERP {

enum class FlowStatementType {
    CashFlow,
    BankFlow,
    JointFlow
};

struct FlowLedgerRow {
    int partyId = 0;
    QString ledgerName;
    double opBalance = 0.0;
    QString opType = "Dr";
    double receipts = 0.0;
    double payments = 0.0;
    double clBalance = 0.0;
    QString clType = "Dr";
    bool isBankOrCashAccount = false;
};

struct FlowStatementSummary {
    double totalOpBalance = 0.0;
    double totalReceipts = 0.0;
    double totalPayments = 0.0;
    double totalClBalance = 0.0;
    double cashClosingBalance = 0.0;
    double bankClosingBalance = 0.0;
    int rowCount = 0;
};

class CashBankFlowController : public QObject {
    Q_OBJECT

public:
    explicit CashBankFlowController(QObject* parent = nullptr);

    FlowStatementType statementType() const { return m_statementType; }
    void setStatementType(FlowStatementType type);

    QString fromDate() const { return m_fromDate; }
    QString toDate() const { return m_toDate; }

    const QVector<FlowLedgerRow>& partyRows() const { return m_partyRows; }
    const QVector<FlowLedgerRow>& bankCashRows() const { return m_bankCashRows; }
    const FlowStatementSummary& summary() const { return m_summary; }

    void loadData(const QString& fromDate, const QString& toDate, const QString& filterText = "");

    QVariantList getPartyRowsAsVariantList() const;
    QVariantList getBankCashRowsAsVariantList() const;
    QVariantMap getSummaryAsVariantMap() const;

    QString generateCsvReport() const;
    QString statementTitle() const;

signals:
    void dataChanged();
    void statementTypeChanged(FlowStatementType type);

private:
    FlowStatementType m_statementType = FlowStatementType::CashFlow;
    QString m_fromDate;
    QString m_toDate;
    QString m_filterText;

    QVector<FlowLedgerRow> m_partyRows;
    QVector<FlowLedgerRow> m_bankCashRows;
    FlowStatementSummary m_summary;

    void recalculate();
    bool isCashLedger(const QString& name, const QString& groupName) const;
    bool isBankLedger(const QString& name, const QString& groupName) const;
};

} // namespace MahadevERP
