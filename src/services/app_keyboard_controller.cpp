#include "app_keyboard_controller.h"
#include "../widgets/main_window.h"
#include "../widgets/balance_sheet_widget.h"
#include "../widgets/profit_loss_widget.h"
#include "../widgets/ledger_statement_widget.h"
#include "../widgets/sales_voucher_widget.h"
#include "../widgets/purchase_voucher_widget.h"
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

void AppKeyboardController::triggerPrint() {
    int vIdx = currentViewIndex();
    if (vIdx == 14 && m_mainWindow && m_mainWindow->salesVoucherWidget()) {
        m_mainWindow->salesVoucherWidget()->printInvoice();
    } else if (vIdx == 15 && m_mainWindow && m_mainWindow->purchaseVoucherWidget()) {
        m_mainWindow->purchaseVoucherWidget()->printInvoice();
    } else if (vIdx == 29 && m_mainWindow && m_mainWindow->balanceSheetWidget()) {
        m_mainWindow->balanceSheetWidget()->printReport();
    } else if (vIdx == 30 && m_mainWindow && m_mainWindow->profitLossWidget()) {
        m_mainWindow->profitLossWidget()->printReport();
    } else if (vIdx == 8 && m_mainWindow && m_mainWindow->ledgerWidget()) {
        m_mainWindow->ledgerWidget()->printStatement();
    }
}

void AppKeyboardController::triggerPdfExport() {
    int vIdx = currentViewIndex();
    if (vIdx == 14 && m_mainWindow && m_mainWindow->salesVoucherWidget()) {
        m_mainWindow->salesVoucherWidget()->exportPdf();
    } else if (vIdx == 15 && m_mainWindow && m_mainWindow->purchaseVoucherWidget()) {
        m_mainWindow->purchaseVoucherWidget()->exportPdf();
    } else if (vIdx == 29 && m_mainWindow && m_mainWindow->balanceSheetWidget()) {
        m_mainWindow->balanceSheetWidget()->exportPdf();
    } else if (vIdx == 30 && m_mainWindow && m_mainWindow->profitLossWidget()) {
        m_mainWindow->profitLossWidget()->exportPdf();
    } else if (vIdx == 8 && m_mainWindow && m_mainWindow->ledgerWidget()) {
        m_mainWindow->ledgerWidget()->exportPdf();
    }
}

void AppKeyboardController::triggerCsvExport() {
    int vIdx = currentViewIndex();
    if (vIdx == 29 && m_mainWindow && m_mainWindow->balanceSheetWidget()) {
        m_mainWindow->balanceSheetWidget()->exportCsv();
    } else if (vIdx == 30 && m_mainWindow && m_mainWindow->profitLossWidget()) {
        m_mainWindow->profitLossWidget()->exportCsv();
    } else if (vIdx == 8 && m_mainWindow && m_mainWindow->ledgerWidget()) {
        m_mainWindow->ledgerWidget()->exportCsv();
    }
}

void AppKeyboardController::focusSearch() {
    int vIdx = currentViewIndex();
    if (vIdx == 8 && m_mainWindow && m_mainWindow->ledgerWidget()) {
        m_mainWindow->ledgerWidget()->focusSearch();
    }
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
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* kEvent = static_cast<QKeyEvent*>(event);
        if (handleKeyPress(kEvent)) {
            return true;
        }
    }
    return QObject::eventFilter(watched, event);
}

bool AppKeyboardController::handleKeyPress(QKeyEvent* event) {
    int key = event->key();
    Qt::KeyboardModifiers mods = event->modifiers();

    // 1. If an active modal dialog is currently showing, allow it to handle keys natively
    if (QApplication::activeModalWidget()) {
        return false;
    }

    // 2. Escape Key (Universal Dismiss / Navigate Back)
    if (key == Qt::Key_Escape) {
        handleEscape();
        return true;
    }

    // 3. Accounting Period Modal (<kbd>F2</kbd> or <kbd>Alt+F2</kbd>)
    if (key == Qt::Key_F2 || (mods.testFlag(Qt::AltModifier) && key == Qt::Key_F2)) {
        openPeriodModal();
        return true;
    }

    // Save Shortcut (<kbd>Ctrl+S</kbd>) in Sales / Purchase Voucher
    if (mods.testFlag(Qt::ControlModifier) && key == Qt::Key_S) {
        if (currentViewIndex() == 14 && m_mainWindow && m_mainWindow->salesVoucherWidget()) {
            m_mainWindow->salesVoucherWidget()->saveVoucher();
            return true;
        } else if (currentViewIndex() == 15 && m_mainWindow && m_mainWindow->purchaseVoucherWidget()) {
            m_mainWindow->purchaseVoucherWidget()->saveVoucher();
            return true;
        }
    }

    // 4. View Specific Navigation Hotkeys (Ctrl+Alt+P, Ctrl+Alt+B, F8, F9, F4, F7, F10, F12)
    if (mods.testFlag(Qt::ControlModifier) && mods.testFlag(Qt::AltModifier)) {
        if (key == Qt::Key_P) {
            navigateTo(30); // Profit & Loss
            return true;
        } else if (key == Qt::Key_B) {
            navigateTo(29); // Balance Sheet
            return true;
        }
    }

    if (key == Qt::Key_F8) {
        if (currentViewIndex() == 14 && m_mainWindow && m_mainWindow->salesVoucherWidget()) {
            m_mainWindow->salesVoucherWidget()->resetForm();
            return true;
        }
        navigateTo(14);
        return true;
    }
    if (key == Qt::Key_F9) {
        if (currentViewIndex() == 15 && m_mainWindow && m_mainWindow->purchaseVoucherWidget()) {
            m_mainWindow->purchaseVoucherWidget()->resetForm();
            return true;
        }
        navigateTo(15);
        return true;
    }
    if (key == Qt::Key_F4) { navigateTo(16); return true; } // Cheque
    if (key == Qt::Key_F7) { navigateTo(17); return true; } // Journal
    if (key == Qt::Key_F10) { navigateTo(18); return true; } // Milling
    if (key == Qt::Key_F12) { navigateTo(13); return true; } // Stock Register

    // 5. Expand (<kbd>F5</kbd>) & Collapse (<kbd>F6</kbd>) for Ledger / Financial trees
    if (key == Qt::Key_F5) {
        expandAll();
        return true;
    }
    if (key == Qt::Key_F6) {
        collapseAll();
        return true;
    }

    // 6. Print & PDF Shortcuts
    if ((mods.testFlag(Qt::ControlModifier) && key == Qt::Key_P) || (mods.testFlag(Qt::AltModifier) && key == Qt::Key_P)) {
        triggerPrint();
        return true;
    }
    if (mods.testFlag(Qt::ControlModifier) && key == Qt::Key_E) {
        triggerPdfExport();
        return true;
    }

    // 7. Search Focus (Alt+S or Alt+L)
    if (mods.testFlag(Qt::AltModifier) && (key == Qt::Key_S || key == Qt::Key_L)) {
        focusSearch();
        return true;
    }

    // 8. Auto-Focus and Instant Navigation for Arrow Keys in native views
    int vIdx = currentViewIndex();
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
