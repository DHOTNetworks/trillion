#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

class TdsVoucherController : public QObject {
    Q_OBJECT

    Q_PROPERTY(int editVoucherId READ editVoucherId WRITE setEditVoucherId NOTIFY editVoucherIdChanged)
    Q_PROPERTY(bool isEditMode READ isEditMode NOTIFY isEditModeChanged)
    Q_PROPERTY(int voucherNo READ voucherNo WRITE setVoucherNo NOTIFY voucherNoChanged)
    Q_PROPERTY(QString voucherDate READ voucherDate WRITE setVoucherDate NOTIFY voucherDateChanged)
    Q_PROPERTY(QString dayOfWeek READ dayOfWeek NOTIFY dayOfWeekChanged)
    Q_PROPERTY(bool postInBooks READ postInBooks WRITE setPostInBooks NOTIFY postInBooksChanged)

    Q_PROPERTY(QString tdsType READ tdsType WRITE setTdsType NOTIFY tdsTypeChanged)
    Q_PROPERTY(int selectedPartyId READ selectedPartyId WRITE setSelectedPartyId NOTIFY selectedPartyIdChanged)
    Q_PROPERTY(QString selectedPartyName READ selectedPartyName WRITE setSelectedPartyName NOTIFY selectedPartyNameChanged)
    Q_PROPERTY(QString partyPan READ partyPan WRITE setPartyPan NOTIFY partyPanChanged)
    Q_PROPERTY(QString partyBalanceText READ partyBalanceText NOTIFY partyBalanceTextChanged)

    Q_PROPERTY(double incomeAmount READ incomeAmount WRITE setIncomeAmount NOTIFY totalsChanged)
    Q_PROPERTY(double previousAmount READ previousAmount WRITE setPreviousAmount NOTIFY totalsChanged)
    Q_PROPERTY(double totalForTds READ totalForTds NOTIFY totalsChanged)

    Q_PROPERTY(double tdsRate READ tdsRate WRITE setTdsRate NOTIFY totalsChanged)
    Q_PROPERTY(double tdsTaxAmount READ tdsTaxAmount NOTIFY totalsChanged)
    Q_PROPERTY(double surchargeRate READ surchargeRate WRITE setSurchargeRate NOTIFY totalsChanged)
    Q_PROPERTY(double surchargeTaxAmount READ surchargeTaxAmount NOTIFY totalsChanged)
    Q_PROPERTY(double cessRate READ cessRate WRITE setCessRate NOTIFY totalsChanged)
    Q_PROPERTY(double cessTaxAmount READ cessTaxAmount NOTIFY totalsChanged)
    Q_PROPERTY(bool useRoundedTotal READ useRoundedTotal WRITE setUseRoundedTotal NOTIFY totalsChanged)
    Q_PROPERTY(double totalTaxRate READ totalTaxRate NOTIFY totalsChanged)
    Q_PROPERTY(double totalTaxAmount READ totalTaxAmount NOTIFY totalsChanged)
    Q_PROPERTY(double netAmount READ netAmount NOTIFY totalsChanged)

    // Formatted INR Strings
    Q_PROPERTY(QString incomeAmountFmt READ incomeAmountFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString totalTaxAmountFmt READ totalTaxAmountFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString netAmountFmt READ netAmountFmt NOTIFY totalsChanged)

    Q_PROPERTY(QString expLedgerName READ expLedgerName WRITE setExpLedgerName NOTIFY expLedgerNameChanged)
    Q_PROPERTY(QString tdsLedgerName READ tdsLedgerName WRITE setTdsLedgerName NOTIFY tdsLedgerNameChanged)
    Q_PROPERTY(QString narration READ narration WRITE setNarration NOTIFY narrationChanged)
    Q_PROPERTY(QString nonDeductionReason READ nonDeductionReason WRITE setNonDeductionReason NOTIFY nonDeductionReasonChanged)

    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(bool isError READ isError NOTIFY statusChanged)

public:
    explicit TdsVoucherController(QObject *parent = nullptr);

    int editVoucherId() const { return m_editVoucherId; }
    void setEditVoucherId(int id);
    bool isEditMode() const { return m_editVoucherId > 0; }

    int voucherNo() const { return m_voucherNo; }
    void setVoucherNo(int no) { if (m_voucherNo != no) { m_voucherNo = no; emit voucherNoChanged(); } }

    QString voucherDate() const { return m_voucherDate; }
    void setVoucherDate(const QString &d);

    QString dayOfWeek() const;

    bool postInBooks() const { return m_postInBooks; }
    void setPostInBooks(bool b) { if (m_postInBooks != b) { m_postInBooks = b; emit postInBooksChanged(); } }

    QString tdsType() const { return m_tdsType; }
    void setTdsType(const QString &t);

