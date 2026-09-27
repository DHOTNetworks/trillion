#include "milling_voucher_widget.h"
#include "item_search_delegate.h"
#include "voucher_date_dialog.h"
#include "kbd_badge_button.h"
#include "custom_dialogs.h"
#include "../database_manager.h"
#include "../engine/fiscal_year_helper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QGroupBox>
#include <QShortcut>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QLineEdit>
#include <QTableWidget>

namespace MahadevERP {

MillingVoucherWidget::MillingVoucherWidget(MillingBatchController* batchCtrl,
                                           MillingModel* millingModel,
                                           PrintExportController* printExportCtrl,
                                           QWidget* parent)
    : QWidget(parent)
    , m_batchCtrl(batchCtrl)
    , m_millingModel(millingModel)
    , m_printExportCtrl(printExportCtrl)
{
    setupUi();
    resetForm();
}

void MillingVoucherWidget::setupUi() {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(
        "MillingVoucherWidget { background-color: #FFF3EA; font-family: -apple-system, BlinkMacSystemFont, 'Helvetica Neue', 'Segoe UI', Arial, sans-serif; }"
        "QLabel { border: none; background: transparent; color: #1E293B; font-size: 11px; font-weight: 700; }"
        "QLineEdit { background-color: #FFFFFF; color: #0F172A; border: 1px solid #CBD5E1; border-radius: 5px; padding: 2px 6px; font-size: 11.5px; font-weight: 700; }"
        "QLineEdit:focus { border: 1.5px solid #7E22CE; background-color: #FAF5FF; color: #0F172A; }"
        "QGroupBox { font-size: 10.5px; font-weight: 800; border-radius: 6px; margin-top: 12px; padding: 6px 8px 4px 8px; }"
        "QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 5px; font-size: 11px; font-weight: 700; color: #475569; padding: 3px 10px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
        "QPushButton:focus { border: 1.5px solid #7E22CE; background-color: #FAF5FF; }"
    );

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(8, 4, 8, 4);
    rootLayout->setSpacing(4);

    // ========================================================================
    // 1. TOP HEADER BAR: Modern Purple Milling Badge + No. + Title + Date & Day
    // ========================================================================
    auto* topHeaderLayout = new QHBoxLayout();
    topHeaderLayout->setContentsMargins(0, 0, 0, 0);
    topHeaderLayout->setSpacing(6);

    auto* badge = new QLabel("Milling / Out-Turn", this);
    badge->setAlignment(Qt::AlignCenter);
    badge->setFixedHeight(22);
    badge->setStyleSheet("background-color: #FAF5FF; color: #7E22CE; font-weight: 900; font-size: 12px; padding: 1px 10px; border: 1px solid #E9D5FF; border-radius: 5px;");
    topHeaderLayout->addWidget(badge);

    auto* batchDisplay = new QLabel("Batch:", this);
    batchDisplay->setStyleSheet("font-size: 12px; font-weight: 800; color: #475569;");
    topHeaderLayout->addWidget(batchDisplay);

    m_batchNoEdit = new QLineEdit(this);
    m_batchNoEdit->setReadOnly(true);
    m_batchNoEdit->setFixedWidth(120);
    m_batchNoEdit->setFixedHeight(22);
    m_batchNoEdit->setStyleSheet("background-color: #FFFFFF; color: #7E22CE; font-size: 12px; font-weight: 900; border: 1px solid #E9D5FF; border-radius: 5px; padding: 1px 6px;");
    topHeaderLayout->addWidget(m_batchNoEdit);

    topHeaderLayout->addStretch(1);

    auto* titleHeaderLabel = new QLabel("F7 : Milling & Rice Out-Turn Production Voucher", this);
    titleHeaderLabel->setAlignment(Qt::AlignCenter);
    titleHeaderLabel->setStyleSheet("font-size: 15px; font-weight: 900; color: #7E22CE; font-family: -apple-system, BlinkMacSystemFont, 'Helvetica Neue', 'Segoe UI', Arial, sans-serif;");
    topHeaderLayout->addWidget(titleHeaderLabel);

    topHeaderLayout->addStretch(1);

    // Date & Day of Week
    auto* dateLabel = new QLabel("Date (F2):", this);
    dateLabel->setStyleSheet("font-size: 11px; font-weight: 800; color: #475569;");
    topHeaderLayout->addWidget(dateLabel);

    m_batchDateEdit = new QLineEdit(this);
    m_batchDateEdit->setReadOnly(true);
    m_batchDateEdit->setFixedHeight(22);
    m_batchDateEdit->setFixedWidth(95);
    m_batchDateEdit->setStyleSheet("background-color: #FFFFFF; color: #0F172A; font-size: 12px; font-weight: 800; border: 1px solid #CBD5E1; border-radius: 5px;");
    topHeaderLayout->addWidget(m_batchDateEdit);

    auto* dateBtn = new QPushButton("📅", this);
    dateBtn->setFixedSize(24, 22);
    dateBtn->setStyleSheet("background-color: #EFF6FF; border: 1px solid #93C5FD; border-radius: 4px; font-size: 11px; color: #1D4ED8; padding: 0px;");
    connect(dateBtn, &QPushButton::clicked, this, [this]() { openDateDialog(false); });
    topHeaderLayout->addWidget(dateBtn);

    m_dayLabel = new QLabel("Wednesday", this);
    m_dayLabel->setStyleSheet("font-size: 11px; font-weight: 700; color: #64748B;");
    topHeaderLayout->addWidget(m_dayLabel);

    auto* backTopBtn = new KbdBadgeButton("Back", "Esc", this);
    backTopBtn->setFixedHeight(22);
    backTopBtn->setPrimaryColor("#64748B", "#475569");
    backTopBtn->setTextColor("#FFFFFF");
    connect(backTopBtn, &QPushButton::clicked, this, &MillingVoucherWidget::backRequested);
    topHeaderLayout->addWidget(backTopBtn);

    rootLayout->addLayout(topHeaderLayout);

    // ========================================================================
    // 2. VOUCHER META CARD (Peach Container)
    // ========================================================================
    auto* metaCard = new QFrame(this);
    metaCard->setStyleSheet("QFrame { background-color: #FFF8F3; border: 1px solid #E2D5C8; border-radius: 6px; }");
    auto* metaLayout = new QHBoxLayout(metaCard);
    metaLayout->setContentsMargins(8, 4, 8, 4);
    metaLayout->setSpacing(8);

    auto* narrLbl = new QLabel("Milling Remarks / Narration:", metaCard);
    narrLbl->setStyleSheet("color: #1E293B; font-weight: 800; font-size: 11px; border: none; background: transparent;");
    metaLayout->addWidget(narrLbl);

    m_narrationEdit = new QLineEdit(metaCard);
    m_narrationEdit->setPlaceholderText("Batch process shift notes, Paddy lot info, Moisture %, Out-turn recovery remarks...");
    m_narrationEdit->setFixedHeight(24);
    m_narrationEdit->setStyleSheet("background-color: #FFFFFF; color: #0F172A; border: 1px solid #CBD5E1; border-radius: 4px; padding: 2px 6px; font-size: 11.5px; font-weight: 600;");
    metaLayout->addWidget(m_narrationEdit, 1);

    rootLayout->addWidget(metaCard);

    // ========================================================================
    // 3. SPLIT TABLE SECTION (Paddy Input VS Rice Output)
    // ========================================================================
    auto* tablesLayout = new QHBoxLayout();
    tablesLayout->setSpacing(8);

    const QString tableStyle =
        "QTableWidget { background-color: #FFFFFF; color: #0F172A; border: 1px solid #CBD5E1; border-radius: 4px; gridline-color: #E2E8F0; font-size: 11.5px; font-weight: 700; }"
        "QTableWidget::item { color: #0F172A; padding: 3px 5px; background-color: #FFFFFF; }"
        "QTableWidget::item:selected { background-color: #EFF6FF; color: #1D4ED8; font-weight: 800; }"
        "QHeaderView::section { background-color: #334155; color: #FFFFFF; font-weight: 800; font-size: 10.5px; padding: 3px; border: none; }";

    // Left: Consumed Paddy
    auto* consumedGroup = new QGroupBox("RAW PADDY INPUT / CONSUMPTION", this);
    consumedGroup->setStyleSheet(
        "QGroupBox { font-size: 10.5px; font-weight: 800; color: #DC2626; border: 1px solid #FECACA; border-radius: 6px; margin-top: 10px; padding: 6px 6px 4px 6px; background-color: #FFF8F8; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 4px; left: 8px; top: 0px; color: #DC2626; font-weight: 800; background-color: #FFF3EA; }"
    );
    auto* cLayout = new QVBoxLayout(consumedGroup);
    cLayout->setContentsMargins(4, 4, 4, 4);
    cLayout->setSpacing(4);

    m_consumedTable = new QTableWidget(consumedGroup);
    m_consumedTable->setColumnCount(4);
    m_consumedTable->setHorizontalHeaderLabels({"Paddy Variety / Item", "Bags", "Weight (Qtl)", "Amount (₹)"});
    m_consumedTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_consumedTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_consumedTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_consumedTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_consumedTable->verticalHeader()->setVisible(false);
    m_consumedTable->setStyleSheet(tableStyle);

    m_consumedItemDelegate = new ItemSearchDelegate(this);
    m_consumedTable->setItemDelegate(m_consumedItemDelegate);
    connect(m_consumedItemDelegate, &ItemSearchDelegate::stockItemConfigured, this, &MillingVoucherWidget::onConsumedItemConfigured);
    connect(m_consumedItemDelegate, &ItemSearchDelegate::moveNextRequested, this, &MillingVoucherWidget::advanceConsumedCell);
    connect(m_consumedItemDelegate, &ItemSearchDelegate::movePrevRequested, this, &MillingVoucherWidget::retreatConsumedCell);

    cLayout->addWidget(m_consumedTable);

    auto* cBtnRow = new QHBoxLayout();
    auto* addCBtn = new QPushButton("+ Add Input Row", consumedGroup);
    addCBtn->setFixedHeight(22);
    addCBtn->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 4px; padding: 2px 8px; font-weight: 800; font-size: 10.5px; color: #DC2626;");
    connect(addCBtn, &QPushButton::clicked, this, &MillingVoucherWidget::onAddConsumedRow);
    cBtnRow->addWidget(addCBtn);

    auto* remCBtn = new QPushButton("- Remove Row", consumedGroup);
    remCBtn->setFixedHeight(22);
    remCBtn->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 4px; padding: 2px 8px; font-weight: 700; font-size: 10.5px; color: #64748B;");
    connect(remCBtn, &QPushButton::clicked, this, &MillingVoucherWidget::onRemoveConsumedRow);
    cBtnRow->addWidget(remCBtn);
    cBtnRow->addStretch();
    cLayout->addLayout(cBtnRow);

    tablesLayout->addWidget(consumedGroup, 1);

    // Right: Produced Rice & By-Products
    auto* producedGroup = new QGroupBox("RICE & BY-PRODUCTS OUTPUT RECOVERY", this);
    producedGroup->setStyleSheet(
        "QGroupBox { font-size: 10.5px; font-weight: 800; color: #16A34A; border: 1px solid #BBF7D0; border-radius: 6px; margin-top: 10px; padding: 6px 6px 4px 6px; background-color: #F8FFF9; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 4px; left: 8px; top: 0px; color: #16A34A; font-weight: 800; background-color: #FFF3EA; }"
    );
    auto* pLayout = new QVBoxLayout(producedGroup);
    pLayout->setContentsMargins(4, 4, 4, 4);
    pLayout->setSpacing(4);

    m_producedTable = new QTableWidget(producedGroup);
    m_producedTable->setColumnCount(5);
    m_producedTable->setHorizontalHeaderLabels({"Output Commodity", "Yield %", "Bags", "Weight (Qtl)", "Amount (₹)"});
    m_producedTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_producedTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_producedTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_producedTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_producedTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_producedTable->verticalHeader()->setVisible(false);
    m_producedTable->setStyleSheet(tableStyle);

    m_producedItemDelegate = new ItemSearchDelegate(this);
    m_producedTable->setItemDelegate(m_producedItemDelegate);
    connect(m_producedItemDelegate, &ItemSearchDelegate::stockItemConfigured, this, &MillingVoucherWidget::onProducedItemConfigured);
    connect(m_producedItemDelegate, &ItemSearchDelegate::moveNextRequested, this, &MillingVoucherWidget::advanceProducedCell);
    connect(m_producedItemDelegate, &ItemSearchDelegate::movePrevRequested, this, &MillingVoucherWidget::retreatProducedCell);

    pLayout->addWidget(m_producedTable);

    auto* pBtnRow = new QHBoxLayout();
    auto* addPBtn = new QPushButton("+ Add Output Row", producedGroup);
    addPBtn->setFixedHeight(22);
    addPBtn->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 4px; padding: 2px 8px; font-weight: 800; font-size: 10.5px; color: #16A34A;");
    connect(addPBtn, &QPushButton::clicked, this, &MillingVoucherWidget::onAddProducedRow);
    pBtnRow->addWidget(addPBtn);

    auto* remPBtn = new QPushButton("- Remove Row", producedGroup);
    remPBtn->setFixedHeight(22);
    remPBtn->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 4px; padding: 2px 8px; font-weight: 700; font-size: 10.5px; color: #64748B;");
    connect(remPBtn, &QPushButton::clicked, this, &MillingVoucherWidget::onRemoveProducedRow);
    pBtnRow->addWidget(remPBtn);
    pBtnRow->addStretch();
    pLayout->addLayout(pBtnRow);

    tablesLayout->addWidget(producedGroup, 1);
    rootLayout->addLayout(tablesLayout, 1);

    // ========================================================================
    // 4. LIVE YIELD BENCHMARKS & METRIC CARDS
    // ========================================================================
    auto* metricsLayout = new QHBoxLayout();
    metricsLayout->setSpacing(8);

    auto createMetric = [this, metricsLayout](const QString& title, const QString& initVal, const QString& color) {
        auto* card = new QFrame(this);
        card->setStyleSheet(
            "QFrame { background-color: #FFFFFF; border: 1px solid #E2D5C8; border-radius: 6px; border-left: 4px solid " + color + "; }"
            "QLabel { border: none; background: transparent; }"
        );
        auto* cLayout = new QVBoxLayout(card);
        cLayout->setContentsMargins(8, 4, 8, 4);
        cLayout->setSpacing(1);

        auto* tLabel = new QLabel(title, card);
        tLabel->setStyleSheet("font-size: 9.5px; font-weight: 800; color: #475569; letter-spacing: 0.5px; border: none; background: transparent;");
        auto* vLabel = new QLabel(initVal, card);
        vLabel->setStyleSheet("font-size: 13.5px; font-weight: 900; color: " + color + "; border: none; background: transparent;");
        cLayout->addWidget(tLabel);
        cLayout->addWidget(vLabel);
        metricsLayout->addWidget(card);
        return vLabel;
    };

    m_totalInputWeightLabel = createMetric("TOTAL PADDY INPUT", "0.00 Qtl", "#DC2626");
    m_totalOutputWeightLabel = createMetric("TOTAL OUTPUT RECOVERED", "0.00 Qtl", "#16A34A");
    m_yieldPctLabel = createMetric("ACTUAL YIELD % (BENCHMARK 67%)", "0.0 %", "#2563EB");
    m_shortageLabel = createMetric("MILLING LOSS / SHORTAGE", "0.00 Qtl (100.0%)", "#7C3AED");

    rootLayout->addLayout(metricsLayout);

    // ========================================================================
    // 5. BOTTOM ACTION BAR
    // ========================================================================
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(6);

    auto* cancelBottomBtn = new KbdBadgeButton("Cancel", "Esc", this);
    cancelBottomBtn->setFixedHeight(28);
    cancelBottomBtn->setPrimaryColor("#F1F5F9", "#E2E8F0");
    cancelBottomBtn->setTextColor("#475569");
    connect(cancelBottomBtn, &QPushButton::clicked, this, &MillingVoucherWidget::backRequested);
    bottomLayout->addWidget(cancelBottomBtn);

    bottomLayout->addStretch();

    auto* saveBtn = new KbdBadgeButton("Save Production Batch", "Ctrl+S", this);
    saveBtn->setFixedHeight(28);
    saveBtn->setPrimaryColor("#16A34A", "#15803D");
    saveBtn->setTextColor("#FFFFFF");
    saveBtn->setMinimumWidth(200);
    connect(saveBtn, &QPushButton::clicked, this, &MillingVoucherWidget::onSaveClicked);
    bottomLayout->addWidget(saveBtn);
    rootLayout->addLayout(bottomLayout);

    connect(m_consumedTable, &QTableWidget::cellChanged, this, &MillingVoucherWidget::onRecalculate);
    connect(m_producedTable, &QTableWidget::cellChanged, this, &MillingVoucherWidget::onRecalculate);

    connect(new QShortcut(QKeySequence(Qt::Key_Escape), this), &QShortcut::activated, this, &MillingVoucherWidget::backRequested);
    connect(new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), this), &QShortcut::activated, this, &MillingVoucherWidget::onSaveClicked);
}

