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
}

void BalanceSheetWidget::setupUi() {
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
    m_titleLabel = new QLabel("Balance Sheet (Provisional)", subHeaderCard);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("font-size: 15px; font-weight: 800; color: #0F172A; background: transparent;");
    centerTitleBox->addWidget(m_titleLabel);

    m_fyBadge = new QLabel("(From 01/04/2025 To 31/03/2026)", subHeaderCard);
    m_fyBadge->setAlignment(Qt::AlignCenter);
    m_fyBadge->setStyleSheet("font-size: 12px; font-weight: 600; color: #475569; background: transparent;");
    centerTitleBox->addWidget(m_fyBadge);
    subHeaderLayout->addLayout(centerTitleBox, 1);

    // Quick Date & Buttons
    QHBoxLayout* btnBox = new QHBoxLayout();
    btnBox->setSpacing(5);

    m_asOnDateEdit = new AccountingDateDisplay(subHeaderCard);
    m_asOnDateEdit->setFixedWidth(110);
    m_asOnDateEdit->setFixedHeight(30);
    m_asOnDateEdit->setStyleSheet(
        "QLabel { background: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; "
        "padding: 2px 6px; font-size: 12px; font-weight: 700; color: #0F172A; } "
        "QLabel:hover { border: 1.5px solid #2563EB; background: #F8FAFC; }"
    );
    connect(m_asOnDateEdit, &AccountingDateDisplay::dateChanged, this, &BalanceSheetWidget::onDateFilterChanged);
    connect(m_asOnDateEdit, &AccountingDateDisplay::clicked, this, &BalanceSheetWidget::requestAccountingPeriodDialog);
    btnBox->addWidget(m_asOnDateEdit);

    m_periodBtn = new KbdBadgeButton("Period", "F2", QColor("#F8FAFC"), QColor("#F1F5F9"), QColor("#0F172A"), QColor("#CBD5E1"), subHeaderCard);
    m_periodBtn->setFixedHeight(30);
    connect(m_periodBtn, &QPushButton::clicked, this, &BalanceSheetWidget::requestAccountingPeriodDialog);
    btnBox->addWidget(m_periodBtn);

    m_expandBtn = new KbdBadgeButton("Expand", "F5", QColor("#F8FAFC"), QColor("#F1F5F9"), QColor("#0F172A"), QColor("#CBD5E1"), subHeaderCard);
    m_expandBtn->setFixedHeight(30);
    connect(m_expandBtn, &QPushButton::clicked, this, &BalanceSheetWidget::expandAllGroups);
    btnBox->addWidget(m_expandBtn);

    m_collapseBtn = new KbdBadgeButton("Collapse", "F6", QColor("#F8FAFC"), QColor("#F1F5F9"), QColor("#0F172A"), QColor("#CBD5E1"), subHeaderCard);
    m_collapseBtn->setFixedHeight(30);
    connect(m_collapseBtn, &QPushButton::clicked, this, &BalanceSheetWidget::collapseAllGroups);
    btnBox->addWidget(m_collapseBtn);

    m_printBtn = new KbdBadgeButton("Print", "Ctrl+P", QColor("#2563EB"), QColor("#1D4ED8"), QColor("#FFFFFF"), QColor("#2563EB"), subHeaderCard);
    m_printBtn->setFixedHeight(30);
    connect(m_printBtn, &QPushButton::clicked, this, &BalanceSheetWidget::printReport);
    btnBox->addWidget(m_printBtn);

    m_pdfBtn = new KbdBadgeButton("PDF", "Ctrl+E", QColor("#059669"), QColor("#047857"), QColor("#FFFFFF"), QColor("#059669"), subHeaderCard);
    m_pdfBtn->setFixedHeight(30);
    connect(m_pdfBtn, &QPushButton::clicked, this, &BalanceSheetWidget::exportPdf);
    btnBox->addWidget(m_pdfBtn);

    m_backBtn = new KbdBadgeButton("Back", "Esc", QColor("#EF4444"), QColor("#DC2626"), QColor("#FFFFFF"), QColor("#EF4444"), subHeaderCard);
    m_backBtn->setFixedHeight(30);
    connect(m_backBtn, &QPushButton::clicked, this, &BalanceSheetWidget::backRequested);
    btnBox->addWidget(m_backBtn);

    subHeaderLayout->addLayout(btnBox);
    mainLayout->addWidget(subHeaderCard);

    // ==========================================
    // 2. MAIN SPLIT TREE VIEWS (DOUBLE COLUMN T-FORMAT)
    // ==========================================
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setHandleWidth(4);
    m_splitter->setStyleSheet("QSplitter::handle { background-color: #E2E8F0; border-radius: 2px; }");

    // Left Side: Liabilities Tree
    QWidget* liabContainer = new QWidget(m_splitter);
    QVBoxLayout* liabLayout = new QVBoxLayout(liabContainer);
    liabLayout->setContentsMargins(0, 0, 2, 0);
    liabLayout->setSpacing(0);

    m_liabilitiesTree = new QTreeWidget(liabContainer);
    m_liabilitiesTree->setHeaderLabels({"Liabilities", "Amount"});
    m_liabilitiesTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_liabilitiesTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_liabilitiesTree->setAlternatingRowColors(true);
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
    assetLayout->setContentsMargins(2, 0, 0, 0);
    assetLayout->setSpacing(0);

    m_assetsTree = new QTreeWidget(assetContainer);
    m_assetsTree->setHeaderLabels({"Assets", "Amount"});
    m_assetsTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_assetsTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_assetsTree->setAlternatingRowColors(true);
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
    // 3. GRAND TOTAL FOOTER
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

    // Left Grand Total
    QHBoxLayout* lTotBox = new QHBoxLayout();
    QLabel* lTotTitle = new QLabel("Grand Total", footerCard);
    lTotTitle->setStyleSheet("font-size: 13.5px; font-weight: 800; color: #DC2626; background: transparent;");
    m_liabilitiesTotalLbl = new QLabel("₹0.00", footerCard);
    m_liabilitiesTotalLbl->setStyleSheet("font-size: 15px; font-weight: 900; color: #0F172A; font-family: 'Segoe UI', -apple-system, sans-serif; background: transparent;");
    lTotBox->addWidget(lTotTitle);
    lTotBox->addStretch();
    lTotBox->addWidget(m_liabilitiesTotalLbl);
    footerLayout->addLayout(lTotBox, 1);

    // Right Grand Total
    QHBoxLayout* rTotBox = new QHBoxLayout();
    QLabel* rTotTitle = new QLabel("Grand Total", footerCard);
    rTotTitle->setStyleSheet("font-size: 13.5px; font-weight: 800; color: #16A34A; background: transparent;");
    m_assetsTotalLbl = new QLabel("₹0.00", footerCard);
    m_assetsTotalLbl->setStyleSheet("font-size: 15px; font-weight: 900; color: #0F172A; font-family: 'Segoe UI', -apple-system, sans-serif; background: transparent;");
    rTotBox->addWidget(rTotTitle);
    rTotBox->addStretch();
    rTotBox->addWidget(m_assetsTotalLbl);
    footerLayout->addLayout(rTotBox, 1);

    mainLayout->addWidget(footerCard);

    // ==========================================
    // 4. BOTTOM COMMAND KEY STRIP
    // ==========================================
    m_keyLegendLbl = new QLabel("Ctrl+Alt+P: Profit & Loss  •  Alt+M: Ledger Alteration  •  F2: Period  •  Ctrl+P: Print  •  Esc: Back", this);
    m_keyLegendLbl->setFixedHeight(26);
    m_keyLegendLbl->setAlignment(Qt::AlignCenter);
    m_keyLegendLbl->setStyleSheet(
        "background-color: #1E293B; color: #94A3B8; font-size: 11.5px; font-weight: 600; "
        "padding: 2px; border-radius: 6px; letter-spacing: 0.3px; border: none;"
    );
    mainLayout->addWidget(m_keyLegendLbl);
}

