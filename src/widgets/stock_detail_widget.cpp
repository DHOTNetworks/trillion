#include "stock_detail_widget.h"
#include "item_movement_dialog.h"
#include "voucher_date_dialog.h"
#include "kbd_badge_button.h"
#include "custom_dialogs.h"
#include "../engine/fiscal_year_helper.h"
#include "../database_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QShortcut>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

namespace MahadevERP {

StockDetailWidget::StockDetailWidget(StockRegisterController* controller, PrintExportController* printExportCtrl, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_printExportCtrl(printExportCtrl)
{
    setupUi();
    populateItemDropdown();

    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    QDate sDate = QDate::fromString(activeFy.startDate, "yyyy-MM-dd");
    QDate eDate = QDate::fromString(activeFy.endDate, "yyyy-MM-dd");
    if (!sDate.isValid()) sDate = QDate(QDate::currentDate().month() < 4 ? QDate::currentDate().year() - 1 : QDate::currentDate().year(), 4, 1);
    if (!eDate.isValid()) eDate = QDate::currentDate();

    m_fromDateEdit->setDate(sDate);
    m_toDateEdit->setDate(eDate);

    if (m_controller) {
        connect(m_controller, &StockRegisterController::totalsChanged, this, &StockDetailWidget::updateSummaryMetrics);
    }

    reloadData(sDate.toString("yyyy-MM-dd"), eDate.toString("yyyy-MM-dd"));
}

void StockDetailWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 12, 14, 12);
    mainLayout->setSpacing(10);

