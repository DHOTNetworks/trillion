#include "balance_sheet_widget.h"
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

BalanceSheetWidget::BalanceSheetWidget(BalanceSheetController* controller,
                                       PrintExportController* printExportCtrl,
                                       QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_printExportCtrl(printExportCtrl)
{
    setupUi();
    applyCustomStyles();

    connect(m_controller, &BalanceSheetController::dataChanged, this, &BalanceSheetWidget::populateTrees);
    connect(m_controller, &BalanceSheetController::totalsChanged, this, &BalanceSheetWidget::populateTrees);

    refreshData();
}

void BalanceSheetWidget::setupUi() {
    setAttribute(Qt::WA_StyledBackground, true);
    setFocusPolicy(Qt::StrongFocus);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 6, 8, 6);
    mainLayout->setSpacing(4);

    // ==========================================
    // 1. TOP FIRM & TITLE BANNER (BAHI KHATA STYLE)
    // ==========================================
    // Top Green Firm Title Banner
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
    m_titleLabel = new QLabel("Balance Sheet (Provisional)", subHeaderCard);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("font-size: 15px; font-weight: 900; font-style: italic; color: #0F172A; background: transparent;");
    centerTitleBox->addWidget(m_titleLabel);

    m_fyBadge = new QLabel("(From 01/04/2025 To 31/03/2026)", subHeaderCard);
    m_fyBadge->setAlignment(Qt::AlignCenter);
    m_fyBadge->setStyleSheet("font-size: 12.5px; font-weight: 700; color: #1E293B; background: transparent;");
    centerTitleBox->addWidget(m_fyBadge);
    subHeaderLayout->addLayout(centerTitleBox, 1);

    // Quick Date & Buttons
    QHBoxLayout* btnBox = new QHBoxLayout();
    btnBox->setSpacing(4);

    m_asOnDateEdit = new AccountingDateEdit(subHeaderCard);
    m_asOnDateEdit->setFixedWidth(105);
    m_asOnDateEdit->setStyleSheet(
        "QLineEdit { background: #FFFFFF; border: 1.5px solid #D97706; border-radius: 4px; "
        "padding: 3px 6px; font-size: 12px; font-weight: 800; color: #78350F; } "
        "QLineEdit:focus { border: 2px solid #B45309; background: #FFFBEB; }"
    );
    connect(m_asOnDateEdit, &AccountingDateEdit::dateChanged, this, &BalanceSheetWidget::onDateFilterChanged);
    btnBox->addWidget(m_asOnDateEdit);

    m_periodBtn = new KbdBadgeButton("Period", "F2", QColor("#F8FAFC"), QColor("#F1F5F9"), QColor("#0F172A"), QColor("#CBD5E1"), subHeaderCard);
    m_periodBtn->setFixedHeight(28);
    connect(m_periodBtn, &QPushButton::clicked, this, &BalanceSheetWidget::requestAccountingPeriodDialog);
    btnBox->addWidget(m_periodBtn);

    m_expandBtn = new KbdBadgeButton("Expand", "F5", QColor("#F8FAFC"), QColor("#F1F5F9"), QColor("#0F172A"), QColor("#CBD5E1"), subHeaderCard);
    m_expandBtn->setFixedHeight(28);
    connect(m_expandBtn, &QPushButton::clicked, this, &BalanceSheetWidget::expandAllGroups);
    btnBox->addWidget(m_expandBtn);

    m_collapseBtn = new KbdBadgeButton("Collapse", "F6", QColor("#F8FAFC"), QColor("#F1F5F9"), QColor("#0F172A"), QColor("#CBD5E1"), subHeaderCard);
    m_collapseBtn->setFixedHeight(28);
    connect(m_collapseBtn, &QPushButton::clicked, this, &BalanceSheetWidget::collapseAllGroups);
    btnBox->addWidget(m_collapseBtn);

    m_printBtn = new KbdBadgeButton("Print", "Ctrl+P", QColor("#F8FAFC"), QColor("#F1F5F9"), QColor("#0F172A"), QColor("#CBD5E1"), subHeaderCard);
    m_printBtn->setFixedHeight(28);
    connect(m_printBtn, &QPushButton::clicked, this, &BalanceSheetWidget::printReport);
    btnBox->addWidget(m_printBtn);

    m_pdfBtn = new KbdBadgeButton("PDF", "Ctrl+E", QColor("#DC2626"), QColor("#B91C1C"), QColor("#FFFFFF"), QColor("#991B1B"), subHeaderCard);
    m_pdfBtn->setFixedHeight(28);
    connect(m_pdfBtn, &QPushButton::clicked, this, &BalanceSheetWidget::exportPdf);
    btnBox->addWidget(m_pdfBtn);

    m_backBtn = new KbdBadgeButton("Back", "Esc", QColor("#64748B"), QColor("#475569"), QColor("#FFFFFF"), QColor("#334155"), subHeaderCard);
    m_backBtn->setFixedHeight(28);
    connect(m_backBtn, &QPushButton::clicked, this, &BalanceSheetWidget::backRequested);
    btnBox->addWidget(m_backBtn);

    subHeaderLayout->addLayout(btnBox);
    mainLayout->addWidget(subHeaderCard);

    // ==========================================
    // 2. MAIN SPLIT TREE VIEWS (DOUBLE COLUMN T-FORMAT)
    // ==========================================
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setHandleWidth(3);
    m_splitter->setStyleSheet("QSplitter::handle { background-color: #94A3B8; }");

    // Left Side: Liabilities Tree
    QWidget* liabContainer = new QWidget(m_splitter);
    QVBoxLayout* liabLayout = new QVBoxLayout(liabContainer);
    liabLayout->setContentsMargins(0, 0, 1, 0);
    liabLayout->setSpacing(0);

    m_liabilitiesTree = new QTreeWidget(liabContainer);
    m_liabilitiesTree->setHeaderLabels({"Liabilities (51)", "Amount"});
    m_liabilitiesTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_liabilitiesTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_liabilitiesTree->setAlternatingRowColors(false);
    m_liabilitiesTree->setRootIsDecorated(false);
    m_liabilitiesTree->setAnimated(false);
    m_liabilitiesTree->setUniformRowHeights(false);
    m_liabilitiesTree->setFocusPolicy(Qt::StrongFocus);
    m_liabilitiesTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_liabilitiesTree->installEventFilter(this);
    connect(m_liabilitiesTree, &QTreeWidget::itemActivated, this, &BalanceSheetWidget::onLiabilitiesItemActivated);
    connect(m_liabilitiesTree, &QTreeWidget::itemDoubleClicked, this, &BalanceSheetWidget::onLiabilitiesItemActivated);
    liabLayout->addWidget(m_liabilitiesTree);
    m_splitter->addWidget(liabContainer);

    // Right Side: Assets Tree
    QWidget* assetContainer = new QWidget(m_splitter);
    QVBoxLayout* assetLayout = new QVBoxLayout(assetContainer);
    assetLayout->setContentsMargins(1, 0, 0, 0);
    assetLayout->setSpacing(0);

    m_assetsTree = new QTreeWidget(assetContainer);
    m_assetsTree->setHeaderLabels({"Assets (68)", "Amount"});
    m_assetsTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_assetsTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_assetsTree->setAlternatingRowColors(false);
    m_assetsTree->setRootIsDecorated(false);
    m_assetsTree->setAnimated(false);
    m_assetsTree->setUniformRowHeights(false);
    m_assetsTree->setFocusPolicy(Qt::StrongFocus);
    m_assetsTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_assetsTree->installEventFilter(this);
    connect(m_assetsTree, &QTreeWidget::itemActivated, this, &BalanceSheetWidget::onAssetsItemActivated);
    connect(m_assetsTree, &QTreeWidget::itemDoubleClicked, this, &BalanceSheetWidget::onAssetsItemActivated);
    assetLayout->addWidget(m_assetsTree);
    m_splitter->addWidget(assetContainer);

    m_splitter->setSizes({600, 600});
    mainLayout->addWidget(m_splitter, 1);

    // ==========================================
    // 3. GRAND TOTAL FOOTER (BAHI KHATA STYLE)
    // ==========================================
    QFrame* footerCard = new QFrame(this);
    footerCard->setFixedHeight(34);
    footerCard->setStyleSheet(
        "background-color: #FED7AA; border: 1.5px solid #FDBA74; border-radius: 4px; padding: 2px 8px;"
    );
    QHBoxLayout* footerLayout = new QHBoxLayout(footerCard);
    footerLayout->setContentsMargins(10, 0, 10, 0);
    footerLayout->setSpacing(20);

    // Left Grand Total
    QHBoxLayout* lTotBox = new QHBoxLayout();
    QLabel* lTotTitle = new QLabel("Grand Total", footerCard);
    lTotTitle->setStyleSheet("font-size: 14.5px; font-weight: 900; color: #DC2626; background: transparent; text-decoration: underline;");
    m_liabilitiesTotalLbl = new QLabel("49,14,29,438.15", footerCard);
    m_liabilitiesTotalLbl->setStyleSheet("font-size: 15px; font-weight: 900; color: #1E3A8A; font-family: 'Consolas', 'Menlo', 'Monaco', 'DejaVu Sans Mono', 'Liberation Mono', 'Courier New', monospace; background: transparent;");
    lTotBox->addWidget(lTotTitle);
    lTotBox->addStretch();
    lTotBox->addWidget(m_liabilitiesTotalLbl);
    footerLayout->addLayout(lTotBox, 1);

    // Right Grand Total
    QHBoxLayout* rTotBox = new QHBoxLayout();
    QLabel* rTotTitle = new QLabel("Grand Total", footerCard);
    rTotTitle->setStyleSheet("font-size: 14.5px; font-weight: 900; color: #DC2626; background: transparent; text-decoration: underline;");
    m_assetsTotalLbl = new QLabel("49,14,29,438.15", footerCard);
    m_assetsTotalLbl->setStyleSheet("font-size: 15px; font-weight: 900; color: #1E3A8A; font-family: 'Consolas', 'Menlo', 'Monaco', 'DejaVu Sans Mono', 'Liberation Mono', 'Courier New', monospace; background: transparent;");
    rTotBox->addWidget(rTotTitle);
    rTotBox->addStretch();
    rTotBox->addWidget(m_assetsTotalLbl);
    footerLayout->addLayout(rTotBox, 1);

    mainLayout->addWidget(footerCard);

    // ==========================================
    // 4. BOTTOM COMMAND KEY STRIP (DEEP MAROON)
    // ==========================================
    m_keyLegendLbl = new QLabel("Ctrl+Alt+P: Profit & Loss  -  Alt+M: Ldgr Alteration  -  Alt+F2: Date Criteria  -  Alt+C: Calculator  -  Alt+P: Print Report", this);
    m_keyLegendLbl->setFixedHeight(26);
    m_keyLegendLbl->setAlignment(Qt::AlignCenter);
    m_keyLegendLbl->setStyleSheet(
        "background-color: #7F1D1D; color: #FFFFFF; font-size: 12px; font-weight: 700; "
        "padding: 2px; border-radius: 3px; letter-spacing: 0.3px;"
    );
    mainLayout->addWidget(m_keyLegendLbl);
}

