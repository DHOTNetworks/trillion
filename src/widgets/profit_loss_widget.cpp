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

    refreshData();
}

void ProfitLossWidget::setupUi() {
    setAttribute(Qt::WA_StyledBackground, true);
    setFocusPolicy(Qt::StrongFocus);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 6, 8, 6);
    mainLayout->setSpacing(4);

    // ==========================================
    // 1. TOP FIRM & TITLE BANNER (BAHI KHATA STYLE)
    // ==========================================
    QLabel* compBanner = new QLabel(this);
    compBanner->setFixedHeight(28);
    compBanner->setAlignment(Qt::AlignCenter);
    compBanner->setStyleSheet(
        "background-color: #86EFAC; color: #064E3B; font-size: 14.5px; font-weight: 900; "
        "font-style: italic; border: 1px solid #4ADE80; border-radius: 4px; letter-spacing: 0.5px;"
    );
    compBanner->setText("M/S MAHADEV RICE INDUSTRY (2025-2026)");
    m_firmLabel = compBanner;
    mainLayout->addWidget(compBanner);

    // Sub-header Bar (Peach/Cream)
    QFrame* subHeaderCard = new QFrame(this);
    subHeaderCard->setFixedHeight(46);
    subHeaderCard->setStyleSheet(
        "background-color: #FED7AA; border: 1px solid #FDBA74; border-radius: 4px; padding: 2px 8px;"
    );
    QHBoxLayout* subHeaderLayout = new QHBoxLayout(subHeaderCard);
    subHeaderLayout->setContentsMargins(6, 0, 6, 0);
    subHeaderLayout->setSpacing(10);

    QLabel* f5Hint = new QLabel("(F5: Extract/UnExtract Group)", subHeaderCard);
    f5Hint->setStyleSheet("color: #1D4ED8; font-size: 12px; font-weight: 700; background: transparent;");
    subHeaderLayout->addWidget(f5Hint);

    QVBoxLayout* centerTitleBox = new QVBoxLayout();
    centerTitleBox->setContentsMargins(0, 2, 0, 2);
    centerTitleBox->setSpacing(0);
    m_titleLabel = new QLabel("Trading and Profit & Loss A/c (Provisional)", subHeaderCard);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("font-size: 15px; font-weight: 900; font-style: italic; color: #0F172A; background: transparent;");
    centerTitleBox->addWidget(m_titleLabel);

    m_fyBadge = new QLabel("(From 01/04/2025 To 31/03/2026)", subHeaderCard);
    m_fyBadge->setAlignment(Qt::AlignCenter);
    m_fyBadge->setStyleSheet("font-size: 12.5px; font-weight: 700; color: #1E293B; background: transparent;");
    centerTitleBox->addWidget(m_fyBadge);
    subHeaderLayout->addLayout(centerTitleBox, 1);

    // Date range & Buttons
    QHBoxLayout* btnBox = new QHBoxLayout();
    btnBox->setSpacing(4);

    m_fromDateEdit = new AccountingDateEdit(subHeaderCard);
    m_fromDateEdit->setFixedWidth(100);
    m_fromDateEdit->setStyleSheet(
        "QLineEdit { background: #FFFFFF; border: 1.5px solid #D97706; border-radius: 4px; "
        "padding: 3px 6px; font-size: 12px; font-weight: 800; color: #78350F; } "
        "QLineEdit:focus { border: 2px solid #B45309; background: #FFFBEB; }"
    );
    connect(m_fromDateEdit, &AccountingDateEdit::dateChanged, this, &ProfitLossWidget::onDateFilterChanged);
    btnBox->addWidget(m_fromDateEdit);

    QLabel* toSep = new QLabel("to", subHeaderCard);
    toSep->setStyleSheet("font-weight: bold; color: #78350F; background: transparent;");
    btnBox->addWidget(toSep);

    m_toDateEdit = new AccountingDateEdit(subHeaderCard);
    m_toDateEdit->setFixedWidth(100);
    m_toDateEdit->setStyleSheet(
        "QLineEdit { background: #FFFFFF; border: 1.5px solid #D97706; border-radius: 4px; "
        "padding: 3px 6px; font-size: 12px; font-weight: 800; color: #78350F; } "
        "QLineEdit:focus { border: 2px solid #B45309; background: #FFFBEB; }"
    );
    connect(m_toDateEdit, &AccountingDateEdit::dateChanged, this, &ProfitLossWidget::onDateFilterChanged);
    btnBox->addWidget(m_toDateEdit);

    m_periodBtn = new KbdBadgeButton("Period", "F2", QColor("#F8FAFC"), QColor("#F1F5F9"), QColor("#0F172A"), QColor("#CBD5E1"), subHeaderCard);
    m_periodBtn->setFixedHeight(28);
    connect(m_periodBtn, &QPushButton::clicked, this, &ProfitLossWidget::requestAccountingPeriodDialog);
    btnBox->addWidget(m_periodBtn);

    m_expandBtn = new KbdBadgeButton("Expand", "F5", QColor("#F8FAFC"), QColor("#F1F5F9"), QColor("#0F172A"), QColor("#CBD5E1"), subHeaderCard);
    m_expandBtn->setFixedHeight(28);
    connect(m_expandBtn, &QPushButton::clicked, this, &ProfitLossWidget::expandAllGroups);
    btnBox->addWidget(m_expandBtn);

    m_collapseBtn = new KbdBadgeButton("Collapse", "F6", QColor("#F8FAFC"), QColor("#F1F5F9"), QColor("#0F172A"), QColor("#CBD5E1"), subHeaderCard);
    m_collapseBtn->setFixedHeight(28);
    connect(m_collapseBtn, &QPushButton::clicked, this, &ProfitLossWidget::collapseAllGroups);
    btnBox->addWidget(m_collapseBtn);

    m_printBtn = new KbdBadgeButton("Print", "Ctrl+P", QColor("#F8FAFC"), QColor("#F1F5F9"), QColor("#0F172A"), QColor("#CBD5E1"), subHeaderCard);
    m_printBtn->setFixedHeight(28);
    connect(m_printBtn, &QPushButton::clicked, this, &ProfitLossWidget::printReport);
    btnBox->addWidget(m_printBtn);

    m_pdfBtn = new KbdBadgeButton("PDF", "Ctrl+E", QColor("#DC2626"), QColor("#B91C1C"), QColor("#FFFFFF"), QColor("#991B1B"), subHeaderCard);
    m_pdfBtn->setFixedHeight(28);
    connect(m_pdfBtn, &QPushButton::clicked, this, &ProfitLossWidget::exportPdf);
    btnBox->addWidget(m_pdfBtn);

    m_backBtn = new KbdBadgeButton("Back", "Esc", QColor("#64748B"), QColor("#475569"), QColor("#FFFFFF"), QColor("#334155"), subHeaderCard);
    m_backBtn->setFixedHeight(28);
    connect(m_backBtn, &QPushButton::clicked, this, &ProfitLossWidget::backRequested);
    btnBox->addWidget(m_backBtn);

    subHeaderLayout->addLayout(btnBox);
    mainLayout->addWidget(subHeaderCard);

    // ==========================================
    // 2. MAIN SPLIT TREE VIEWS (T-FORMAT TRADING & P&L)
    // ==========================================
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setHandleWidth(3);
    m_splitter->setStyleSheet("QSplitter::handle { background-color: #94A3B8; }");

    // Left Side: Expenses Tree (Dr)
    QWidget* expContainer = new QWidget(m_splitter);
    QVBoxLayout* expLayout = new QVBoxLayout(expContainer);
    expLayout->setContentsMargins(0, 0, 1, 0);
    expLayout->setSpacing(0);

    m_expensesTree = new QTreeWidget(expContainer);
    m_expensesTree->setHeaderLabels({"Trading & P&L Particulars (Expenses / Dr)", "Amount"});
    m_expensesTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_expensesTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_expensesTree->setAlternatingRowColors(false);
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
    incLayout->setContentsMargins(1, 0, 0, 0);
    incLayout->setSpacing(0);

    m_incomesTree = new QTreeWidget(incContainer);
    m_incomesTree->setHeaderLabels({"Trading & P&L Particulars (Incomes / Cr)", "Amount"});
    m_incomesTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_incomesTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_incomesTree->setAlternatingRowColors(false);
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
    // 3. FOOTER SUMMARY (BAHI KHATA GRAND TOTAL STYLE)
    // ==========================================
    QFrame* footerCard = new QFrame(this);
    footerCard->setFixedHeight(34);
    footerCard->setStyleSheet(
        "background-color: #FED7AA; border: 1.5px solid #FDBA74; border-radius: 4px; padding: 2px 8px;"
    );

    QHBoxLayout* footerLayout = new QHBoxLayout(footerCard);
    footerLayout->setContentsMargins(10, 0, 10, 0);
    footerLayout->setSpacing(20);

    m_grossProfitBadge = new QLabel("Gross Profit: ₹ 0.00", footerCard);
    m_grossProfitBadge->setStyleSheet("color: #92400E; font-size: 13.5px; font-weight: 900; background: transparent;");
    footerLayout->addWidget(m_grossProfitBadge);

    m_netProfitBadge = new QLabel("Net Profit: ₹ 0.00", footerCard);
    m_netProfitBadge->setStyleSheet("color: #DC2626; font-size: 14px; font-weight: 900; background: transparent;");
    footerLayout->addWidget(m_netProfitBadge);

    footerLayout->addStretch(1);

    mainLayout->addWidget(footerCard);

    // ==========================================
    // 4. BOTTOM COMMAND KEY STRIP (DEEP MAROON)
    // ==========================================
    m_keyLegendLbl = new QLabel("Alt+B: Balance Sheet  -  Alt+M: Ldgr Alteration  -  Alt+F2: Date Criteria  -  Alt+C: Calculator  -  Alt+P: Print Report", this);
    m_keyLegendLbl->setFixedHeight(26);
    m_keyLegendLbl->setAlignment(Qt::AlignCenter);
    m_keyLegendLbl->setStyleSheet(
        "background-color: #7F1D1D; color: #FFFFFF; font-size: 12px; font-weight: 700; "
        "padding: 2px; border-radius: 3px; letter-spacing: 0.3px;"
    );
    mainLayout->addWidget(m_keyLegendLbl);
}

