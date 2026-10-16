#include "app_keyboard_controller.h"
#include "keymap.h"
#include "../widgets/main_window.h"
#include "../widgets/balance_sheet_widget.h"
#include "../widgets/profit_loss_widget.h"
#include "../widgets/ledger_statement_widget.h"
#include "../widgets/sales_voucher_widget.h"
#include "../widgets/purchase_voucher_widget.h"
#include "../widgets/cheque_voucher_widget.h"
#include "../widgets/journal_voucher_widget.h"
#include "../widgets/cash_voucher_widget.h"
#include "../widgets/tds_voucher_widget.h"
#include "../widgets/jform_voucher_widget.h"
#include "../widgets/iform_voucher_widget.h"
#include "../widgets/manufacturing_voucher_widget.h"
#include "../widgets/debit_credit_note_widget.h"
#include "../widgets/keyboard_shortcuts_dialog.h"
#include <QTimer>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QTreeWidget>
#include <QDebug>

AppKeyboardController* AppKeyboardController::s_instance = nullptr;

AppKeyboardController::AppKeyboardController(MainWindow* mainWindow, QObject* parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
{
    s_instance = this;
    if (qApp) {
        qApp->installEventFilter(this);
    }
}

AppKeyboardController::~AppKeyboardController() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

AppKeyboardController* AppKeyboardController::instance() {
    return s_instance;
}

int AppKeyboardController::currentViewIndex() const {
    return m_mainWindow ? m_mainWindow->currentViewIndex() : 0;
}

void AppKeyboardController::handleEscape() {
    if (!m_mainWindow) return;

    // 1. If an active modal widget/dialog is open, let the modal close naturally
    if (QApplication::activeModalWidget()) {
        QApplication::activeModalWidget()->close();
        return;
    }

    // 2. Delegate to unified stack-based navigation unwinding
    m_mainWindow->navigateBack();
}

void AppKeyboardController::handleEnter() {
    if (!m_mainWindow) return;

    int vIdx = currentViewIndex();

    // In Balance Sheet: Trigger drill-down on current item
    if (vIdx == 29) {
        BalanceSheetWidget* bw = m_mainWindow->balanceSheetWidget();
        if (bw) {
            bw->triggerDrillDownOnCurrentItem();
            return;
        }
    }

    // In Profit & Loss: Trigger drill-down on current item
    if (vIdx == 30) {
        ProfitLossWidget* pw = m_mainWindow->profitLossWidget();
        if (pw) {
            pw->triggerDrillDownOnCurrentItem();
            return;
        }
    }

    // In Ledger Statement: Open highlighted voucher
    if (vIdx == 8) {
        LedgerStatementWidget* lw = m_mainWindow->ledgerWidget();
        if (lw) {
            lw->openSelectedVoucher();
            return;
        }
    }
}

void AppKeyboardController::handleArrowKey(const QString& direction) {
    if (!m_mainWindow) return;

    int vIdx = currentViewIndex();

    if (vIdx == 29) {
        BalanceSheetWidget* bw = m_mainWindow->balanceSheetWidget();
        if (bw) {
            QTreeWidget* lt = bw->liabilitiesTree();
            QTreeWidget* at = bw->assetsTree();
            if (direction == "Left" && lt) {
                lt->setFocus(Qt::OtherFocusReason);
                if (!lt->currentItem() && lt->topLevelItemCount() > 0) {
                    lt->setCurrentItem(lt->topLevelItem(0));
                }
            } else if (direction == "Right" && at) {
                at->setFocus(Qt::OtherFocusReason);
                if (!at->currentItem() && at->topLevelItemCount() > 0) {
                    at->setCurrentItem(at->topLevelItem(0));
                }
            }
        }
    } else if (vIdx == 30) {
        ProfitLossWidget* pw = m_mainWindow->profitLossWidget();
        if (pw) {
            QTreeWidget* et = pw->expensesTree();
            QTreeWidget* it = pw->incomesTree();
            if (direction == "Left" && et) {
                et->setFocus(Qt::OtherFocusReason);
                if (!et->currentItem() && et->topLevelItemCount() > 0) {
                    et->setCurrentItem(et->topLevelItem(0));
                }
            } else if (direction == "Right" && it) {
                it->setFocus(Qt::OtherFocusReason);
                if (!it->currentItem() && it->topLevelItemCount() > 0) {
                    it->setCurrentItem(it->topLevelItem(0));
                }
            }
        }
    } else if (vIdx == 8) {
        LedgerStatementWidget* lw = m_mainWindow->ledgerWidget();
        if (lw) {
            if (direction == "Left" && lw->crTable()) {
                lw->onSwitchSideRequested("Cr");
            } else if (direction == "Right" && lw->drTable()) {
                lw->onSwitchSideRequested("Dr");
            }
        }
    }
}

void AppKeyboardController::openPeriodModal() {
    if (m_mainWindow) {
        m_mainWindow->openAccountingPeriodDialog();
    }
}

void AppKeyboardController::expandAll() {
    int vIdx = currentViewIndex();
    if (vIdx == 29 && m_mainWindow && m_mainWindow->balanceSheetWidget()) {
        m_mainWindow->balanceSheetWidget()->expandAllGroups();
    } else if (vIdx == 30 && m_mainWindow && m_mainWindow->profitLossWidget()) {
        m_mainWindow->profitLossWidget()->expandAllGroups();
    }
}

void AppKeyboardController::collapseAll() {
    int vIdx = currentViewIndex();
    if (vIdx == 29 && m_mainWindow && m_mainWindow->balanceSheetWidget()) {
        m_mainWindow->balanceSheetWidget()->collapseAllGroups();
    } else if (vIdx == 30 && m_mainWindow && m_mainWindow->profitLossWidget()) {
        m_mainWindow->profitLossWidget()->collapseAllGroups();
    }
}

bool AppKeyboardController::triggerPrint() {
    int vIdx = currentViewIndex();
    if (vIdx == 14 && m_mainWindow && m_mainWindow->salesVoucherWidget()) {
        m_mainWindow->salesVoucherWidget()->printInvoice();
        return true;
    } else if (vIdx == 15 && m_mainWindow && m_mainWindow->purchaseVoucherWidget()) {
        m_mainWindow->purchaseVoucherWidget()->printInvoice();
        return true;
    } else if (vIdx == 29 && m_mainWindow && m_mainWindow->balanceSheetWidget()) {
        m_mainWindow->balanceSheetWidget()->printReport();
        return true;
    } else if (vIdx == 30 && m_mainWindow && m_mainWindow->profitLossWidget()) {
        m_mainWindow->profitLossWidget()->printReport();
        return true;
    } else if (vIdx == 8 && m_mainWindow && m_mainWindow->ledgerWidget()) {
        m_mainWindow->ledgerWidget()->printStatement();
        return true;
    }
    return false;
}

bool AppKeyboardController::triggerPdfExport() {
    int vIdx = currentViewIndex();
    if (vIdx == 14 && m_mainWindow && m_mainWindow->salesVoucherWidget()) {
        m_mainWindow->salesVoucherWidget()->exportPdf();
        return true;
    } else if (vIdx == 15 && m_mainWindow && m_mainWindow->purchaseVoucherWidget()) {
        m_mainWindow->purchaseVoucherWidget()->exportPdf();
        return true;
    } else if (vIdx == 29 && m_mainWindow && m_mainWindow->balanceSheetWidget()) {
        m_mainWindow->balanceSheetWidget()->exportPdf();
        return true;
    } else if (vIdx == 30 && m_mainWindow && m_mainWindow->profitLossWidget()) {
        m_mainWindow->profitLossWidget()->exportPdf();
        return true;
    } else if (vIdx == 8 && m_mainWindow && m_mainWindow->ledgerWidget()) {
        m_mainWindow->ledgerWidget()->exportPdf();
        return true;
    }
    return false;
}

bool AppKeyboardController::triggerCsvExport() {
    int vIdx = currentViewIndex();
    if (vIdx == 29 && m_mainWindow && m_mainWindow->balanceSheetWidget()) {
        m_mainWindow->balanceSheetWidget()->exportCsv();
        return true;
    } else if (vIdx == 30 && m_mainWindow && m_mainWindow->profitLossWidget()) {
        m_mainWindow->profitLossWidget()->exportCsv();
        return true;
    } else if (vIdx == 8 && m_mainWindow && m_mainWindow->ledgerWidget()) {
        m_mainWindow->ledgerWidget()->exportCsv();
        return true;
    }
    return false;
}

bool AppKeyboardController::focusSearch() {
    int vIdx = currentViewIndex();
    if (vIdx == 8 && m_mainWindow && m_mainWindow->ledgerWidget()) {
        m_mainWindow->ledgerWidget()->focusSearch();
        return true;
    }
    return false;
}

// Single dispatch for F2 date dialogs across every voucher + statement view.
// Returns false (never steals the key) on views without a date dialog.
bool AppKeyboardController::openDateForView(int vIdx) {
    if (!m_mainWindow) return false;
    switch (vIdx) {
    case 60: if (m_mainWindow->cashVoucherWidget()) { m_mainWindow->cashVoucherWidget()->openDateDialog(false); return true; } break;
    case 16: if (m_mainWindow->chequeVoucherWidget()) { m_mainWindow->chequeVoucherWidget()->openDateDialog(false); return true; } break;
    case 17: if (m_mainWindow->journalVoucherWidget()) { m_mainWindow->journalVoucherWidget()->openDateDialog(false); return true; } break;
    case 18: if (m_mainWindow->jformVoucherWidget()) { m_mainWindow->jformVoucherWidget()->openDateDialog(false); return true; } break;
    case 19: if (m_mainWindow->iformVoucherWidget()) { m_mainWindow->iformVoucherWidget()->openDateDialog(false); return true; } break;
    case 31: if (m_mainWindow->manufacturingVoucherWidget()) { m_mainWindow->manufacturingVoucherWidget()->openDateDialog(false); return true; } break;
    case 28: if (m_mainWindow->debitCreditNoteWidget()) { m_mainWindow->debitCreditNoteWidget()->openDateDialog(); return true; } break;
    case 24: if (m_mainWindow->tdsVoucherWidget()) { m_mainWindow->tdsVoucherWidget()->openDateDialog(); return true; } break;
    case 14: if (m_mainWindow->salesVoucherWidget()) { m_mainWindow->salesVoucherWidget()->openDateDialog(false); return true; } break;
    case 15: if (m_mainWindow->purchaseVoucherWidget()) { m_mainWindow->purchaseVoucherWidget()->openDateDialog(false); return true; } break;
    case 8:  if (m_mainWindow->ledgerWidget()) { m_mainWindow->ledgerWidget()->onPeriodClicked(); return true; } break;
    case 40: if (m_mainWindow->capitalAccountsWidget()) { m_mainWindow->capitalAccountsWidget()->onPeriodClicked(); return true; } break;
    case 61: case 62: case 63:
        if (m_mainWindow->cashBankFlowWidget()) { m_mainWindow->cashBankFlowWidget()->onAccountingPeriod(); return true; } break;
    default: break;
    }
    return false;
}

// Single dispatch for Ctrl+S. Every voucher view in ErpKeymap::isVoucherView
// must be handled here; returns false elsewhere so the key stays available.
bool AppKeyboardController::saveActiveVoucher(int vIdx) {
    if (!m_mainWindow) return false;
    switch (vIdx) {
    case 14: if (m_mainWindow->salesVoucherWidget()) { m_mainWindow->salesVoucherWidget()->saveVoucher(); return true; } break;
    case 15: if (m_mainWindow->purchaseVoucherWidget()) { m_mainWindow->purchaseVoucherWidget()->saveVoucher(); return true; } break;
    case 16: if (m_mainWindow->chequeVoucherWidget()) { m_mainWindow->chequeVoucherWidget()->saveVoucher(); return true; } break;
    case 17: if (m_mainWindow->journalVoucherWidget()) { m_mainWindow->journalVoucherWidget()->saveVoucher(); return true; } break;
    case 18: if (m_mainWindow->jformVoucherWidget()) { m_mainWindow->jformVoucherWidget()->saveVoucher(); return true; } break;
    case 19: if (m_mainWindow->iformVoucherWidget()) { m_mainWindow->iformVoucherWidget()->saveVoucher(); return true; } break;
    case 60: if (m_mainWindow->cashVoucherWidget()) { m_mainWindow->cashVoucherWidget()->saveVoucher(); return true; } break;
    case 24: if (m_mainWindow->tdsVoucherWidget()) { m_mainWindow->tdsVoucherWidget()->onSaveClicked(); return true; } break;
    case 28: if (m_mainWindow->debitCreditNoteWidget()) { m_mainWindow->debitCreditNoteWidget()->saveVoucher(); return true; } break;
    case 31: if (m_mainWindow->manufacturingVoucherWidget()) { m_mainWindow->manufacturingVoucherWidget()->saveVoucher(); return true; } break;
    default: break;
    }
    return false;
}

// Single dispatch for Ctrl+D. Manufacturing (31) intentionally excluded: batches are
// never hard-deleted, only re-milled via a new batch.
bool AppKeyboardController::deleteActiveVoucher(int vIdx) {
    if (!m_mainWindow) return false;
    switch (vIdx) {
    case 14: if (m_mainWindow->salesVoucherWidget()) { m_mainWindow->salesVoucherWidget()->deleteVoucher(); return true; } break;
    case 15: if (m_mainWindow->purchaseVoucherWidget()) { m_mainWindow->purchaseVoucherWidget()->deleteVoucher(); return true; } break;
    case 16: if (m_mainWindow->chequeVoucherWidget()) { m_mainWindow->chequeVoucherWidget()->deleteVoucher(); return true; } break;
    case 17: if (m_mainWindow->journalVoucherWidget()) { m_mainWindow->journalVoucherWidget()->deleteVoucher(); return true; } break;
    case 18: if (m_mainWindow->jformVoucherWidget()) { m_mainWindow->jformVoucherWidget()->deleteVoucher(); return true; } break;
    case 19: if (m_mainWindow->iformVoucherWidget()) { m_mainWindow->iformVoucherWidget()->deleteVoucher(); return true; } break;
    case 60: if (m_mainWindow->cashVoucherWidget()) { m_mainWindow->cashVoucherWidget()->deleteVoucher(); return true; } break;
    case 24: if (m_mainWindow->tdsVoucherWidget()) { m_mainWindow->tdsVoucherWidget()->onDeleteClicked(); return true; } break;
    case 28: if (m_mainWindow->debitCreditNoteWidget()) { m_mainWindow->debitCreditNoteWidget()->deleteVoucher(); return true; } break;
    default: break;
    }
    return false;
}

void AppKeyboardController::navigateTo(int viewIndex) {
    if (m_mainWindow) {
        m_mainWindow->navigateToView(viewIndex);
    }
}

void AppKeyboardController::restoreFocus() {
    if (m_mainWindow) {
        m_mainWindow->restoreActiveViewFocus();
    }
}

bool AppKeyboardController::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease || event->type() == QEvent::ShortcutOverride) {
        if (QApplication::activeModalWidget() || QApplication::activePopupWidget()) {
            return false;
        }
        if (watched && watched->isWidgetType()) {
            QWidget* w = static_cast<QWidget*>(watched);
            if (qobject_cast<QDialog*>(w) || qobject_cast<QDialog*>(w->window())) {
                return false;
            }
        }
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* kEvent = static_cast<QKeyEvent*>(event);
            if (handleKeyPress(kEvent)) {
                return true;
            }
        }
    }
    return QObject::eventFilter(watched, event);
}