void BalanceSheetWidget::applyCustomStyles() {
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

    m_liabilitiesTree->setStyleSheet(treeStyle);
    m_assetsTree->setStyleSheet(treeStyle);
}

void BalanceSheetWidget::refreshData(const QString& asOnDateIso) {
    if (m_controller) {
        m_controller->reload(asOnDateIso);
    }
}

void BalanceSheetWidget::setAsOnDate(const QString& asOnDateIso) {
    if (m_asOnDateEdit) {
        m_asOnDateEdit->setIsoDate(asOnDateIso);
    }
    refreshData(asOnDateIso);
}

void BalanceSheetWidget::onDateFilterChanged() {
    if (m_asOnDateEdit) {
        refreshData(m_asOnDateEdit->isoDate());
    }
}

void BalanceSheetWidget::populateTrees() {
    if (!m_controller) return;

    const BalanceSheetData& d = m_controller->data();

    // Update Header labels
    FiscalYearInfo fy = FiscalYearHelper::getFiscalYearForDate(d.asOnDate);
    m_firmLabel->setText(QString("%1 (%2)").arg(d.firmName, d.financialYear));
    m_fyBadge->setText(QString("(From %1 To %2)").arg(FiscalYearHelper::formatDisplayDate(fy.startDate), FiscalYearHelper::formatDisplayDate(d.asOnDate)));

    m_asOnDateEdit->blockSignals(true);
    if (m_asOnDateEdit->isoDate() != d.asOnDate) {
        m_asOnDateEdit->setIsoDate(d.asOnDate);
    }
    m_asOnDateEdit->blockSignals(false);

    // Block signals while populating
    m_liabilitiesTree->setUpdatesEnabled(false);
    m_assetsTree->setUpdatesEnabled(false);

    m_liabilitiesTree->clear();
    m_assetsTree->clear();

    int liabItemCount = 0;
    for (const auto& g : d.liabilitiesGroups) {
        liabItemCount += g.children.isEmpty() ? 1 : g.children.size();
    }
    int assetItemCount = 0;
    for (const auto& g : d.assetsGroups) {
        assetItemCount += g.children.isEmpty() ? 1 : g.children.size();
    }
    m_liabilitiesTree->setHeaderLabels({QString("Liabilities (%1)").arg(liabItemCount), "Amount"});
    m_assetsTree->setHeaderLabels({QString("Assets (%1)").arg(assetItemCount), "Amount"});

    QFont groupFont;
    groupFont.setBold(true);
    groupFont.setPixelSize(13);
    groupFont.setUnderline(true);

    QFont itemFont;
    itemFont.setPixelSize(12);

    QFont amtFont;
    amtFont.setBold(true);
    amtFont.setPixelSize(12);

    // 1. Populate Liabilities
    for (const auto& g : d.liabilitiesGroups) {
        QTreeWidgetItem* gItem = new QTreeWidgetItem(m_liabilitiesTree);
        gItem->setText(0, g.name.toUpper());
        gItem->setText(1, g.amountFmt);
        gItem->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
        gItem->setFont(0, groupFont);
        gItem->setFont(1, amtFont);
        gItem->setForeground(0, QBrush(QColor("#0F172A")));
        gItem->setForeground(1, QBrush(QColor("#0F172A")));
        gItem->setData(0, Qt::UserRole, "GROUP");

        for (const auto& c : g.children) {
            QTreeWidgetItem* cItem = new QTreeWidgetItem(gItem);
            cItem->setText(0, "-" + c.name);
            cItem->setText(1, c.amountFmt);
            cItem->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
            cItem->setFont(0, itemFont);
            cItem->setFont(1, amtFont);
            cItem->setData(0, Qt::UserRole, c.isCalculated ? "CALCULATED" : "PARTY");
            cItem->setData(0, Qt::UserRole + 1, c.name);
            cItem->setData(0, Qt::UserRole + 2, c.partyId);
            cItem->setData(0, Qt::UserRole + 3, g.name);

            if (c.isCalculated || c.name.contains("Profit", Qt::CaseInsensitive)) {
                cItem->setForeground(0, QBrush(QColor("#DC2626"))); // Red for Net profit
                cItem->setForeground(1, QBrush(QColor("#DC2626")));
                cItem->setFont(0, groupFont);
            }
        }
    }

    // 2. Populate Assets
    for (const auto& g : d.assetsGroups) {
        QTreeWidgetItem* gItem = new QTreeWidgetItem(m_assetsTree);
        gItem->setText(0, g.name.toUpper());
        gItem->setText(1, g.amountFmt);
        gItem->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
        gItem->setFont(0, groupFont);
        gItem->setFont(1, amtFont);
        gItem->setForeground(0, QBrush(QColor("#0F172A")));
        gItem->setForeground(1, QBrush(QColor("#0F172A")));
        gItem->setData(0, Qt::UserRole, "GROUP");
        gItem->setData(0, Qt::UserRole + 1, g.name);

        for (const auto& c : g.children) {
            QTreeWidgetItem* cItem = new QTreeWidgetItem(gItem);
            cItem->setText(0, "-" + c.name);
            cItem->setText(1, c.amountFmt);
            cItem->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
            cItem->setFont(0, itemFont);
            cItem->setFont(1, amtFont);
            cItem->setData(0, Qt::UserRole, c.isCalculated ? "CALCULATED" : "PARTY");
            cItem->setData(0, Qt::UserRole + 1, c.name);
            cItem->setData(0, Qt::UserRole + 2, c.partyId);
            cItem->setData(0, Qt::UserRole + 3, g.name);

            if (c.name.contains("Stock", Qt::CaseInsensitive) || c.isCalculated) {
                cItem->setForeground(0, QBrush(QColor("#1D4ED8")));
            }
        }
    }

    // Expand all items by default for clear overview
    m_liabilitiesTree->expandAll();
    m_assetsTree->expandAll();

    m_liabilitiesTree->setUpdatesEnabled(true);
    m_assetsTree->setUpdatesEnabled(true);

    // Update Totals
    m_liabilitiesTotalLbl->setText(d.totalLiabilitiesFmt);
    m_assetsTotalLbl->setText(d.totalAssetsFmt);

    if (m_balanceStatusBadge) {
        if (d.isBalanced) {
            m_balanceStatusBadge->setText("✔ BALANCED (Diff: ₹ 0.00)");
            m_balanceStatusBadge->setStyleSheet(
                "background-color: #DCFCE7; color: #15803D; font-size: 12px; font-weight: 800; "
                "padding: 4px 14px; border-radius: 6px; border: 1.5px solid #86EFAC;"
            );
        } else {
            m_balanceStatusBadge->setText(QString("⚠ DIFF: %1").arg(m_controller->differenceFmt()));
            m_balanceStatusBadge->setStyleSheet(
                "background-color: #FEE2E2; color: #B91C1C; font-size: 12px; font-weight: 800; "
                "padding: 4px 14px; border-radius: 6px; border: 1.5px solid #FCA5A5;"
            );
        }
    }

    // Select first row in liabilities tree by default
    if (m_liabilitiesTree->topLevelItemCount() > 0) {
        m_liabilitiesTree->setCurrentItem(m_liabilitiesTree->topLevelItem(0));
        m_liabilitiesTree->setFocus();
    }
}