    // ========================================================================
    // TIER 1: STICKY HEADER CARD
    // ========================================================================
    auto* headerCard = new QFrame(this);
    headerCard->setFixedHeight(50);
    headerCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }"
    );
    auto* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 4, 14, 4);
    headerLayout->setSpacing(10);

    auto* titleCol = new QVBoxLayout();
    titleCol->setSpacing(1);
    m_titleLabel = new QLabel("Stock Register (Show Only Stock)", headerCard);
    m_titleLabel->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
    m_subtitleLabel = new QLabel("Real-time physical inventory ledger with opening, inward, outward, and closing quantities • Press F4 for View Options", headerCard);
    m_subtitleLabel->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    titleCol->addWidget(m_titleLabel);
    titleCol->addWidget(m_subtitleLabel);
    headerLayout->addLayout(titleCol);

    headerLayout->addStretch();

    auto* optBtn = new KbdBadgeButton("Stock Options", "F4", headerCard);
    optBtn->setPrimaryColor("#4338CA", "#3730A3");
    optBtn->setTextColor("#FFFFFF");
    connect(optBtn, &QPushButton::clicked, this, &StockDetailWidget::onOpenOptionsDialog);
    headerLayout->addWidget(optBtn);

    auto* printBtn = new KbdBadgeButton("Print", "Ctrl+P", headerCard);
    printBtn->setPrimaryColor("#059669", "#047857");
    printBtn->setTextColor("#FFFFFF");
    connect(printBtn, &QPushButton::clicked, this, &StockDetailWidget::onPrintRegister);
    headerLayout->addWidget(printBtn);

    auto* pdfBtn = new KbdBadgeButton("Export PDF", "Alt+P", headerCard);
    pdfBtn->setPrimaryColor("#0284C7", "#0369A1");
    pdfBtn->setTextColor("#FFFFFF");
    connect(pdfBtn, &QPushButton::clicked, this, &StockDetailWidget::onExportPdf);
    headerLayout->addWidget(pdfBtn);

    auto* csvBtn = new QPushButton("Export CSV", headerCard);
    csvBtn->setStyleSheet("QPushButton { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 5px 12px; font-weight: 700; font-size: 12px; color: #334155; } QPushButton:hover { background-color: #F8FAFC; }");
    csvBtn->setCursor(Qt::PointingHandCursor);
    connect(csvBtn, &QPushButton::clicked, this, &StockDetailWidget::onExportCsv);
    headerLayout->addWidget(csvBtn);

    auto* backBtn = new KbdBadgeButton("← Back", "Esc", headerCard);
    backBtn->setPrimaryColor("#F1F5F9", "#E2E8F0");
    backBtn->setTextColor("#475569");
    connect(backBtn, &QPushButton::clicked, this, &StockDetailWidget::backRequested);
    headerLayout->addWidget(backBtn);

    mainLayout->addWidget(headerCard);

    // ========================================================================
    // TIER 2: FILTER & CONTROL BAR CARD
    // ========================================================================
    auto* filterCard = new QFrame(this);
    filterCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; }"
        "QLabel { color: #475569; font-weight: 700; font-size: 11.5px; border: none; background: transparent; }"
        "QDateEdit, QLineEdit, QComboBox { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "QDateEdit:focus, QLineEdit:focus, QComboBox:focus { border-color: #2563EB; background-color: #F8FAFC; }"
    );
    auto* filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(12, 6, 12, 6);
    filterLayout->setSpacing(10);

    // Date Range
    filterLayout->addWidget(new QLabel("From Date:", filterCard));
    m_fromDateEdit = new QDateEdit(filterCard);
    m_fromDateEdit->setDisplayFormat("dd-MM-yyyy");
    m_fromDateEdit->setCalendarPopup(true);
    connect(m_fromDateEdit, &QDateEdit::dateChanged, this, &StockDetailWidget::onDateFilterChanged);
    filterLayout->addWidget(m_fromDateEdit);

    filterLayout->addWidget(new QLabel("To Date:", filterCard));
    m_toDateEdit = new QDateEdit(filterCard);
    m_toDateEdit->setDisplayFormat("dd-MM-yyyy");
    m_toDateEdit->setCalendarPopup(true);
    connect(m_toDateEdit, &QDateEdit::dateChanged, this, &StockDetailWidget::onDateFilterChanged);
    filterLayout->addWidget(m_toDateEdit);

    // Mode Selector
    filterLayout->addWidget(new QLabel("View Mode:", filterCard));
    m_modeCombo = new QComboBox(filterCard);
    m_modeCombo->addItem("Show Only Stock", static_cast<int>(StockViewMode::OnlyStock));
    m_modeCombo->addItem("Show Stock With Amount", static_cast<int>(StockViewMode::StockWithAmount));
    m_modeCombo->addItem("Show Item Wise Profit & Loss", static_cast<int>(StockViewMode::ProfitLoss));
    m_modeCombo->addItem("Item Monthly/Daily Stock", static_cast<int>(StockViewMode::MonthlyDaily));
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StockDetailWidget::onModeComboChanged);
    filterLayout->addWidget(m_modeCombo);

    // Grouping Selector
    m_groupingCombo = new QComboBox(filterCard);
    m_groupingCombo->addItem("Item Wise", static_cast<int>(StockGrouping::ItemWise));
    m_groupingCombo->addItem("Group Wise", static_cast<int>(StockGrouping::GroupWise));
    m_groupingCombo->addItem("Company Wise", static_cast<int>(StockGrouping::CompanyWise));
    m_groupingCombo->addItem("Total Stock", static_cast<int>(StockGrouping::TotalSummary));
    m_groupingCombo->addItem("HSN Wise", static_cast<int>(StockGrouping::HsnWise));
    connect(m_groupingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StockDetailWidget::onGroupingComboChanged);
    filterLayout->addWidget(m_groupingCombo);

    // Item Selector (for Monthly/Daily)
    m_itemSelectorCombo = new QComboBox(filterCard);
    m_itemSelectorCombo->setMinimumWidth(180);
    connect(m_itemSelectorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StockDetailWidget::onItemComboChanged);
    m_itemSelectorCombo->setVisible(false);
    filterLayout->addWidget(m_itemSelectorCombo);

    // Timeline Selector (for Monthly/Daily)
    m_timelineCombo = new QComboBox(filterCard);
    m_timelineCombo->addItem("Monthly Summary", false);
    m_timelineCombo->addItem("Daily Detail Movement", true);
    connect(m_timelineCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StockDetailWidget::onTimelineComboChanged);
    m_timelineCombo->setVisible(false);
    filterLayout->addWidget(m_timelineCombo);

    // Search Input
    filterLayout->addWidget(new QLabel("Search:", filterCard));
    m_searchEdit = new QLineEdit(filterCard);
    m_searchEdit->setPlaceholderText("Search commodity, group, code, HSN...");
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &StockDetailWidget::onSearchChanged);
    filterLayout->addWidget(m_searchEdit, 1);

    mainLayout->addWidget(filterCard);

    // ========================================================================
    // TIER 3: HIGH-PERFORMANCE DATA SURFACE (TABLE)
    // ========================================================================
    m_table = new QTableWidget(this);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(
        "QTableWidget {"
        "  background-color: #FFFFFF;"
        "  alternate-background-color: #F8FAFC;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 8px;"
        "  gridline-color: #E2E8F0;"
        "  font-size: 12px;"
        "  color: #0F172A;"
        "}"
        "QTableWidget::item { padding: 6px 8px; }"
        "QTableWidget::item:selected { background-color: #EFF6FF; color: #1E3A8A; font-weight: bold; }"
        "QHeaderView::section {"
        "  background-color: #0F172A;"
        "  color: #FFFFFF;"
        "  font-weight: 800;"
        "  font-size: 11px;"
        "  padding: 8px 6px;"
        "  border: 1px solid #1E293B;"
        "}"
    );
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &StockDetailWidget::onTableDoubleClicked);
    mainLayout->addWidget(m_table, 1);

    // ========================================================================
    // TIER 4: SUMMARY METRICS FOOTER CARDS
    // ========================================================================
    auto* metricsLayout = new QHBoxLayout();
    metricsLayout->setSpacing(10);

    auto createMetricCard = [this, metricsLayout](QFrame*& cardOut, QLabel*& tLabelOut, QLabel*& vLabelOut, const QString& title, const QString& initVal, const QString& color) {
        cardOut = new QFrame(this);
        cardOut->setStyleSheet(
            "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; border-left: 4px solid " + color + "; }"
            "QLabel { border: none; background: transparent; }"
        );
        auto* cLayout = new QVBoxLayout(cardOut);
        cLayout->setContentsMargins(10, 6, 10, 6);
        cLayout->setSpacing(2);

        tLabelOut = new QLabel(title, cardOut);
        tLabelOut->setStyleSheet("font-size: 9.5px; font-weight: 800; color: #64748B; letter-spacing: 0.5px; border: none; background: transparent;");
        vLabelOut = new QLabel(initVal, cardOut);
        vLabelOut->setStyleSheet("font-size: 14px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
        cLayout->addWidget(tLabelOut);
        cLayout->addWidget(vLabelOut);
        metricsLayout->addWidget(cardOut);
    };

    createMetricCard(m_card1, m_card1Title, m_card1Val, "TOTAL COMMODITIES / ENTRIES", "0", "#2563EB");
    createMetricCard(m_card2, m_card2Title, m_card2Val, "TOTAL INWARD QUANTITY", "0.00 Qtl", "#0284C7");
    createMetricCard(m_card3, m_card3Title, m_card3Val, "TOTAL OUTWARD QUANTITY", "0.00 Qtl", "#D97706");
    createMetricCard(m_card4, m_card4Title, m_card4Val, "TOTAL CLOSING STOCK", "0.00 Qtl", "#16A34A");

    mainLayout->addLayout(metricsLayout);

    // Shortcuts
    connect(new QShortcut(QKeySequence(Qt::Key_Escape), this), &QShortcut::activated, this, &StockDetailWidget::backRequested);
    connect(new QShortcut(QKeySequence(Qt::Key_F4), this), &QShortcut::activated, this, &StockDetailWidget::onOpenOptionsDialog);
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_P), this, SLOT(onPrintRegister()));
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_P), this, SLOT(onExportPdf()));

    configureTableColumns();
}

