#include "profit_loss_widget.h"
#include "../engine/fiscal_year_helper.h"
#include "../engine/accounting_engine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QPainter>
#include <QStyleOption>
#include <QMessageBox>
#include <QScrollBar>
#include <QDebug>

ProfitLossWidget::ProfitLossWidget(ProfitLossController* controller,
                                   PrintExportController* printExportCtrl,
                                   QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_printExportCtrl(printExportCtrl)
{
    setupUi();
    applyCustomStyles();

    connect(m_controller, &ProfitLossController::dataChanged, this, &ProfitLossWidget::populateTrees);
    connect(m_controller, &ProfitLossController::totalsChanged, this, &ProfitLossWidget::populateTrees);
}

void ProfitLossWidget::setupUi() {
    setAttribute(Qt::WA_StyledBackground, true);
    setFocusPolicy(Qt::StrongFocus);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 6, 8, 6);
    mainLayout->setSpacing(4);

    // ==========================================
    // 1. TOP FIRM & TITLE BANNER
    // ==========================================
    QLabel* compBanner = new QLabel(this);
    compBanner->setFixedHeight(32);
    compBanner->setAlignment(Qt::AlignCenter);
    compBanner->setStyleSheet(
        "background-color: #F0FDF4; color: #166534; font-size: 14.5px; font-weight: 800; "
        "border: 1px solid #BBF7D0; border-radius: 6px; letter-spacing: 0.3px;"
    );
    compBanner->setText("—");
    m_firmLabel = compBanner;
    mainLayout->addWidget(compBanner);

    // Sub-header Bar (Modern White Card)
    QFrame* subHeaderCard = new QFrame(this);
    subHeaderCard->setFixedHeight(48);
    subHeaderCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }"
        "QLabel { border: none; background: transparent; }"
    );
    QHBoxLayout* subHeaderLayout = new QHBoxLayout(subHeaderCard);
    subHeaderLayout->setContentsMargins(12, 0, 12, 0);
    subHeaderLayout->setSpacing(10);

    QLabel* f5Hint = new QLabel("F5: Extract Group", subHeaderCard);
    f5Hint->setStyleSheet("color: #64748B; font-size: 11.5px; font-weight: 700; background: transparent;");
    subHeaderLayout->addWidget(f5Hint);

    QVBoxLayout* centerTitleBox = new QVBoxLayout();
    centerTitleBox->setContentsMargins(0, 3, 0, 3);
    centerTitleBox->setSpacing(1);
    m_titleLabel = new QLabel("Trading and Profit & Loss A/c (Provisional)", subHeaderCard);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("font-size: 15px; font-weight: 800; color: #0F172A; background: transparent;");
    centerTitleBox->addWidget(m_titleLabel);

    m_fyBadge = new QLabel("(From 01/04/2025 To 31/03/2026)", subHeaderCard);
    m_fyBadge->setAlignment(Qt::AlignCenter);
    m_fyBadge->setStyleSheet("font-size: 12px; font-weight: 600; color: #475569; background: transparent;");
    centerTitleBox->addWidget(m_fyBadge);
    subHeaderLayout->addLayout(centerTitleBox, 1);

    // Date range & Buttons
    QHBoxLayout* btnBox = new QHBoxLayout();
    btnBox->setSpacing(5);

    m_fromDateEdit = new AccountingDateDisplay(subHeaderCard);
    m_fromDateEdit->setFixedWidth(105);
    m_fromDateEdit->setFixedHeight(30);
    m_fromDateEdit->setStyleSheet(
        "QLabel { background: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; "
        "padding: 2px 6px; font-size: 12px; font-weight: 700; color: #0F172A; } "
        "QLabel:hover { border: 1.5px solid #2563EB; background: #F8FAFC; }"
    );
    connect(m_fromDateEdit, &AccountingDateDisplay::dateChanged, this, &ProfitLossWidget::onDateFilterChanged);
    connect(m_fromDateEdit, &AccountingDateDisplay::clicked, this, &ProfitLossWidget::requestAccountingPeriodDialog);
    btnBox->addWidget(m_fromDateEdit);

    QLabel* toSep = new QLabel("to", subHeaderCard);
    toSep->setStyleSheet("font-weight: bold; color: #64748B; background: transparent; font-size: 11.5px;");
    btnBox->addWidget(toSep);

    m_toDateEdit = new AccountingDateDisplay(subHeaderCard);
    m_toDateEdit->setFixedWidth(105);
    m_toDateEdit->setFixedHeight(30);
    m_toDateEdit->setStyleSheet(
        "QLabel { background: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; "
        "padding: 2px 6px; font-size: 12px; font-weight: 700; color: #0F172A; } "
        "QLabel:hover { border: 1.5px solid #2563EB; background: #F8FAFC; }"
    );
    connect(m_toDateEdit, &AccountingDateDisplay::dateChanged, this, &ProfitLossWidget::onDateFilterChanged);
    connect(m_toDateEdit, &AccountingDateDisplay::clicked, this, &ProfitLossWidget::requestAccountingPeriodDialog);
    btnBox->addWidget(m_toDateEdit);

    m_periodBtn = new KbdBadgeButton("Period", "F2", QColor("#F8FAFC"), QColor("#F1F5F9"), QColor("#0F172A"), QColor("#CBD5E1"), subHeaderCard);
    m_periodBtn->setFixedHeight(30);
    connect(m_periodBtn, &QPushButton::clicked, this, &ProfitLossWidget::requestAccountingPeriodDialog);
    btnBox->addWidget(m_periodBtn);

    m_expandBtn = new KbdBadgeButton("Expand", "F5", QColor("#F8FAFC"), QColor("#F1F5F9"), QColor("#0F172A"), QColor("#CBD5E1"), subHeaderCard);
    m_expandBtn->setFixedHeight(30);
    connect(m_expandBtn, &QPushButton::clicked, this, &ProfitLossWidget::expandAllGroups);
    btnBox->addWidget(m_expandBtn);

    m_collapseBtn = new KbdBadgeButton("Collapse", "F6", QColor("#F8FAFC"), QColor("#F1F5F9"), QColor("#0F172A"), QColor("#CBD5E1"), subHeaderCard);
    m_collapseBtn->setFixedHeight(30);
    connect(m_collapseBtn, &QPushButton::clicked, this, &ProfitLossWidget::collapseAllGroups);
    btnBox->addWidget(m_collapseBtn);

    m_printBtn = new KbdBadgeButton("Print", "Ctrl+P", QColor("#2563EB"), QColor("#1D4ED8"), QColor("#FFFFFF"), QColor("#2563EB"), subHeaderCard);
    m_printBtn->setFixedHeight(30);
    connect(m_printBtn, &QPushButton::clicked, this, &ProfitLossWidget::printReport);
    btnBox->addWidget(m_printBtn);

    m_pdfBtn = new KbdBadgeButton("PDF", "Ctrl+E", QColor("#059669"), QColor("#047857"), QColor("#FFFFFF"), QColor("#059669"), subHeaderCard);
    m_pdfBtn->setFixedHeight(30);
    connect(m_pdfBtn, &QPushButton::clicked, this, &ProfitLossWidget::exportPdf);
    btnBox->addWidget(m_pdfBtn);

    m_backBtn = new KbdBadgeButton("Back", "Esc", QColor("#EF4444"), QColor("#DC2626"), QColor("#FFFFFF"), QColor("#EF4444"), subHeaderCard);
    m_backBtn->setFixedHeight(30);
    connect(m_backBtn, &QPushButton::clicked, this, &ProfitLossWidget::backRequested);
    btnBox->addWidget(m_backBtn);

    subHeaderLayout->addLayout(btnBox);
    mainLayout->addWidget(subHeaderCard);

    // ==========================================
    // 2. MAIN SPLIT TREE VIEWS (T-FORMAT TRADING & P&L)
    // ==========================================
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setHandleWidth(4);
    m_splitter->setStyleSheet("QSplitter::handle { background-color: #E2E8F0; border-radius: 2px; }");

    // Left Side: Expenses Tree (Dr)
    QWidget* expContainer = new QWidget(m_splitter);
    QVBoxLayout* expLayout = new QVBoxLayout(expContainer);
    expLayout->setContentsMargins(0, 0, 2, 0);
    expLayout->setSpacing(0);

    m_expensesTree = new QTreeWidget(expContainer);
    m_expensesTree->setHeaderLabels({"Trading & P&L Particulars (Expenses / Dr)", "Amount"});
    m_expensesTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_expensesTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_expensesTree->setAlternatingRowColors(true);
    m_expensesTree->setRootIsDecorated(false);
    m_expensesTree->setAnimated(false);
    m_expensesTree->setUniformRowHeights(false);
    m_expensesTree->setFocusPolicy(Qt::StrongFocus);
    m_expensesTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_expensesTree->installEventFilter(this);
    connect(m_expensesTree, &QTreeWidget::itemActivated, this, &ProfitLossWidget::onExpensesItemActivated);
    connect(m_expensesTree, &QTreeWidget::itemDoubleClicked, this, &ProfitLossWidget::onExpensesItemActivated);
    expLayout->addWidget(m_expensesTree);
    m_splitter->addWidget(expContainer);

    // Right Side: Incomes Tree (Cr)
    QWidget* incContainer = new QWidget(m_splitter);
    QVBoxLayout* incLayout = new QVBoxLayout(incContainer);
    incLayout->setContentsMargins(2, 0, 0, 0);
    incLayout->setSpacing(0);

    m_incomesTree = new QTreeWidget(incContainer);
    m_incomesTree->setHeaderLabels({"Trading & P&L Particulars (Incomes / Cr)", "Amount"});
    m_incomesTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_incomesTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_incomesTree->setAlternatingRowColors(true);
    m_incomesTree->setRootIsDecorated(false);
    m_incomesTree->setAnimated(false);
    m_incomesTree->setUniformRowHeights(false);
    m_incomesTree->setFocusPolicy(Qt::StrongFocus);
    m_incomesTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_incomesTree->installEventFilter(this);
    connect(m_incomesTree, &QTreeWidget::itemActivated, this, &ProfitLossWidget::onIncomesItemActivated);
    connect(m_incomesTree, &QTreeWidget::itemDoubleClicked, this, &ProfitLossWidget::onIncomesItemActivated);
    incLayout->addWidget(m_incomesTree);
    m_splitter->addWidget(incContainer);

    m_splitter->setSizes({600, 600});
    mainLayout->addWidget(m_splitter, 1);

    // ==========================================
    // 3. FOOTER SUMMARY
    // ==========================================
    QFrame* footerCard = new QFrame(this);
    footerCard->setFixedHeight(40);
    footerCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 8px; }"
        "QLabel { border: none; background: transparent; }"
    );

    QHBoxLayout* footerLayout = new QHBoxLayout(footerCard);
    footerLayout->setContentsMargins(16, 0, 16, 0);
    footerLayout->setSpacing(24);

    m_grossProfitBadge = new QLabel("Gross Profit: ₹ 0.00", footerCard);
    m_grossProfitBadge->setStyleSheet("color: #0F172A; font-size: 13.5px; font-weight: 800; background: transparent;");
    footerLayout->addWidget(m_grossProfitBadge);

    m_netProfitBadge = new QLabel("Net Profit: ₹ 0.00", footerCard);
    m_netProfitBadge->setStyleSheet("color: #16A34A; font-size: 14.5px; font-weight: 900; background: transparent;");
    footerLayout->addWidget(m_netProfitBadge);

    footerLayout->addStretch(1);

    mainLayout->addWidget(footerCard);

    // ==========================================
    // 4. BOTTOM COMMAND KEY STRIP
    // ==========================================
    m_keyLegendLbl = new QLabel("Alt+B: Balance Sheet  •  Alt+M: Ledger Alteration  •  F2: Period  •  Ctrl+P: Print  •  Esc: Back", this);
    m_keyLegendLbl->setFixedHeight(26);
    m_keyLegendLbl->setAlignment(Qt::AlignCenter);
    m_keyLegendLbl->setStyleSheet(
        "background-color: #1E293B; color: #94A3B8; font-size: 11.5px; font-weight: 600; "
        "padding: 2px; border-radius: 6px; letter-spacing: 0.3px; border: none;"
    );
    mainLayout->addWidget(m_keyLegendLbl);
}