void ProfitLossWidget::applyCustomStyles() {
    QString treeStyle =
        "QTreeWidget {"
        "    background-color: #FFEDD5;"
        "    border: 1px solid #CBD5E1;"
        "    font-size: 13.5px;"
        "    color: #000000;"
        "}"
        "QTreeWidget::item {"
        "    padding: 3px 6px;"
        "    min-height: 22px;"
        "}"
        "QTreeWidget::item:hover {"
        "    background-color: #FED7AA;"
        "}"
        "QTreeWidget::item:selected {"
        "    background-color: #818CF8;"
        "    color: #000000;"
        "    font-weight: bold;"
        "}"
        "QHeaderView::section {"
        "    background-color: #64748B;"
        "    color: #FFFFFF;"
        "    font-size: 13px;"
        "    font-weight: 900;"
        "    padding: 6px 8px;"
        "    border: 1px solid #475569;"
        "}";

    m_expensesTree->setStyleSheet(treeStyle);
    m_incomesTree->setStyleSheet(treeStyle);
}

void ProfitLossWidget::refreshData(const QString& fromDateIso, const QString& toDateIso) {
    m_controller->reload(fromDateIso, toDateIso);
}

void ProfitLossWidget::setDateRange(const QString& fromDateIso, const QString& toDateIso) {
    m_fromDateEdit->setIsoDate(fromDateIso);
    m_toDateEdit->setIsoDate(toDateIso);
    refreshData(fromDateIso, toDateIso);
}

