#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QString>
#include <QVector>
#include <QVariantList>
#include <QVariantMap>
#include "../services/canara_bank_statement_parser.h"

class BankStatementRowsModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        IndexRole = Qt::UserRole + 1,
        DateRole,
        IsoDateRole,
        NarrationRole,
        UtrRefRole,
        WithdrawalRole,
        DepositRole,
        BalanceRole,
        AmountRole,
        CategoryRole,
        ExtractedPartyRole,
        DrAccountRole,
        CrAccountRole,
        VoucherTypeRole,
        ConfidenceRole,
        IsDuplicateRole,
        IsSelectedRole
    };

    explicit BankStatementRowsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

    void setRows(const QVector<CanaraBankTransaction> &rows);
    const QVector<CanaraBankTransaction>& rows() const { return m_rows; }
    void clear();

    Q_INVOKABLE void setRowSelected(int index, bool selected);
    Q_INVOKABLE void setRowDrAccount(int index, const QString &acc);
    Q_INVOKABLE void setRowCrAccount(int index, const QString &acc);
    Q_INVOKABLE void setRowVoucherType(int index, const QString &vType);
    Q_INVOKABLE void selectAll(bool selected);
    Q_INVOKABLE void deselectDuplicates();
    Q_INVOKABLE QVariantMap getRow(int index) const;
    Q_INVOKABLE int count() const { return m_rows.size(); }

signals:
    void rowsChanged();
    void selectionChanged();

private:
    QVector<CanaraBankTransaction> m_rows;
};

class BankStatementController : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString statementPath READ statementPath NOTIFY statementLoaded)
    Q_PROPERTY(QString fileName READ fileName NOTIFY statementLoaded)
    Q_PROPERTY(QString accountNo READ accountNo NOTIFY statementLoaded)
    Q_PROPERTY(QString firmName READ firmName NOTIFY statementLoaded)
    Q_PROPERTY(QString ifscCode READ ifscCode NOTIFY statementLoaded)
    Q_PROPERTY(QString dateRange READ dateRange NOTIFY statementLoaded)
    Q_PROPERTY(QString bankLedgerName READ bankLedgerName WRITE setBankLedger NOTIFY bankLedgerChanged)

    Q_PROPERTY(int totalCount READ totalCount NOTIFY countsChanged)
    Q_PROPERTY(double totalWithdrawals READ totalWithdrawals NOTIFY totalsChanged)
    Q_PROPERTY(double totalDeposits READ totalDeposits NOTIFY totalsChanged)
    Q_PROPERTY(QString totalWithdrawalsFmt READ totalWithdrawalsFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString totalDepositsFmt READ totalDepositsFmt NOTIFY totalsChanged)

    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionChanged)
    Q_PROPERTY(double selectedWithdrawals READ selectedWithdrawals NOTIFY selectionChanged)
    Q_PROPERTY(double selectedDeposits READ selectedDeposits NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedWithdrawalsFmt READ selectedWithdrawalsFmt NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedDepositsFmt READ selectedDepositsFmt NOTIFY selectionChanged)

    Q_PROPERTY(int unmatchedCount READ unmatchedCount NOTIFY countsChanged)
    Q_PROPERTY(int duplicateCount READ duplicateCount NOTIFY countsChanged)

    Q_PROPERTY(BankStatementRowsModel* rowsModel READ rowsModel CONSTANT)

    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(bool isError READ isError NOTIFY statusChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)

public:
    explicit BankStatementController(QObject *parent = nullptr);

    QString statementPath() const { return m_statementPath; }
    QString fileName() const;
    QString accountNo() const { return m_header.accountNo; }
    QString firmName() const { return m_header.firmName; }
    QString ifscCode() const { return m_header.ifscCode; }
    QString dateRange() const;
    QString bankLedgerName() const { return m_bankLedgerName; }
    void setBankLedger(const QString &name);

    int totalCount() const { return m_rowsModel.count(); }
    double totalWithdrawals() const { return m_totalWithdrawals; }
    double totalDeposits() const { return m_totalDeposits; }
    QString totalWithdrawalsFmt() const;
    QString totalDepositsFmt() const;

    int selectedCount() const { return m_selectedCount; }
    double selectedWithdrawals() const { return m_selectedWithdrawals; }
    double selectedDeposits() const { return m_selectedDeposits; }
    QString selectedWithdrawalsFmt() const;
    QString selectedDepositsFmt() const;

    int unmatchedCount() const { return m_unmatchedCount; }
    int duplicateCount() const { return m_duplicateCount; }

    BankStatementRowsModel* rowsModel() { return &m_rowsModel; }

    QString statusMessage() const { return m_statusMessage; }
    bool isError() const { return m_isError; }
    bool isLoading() const { return m_isLoading; }

    Q_INVOKABLE bool loadStatement(const QString &filePath);
    Q_INVOKABLE QString browseStatementFile();
    Q_INVOKABLE void reloadFromCurrentStatement();
    Q_INVOKABLE QVariantMap postSelectedVouchers();
    Q_INVOKABLE void saveAlias(const QString &narration, const QString &mappedParty);
    Q_INVOKABLE QStringList getAvailableBankLedgers() const;
    Q_INVOKABLE QStringList getAllPartyLedgers() const;
    Q_INVOKABLE QVariantList searchPartyLedgers(const QString &query) const;

signals:
    void statementLoaded();
    void bankLedgerChanged();
    void totalsChanged();
    void selectionChanged();
    void countsChanged();
    void statusChanged();
    void loadingChanged();
    void vouchersPostedSuccess(int count, double totalAmt);

private slots:
    void onModelSelectionChanged();
    void recalculateTotalsAndCounts();

private:
    void detectBankLedger();
    void matchPartiesAndDuplicates(QVector<CanaraBankTransaction> &txns);
    QString findMatchingParty(const QString &extractedName, const QString &narration, QString &outConfidence);

    QString m_statementPath;
    CanaraBankStatementHeader m_header;
    QString m_bankLedgerName;

    BankStatementRowsModel m_rowsModel;

    double m_totalWithdrawals = 0.0;
    double m_totalDeposits = 0.0;
    int m_selectedCount = 0;
    double m_selectedWithdrawals = 0.0;
    double m_selectedDeposits = 0.0;
    int m_unmatchedCount = 0;
    int m_duplicateCount = 0;

    QString m_statusMessage;
    bool m_isError = false;
    bool m_isLoading = false;
};