void ProfitLossWidget::applyCustomStyles() {
    QString treeStyle =
        "QTreeWidget {"
        "    background-color: #FFFFFF;"
        "    alternate-background-color: #F8FAFC;"
        "    border: 1px solid #CBD5E1;"
        "    border-radius: 8px;"
        "    font-size: 12.5px;"
        "    color: #0F172A;"
        "    outline: none;"
        "}"
        "QTreeWidget::item {"
        "    padding: 5px 8px;"
        "    min-height: 26px;"
        "    border-bottom: 1px solid #F1F5F9;"
        "}"
        "QTreeWidget::item:hover {"
        "    background-color: #F1F5F9;"
        "}"
        "QTreeWidget::item:selected {"
        "    background-color: #EFF6FF;"
        "    color: #1D4ED8;"
        "    font-weight: bold;"
        "}"
        "QHeaderView::section {"
        "    background-color: #F1F5F9;"
        "    color: #1E293B;"
        "    font-size: 12px;"
        "    font-weight: 800;"
        "    padding: 8px 10px;"
        "    border: none;"
        "    border-bottom: 2px solid #CBD5E1;"
        "    border-right: 1px solid #E2E8F0;"
        "}";

    m_expensesTree->setStyleSheet(treeStyle);
    m_incomesTree->setStyleSheet(treeStyle);
}