void StockDetailWidget::populateItemDropdown()
{
    int currentId = m_itemSelectorCombo->currentData().toInt();
    if (currentId <= 0 && m_controller) {
        currentId = m_controller->selectedItemId();
    }

    m_itemSelectorCombo->blockSignals(true);
    m_itemSelectorCombo->clear();
    DatabaseManager &db = DatabaseManager::instance();
    QVariantList items = db.executeQuery("SELECT id, name, code FROM stock_items ORDER BY name COLLATE NOCASE ASC;");
    int selectIdx = 0;
    for (int i = 0; i < items.size(); ++i) {
        QVariantMap itm = items[i].toMap();
        int id = itm.value("id").toInt();
        QString name = itm.value("name").toString();
        QString code = itm.value("code").toString();
        QString label = code.isEmpty() ? name : QString("%1 (%2)").arg(name, code);
        m_itemSelectorCombo->addItem(label, id);
        if (id == currentId) {
            selectIdx = i;
        }
    }
    if (m_itemSelectorCombo->count() > 0) {
        m_itemSelectorCombo->setCurrentIndex(selectIdx);
        if (m_controller) {
            m_controller->setSelectedItemId(m_itemSelectorCombo->currentData().toInt());
        }
    }
    m_itemSelectorCombo->blockSignals(false);
}

