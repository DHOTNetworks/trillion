#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

class PaddyProcurementController : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString farmerName READ farmerName WRITE setFarmerName NOTIFY farmerNameChanged)
    Q_PROPERTY(QString paddyVariety READ paddyVariety WRITE setPaddyVariety NOTIFY paddyVarietyChanged)
    Q_PROPERTY(QString arrivalDate READ arrivalDate WRITE setArrivalDate NOTIFY arrivalDateChanged)
    Q_PROPERTY(QString dayOfWeek READ dayOfWeek NOTIFY dayOfWeekChanged)

    Q_PROPERTY(int bagCount READ bagCount WRITE setBagCount NOTIFY calculationsChanged)
    Q_PROPERTY(double grossWeightQtl READ grossWeightQtl WRITE setGrossWeightQtl NOTIFY calculationsChanged)
    Q_PROPERTY(double moisturePct READ moisturePct WRITE setMoisturePct NOTIFY calculationsChanged)
    Q_PROPERTY(double moistureDeductionQtl READ moistureDeductionQtl NOTIFY calculationsChanged)
    Q_PROPERTY(double netWeightQtl READ netWeightQtl NOTIFY calculationsChanged)
    Q_PROPERTY(double ratePerQtl READ ratePerQtl WRITE setRatePerQtl NOTIFY calculationsChanged)
    Q_PROPERTY(double hamaliPerBag READ hamaliPerBag WRITE setHamaliPerBag NOTIFY calculationsChanged)
    Q_PROPERTY(double hamaliTotal READ hamaliTotal NOTIFY calculationsChanged)
    Q_PROPERTY(double netAmount READ netAmount NOTIFY calculationsChanged)
    Q_PROPERTY(QString netAmountFmt READ netAmountFmt NOTIFY calculationsChanged)

    Q_PROPERTY(QString paymentStatus READ paymentStatus WRITE setPaymentStatus NOTIFY paymentStatusChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(bool isError READ isError NOTIFY statusChanged)

public:
    explicit PaddyProcurementController(QObject *parent = nullptr);

    QString farmerName() const { return m_farmerName; }
    void setFarmerName(const QString &v) { if (m_farmerName != v) { m_farmerName = v; emit farmerNameChanged(); } }

    QString paddyVariety() const { return m_paddyVariety; }
    void setPaddyVariety(const QString &v) { if (m_paddyVariety != v) { m_paddyVariety = v; emit paddyVarietyChanged(); } }

    QString arrivalDate() const { return m_arrivalDate; }
    void setArrivalDate(const QString &v);

    QString dayOfWeek() const;

    int bagCount() const { return m_bagCount; }
    void setBagCount(int b);

    double grossWeightQtl() const { return m_grossWeightQtl; }
    void setGrossWeightQtl(double w);

    double moisturePct() const { return m_moisturePct; }
    void setMoisturePct(double m);

    double moistureDeductionQtl() const { return m_moistureDeductionQtl; }
    double netWeightQtl() const { return m_netWeightQtl; }

    double ratePerQtl() const { return m_ratePerQtl; }
    void setRatePerQtl(double r);

    double hamaliPerBag() const { return m_hamaliPerBag; }
    void setHamaliPerBag(double h);

    double hamaliTotal() const { return m_hamaliTotal; }
    double netAmount() const { return m_netAmount; }
    QString netAmountFmt() const;

    QString paymentStatus() const { return m_paymentStatus; }
    void setPaymentStatus(const QString &s) { if (m_paymentStatus != s) { m_paymentStatus = s; emit paymentStatusChanged(); } }

    QString statusMessage() const { return m_statusMessage; }
    bool isError() const { return m_isError; }

    Q_INVOKABLE void resetForm(const QString &workingDate = "");
    Q_INVOKABLE void recalculateDeductions();
    Q_INVOKABLE bool saveArrivalSlip();

signals:
    void farmerNameChanged();
    void paddyVarietyChanged();
    void arrivalDateChanged();
    void dayOfWeekChanged();
    void calculationsChanged();
    void paymentStatusChanged();
    void statusChanged();
    void arrivalSaved();

private:
    QString m_farmerName;
    QString m_paddyVariety = "Sona Masoori";
    QString m_arrivalDate;

    int m_bagCount = 0;
    double m_grossWeightQtl = 0.0;
    double m_moisturePct = 14.0;
    double m_moistureDeductionQtl = 0.0;
    double m_netWeightQtl = 0.0;
    double m_ratePerQtl = 0.0;
    double m_hamaliPerBag = 0.0;
    double m_hamaliTotal = 0.0;
    double m_netAmount = 0.0;

    QString m_paymentStatus = "Pending";
    QString m_statusMessage;
    bool m_isError = false;
};
