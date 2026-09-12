#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

struct JFormLineItem {
    QString itemName;
    int bags = 0;
    double loose = 0.0;
    QString packing;
    double weight = 0.0;
    double rate = 0.0;
    double amount = 0.0;
};

class JFormLineItemsModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        ItemNameRole = Qt::UserRole + 1,
        BagsRole,
        LooseRole,
        PackingRole,
        WeightRole,
        RateRole,
        AmountRole
    };

    explicit JFormLineItemsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void appendRow(const QString &itemName = "", int bags = 0, double loose = 0.0,
                               const QString &packing = "", double weight = 0.0, double rate = 0.0, double amount = 0.0);
    Q_INVOKABLE void removeRowAt(int index);
    Q_INVOKABLE void clear();
    Q_INVOKABLE int count() const { return m_items.size(); }
    Q_INVOKABLE QVariantMap getRow(int index) const;

    const QVector<JFormLineItem>& items() const { return m_items; }
    QVariantList toVariantList() const;

signals:
    void itemsChanged();

private:
    QVector<JFormLineItem> m_items;
};

class JFormVoucherController : public QObject {
    Q_OBJECT

    // Header & Meta Properties
    Q_PROPERTY(int voucherNo READ voucherNo WRITE setVoucherNo NOTIFY voucherNoChanged)
    Q_PROPERTY(QString jformNo READ jformNo WRITE setJformNo NOTIFY jformNoChanged)
    Q_PROPERTY(QString voucherDate READ voucherDate WRITE setVoucherDate NOTIFY voucherDateChanged)
    Q_PROPERTY(QString dayOfWeek READ dayOfWeek NOTIFY dayOfWeekChanged)
    Q_PROPERTY(int dueDays READ dueDays WRITE setDueDays NOTIFY dueDaysChanged)

    // Party / Zimidar
    Q_PROPERTY(int zimidarId READ zimidarId WRITE setZimidarId NOTIFY zimidarIdChanged)
    Q_PROPERTY(QString zimidarName READ zimidarName WRITE setZimidarName NOTIFY zimidarNameChanged)
    Q_PROPERTY(QString zimidarBalanceText READ zimidarBalanceText NOTIFY zimidarBalanceTextChanged)
    Q_PROPERTY(int partyId READ partyId WRITE setPartyId NOTIFY partyIdChanged)
    Q_PROPERTY(QString partyName READ partyName WRITE setPartyName NOTIFY partyNameChanged)
    Q_PROPERTY(QString saleStatus READ saleStatus WRITE setSaleStatus NOTIFY saleStatusChanged)
    Q_PROPERTY(QString procurementMode READ procurementMode WRITE setProcurementMode NOTIFY procurementModeChanged)

    // Logistics & Transport
    Q_PROPERTY(QString vehicleNo READ vehicleNo WRITE setVehicleNo NOTIFY vehicleNoChanged)
    Q_PROPERTY(QString driverName READ driverName WRITE setDriverName NOTIFY driverNameChanged)
    Q_PROPERTY(QString gatePassNo READ gatePassNo WRITE setGatePassNo NOTIFY gatePassNoChanged)
    Q_PROPERTY(QString ewayBillNo READ ewayBillNo WRITE setEwayBillNo NOTIFY ewayBillNoChanged)
    Q_PROPERTY(QString billTime READ billTime WRITE setBillTime NOTIFY billTimeChanged)
    Q_PROPERTY(QString saudaDate READ saudaDate WRITE setSaudaDate NOTIFY saudaDateChanged)
    Q_PROPERTY(QString mandiPlace READ mandiPlace WRITE setMandiPlace NOTIFY mandiPlaceChanged)
    Q_PROPERTY(QString lotNo READ lotNo WRITE setLotNo NOTIFY lotNoChanged)
    Q_PROPERTY(QString grade READ grade WRITE setGrade NOTIFY gradeChanged)
    Q_PROPERTY(QString transportName READ transportName WRITE setTransportName NOTIFY transportNameChanged)
    Q_PROPERTY(QString brokerName READ brokerName WRITE setBrokerName NOTIFY brokerNameChanged)
    Q_PROPERTY(QString challanNo READ challanNo WRITE setChallanNo NOTIFY challanNoChanged)
    Q_PROPERTY(QString kandaWeight READ kandaWeight WRITE setKandaWeight NOTIFY kandaWeightChanged)
    Q_PROPERTY(QString narration READ narration WRITE setNarration NOTIFY narrationChanged)

