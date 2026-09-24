#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>
#include <QVariantMap>
#include <QVariantList>

namespace MahadevERP {

enum class StockViewMode {
    OnlyStock,          // Physical stock quantities only (No monetary clutter)
    StockWithAmount,    // Quantities + Financial Valuation & Rates
    ProfitLoss,         // Item/Group/HSN Wise Gross Profit & Loss Margins
    MonthlyDaily        // Chronological Item Monthly / Daily Timeline Breakdown
};

enum class StockGrouping {
    ItemWise,
    GroupWise,
    CompanyWise,
    TotalSummary,
    HsnWise
};

struct StockRegisterEntry {
    int id = 0;
    QString nameVal;       // Item Name, Group Name, Company Name, or HSN Code
    QString codeVal;       // Code / identifier
    QString typeVal;       // Commodity Type
    QString groupVal;      // Group Name
    QString companyVal;    // Company Name
    QString hsnVal;        // HSN Code
    QString unitVal = "Qtl";
    int itemsCount = 1;

    // Quantities & Bags
    long long opBags = 0;
    double opQty = 0.0;
    QString opQtyVal;

    long long inBags = 0;
    double inQty = 0.0;
    QString inQtyVal;

    long long outBags = 0;
    double outQty = 0.0;
    QString outQtyVal;

    long long closeBags = 0;
    double closeQty = 0.0;
    QString closeQtyVal;

    // Monetary Valuation (Stock with Amount)
    double opVal = 0.0;
    QString opValVal;

    double inVal = 0.0;
    QString inValVal;

    double outVal = 0.0;
    QString outValVal;

    double rate = 0.0;        // Valuation rate
    QString rateVal;

    double closeVal = 0.0;    // Closing inventory valuation
    QString closeValVal;

    // Profit & Loss Performance (Item Wise P&L)
    double salesQty = 0.0;
    double salesValue = 0.0;
    QString salesValueVal;

    double avgSaleRate = 0.0;
    QString avgSaleRateVal;

    double costRate = 0.0;
    QString costRateVal;

    double cogs = 0.0;
    QString cogsVal;

    double grossProfit = 0.0;
    QString grossProfitVal;

    double gpMarginPct = 0.0;
    QString gpMarginPctVal;

    // Timeline attributes (for Monthly / Daily mode)
    QString periodDate;       // e.g. "Apr 2025" or "14-04-2025"
    QString voucherNo;
    QString voucherType;
    QString partyName;
    double runningBalance = 0.0;
    QString runningBalanceVal;
};

class StockRegisterModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        IdValRole = Qt::UserRole + 1,
        NameValRole,
        CodeValRole,
        TypeValRole,
        GroupValRole,
        CompanyValRole,
        HsnValRole,
        UnitValRole,
        ItemsCountRole,

        OpBagsRole,
        InBagsRole,
        OutBagsRole,
        CloseBagsRole,

        OpQtyValRole,
        InQtyValRole,
        OutQtyValRole,
        CloseQtyValRole,
        CloseQtyNumRole,

        OpValValRole,
        InValValRole,
        OutValValRole,
        RateValRole,
        CloseValValRole,
        CloseValNumRole,

        SalesQtyRole,
        SalesValueValRole,
        AvgSaleRateValRole,
        CostRateValRole,
        CogsValRole,
        GrossProfitValRole,
        GrossProfitNumRole,
        GpMarginPctValRole,

        PeriodDateRole,
        VoucherNoRole,
        VoucherTypeRole,
        PartyNameRole,
        RunningBalanceValRole
    };

    explicit StockRegisterModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(const QVector<StockRegisterEntry>& entries);
    void clear();
    Q_INVOKABLE QVariantMap get(int row) const;
    const QVector<StockRegisterEntry>& entries() const { return m_entries; }

signals:
    void countChanged();

private:
    QVector<StockRegisterEntry> m_entries;
};

class StockRegisterController : public QObject {
    Q_OBJECT
    Q_PROPERTY(StockRegisterModel* model READ model CONSTANT)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)

    // Summary Totals
    Q_PROPERTY(int totalItemsCount READ totalItemsCount NOTIFY totalsChanged)
    Q_PROPERTY(double totalOpeningQty READ totalOpeningQty NOTIFY totalsChanged)
    Q_PROPERTY(double totalInwardQty READ totalInwardQty NOTIFY totalsChanged)
    Q_PROPERTY(double totalOutwardQty READ totalOutwardQty NOTIFY totalsChanged)
    Q_PROPERTY(double totalClosingQty READ totalClosingQty NOTIFY totalsChanged)
    Q_PROPERTY(double totalClosingVal READ totalClosingVal NOTIFY totalsChanged)

    Q_PROPERTY(double totalSalesTurnover READ totalSalesTurnover NOTIFY totalsChanged)
    Q_PROPERTY(double totalCogs READ totalCogs NOTIFY totalsChanged)
    Q_PROPERTY(double totalGrossProfit READ totalGrossProfit NOTIFY totalsChanged)
    Q_PROPERTY(double overallGpMargin READ overallGpMargin NOTIFY totalsChanged)

    Q_PROPERTY(QString totalClosingQtyFmt READ totalClosingQtyFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString totalClosingValFmt READ totalClosingValFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString totalInwardQtyFmt READ totalInwardQtyFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString totalOutwardQtyFmt READ totalOutwardQtyFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString totalSalesTurnoverFmt READ totalSalesTurnoverFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString totalCogsFmt READ totalCogsFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString totalGrossProfitFmt READ totalGrossProfitFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString overallGpMarginFmt READ overallGpMarginFmt NOTIFY totalsChanged)

