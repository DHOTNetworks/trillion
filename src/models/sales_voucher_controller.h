#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

struct SalesLineItem {
    QString itemName;
    QString hsnCode;
    QString unit;
    int bags = 0;
    double packingKg = 0.0;
    double weightQtl = 0.0;
    double rate = 0.0;
    double amount = 0.0;
    bool isStock = true;
};

class SalesLineItemsModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum ItemRoles {
        ItemNameRole = Qt::UserRole + 1,
        HsnCodeRole,
        UnitRole,
        BagsRole,
        PackingKgRole,
        WeightQtlRole,
        RateRole,
        AmountRole,
        IsStockRole
    };

    explicit SalesLineItemsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void appendRow(const QString &itemName = "", const QString &hsn = "", const QString &unit = "QTL",
                               int bags = 0, double packing = 0.0, double weight = 0.0, double rate = 0.0, double amount = 0.0);
    Q_INVOKABLE void removeRowAt(int index);
    Q_INVOKABLE void clear();
    Q_INVOKABLE int count() const { return m_items.size(); }
    Q_INVOKABLE QVariantMap getRow(int index) const;
    Q_INVOKABLE void setRowProperty(int index, const QString &property, const QVariant &value);

    const QVector<SalesLineItem>& items() const { return m_items; }
    QVariantList toVariantList() const;
    void loadFromVariantList(const QVariantList &list);

signals:
    void itemsChanged();

private:
    QVector<SalesLineItem> m_items;
};

class SalesModel;

class SalesVoucherController : public QObject {
    Q_OBJECT

    // Header & Meta Properties
    Q_PROPERTY(int editingInvoiceId READ editingInvoiceId WRITE setEditingInvoiceId NOTIFY editingInvoiceIdChanged)
    Q_PROPERTY(bool isEditMode READ isEditMode NOTIFY isEditModeChanged)
    Q_PROPERTY(QString voucherNo READ voucherNo WRITE setVoucherNo NOTIFY voucherNoChanged)
    Q_PROPERTY(QString invoiceNo READ invoiceNo WRITE setInvoiceNo NOTIFY invoiceNoChanged)
    Q_PROPERTY(QString invoiceDate READ invoiceDate WRITE setInvoiceDate NOTIFY invoiceDateChanged)
    Q_PROPERTY(QString dayOfWeek READ dayOfWeek NOTIFY dayOfWeekChanged)
    Q_PROPERTY(QString marketType READ marketType WRITE setMarketType NOTIFY marketTypeChanged)
    Q_PROPERTY(QString saleStatus READ saleStatus WRITE setSaleStatus NOTIFY saleStatusChanged)
    Q_PROPERTY(QString paymentMode READ paymentMode WRITE setPaymentMode NOTIFY paymentModeChanged)
    Q_PROPERTY(QString taxStatus READ taxStatus WRITE setTaxStatus NOTIFY taxStatusChanged)

    // Party Properties
    Q_PROPERTY(QString partyLedger READ partyLedger WRITE setPartyLedger NOTIFY partyLedgerChanged)
    Q_PROPERTY(QString gstin READ gstin WRITE setGstin NOTIFY gstinChanged)
    Q_PROPERTY(int dueDays READ dueDays WRITE setDueDays NOTIFY dueDaysChanged)
    Q_PROPERTY(QString salesAccount READ salesAccount WRITE setSalesAccount NOTIFY salesAccountChanged)

