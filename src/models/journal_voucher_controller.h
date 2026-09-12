#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

struct JournalRowItem {
    QString drcr = "Dr";
    QString ledgerName;
    double debitAmt = 0.0;
    double creditAmt = 0.0;
    QString refNo;
};

class JournalRowsModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        DrCrRole = Qt::UserRole + 1,
        LedgerNameRole,
        DebitAmtRole,
        CreditAmtRole,
        RefNoRole
    };

    explicit JournalRowsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void appendRow(const QString &drcr = "Dr", const QString &ledgerName = "",
                               double debitAmt = 0.0, double creditAmt = 0.0, const QString &refNo = "");
    Q_INVOKABLE void removeRowAt(int index);
    Q_INVOKABLE void clear();
    Q_INVOKABLE int count() const { return m_rows.size(); }
    Q_INVOKABLE QVariantMap getRow(int index) const;
    Q_INVOKABLE void setRowProperty(int index, const QString &property, const QVariant &value);

    const QVector<JournalRowItem>& rows() const { return m_rows; }
    QVariantList toVariantList() const;

signals:
    void rowsChanged();

private:
    QVector<JournalRowItem> m_rows;
};

class JournalVoucherController : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString voucherNo READ voucherNo WRITE setVoucherNo NOTIFY voucherNoChanged)
    Q_PROPERTY(QString voucherDate READ voucherDate WRITE setVoucherDate NOTIFY voucherDateChanged)
    Q_PROPERTY(QString dayOfWeek READ dayOfWeek NOTIFY dayOfWeekChanged)
    Q_PROPERTY(QString narration READ narration WRITE setNarration NOTIFY narrationChanged)

    Q_PROPERTY(double totalDebit READ totalDebit NOTIFY totalsChanged)
    Q_PROPERTY(double totalCredit READ totalCredit NOTIFY totalsChanged)
    Q_PROPERTY(QString totalDebitFmt READ totalDebitFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString totalCreditFmt READ totalCreditFmt NOTIFY totalsChanged)

    Q_PROPERTY(JournalRowsModel* rowsModel READ rowsModel CONSTANT)

    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(bool isError READ isError NOTIFY statusChanged)

public:
    explicit JournalVoucherController(QObject *parent = nullptr);

    QString voucherNo() const { return m_voucherNo; }
    void setVoucherNo(const QString &v) { if (m_voucherNo != v) { m_voucherNo = v; emit voucherNoChanged(); } }

    QString voucherDate() const { return m_voucherDate; }
    void setVoucherDate(const QString &v);

    QString dayOfWeek() const;

    QString narration() const { return m_narration; }
    void setNarration(const QString &v) { if (m_narration != v) { m_narration = v; emit narrationChanged(); } }

    double totalDebit() const { return m_totalDebit; }
    double totalCredit() const { return m_totalCredit; }
    QString totalDebitFmt() const;
    QString totalCreditFmt() const;

    JournalRowsModel* rowsModel() { return &m_rowsModel; }

    QString statusMessage() const { return m_statusMessage; }
    bool isError() const { return m_isError; }

    Q_INVOKABLE void resetForm(const QString &workingDate = "");
    Q_INVOKABLE void addNewRow();
    Q_INVOKABLE void removeRow(int idx);
    Q_INVOKABLE void recalculateTotals();
    Q_INVOKABLE bool saveVoucher();

signals:
    void voucherNoChanged();
    void voucherDateChanged();
    void dayOfWeekChanged();
    void narrationChanged();
    void totalsChanged();
    void statusChanged();
    void voucherSaved();

private:
    QString m_voucherNo;
    QString m_voucherDate;
    QString m_narration;

    double m_totalDebit = 0.0;
    double m_totalCredit = 0.0;

    QString m_statusMessage;
    bool m_isError = false;

    JournalRowsModel m_rowsModel;
};