void ProfitLossWidget::onDateFilterChanged() {
    QString fDate = m_fromDateEdit->isoDate();
    QString tDate = m_toDateEdit->isoDate();
    if (!fDate.isEmpty() && !tDate.isEmpty()) {
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

    // Save scroll & selection positions
    m_lastExpIndex = m_expensesTree->currentIndex().row();
    m_lastIncIndex = m_incomesTree->currentIndex().row();

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
            row->setBackground(0, QBrush(QColor("#64748B")));
            row->setBackground(1, QBrush(QColor("#64748B")));
            row->setForeground(0, QBrush(QColor("#FFFFFF")));
            row->setForeground(1, QBrush(QColor("#FFFFFF")));
            row->setFlags(Qt::ItemIsEnabled);
        } else if (item.isCalculated) {
            row->setFont(0, grpFont);
            row->setFont(1, amtFont);
            row->setForeground(0, QBrush(QColor("#DC2626")));
            row->setForeground(1, QBrush(QColor("#DC2626")));
        } else if (item.isGroup) {
            row->setFont(0, grpFont);
            row->setFont(1, amtFont);
            row->setForeground(0, QBrush(QColor("#0F172A")));
            row->setForeground(1, QBrush(QColor("#0F172A")));

            for (const auto& child : item.children) {
                QTreeWidgetItem* childRow = new QTreeWidgetItem(row);
                childRow->setText(0, "-" + child.name);
                childRow->setText(1, child.amountFmt);
                childRow->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
                childRow->setFont(0, subFont);
                childRow->setFont(1, amtFont);
                childRow->setForeground(0, QBrush(QColor("#0F172A")));
                childRow->setForeground(1, QBrush(QColor("#0F172A")));
                childRow->setData(0, Qt::UserRole, child.partyId);
                childRow->setData(0, Qt::UserRole + 1, child.name);
                childRow->setData(0, Qt::UserRole + 2, child.groupName);
            }
        } else {
            row->setFont(0, subFont);
            row->setFont(1, amtFont);
            row->setForeground(0, QBrush(QColor("#0F172A")));
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
            row->setBackground(0, QBrush(QColor("#64748B")));
            row->setBackground(1, QBrush(QColor("#64748B")));
            row->setForeground(0, QBrush(QColor("#FFFFFF")));
            row->setForeground(1, QBrush(QColor("#FFFFFF")));
            row->setFlags(Qt::ItemIsEnabled);
        } else if (item.isCalculated) {
            row->setFont(0, grpFont);
            row->setFont(1, amtFont);
            row->setForeground(0, QBrush(QColor("#059669")));
            row->setForeground(1, QBrush(QColor("#059669")));
        } else if (item.isGroup) {
            row->setFont(0, grpFont);
            row->setFont(1, amtFont);
            row->setForeground(0, QBrush(QColor("#0F172A")));
            row->setForeground(1, QBrush(QColor("#0F172A")));

            for (const auto& child : item.children) {
                QTreeWidgetItem* childRow = new QTreeWidgetItem(row);
                childRow->setText(0, "-" + child.name);
                childRow->setText(1, child.amountFmt);
                childRow->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
                childRow->setFont(0, subFont);
                childRow->setFont(1, amtFont);
                childRow->setForeground(0, QBrush(QColor("#0F172A")));
                childRow->setForeground(1, QBrush(QColor("#0F172A")));
                childRow->setData(0, Qt::UserRole, child.partyId);
                childRow->setData(0, Qt::UserRole + 1, child.name);
                childRow->setData(0, Qt::UserRole + 2, child.groupName);
            }
        } else {
            row->setFont(0, subFont);
            row->setFont(1, amtFont);
            row->setForeground(0, QBrush(QColor("#0F172A")));
            row->setForeground(1, QBrush(QColor("#0F172A")));
        }
    }

    m_expensesTree->expandAll();
    m_incomesTree->expandAll();

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

    // Restore focus row
    if (m_expensesTree->topLevelItemCount() > 0) {
        int idx = qBound(0, m_lastExpIndex, m_expensesTree->topLevelItemCount() - 1);
        m_expensesTree->setCurrentItem(m_expensesTree->topLevelItem(idx));
        m_expensesTree->setFocus(Qt::OtherFocusReason);
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
    handleItemDrillDown(item);
}

