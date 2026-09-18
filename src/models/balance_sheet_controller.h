#pragma once

#include <QObject>
#include <QString>
#include "../engine/balance_sheet_calculator.h"
#include "../services/print_export_controller.h"

class BalanceSheetController : public QObject {
    Q_OBJECT

public:
    explicit BalanceSheetController(PrintExportController* printExportCtrl = nullptr, QObject* parent = nullptr);

    const BalanceSheetData& data() const { return m_data; }

    QString asOnDate() const { return m_data.asOnDate; }
    QString displayAsOnDate() const { return m_data.displayAsOnDate; }
    QString financialYear() const { return m_data.financialYear; }
    QString firmName() const { return m_data.firmName; }

    double totalLiabilities() const { return m_data.totalLiabilities; }
    QString totalLiabilitiesFmt() const { return m_data.totalLiabilitiesFmt; }

    double totalAssets() const { return m_data.totalAssets; }
    QString totalAssetsFmt() const { return m_data.totalAssetsFmt; }

    double difference() const { return m_data.difference; }
    QString differenceFmt() const;
    bool isBalanced() const { return m_data.isBalanced; }

    double netProfit() const { return m_data.netProfit; }
    QString netProfitFmt() const;

    double closingStockValue() const { return m_data.closingStockValue; }
    QString closingStockValueFmt() const;

public slots:
    void reload(const QString& requestedAsOnDate = "");
    QString exportPdf(const QString& customPath = "");
    QString exportCsv(const QString& customPath = "");
    bool print();

signals:
    void dataChanged();
    void totalsChanged();
    void exportCompleted(bool success, const QString& pathOrMsg);

private:
    QString renderHtml() const;

    BalanceSheetData m_data;
    PrintExportController* m_printExportCtrl = nullptr;
};
