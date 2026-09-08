#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>
#include <QVariantMap>
#include <QVariantList>

struct StockRegisterEntry {
    int id = 0;
    QString nameVal;
    QString codeVal;
    QString typeVal;
    QString unitVal = "Qtl";
    long long opBags = 0;
    long long inBags = 0;
    long long outBags = 0;
    long long closeBags = 0;
    double opQty = 0.0;
    QString opQtyVal;
    double inQty = 0.0;
    QString inQtyVal;
    double outQty = 0.0;
    QString outQtyVal;
    double closeQty = 0.0;
    QString closeQtyVal;
    double rate = 0.0;
    QString rateVal;
    double closeVal = 0.0;
    QString closeValVal;
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
        UnitValRole,
        OpBagsRole,
        InBagsRole,
        OutBagsRole,
        CloseBagsRole,
        OpQtyValRole,
        InQtyValRole,
        OutQtyValRole,
        CloseQtyValRole,
        CloseQtyNumRole,
        RateValRole,
        CloseValValRole,
        CloseValNumRole
    };

    explicit StockRegisterModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(const QVector<StockRegisterEntry>& entries);
    void clear();
    Q_INVOKABLE QVariantMap get(int row) const;

signals:
    void countChanged();

private:
    QVector<StockRegisterEntry> m_entries;
};

class StockRegisterController : public QObject {
    Q_OBJECT
    Q_PROPERTY(StockRegisterModel* model READ model CONSTANT)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
    Q_PROPERTY(int totalItemsCount READ totalItemsCount NOTIFY totalsChanged)
    Q_PROPERTY(double totalClosingQty READ totalClosingQty NOTIFY totalsChanged)
    Q_PROPERTY(double totalClosingVal READ totalClosingVal NOTIFY totalsChanged)
    Q_PROPERTY(QString totalClosingQtyFmt READ totalClosingQtyFmt NOTIFY totalsChanged)
    Q_PROPERTY(QString totalClosingValFmt READ totalClosingValFmt NOTIFY totalsChanged)

public:
    explicit StockRegisterController(QObject* parent = nullptr);

    StockRegisterModel* model() { return &m_model; }

    QString searchQuery() const { return m_searchQuery; }
    void setSearchQuery(const QString& query);

    int totalItemsCount() const { return m_totalItemsCount; }
    double totalClosingQty() const { return m_totalClosingQty; }
    double totalClosingVal() const { return m_totalClosingVal; }

    QString totalClosingQtyFmt() const { return m_totalClosingQtyFmt; }
    QString totalClosingValFmt() const { return m_totalClosingValFmt; }

    Q_INVOKABLE void reload(const QString& fromDate = "", const QString& toDate = "");

signals:
    Q_SIGNAL void searchQueryChanged();
    Q_SIGNAL void totalsChanged();

private:
    void applyFilter();

    StockRegisterModel m_model;
    QVector<StockRegisterEntry> m_allEntries;
    QString m_searchQuery;

    int m_totalItemsCount = 0;
    double m_totalClosingQty = 0.0;
    double m_totalClosingVal = 0.0;

    QString m_totalClosingQtyFmt = "0.00 Qtl";
    QString m_totalClosingValFmt = "₹0.00";
};