void BalanceSheetWidget::expandAllGroups() {
    m_liabilitiesTree->expandAll();
    m_assetsTree->expandAll();
}

void BalanceSheetWidget::collapseAllGroups() {
    m_liabilitiesTree->collapseAll();
    m_assetsTree->collapseAll();
}

void BalanceSheetWidget::exportPdf() {
    if (!m_controller) return;
    QString outPath = m_controller->exportPdf();
    if (!outPath.isEmpty()) {
        QMessageBox::information(this, "Balance Sheet PDF Export",
                                 QString("Balance Sheet exported successfully to:\n%1").arg(outPath));
    }
}

void BalanceSheetWidget::exportCsv() {
    if (!m_controller) return;
    QString outPath = m_controller->exportCsv();
    if (!outPath.isEmpty()) {
        QMessageBox::information(this, "Balance Sheet CSV Export",
                                 QString("Balance Sheet CSV exported successfully to:\n%1").arg(outPath));
    }
}

void BalanceSheetWidget::printReport() {
    if (!m_controller) return;
    m_controller->print();
}

void BalanceSheetWidget::onLiabilitiesItemActivated(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    handleItemDrillDown(item);
}

void BalanceSheetWidget::onAssetsItemActivated(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    handleItemDrillDown(item);
}

void BalanceSheetWidget::triggerDrillDownOnCurrentItem() {
    QTreeWidget* focused = nullptr;
    if (m_liabilitiesTree && m_liabilitiesTree->hasFocus()) focused = m_liabilitiesTree;
    else if (m_assetsTree && m_assetsTree->hasFocus()) focused = m_assetsTree;
    else if (m_liabilitiesTree && m_liabilitiesTree->currentItem()) focused = m_liabilitiesTree;
    else if (m_assetsTree && m_assetsTree->currentItem()) focused = m_assetsTree;

    if (focused && focused->currentItem()) {
        handleItemDrillDown(focused->currentItem());
    }
}

