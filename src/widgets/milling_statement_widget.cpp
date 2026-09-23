#include "milling_statement_widget.h"
#include "kbd_badge_button.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"
#include "../database_manager.h"
#include <QHeaderView>
#include <QKeyEvent>
#include <QFrame>
#include <QGroupBox>
#include <QFormLayout>

namespace MahadevERP {

MillingStatementWidget::MillingStatementWidget(PrintExportController* printExportCtrl, QWidget* parent)
    : QWidget(parent)
    , m_printExportCtrl(printExportCtrl)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);
    setupUi();
    applyCustomStyles();
}

void MillingStatementWidget::setupUi() {
    setStyleSheet("background-color: #F8FAFC;");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 12, 14, 12);
    mainLayout->setSpacing(10);

    // ================= 1. TOP HEADER BAR CARD =================
    QFrame* headerCard = new QFrame(this);
    headerCard->setFixedHeight(58);
    headerCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    QHBoxLayout* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 6, 14, 6);
    headerLayout->setSpacing(10);

    // Icon Badge
    QLabel* iconBox = new QLabel("MILL", headerCard);
    iconBox->setFixedSize(42, 36);
    iconBox->setAlignment(Qt::AlignCenter);
    iconBox->setStyleSheet("background-color: #FEF3C7; border: 1.5px solid #F59E0B; border-radius: 8px; color: #B45309; font-size: 12px; font-weight: 800;");
    headerLayout->addWidget(iconBox);

    QVBoxLayout* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(1);
    QLabel* titleLabel = new QLabel("Paddy Milling & Out-turn Statement Register", headerCard);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif; border: none; background: transparent;");
    QLabel* subtitleLabel = new QLabel("Batch-wise paddy consumption, head rice recoveries, by-products (broken/bran/husk), and milling yield analysis.", headerCard);
    subtitleLabel->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);
    headerLayout->addLayout(titleLayout);

    headerLayout->addStretch(1);

    m_newBatchBtn = new KbdBadgeButton("New Batch", "F2", QColor("#059669"), QColor("#047857"), QColor("#FFFFFF"), QColor("#059669"), headerCard);
    connect(m_newBatchBtn, &QPushButton::clicked, this, &MillingStatementWidget::onNewBatchClicked);
    headerLayout->addWidget(m_newBatchBtn);

    m_refreshBtn = new KbdBadgeButton("Refresh Register", "F5", QColor("#2563EB"), QColor("#1D4ED8"), QColor("#FFFFFF"), QColor("#2563EB"), headerCard);
    connect(m_refreshBtn, &QPushButton::clicked, this, &MillingStatementWidget::onRefreshClicked);
    headerLayout->addWidget(m_refreshBtn);

    m_backBtn = new KbdBadgeButton("Back to Dashboard", "Esc", QColor("#EF4444"), QColor("#DC2626"), QColor("#FFFFFF"), QColor("#EF4444"), headerCard);
    connect(m_backBtn, &QPushButton::clicked, this, &MillingStatementWidget::backRequested);
    headerLayout->addWidget(m_backBtn);

    mainLayout->addWidget(headerCard);

    // ================= 2. FILTER & CONTROL BAR CARD =================
    QFrame* filterCard = new QFrame(this);
    filterCard->setFixedHeight(54);
    filterCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    QHBoxLayout* filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(12, 6, 12, 6);
    filterLayout->setSpacing(10);

    QLabel* fromLbl = new QLabel("From Date:", filterCard);
    fromLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155; border: none; background: transparent;");
    filterLayout->addWidget(fromLbl);

    m_fromDateEdit = new QDateEdit(filterCard);
    m_fromDateEdit->setCalendarPopup(true);
    m_fromDateEdit->setDisplayFormat("dd-MM-yyyy");
    m_fromDateEdit->setFixedHeight(34);
    m_fromDateEdit->setMinimumWidth(120);
    filterLayout->addWidget(m_fromDateEdit);

    QLabel* toLbl = new QLabel("To Date:", filterCard);
    toLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155; border: none; background: transparent;");
    filterLayout->addWidget(toLbl);

    m_toDateEdit = new QDateEdit(filterCard);
    m_toDateEdit->setCalendarPopup(true);
    m_toDateEdit->setDisplayFormat("dd-MM-yyyy");
    m_toDateEdit->setFixedHeight(34);
    m_toDateEdit->setMinimumWidth(120);
    filterLayout->addWidget(m_toDateEdit);

    QLabel* varLbl = new QLabel("Variety:", filterCard);
    varLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155; border: none; background: transparent;");
    filterLayout->addWidget(varLbl);

    m_varietyCombo = new QComboBox(filterCard);
    m_varietyCombo->setFixedHeight(34);
    m_varietyCombo->setMinimumWidth(160);
    m_varietyCombo->addItem("All Varieties");
    QVariantList stockItems = DatabaseManager::instance().executeQuery(
        "SELECT DISTINCT name FROM stock_items WHERE name != '' ORDER BY name ASC;"
    );
    for (const QVariant& row : stockItems) {
        QString vName = row.toMap().value("name").toString();
        if (!vName.isEmpty()) {
            m_varietyCombo->addItem(vName);
        }
    }
    connect(m_varietyCombo, &QComboBox::currentTextChanged, this, &MillingStatementWidget::onRefreshClicked);
    filterLayout->addWidget(m_varietyCombo);

    m_searchBox = new QLineEdit(filterCard);
    m_searchBox->setPlaceholderText("Search Batch No, Variety, Remarks...");
    m_searchBox->setFixedHeight(34);
    m_searchBox->setMinimumWidth(220);
    connect(m_searchBox, &QLineEdit::textChanged, this, &MillingStatementWidget::onRefreshClicked);
    filterLayout->addWidget(m_searchBox);

    filterLayout->addStretch(1);

    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    QLabel* fyBadge = new QLabel(activeFy.name + " (Active)", filterCard);
    fyBadge->setAlignment(Qt::AlignCenter);
    fyBadge->setFixedHeight(34);
    fyBadge->setStyleSheet("background-color: #F0FDF4; color: #16A34A; border: 1.5px solid #86EFAC; border-radius: 6px; padding: 0px 14px; font-weight: 800; font-size: 12px;");
    filterLayout->addWidget(fyBadge);

    mainLayout->addWidget(filterCard);

    // ================= 3. SUMMARY KPI CARDS =================
    QFrame* kpiCard = new QFrame(this);
    kpiCard->setFixedHeight(50);
    kpiCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    QHBoxLayout* kpiLayout = new QHBoxLayout(kpiCard);
    kpiLayout->setContentsMargins(14, 4, 14, 4);
    kpiLayout->setSpacing(16);

    m_totalBatchesLabel = new QLabel("Total Batches: 0", kpiCard);
    m_totalBatchesLabel->setStyleSheet("font-weight: 800; color: #0F172A; font-size: 12px;");
    kpiLayout->addWidget(m_totalBatchesLabel);

    m_totalPaddyInputLabel = new QLabel("Paddy Milled: 0.00 Qtl", kpiCard);
    m_totalPaddyInputLabel->setStyleSheet("font-weight: 700; color: #B45309; font-size: 12px;");
    kpiLayout->addWidget(m_totalPaddyInputLabel);

    m_totalHeadRiceLabel = new QLabel("Head Rice: 0.00 Qtl", kpiCard);
    m_totalHeadRiceLabel->setStyleSheet("font-weight: 700; color: #16A34A; font-size: 12px;");
    kpiLayout->addWidget(m_totalHeadRiceLabel);

    m_avgYieldLabel = new QLabel("Avg Yield: 0.00%", kpiCard);
    m_avgYieldLabel->setStyleSheet("font-weight: 800; color: #2563EB; font-size: 12px;");
    kpiLayout->addWidget(m_avgYieldLabel);

    kpiLayout->addStretch(1);

    m_totalByProductsLabel = new QLabel("Bran & Husk: 0.00 Qtl", kpiCard);
    m_totalByProductsLabel->setStyleSheet("font-weight: 700; color: #64748B; font-size: 12px;");
    kpiLayout->addWidget(m_totalByProductsLabel);

    mainLayout->addWidget(kpiCard);

    // ================= 4. SPLIT TABLES (BATCHES & DETAILS) =================
    auto* splitter = new QSplitter(Qt::Vertical, this);
    splitter->setChildrenCollapsible(false);

    // --- Top Panel: All Batches Table ---
    auto* batchWidget = new QWidget(splitter);
    auto* batchLayout = new QVBoxLayout(batchWidget);
    batchLayout->setContentsMargins(0, 0, 0, 0);

    m_batchTable = new QTableWidget(batchWidget);
    m_batchTable->setColumnCount(11);
    m_batchTable->setHorizontalHeaderLabels({
        "ID", "Batch No", "Date", "Paddy Variety", "Input (Qtl)",
        "Head Rice (Qtl)", "Broken (Qtl)", "Bran (Qtl)", "Husk (Qtl)", "Yield %", "Benchmark Status"
    });
    m_batchTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_batchTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_batchTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_batchTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_batchTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_batchTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_batchTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_batchTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    m_batchTable->horizontalHeader()->setSectionResizeMode(8, QHeaderView::ResizeToContents);
    m_batchTable->horizontalHeader()->setSectionResizeMode(9, QHeaderView::ResizeToContents);
    m_batchTable->horizontalHeader()->setSectionResizeMode(10, QHeaderView::ResizeToContents);
    m_batchTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_batchTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_batchTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_batchTable->setAlternatingRowColors(true);
    m_batchTable->verticalHeader()->setVisible(false);
    m_batchTable->verticalHeader()->setDefaultSectionSize(28);
    connect(m_batchTable, &QTableWidget::itemSelectionChanged, this, &MillingStatementWidget::onBatchSelectionChanged);
    batchLayout->addWidget(m_batchTable);
    splitter->addWidget(batchWidget);

    // --- Bottom Panel: Batch Item Breakdown & Out-turn Card ---
    auto* detailWidget = new QWidget(splitter);
    auto* detailLayout = new QVBoxLayout(detailWidget);
    detailLayout->setContentsMargins(0, 0, 0, 0);

    QFrame* detailHeaderCard = new QFrame(detailWidget);
    detailHeaderCard->setFixedHeight(40);
    detailHeaderCard->setStyleSheet("background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px;");
    QHBoxLayout* detailHeaderLayout = new QHBoxLayout(detailHeaderCard);
    detailHeaderLayout->setContentsMargins(12, 2, 12, 2);
    detailHeaderLayout->setSpacing(14);

    m_detailBatchNoLabel = new QLabel("Selected Batch: None", detailHeaderCard);
    m_detailBatchNoLabel->setStyleSheet("font-weight: 800; color: #0F172A; font-size: 12px;");
    detailHeaderLayout->addWidget(m_detailBatchNoLabel);

    m_detailDateLabel = new QLabel("", detailHeaderCard);
    m_detailDateLabel->setStyleSheet("font-weight: 600; color: #475569; font-size: 11px;");
    detailHeaderLayout->addWidget(m_detailDateLabel);

    m_detailVarietyLabel = new QLabel("", detailHeaderCard);
    m_detailVarietyLabel->setStyleSheet("font-weight: 600; color: #2563EB; font-size: 11px;");
    detailHeaderLayout->addWidget(m_detailVarietyLabel);

    detailHeaderLayout->addStretch(1);

    m_detailYieldLabel = new QLabel("Out-turn: 0.00%", detailHeaderCard);
    m_detailYieldLabel->setStyleSheet("font-weight: 800; color: #16A34A; font-size: 12px;");
    detailHeaderLayout->addWidget(m_detailYieldLabel);

    m_detailVarianceLabel = new QLabel("Variance vs Std (67%): 0.00%", detailHeaderCard);
    m_detailVarianceLabel->setStyleSheet("font-weight: 700; color: #64748B; font-size: 11px;");
    detailHeaderLayout->addWidget(m_detailVarianceLabel);

    detailLayout->addWidget(detailHeaderCard);

    m_detailTable = new QTableWidget(detailWidget);
    m_detailTable->setColumnCount(8);
    m_detailTable->setHorizontalHeaderLabels({
        "Dr / Cr", "Item Type", "Item Name", "Yield %", "Bags", "Weight (Qtl)", "Rate (₹/Qtl)", "Amount (₹)"
    });
    m_detailTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_detailTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_detailTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_detailTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_detailTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_detailTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_detailTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_detailTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    m_detailTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_detailTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_detailTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_detailTable->setAlternatingRowColors(true);
    m_detailTable->verticalHeader()->setVisible(false);
    m_detailTable->verticalHeader()->setDefaultSectionSize(26);
    detailLayout->addWidget(m_detailTable);

    splitter->addWidget(detailWidget);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    mainLayout->addWidget(splitter, 1);

    // Initialize with active financial year dates
    QDate sDate = QDate::fromString(activeFy.startDate, "yyyy-MM-dd");
    QDate eDate = QDate::fromString(activeFy.endDate, "yyyy-MM-dd");
    if (!sDate.isValid()) sDate = QDate(QDate::currentDate().month() < 4 ? QDate::currentDate().year() - 1 : QDate::currentDate().year(), 4, 1);
    if (!eDate.isValid()) eDate = QDate::currentDate();
    m_fromDateEdit->setDate(sDate);
    m_toDateEdit->setDate(eDate);
}

