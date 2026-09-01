#pragma once

#include <QObject>
#include <QString>

class DashboardController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString paddyStock READ paddyStock NOTIFY statsChanged)
    Q_PROPERTY(QString riceStock READ riceStock NOTIFY statsChanged)
    Q_PROPERTY(QString totalSales READ totalSales NOTIFY statsChanged)
    Q_PROPERTY(QString totalProcurement READ totalProcurement NOTIFY statsChanged)
    Q_PROPERTY(QString millingEfficiency READ millingEfficiency NOTIFY statsChanged)

public:
    explicit DashboardController(QObject* parent = nullptr);

    QString paddyStock() const { return m_paddyStock; }
    QString riceStock() const { return m_riceStock; }
    QString totalSales() const { return m_totalSales; }
    QString totalProcurement() const { return m_totalProcurement; }
    QString millingEfficiency() const { return m_millingEfficiency; }

    Q_INVOKABLE void refresh_stats(const QString& fromDate = "", const QString& toDate = "", const QString& fyLabel = "");
    Q_INVOKABLE QString format_inr(double amount);
    Q_INVOKABLE QString format_inr(const QString& amount);
    Q_INVOKABLE QString format_qty(double qty);
    Q_INVOKABLE QString format_qty(const QString& qty);

signals:
    void statsChanged();

private:
    QString m_paddyStock = "0.0 Qtl";
    QString m_riceStock = "0.0 Qtl";
    QString m_totalSales = "₹0.00";
    QString m_totalProcurement = "₹0.00";
    QString m_millingEfficiency = "0.0%";
};
