#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QDateEdit>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include "../models/salary_register_controller.h"
#include "../services/print_export_controller.h"

namespace MahadevERP {

class SalaryRegisterWidget : public QWidget {
    Q_OBJECT

public:
    explicit SalaryRegisterWidget(SalaryRegisterController* controller, PrintExportController* printExportCtrl, QWidget* parent = nullptr);

    void loadData();
    bool isDirty() const { return m_isDirty; }
    void markDirty() { m_isDirty = true; }
    void focusTable();

signals:
    void backRequested();
    void statementRequested(const QString& partyName);

private slots:
    void onPeriodChanged();
    void onFilterChanged();
    void onSearchChanged(const QString& text);
    void onCellChanged(int row, int column);
    void onCellDoubleClicked(int row, int column);
    void onSelectAllClicked();
    void onDeselectAllClicked();
    void onSelectPendingClicked();
    void onPostSalariesClicked();
    void onSyncMasterClicked();
    void onExportCsv();
    void onExportPdf();
    void onUpdateSummary();

private:
    SalaryRegisterController* m_controller = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;
    bool m_isDirty = true;
    bool m_isUpdatingTable = false;

    // UI Controls
    QComboBox* m_monthCombo = nullptr;
    QComboBox* m_yearCombo = nullptr;
    QDateEdit* m_voucherDateEdit = nullptr;
    QComboBox* m_statusCombo = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QTableWidget* m_table = nullptr;

    // Summary Labels
    QLabel* m_lblTotalEmployees = nullptr;
    QLabel* m_lblSelectedCount = nullptr;
    QLabel* m_lblTotalMaster = nullptr;
    QLabel* m_lblTotalDeductions = nullptr;
    QLabel* m_lblTotalNetPayable = nullptr;
    QLabel* m_lblTotalPosted = nullptr;

    void setupUi();
    void populateTable();
    void updateVoucherDateForSelectedMonth();
    int getSelectedMonthNumber() const;
    int getSelectedYearNumber() const;
    void applyRowVisibility(int row);
};

} // namespace MahadevERP