void MillingStatementWidget::applyCustomStyles() {
    setStyleSheet(
        "MillingStatementWidget { background-color: #F8FAFC; }"
        "QTableWidget {"
        "  background-color: #FFFFFF;"
        "  alternate-background-color: #F8FAFC;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  gridline-color: #E2E8F0;"
        "  font-size: 12px;"
        "  color: #0F172A;"
        "}"
        "QTableWidget::item {"
        "  padding: 4px 8px;"
        "  border-bottom: 1px solid #F1F5F9;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: #2563EB;"
        "  color: #FFFFFF;"
        "  font-weight: 700;"
        "}"
        "QHeaderView::section {"
        "  background-color: #0F172A;"
        "  color: #FFFFFF;"
        "  font-weight: 700;"
        "  font-size: 12px;"
        "  padding: 6px 8px;"
        "  border: none;"
        "  border-right: 1px solid #334155;"
        "}"
        "QDateEdit, QComboBox, QLineEdit {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  border: 1.5px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  padding: 4px 10px;"
        "  font-weight: 700;"
        "  font-size: 12px;"
        "}"
        "QDateEdit:focus, QComboBox:focus, QLineEdit:focus {"
        "  border: 2px solid #2563EB;"
        "  background-color: #FFFFF0;"
        "}"
    );
}