void BalanceSheetWidget::handleItemDrillDown(QTreeWidgetItem* item) {
    if (!item) return;

    QString itemType = item->data(0, Qt::UserRole).toString();
    QString partyName = item->data(0, Qt::UserRole + 1).toString().trimmed();
    QString groupName = item->data(0, Qt::UserRole + 3).toString().trimmed();

    if (partyName.isEmpty()) {
        partyName = item->text(0).trimmed();
    }
    while (partyName.startsWith('-') || partyName.startsWith(QChar(0x2022)) || partyName.startsWith(' ')) {
        partyName = partyName.mid(1).trimmed();
    }

    if (itemType == "GROUP") {
        if (item->childCount() > 0) {
            item->setExpanded(!item->isExpanded());
            return;
        }
        if (partyName.contains("Stock", Qt::CaseInsensitive) || groupName.contains("Stock", Qt::CaseInsensitive)) {
            emit openStockRegisterRequested("");
            return;
        }
        return;
    }

    // 1. Stock item or stock valuation drill-down
    if (groupName.contains("Stock", Qt::CaseInsensitive) ||
        partyName.contains("Stock", Qt::CaseInsensitive) ||
        partyName.contains("Inventory", Qt::CaseInsensitive) ||
        partyName.contains("Paddy", Qt::CaseInsensitive) ||
        partyName.contains("Rice", Qt::CaseInsensitive) ||
        partyName.contains("Bran", Qt::CaseInsensitive) ||
        partyName.contains("Husk", Qt::CaseInsensitive) ||
        partyName.contains("Nakku", Qt::CaseInsensitive) ||
        partyName.contains("Bardana", Qt::CaseInsensitive)) {
        emit openStockRegisterRequested(partyName);
        return;
    }

    // 2. Ignore non-party summary calculated rows (e.g. Net Profit / Net Loss)
    if (itemType == "CALCULATED" && (partyName.contains("Profit", Qt::CaseInsensitive) || partyName.contains("Loss", Qt::CaseInsensitive))) {
        return;
    }

    // 3. Ledger / Party Statement drill-down
    if (!partyName.isEmpty()) {
        FiscalYearInfo fy = FiscalYearHelper::getActiveFiscalYear();
        emit openPartyStatement(partyName, fy.startDate, m_controller->asOnDate());
    }
}