void MillingVoucherWidget::resetForm() {
    m_editingBatchId = 0;
    QDate cur = QDate::currentDate();
    m_batchDateEdit->setText(cur.toString("dd-MM-yyyy"));
    m_dayLabel->setText(cur.toString("dddd"));
    m_narrationEdit->clear();

    if (m_millingModel) {
        m_batchNoEdit->setText(m_millingModel->get_next_batch_no(cur.toString("yyyy-MM-dd")));
    } else {
        m_batchNoEdit->setText("MB-2026-0001");
    }

    m_consumedTable->setRowCount(0);
    m_producedTable->setRowCount(0);

    // Dynamic Consumed Item from database
    onAddConsumedRow();
    QVariant paddyItem = DatabaseManager::instance().executeScalar(
        "SELECT name FROM stock_items WHERE item_type LIKE '%Paddy%' OR trading_group LIKE '%Paddy%' OR name LIKE '%Paddy%' ORDER BY id ASC LIMIT 1;"
    );
    if (paddyItem.isValid() && !paddyItem.isNull()) {
        auto* cItem = m_consumedTable->item(0, 0);
        if (cItem) cItem->setText(paddyItem.toString().trimmed());
    }

    // Dynamic Produced Items from database
    QVariantList prodItems = DatabaseManager::instance().executeQuery(
        "SELECT name FROM stock_items WHERE item_type LIKE '%Rice%' OR trading_group LIKE '%Rice%' OR name LIKE '%Rice%' OR name LIKE '%Bran%' OR name LIKE '%Husk%' ORDER BY id ASC LIMIT 4;"
    );
    if (!prodItems.isEmpty()) {
        for (const auto& pi : prodItems) {
            QString d = pi.toMap().value("name").toString().trimmed();
            if (d.isEmpty()) continue;
            int r = m_producedTable->rowCount();
            m_producedTable->insertRow(r);

            auto* it0 = new QTableWidgetItem(d);
            it0->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            m_producedTable->setItem(r, 0, it0);

            auto* it1 = new QTableWidgetItem("0.0");
            it1->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_producedTable->setItem(r, 1, it1);

            auto* it2 = new QTableWidgetItem("0");
            it2->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_producedTable->setItem(r, 2, it2);

            auto* it3 = new QTableWidgetItem("0.00");
            it3->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_producedTable->setItem(r, 3, it3);

            auto* it4 = new QTableWidgetItem("0.00");
            it4->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_producedTable->setItem(r, 4, it4);
        }
    } else {
        onAddProducedRow();
    }

    onRecalculate();
}