    // Financial & Summary Properties
    Q_PROPERTY(int totalBags READ totalBags NOTIFY totalsChanged)
    Q_PROPERTY(double totalWeightQtl READ totalWeightQtl NOTIFY totalsChanged)
    Q_PROPERTY(double taxableAmount READ taxableAmount NOTIFY totalsChanged)
    Q_PROPERTY(double gstRate READ gstRate WRITE setGstRate NOTIFY gstRateChanged)
    Q_PROPERTY(bool isInterstate READ isInterstate WRITE setIsInterstate NOTIFY isInterstateChanged)
    Q_PROPERTY(double cgstAmount READ cgstAmount NOTIFY totalsChanged)
    Q_PROPERTY(double sgstAmount READ sgstAmount NOTIFY totalsChanged)
    Q_PROPERTY(double igstAmount READ igstAmount NOTIFY totalsChanged)
    Q_PROPERTY(double totalTaxAmount READ totalTaxAmount NOTIFY totalsChanged)
    Q_PROPERTY(double freightCharges READ freightCharges WRITE setFreightCharges NOTIFY totalsChanged)
    Q_PROPERTY(double tcsRate READ tcsRate WRITE setTcsRate NOTIFY totalsChanged)
    Q_PROPERTY(double tcsAmount READ tcsAmount NOTIFY totalsChanged)
    Q_PROPERTY(double roundOff READ roundOff NOTIFY totalsChanged)
    Q_PROPERTY(double grandTotal READ grandTotal NOTIFY totalsChanged)

    // Formatted Rupee Strings for Direct QML Binding
    Q_PROPERTY(QString taxableAmountFmt READ taxableAmountFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString cgstAmountFmt READ cgstAmountFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString sgstAmountFmt READ sgstAmountFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString igstAmountFmt READ igstAmountFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString roundOffFmt READ roundOffFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString grandTotalFmt READ grandTotalFmt NOTIFY totalsChanged)

    // Additional Expenses
    Q_PROPERTY(double dami READ dami WRITE setDami NOTIFY totalsChanged)
    Q_PROPERTY(double labour READ labour WRITE setLabour NOTIFY totalsChanged)
    Q_PROPERTY(double auction READ auction WRITE setAuction NOTIFY totalsChanged)
    Q_PROPERTY(double marketFee READ marketFee WRITE setMarketFee NOTIFY totalsChanged)
    Q_PROPERTY(double hrdf READ hrdf WRITE setHrdf NOTIFY totalsChanged)
    Q_PROPERTY(double otherExp READ otherExp WRITE setOtherExp NOTIFY totalsChanged)

    // Transport & Shipping
    Q_PROPERTY(QString vehicleNo READ vehicleNo WRITE setVehicleNo NOTIFY vehicleNoChanged)
    Q_PROPERTY(QString ewayBillNo READ ewayBillNo WRITE setEwayBillNo NOTIFY ewayBillNoChanged)
    Q_PROPERTY(QString grNo READ grNo WRITE setGrNo NOTIFY grNoChanged)
    Q_PROPERTY(QString transportName READ transportName WRITE setTransportName NOTIFY transportNameChanged)
    Q_PROPERTY(QString shippingAddress READ shippingAddress WRITE setShippingAddress NOTIFY shippingAddressChanged)
    Q_PROPERTY(QString poNo READ poNo WRITE setPoNo NOTIFY poNoChanged)
    Q_PROPERTY(QString narration READ narration WRITE setNarration NOTIFY narrationChanged)

    // Line Items Model
    Q_PROPERTY(SalesLineItemsModel* lineItemsModel READ lineItemsModel CONSTANT)

    // Status / Messages
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(bool isError READ isError NOTIFY statusChanged)

public:
    explicit SalesVoucherController(QObject *parent = nullptr);

    int editingInvoiceId() const { return m_editingInvoiceId; }
    void setEditingInvoiceId(int id);
    bool isEditMode() const { return m_editingInvoiceId > 0; }

    QString voucherNo() const { return m_voucherNo; }
    void setVoucherNo(const QString &v) { if (m_voucherNo != v) { m_voucherNo = v; emit voucherNoChanged(); } }

    QString invoiceNo() const { return m_invoiceNo; }
    void setInvoiceNo(const QString &v) { if (m_invoiceNo != v) { m_invoiceNo = v; emit invoiceNoChanged(); } }

    QString invoiceDate() const { return m_invoiceDate; }
    void setInvoiceDate(const QString &v);

    QString dayOfWeek() const;

    QString marketType() const { return m_marketType; }
    void setMarketType(const QString &v) { if (m_marketType != v) { m_marketType = v; emit marketTypeChanged(); } }

