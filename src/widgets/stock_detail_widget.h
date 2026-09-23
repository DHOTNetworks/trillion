#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QDateEdit>
#include <QPushButton>
#include <QLabel>
#include "../models/stock_register_model.h"
#include "../services/print_export_controller.h"

namespace MahadevERP {

class StockDetailWidget : public QWidget {
    Q_OBJECT

public:
    explicit StockDetailWidget(StockRegisterController* controller = nullptr, PrintExportController* printExportCtrl = nullptr, QWidget* parent = nullptr);

    void reloadData(const QString& fromDate = "", const QString& toDate = "");
    void focusTable();

signals:
    void backRequested();
    void openItemMovement(const QString& itemName);

private slots:
    void onSearchChanged(const QString& query);
    void onDateFilterChanged();
    void onTableDoubleClicked(int row, int col);
    void onPrintRegister();
    void onExportPdf();
    void onExportCsv();

private:
    void setupUi();
    void populateTable();

    StockRegisterController* m_controller = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;

    QDateEdit* m_fromDateEdit = nullptr;
    QDateEdit* m_toDateEdit = nullptr;
    QLineEdit* m_searchEdit = nullptr;

    // Metric Cards
    QLabel* m_totalItemsLabel = nullptr;
    QLabel* m_totalClosingQtyLabel = nullptr;
    QLabel* m_totalClosingValLabel = nullptr;

    QTableWidget* m_table = nullptr;
};

} // namespace MahadevERP
