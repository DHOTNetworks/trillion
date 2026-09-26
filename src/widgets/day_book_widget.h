#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QDateEdit>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "../services/print_export_controller.h"

namespace MahadevERP {

class DayBookWidget : public QWidget {
    Q_OBJECT

public:
    explicit DayBookWidget(PrintExportController* printExportCtrl = nullptr, QWidget* parent = nullptr);

    void loadDayBookData(const QDate& fromDate, const QDate& toDate, const QString& voucherTypeFilter = "ALL");

signals:
    void backRequested();
    void alterVoucherRequested(int targetViewIndex, const QVariantMap& entry);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onRefreshClicked();
    void onRowDoubleClicked(int row, int column);
    void onExportPdfClicked();
    void onExportOdfClicked();
    void onExportExcelClicked();
    void onPrintClicked();
    void onDateChanged();

private:
    void setupUi();
    void applyCustomStyles();

    QDateEdit* m_fromDateEdit = nullptr;
    QDateEdit* m_toDateEdit = nullptr;
    QComboBox* m_typeFilterCombo = nullptr;
    QLineEdit* m_searchBox = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QPushButton* m_pdfBtn = nullptr;
    QPushButton* m_backBtn = nullptr;

    QTableWidget* m_table = nullptr;
    QLabel* m_totalDrLabel = nullptr;
    QLabel* m_totalCrLabel = nullptr;
    QLabel* m_rowCountLabel = nullptr;

    PrintExportController* m_printExportCtrl = nullptr;
    QVariantList m_records;
};

} // namespace MahadevERP