void MillingVoucherWidget::openDateDialog(bool isInitial) {
    Q_UNUSED(isInitial);
    QDate curDate = QDate::fromString(m_batchDateEdit->text(), "dd-MM-yyyy");
    if (!curDate.isValid()) curDate = QDate::currentDate();

    QDate chosen = VoucherDateDialog::selectDate(this, curDate);
    if (chosen.isValid()) {
        m_batchDateEdit->setText(chosen.toString("dd-MM-yyyy"));
        m_dayLabel->setText(chosen.toString("dddd"));
        if (m_editingBatchId == 0 && m_millingModel) {
            m_batchNoEdit->setText(m_millingModel->get_next_batch_no(chosen.toString("yyyy-MM-dd")));
        }
    }
}

void MillingVoucherWidget::onAddConsumedRow() {
    int r = m_consumedTable->rowCount();
    m_consumedTable->insertRow(r);
    auto* it0 = new QTableWidgetItem("Paddy Raw");
    it0->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_consumedTable->setItem(r, 0, it0);

    auto* it1 = new QTableWidgetItem("0");
    it1->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_consumedTable->setItem(r, 1, it1);

    auto* it2 = new QTableWidgetItem("0.00");
    it2->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_consumedTable->setItem(r, 2, it2);

    auto* it3 = new QTableWidgetItem("0.00");
    it3->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_consumedTable->setItem(r, 3, it3);
}

