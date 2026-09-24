#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class TaxChallanController : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString taxType READ taxType WRITE setTaxType NOTIFY taxTypeChanged)
    Q_PROPERTY(QString challanNo READ challanNo WRITE setChallanNo NOTIFY challanNoChanged)
    Q_PROPERTY(QString challanDate READ challanDate WRITE setChallanDate NOTIFY challanDateChanged)
    Q_PROPERTY(bool postInBooks READ postInBooks WRITE setPostInBooks NOTIFY postInBooksChanged)
    Q_PROPERTY(QString periodFrom READ periodFrom WRITE setPeriodFrom NOTIFY periodChanged)
    Q_PROPERTY(QString periodTo READ periodTo WRITE setPeriodTo NOTIFY periodChanged)

    Q_PROPERTY(double basicTax READ basicTax NOTIFY totalsChanged)
    Q_PROPERTY(double surcharge READ surcharge WRITE setSurcharge NOTIFY totalsChanged)
    Q_PROPERTY(double cess READ cess WRITE setCess NOTIFY totalsChanged)
    Q_PROPERTY(double totalTax READ totalTax NOTIFY totalsChanged)

    Q_PROPERTY(double interestAmount READ interestAmount WRITE setInterestAmount NOTIFY totalsChanged)
    Q_PROPERTY(int interestLedgerId READ interestLedgerId WRITE setInterestLedgerId NOTIFY interestLedgerIdChanged)
    Q_PROPERTY(double penaltyAmount READ penaltyAmount WRITE setPenaltyAmount NOTIFY totalsChanged)
    Q_PROPERTY(int penaltyLedgerId READ penaltyLedgerId WRITE setPenaltyLedgerId NOTIFY penaltyLedgerIdChanged)
    Q_PROPERTY(double otherAmount READ otherAmount WRITE setOtherAmount NOTIFY totalsChanged)
    Q_PROPERTY(int otherLedgerId READ otherLedgerId WRITE setOtherLedgerId NOTIFY otherLedgerIdChanged)

    Q_PROPERTY(double totalChallanAmount READ totalChallanAmount NOTIFY totalsChanged)
    Q_PROPERTY(int bankLedgerId READ bankLedgerId WRITE setBankLedgerId NOTIFY bankLedgerIdChanged)
    Q_PROPERTY(QString chequeNo READ chequeNo WRITE setChequeNo NOTIFY chequeNoChanged)
    Q_PROPERTY(QString chequeDate READ chequeDate WRITE setChequeDate NOTIFY chequeDateChanged)
    Q_PROPERTY(QString bsrCode READ bsrCode WRITE setBsrCode NOTIFY bsrCodeChanged)
    Q_PROPERTY(QString minorHead READ minorHead WRITE setMinorHead NOTIFY minorHeadChanged)
    Q_PROPERTY(QString majorHead READ majorHead WRITE setMajorHead NOTIFY majorHeadChanged)
    Q_PROPERTY(QString narration READ narration WRITE setNarration NOTIFY narrationChanged)

    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(bool isError READ isError NOTIFY statusChanged)

public:
    explicit TaxChallanController(QObject *parent = nullptr);

    QString taxType() const { return m_taxType; }
    void setTaxType(const QString &type);

    QString challanNo() const { return m_challanNo; }
    void setChallanNo(const QString &no);

    QString challanDate() const { return m_challanDate; }
    void setChallanDate(const QString &d);

    bool postInBooks() const { return m_postInBooks; }
    void setPostInBooks(bool b);

    QString periodFrom() const { return m_periodFrom; }
    void setPeriodFrom(const QString &d);

    QString periodTo() const { return m_periodTo; }
    void setPeriodTo(const QString &d);

    double basicTax() const { return m_basicTax; }
    double surcharge() const { return m_surcharge; }
    void setSurcharge(double amt);

    double cess() const { return m_cess; }
    void setCess(double amt);

    double totalTax() const { return m_totalTax; }

    double interestAmount() const { return m_interestAmount; }
    void setInterestAmount(double amt);

    int interestLedgerId() const { return m_interestLedgerId; }
    void setInterestLedgerId(int id);

    double penaltyAmount() const { return m_penaltyAmount; }
    void setPenaltyAmount(double amt);

    int penaltyLedgerId() const { return m_penaltyLedgerId; }
    void setPenaltyLedgerId(int id);

    double otherAmount() const { return m_otherAmount; }
    void setOtherAmount(double amt);

    int otherLedgerId() const { return m_otherLedgerId; }
    void setOtherLedgerId(int id);

    double totalChallanAmount() const { return m_totalChallanAmount; }

    int bankLedgerId() const { return m_bankLedgerId; }
    void setBankLedgerId(int id);

    QString chequeNo() const { return m_chequeNo; }
    void setChequeNo(const QString &no);

    QString chequeDate() const { return m_chequeDate; }
    void setChequeDate(const QString &d);

    QString bsrCode() const { return m_bsrCode; }
    void setBsrCode(const QString &code);

    QString minorHead() const { return m_minorHead; }
    void setMinorHead(const QString &h);

    QString majorHead() const { return m_majorHead; }
    void setMajorHead(const QString &h);

    QString narration() const { return m_narration; }
    void setNarration(const QString &n);

    QString statusMessage() const { return m_statusMessage; }
    bool isError() const { return m_isError; }

    Q_INVOKABLE void resetForm();
    Q_INVOKABLE void fetchUndepositedVouchers();
    Q_INVOKABLE void toggleVoucherSelection(int index, bool selected);
    Q_INVOKABLE void selectAllVouchers(bool selected);
    Q_INVOKABLE void recalculate();
    Q_INVOKABLE bool saveChallan();
    Q_INVOKABLE bool loadChallan(int challanId);
    Q_INVOKABLE bool deleteChallan(int challanId);

    QVariantList undepositedVouchers() const { return m_undepositedVouchers; }
    QVariantList getChallansList(const QString &typeFilter = "") const;

signals:
    void taxTypeChanged();
    void challanNoChanged();
    void challanDateChanged();
    void postInBooksChanged();
    void periodChanged();
    void totalsChanged();
    void interestLedgerIdChanged();
    void penaltyLedgerIdChanged();
    void otherLedgerIdChanged();
    void bankLedgerIdChanged();
    void chequeNoChanged();
    void chequeDateChanged();
    void bsrCodeChanged();
    void minorHeadChanged();
    void majorHeadChanged();
    void narrationChanged();
    void statusChanged();
    void undepositedVouchersChanged();
    void challanSaved(int challanId);

private:
    QString nextChallanNumber() const;
    void ensureDefaultLedgers();

    QString m_taxType = "TDS"; // TDS or TCS
    QString m_challanNo;
    QString m_challanDate;
    bool m_postInBooks = true;
    QString m_periodFrom;
    QString m_periodTo;

    QVariantList m_undepositedVouchers;

    double m_basicTax = 0.0;
    double m_surcharge = 0.0;
    double m_cess = 0.0;
    double m_totalTax = 0.0;

    double m_interestAmount = 0.0;
    int m_interestLedgerId = 0;
    double m_penaltyAmount = 0.0;
    int m_penaltyLedgerId = 0;
    double m_otherAmount = 0.0;
    int m_otherLedgerId = 0;

    double m_totalChallanAmount = 0.0;
    int m_bankLedgerId = 0;
    QString m_chequeNo;
    QString m_chequeDate;
    QString m_bsrCode;
    QString m_minorHead = "200";
    QString m_majorHead = "0021";
    QString m_narration;

    int m_editChallanId = 0;
    QString m_statusMessage;
    bool m_isError = false;
};
