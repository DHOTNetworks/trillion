#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QString>
#include <QVector>
#include <QVariantMap>
#include <QVariantList>
#include <QDateTime>

struct TransportDispatchRecord {
    int id = 0;
    int fyId = 0;
    QString financialYear;
    QString dispatchDate;
    QString dispatchTime;
    QString slipNo;
    QString voucherNo;
    QString invoiceNo;
    QString voucherType = "Sale";
    int partyId = 0;
    QString partyName;
    int itemId = 0;
    QString itemName;
    QString grade;
    QString vehicleNo;
    QString driverName;
    QString driverPhone;
    QString transporterName;
    QString transporterGstin;
    QString grNo;
    QString grDate;
    QString destination;
    int distanceKm = 0;
    int bagCount = 0;
    double packingKg = 50.0;
    double grossWeightQtl = 0.0;
    double tareWeightQtl = 0.0;
    double bagTareKg = 0.0;
    double netWeightQtl = 0.0;
    QString freightCalcType = "Per Qtl"; // "Per Qtl", "Per Bag", "Fixed"
    double freightRate = 0.0;
    double totalFreight = 0.0;
    double advanceFreight = 0.0;
    double balanceFreight = 0.0;
    QString freightPaymentStatus = "Unpaid"; // "Unpaid", "Partially Paid", "Settled"
    QString ewayBillNo;
    QString irnNo;
    QString notes;
};

class TransportDispatchModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int filteredCount READ count NOTIFY filterSummaryChanged)
    Q_PROPERTY(double filteredNetWeightQtl READ filteredNetWeightQtl NOTIFY filterSummaryChanged)
    Q_PROPERTY(double filteredFreightAmount READ filteredFreightAmount NOTIFY filterSummaryChanged)
    Q_PROPERTY(double filteredBalancePayable READ filteredBalancePayable NOTIFY filterSummaryChanged)
    Q_PROPERTY(QString filteredNetWeightFmt READ filteredNetWeightFmt NOTIFY filterSummaryChanged)
    Q_PROPERTY(QString filteredFreightAmountFmt READ filteredFreightAmountFmt NOTIFY filterSummaryChanged)
    Q_PROPERTY(QString filteredBalancePayableFmt READ filteredBalancePayableFmt NOTIFY filterSummaryChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        SlipNoRole,
        DispatchDateRole,
        DispatchTimeRole,
        InvoiceNoRole,
        VoucherTypeRole,
        PartyNameRole,
        ItemNameRole,
        GradeRole,
        VehicleNoRole,
        DriverNameRole,
        DriverPhoneRole,
        TransporterNameRole,
        TransporterGstinRole,
        GrNoRole,
        GrDateRole,
        DestinationRole,
        DistanceKmRole,
        BagCountRole,
        PackingKgRole,
        GrossWeightRole,
        TareWeightRole,
        BagTareKgRole,
        NetWeightRole,
        FreightCalcTypeRole,
        FreightRateRole,
        TotalFreightRole,
        AdvanceFreightRole,
        BalanceFreightRole,
        FreightPaymentStatusRole,
        EwayBillNoRole,
        IrnNoRole,
        NotesRole,
        TotalFreightFmtRole,
        AdvanceFreightFmtRole,
        BalanceFreightFmtRole
    };

    explicit TransportDispatchModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setAllRecords(const QVector<TransportDispatchRecord> &records);
    const QVector<TransportDispatchRecord>& allRecords() const { return m_allRecords; }

    double filteredNetWeightQtl() const { return m_filteredNetWeightQtl; }
    double filteredFreightAmount() const { return m_filteredFreightAmount; }
    double filteredBalancePayable() const { return m_filteredBalancePayable; }
    QString filteredNetWeightFmt() const;
    QString filteredFreightAmountFmt() const;
    QString filteredBalancePayableFmt() const;

    Q_INVOKABLE void setFilter(const QString &filterType, const QString &searchQuery);
    Q_INVOKABLE QVariantMap get(int visibleIndex) const;
    Q_INVOKABLE int count() const { return m_visibleIndices.size(); }

signals:
    void countChanged();
    void filterSummaryChanged();

private:
    void applyFilter();

    QVector<TransportDispatchRecord> m_allRecords;
    QVector<int> m_visibleIndices;
    QString m_filterType = "ALL";
    QString m_searchQuery;
    double m_filteredNetWeightQtl = 0.0;
    double m_filteredFreightAmount = 0.0;
    double m_filteredBalancePayable = 0.0;
};

class TransportDispatchController : public QObject {
    Q_OBJECT

    Q_PROPERTY(TransportDispatchModel* model READ model CONSTANT)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY summaryChanged)
    Q_PROPERTY(double totalNetWeightQtl READ totalNetWeightQtl NOTIFY summaryChanged)
    Q_PROPERTY(double totalFreightAmount READ totalFreightAmount NOTIFY summaryChanged)
    Q_PROPERTY(double totalAdvancePaid READ totalAdvancePaid NOTIFY summaryChanged)
    Q_PROPERTY(double totalBalancePayable READ totalBalancePayable NOTIFY summaryChanged)
    Q_PROPERTY(QString totalNetWeightFmt READ totalNetWeightFmt NOTIFY summaryChanged)
    Q_PROPERTY(QString totalFreightAmountFmt READ totalFreightAmountFmt NOTIFY summaryChanged)
    Q_PROPERTY(QString totalAdvancePaidFmt READ totalAdvancePaidFmt NOTIFY summaryChanged)
    Q_PROPERTY(QString totalBalancePayableFmt READ totalBalancePayableFmt NOTIFY summaryChanged)

public:
    explicit TransportDispatchController(QObject *parent = nullptr);

    TransportDispatchModel* model() { return &m_model; }

    int totalCount() const { return m_totalCount; }
    double totalNetWeightQtl() const { return m_totalNetWeightQtl; }
    double totalFreightAmount() const { return m_totalFreightAmount; }
    double totalAdvancePaid() const { return m_totalAdvancePaid; }
    double totalBalancePayable() const { return m_totalBalancePayable; }

    QString totalNetWeightFmt() const;
    QString totalFreightAmountFmt() const;
    QString totalAdvancePaidFmt() const;
    QString totalBalancePayableFmt() const;

    Q_INVOKABLE void reload();
    Q_INVOKABLE double calculateNetWeight(double grossQtl, double tareQtl, int bagCount, double bagTareKg);
    Q_INVOKABLE QVariantMap calculateFreight(const QString &calcType, double rate, double netWeightQtl, int bagCount, double advancePaid);
    Q_INVOKABLE QVariantMap saveDispatch(const QVariantMap &data);
    Q_INVOKABLE bool deleteDispatch(int id);
    Q_INVOKABLE QVariantMap getDispatch(int id);
    Q_INVOKABLE QString getNextSlipNo();
    Q_INVOKABLE QString currentDateIso() const;
    Q_INVOKABLE QString currentTimeIso() const;
    Q_INVOKABLE QVariantList searchInvoices(const QString &query);

signals:
    void summaryChanged();
    void dispatchSaved(int id, const QString &slipNo);
    void dispatchDeleted(int id);

private:
    void recalculateSummary();

    TransportDispatchModel m_model;
    int m_totalCount = 0;
    double m_totalNetWeightQtl = 0.0;
    double m_totalFreightAmount = 0.0;
    double m_totalAdvancePaid = 0.0;
    double m_totalBalancePayable = 0.0;
};