void MillingVoucherWidget::onRemoveConsumedRow() {
    int r = m_consumedTable->currentRow();
    if (r >= 0 && m_consumedTable->rowCount() > 1) {
        m_consumedTable->removeRow(r);
        onRecalculate();
    }
}

void MillingVoucherWidget::onAddProducedRow() {
    int r = m_producedTable->rowCount();
    m_producedTable->insertRow(r);
    auto* it0 = new QTableWidgetItem("Rice Finished");
    it0->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_producedTable->setItem(r, 0, it0);

    auto* it1 = new QTableWidgetItem("0.0");
    it1->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_producedTable->setItem(r, 1, it1);

    auto* it2 = new QTableWidgetItem("0");
    it2->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_producedTable->setItem(r, 2, it2);

    auto* it3 = new QTableWidgetItem("0.00");
    it3->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_producedTable->setItem(r, 3, it3);

    auto* it4 = new QTableWidgetItem("0.00");
    it4->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_producedTable->setItem(r, 4, it4);
}

void MillingVoucherWidget::onRemoveProducedRow() {
    int r = m_producedTable->currentRow();
    if (r >= 0 && m_producedTable->rowCount() > 1) {
        m_producedTable->removeRow(r);
        onRecalculate();
    }
}

void MillingVoucherWidget::onConsumedItemConfigured(int row, const QVariantMap& itemData) {
    QString name = itemData.value("name").toString().trimmed();
    if (!name.isEmpty() && row >= 0 && row < m_consumedTable->rowCount()) {
        auto* it = m_consumedTable->item(row, 0);
        if (!it) {
            it = new QTableWidgetItem(name);
            it->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            m_consumedTable->setItem(row, 0, it);
        } else {
            it->setText(name);
        }
    }
    onRecalculate();
}

