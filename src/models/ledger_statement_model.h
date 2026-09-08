#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>
#include <QVariantMap>
#include <QVariantList>

struct LedgerStatementEntry {
    int id = 0;
    bool isSelected = false;
    QString vIso;
    QString vDate;
    QString refNo;
    QString voucherNo;
    QString invoiceNo;
    QString voucherType;
    QString legacyType;
    QString transType;
    QString particulars;
    double amount = 0.0;
    QString amountFmt;
    QString financialYear;
    QString side; // "Dr" or "Cr"
};

class LedgerStatementSideModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(double totalAmount READ totalAmount NOTIFY totalsChanged)
    Q_PROPERTY(QString totalAmountFmt READ totalAmountFmt NOTIFY totalsChanged)
    Q_PROPERTY(double selectedTotal READ selectedTotal NOTIFY totalsChanged)
    Q_PROPERTY(QString selectedTotalFmt READ selectedTotalFmt NOTIFY totalsChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        IsSelectedRole,
        VIsoRole,
        VDateRole,
        RefNoRole,
        VoucherNoRole,
        InvoiceNoRole,
        VoucherTypeRole,
        LegacyTypeRole,
        TransTypeRole,
        ParticularsRole,
        AmountRole,
        AmountFmtRole,
        FinancialYearRole,
        SideRole
    };

    explicit LedgerStatementSideModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(const QVector<LedgerStatementEntry>& entries);
    void clear();

    double totalAmount() const { return m_totalAmount; }
    QString totalAmountFmt() const { return m_totalAmountFmt; }
    double selectedTotal() const { return m_selectedTotal; }
    QString selectedTotalFmt() const { return m_selectedTotalFmt; }

    Q_INVOKABLE void toggleSelection(int row);
    Q_INVOKABLE void setSelection(int row, bool selected);
    Q_INVOKABLE void selectAll(bool selected);
    Q_INVOKABLE QVariantMap get(int row) const;

signals:
    void countChanged();
    void totalsChanged();

private:
    void recalculateTotals();

    QVector<LedgerStatementEntry> m_entries;
    double m_totalAmount = 0.0;
    QString m_totalAmountFmt = "₹0.00";
    double m_selectedTotal = 0.0;
    QString m_selectedTotalFmt = "₹0.00";
};

class LedgerStatementController : public QObject {
    Q_OBJECT
    Q_PROPERTY(LedgerStatementSideModel* drModel READ drModel CONSTANT)
    Q_PROPERTY(LedgerStatementSideModel* crModel READ crModel CONSTANT)
    Q_PROPERTY(QString currentPartyName READ currentPartyName WRITE setCurrentPartyName NOTIFY currentPartyNameChanged)
    Q_PROPERTY(double drTotal READ drTotal NOTIFY statementTotalsChanged)
    Q_PROPERTY(double crTotal READ crTotal NOTIFY statementTotalsChanged)
    Q_PROPERTY(QString drTotalFmt READ drTotalFmt NOTIFY statementTotalsChanged)
    Q_PROPERTY(QString crTotalFmt READ crTotalFmt NOTIFY statementTotalsChanged)
    Q_PROPERTY(double drSelectedTotal READ drSelectedTotal NOTIFY statementTotalsChanged)
    Q_PROPERTY(double crSelectedTotal READ crSelectedTotal NOTIFY statementTotalsChanged)
    Q_PROPERTY(QString drSelectedTotalFmt READ drSelectedTotalFmt NOTIFY statementTotalsChanged)
    Q_PROPERTY(QString crSelectedTotalFmt READ crSelectedTotalFmt NOTIFY statementTotalsChanged)
    Q_PROPERTY(double netBalance READ netBalance NOTIFY statementTotalsChanged)
    Q_PROPERTY(QString netBalanceFmt READ netBalanceFmt NOTIFY statementTotalsChanged)
    Q_PROPERTY(QString netBalanceType READ netBalanceType NOTIFY statementTotalsChanged)

public:
    explicit LedgerStatementController(QObject* parent = nullptr);

    LedgerStatementSideModel* drModel() { return &m_drModel; }
    LedgerStatementSideModel* crModel() { return &m_crModel; }

    QString currentPartyName() const { return m_currentPartyName; }
    void setCurrentPartyName(const QString& name);

    double drTotal() const { return m_drModel.totalAmount(); }
    double crTotal() const { return m_crModel.totalAmount(); }
    QString drTotalFmt() const { return m_drModel.totalAmountFmt(); }
    QString crTotalFmt() const { return m_crModel.totalAmountFmt(); }
    double drSelectedTotal() const { return m_drModel.selectedTotal(); }
    double crSelectedTotal() const { return m_crModel.selectedTotal(); }
    QString drSelectedTotalFmt() const { return m_drModel.selectedTotalFmt(); }
    QString crSelectedTotalFmt() const { return m_crModel.selectedTotalFmt(); }
    double netBalance() const;
    QString netBalanceFmt() const;
    QString netBalanceType() const;

    Q_INVOKABLE void loadPartyStatement(const QString& partyName, const QString& fromDate = "", const QString& toDate = "");
    Q_INVOKABLE void applyDateFilter(const QString& fromDate, const QString& toDate);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE QVariantList searchParties(const QString& query) const;

signals:
    void currentPartyNameChanged();
    void statementTotalsChanged();
    void statementLoaded();

private slots:
    void onSideTotalsChanged();

private:
    void reloadData();
    void applyFilterInternal();

    LedgerStatementSideModel m_drModel;
    LedgerStatementSideModel m_crModel;
    QString m_currentPartyName;
    QString m_fromDate;
    QString m_toDate;

    QVector<LedgerStatementEntry> m_rawDrEntries;
    QVector<LedgerStatementEntry> m_rawCrEntries;
};
