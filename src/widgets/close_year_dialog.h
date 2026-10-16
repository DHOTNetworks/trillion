#pragma once

// CloseFinancialYearDialog: F3 year-end close checklist UI skeleton (§24).
// Renders state only. Reads go through existing calculators NOW (trial via
// TrialBalanceController, would-transfer P&L via ProfitLossController —
// both read-only). Mutations (post closing journal, snapshot openings,
// lock, reopen) wait for Agent 1's engine close procedure: their buttons
// stay disabled with an explicit "engine pending" status until it lands.
// No direct SQL beyond reads; S4 custom steps come from close_steps/*
// registry JSON (in-memory sign-off; the engine persists/audits later).

#include <QDialog>
#include <QKeyEvent>
#include <QComboBox>
#include "account_search_box.h"
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QMap>
#include <QString>

namespace MahadevERP {

class TrialBalanceController;

} // namespace MahadevERP

class FinancialYearsModel;
class ProfitLossController;

namespace MahadevERP {

class CloseYearDialog : public QDialog {
    Q_OBJECT

public:
    explicit CloseYearDialog(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onYearChanged(int index);
    void onRunTrialCheck();
    void onCloseYear();
    void onLockYear();
    void onReopenYear();

private:
    void reloadYearCombo(const QString& keepYear);

private:
    void refreshAll();
    void refreshTrialStep();
    void refreshCustomSteps();

    QComboBox* m_yearCombo = nullptr;
    QLabel* m_statusLabel = nullptr;

    QLabel* m_s1Status = nullptr;
    QLabel* m_s1Detail = nullptr;
    QPushButton* m_s1RunBtn = nullptr;

    QLabel* m_s2Status = nullptr;
    QLabel* m_s2Detail = nullptr;
    QPushButton* m_s2PostBtn = nullptr;
    AccountSearchBox* m_capitalBox = nullptr;

    QLabel* m_s3Status = nullptr;
    QLabel* m_s3Detail = nullptr;
    QPushButton* m_s3SnapBtn = nullptr;

    QWidget* m_s4Box = nullptr;
    QLabel* m_s4Status = nullptr;
    QMap<QString, QCheckBox*> m_s4Checks; // step id -> box (session only)

    QLabel* m_s5Status = nullptr;
    QPushButton* m_s5LockBtn = nullptr;
    QPushButton* m_reopenBtn = nullptr;

    TrialBalanceController* m_trial = nullptr;
    ProfitLossController* m_pnl = nullptr;
    FinancialYearsModel* m_years = nullptr;
};

} // namespace MahadevERP