    QString saleStatus() const { return m_saleStatus; }
    void setSaleStatus(const QString &v) { if (m_saleStatus != v) { m_saleStatus = v; emit saleStatusChanged(); } }

    QString paymentMode() const { return m_paymentMode; }
    void setPaymentMode(const QString &v) { if (m_paymentMode != v) { m_paymentMode = v; emit paymentModeChanged(); } }

    QString taxStatus() const { return m_taxStatus; }
    void setTaxStatus(const QString &v) { if (m_taxStatus != v) { m_taxStatus = v; emit taxStatusChanged(); recalculateTotals(); } }

    QString partyLedger() const { return m_partyLedger; }
    void setPartyLedger(const QString &v) { if (m_partyLedger != v) { m_partyLedger = v; emit partyLedgerChanged(); } }

    QString gstin() const { return m_gstin; }
    void setGstin(const QString &v);

    int dueDays() const { return m_dueDays; }
    void setDueDays(int d) { if (m_dueDays != d) { m_dueDays = d; emit dueDaysChanged(); } }

    QString salesAccount() const { return m_salesAccount; }
    void setSalesAccount(const QString &v) { if (m_salesAccount != v) { m_salesAccount = v; emit salesAccountChanged(); } }

    int totalBags() const { return m_totalBags; }
    double totalWeightQtl() const { return m_totalWeightQtl; }
    double taxableAmount() const { return m_taxableAmount; }

    double gstRate() const { return m_gstRate; }
    void setGstRate(double r) { if (std::abs(m_gstRate - r) > 0.001) { m_gstRate = r; emit gstRateChanged(); recalculateTotals(); } }

    bool isInterstate() const { return m_isInterstate; }
    void setIsInterstate(bool b) { if (m_isInterstate != b) { m_isInterstate = b; emit isInterstateChanged(); recalculateTotals(); } }

    double cgstAmount() const { return m_cgstAmount; }
    double sgstAmount() const { return m_sgstAmount; }
    double igstAmount() const { return m_igstAmount; }
    double totalTaxAmount() const { return m_totalTaxAmount; }

    double freightCharges() const { return m_freightCharges; }
    void setFreightCharges(double f) { if (std::abs(m_freightCharges - f) > 0.001) { m_freightCharges = f; recalculateTotals(); } }

    double tcsRate() const { return m_tcsRate; }
    void setTcsRate(double r) { if (std::abs(m_tcsRate - r) > 0.001) { m_tcsRate = r; recalculateTotals(); } }

    double tcsAmount() const { return m_tcsAmount; }
    double roundOff() const { return m_roundOff; }
    double grandTotal() const { return m_grandTotal; }

    QString taxableAmountFmt() const;
    QString cgstAmountFmt() const;
    QString sgstAmountFmt() const;
    QString igstAmountFmt() const;
    QString roundOffFmt() const;
    QString grandTotalFmt() const;

    double dami() const { return m_dami; }
    void setDami(double v) { if (std::abs(m_dami - v) > 0.001) { m_dami = v; recalculateTotals(); } }

    double labour() const { return m_labour; }
    void setLabour(double v) { if (std::abs(m_labour - v) > 0.001) { m_labour = v; recalculateTotals(); } }

    double auction() const { return m_auction; }
    void setAuction(double v) { if (std::abs(m_auction - v) > 0.001) { m_auction = v; recalculateTotals(); } }

    double marketFee() const { return m_marketFee; }
    void setMarketFee(double v) { if (std::abs(m_marketFee - v) > 0.001) { m_marketFee = v; recalculateTotals(); } }

    double hrdf() const { return m_hrdf; }
    void setHrdf(double v) { if (std::abs(m_hrdf - v) > 0.001) { m_hrdf = v; recalculateTotals(); } }

    double otherExp() const { return m_otherExp; }
    void setOtherExp(double v) { if (std::abs(m_otherExp - v) > 0.001) { m_otherExp = v; recalculateTotals(); } }

    QString vehicleNo() const { return m_vehicleNo; }
    void setVehicleNo(const QString &v) { if (m_vehicleNo != v) { m_vehicleNo = v; emit vehicleNoChanged(); } }

