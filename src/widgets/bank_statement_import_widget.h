#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include "../models/bank_statement_controller.h"
#include "../services/print_export_controller.h"

class KbdBadgeButton;

namespace MahadevERP {

class BankStatementImportWidget : public QWidget {
    Q_OBJECT

public:
    explicit BankStatementImportWidget(BankStatementController* controller = nullptr,
                                       PrintExportController* printExportCtrl = nullptr,
                                       QWidget* parent = nullptr);

    void resetForm();

signals:
    void backRequested();
    void importCompleted();

private slots:
    void onSelectFileClicked();
    void onFilterChanged();
    void onSelectAllToggled(bool checked);
    void onDeselectDuplicatesClicked();
    void onPostSelectedClicked();
    void onTableItemChanged(QTableWidgetItem* item);

protected:
    void showEvent(QShowEvent* event) override;

private:
    void setupUi();
    void populateBankLedgers();
    void refreshTable();
    void updateSummary();

    BankStatementController* m_controller = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;

    QLineEdit* m_filePathEdit = nullptr;
    QComboBox* m_bankLedgerCombo = nullptr;
    QComboBox* m_filterTypeCombo = nullptr;
    QLineEdit* m_searchEdit = nullptr;

    // Stat Cards
    QLabel* m_totalCountLabel = nullptr;
    QLabel* m_totalWithdrawalsLabel = nullptr;
    QLabel* m_totalDepositsLabel = nullptr;
    QLabel* m_duplicateCountLabel = nullptr;
    QLabel* m_selectedCountLabel = nullptr;

    QTableWidget* m_table = nullptr;
    QCheckBox* m_selectAllCheck = nullptr;
    KbdBadgeButton* m_postBtn = nullptr;
    class AccountSearchDelegate* m_accountDelegate = nullptr;
};

} // namespace MahadevERP