void BalanceSheetWidget::applyCustomStyles() {
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
    groupFont.setUnderline(false);

    QFont itemFont;
    itemFont.setPixelSize(12);
    itemFont.setBold(false);

    QFont amtFont;
    amtFont.setBold(true);
    amtFont.setPixelSize(13);

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
            cItem->setText(0, "   " + c.name);
            cItem->setText(1, c.amountFmt);
            cItem->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
            cItem->setFont(0, itemFont);
            cItem->setFont(1, amtFont);
            cItem->setForeground(0, QBrush(QColor("#334155")));
            cItem->setForeground(1, QBrush(QColor("#0F172A")));
            cItem->setData(0, Qt::UserRole, c.isCalculated ? "CALCULATED" : "PARTY");
            cItem->setData(0, Qt::UserRole + 1, c.name);
            cItem->setData(0, Qt::UserRole + 2, c.partyId);
            cItem->setData(0, Qt::UserRole + 3, g.name);

            if (c.isCalculated || c.name.contains("Profit", Qt::CaseInsensitive)) {
                cItem->setForeground(0, QBrush(QColor("#16A34A"))); // Green for Net Profit
                cItem->setForeground(1, QBrush(QColor("#16A34A")));
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
            cItem->setText(0, "   " + c.name);
            cItem->setText(1, c.amountFmt);
            cItem->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
            cItem->setFont(0, itemFont);
            cItem->setFont(1, amtFont);
            cItem->setForeground(0, QBrush(QColor("#334155")));
            cItem->setForeground(1, QBrush(QColor("#0F172A")));
            cItem->setData(0, Qt::UserRole, c.isCalculated ? "CALCULATED" : "PARTY");
            cItem->setData(0, Qt::UserRole + 1, c.name);
            cItem->setData(0, Qt::UserRole + 2, c.partyId);
            cItem->setData(0, Qt::UserRole + 3, g.name);

            if (c.name.contains("Stock", Qt::CaseInsensitive) || c.isCalculated) {
                cItem->setForeground(0, QBrush(QColor("#2563EB")));
                cItem->setForeground(1, QBrush(QColor("#2563EB")));
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

    if (m_liabilitiesTree->topLevelItemCount() > 0 && !m_liabilitiesTree->currentItem() && !m_assetsTree->currentItem()) {
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
        if (!m_liabilitiesTree->hasFocus()) {
            event->accept();
            m_lastAssetIndex = m_assetsTree->indexOfTopLevelItem(m_assetsTree->currentItem());
            m_liabilitiesTree->setFocus(Qt::OtherFocusReason);
            if (m_liabilitiesTree->currentItem()) {
                m_liabilitiesTree->setCurrentItem(m_liabilitiesTree->currentItem());
            } else if (m_liabilitiesTree->topLevelItemCount() > 0) {
                int idx = qBound(0, m_lastLiabIndex, m_liabilitiesTree->topLevelItemCount() - 1);
                m_liabilitiesTree->setCurrentItem(m_liabilitiesTree->topLevelItem(idx));
            }
            return;
        }
    } else if (event->key() == Qt::Key_Right) {
        if (!m_assetsTree->hasFocus()) {
            event->accept();
            m_lastLiabIndex = m_liabilitiesTree->indexOfTopLevelItem(m_liabilitiesTree->currentItem());
            m_assetsTree->setFocus(Qt::OtherFocusReason);
            if (m_assetsTree->currentItem()) {
                m_assetsTree->setCurrentItem(m_assetsTree->currentItem());
            } else if (m_assetsTree->topLevelItemCount() > 0) {
                int idx = qBound(0, m_lastAssetIndex, m_assetsTree->topLevelItemCount() - 1);
                m_assetsTree->setCurrentItem(m_assetsTree->topLevelItem(idx));
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
}

void BalanceSheetWidget::paintEvent(QPaintEvent* event) {
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