void StockDetailWidget::setViewConfiguration(StockViewMode mode, StockGrouping grouping, const QString& title)
{
    m_isUpdatingUi = true;
    m_customTitle = title;

    if (m_itemSelectorCombo->count() == 0) {
        populateItemDropdown();
    }

    if (m_controller) {
        m_controller->setViewMode(mode);
        m_controller->setGrouping(grouping);
        if (mode == StockViewMode::MonthlyDaily && m_itemSelectorCombo->count() > 0) {
            if (m_controller->selectedItemId() <= 0) {
                m_controller->setSelectedItemId(m_itemSelectorCombo->currentData().toInt());
            }
        }
    }

    int modeIdx = m_modeCombo->findData(static_cast<int>(mode));
    if (modeIdx >= 0) m_modeCombo->setCurrentIndex(modeIdx);

    int groupIdx = m_groupingCombo->findData(static_cast<int>(grouping));
    if (groupIdx >= 0) m_groupingCombo->setCurrentIndex(groupIdx);

    bool isMonthly = (mode == StockViewMode::MonthlyDaily);
    m_groupingCombo->setVisible(!isMonthly);
    m_itemSelectorCombo->setVisible(isMonthly);
    m_timelineCombo->setVisible(isMonthly);

    updateHeaderLabels();
    configureTableColumns();
    m_isUpdatingUi = false;

    reloadData(m_fromDateEdit->date().toString("yyyy-MM-dd"), m_toDateEdit->date().toString("yyyy-MM-dd"));
}

void StockDetailWidget::updateHeaderLabels()
{
    StockViewMode mode = m_controller ? m_controller->viewMode() : StockViewMode::OnlyStock;
    StockGrouping grouping = m_controller ? m_controller->grouping() : StockGrouping::ItemWise;

    QString gStr = "Item Wise";
    if (grouping == StockGrouping::GroupWise) gStr = "Group Wise";
    else if (grouping == StockGrouping::CompanyWise) gStr = "Company Wise";
    else if (grouping == StockGrouping::TotalSummary) gStr = "Total Stock Summary";
    else if (grouping == StockGrouping::HsnWise) gStr = "HSN Wise";

    if (!m_customTitle.isEmpty()) {
        m_titleLabel->setText(QString("Stock Register (%1)").arg(m_customTitle));
    } else if (mode == StockViewMode::OnlyStock) {
        m_titleLabel->setText(QString("Stock Register (Show Only Stock - %1)").arg(gStr));
        m_subtitleLabel->setText("Real-time physical inventory ledger with opening, inward, outward, and closing quantities • Press F4 for View Options");
    } else if (mode == StockViewMode::StockWithAmount) {
        m_titleLabel->setText(QString("Stock Register With Amount (%1)").arg(gStr));
        m_subtitleLabel->setText("Comprehensive financial stock register with inward/outward valuations and closing rate • Press F4 for View Options");
    } else if (mode == StockViewMode::ProfitLoss) {
        m_titleLabel->setText(QString("Item Wise Trading Profit & Loss Statement (%1)").arg(gStr));
        m_subtitleLabel->setText("Item-level gross profit margin analysis, sales realizations, and cost of goods sold (COGS) • Press F4 for View Options");
    } else if (mode == StockViewMode::MonthlyDaily) {
        QString itemName = m_itemSelectorCombo->currentText();
        m_titleLabel->setText(QString("Item Monthly/Daily Stock Register • %1").arg(itemName));
        m_subtitleLabel->setText("Chronological inventory movement timeline with running balance day-by-day or month-by-month");
    }
}