bool BalanceSheetWidget::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* kEvent = static_cast<QKeyEvent*>(event);
        if (kEvent->key() == Qt::Key_Return || kEvent->key() == Qt::Key_Enter) {
            QTreeWidget* tree = qobject_cast<QTreeWidget*>(watched);
            if (tree && tree->currentItem()) {
                handleItemDrillDown(tree->currentItem());
                return true;
            }
        } else if (kEvent->key() == Qt::Key_Right && watched == m_liabilitiesTree) {
            if (m_assetsTree) {
                m_assetsTree->setFocus(Qt::OtherFocusReason);
                if (!m_assetsTree->currentItem() && m_assetsTree->topLevelItemCount() > 0) {
                    m_assetsTree->setCurrentItem(m_assetsTree->topLevelItem(0));
                }
                return true;
            }
        } else if (kEvent->key() == Qt::Key_Left && watched == m_assetsTree) {
            if (m_liabilitiesTree) {
                m_liabilitiesTree->setFocus(Qt::OtherFocusReason);
                if (!m_liabilitiesTree->currentItem() && m_liabilitiesTree->topLevelItemCount() > 0) {
                    m_liabilitiesTree->setCurrentItem(m_liabilitiesTree->topLevelItem(0));
                }
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void BalanceSheetWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        event->accept();
        emit backRequested();
        return;
    }

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QTreeWidget* focusedTree = nullptr;
        if (m_liabilitiesTree->hasFocus()) focusedTree = m_liabilitiesTree;
        else if (m_assetsTree->hasFocus()) focusedTree = m_assetsTree;
        else if (m_liabilitiesTree->currentItem()) focusedTree = m_liabilitiesTree;
        else if (m_assetsTree->currentItem()) focusedTree = m_assetsTree;

        if (focusedTree && focusedTree->currentItem()) {
            event->accept();
            handleItemDrillDown(focusedTree->currentItem());
            return;
        }
    }

    if (event->key() == Qt::Key_F2) {
        event->accept();
        emit requestAccountingPeriodDialog();
        return;
    }

    if (event->key() == Qt::Key_F5) {
        event->accept();
        expandAllGroups();
        return;
    }

    if (event->key() == Qt::Key_F6) {
        event->accept();
        collapseAllGroups();
        return;
    }

    if (event->matches(QKeySequence::Print) || (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_P)) {
        event->accept();
        printReport();
        return;
    }

    if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_E) {
        event->accept();
        exportPdf();
        return;
    }

    // Instantaneous Arrow Key Navigation
    if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down) {
        if (!m_liabilitiesTree->hasFocus() && !m_assetsTree->hasFocus()) {
            m_liabilitiesTree->setFocus(Qt::OtherFocusReason);
            if (m_liabilitiesTree->topLevelItemCount() > 0 && !m_liabilitiesTree->currentItem()) {
                m_liabilitiesTree->setCurrentItem(m_liabilitiesTree->topLevelItem(0));
            }
        }
    }

    // Switch between Liabilities (Left) and Assets (Right) columns
    if (event->key() == Qt::Key_Left) {
        if (m_liabilitiesTree) {
            event->accept();
            m_liabilitiesTree->setFocus(Qt::OtherFocusReason);
            if (!m_liabilitiesTree->currentItem() && m_liabilitiesTree->topLevelItemCount() > 0) {
                m_liabilitiesTree->setCurrentItem(m_liabilitiesTree->topLevelItem(0));
            }
            return;
        }
    } else if (event->key() == Qt::Key_Right) {
        if (m_assetsTree) {
            event->accept();
            m_assetsTree->setFocus(Qt::OtherFocusReason);
            if (!m_assetsTree->currentItem() && m_assetsTree->topLevelItemCount() > 0) {
                m_assetsTree->setCurrentItem(m_assetsTree->topLevelItem(0));
            }
            return;
        }
    }

    QWidget::keyPressEvent(event);
}

void BalanceSheetWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    if (m_controller && (m_controller->data().financialYear != activeFy.name || m_controller->data().asOnDate.isEmpty())) {
        refreshData(activeFy.endDate);
    }
    if (m_liabilitiesTree && m_liabilitiesTree->topLevelItemCount() > 0) {
        m_liabilitiesTree->setFocus(Qt::OtherFocusReason);
        if (!m_liabilitiesTree->currentItem()) {
            m_liabilitiesTree->setCurrentItem(m_liabilitiesTree->topLevelItem(0));
        }
    }
}

void BalanceSheetWidget::paintEvent(QPaintEvent* event) {
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
