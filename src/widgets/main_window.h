#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QQuickWindow>
#include "ledger_statement_widget.h"
#include "../models/ledger_statement_model.h"
#include "../services/print_export_controller.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QQuickWindow* qmlWindow,
                        LedgerStatementController* ledgerCtrl,
                        PrintExportController* printExportCtrl,
                        QWidget* parent = nullptr);

    LedgerStatementWidget* ledgerWidget() const { return m_ledgerWidget; }

public slots:
    void navigateToView(int viewIndex);
    void checkQmlView();
    void onLedgerBackRequested();
    void onLedgerAlterVoucherRequested(int targetViewIndex, const QVariantMap& entry);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    QStackedWidget* m_stackedWidget = nullptr;
    QWidget* m_qmlContainer = nullptr;
    QQuickWindow* m_qmlWindow = nullptr;
    LedgerStatementWidget* m_ledgerWidget = nullptr;

    LedgerStatementController* m_ledgerCtrl = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;
};