void StockDetailWidget::configureTableColumns()
{
    m_table->clear();
    StockViewMode mode = m_controller ? m_controller->viewMode() : StockViewMode::OnlyStock;

    if (mode == StockViewMode::OnlyStock) {
        m_table->setColumnCount(11);
        m_table->setHorizontalHeaderLabels({
            "Particular / Name", "Code", "Type / Group", "Unit",
            "Op Bags", "Op Qtl", "Inward Bags", "Inward Qtl", "Outward Bags", "Outward Qtl", "Closing Qtl"
        });
        m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        for (int i = 1; i < 11; ++i) {
            m_table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
        }
    } else if (mode == StockViewMode::StockWithAmount) {
        m_table->setColumnCount(13);
        m_table->setHorizontalHeaderLabels({
            "Particular / Name", "Code", "Unit",
            "Op Qtl", "Opening Value (₹)", "Inward Qtl", "Inward Value (₹)",
            "Outward Qtl", "Outward Value (₹)", "Close Bags", "Close Qtl", "Valuation Rate (₹)", "Closing Value (₹)"
        });
        m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        for (int i = 1; i < 13; ++i) {
            m_table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
        }
    } else if (mode == StockViewMode::ProfitLoss) {
        m_table->setColumnCount(9);
        m_table->setHorizontalHeaderLabels({
            "Particular / Name", "Code / Group", "Sales Qty (Qtl)", "Sales Realization (₹)",
            "Avg Sale Rate (₹)", "Cost Rate (₹)", "Cost of Goods Sold (₹)", "Gross Profit / (Loss) (₹)", "GP Margin (%)"
        });
        m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        for (int i = 1; i < 9; ++i) {
            m_table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
        }
    } else if (mode == StockViewMode::MonthlyDaily) {
        m_table->setColumnCount(12);
        m_table->setHorizontalHeaderLabels({
            "Period / Date", "Voucher No", "Type / Particulars", "Party Name",
            "In Bags", "Inward Qtl", "Inward Value (₹)", "Out Bags", "Outward Qtl", "Outward Value (₹)", "Balance Stock (Qtl)", "Closing Valuation (₹)"
        });
        m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
        for (int i = 4; i < 12; ++i) {
            m_table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
        }
    }
}

void StockDetailWidget::reloadData(const QString& fromDate, const QString& toDate) {
    if (!fromDate.isEmpty()) m_fromDateEdit->setDate(QDate::fromString(fromDate, "yyyy-MM-dd"));
    if (!toDate.isEmpty()) m_toDateEdit->setDate(QDate::fromString(toDate, "yyyy-MM-dd"));

    if (m_itemSelectorCombo->count() == 0) {
        populateItemDropdown();
    }

    if (m_controller) {
        if (m_controller->viewMode() == StockViewMode::MonthlyDaily && m_controller->selectedItemId() <= 0 && m_itemSelectorCombo->count() > 0) {
            m_controller->setSelectedItemId(m_itemSelectorCombo->currentData().toInt());
        }
        m_controller->reload(m_fromDateEdit->date().toString("yyyy-MM-dd"), m_toDateEdit->date().toString("yyyy-MM-dd"));
    }

    updateHeaderLabels();
    populateTable();
    updateSummaryMetrics();
}

