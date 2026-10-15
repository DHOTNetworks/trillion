#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QFrame>
#include "accounting_date_edit.h"
#include "account_search_box.h"
#include "kbd_badge_button.h"
#include "item_search_delegate.h"
#include "../models/iform_model.h"
#include "../models/parties_model.h"
#include "../models/stock_items_model.h"
#include "../services/print_export_controller.h"

class IFormVoucherWidget : public QWidget {
    Q_OBJECT

public:
    explicit IFormVoucherWidget(PrintExportController* printExportCtrl = nullptr,
                                QWidget* parent = nullptr);
    ~IFormVoucherWidget() override;

    bool isEditMode() const { return m_editingVoucherId > 0; }
    int editingVoucherId() const { return m_editingVoucherId; }

    void resetForm();
    // popupDateOnLoad: drill-down opens pop the date dialog first (dates are
    // never keyboard-focusable); after it closes focus lands on the buyer.
    // In-voucher browsing (prev/next/alter) keeps false.
    bool loadVoucherForEditing(const QVariant& vchNoOrId, const QString& dateHint = "", bool popupDateOnLoad = false);
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
    void voucherSaved(const QString& iformNo);
    void voucherDeleted(const QString& iformNo);

protected:
    void showEvent(QShowEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onDateChanged(const QDate& date);
    void onBuyerSelected(const QVariantMap& partyData);
    void onTableCellChanged(int row, int column);
    void onStockItemConfigured(int row, const QVariantMap& itemData);

private:
    void setupUi();
    void setupNavigationChains();
    void applyCustomStyles();
    void updateNextNumbers();
    void updateFiscalYearBadge();
    void updateDayOfWeek(const QDate& date);
    void populateRow(int row, const QString& itemName, int bags, double weight, double rate, double goodsAmt, double dami, double mfee, double hrdf, double lineTotal);
    void setStatusMessage(const QString& message, bool isError);

    IFormModel m_iformModel;
    PartiesModel m_partiesModel;
    StockItemsModel m_stockModel;
    PrintExportController* m_printExportCtrl = nullptr;

    int m_editingVoucherId = 0;
    QString m_editingIFormNo;
    QString m_editingVoucherNo;
    bool m_hasInitialDateOpened = false;

    // Top Bar
    QLabel* m_badgeLabel = nullptr;
    QLabel* m_voucherNoDisplay = nullptr;
    QLineEdit* m_iformNoEdit = nullptr;
    AccountingDateEdit* m_dateEdit = nullptr;
    QLabel* m_dayOfWeekLabel = nullptr;
    QLabel* m_fyBadgeLabel = nullptr;

    // Buyer Search & Info
    AccountSearchBox* m_buyerSearch = nullptr;
    QLabel* m_buyerBalanceLabel = nullptr;
    AccountSearchBox* m_brokerNameEdit = nullptr;
    QLineEdit* m_dueDaysEdit = nullptr;
    QLineEdit* m_vehicleNoEdit = nullptr;
    QLineEdit* m_grNoEdit = nullptr;

    // Table
    QTableWidget* m_table = nullptr;
    ItemSearchDelegate* m_itemDelegate = nullptr;
    // Guards cellChanged reentrancy during programmatic fills (sale pattern).
    bool m_isUpdatingTable = false;

    // Summary Frame
    QLabel* m_totalBagsLabel = nullptr;
    QLabel* m_totalWeightLabel = nullptr;
    QLabel* m_goodsAmountLabel = nullptr;
    QLabel* m_damiLabel = nullptr;
    QLabel* m_mandiFeeLabel = nullptr;
    QLabel* m_hrdfLabel = nullptr;
    QLineEdit* m_labourEdit = nullptr;
    QLineEdit* m_roundOffEdit = nullptr;
    QLabel* m_grandTotalLabel = nullptr;
    QLineEdit* m_narrationEdit = nullptr;

    // Bottom Action Buttons
    KbdBadgeButton* m_saveButton = nullptr;
    KbdBadgeButton* m_deleteButton = nullptr;
    KbdBadgeButton* m_cancelButton = nullptr;
    KbdBadgeButton* m_dateButton = nullptr;
    KbdBadgeButton* m_printButton = nullptr;
    QLabel* m_statusLabel = nullptr;

    int m_selectedBuyerId = 0;
    QString m_selectedBuyerName;
};