void ProfitLossWidget::refreshData(const QString& fromDateIso, const QString& toDateIso) {
    if (m_controller) {
        m_controller->reload(fromDateIso, toDateIso);
    }
    m_isDirty = false;
}

void ProfitLossWidget::setDateRange(const QString& fromDateIso, const QString& toDateIso) {
    if (m_fromDateEdit) m_fromDateEdit->setIsoDate(fromDateIso);
    if (m_toDateEdit) m_toDateEdit->setIsoDate(toDateIso);
    m_isDirty = true;
    refreshData(fromDateIso, toDateIso);
}

void ProfitLossWidget::onDateFilterChanged() {
    QString fDate = m_fromDateEdit ? m_fromDateEdit->isoDate() : "";
    QString tDate = m_toDateEdit ? m_toDateEdit->isoDate() : "";
    if (!fDate.isEmpty() && !tDate.isEmpty()) {
        m_isDirty = true;
        refreshData(fDate, tDate);
    }
}

void ProfitLossWidget::populateTrees() {
    const auto& d = m_controller->data();

    // Update Header
    m_firmLabel->setText(QString("%1 (%2)").arg(d.firmName, d.financialYear));
    m_fyBadge->setText(QString("(From %1 To %2)").arg(FiscalYearHelper::formatDisplayDate(d.fromDate), FiscalYearHelper::formatDisplayDate(d.toDate)));

    m_fromDateEdit->blockSignals(true);
    m_toDateEdit->blockSignals(true);
    m_fromDateEdit->setIsoDate(d.fromDate);
    m_toDateEdit->setIsoDate(d.toDate);
    m_fromDateEdit->blockSignals(false);
    m_toDateEdit->blockSignals(false);

    m_expensesTree->setUpdatesEnabled(false);
    m_incomesTree->setUpdatesEnabled(false);

    m_expensesTree->clear();
    m_incomesTree->clear();

    QFont secFont;
    secFont.setBold(true);
    secFont.setPixelSize(13);

    QFont grpFont;
    grpFont.setBold(true);
    grpFont.setPixelSize(13);
    grpFont.setUnderline(true);

    QFont subFont;
    subFont.setPixelSize(12);

    QFont amtFont;
    amtFont.setBold(true);
    amtFont.setPixelSize(12);

    // -------------------------------------------------------------
    // POPULATE EXPENSES (DR) TREE
    // -------------------------------------------------------------
    for (const auto& item : d.expensesSide) {
        QTreeWidgetItem* row = new QTreeWidgetItem(m_expensesTree);
        row->setText(0, item.name);
        row->setText(1, item.amountFmt);
        row->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
        row->setData(0, Qt::UserRole, item.partyId);
        row->setData(0, Qt::UserRole + 1, item.name);
        row->setData(0, Qt::UserRole + 2, item.groupName);

        if (item.isSectionHeader) {
            row->setFont(0, secFont);
            row->setFont(1, secFont);
            row->setBackground(0, QBrush(QColor("#F1F5F9")));
            row->setBackground(1, QBrush(QColor("#F1F5F9")));
            row->setForeground(0, QBrush(QColor("#0F172A")));
            row->setForeground(1, QBrush(QColor("#0F172A")));
            row->setFlags(Qt::ItemIsEnabled);
        } else if (item.isCalculated) {
            row->setFont(0, grpFont);
            row->setFont(1, amtFont);
            QColor clr = item.name.contains("Loss", Qt::CaseInsensitive) ? QColor("#DC2626") : QColor("#16A34A");
            row->setForeground(0, QBrush(clr));
            row->setForeground(1, QBrush(clr));
        } else if (item.isGroup) {
            row->setFont(0, grpFont);
            row->setFont(1, amtFont);
            row->setForeground(0, QBrush(QColor("#0F172A")));
            row->setForeground(1, QBrush(QColor("#0F172A")));

            for (const auto& child : item.children) {
                QTreeWidgetItem* childRow = new QTreeWidgetItem(row);
                childRow->setText(0, "   " + child.name);
                childRow->setText(1, child.amountFmt);
                childRow->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
                childRow->setFont(0, subFont);
                childRow->setFont(1, amtFont);
                childRow->setForeground(0, QBrush(QColor("#334155")));
                childRow->setForeground(1, QBrush(QColor("#0F172A")));
                childRow->setData(0, Qt::UserRole, child.partyId);
                childRow->setData(0, Qt::UserRole + 1, child.name);
                childRow->setData(0, Qt::UserRole + 2, child.groupName);
            }
        } else {
            row->setFont(0, subFont);
            row->setFont(1, amtFont);
            row->setForeground(0, QBrush(QColor("#334155")));
            row->setForeground(1, QBrush(QColor("#0F172A")));
        }
    }

    // -------------------------------------------------------------
    // POPULATE INCOMES (CR) TREE
    // -------------------------------------------------------------
    for (const auto& item : d.incomesSide) {
        QTreeWidgetItem* row = new QTreeWidgetItem(m_incomesTree);
        row->setText(0, item.name);
        row->setText(1, item.amountFmt);
        row->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
        row->setData(0, Qt::UserRole, item.partyId);
        row->setData(0, Qt::UserRole + 1, item.name);
        row->setData(0, Qt::UserRole + 2, item.groupName);

        if (item.isSectionHeader) {
            row->setFont(0, secFont);
            row->setFont(1, secFont);
            row->setBackground(0, QBrush(QColor("#F1F5F9")));
            row->setBackground(1, QBrush(QColor("#F1F5F9")));
            row->setForeground(0, QBrush(QColor("#0F172A")));
            row->setForeground(1, QBrush(QColor("#0F172A")));
            row->setFlags(Qt::ItemIsEnabled);
        } else if (item.isCalculated) {
            row->setFont(0, grpFont);
            row->setFont(1, amtFont);
            QColor clr = item.name.contains("Loss", Qt::CaseInsensitive) ? QColor("#DC2626") : QColor("#16A34A");
            row->setForeground(0, QBrush(clr));
            row->setForeground(1, QBrush(clr));
        } else if (item.isGroup) {
            row->setFont(0, grpFont);
            row->setFont(1, amtFont);
            row->setForeground(0, QBrush(QColor("#0F172A")));
            row->setForeground(1, QBrush(QColor("#0F172A")));

            for (const auto& child : item.children) {
                QTreeWidgetItem* childRow = new QTreeWidgetItem(row);
                childRow->setText(0, "   " + child.name);
                childRow->setText(1, child.amountFmt);
                childRow->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
                childRow->setFont(0, subFont);
                childRow->setFont(1, amtFont);
                childRow->setForeground(0, QBrush(QColor("#334155")));
                childRow->setForeground(1, QBrush(QColor("#0F172A")));
                childRow->setData(0, Qt::UserRole, child.partyId);
                childRow->setData(0, Qt::UserRole + 1, child.name);
                childRow->setData(0, Qt::UserRole + 2, child.groupName);
            }
        } else {
            row->setFont(0, subFont);
            row->setFont(1, amtFont);
            row->setForeground(0, QBrush(QColor("#334155")));
            row->setForeground(1, QBrush(QColor("#0F172A")));
        }
    }

    m_expensesTree->expandAll();
    m_incomesTree->expandAll();
    if (m_expensesTree->topLevelItemCount() > 0 && !m_expensesTree->currentItem() && !m_incomesTree->currentItem()) {
        m_expensesTree->setCurrentItem(m_expensesTree->topLevelItem(0));
    }

    m_expensesTree->setUpdatesEnabled(true);
    m_incomesTree->setUpdatesEnabled(true);

    // Footer Updates
    if (d.grossProfit >= 0.0) {
        m_grossProfitBadge->setText(QString("Gross Profit: %1").arg(m_controller->grossProfitFmt()));
        m_grossProfitBadge->setStyleSheet("color: #92400E; font-size: 13.5px; font-weight: 900; background: transparent;");
    } else {
        m_grossProfitBadge->setText(QString("Gross Loss: %1").arg(m_controller->grossProfitFmt()));
        m_grossProfitBadge->setStyleSheet("color: #DC2626; font-size: 13.5px; font-weight: 900; background: transparent;");
    }

    if (d.netProfit >= 0.0) {
        m_netProfitBadge->setText(QString("Net Profit: %1").arg(m_controller->netProfitFmt()));
        m_netProfitBadge->setStyleSheet("color: #15803D; font-size: 14px; font-weight: 900; background: transparent;");
    } else {
        m_netProfitBadge->setText(QString("Net Loss: %1").arg(m_controller->netProfitFmt()));
        m_netProfitBadge->setStyleSheet("color: #DC2626; font-size: 14px; font-weight: 900; background: transparent;");
    }
}