void StockDetailWidget::populateTable() {
    m_table->setRowCount(0);
    if (!m_controller) return;

    auto* mdl = m_controller->model();
    const auto& entries = mdl->entries();
    StockViewMode mode = m_controller->viewMode();

    m_table->setRowCount(entries.size());

    for (int row = 0; row < entries.size(); ++row) {
        const auto& e = entries[row];

        if (mode == StockViewMode::OnlyStock) {
            auto* nameItem = new QTableWidgetItem(e.nameVal);
            nameItem->setFont(QFont("Segoe UI", 9, QFont::Bold));
            nameItem->setForeground(QBrush(QColor("#0F172A")));
            m_table->setItem(row, 0, nameItem);

            m_table->setItem(row, 1, new QTableWidgetItem(e.codeVal));
            m_table->setItem(row, 2, new QTableWidgetItem(e.typeVal));
            m_table->setItem(row, 3, new QTableWidgetItem(e.unitVal));

            auto makeNum = [](const QString& val) {
                auto* itm = new QTableWidgetItem(val);
                itm->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                itm->setForeground(QBrush(QColor("#0F172A")));
                return itm;
            };

            m_table->setItem(row, 4, makeNum(QString::number(e.opBags)));
            m_table->setItem(row, 5, makeNum(e.opQtyVal));
            m_table->setItem(row, 6, makeNum(QString::number(e.inBags)));
            m_table->setItem(row, 7, makeNum(e.inQtyVal));
            m_table->setItem(row, 8, makeNum(QString::number(e.outBags)));
            m_table->setItem(row, 9, makeNum(e.outQtyVal));

            auto* closeItem = makeNum(e.closeQtyVal);
            closeItem->setFont(QFont("Segoe UI", 9, QFont::Bold));
            closeItem->setForeground(QBrush(QColor("#16A34A")));
            m_table->setItem(row, 10, closeItem);

        } else if (mode == StockViewMode::StockWithAmount) {
            auto* nameItem = new QTableWidgetItem(e.nameVal);
            nameItem->setFont(QFont("Segoe UI", 9, QFont::Bold));
            nameItem->setForeground(QBrush(QColor("#0F172A")));
            m_table->setItem(row, 0, nameItem);

            m_table->setItem(row, 1, new QTableWidgetItem(e.codeVal));
            m_table->setItem(row, 2, new QTableWidgetItem(e.unitVal));

            auto makeNum = [](const QString& val) {
                auto* itm = new QTableWidgetItem(val);
                itm->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                itm->setForeground(QBrush(QColor("#0F172A")));
                return itm;
            };

            m_table->setItem(row, 3, makeNum(e.opQtyVal));
            m_table->setItem(row, 4, makeNum(e.opValVal));
            m_table->setItem(row, 5, makeNum(e.inQtyVal));
            m_table->setItem(row, 6, makeNum(e.inValVal));
            m_table->setItem(row, 7, makeNum(e.outQtyVal));
            m_table->setItem(row, 8, makeNum(e.outValVal));
            m_table->setItem(row, 9, makeNum(QString::number(e.closeBags)));

            auto* closeQtyItem = makeNum(e.closeQtyVal);
            closeQtyItem->setFont(QFont("Segoe UI", 9, QFont::Bold));
            m_table->setItem(row, 10, closeQtyItem);

            m_table->setItem(row, 11, makeNum(e.rateVal));

            auto* closeValItem = makeNum(e.closeValVal);
            closeValItem->setFont(QFont("Segoe UI", 9, QFont::Bold));
            closeValItem->setForeground(QBrush(QColor("#16A34A")));
            m_table->setItem(row, 12, closeValItem);

        } else if (mode == StockViewMode::ProfitLoss) {
            auto* nameItem = new QTableWidgetItem(e.nameVal);
            nameItem->setFont(QFont("Segoe UI", 9, QFont::Bold));
            nameItem->setForeground(QBrush(QColor("#0F172A")));
            m_table->setItem(row, 0, nameItem);

            m_table->setItem(row, 1, new QTableWidgetItem(e.codeVal));

            auto makeNum = [](const QString& val) {
                auto* itm = new QTableWidgetItem(val);
                itm->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                itm->setForeground(QBrush(QColor("#0F172A")));
                return itm;
            };

            m_table->setItem(row, 2, makeNum(QString::number(e.salesQty, 'f', 2)));
            m_table->setItem(row, 3, makeNum(e.salesValueVal));
            m_table->setItem(row, 4, makeNum(e.avgSaleRateVal));
            m_table->setItem(row, 5, makeNum(e.costRateVal));
            m_table->setItem(row, 6, makeNum(e.cogsVal));

            auto* gpItem = makeNum(e.grossProfitVal);
            gpItem->setFont(QFont("Segoe UI", 9, QFont::Bold));
            if (e.grossProfit >= 0.0) {
                gpItem->setForeground(QBrush(QColor("#16A34A")));
            } else {
                gpItem->setForeground(QBrush(QColor("#DC2626")));
            }
            m_table->setItem(row, 7, gpItem);

            auto* marginItem = makeNum(e.gpMarginPctVal);
            marginItem->setFont(QFont("Segoe UI", 9, QFont::Bold));
            if (e.grossProfit >= 0.0) {
                marginItem->setForeground(QBrush(QColor("#16A34A")));
            } else {
                marginItem->setForeground(QBrush(QColor("#DC2626")));
            }
            m_table->setItem(row, 8, marginItem);

        } else if (mode == StockViewMode::MonthlyDaily) {
            auto* pItem = new QTableWidgetItem(e.periodDate);
            pItem->setFont(QFont("Segoe UI", 9, QFont::Bold));
            pItem->setForeground(QBrush(QColor("#0F172A")));
            m_table->setItem(row, 0, pItem);

            m_table->setItem(row, 1, new QTableWidgetItem(e.voucherNo));
            m_table->setItem(row, 2, new QTableWidgetItem(e.voucherType));
            m_table->setItem(row, 3, new QTableWidgetItem(e.partyName));

            auto makeNum = [](const QString& val) {
                auto* itm = new QTableWidgetItem(val);
                itm->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                itm->setForeground(QBrush(QColor("#0F172A")));
                return itm;
            };

            m_table->setItem(row, 4, makeNum(QString::number(e.inBags)));
            m_table->setItem(row, 5, makeNum(e.inQtyVal));
            m_table->setItem(row, 6, makeNum(e.inValVal));
            m_table->setItem(row, 7, makeNum(QString::number(e.outBags)));
            m_table->setItem(row, 8, makeNum(e.outQtyVal));
            m_table->setItem(row, 9, makeNum(e.outValVal));

            auto* balItem = makeNum(e.runningBalanceVal);
            balItem->setFont(QFont("Segoe UI", 9, QFont::Bold));
            balItem->setForeground(QBrush(QColor("#16A34A")));
            m_table->setItem(row, 10, balItem);

            m_table->setItem(row, 11, makeNum(e.closeValVal));
        }
    }
}

