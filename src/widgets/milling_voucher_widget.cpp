#include "milling_voucher_widget.h"
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
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    // 1. Top Header Bar
    auto* headerLayout = new QHBoxLayout();
    auto* titleCol = new QVBoxLayout();
    auto* titleLabel = new QLabel("Milling Production & Out-Turn Voucher (Batch Yield)", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #0F172A;");
    auto* subLabel = new QLabel("Record paddy milling batches, track rice & by-product out-turn recovery against statutory 67% yield benchmark.", this);
    subLabel->setStyleSheet("font-size: 11px; color: #64748B;");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();

    auto* backBtn = new KbdBadgeButton("← Back to Dashboard", "Esc", this);
    backBtn->setPrimaryColor("#F1F5F9", "#E2E8F0");
    backBtn->setTextColor("#475569");
    connect(backBtn, &QPushButton::clicked, this, &MillingVoucherWidget::backRequested);
    headerLayout->addWidget(backBtn);
    rootLayout->addLayout(headerLayout);

    // 2. Voucher Meta Card
    auto* metaCard = new QFrame(this);
    metaCard->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }");
    auto* metaGrid = new QGridLayout(metaCard);
    metaGrid->setContentsMargins(14, 10, 14, 10);
    metaGrid->setSpacing(12);

    m_batchNoEdit = new QLineEdit(metaCard);
    m_batchNoEdit->setReadOnly(true);
    m_batchNoEdit->setStyleSheet("background-color: #F8FAFC; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 6px 10px; font-weight: bold; color: #1D4ED8;");
    metaGrid->addWidget(new QLabel("Batch / Voucher No:"), 0, 0);
    metaGrid->addWidget(m_batchNoEdit, 0, 1);

    m_batchDateEdit = new QLineEdit(metaCard);
    m_batchDateEdit->setReadOnly(true);
    m_batchDateEdit->setStyleSheet("background-color: #F8FAFC; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 6px 10px; font-weight: bold;");
    auto* dateBtn = new QPushButton("📅 Date (F2)", metaCard);
    dateBtn->setStyleSheet("background-color: #EFF6FF; border: 1px solid #93C5FD; border-radius: 6px; padding: 6px 10px; font-weight: bold; color: #1D4ED8;");
    connect(dateBtn, &QPushButton::clicked, this, [this]() { openDateDialog(false); });

    auto* dateRow = new QHBoxLayout();
    dateRow->addWidget(m_batchDateEdit, 1);
    dateRow->addWidget(dateBtn);

    metaGrid->addWidget(new QLabel("Batch Date *:"), 0, 2);
    metaGrid->addLayout(dateRow, 0, 3);

    m_dayLabel = new QLabel("Wednesday", metaCard);
    m_dayLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #64748B;");
    metaGrid->addWidget(m_dayLabel, 0, 4);

    m_narrationEdit = new QLineEdit(metaCard);
    m_narrationEdit->setPlaceholderText("Batch process narration / Milling shift remarks...");
    m_narrationEdit->setStyleSheet("background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 6px 10px;");
    metaGrid->addWidget(new QLabel("Narration / Notes:"), 1, 0);
    metaGrid->addWidget(m_narrationEdit, 1, 1, 1, 4);

    rootLayout->addWidget(metaCard);

    // 3. Split Table Section (Input Paddy Consumed VS Output Rice Produced)
    auto* tablesLayout = new QHBoxLayout();
    tablesLayout->setSpacing(14);

    const QString tableStyle =
        "QTableWidget { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 6px; gridline-color: #F1F5F9; font-size: 12px; color: #0F172A; }"
        "QTableWidget::item { padding: 4px 6px; }"
        "QHeaderView::section { background-color: #0F172A; color: #FFFFFF; font-weight: bold; font-size: 11px; padding: 5px; border: none; }";

    // Left: Consumed Paddy
    auto* consumedGroup = new QGroupBox("RAW PADDY INPUT / CONSUMPTION", this);
    consumedGroup->setStyleSheet("QGroupBox { font-size: 11px; font-weight: bold; color: #DC2626; border: 1px solid #FECACA; border-radius: 8px; margin-top: 6px; padding-top: 14px; background-color: #FEF2F2; }");
    auto* cLayout = new QVBoxLayout(consumedGroup);
    cLayout->setContentsMargins(8, 8, 8, 8);

    m_consumedTable = new QTableWidget(consumedGroup);
    m_consumedTable->setColumnCount(4);
    m_consumedTable->setHorizontalHeaderLabels({"Paddy Variety / Item", "Bags", "Weight (Qtl)", "Amount (₹)"});
    m_consumedTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_consumedTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_consumedTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_consumedTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_consumedTable->verticalHeader()->setVisible(false);
    m_consumedTable->setStyleSheet(tableStyle);
    cLayout->addWidget(m_consumedTable);

    auto* cBtnRow = new QHBoxLayout();
    auto* addCBtn = new QPushButton("+ Add Input Row", consumedGroup);
    addCBtn->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 4px; padding: 4px 8px; font-weight: bold;");
    connect(addCBtn, &QPushButton::clicked, this, &MillingVoucherWidget::onAddConsumedRow);
    cBtnRow->addWidget(addCBtn);

    auto* remCBtn = new QPushButton("- Remove Row", consumedGroup);
    remCBtn->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 4px; padding: 4px 8px;");
    connect(remCBtn, &QPushButton::clicked, this, &MillingVoucherWidget::onRemoveConsumedRow);
    cBtnRow->addWidget(remCBtn);
    cBtnRow->addStretch();
    cLayout->addLayout(cBtnRow);

    tablesLayout->addWidget(consumedGroup, 1);

    // Right: Produced Rice & By-Products
    auto* producedGroup = new QGroupBox("RICE & BY-PRODUCTS OUTPUT RECOVERY", this);
    producedGroup->setStyleSheet("QGroupBox { font-size: 11px; font-weight: bold; color: #16A34A; border: 1px solid #BBF7D0; border-radius: 8px; margin-top: 6px; padding-top: 14px; background-color: #F0FDF4; }");
    auto* pLayout = new QVBoxLayout(producedGroup);
    pLayout->setContentsMargins(8, 8, 8, 8);

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
    pLayout->addWidget(m_producedTable);

    auto* pBtnRow = new QHBoxLayout();
    auto* addPBtn = new QPushButton("+ Add Output Row", producedGroup);
    addPBtn->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 4px; padding: 4px 8px; font-weight: bold;");
    connect(addPBtn, &QPushButton::clicked, this, &MillingVoucherWidget::onAddProducedRow);
    pBtnRow->addWidget(addPBtn);

    auto* remPBtn = new QPushButton("- Remove Row", producedGroup);
    remPBtn->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 4px; padding: 4px 8px;");
    connect(remPBtn, &QPushButton::clicked, this, &MillingVoucherWidget::onRemoveProducedRow);
    pBtnRow->addWidget(remPBtn);
    pBtnRow->addStretch();
    pLayout->addLayout(pBtnRow);

    tablesLayout->addWidget(producedGroup, 1);
    rootLayout->addLayout(tablesLayout, 1);

    // 4. Live Yield Benchmarks & Metric Cards
    auto* metricsLayout = new QHBoxLayout();
    metricsLayout->setSpacing(10);

    auto createMetric = [this, metricsLayout](const QString& title, const QString& initVal, const QString& color) {
        auto* card = new QFrame(this);
        card->setStyleSheet(
            "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; border-left: 4px solid " + color + "; }"
            "QLabel { border: none; background: transparent; }"
        );
        auto* cLayout = new QVBoxLayout(card);
        cLayout->setContentsMargins(10, 6, 10, 6);
        cLayout->setSpacing(2);

        auto* tLabel = new QLabel(title, card);
        tLabel->setStyleSheet("font-size: 9.5px; font-weight: 800; color: #64748B; letter-spacing: 0.5px; border: none; background: transparent;");
        auto* vLabel = new QLabel(initVal, card);
        vLabel->setStyleSheet("font-size: 14px; font-weight: 800; color: " + color + "; border: none; background: transparent;");
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

    // 5. Bottom Action Bar
    auto* bottomLayout = new QHBoxLayout();
    auto* cancelBottomBtn = new KbdBadgeButton("Cancel", "Esc", this);
    cancelBottomBtn->setPrimaryColor("#F1F5F9", "#E2E8F0");
    cancelBottomBtn->setTextColor("#475569");
    connect(cancelBottomBtn, &QPushButton::clicked, this, &MillingVoucherWidget::backRequested);
    bottomLayout->addWidget(cancelBottomBtn);

    bottomLayout->addStretch();

    auto* saveBtn = new KbdBadgeButton("Save Production Batch", "Ctrl+S", this);
    saveBtn->setPrimaryColor("#16A34A", "#15803D");
    saveBtn->setTextColor("#FFFFFF");
    saveBtn->setMinimumWidth(220);
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
            m_producedTable->setItem(r, 0, new QTableWidgetItem(d));
            m_producedTable->setItem(r, 1, new QTableWidgetItem("0.0"));
            m_producedTable->setItem(r, 2, new QTableWidgetItem("0"));
            m_producedTable->setItem(r, 3, new QTableWidgetItem("0.00"));
            m_producedTable->setItem(r, 4, new QTableWidgetItem("0.00"));
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
    m_consumedTable->setItem(r, 0, new QTableWidgetItem("Paddy Raw"));
    m_consumedTable->setItem(r, 1, new QTableWidgetItem("0"));
    m_consumedTable->setItem(r, 2, new QTableWidgetItem("0.00"));
    m_consumedTable->setItem(r, 3, new QTableWidgetItem("0.00"));
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
    m_producedTable->setItem(r, 0, new QTableWidgetItem("Rice Finished"));
    m_producedTable->setItem(r, 1, new QTableWidgetItem("0.0"));
    m_producedTable->setItem(r, 2, new QTableWidgetItem("0"));
    m_producedTable->setItem(r, 3, new QTableWidgetItem("0.00"));
    m_producedTable->setItem(r, 4, new QTableWidgetItem("0.00"));
}

