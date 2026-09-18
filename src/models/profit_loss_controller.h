#pragma once

#include <QObject>
#include <QString>
#include "../engine/profit_loss_calculator.h"

class PrintExportController;

class ProfitLossController : public QObject {
    Q_OBJECT

public:
    explicit ProfitLossController(PrintExportController* printExportCtrl = nullptr, QObject* parent = nullptr);

    const ProfitLossData& data() const { return m_data; }

    QString fromDate() const { return m_data.fromDate; }
    QString toDate() const { return m_data.toDate; }
    QString displayFromDate() const { return m_data.displayFromDate; }
    QString displayToDate() const { return m_data.displayToDate; }
    QString financialYear() const { return m_data.financialYear; }
    QString firmName() const { return m_data.firmName; }

    double grossProfit() const { return m_data.grossProfit; }
    double grossLoss() const { return m_data.grossLoss; }
    double netProfit() const { return m_data.netProfit; }
    double netLoss() const { return m_data.netLoss; }

    QString grossProfitFmt() const;
    QString netProfitFmt() const;

    double totalTradingDr() const { return m_data.totalTradingDr; }
    double totalTradingCr() const { return m_data.totalTradingCr; }
    double totalPlDr() const { return m_data.totalPlDr; }
    double totalPlCr() const { return m_data.totalPlCr; }

    QString totalTradingDrFmt() const { return m_data.totalTradingDrFmt; }
    QString totalTradingCrFmt() const { return m_data.totalTradingCrFmt; }
    QString totalPlDrFmt() const { return m_data.totalPlDrFmt; }
    QString totalPlCrFmt() const { return m_data.totalPlCrFmt; }

public slots:
    void reload(const QString& requestedFromDate = "", const QString& requestedToDate = "");
    QString exportPdf(const QString& customPath = "");
    QString exportCsv(const QString& customPath = "");
    bool print();

signals:
    void dataChanged();
    void totalsChanged();
    void exportCompleted(bool success, const QString& pathOrMsg);

private:
    QString renderHtml() const;

    ProfitLossData m_data;
    PrintExportController* m_printExportCtrl = nullptr;
};