void StockDetailWidget::updateSummaryMetrics()
{
    if (!m_controller) return;
    StockViewMode mode = m_controller->viewMode();

    if (mode == StockViewMode::OnlyStock) {
        m_card1Title->setText("TOTAL COMMODITIES / ENTRIES");
        m_card1Val->setText(QString("%1 Entries").arg(m_controller->totalItemsCount()));

        m_card2Title->setText("TOTAL INWARD QUANTITY");
        m_card2Val->setText(m_controller->totalInwardQtyFmt());

        m_card3Title->setText("TOTAL OUTWARD QUANTITY");
        m_card3Val->setText(m_controller->totalOutwardQtyFmt());

        m_card4Title->setText("TOTAL CLOSING STOCK");
        m_card4Val->setText(m_controller->totalClosingQtyFmt());

    } else if (mode == StockViewMode::StockWithAmount) {
        m_card1Title->setText("TOTAL ITEMS / ENTRIES");
        m_card1Val->setText(QString("%1 Entries").arg(m_controller->totalItemsCount()));

        m_card2Title->setText("TOTAL INWARD VALUE (₹)");
        m_card2Val->setText(m_controller->totalInwardQtyFmt()); // Or Inward amount

        m_card3Title->setText("TOTAL SALES TURNOVER (₹)");
        m_card3Val->setText(m_controller->totalSalesTurnoverFmt());

        m_card4Title->setText("TOTAL CLOSING VALUATION (₹)");
        m_card4Val->setText(m_controller->totalClosingValFmt());

    } else if (mode == StockViewMode::ProfitLoss) {
        m_card1Title->setText("TOTAL SALES TURNOVER (₹)");
        m_card1Val->setText(m_controller->totalSalesTurnoverFmt());

        m_card2Title->setText("TOTAL COST OF GOODS SOLD (₹)");
        m_card2Val->setText(m_controller->totalCogsFmt());

        m_card3Title->setText("TOTAL GROSS TRADING PROFIT (₹)");
        m_card3Val->setText(m_controller->totalGrossProfitFmt());

        m_card4Title->setText("OVERALL GP MARGIN (%)");
        m_card4Val->setText(m_controller->overallGpMarginFmt());

    } else if (mode == StockViewMode::MonthlyDaily) {
        m_card1Title->setText("TOTAL PERIOD INWARD");
        m_card1Val->setText(m_controller->totalInwardQtyFmt());

        m_card2Title->setText("TOTAL PERIOD OUTWARD");
        m_card2Val->setText(m_controller->totalOutwardQtyFmt());

        m_card3Title->setText("FINAL CLOSING BALANCE");
        m_card3Val->setText(m_controller->totalClosingQtyFmt());

        m_card4Title->setText("FINAL CLOSING VALUATION");
        m_card4Val->setText(m_controller->totalClosingValFmt());
    }
}

void StockDetailWidget::onSearchChanged(const QString& query) {
    if (m_controller) {
        m_controller->setSearchQuery(query);
    }
    populateTable();
}

void StockDetailWidget::onDateFilterChanged() {
    reloadData(m_fromDateEdit->date().toString("yyyy-MM-dd"), m_toDateEdit->date().toString("yyyy-MM-dd"));
}