void MillingVoucherWidget::onProducedItemConfigured(int row, const QVariantMap& itemData) {
    QString name = itemData.value("name").toString().trimmed();
    if (!name.isEmpty() && row >= 0 && row < m_producedTable->rowCount()) {
        auto* it = m_producedTable->item(row, 0);
        if (!it) {
            it = new QTableWidgetItem(name);
            it->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            m_producedTable->setItem(row, 0, it);
        } else {
            it->setText(name);
        }
    }
    onRecalculate();
}

void MillingVoucherWidget::advanceConsumedCell() {
    if (!m_consumedTable) return;
    int curRow = m_consumedTable->currentRow();
    int curCol = m_consumedTable->currentColumn();

    if (curRow < 0) curRow = 0;
    if (curCol < 0) curCol = 0;

    int nextCol = curCol + 1;
    if (nextCol < m_consumedTable->columnCount()) {
        m_consumedTable->setCurrentCell(curRow, nextCol);
        m_consumedTable->edit(m_consumedTable->currentIndex());
    } else {
        if (curRow == m_consumedTable->rowCount() - 1) {
            onAddConsumedRow();
        }
        m_consumedTable->setCurrentCell(curRow + 1, 0);
        m_consumedTable->edit(m_consumedTable->currentIndex());
    }
}