    QString ewayBillNo() const { return m_ewayBillNo; }
    void setEwayBillNo(const QString &v) { if (m_ewayBillNo != v) { m_ewayBillNo = v; emit ewayBillNoChanged(); } }

    QString grNo() const { return m_grNo; }
    void setGrNo(const QString &v) { if (m_grNo != v) { m_grNo = v; emit grNoChanged(); } }

    QString transportName() const { return m_transportName; }
    void setTransportName(const QString &v) { if (m_transportName != v) { m_transportName = v; emit transportNameChanged(); } }

    QString shippingAddress() const { return m_shippingAddress; }
    void setShippingAddress(const QString &v) { if (m_shippingAddress != v) { m_shippingAddress = v; emit shippingAddressChanged(); } }

    QString poNo() const { return m_poNo; }
    void setPoNo(const QString &v) { if (m_poNo != v) { m_poNo = v; emit poNoChanged(); } }

    QString narration() const { return m_narration; }
    void setNarration(const QString &v) { if (m_narration != v) { m_narration = v; emit narrationChanged(); } }

    SalesLineItemsModel* lineItemsModel() { return &m_lineItemsModel; }

    QString statusMessage() const { return m_statusMessage; }
    bool isError() const { return m_isError; }

    // Invokable Business Actions
    Q_INVOKABLE void resetForm(const QString &workingDate = "");
    Q_INVOKABLE bool loadInvoice(int invoiceId);
    Q_INVOKABLE bool loadInvoiceByNo(const QString &invNo);
    Q_INVOKABLE void recalculateTotals();
    Q_INVOKABLE bool saveVoucher();

signals:
    void editingInvoiceIdChanged();
    void isEditModeChanged();
    void voucherNoChanged();
    void invoiceNoChanged();
    void invoiceDateChanged();
    void dayOfWeekChanged();
    void marketTypeChanged();
    void saleStatusChanged();
    void paymentModeChanged();
    void taxStatusChanged();
    void partyLedgerChanged();
    void gstinChanged();
    void dueDaysChanged();
    void salesAccountChanged();
    void gstRateChanged();
    void isInterstateChanged();
    void totalsChanged();
    void vehicleNoChanged();
    void ewayBillNoChanged();
    void grNoChanged();
    void transportNameChanged();
    void shippingAddressChanged();
    void poNoChanged();
    void narrationChanged();
    void statusChanged();
    void savedSuccess(const QString &savedInvoiceNo);

private:
    int m_editingInvoiceId = 0;
    QString m_voucherNo;
    QString m_invoiceNo;
    QString m_invoiceDate;
    QString m_marketType = "Market Type (With Stock)";
    QString m_saleStatus = "Self Sale";
    QString m_paymentMode = "Credit";
    QString m_taxStatus = "GST / Exempt";

    QString m_partyLedger;
    QString m_gstin;
    int m_dueDays = 30;
    QString m_salesAccount = "Sales Account";

    int m_totalBags = 0;
    double m_totalWeightQtl = 0.0;
    double m_taxableAmount = 0.0;
    double m_gstRate = 5.0;
    bool m_isInterstate = false;
    double m_cgstAmount = 0.0;
    double m_sgstAmount = 0.0;
    double m_igstAmount = 0.0;
    double m_totalTaxAmount = 0.0;
    double m_freightCharges = 0.0;
    double m_tcsRate = 0.0;
    double m_tcsAmount = 0.0;
    double m_roundOff = 0.0;
    double m_grandTotal = 0.0;

    double m_dami = 0.0;
    double m_labour = 0.0;
    double m_auction = 0.0;
    double m_marketFee = 0.0;
    double m_hrdf = 0.0;
    double m_otherExp = 0.0;

    QString m_vehicleNo;
    QString m_ewayBillNo;
    QString m_grNo;
    QString m_transportName;
    QString m_shippingAddress;
    QString m_poNo;
    QString m_narration;

    QString m_statusMessage;
    bool m_isError = false;

    SalesLineItemsModel m_lineItemsModel;
};