public:
    explicit StockRegisterController(QObject* parent = nullptr);

    StockRegisterModel* model() { return &m_model; }

    StockViewMode viewMode() const { return m_viewMode; }
    void setViewMode(StockViewMode mode);

    StockGrouping grouping() const { return m_grouping; }
    void setGrouping(StockGrouping grouping);

    int selectedItemId() const { return m_selectedItemId; }
    void setSelectedItemId(int itemId);

    bool isDailyDetail() const { return m_isDailyDetail; }
    void setIsDailyDetail(bool b);

    QString searchQuery() const { return m_searchQuery; }
    void setSearchQuery(const QString& query);

    int totalItemsCount() const { return m_totalItemsCount; }
    double totalOpeningQty() const { return m_totalOpeningQty; }
    double totalInwardQty() const { return m_totalInwardQty; }
    double totalOutwardQty() const { return m_totalOutwardQty; }
    double totalClosingQty() const { return m_totalClosingQty; }
    double totalClosingVal() const { return m_totalClosingVal; }

    double totalSalesTurnover() const { return m_totalSalesTurnover; }
    double totalCogs() const { return m_totalCogs; }
    double totalGrossProfit() const { return m_totalGrossProfit; }
    double overallGpMargin() const { return m_overallGpMargin; }

    QString totalClosingQtyFmt() const { return m_totalClosingQtyFmt; }
    QString totalClosingValFmt() const { return m_totalClosingValFmt; }
    QString totalInwardQtyFmt() const { return m_totalInwardQtyFmt; }
    QString totalOutwardQtyFmt() const { return m_totalOutwardQtyFmt; }
    QString totalSalesTurnoverFmt() const { return m_totalSalesTurnoverFmt; }
    QString totalCogsFmt() const { return m_totalCogsFmt; }
    QString totalGrossProfitFmt() const { return m_totalGrossProfitFmt; }
    QString overallGpMarginFmt() const { return m_overallGpMarginFmt; }

    Q_INVOKABLE void reload(const QString& fromDate = "", const QString& toDate = "");

signals:
    void searchQueryChanged();
    void totalsChanged();
    void configChanged();

private:
    void fetchRawData(const QString& fromDate, const QString& toDate);
    void applyFilter();

    StockRegisterModel m_model;
    QVector<StockRegisterEntry> m_allEntries;
    QString m_searchQuery;

    StockViewMode m_viewMode = StockViewMode::OnlyStock;
    StockGrouping m_grouping = StockGrouping::ItemWise;
    int m_selectedItemId = 0;
    bool m_isDailyDetail = false;

    QString m_currentFromDate;
    QString m_currentToDate;

    // Aggregates
    int m_totalItemsCount = 0;
    double m_totalOpeningQty = 0.0;
    double m_totalInwardQty = 0.0;
    double m_totalOutwardQty = 0.0;
    double m_totalClosingQty = 0.0;
    double m_totalClosingVal = 0.0;

    double m_totalSalesTurnover = 0.0;
    double m_totalCogs = 0.0;
    double m_totalGrossProfit = 0.0;
    double m_overallGpMargin = 0.0;

    QString m_totalClosingQtyFmt = "0.00 Qtl";
    QString m_totalClosingValFmt = "₹0.00";
    QString m_totalInwardQtyFmt = "0.00 Qtl";
    QString m_totalOutwardQtyFmt = "0.00 Qtl";
    QString m_totalSalesTurnoverFmt = "₹0.00";
    QString m_totalCogsFmt = "₹0.00";
    QString m_totalGrossProfitFmt = "₹0.00";
    QString m_overallGpMarginFmt = "0.00%";
};

} // namespace MahadevERP

using MahadevERP::StockViewMode;
using MahadevERP::StockGrouping;
using MahadevERP::StockRegisterEntry;
using MahadevERP::StockRegisterModel;
using MahadevERP::StockRegisterController;