void MillingVoucherWidget::retreatConsumedCell() {
    if (!m_consumedTable) return;
    int curRow = m_consumedTable->currentRow();
    int curCol = m_consumedTable->currentColumn();

    if (curRow < 0) curRow = 0;
    if (curCol < 0) curCol = 0;

    int prevCol = curCol - 1;
    if (prevCol >= 0) {
        m_consumedTable->setCurrentCell(curRow, prevCol);
        m_consumedTable->edit(m_consumedTable->currentIndex());
    } else if (curRow > 0) {
        m_consumedTable->setCurrentCell(curRow - 1, m_consumedTable->columnCount() - 1);
        m_consumedTable->edit(m_consumedTable->currentIndex());
    }
}

void MillingVoucherWidget::advanceProducedCell() {
    if (!m_producedTable) return;
    int curRow = m_producedTable->currentRow();
    int curCol = m_producedTable->currentColumn();

    if (curRow < 0) curRow = 0;
    if (curCol < 0) curCol = 0;

    int nextCol = curCol + 1;
    if (nextCol < m_producedTable->columnCount()) {
        m_producedTable->setCurrentCell(curRow, nextCol);
        m_producedTable->edit(m_producedTable->currentIndex());
    } else {
        if (curRow == m_producedTable->rowCount() - 1) {
            onAddProducedRow();
        }
        m_producedTable->setCurrentCell(curRow + 1, 0);
        m_producedTable->edit(m_producedTable->currentIndex());
    }
}

