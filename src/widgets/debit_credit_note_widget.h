#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
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
    bool loadNoteForEditing(int noteId);
    void openDateDialog();

signals:
    void backRequested();
    void noteSaved(const QString& noteNo);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onNoteTypeChanged(int index);
    void onPartySelected(const QString& partyName);
    void onStockItemConfigured(int row, const QVariantMap& itemData);
    void onAddItemRow();
    void onRemoveItemRow();
    void onRecalculateTotals();
    void onSaveClicked();
    void onDeleteClicked();

private:
    void setupUi();
    void setupConnections();
    void populateStates();

    DebitCreditNoteController* m_controller = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;
    ItemSearchDelegate* m_itemDelegate = nullptr;

    int m_editNoteId = 0;
    bool m_isUpdatingTable = false;

    // Header & Meta Controls
    QComboBox* m_noteTypeCombo = nullptr;
    QLineEdit* m_noteNoEdit = nullptr;
    AccountingDateEdit* m_noteDateEdit = nullptr;
    QLabel* m_dayLabel = nullptr;
    QLabel* m_fyBadge = nullptr;

    QComboBox* m_origInvoiceTypeCombo = nullptr;
    QLineEdit* m_origInvoiceNoEdit = nullptr;
    QLineEdit* m_origInvoiceDateEdit = nullptr;

    // Party & Statutory Controls
    AccountSearchBox* m_partySearch = nullptr;
    QLineEdit* m_partyGstinEdit = nullptr;
    QComboBox* m_reasonCodeCombo = nullptr;
    QComboBox* m_posCombo = nullptr;

    // Table
    QTableWidget* m_itemsTable = nullptr;
    QPushButton* m_addRowBtn = nullptr;
    QPushButton* m_remRowBtn = nullptr;

    // Summary Metrics Cards (Tier 4)
    QLabel* m_taxableValLabel = nullptr;
    QLabel* m_cgstSgstValLabel = nullptr;
    QLabel* m_igstValLabel = nullptr;
    QLabel* m_grandTotalValLabel = nullptr;
    QLabel* m_statsSummaryLabel = nullptr;

    // Narration & Actions
    QLineEdit* m_narrationEdit = nullptr;
    KbdBadgeButton* m_saveBtn = nullptr;
    KbdBadgeButton* m_deleteBtn = nullptr;
    KbdBadgeButton* m_cancelBtn = nullptr;
    KbdBadgeButton* m_newBtn = nullptr;
};

} // namespace MahadevERP