bool AppKeyboardController::handleKeyPress(QKeyEvent* event) {
    // 1. Modals and popups always handle their own keys.
    if (QApplication::activeModalWidget()) {
        return false;
    }

    const int key = event->key();
    const Qt::KeyboardModifiers mods = event->modifiers();

    // 2. Escape: universal dismiss / navigate back.
    if (key == Qt::Key_Escape) {
        handleEscape();
        return true;
    }

    int vIdx = currentViewIndex();
    auto chord = [&](Qt::KeyboardModifiers wantMods, int wantKey) {
        return mods == wantMods && key == wantKey;
    };
    const Qt::KeyboardModifiers kNone = Qt::NoModifier;
    const Qt::KeyboardModifiers kCtrl = Qt::ControlModifier;
    const Qt::KeyboardModifiers kAlt = Qt::AltModifier;

    // 3. Period / firm / help. See ErpKeymap::bindings() — the single truth table.
    if (chord(kAlt, Qt::Key_F2)) {
        openPeriodModal();
        return true;
    }
    if (chord(kAlt, Qt::Key_F1)) {
        navigateTo(ErpKeymap::navTargetFor("app.firm")); // Firm selector (22)
        return true;
    }
    if (chord(kNone, Qt::Key_F1)) {
        MahadevERP::KeyboardShortcutsDialog::showShortcuts(m_mainWindow);
        return true;
    }

    // 4. Voucher / statement date (F2). Never stolen on other views.
    if (chord(kNone, Qt::Key_F2)) {
        return openDateForView(vIdx);
    }

    // 5. Save (Ctrl+S) / delete (Ctrl+D) on every voucher view.
    if (chord(kCtrl, Qt::Key_S)) {
        return saveActiveVoucher(vIdx);
    }
    if (chord(kCtrl, Qt::Key_D)) {
        return deleteActiveVoucher(vIdx);
    }

    // 6. Tally F-key navigation. Views with local F-key behaviour keep it.
    if (chord(kNone, Qt::Key_F3)) {
        return false; // Bank/cash mode switch handled locally by the view.
    }
    if (chord(kNone, Qt::Key_F4)) {
        if (vIdx == 16 || vIdx == 60 || vIdx == 14) return false;
        navigateTo(ErpKeymap::navTargetFor("nav.contra"));
        return true;
    }
    if (chord(kNone, Qt::Key_F7)) {
        if (vIdx == 17) return false;
        navigateTo(ErpKeymap::navTargetFor("nav.journal"));
        return true;
    }
    if (chord(kNone, Qt::Key_F8)) {
        if (vIdx == 14 && m_mainWindow && m_mainWindow->salesVoucherWidget()) {
            m_mainWindow->salesVoucherWidget()->resetForm();
            return true;
        }
        navigateTo(ErpKeymap::navTargetFor("nav.sales"));
        return true;
    }
    if (chord(kNone, Qt::Key_F9)) {
        if (vIdx == 15 && m_mainWindow && m_mainWindow->purchaseVoucherWidget()) {
            m_mainWindow->purchaseVoucherWidget()->resetForm();
            return true;
        }
        navigateTo(ErpKeymap::navTargetFor("nav.purchase"));
        return true;
    }
    if (chord(kNone, Qt::Key_F10)) { navigateTo(ErpKeymap::navTargetFor("nav.jform")); return true; }
    if (chord(kNone, Qt::Key_F11)) { navigateTo(ErpKeymap::navTargetFor("nav.iform")); return true; }
    if (chord(kNone, Qt::Key_F12)) { navigateTo(ErpKeymap::navTargetFor("nav.stock")); return true; }

    // 7. Expand (F5) / collapse (F6) — reports only, never swallowed elsewhere.
    if (chord(kNone, Qt::Key_F5)) {
        if (vIdx == 29 || vIdx == 30) { expandAll(); return true; }
        return false;
    }
    if (chord(kNone, Qt::Key_F6)) {
        if (vIdx == 29 || vIdx == 30) { collapseAll(); return true; }
        return false;
    }

    // 8. Print & export. Only swallowed where the view supports the action.
    if (chord(kCtrl, Qt::Key_P) || chord(kAlt, Qt::Key_P)) {
        return triggerPrint();
    }
    if (chord(kCtrl, Qt::Key_E)) {
        return triggerPdfExport();
    }
    if (chord(kAlt, Qt::Key_E)) {
        return triggerCsvExport();
    }

    // 9. Search focus: ledger (Ctrl+F everywhere else stays available).
    if (chord(kCtrl, Qt::Key_F)) {
        return focusSearch();
    }
    if (chord(kAlt, Qt::Key_S) || chord(kAlt, Qt::Key_L)) {
        if (vIdx == 8) return focusSearch();
        return false;
    }

    // 10. Auto-Focus and Instant Navigation for Arrow Keys in native views
    if (vIdx == 29 || vIdx == 30 || vIdx == 8) {
        QWidget* fw = QApplication::focusWidget();
        bool isTextInput = (qobject_cast<QLineEdit*>(fw) || qobject_cast<QTextEdit*>(fw) || qobject_cast<QComboBox*>(fw));

        if (!isTextInput) {
            if (key == Qt::Key_Return || key == Qt::Key_Enter) {
                handleEnter();
                return true;
            }
            if (key == Qt::Key_Up || key == Qt::Key_Down) {
                if (vIdx == 29 && m_mainWindow && m_mainWindow->balanceSheetWidget()) {
                    QTreeWidget* lt = m_mainWindow->balanceSheetWidget()->liabilitiesTree();
                    QTreeWidget* at = m_mainWindow->balanceSheetWidget()->assetsTree();
                    if (lt && at) {
                        if (!lt->hasFocus() && !at->hasFocus()) {
                            lt->setFocus(Qt::OtherFocusReason);
                            if (lt->topLevelItemCount() > 0 && !lt->currentItem()) {
                                lt->setCurrentItem(lt->topLevelItem(0));
                            }
                            QCoreApplication::sendEvent(lt, event);
                            return true;
                        }
                    }
                } else if (vIdx == 30 && m_mainWindow && m_mainWindow->profitLossWidget()) {
                    QTreeWidget* et = m_mainWindow->profitLossWidget()->expensesTree();
                    QTreeWidget* it = m_mainWindow->profitLossWidget()->incomesTree();
                    if (et && it) {
                        if (!et->hasFocus() && !it->hasFocus()) {
                            et->setFocus(Qt::OtherFocusReason);
                            if (et->topLevelItemCount() > 0 && !et->currentItem()) {
                                et->setCurrentItem(et->topLevelItem(0));
                            }
                            QCoreApplication::sendEvent(et, event);
                            return true;
                        }
                    }
                } else if (vIdx == 8 && m_mainWindow && m_mainWindow->ledgerWidget()) {
                    LedgerStatementWidget* lw = m_mainWindow->ledgerWidget();
                    if (lw) {
                        LedgerTableView* dt = lw->drTable();
                        LedgerTableView* ct = lw->crTable();
                        if (dt && ct && !dt->hasFocus() && !ct->hasFocus()) {
                            LedgerTableView* targetTable = (dt->model() && dt->model()->rowCount() > 0) ? dt : ct;
                            targetTable->setFocus(Qt::OtherFocusReason);
                            if (targetTable->selectedRowIndex() < 0 && targetTable->model() && targetTable->model()->rowCount() > 0) {
                                targetTable->selectRowIndex(0);
                            }
                            QCoreApplication::sendEvent(targetTable, event);
                            return true;
                        }
                    }
                }
                return false;
            }
            if (key == Qt::Key_Left) {
                handleArrowKey("Left");
                return true;
            } else if (key == Qt::Key_Right) {
                handleArrowKey("Right");
                return true;
            }
        }
    }

    return false;
}