    // Summary & Totals
    Q_PROPERTY(int totalBags READ totalBags NOTIFY totalsChanged)
    Q_PROPERTY(double totalWeight READ totalWeight NOTIFY totalsChanged)
    Q_PROPERTY(double goodsAmount READ goodsAmount NOTIFY totalsChanged)
    Q_PROPERTY(double bonusAmount READ bonusAmount WRITE setBonusAmount NOTIFY totalsChanged)
    Q_PROPERTY(double reliefAmount READ reliefAmount WRITE setReliefAmount NOTIFY totalsChanged)
    Q_PROPERTY(double subtotalAmount READ subtotalAmount NOTIFY totalsChanged)
    Q_PROPERTY(double labourAmount READ labourAmount WRITE setLabourAmount NOTIFY totalsChanged)
    Q_PROPERTY(double roundOffAmount READ roundOffAmount WRITE setRoundOffAmount NOTIFY totalsChanged)
    Q_PROPERTY(double grandTotal READ grandTotal NOTIFY totalsChanged)

    // Formatted INR Strings
    Q_PROPERTY(QString goodsAmountFmt READ goodsAmountFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString subtotalAmountFmt READ subtotalAmountFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString grandTotalFmt READ grandTotalFmt NOTIFY totalsChanged)

    // Model & Status
    Q_PROPERTY(JFormLineItemsModel* lineItemsModel READ lineItemsModel CONSTANT)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(bool isError READ isError NOTIFY statusChanged)

public:
    explicit JFormVoucherController(QObject *parent = nullptr);

    int voucherNo() const { return m_voucherNo; }
    void setVoucherNo(int no) { if (m_voucherNo != no) { m_voucherNo = no; emit voucherNoChanged(); } }

    QString jformNo() const { return m_jformNo; }
    void setJformNo(const QString &no) { if (m_jformNo != no) { m_jformNo = no; emit jformNoChanged(); } }

    QString voucherDate() const { return m_voucherDate; }
    void setVoucherDate(const QString &d);

    QString dayOfWeek() const;

    int dueDays() const { return m_dueDays; }
    void setDueDays(int d) { if (m_dueDays != d) { m_dueDays = d; emit dueDaysChanged(); } }

    int zimidarId() const { return m_zimidarId; }
    void setZimidarId(int id) { if (m_zimidarId != id) { m_zimidarId = id; emit zimidarIdChanged(); } }

    QString zimidarName() const { return m_zimidarName; }
    void setZimidarName(const QString &name);

    QString zimidarBalanceText() const { return m_zimidarBalanceText; }

    int partyId() const { return m_partyId; }
    void setPartyId(int id) { if (m_partyId != id) { m_partyId = id; emit partyIdChanged(); } }

    QString partyName() const { return m_partyName; }
    void setPartyName(const QString &name);

    QString saleStatus() const { return m_saleStatus; }
    void setSaleStatus(const QString &s) { if (m_saleStatus != s) { m_saleStatus = s; emit saleStatusChanged(); } }

    QString procurementMode() const { return m_procurementMode; }
    void setProcurementMode(const QString &m) { if (m_procurementMode != m) { m_procurementMode = m; emit procurementModeChanged(); } }

    QString vehicleNo() const { return m_vehicleNo; }
    void setVehicleNo(const QString &v) { if (m_vehicleNo != v) { m_vehicleNo = v; emit vehicleNoChanged(); } }

    QString driverName() const { return m_driverName; }
    void setDriverName(const QString &v) { if (m_driverName != v) { m_driverName = v; emit driverNameChanged(); } }

    QString gatePassNo() const { return m_gatePassNo; }
    void setGatePassNo(const QString &v) { if (m_gatePassNo != v) { m_gatePassNo = v; emit gatePassNoChanged(); } }

    QString ewayBillNo() const { return m_ewayBillNo; }
    void setEwayBillNo(const QString &v) { if (m_ewayBillNo != v) { m_ewayBillNo = v; emit ewayBillNoChanged(); } }

    QString billTime() const { return m_billTime; }
    void setBillTime(const QString &v) { if (m_billTime != v) { m_billTime = v; emit billTimeChanged(); } }

    QString saudaDate() const { return m_saudaDate; }
    void setSaudaDate(const QString &v) { if (m_saudaDate != v) { m_saudaDate = v; emit saudaDateChanged(); } }

    QString mandiPlace() const { return m_mandiPlace; }
    void setMandiPlace(const QString &v) { if (m_mandiPlace != v) { m_mandiPlace = v; emit mandiPlaceChanged(); } }

    QString lotNo() const { return m_lotNo; }
    void setLotNo(const QString &v) { if (m_lotNo != v) { m_lotNo = v; emit lotNoChanged(); } }