void MillingStatementWidget::loadMillingData(const QDate& fromDate, const QDate& toDate, const QString& varietyFilter) {
    if (m_fromDateEdit->date() != fromDate) m_fromDateEdit->setDate(fromDate);
    if (m_toDateEdit->date() != toDate) m_toDateEdit->setDate(toDate);

    QString fromStr = fromDate.toString("yyyy-MM-dd");
    QString toStr = toDate.toString("yyyy-MM-dd");

    QString sql = QString(
        "SELECT id, batch_no, batch_date, paddy_variety, paddy_input_qtl, "
        "head_rice_qtl, broken_rice_qtl, bran_qtl, husk_qtl, wastage_qtl, yield_pct, narration "
        "FROM milling_batches "
        "WHERE batch_date >= '%1' AND batch_date <= '%2' "
        "ORDER BY batch_date ASC, id ASC;"
    ).arg(fromStr, toStr);

    m_rawBatches = DatabaseManager::instance().executeQuery(sql);
    populateBatchTable();
}

void MillingStatementWidget::populateBatchTable() {
    m_batchTable->setRowCount(0);
    m_detailTable->setRowCount(0);

    double totalPaddy = 0.0;
    double totalHead = 0.0;
    double totalBroken = 0.0;
    double totalBran = 0.0;
    double totalHusk = 0.0;

    QString selectedVar = m_varietyCombo ? m_varietyCombo->currentText() : "All Varieties";
    QString query = m_searchBox ? m_searchBox->text().trimmed().toLower() : "";

    int row = 0;
    for (const auto& var : m_rawBatches) {
        QVariantMap r = var.toMap();
        QString bNo = r.value("batch_no").toString();
        QString bDate = r.value("batch_date").toString();
        QString variety = r.value("paddy_variety").toString();
        QString narr = r.value("narration").toString();
        double pInput = r.value("paddy_input_qtl").toDouble();
        double hRice = r.value("head_rice_qtl").toDouble();
        double bRice = r.value("broken_rice_qtl").toDouble();
        double bran = r.value("bran_qtl").toDouble();
        double husk = r.value("husk_qtl").toDouble();
        double yieldPct = r.value("yield_pct").toDouble();

        if (selectedVar != "All Varieties" && !variety.contains(selectedVar, Qt::CaseInsensitive)) {
            continue;
        }

        if (!query.isEmpty()) {
            if (!bNo.toLower().contains(query) && !variety.toLower().contains(query) && !narr.toLower().contains(query)) {
                continue;
            }
        }

        m_batchTable->insertRow(row);
        m_batchTable->setItem(row, 0, new QTableWidgetItem(r.value("id").toString()));
        m_batchTable->setItem(row, 1, new QTableWidgetItem(bNo));
        m_batchTable->setItem(row, 2, new QTableWidgetItem(bDate));
        m_batchTable->setItem(row, 3, new QTableWidgetItem(variety));

        auto* inItem = new QTableWidgetItem(QString::number(pInput, 'f', 2));
        inItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_batchTable->setItem(row, 4, inItem);

        auto* hrItem = new QTableWidgetItem(QString::number(hRice, 'f', 2));
        hrItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        hrItem->setForeground(QColor("#16A34A"));
        m_batchTable->setItem(row, 5, hrItem);

        auto* brItem = new QTableWidgetItem(QString::number(bRice, 'f', 2));
        brItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_batchTable->setItem(row, 6, brItem);

        auto* branItem = new QTableWidgetItem(QString::number(bran, 'f', 2));
        branItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_batchTable->setItem(row, 7, branItem);

        auto* huskItem = new QTableWidgetItem(QString::number(husk, 'f', 2));
        huskItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_batchTable->setItem(row, 8, huskItem);

        auto* yldItem = new QTableWidgetItem(QString("%1%").arg(QString::number(yieldPct, 'f', 2)));
        yldItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        yldItem->setForeground(yieldPct >= 66.0 ? QColor("#16A34A") : QColor("#D97706"));
        m_batchTable->setItem(row, 9, yldItem);

        QString status = (yieldPct >= 67.0) ? "Optimal (>=67%)" : (yieldPct >= 65.0 ? "Normal" : "Low Recovery (<65%)");
        auto* statusItem = new QTableWidgetItem(status);
        statusItem->setTextAlignment(Qt::AlignCenter);
        statusItem->setForeground(yieldPct >= 66.0 ? QColor("#16A34A") : QColor("#DC2626"));
        m_batchTable->setItem(row, 10, statusItem);

        totalPaddy += pInput;
        totalHead += hRice;
        totalBroken += bRice;
        totalBran += bran;
        totalHusk += husk;
        row++;
    }

    double avgYield = (totalPaddy > 0.0) ? (totalHead / totalPaddy) * 100.0 : 0.0;
    m_totalBatchesLabel->setText(QString("Total Batches: %1").arg(row));
    m_totalPaddyInputLabel->setText(QString("Paddy Milled: %1 Qtl").arg(QString::number(totalPaddy, 'f', 2)));
    m_totalHeadRiceLabel->setText(QString("Head Rice: %1 Qtl").arg(QString::number(totalHead, 'f', 2)));
    m_avgYieldLabel->setText(QString("Avg Yield: %1%").arg(QString::number(avgYield, 'f', 2)));
    m_totalByProductsLabel->setText(QString("Bran & Husk: %1 Qtl").arg(QString::number(totalBran + totalHusk, 'f', 2)));

    if (row > 0) {
        m_batchTable->selectRow(0);
    }
}

