#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QGroupBox>
#include <QFrame>
#include "accounting_date_edit.h"
#include "party_search_widget.h"
#include "item_search_delegate.h"
#include "../models/jform_model.h"
#include "../models/parties_model.h"
#include "../models/stock_items_model.h"
#include "../services/print_export_controller.h"

class JFormVoucherWidget : public QWidget {
    Q_OBJECT

public:
    explicit JFormVoucherWidget(PrintExportController* printExportCtrl = nullptr,
                                QWidget* parent = nullptr);
    ~JFormVoucherWidget() override;

    bool isEditMode() const { return m_editingVoucherId > 0; }
    int editingVoucherId() const { return m_editingVoucherId; }

    void resetForm();
    bool loadVoucherForEditing(const QVariant& vchNoOrId, const QString& dateHint = "");
    void setWorkingDate(const QString& dateStr);

public slots:
    void openDateDialog(bool isInitial = false);
    void openAlterVoucherDialog();
    void loadPreviousVoucher();
    void loadNextVoucher();
    void saveVoucher();
    void deleteVoucher();
    void printVoucher();
    void exportPdf();
    void addNewLineRow();
    void removeLineRow(int row);
    void recalculateTotals();
    void advanceCell();
    void retreatCell();
    void moveCell(int row, int col);
    void focusTableAt(int row, int col);

signals:
    void backRequested();
    void voucherSaved(const QString& jformNo);
    void voucherDeleted(const QString& jformNo);

protected:
    void showEvent(QShowEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onDateChanged(const QDate& date);
    void onFarmerSelected(const QVariantMap& partyData);
    void onTableCellChanged(int row, int column);
    void onStockItemConfigured(int row, const QVariantMap& itemData);

private:
    void setupUi();
    void setupNavigationChains();
    void applyCustomStyles();
    void updateNextNumbers();
    void updateFiscalYearBadge();
    void updateDayOfWeek(const QDate& date);
    void populateRow(int row, const QString& itemName, int bags, double packing, double loose, double weight, double rate, double amount);
    void syncModelFromTable();
    void syncTableFromModel();
    void setStatusMessage(const QString& message, bool isError);

    JFormModel m_jformModel;
    PartiesModel m_partiesModel;
    StockItemsModel m_stockModel;
    PrintExportController* m_printExportCtrl = nullptr;

    int m_editingVoucherId = 0;
    QString m_editingJFormNo;
    QString m_editingVoucherNo;
    bool m_hasInitialDateOpened = false;

    // Top Bar
    QLabel* m_badgeLabel = nullptr;
    QLabel* m_voucherNoDisplay = nullptr;
    QLabel* m_jformNoDisplay = nullptr;
    QLineEdit* m_jformNoEdit = nullptr;
    AccountingDateEdit* m_dateEdit = nullptr;
    QLabel* m_dayOfWeekLabel = nullptr;
    QLabel* m_fyBadgeLabel = nullptr;

    // Farmer Search & Info
    PartySearchWidget* m_farmerSearch = nullptr;
    QLabel* m_farmerBalanceLabel = nullptr;
    QComboBox* m_saleStatusCombo = nullptr;
    QLineEdit* m_dueDaysEdit = nullptr;

    // Logistics info
    QLineEdit* m_vehicleNoEdit = nullptr;
    QLineEdit* m_brokerNameEdit = nullptr;

    // Table
    QTableWidget* m_table = nullptr;
    ItemSearchDelegate* m_itemDelegate = nullptr;

    // Calculations Summary
    QLabel* m_totalBagsLabel = nullptr;
    QLabel* m_totalWeightLabel = nullptr;
    QLabel* m_goodsAmountLabel = nullptr;
    QLineEdit* m_labourEdit = nullptr;
    QLineEdit* m_bonusEdit = nullptr;
    QLineEdit* m_roundOffEdit = nullptr;
    QLabel* m_grandTotalLabel = nullptr;
    QLineEdit* m_narrationEdit = nullptr;

    // Bottom Action Buttons
    QPushButton* m_saveButton = nullptr;
    QPushButton* m_deleteButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    QPushButton* m_dateButton = nullptr;
    QPushButton* m_printButton = nullptr;
    QLabel* m_statusLabel = nullptr;

    int m_selectedFarmerId = 0;
    QString m_selectedFarmerName;
};