void StockDetailWidget::onModeComboChanged(int index) {
    if (m_isUpdatingUi) return;
    StockViewMode mode = static_cast<StockViewMode>(m_modeCombo->itemData(index).toInt());
    StockGrouping grouping = m_controller ? m_controller->grouping() : StockGrouping::ItemWise;
    setViewConfiguration(mode, grouping);
}

void StockDetailWidget::onGroupingComboChanged(int index) {
    if (m_isUpdatingUi) return;
    StockGrouping grouping = static_cast<StockGrouping>(m_groupingCombo->itemData(index).toInt());
    StockViewMode mode = m_controller ? m_controller->viewMode() : StockViewMode::OnlyStock;
    setViewConfiguration(mode, grouping);
}

void StockDetailWidget::onItemComboChanged(int index) {
    if (m_isUpdatingUi) return;
    int itmId = m_itemSelectorCombo->itemData(index).toInt();
    if (m_controller) {
        m_controller->setSelectedItemId(itmId);
    }
    updateHeaderLabels();
    reloadData(m_fromDateEdit->date().toString("yyyy-MM-dd"), m_toDateEdit->date().toString("yyyy-MM-dd"));
}

void StockDetailWidget::onTimelineComboChanged(int index) {
    if (m_isUpdatingUi) return;
    bool isDaily = m_timelineCombo->itemData(index).toBool();
    if (m_controller) {
        m_controller->setIsDailyDetail(isDaily);
    }
    reloadData(m_fromDateEdit->date().toString("yyyy-MM-dd"), m_toDateEdit->date().toString("yyyy-MM-dd"));
}

void StockDetailWidget::onOpenOptionsDialog() {
    MahadevERP::MenuTreeManager::instance().executeMenu("stock_register_hub", this);
}

void StockDetailWidget::onTableDoubleClicked(int row, int col) {
    Q_UNUSED(col);
    if (row < 0 || row >= m_table->rowCount()) return;

    QString itemName = m_table->item(row, 0) ? m_table->item(row, 0)->text() : "";
    if (itemName.isEmpty()) return;

    ItemMovementDialog dlg(itemName, m_fromDateEdit->date(), m_toDateEdit->date(), m_printExportCtrl, this);
    dlg.exec();
}

void StockDetailWidget::onPrintRegister() {
    if (m_printExportCtrl) {
        m_printExportCtrl->print_stock_register(
            m_fromDateEdit->date().toString("yyyy-MM-dd"),
            m_toDateEdit->date().toString("yyyy-MM-dd")
        );
    }
}

void StockDetailWidget::onExportPdf() {
    if (m_printExportCtrl) {
        m_printExportCtrl->export_stock_register_pdf(
            m_fromDateEdit->date().toString("yyyy-MM-dd"),
            m_toDateEdit->date().toString("yyyy-MM-dd")
        );
    }
}

void StockDetailWidget::onExportCsv() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Stock Register to CSV", "stock_register.csv", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Export Error", "Could not open file for writing: " + file.errorString());
        return;
    }

    QTextStream out(&file);
    int cols = m_table->columnCount();
    QStringList headerList;
    for (int c = 0; c < cols; ++c) {
        headerList << QString("\"%1\"").arg(m_table->horizontalHeaderItem(c)->text());
    }
    out << headerList.join(",") << "\n";

    for (int r = 0; r < m_table->rowCount(); ++r) {
        QStringList rowList;
        for (int c = 0; c < cols; ++c) {
            auto* itm = m_table->item(r, c);
            rowList << QString("\"%1\"").arg(itm ? itm->text().replace("\"", "\"\"") : "");
        }
        out << rowList.join(",") << "\n";
    }
    file.close();
    QMessageBox::information(this, "Export Complete", "Stock register exported successfully to:\n" + fileName);
}

void StockDetailWidget::focusTable() {
    if (m_table) {
        m_table->setFocus();
        if (m_table->rowCount() > 0) {
            m_table->selectRow(0);
        }
    }
}

void StockDetailWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        return;
    }
    if (event->key() == Qt::Key_F4) {
        onOpenOptionsDialog();
        return;
    }
    if (event->key() == Qt::Key_F2) {
        QDate cur = m_fromDateEdit->date();
        QDate sel = VoucherDateDialog::selectDate(this, cur);
        if (sel.isValid()) {
            m_fromDateEdit->setDate(sel);
        }
        return;
    }
    QWidget::keyPressEvent(event);
}

} // namespace MahadevERP
