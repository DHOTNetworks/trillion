#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

class TcsReceiptVoucherController : public QObject {
    Q_OBJECT

    Q_PROPERTY(int receiptNo READ receiptNo WRITE setReceiptNo NOTIFY receiptNoChanged)
    Q_PROPERTY(QString receiptDate READ receiptDate WRITE setReceiptDate NOTIFY receiptDateChanged)
    Q_PROPERTY(QString receiptType READ receiptType WRITE setReceiptType NOTIFY receiptTypeChanged)
    Q_PROPERTY(bool postInBooks READ postInBooks WRITE setPostInBooks NOTIFY postInBooksChanged)

    Q_PROPERTY(int partyId READ partyId WRITE setPartyId NOTIFY partyIdChanged)
    Q_PROPERTY(QString partyName READ partyName NOTIFY partyNameChanged)
    Q_PROPERTY(QString partyPan READ partyPan NOTIFY partyPanChanged)

    Q_PROPERTY(int bankLedgerId READ bankLedgerId WRITE setBankLedgerId NOTIFY bankLedgerIdChanged)
    Q_PROPERTY(double bankAmount READ bankAmount WRITE setBankAmount NOTIFY totalsChanged)
    Q_PROPERTY(double withoutTcsAmount READ withoutTcsAmount WRITE setWithoutTcsAmount NOTIFY totalsChanged)
    Q_PROPERTY(double tcsRate READ tcsRate WRITE setTcsRate NOTIFY totalsChanged)
    Q_PROPERTY(double tcsAmount READ tcsAmount NOTIFY totalsChanged)
    Q_PROPERTY(bool tcsReceived READ tcsReceived WRITE setTcsReceived NOTIFY totalsChanged)
    Q_PROPERTY(double netBankReceipt READ netBankReceipt NOTIFY totalsChanged)

    Q_PROPERTY(double interestReceived READ interestReceived WRITE setInterestReceived NOTIFY totalsChanged)
    Q_PROPERTY(int interestLedgerId READ interestLedgerId WRITE setInterestLedgerId NOTIFY interestLedgerIdChanged)
    Q_PROPERTY(double discountAllowed READ discountAllowed WRITE setDiscountAllowed NOTIFY totalsChanged)
    Q_PROPERTY(int discountLedgerId READ discountLedgerId WRITE setDiscountLedgerId NOTIFY discountLedgerIdChanged)
    Q_PROPERTY(double otherAmount READ otherAmount WRITE setOtherAmount NOTIFY totalsChanged)
    Q_PROPERTY(int otherLedgerId READ otherLedgerId WRITE setOtherLedgerId NOTIFY otherLedgerIdChanged)

    Q_PROPERTY(double netCreditToParty READ netCreditToParty NOTIFY totalsChanged)
    Q_PROPERTY(QString narration READ narration WRITE setNarration NOTIFY narrationChanged)

    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(bool isError READ isError NOTIFY statusChanged)

public:
    explicit TcsReceiptVoucherController(QObject *parent = nullptr);

    int receiptNo() const { return m_receiptNo; }
    void setReceiptNo(int no);

    QString receiptDate() const { return m_receiptDate; }
    void setReceiptDate(const QString &d);

    QString receiptType() const { return m_receiptType; }
    void setReceiptType(const QString &t);

    bool postInBooks() const { return m_postInBooks; }
    void setPostInBooks(bool b);

    int partyId() const { return m_partyId; }
    void setPartyId(int id);

    QString partyName() const { return m_partyName; }
    QString partyPan() const { return m_partyPan; }

    int bankLedgerId() const { return m_bankLedgerId; }
    void setBankLedgerId(int id);

    double bankAmount() const { return m_bankAmount; }
    void setBankAmount(double amt);

    double withoutTcsAmount() const { return m_withoutTcsAmount; }
    void setWithoutTcsAmount(double amt);

    double tcsRate() const { return m_tcsRate; }
    void setTcsRate(double r);

    double tcsAmount() const { return m_tcsAmount; }

    bool tcsReceived() const { return m_tcsReceived; }
    void setTcsReceived(bool b);

    double netBankReceipt() const { return m_netBankReceipt; }

    double interestReceived() const { return m_interestReceived; }
    void setInterestReceived(double amt);

    int interestLedgerId() const { return m_interestLedgerId; }
    void setInterestLedgerId(int id);

    double discountAllowed() const { return m_discountAllowed; }
    void setDiscountAllowed(double amt);

    int discountLedgerId() const { return m_discountLedgerId; }
    void setDiscountLedgerId(int id);

    double otherAmount() const { return m_otherAmount; }
    void setOtherAmount(double amt);

    int otherLedgerId() const { return m_otherLedgerId; }
    void setOtherLedgerId(int id);

    double netCreditToParty() const { return m_netCreditToParty; }

    QString narration() const { return m_narration; }
    void setNarration(const QString &n);

    QString statusMessage() const { return m_statusMessage; }
    bool isError() const { return m_isError; }

    Q_INVOKABLE void resetForm();
    Q_INVOKABLE void recalculate();
    Q_INVOKABLE bool saveVoucher();
    Q_INVOKABLE bool loadVoucher(int voucherId);
    Q_INVOKABLE bool deleteVoucher(int voucherId);

    QVariantList getReceiptVouchersList() const;

signals:
    void receiptNoChanged();
    void receiptDateChanged();
    void receiptTypeChanged();
    void postInBooksChanged();
    void partyIdChanged();
    void partyNameChanged();
    void partyPanChanged();
    void bankLedgerIdChanged();
    void totalsChanged();
    void interestLedgerIdChanged();
    void discountLedgerIdChanged();
    void otherLedgerIdChanged();
    void narrationChanged();
    void statusChanged();
    void voucherSaved(int id);

private:
    int nextReceiptNumber() const;
    void ensureDefaultLedgers();

    int m_editVoucherId = 0;
    int m_receiptNo = 1;
    QString m_receiptDate;
    QString m_receiptType = "BANK";
    bool m_postInBooks = true;

    int m_partyId = 0;
    QString m_partyName;
    QString m_partyPan;

    int m_bankLedgerId = 0;
    double m_bankAmount = 0.0;
    double m_withoutTcsAmount = 0.0;
    double m_tcsRate = 0.10;
    double m_tcsAmount = 0.0;
    bool m_tcsReceived = true;
    double m_netBankReceipt = 0.0;

    double m_interestReceived = 0.0;
    int m_interestLedgerId = 0;
    double m_discountAllowed = 0.0;
    int m_discountLedgerId = 0;
    double m_otherAmount = 0.0;
    int m_otherLedgerId = 0;

    double m_netCreditToParty = 0.0;
    QString m_narration;

    QString m_statusMessage;
    bool m_isError = false;
};