void MillingVoucherWidget::retreatProducedCell() {
    if (!m_producedTable) return;
    int curRow = m_producedTable->currentRow();
    int curCol = m_producedTable->currentColumn();

    if (curRow < 0) curRow = 0;
    if (curCol < 0) curCol = 0;

    int prevCol = curCol - 1;
    if (prevCol >= 0) {
        m_producedTable->setCurrentCell(curRow, prevCol);
        m_producedTable->edit(m_producedTable->currentIndex());
    } else if (curRow > 0) {
        m_producedTable->setCurrentCell(curRow - 1, m_producedTable->columnCount() - 1);
        m_producedTable->edit(m_producedTable->currentIndex());
    }
}

void MillingVoucherWidget::onRecalculate() {
    double totalInput = 0.0;
    for (int r = 0; r < m_consumedTable->rowCount(); ++r) {
        auto* it = m_consumedTable->item(r, 2);
        if (it) totalInput += it->text().toDouble();
    }

    double totalOutput = 0.0;
    double headRiceOutput = 0.0;
    for (int r = 0; r < m_producedTable->rowCount(); ++r) {
        auto* itName = m_producedTable->item(r, 0);
        auto* itWt = m_producedTable->item(r, 3);
        double wt = itWt ? itWt->text().toDouble() : 0.0;
        totalOutput += wt;

        if (totalInput > 0.0) {
            double rowPct = (wt / totalInput) * 100.0;
            auto* itPct = m_producedTable->item(r, 1);
            if (itPct) {
                m_producedTable->blockSignals(true);
                itPct->setText(QString::number(rowPct, 'f', 1));
                m_producedTable->blockSignals(false);
            }
        }

        if (itName && (itName->text().contains("Rice", Qt::CaseInsensitive) && !itName->text().contains("Bran", Qt::CaseInsensitive) && !itName->text().contains("Husk", Qt::CaseInsensitive))) {
            headRiceOutput += wt;
        }
    }

    double yieldPct = (totalInput > 0.0) ? (headRiceOutput / totalInput) * 100.0 : 0.0;
    double shortage = std::max(0.0, totalInput - totalOutput);
    double shortPct = (totalInput > 0.0) ? (shortage / totalInput) * 100.0 : 0.0;

    m_totalInputWeightLabel->setText(QString::number(totalInput, 'f', 2) + " Qtl");
    m_totalOutputWeightLabel->setText(QString::number(totalOutput, 'f', 2) + " Qtl");
    m_yieldPctLabel->setText(QString::number(yieldPct, 'f', 2) + " %");
    m_shortageLabel->setText(QString("%1 Qtl (%2%)").arg(QString::number(shortage, 'f', 2), QString::number(shortPct, 'f', 1)));
}

