#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QVector>
#include <QKeyEvent>
#include <QShowEvent>
#include "../models/debit_credit_note_controller.h"
#include "../services/print_export_controller.h"
#include "account_search_box.h"
#include "item_search_delegate.h"
#include "accounting_date_edit.h"
#include "kbd_badge_button.h"

namespace MahadevERP {

class DebitCreditNoteWidget : public QWidget {
    Q_OBJECT

public:
    explicit DebitCreditNoteWidget(DebitCreditNoteController* controller = nullptr,
                                   PrintExportController* printExportCtrl = nullptr,
                                   QWidget* parent = nullptr);

    void resetForm();
    // popupDateOnLoad: drill-down opens pop the date dialog first (dates are
    // never keyboard-focusable); after it closes focus lands on the party.
    // In-voucher browsing (prev/next/alter) keeps false.
    bool loadNoteForEditing(int noteId, bool popupDateOnLoad = false);
    void openDateDialog(bool isInitial = false);
    void saveVoucher() { onSaveClicked(); }
    void deleteVoucher() { onDeleteClicked(); }

signals:
    void backRequested();
    void noteSaved(const QString& noteNo);

public slots:
    void openAlterNoteDialog();
    void loadPreviousNote();
    void loadNextNote();
    void printNote();
    void exportPdf();
    void focusTableAt(int row, int col);
    void advanceCell();
    void retreatCell();
    void moveCell(int row, int col);

protected:
    void showEvent(QShowEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onNoteTypeChanged(int index);
    void onPartyDataSelected(const QVariantMap& partyData);
    void onOrigInvoiceChosen(int index);
    void onGridCellChanged(int row, int column);
    void onStockItemConfigured(int row, const QVariantMap& itemData);
    void onAddItemRow();
    void onRemoveItemRow();
    void onRecalculateTotals();
    void onSaveClicked();
    void onDeleteClicked();

private:
    void setupUi();
    void setupConnections();
    void setupNavigationChains();
    void populateStates();
    void refreshOrigInvoiceCombo(bool preserveSelection = false);
    void syncTableFromDraft();
    // Pulls unit/HSN/rate/GST for the row's item name from the stock-item
    // master (exact name match). Runs on popup-pick AND on typed+committed
    // names, so Tab-out fills exactly like Space-search does.
    void fillItemDetailsFromMaster(int row, bool focusNext);
    QString currentOrigInvType() const;
    int findNoteRow(int noteId) const;
    void setStatusMessage(const QString& message, bool isError);

    DebitCreditNoteController* m_controller = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;
    ItemSearchDelegate* m_itemDelegate = nullptr;

    int m_editNoteId = 0;
    bool m_isUpdatingTable = false;

    // Header & Meta Controls
    QLabel* m_noteBadge = nullptr;
    QComboBox* m_noteTypeCombo = nullptr;
    QLineEdit* m_noteNoEdit = nullptr;
    AccountingDateEdit* m_noteDateEdit = nullptr;
    QLabel* m_dayLabel = nullptr;
    QLabel* m_fyBadge = nullptr;
    QLabel* m_voucherNoDisplay = nullptr;

    // Orig-invoice link: combo fed by the selected party's invoices
    // (latest first); the date fills automatically on selection.
    QComboBox* m_origInvoiceTypeCombo = nullptr;
    QComboBox* m_origInvoiceCombo = nullptr;
    QLineEdit* m_origInvoiceDateEdit = nullptr;
    bool m_suppressInvoiceFetch = false;
    // Last invoice number fully applied (date + party + items). Tab-out
    // re-fires editingFinished after a dropdown pick — this skips the
    // duplicate fetch while still retrying numbers that failed before.
    QString m_lastAppliedInvNo;
    // Commits typed (not picked) invoice text: exact list match selects
    // the row, otherwise the number is fetched straight from the books.
    void commitOrigInvoiceText();
    void applyOrigInvoice(const QString& invNo);

    // Party & Statutory Controls
    AccountSearchBox* m_partySearch = nullptr;
    QLabel* m_partyBalLabel = nullptr;
    QLineEdit* m_partyGstinEdit = nullptr;
    QComboBox* m_reasonCodeCombo = nullptr;
    QComboBox* m_posCombo = nullptr;

    // Table
    QTableWidget* m_itemsTable = nullptr;
    KbdBadgeButton* m_addRowBtn = nullptr;
    KbdBadgeButton* m_remRowBtn = nullptr;

    // Summary Metrics Cards (Tier 4)
    QLabel* m_taxableValLabel = nullptr;
    QLabel* m_cgstSgstValLabel = nullptr;
    QLabel* m_igstValLabel = nullptr;
    QLabel* m_grandTotalValLabel = nullptr;
    QLabel* m_statsSummaryLabel = nullptr;

    // Narration & Actions (badge text is always truthful: F2/F4/PgUp/PgDn/
    // Ctrl+S/Ctrl+P/Ctrl+E/Esc owned by keyPressEvent; Del removes a row
    // only while the grid has focus)
    QLineEdit* m_narrationEdit = nullptr;
    KbdBadgeButton* m_saveBtn = nullptr;
    KbdBadgeButton* m_deleteBtn = nullptr;
    KbdBadgeButton* m_printBtn = nullptr;
    KbdBadgeButton* m_pdfBtn = nullptr;
    KbdBadgeButton* m_cancelBtn = nullptr;
    KbdBadgeButton* m_newBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
};

} // namespace MahadevERP