void ProfitLossWidget::expandAllGroups() {
    m_expensesTree->expandAll();
    m_incomesTree->expandAll();
}

void ProfitLossWidget::collapseAllGroups() {
    m_expensesTree->collapseAll();
    m_incomesTree->collapseAll();
}

void ProfitLossWidget::exportPdf() {
    QString path = m_controller->exportPdf();
    if (!path.isEmpty()) {
        QMessageBox::information(this, "Export Success", "Trading and Profit & Loss Statement exported to PDF successfully:\n" + path);
    }
}

void ProfitLossWidget::exportCsv() {
    QString path = m_controller->exportCsv();
    if (!path.isEmpty()) {
        QMessageBox::information(this, "Export Success", "Trading and Profit & Loss Statement exported to CSV successfully:\n" + path);
    }
}

void ProfitLossWidget::printReport() {
    m_controller->print();
}

void ProfitLossWidget::onExpensesItemActivated(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    m_activeSide = ActiveSide::Expenses;
    handleItemDrillDown(item);
}

void ProfitLossWidget::onIncomesItemActivated(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    m_activeSide = ActiveSide::Incomes;
    handleItemDrillDown(item);
}

void ProfitLossWidget::triggerDrillDownOnCurrentItem() {
    QTreeWidget* focused = nullptr;
    if (m_expensesTree && m_expensesTree->hasFocus()) {
        m_activeSide = ActiveSide::Expenses;
        focused = m_expensesTree;
    } else if (m_incomesTree && m_incomesTree->hasFocus()) {
        m_activeSide = ActiveSide::Incomes;
        focused = m_incomesTree;
    } else if (m_activeSide == ActiveSide::Incomes && m_incomesTree && m_incomesTree->currentItem()) {
        focused = m_incomesTree;
    } else if (m_expensesTree && m_expensesTree->currentItem()) {
        focused = m_expensesTree;
    }

    if (focused && focused->currentItem()) {
        handleItemDrillDown(focused->currentItem());
    }
}

