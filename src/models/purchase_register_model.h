#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>
#include <QVariantMap>
#include <QVariantList>

struct PurchaseRegisterEntry {
    int id = 0;
    QString vchNoVal;
    QString invNoVal;
    QString dateVal;
    QString suppVal;
    QString itemVal;
    long long bagsVal = 0;
    double weightVal = 0.0;
    double rateVal = 0.0;
    QString rateFmt;
    double taxableVal = 0.0;
    QString taxableFmt;
    double gstVal = 0.0;
    QString gstFmt;
    double totalVal = 0.0;
    QString totalFmt;
    QString vehVal;
};

class PurchaseRegisterModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        VchNoValRole,
        InvNoValRole,
        DateValRole,
        SuppValRole,
        ItemValRole,
        BagsValRole,
        WeightValRole,
        RateValRole,
        RateFmtRole,
        TaxableValRole,
        TaxableFmtRole,
        GstValRole,
        GstFmtRole,
        TotalValRole,
        TotalFmtRole,
        VehValRole
    };

    explicit PurchaseRegisterModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(const QVector<PurchaseRegisterEntry>& entries);
    void clear();
    Q_INVOKABLE QVariantMap get(int row) const;

signals:
    void countChanged();

private:
    QVector<PurchaseRegisterEntry> m_entries;
};

class PurchaseRegisterController : public QObject {
    Q_OBJECT
    Q_PROPERTY(PurchaseRegisterModel* model READ model CONSTANT)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
    Q_PROPERTY(int totalInvoicesCount READ totalInvoicesCount NOTIFY totalsChanged)
    Q_PROPERTY(long long totalBagsCount READ totalBagsCount NOTIFY totalsChanged)
    Q_PROPERTY(double totalWeightQtl READ totalWeightQtl NOTIFY totalsChanged)
    Q_PROPERTY(double totalTaxableAmt READ totalTaxableAmt NOTIFY totalsChanged)
    Q_PROPERTY(double totalGstAmt READ totalGstAmt NOTIFY totalsChanged)
    Q_PROPERTY(double totalGrossAmt READ totalGrossAmt NOTIFY totalsChanged)
    Q_PROPERTY(QString totalWeightFmt READ totalWeightFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString totalTaxableFmt READ totalTaxableFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString totalGrossFmt READ totalGrossFmt NOTIFY totalsChanged)

public:
    explicit PurchaseRegisterController(QObject* parent = nullptr);

    PurchaseRegisterModel* model() { return &m_model; }

    QString searchQuery() const { return m_searchQuery; }
    void setSearchQuery(const QString& query);

    int totalInvoicesCount() const { return m_totalInvoicesCount; }
    long long totalBagsCount() const { return m_totalBagsCount; }
    double totalWeightQtl() const { return m_totalWeightQtl; }
    double totalTaxableAmt() const { return m_totalTaxableAmt; }
    double totalGstAmt() const { return m_totalGstAmt; }
    double totalGrossAmt() const { return m_totalGrossAmt; }

    QString totalWeightFmt() const { return m_totalWeightFmt; }
    QString totalTaxableFmt() const { return m_totalTaxableFmt; }
    QString totalGrossFmt() const { return m_totalGrossFmt; }

    Q_INVOKABLE void reload(const QString& fromDate = "", const QString& toDate = "");

signals:
    Q_SIGNAL void searchQueryChanged();
    Q_SIGNAL void totalsChanged();

private:
    void applyFilter();

    PurchaseRegisterModel m_model;
    QVector<PurchaseRegisterEntry> m_allEntries;
    QString m_searchQuery;

    int m_totalInvoicesCount = 0;
    long long m_totalBagsCount = 0;
    double m_totalWeightQtl = 0.0;
    double m_totalTaxableAmt = 0.0;
    double m_totalGstAmt = 0.0;
    double m_totalGrossAmt = 0.0;

    QString m_totalWeightFmt = "0.00 Qtl";
    QString m_totalTaxableFmt = "₹0.00";
    QString m_totalGrossFmt = "₹0.00";
};