void MillingStatementWidget::onBatchSelectionChanged() {
    int curRow = m_batchTable->currentRow();
    if (curRow < 0) return;

    int batchId = m_batchTable->item(curRow, 0)->text().toInt();
    QString bNo = m_batchTable->item(curRow, 1)->text();
    QString bDate = m_batchTable->item(curRow, 2)->text();
    QString varName = m_batchTable->item(curRow, 3)->text();
    QString yldStr = m_batchTable->item(curRow, 9)->text();

    m_detailBatchNoLabel->setText("Selected Batch: " + bNo);
    m_detailDateLabel->setText("Date: " + bDate);
    m_detailVarietyLabel->setText("Variety: " + varName);
    m_detailYieldLabel->setText("Out-turn: " + yldStr);

    double yldVal = yldStr.remove('%').toDouble();
    double variance = yldVal - 67.0;
    QString varStr = QString("Variance vs Std (67%): %1%2%").arg(variance >= 0 ? "+" : "").arg(QString::number(variance, 'f', 2));
    m_detailVarianceLabel->setText(varStr);
    m_detailVarianceLabel->setStyleSheet(QString("font-weight: 700; color: %1; font-size: 11px;").arg(variance >= 0 ? "#16A34A" : "#DC2626"));

    populateDetailCard(batchId);
}