void ProfitLossWidget::focusActiveTree() {
    QTreeWidget* targetTree = (m_activeSide == ActiveSide::Incomes) ? m_incomesTree : m_expensesTree;
    if (targetTree) {
        targetTree->setFocus(Qt::OtherFocusReason);
        if (targetTree->currentItem()) {
            targetTree->scrollToItem(targetTree->currentItem());
        }
    }
}

void ProfitLossWidget::focusInEvent(QFocusEvent* event) {
    QWidget::focusInEvent(event);
    focusActiveTree();
}

void ProfitLossWidget::handleItemDrillDown(QTreeWidgetItem* item) {
    if (!item) return;

    int pId = item->data(0, Qt::UserRole).toInt();
    QString rawName = item->data(0, Qt::UserRole + 1).toString().trimmed();
    QString grpName = item->data(0, Qt::UserRole + 2).toString().trimmed();

    if (rawName.isEmpty()) rawName = item->text(0).trimmed();
    while (rawName.startsWith('-') || rawName.startsWith(QChar(0x2022)) || rawName.startsWith(' ')) {
        rawName = rawName.mid(1).trimmed();
    }

    if (item->childCount() > 0) {
        item->setExpanded(!item->isExpanded());
        return;
    }

    // 1. Stock item or stock valuation drill-down
    if (grpName.contains("Stock", Qt::CaseInsensitive) ||
        rawName.contains("Stock", Qt::CaseInsensitive) ||
        rawName.contains("Inventory", Qt::CaseInsensitive) ||
        rawName.contains("Paddy", Qt::CaseInsensitive) ||
        rawName.contains("Rice", Qt::CaseInsensitive) ||
        rawName.contains("Bran", Qt::CaseInsensitive) ||
        rawName.contains("Husk", Qt::CaseInsensitive) ||
        rawName.contains("Nakku", Qt::CaseInsensitive) ||
        rawName.contains("Bardana", Qt::CaseInsensitive)) {
        emit openStockRegisterRequested(rawName);
        return;
    }

    // 2. Ledger / Party Statement drill-down
    if (!rawName.isEmpty() &&
        !rawName.startsWith("TRADING", Qt::CaseInsensitive) &&
        !rawName.startsWith("PROFIT", Qt::CaseInsensitive) &&
        !rawName.contains("Gross Profit", Qt::CaseInsensitive) &&
        !rawName.contains("Gross Loss", Qt::CaseInsensitive) &&
        !rawName.contains("Net Profit", Qt::CaseInsensitive) &&
        !rawName.contains("Net Loss", Qt::CaseInsensitive)) {
        const auto& d = m_controller->data();
        emit openPartyStatement(rawName, d.fromDate, d.toDate);
    }
}