void ProfitLossWidget::onIncomesItemActivated(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    handleItemDrillDown(item);
}

void ProfitLossWidget::triggerDrillDownOnCurrentItem() {
    QTreeWidget* focused = nullptr;
    if (m_expensesTree && m_expensesTree->hasFocus()) focused = m_expensesTree;
    else if (m_incomesTree && m_incomesTree->hasFocus()) focused = m_incomesTree;
    else if (m_expensesTree && m_expensesTree->currentItem()) focused = m_expensesTree;
    else if (m_incomesTree && m_incomesTree->currentItem()) focused = m_incomesTree;

    if (focused && focused->currentItem()) {
        handleItemDrillDown(focused->currentItem());
    }
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
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* kEvent = static_cast<QKeyEvent*>(event);
        if (kEvent->key() == Qt::Key_Return || kEvent->key() == Qt::Key_Enter) {
            QTreeWidget* tree = qobject_cast<QTreeWidget*>(watched);
            if (tree && tree->currentItem()) {
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
        if (m_expensesTree->hasFocus()) focusedTree = m_expensesTree;
        else if (m_incomesTree->hasFocus()) focusedTree = m_incomesTree;
        else if (m_expensesTree->currentItem()) focusedTree = m_expensesTree;
        else if (m_incomesTree->currentItem()) focusedTree = m_incomesTree;

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
            m_expensesTree->setFocus(Qt::OtherFocusReason);
            if (m_expensesTree->topLevelItemCount() > 0 && !m_expensesTree->currentItem()) {
                m_expensesTree->setCurrentItem(m_expensesTree->topLevelItem(0));
            }
        }
    }

    // Switch between Expenses and Incomes trees with Left / Right arrows
    if (event->key() == Qt::Key_Right) {
        if (!m_incomesTree->hasFocus()) {
            m_incomesTree->setFocus(Qt::OtherFocusReason);
            if (m_incomesTree->topLevelItemCount() > 0) {
                if (!m_incomesTree->currentItem()) {
                    m_incomesTree->setCurrentItem(m_incomesTree->topLevelItem(0));
                }
            }
            event->accept();
            return;
        }
    }

    if (event->key() == Qt::Key_Left) {
        if (!m_expensesTree->hasFocus()) {
            m_expensesTree->setFocus(Qt::OtherFocusReason);
            if (m_expensesTree->topLevelItemCount() > 0) {
                if (!m_expensesTree->currentItem()) {
                    m_expensesTree->setCurrentItem(m_expensesTree->topLevelItem(0));
                }
            }
            event->accept();
            return;
        }
    }

    QWidget::keyPressEvent(event);
}

void ProfitLossWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    if (m_controller && (m_controller->data().financialYear != activeFy.name || m_controller->data().fromDate != activeFy.startDate || m_controller->data().toDate != activeFy.endDate)) {
        refreshData(activeFy.startDate, activeFy.endDate);
    }
    if (m_expensesTree && m_expensesTree->topLevelItemCount() > 0) {
        m_expensesTree->setFocus(Qt::OtherFocusReason);
        if (!m_expensesTree->currentItem()) {
            m_expensesTree->setCurrentItem(m_expensesTree->topLevelItem(0));
        }
    }
}

void ProfitLossWidget::paintEvent(QPaintEvent* event) {
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    QWidget::paintEvent(event);
}