void MillingVoucherWidget::onRemoveProducedRow() {
    int r = m_producedTable->currentRow();
    if (r >= 0 && m_producedTable->rowCount() > 1) {
        m_producedTable->removeRow(r);
        onRecalculate();
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
        auto* it = m_producedTable->item(r, 3);
        double wt = it ? it->text().toDouble() : 0.0;
        totalOutput += wt;
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
        m_consumedTable->setItem(r, 0, new QTableWidgetItem(itMap.value("item_name").toString()));
        m_consumedTable->setItem(r, 1, new QTableWidgetItem(QString::number(itMap.value("bags").toInt())));
        m_consumedTable->setItem(r, 2, new QTableWidgetItem(QString::number(itMap.value("weight_qtl").toDouble(), 'f', 2)));
        m_consumedTable->setItem(r, 3, new QTableWidgetItem(QString::number(itMap.value("amount").toDouble(), 'f', 2)));
    }

    m_producedTable->setRowCount(0);
    QVariantList pList = batch.value("produced_items").toList();
    for (const auto& itVal : pList) {
        auto itMap = itVal.toMap();
        int r = m_producedTable->rowCount();
        m_producedTable->insertRow(r);
        m_producedTable->setItem(r, 0, new QTableWidgetItem(itMap.value("item_name").toString()));
        m_producedTable->setItem(r, 1, new QTableWidgetItem(QString::number(itMap.value("percentage").toDouble(), 'f', 1)));
        m_producedTable->setItem(r, 2, new QTableWidgetItem(QString::number(itMap.value("bags").toInt())));
        m_producedTable->setItem(r, 3, new QTableWidgetItem(QString::number(itMap.value("weight_qtl").toDouble(), 'f', 2)));
        m_producedTable->setItem(r, 4, new QTableWidgetItem(QString::number(itMap.value("amount").toDouble(), 'f', 2)));
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