    QString grade() const { return m_grade; }
    void setGrade(const QString &v) { if (m_grade != v) { m_grade = v; emit gradeChanged(); } }

    QString transportName() const { return m_transportName; }
    void setTransportName(const QString &v) { if (m_transportName != v) { m_transportName = v; emit transportNameChanged(); } }

    QString brokerName() const { return m_brokerName; }
    void setBrokerName(const QString &v) { if (m_brokerName != v) { m_brokerName = v; emit brokerNameChanged(); } }

    QString challanNo() const { return m_challanNo; }
    void setChallanNo(const QString &v) { if (m_challanNo != v) { m_challanNo = v; emit challanNoChanged(); } }

    QString kandaWeight() const { return m_kandaWeight; }
    void setKandaWeight(const QString &v) { if (m_kandaWeight != v) { m_kandaWeight = v; emit kandaWeightChanged(); } }

    QString narration() const { return m_narration; }
    void setNarration(const QString &v) { if (m_narration != v) { m_narration = v; emit narrationChanged(); } }

    int totalBags() const { return m_totalBags; }
    double totalWeight() const { return m_totalWeight; }
    double goodsAmount() const { return m_goodsAmount; }

    double bonusAmount() const { return m_bonusAmount; }
    void setBonusAmount(double v);

    double reliefAmount() const { return m_reliefAmount; }
    void setReliefAmount(double v);

    double subtotalAmount() const { return m_subtotalAmount; }

    double labourAmount() const { return m_labourAmount; }
    void setLabourAmount(double v);

    double roundOffAmount() const { return m_roundOffAmount; }
    void setRoundOffAmount(double v);

    double grandTotal() const { return m_grandTotal; }

    QString goodsAmountFmt() const;
    QString subtotalAmountFmt() const;
    QString grandTotalFmt() const;

    JFormLineItemsModel* lineItemsModel() { return &m_lineItemsModel; }

    QString statusMessage() const { return m_statusMessage; }
    bool isError() const { return m_isError; }

    Q_INVOKABLE void resetForm(const QString &workingDate = "");
    Q_INVOKABLE void updateZimidarBalance(const QString &partyName);
    Q_INVOKABLE void addItemRow(const QString &itemName, int bags, double loose,
                                const QString &packing, double weight, double rate, double amount);
    Q_INVOKABLE void removeLineItem(int index);
    Q_INVOKABLE void recalculateTotals();
    Q_INVOKABLE bool saveVoucher();

signals:
    void voucherNoChanged();
    void jformNoChanged();
    void voucherDateChanged();
    void dayOfWeekChanged();
    void dueDaysChanged();
    void zimidarIdChanged();
    void zimidarNameChanged();
    void zimidarBalanceTextChanged();
    void partyIdChanged();
    void partyNameChanged();
    void saleStatusChanged();
    void procurementModeChanged();
    void vehicleNoChanged();
    void driverNameChanged();
    void gatePassNoChanged();
    void ewayBillNoChanged();
    void billTimeChanged();
    void saudaDateChanged();
    void mandiPlaceChanged();
    void lotNoChanged();
    void gradeChanged();
    void transportNameChanged();
    void brokerNameChanged();
    void challanNoChanged();
    void kandaWeightChanged();
    void narrationChanged();
    void totalsChanged();
    void statusChanged();
    void voucherSaved();

private:
    int m_voucherNo = 1;
    QString m_jformNo = "1";
    QString m_voucherDate;
    int m_dueDays = 0;

    int m_zimidarId = 0;
    QString m_zimidarName;
    QString m_zimidarBalanceText = "Date Bal. 0.00 Dr";
    int m_partyId = 0;
    QString m_partyName = "Self Purchase";
    QString m_saleStatus = "Zimidara Self Purchase";
    QString m_procurementMode = "Direct Farmer";

    QString m_vehicleNo;
    QString m_driverName;
    QString m_gatePassNo;
    QString m_ewayBillNo;
    QString m_billTime;
    QString m_saudaDate;
    QString m_mandiPlace;
    QString m_lotNo;
    QString m_grade;
    QString m_transportName;
    QString m_brokerName;
    QString m_challanNo;
    QString m_kandaWeight;
    QString m_narration;

    int m_totalBags = 0;
    double m_totalWeight = 0.0;
    double m_goodsAmount = 0.0;
    double m_bonusAmount = 0.0;
    double m_reliefAmount = 0.0;
    double m_subtotalAmount = 0.0;
    double m_labourAmount = 0.0;
    double m_roundOffAmount = 0.0;
    double m_grandTotal = 0.0;

    QString m_statusMessage;
    bool m_isError = false;

    JFormLineItemsModel m_lineItemsModel;
};
