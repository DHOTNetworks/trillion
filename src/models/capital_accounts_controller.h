#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QDate>
#include <QVariantMap>

namespace MahadevERP {

struct CapitalAccountItem {
    int ledgerId = 0;
    QString partnerName;      // Partner / Proprietor Name
    double opCapital = 0.0;   // Opening Balance as on 01-Apr
    double additions = 0.0;   // Capital introduced (Credits)
    double drawings = 0.0;    // Drawings taken (Debits)
    double profitShare = 0.0; // Share of Net Profit / (Loss)
    double interest = 0.0;    // Interest on Capital
    double closingCapital = 0.0; // Closing Capital Balance

    QString opCapitalFmt;
    QString additionsFmt;
    QString drawingsFmt;
    QString profitShareFmt;
    QString interestFmt;
    QString closingCapitalFmt;
};

struct CapitalAccountTotals {
    int totalPartners = 0;
    double totalOpCapital = 0.0;
    double totalAdditions = 0.0;
    double totalDrawings = 0.0;
    double totalProfitShare = 0.0;
    double totalInterest = 0.0;
    double totalClosingCapital = 0.0;

    QString totalOpCapitalFmt;
    QString totalAdditionsFmt;
    QString totalDrawingsFmt;
    QString totalProfitShareFmt;
    QString totalInterestFmt;
    QString totalClosingCapitalFmt;
};

class CapitalAccountsController : public QObject {
    Q_OBJECT

public:
    explicit CapitalAccountsController(QObject* parent = nullptr);

    QDate fromDate() const { return m_fromDate; }
    QDate toDate() const { return m_toDate; }
    void setDateRange(const QDate& fromDate, const QDate& toDate);

    void reload();

    const QVector<CapitalAccountItem>& items() const { return m_items; }
    const CapitalAccountTotals& totals() const { return m_totals; }

signals:
    void dataChanged();

private:
    void calculate();

    QDate m_fromDate;
    QDate m_toDate;
    QVector<CapitalAccountItem> m_items;
    CapitalAccountTotals m_totals;
};

} // namespace MahadevERP