bool ProfitLossWidget::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::FocusIn) {
        if (watched == m_expensesTree) m_activeSide = ActiveSide::Expenses;
        else if (watched == m_incomesTree) m_activeSide = ActiveSide::Incomes;
    } else if (event->type() == QEvent::KeyPress) {
        QKeyEvent* kEvent = static_cast<QKeyEvent*>(event);
        if (kEvent->key() == Qt::Key_Return || kEvent->key() == Qt::Key_Enter) {
            QTreeWidget* tree = qobject_cast<QTreeWidget*>(watched);
            if (tree && tree->currentItem()) {
                m_activeSide = (tree == m_incomesTree) ? ActiveSide::Incomes : ActiveSide::Expenses;
                handleItemDrillDown(tree->currentItem());
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ProfitLossWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QTreeWidget* focusedTree = nullptr;
        if (m_expensesTree->hasFocus()) {
            m_activeSide = ActiveSide::Expenses;
            focusedTree = m_expensesTree;
        } else if (m_incomesTree->hasFocus()) {
            m_activeSide = ActiveSide::Incomes;
            focusedTree = m_incomesTree;
        } else if (m_activeSide == ActiveSide::Incomes && m_incomesTree && m_incomesTree->currentItem()) {
            focusedTree = m_incomesTree;
        } else if (m_expensesTree && m_expensesTree->currentItem()) {
            focusedTree = m_expensesTree;
        }

        if (focusedTree && focusedTree->currentItem()) {
            event->accept();
            handleItemDrillDown(focusedTree->currentItem());
            return;
        }
    }

    if (event->key() == Qt::Key_F2) {
        emit requestAccountingPeriodDialog();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_F5) {
        expandAllGroups();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_F6) {
        collapseAllGroups();
        event->accept();
        return;
    }

    if (event->matches(QKeySequence::Print) || (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_P)) {
        printReport();
        event->accept();
        return;
    }

    if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_E) {
        exportPdf();
        event->accept();
        return;
    }

    // Instantaneous Arrow Key Navigation
    if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down) {
        if (!m_expensesTree->hasFocus() && !m_incomesTree->hasFocus()) {
            focusActiveTree();
            if (m_activeSide == ActiveSide::Expenses && m_expensesTree->topLevelItemCount() > 0 && !m_expensesTree->currentItem()) {
                m_expensesTree->setCurrentItem(m_expensesTree->topLevelItem(0));
            } else if (m_activeSide == ActiveSide::Incomes && m_incomesTree->topLevelItemCount() > 0 && !m_incomesTree->currentItem()) {
                m_incomesTree->setCurrentItem(m_incomesTree->topLevelItem(0));
            }
        }
    }

    // Switch between Expenses and Incomes trees with Left / Right arrows
    if (event->key() == Qt::Key_Right) {
        if (!m_incomesTree->hasFocus()) {
            event->accept();
            m_activeSide = ActiveSide::Incomes;
            m_lastExpIndex = m_expensesTree->indexOfTopLevelItem(m_expensesTree->currentItem());
            m_incomesTree->setFocus(Qt::OtherFocusReason);
            if (m_incomesTree->currentItem()) {
                m_incomesTree->setCurrentItem(m_incomesTree->currentItem());
            } else if (m_incomesTree->topLevelItemCount() > 0) {
                int idx = qBound(0, m_lastIncIndex, m_incomesTree->topLevelItemCount() - 1);
                m_incomesTree->setCurrentItem(m_incomesTree->topLevelItem(idx));
            }
            return;
        }
    } else if (event->key() == Qt::Key_Left) {
        if (!m_expensesTree->hasFocus()) {
            event->accept();
            m_activeSide = ActiveSide::Expenses;
            m_lastIncIndex = m_incomesTree->indexOfTopLevelItem(m_incomesTree->currentItem());
            m_expensesTree->setFocus(Qt::OtherFocusReason);
            if (m_expensesTree->currentItem()) {
                m_expensesTree->setCurrentItem(m_expensesTree->currentItem());
            } else if (m_expensesTree->topLevelItemCount() > 0) {
                int idx = qBound(0, m_lastExpIndex, m_expensesTree->topLevelItemCount() - 1);
                m_expensesTree->setCurrentItem(m_expensesTree->topLevelItem(idx));
            }
            return;
        }
    }

    QWidget::keyPressEvent(event);
}

void ProfitLossWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    if (m_isDirty || (m_controller && (m_controller->data().financialYear != activeFy.name || m_controller->data().fromDate.isEmpty()))) {
        refreshData(activeFy.startDate, activeFy.endDate);
    }
    focusActiveTree();
}

void ProfitLossWidget::paintEvent(QPaintEvent* event) {
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    QWidget::paintEvent(event);
}