    int selectedPartyId() const { return m_selectedPartyId; }
    void setSelectedPartyId(int id) { if (m_selectedPartyId != id) { m_selectedPartyId = id; emit selectedPartyIdChanged(); } }

    QString selectedPartyName() const { return m_selectedPartyName; }
    void setSelectedPartyName(const QString &name);

    QString partyPan() const { return m_partyPan; }
    void setPartyPan(const QString &pan) { if (m_partyPan != pan) { m_partyPan = pan; emit partyPanChanged(); } }

    QString partyBalanceText() const { return m_partyBalanceText; }

    double incomeAmount() const { return m_incomeAmount; }
    void setIncomeAmount(double amt);

    double previousAmount() const { return m_previousAmount; }
    void setPreviousAmount(double amt);

    double totalForTds() const { return m_totalForTds; }

    double tdsRate() const { return m_tdsRate; }
    void setTdsRate(double r);

    double tdsTaxAmount() const { return m_tdsTaxAmount; }

    double surchargeRate() const { return m_surchargeRate; }
    void setSurchargeRate(double r);

    double surchargeTaxAmount() const { return m_surchargeTaxAmount; }

    double cessRate() const { return m_cessRate; }
    void setCessRate(double r);

    double cessTaxAmount() const { return m_cessTaxAmount; }

    bool useRoundedTotal() const { return m_useRoundedTotal; }
    void setUseRoundedTotal(bool b);

    double totalTaxRate() const { return m_totalTaxRate; }
    double totalTaxAmount() const { return m_totalTaxAmount; }
    double netAmount() const { return m_netAmount; }

    QString incomeAmountFmt() const;
    QString totalTaxAmountFmt() const;
    QString netAmountFmt() const;

    QString expLedgerName() const { return m_expLedgerName; }
    void setExpLedgerName(const QString &n) { if (m_expLedgerName != n) { m_expLedgerName = n; emit expLedgerNameChanged(); } }

    QString tdsLedgerName() const { return m_tdsLedgerName; }
    void setTdsLedgerName(const QString &n) { if (m_tdsLedgerName != n) { m_tdsLedgerName = n; emit tdsLedgerNameChanged(); } }

    QString narration() const { return m_narration; }
    void setNarration(const QString &n) { if (m_narration != n) { m_narration = n; emit narrationChanged(); } }

    QString nonDeductionReason() const { return m_nonDeductionReason; }
    void setNonDeductionReason(const QString &r) { if (m_nonDeductionReason != r) { m_nonDeductionReason = r; emit nonDeductionReasonChanged(); } }

    QString statusMessage() const { return m_statusMessage; }
    bool isError() const { return m_isError; }

    Q_INVOKABLE void resetForm(const QString &workingDate = "");
    Q_INVOKABLE bool loadVoucher(int voucherId);
    Q_INVOKABLE void updatePartyInfo(const QString &partyName);
    Q_INVOKABLE void recalculateTotals();
    Q_INVOKABLE bool saveVoucher();

signals:
    void editVoucherIdChanged();
    void isEditModeChanged();
    void voucherNoChanged();
    void voucherDateChanged();
    void dayOfWeekChanged();
    void postInBooksChanged();
    void tdsTypeChanged();
    void selectedPartyIdChanged();
    void selectedPartyNameChanged();
    void partyPanChanged();
    void partyBalanceTextChanged();
    void totalsChanged();
    void expLedgerNameChanged();
    void tdsLedgerNameChanged();
    void narrationChanged();
    void nonDeductionReasonChanged();
    void statusChanged();
    void voucherSaved();

private:
    int m_editVoucherId = 0;
    int m_voucherNo = 1;
    QString m_voucherDate;
    bool m_postInBooks = true;

    QString m_tdsType = "RENT";
    int m_selectedPartyId = 0;
    QString m_selectedPartyName;
    QString m_partyPan;
    QString m_partyBalanceText = "Date Bal. 0.00";

    double m_incomeAmount = 0.0;
    double m_previousAmount = 0.0;
    double m_totalForTds = 0.0;

    double m_tdsRate = 10.0;
    double m_tdsTaxAmount = 0.0;
    double m_surchargeRate = 0.0;
    double m_surchargeTaxAmount = 0.0;
    double m_cessRate = 0.0;
    double m_cessTaxAmount = 0.0;
    bool m_useRoundedTotal = true;
    double m_totalTaxRate = 10.0;
    double m_totalTaxAmount = 0.0;
    double m_netAmount = 0.0;

    QString m_expLedgerName;
    QString m_tdsLedgerName;
    QString m_narration;
    QString m_nonDeductionReason;

    QString m_statusMessage;
    bool m_isError = false;
};