bool MillingVoucherWidget::loadBatchForEditing(const QVariant& batchIdOrNo) {
    if (!m_millingModel) return false;

    QVariantMap batch = m_millingModel->get_milling_batch(batchIdOrNo);
    if (batch.isEmpty()) return false;

    m_editingBatchId = batch.value("id").toInt();
    m_batchNoEdit->setText(batch.value("batch_no").toString());
    QString dStr = batch.value("batch_date").toString();
    QDate d = QDate::fromString(dStr, "yyyy-MM-dd");
    if (d.isValid()) {
        m_batchDateEdit->setText(d.toString("dd-MM-yyyy"));
        m_dayLabel->setText(d.toString("dddd"));
    }
    m_narrationEdit->setText(batch.value("narration").toString());

    m_consumedTable->setRowCount(0);
    QVariantList cList = batch.value("consumed_items").toList();
    for (const auto& itVal : cList) {
        auto itMap = itVal.toMap();
        int r = m_consumedTable->rowCount();
        m_consumedTable->insertRow(r);

        auto* it0 = new QTableWidgetItem(itMap.value("item_name").toString());
        it0->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_consumedTable->setItem(r, 0, it0);

        auto* it1 = new QTableWidgetItem(QString::number(itMap.value("bags").toInt()));
        it1->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_consumedTable->setItem(r, 1, it1);

        auto* it2 = new QTableWidgetItem(QString::number(itMap.value("weight_qtl").toDouble(), 'f', 2));
        it2->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_consumedTable->setItem(r, 2, it2);

        auto* it3 = new QTableWidgetItem(QString::number(itMap.value("amount").toDouble(), 'f', 2));
        it3->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_consumedTable->setItem(r, 3, it3);
    }

    m_producedTable->setRowCount(0);
    QVariantList pList = batch.value("produced_items").toList();
    for (const auto& itVal : pList) {
        auto itMap = itVal.toMap();
        int r = m_producedTable->rowCount();
        m_producedTable->insertRow(r);

        auto* it0 = new QTableWidgetItem(itMap.value("item_name").toString());
        it0->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_producedTable->setItem(r, 0, it0);

        auto* it1 = new QTableWidgetItem(QString::number(itMap.value("percentage").toDouble(), 'f', 1));
        it1->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_producedTable->setItem(r, 1, it1);

        auto* it2 = new QTableWidgetItem(QString::number(itMap.value("bags").toInt()));
        it2->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_producedTable->setItem(r, 2, it2);

        auto* it3 = new QTableWidgetItem(QString::number(itMap.value("weight_qtl").toDouble(), 'f', 2));
        it3->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_producedTable->setItem(r, 3, it3);

        auto* it4 = new QTableWidgetItem(QString::number(itMap.value("amount").toDouble(), 'f', 2));
        it4->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_producedTable->setItem(r, 4, it4);
    }

    onRecalculate();
    return true;
}

void MillingVoucherWidget::onSaveClicked() {
    double totalInput = 0.0;
    for (int r = 0; r < m_consumedTable->rowCount(); ++r) {
        auto* it = m_consumedTable->item(r, 2);
        if (it) totalInput += it->text().toDouble();
    }

    if (totalInput <= 0.0) {
        CustomMessageBox::showWarning(this, "Validation Error", "Please enter valid Paddy input weight.");
        return;
    }

    if (!CustomMessageBox::showConfirmation(this, "Confirm Save", "Are you sure you want to save Production Batch '" + m_batchNoEdit->text() + "'?")) {
        return;
    }

    bool ok = false;
    if (m_millingModel) {
        QVariantList cItems;
        for (int r = 0; r < m_consumedTable->rowCount(); ++r) {
            QVariantMap m;
            m["item_name"] = m_consumedTable->item(r, 0)->text();
            m["bags"] = m_consumedTable->item(r, 1)->text().toInt();
            m["weight_qtl"] = m_consumedTable->item(r, 2)->text().toDouble();
            m["amount"] = m_consumedTable->item(r, 3)->text().toDouble();
            cItems.append(m);
        }

        QVariantList pItems;
        for (int r = 0; r < m_producedTable->rowCount(); ++r) {
            QVariantMap m;
            m["item_name"] = m_producedTable->item(r, 0)->text();
            m["percentage"] = m_producedTable->item(r, 1)->text().toDouble();
            m["bags"] = m_producedTable->item(r, 2)->text().toInt();
            m["weight_qtl"] = m_producedTable->item(r, 3)->text().toDouble();
            m["amount"] = m_producedTable->item(r, 4)->text().toDouble();
            pItems.append(m);
        }

        QDate d = QDate::fromString(m_batchDateEdit->text(), "dd-MM-yyyy");
        ok = m_millingModel->add_milling_voucher(
            m_batchNoEdit->text(),
            d.toString("yyyy-MM-dd"),
            m_narrationEdit->text().trimmed(),
            cItems,
            pItems,
            m_editingBatchId
        );
    } else {
        ok = true;
    }

    if (ok) {
        CustomMessageBox::showInformation(this, "Success", "Production Batch saved successfully.");
        QString bNo = m_batchNoEdit->text();
        resetForm();
        emit batchSaved(bNo);
    } else {
        CustomMessageBox::showCritical(this, "Save Failed", "Failed to save Production Batch.");
    }
}

} // namespace MahadevERP