void MillingStatementWidget::populateDetailCard(int batchId) {
    m_detailTable->setRowCount(0);

    QString sql = QString(
        "SELECT row_no, drcr, item_name, percentage, weight_qtl, bags, rate, amount, narration "
        "FROM milling_voucher_items "
        "WHERE batch_id = %1 "
        "ORDER BY drcr DESC, row_no ASC;"
    ).arg(batchId);

    QVariantList items = DatabaseManager::instance().executeQuery(sql);
    int row = 0;
    for (const auto& var : items) {
        QVariantMap r = var.toMap();
        QString drcr = r.value("drcr").toString();
        QString iName = r.value("item_name").toString();
        double pct = r.value("percentage").toDouble();
        double wQtl = r.value("weight_qtl").toDouble();
        int bags = r.value("bags").toInt();
        double rate = r.value("rate").toDouble();
        double amt = r.value("amount").toDouble();

        m_detailTable->insertRow(row);

        auto* sideItem = new QTableWidgetItem(drcr);
        sideItem->setTextAlignment(Qt::AlignCenter);
        sideItem->setForeground(drcr == "Dr" ? QColor("#16A34A") : QColor("#B45309"));
        m_detailTable->setItem(row, 0, sideItem);

        QString typeStr = (drcr == "Dr") ? "Produced (Output)" : "Consumed (Input)";
        m_detailTable->setItem(row, 1, new QTableWidgetItem(typeStr));
        m_detailTable->setItem(row, 2, new QTableWidgetItem(iName));

        auto* pctItem = new QTableWidgetItem(pct > 0.0 ? QString("%1%").arg(QString::number(pct, 'f', 2)) : "-");
        pctItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_detailTable->setItem(row, 3, pctItem);

        auto* bagItem = new QTableWidgetItem(bags > 0 ? QString::number(bags) : "-");
        bagItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_detailTable->setItem(row, 4, bagItem);

        auto* wItem = new QTableWidgetItem(QString::number(wQtl, 'f', 2));
        wItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_detailTable->setItem(row, 5, wItem);

        auto* rateItem = new QTableWidgetItem(rate > 0.0 ? AccountingEngine::formatCurrency(rate) : "-");
        rateItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_detailTable->setItem(row, 6, rateItem);

        auto* amtItem = new QTableWidgetItem(amt > 0.0 ? AccountingEngine::formatCurrency(amt) : "-");
        amtItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_detailTable->setItem(row, 7, amtItem);

        row++;
    }
}

void MillingStatementWidget::onRefreshClicked() {
    loadMillingData(m_fromDateEdit->date(), m_toDateEdit->date(), m_varietyCombo->currentText());
}

void MillingStatementWidget::onNewBatchClicked() {
    emit newBatchRequested();
}

void MillingStatementWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        event->accept();
    } else if (event->key() == Qt::Key_F5) {
        onRefreshClicked();
        event->accept();
    } else if (event->key() == Qt::Key_F2) {
        onNewBatchClicked();
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

} // namespace MahadevERP
