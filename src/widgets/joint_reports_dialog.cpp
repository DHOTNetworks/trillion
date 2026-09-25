#include "joint_reports_dialog.h"
#include <QKeyEvent>

namespace MahadevERP {

JointReportsDialog::JointReportsDialog(QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint)
{
    setModal(true);
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName("jointReportsDialog");
    setupUi();
}

void JointReportsDialog::setupUi() {
    setFixedWidth(380);
    setStyleSheet(
        "#jointReportsDialog { background-color: #FFFFFF; border: 2.5px solid #059669; border-radius: 12px; }"
        "QLabel { border: none; background: transparent; }"
        "QCheckBox {"
        "  font-size: 13px;"
        "  font-weight: 700;"
        "  color: #0F172A;"
        "  spacing: 10px;"
        "  padding: 8px 12px;"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #E2E8F0;"
        "  border-radius: 6px;"
        "}"
        "QCheckBox:hover {"
        "  background-color: #F8FAFC;"
        "  border-color: #CBD5E1;"
        "}"
        "QCheckBox:focus {"
        "  border: 1.5px solid #059669;"
        "  background-color: #F0FDF4;"
        "}"
        "QCheckBox::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "  border: 1.5px solid #64748B;"
        "  border-radius: 4px;"
        "  background: #FFFFFF;"
        "}"
        "QCheckBox::indicator:checked {"
        "  background: #059669;"
        "  border-color: #059669;"
        "  image: url(:/icons/check.png);"
        "}"
    );

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 14, 16, 14);
    root->setSpacing(10);

    // Header
    auto* header = new QHBoxLayout();
    auto* titleLabel = new QLabel("SELECT JOINT REPORTS", this);
    titleLabel->setStyleSheet("color: #059669; font-size: 13px; font-weight: 800; letter-spacing: 1px;");
    header->addWidget(titleLabel);

    header->addStretch(1);
    auto* hint = new QLabel("Press Space & Enter", this);
    hint->setStyleSheet("color: #64748B; font-size: 11px; font-weight: 700;");
    header->addWidget(hint);
    root->addLayout(header);

    // Checkboxes
    m_chkTrading = new QCheckBox("Trading A/c (Commodities & Out-turn)", this);
    m_chkTrading->setChecked(true);
    root->addWidget(m_chkTrading);

    m_chkPnL = new QCheckBox("Profit & Loss Statement (Trading & P&L)", this);
    m_chkPnL->setChecked(true);
    root->addWidget(m_chkPnL);

    m_chkCapital = new QCheckBox("Capital A/c Schedule (Partners / Proprietor)", this);
    m_chkCapital->setChecked(true);
    root->addWidget(m_chkCapital);

    m_chkBalanceSheet = new QCheckBox("Balance Sheet (Normal Assets & Liabilities)", this);
    m_chkBalanceSheet->setChecked(true);
    root->addWidget(m_chkBalanceSheet);

    root->addSpacing(6);

    // Button Row
    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);

    auto* selectAllBtn = new QPushButton("Select All", this);
    selectAllBtn->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; color: #475569; border: 1px solid #CBD5E1; border-radius: 6px; padding: 6px 12px; font-weight: 700; font-size: 11.5px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(selectAllBtn, &QPushButton::clicked, this, [this]() {
        m_chkTrading->setChecked(true);
        m_chkPnL->setChecked(true);
        m_chkCapital->setChecked(true);
        m_chkBalanceSheet->setChecked(true);
    });
    btnRow->addWidget(selectAllBtn);

    btnRow->addStretch(1);

    auto* cancelBtn = new QPushButton("Cancel (Esc)", this);
    cancelBtn->setStyleSheet(
        "QPushButton { background-color: #FFFFFF; color: #64748B; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 6px 12px; font-weight: 700; font-size: 11.5px; }"
        "QPushButton:hover { background-color: #F8FAFC; }"
    );
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(cancelBtn);

    auto* printBtn = new QPushButton("Print / Export (Enter)", this);
    printBtn->setStyleSheet(
        "QPushButton { background-color: #059669; color: #FFFFFF; border: none; border-radius: 6px; padding: 6px 14px; font-weight: 800; font-size: 11.5px; }"
        "QPushButton:hover { background-color: #047857; }"
    );
    connect(printBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnRow->addWidget(printBtn);

    root->addLayout(btnRow);

    m_chkTrading->setFocus();
}

void JointReportsDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        reject();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        accept();
        event->accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

} // namespace MahadevERP
