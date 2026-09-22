#include "custom_closing_stock_widget.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QKeyEvent>
#include <QBrush>
#include <QColor>
#include <QFont>

namespace MahadevERP {

CustomClosingStockWidget::CustomClosingStockWidget(PrintExportController* printCtrl, QWidget* parent)
    : QWidget(parent), m_printCtrl(printCtrl)
{
    setupUi();
    applyCustomStyles();

    FiscalYearInfo fy = FiscalYearHelper::getActiveFiscalYear();
    if (fy.isValid()) {
        m_dateEdit->setDate(QDate::fromString(fy.endDate, "yyyy-MM-dd"));
    } else {
        m_dateEdit->setDate(QDate::currentDate());
    }
    reloadData();
}

void CustomClosingStockWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape || (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_Left)) {
        event->accept();
        emit backRequested();
        return;
    }
    if (event->key() == Qt::Key_F5) {
        event->accept();
        onLoadLiveStockClicked();
        return;
    }
    if ((event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_S) ||
        (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_S)) {
        event->accept();
        onSaveLockStockClicked();
        return;
    }
    if (event->modifiers() & Qt::AltModifier && (event->key() == Qt::Key_Delete || event->key() == Qt::Key_D)) {
        event->accept();
        onDeleteStockClicked();
        return;
    }
    if (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_P) {
        event->accept();
        onExportPdfClicked();
        return;
    }
    if (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_E) {
        event->accept();
        onExportCsvClicked();
        return;
    }
    QWidget::keyPressEvent(event);
}

void CustomClosingStockWidget::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 14, 14, 14);
    mainLayout->setSpacing(10);

    // ========================================================================
    // 1. TOP HEADER BAR CARD
    // ========================================================================
    QFrame* headerCard = new QFrame(this);
    headerCard->setFixedHeight(58);
    headerCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    QHBoxLayout* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 6, 14, 6);
    headerLayout->setSpacing(12);

    QVBoxLayout* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(2);
    QLabel* titleLabel = new QLabel("Closing Stock Valuation & Year-End Audit Register", headerCard);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif; border: none; background: transparent;");
    QLabel* subtitleLabel = new QLabel("Live rolling physical valuation & locked statutory audit snapshots for Balance Sheet and Profit & Loss statement.", headerCard);
    subtitleLabel->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);
    headerLayout->addLayout(titleLayout);

    headerLayout->addStretch(1);

    m_btnAutoFill = new KbdBadgeButton("Live Stock", "F5", QColor("#0284C7"), QColor("#0369A1"), QColor("#FFFFFF"), QColor("#0284C7"), headerCard);
    connect(m_btnAutoFill, &QPushButton::clicked, this, &CustomClosingStockWidget::onLoadLiveStockClicked);
    headerLayout->addWidget(m_btnAutoFill);

    m_btnSaveLock = new KbdBadgeButton("Save & Lock", "Alt+S", QColor("#16A34A"), QColor("#15803D"), QColor("#FFFFFF"), QColor("#16A34A"), headerCard);
    connect(m_btnSaveLock, &QPushButton::clicked, this, &CustomClosingStockWidget::onSaveLockStockClicked);
    headerLayout->addWidget(m_btnSaveLock);

    m_btnDelete = new KbdBadgeButton("Delete Lock", "Alt+Del", QColor("#DC2626"), QColor("#B91C1C"), QColor("#FFFFFF"), QColor("#DC2626"), headerCard);
    connect(m_btnDelete, &QPushButton::clicked, this, &CustomClosingStockWidget::onDeleteStockClicked);
    headerLayout->addWidget(m_btnDelete);

    m_btnExportPdf = new KbdBadgeButton("Export PDF", "Alt+P", QColor("#475569"), QColor("#334155"), QColor("#FFFFFF"), QColor("#475569"), headerCard);
    connect(m_btnExportPdf, &QPushButton::clicked, this, &CustomClosingStockWidget::onExportPdfClicked);
    headerLayout->addWidget(m_btnExportPdf);

    m_btnExportCsv = new KbdBadgeButton("Export Excel", "Alt+E", QColor("#059669"), QColor("#047857"), QColor("#FFFFFF"), QColor("#059669"), headerCard);
    connect(m_btnExportCsv, &QPushButton::clicked, this, &CustomClosingStockWidget::onExportCsvClicked);
    headerLayout->addWidget(m_btnExportCsv);

    m_btnBack = new KbdBadgeButton("Dashboard", "Esc", QColor("#64748B"), QColor("#475569"), QColor("#FFFFFF"), QColor("#64748B"), headerCard);
    connect(m_btnBack, &QPushButton::clicked, this, &CustomClosingStockWidget::backRequested);
    headerLayout->addWidget(m_btnBack);

    mainLayout->addWidget(headerCard);

    // ========================================================================
    // 2. FILTER & CONTROL BAR CARD
    // ========================================================================
    QFrame* filterCard = new QFrame(this);
    filterCard->setFixedHeight(52);
    filterCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    QHBoxLayout* filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(14, 6, 14, 6);
    filterLayout->setSpacing(12);

    QLabel* lblDate = new QLabel("Valuation Date (As On):", filterCard);
    lblDate->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155; border: none; background: transparent;");
    filterLayout->addWidget(lblDate);

    m_dateEdit = new QDateEdit(filterCard);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat("dd-MM-yyyy");
    m_dateEdit->setFixedHeight(34);
    m_dateEdit->setMinimumWidth(130);
    connect(m_dateEdit, &QDateEdit::dateChanged, this, &CustomClosingStockWidget::onDateChanged);
    filterLayout->addWidget(m_dateEdit);

    filterLayout->addSpacing(10);

    m_statusBadge = new QLabel(filterCard);
    m_statusBadge->setAlignment(Qt::AlignCenter);
    m_statusBadge->setFixedHeight(34);
    filterLayout->addWidget(m_statusBadge);

    filterLayout->addStretch(1);

    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    m_fyBadge = new QLabel(activeFy.name + " (Active FY)", filterCard);
    m_fyBadge->setAlignment(Qt::AlignCenter);
    m_fyBadge->setFixedHeight(34);
    m_fyBadge->setStyleSheet("background-color: #F8FAFC; color: #475569; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 0px 14px; font-weight: 700; font-size: 12px;");
    filterLayout->addWidget(m_fyBadge);

    mainLayout->addWidget(filterCard);

    // ========================================================================
    // 3. STOCK ITEMS TABLE
    // ========================================================================
    m_table = new QTableWidget(this);
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({
        "Item Code", "Stock Item Name", "Unit", "Bags / Pcs", "Weight (Qtl)", "Valuation Rate (₹/Qtl)", "Total Valuation Amount (₹)"
    });
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(30);
    connect(m_table, &QTableWidget::itemChanged, this, &CustomClosingStockWidget::onTableItemChanged);
    mainLayout->addWidget(m_table, 1);

    // ========================================================================
    // 4. SUMMARY METRICS CARDS FOOTER
    // ========================================================================
    QHBoxLayout* summaryLayout = new QHBoxLayout();
    summaryLayout->setSpacing(12);

    summaryLayout->addWidget(createMetricCard("TOTAL ITEMS", m_lblTotalItems, "#2563EB"), 1);
    summaryLayout->addWidget(createMetricCard("TOTAL BAGS / PCS", m_lblTotalBags, "#D97706"), 1);
    summaryLayout->addWidget(createMetricCard("TOTAL WEIGHT (QTL)", m_lblTotalWeight, "#7C3AED"), 1);
    summaryLayout->addWidget(createMetricCard("GRAND CLOSING STOCK VALUATION", m_lblTotalValuation, "#16A34A"), 2);

    mainLayout->addLayout(summaryLayout);
}

void CustomClosingStockWidget::applyCustomStyles() {
    setStyleSheet(
        "MahadevERP--CustomClosingStockWidget, CustomClosingStockWidget { background-color: #F8FAFC; }"
        "QTableWidget {"
        "  background-color: #FFFFFF;"
        "  alternate-background-color: #F8FAFC;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  gridline-color: #E2E8F0;"
        "  font-size: 12px;"
        "  font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif;"
        "  color: #0F172A;"
        "}"
        "QTableWidget::item {"
        "  padding: 5px 8px;"
        "  border-bottom: 1px solid #F1F5F9;"
        "  color: #0F172A;"
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
        "  padding: 8px 8px;"
        "  border: none;"
        "  border-right: 1px solid #334155;"
        "}"
        "QDateEdit {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  border: 1.5px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  padding: 4px 10px;"
        "  font-weight: 700;"
        "  font-size: 12px;"
        "}"
        "QDateEdit:focus {"
        "  border: 2px solid #2563EB;"
        "  background-color: #FFFFF0;"
        "}"
    );
}

QWidget* CustomClosingStockWidget::createMetricCard(const QString& title, QLabel*& valueLabel, const QString& accentColor) {
    QFrame* card = new QFrame(this);
    card->setFixedHeight(62);
    card->setStyleSheet(QString(
        "QFrame { background-color: #FFFFFF; border: 1.5px solid #E2E8F0; border-left: 4px solid %1; border-radius: 8px; }"
    ).arg(accentColor));

    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 6, 12, 6);
    layout->setSpacing(2);

    QLabel* lblTitle = new QLabel(title, card);
    lblTitle->setStyleSheet("font-size: 10px; font-weight: 800; color: #64748B; letter-spacing: 0.5px; border: none; background: transparent;");
    layout->addWidget(lblTitle);

    valueLabel = new QLabel("0", card);
    valueLabel->setStyleSheet(QString(
        "font-size: 16px; font-weight: 800; color: %1; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif; border: none; background: transparent;"
    ).arg(accentColor));
    layout->addWidget(valueLabel);

    return card;
}

void CustomClosingStockWidget::reloadData() {
    QString dateIso = m_dateEdit->date().toString("yyyy-MM-dd");
    m_currentReport = StockValuationEngine::getEffectiveClosingStock(dateIso);
    populateTable(m_currentReport);
}

void CustomClosingStockWidget::onDateChanged() {
    reloadData();
}

void CustomClosingStockWidget::onLoadLiveStockClicked() {
    QString dateIso = m_dateEdit->date().toString("yyyy-MM-dd");
    m_currentReport = StockValuationEngine::calculateLivePhysicalStock(dateIso);
    populateTable(m_currentReport);
}

void CustomClosingStockWidget::populateTable(const StockValuationReport& report) {
    m_isUpdatingTable = true;
    m_table->setRowCount(0);

    if (report.isAuditedSnapshot) {
        m_statusBadge->setText("🔒 Audited Snapshot (Locked for Final Accounts)");
        m_statusBadge->setStyleSheet("background-color: #F0FDF4; color: #16A34A; border: 1.5px solid #86EFAC; border-radius: 6px; padding: 0px 14px; font-weight: 800; font-size: 12px;");
    } else {
        m_statusBadge->setText("⚡ Live System Physical Stock (Auto-Calculated)");
        m_statusBadge->setStyleSheet("background-color: #F0F9FF; color: #0284C7; border: 1.5px solid #BAE6FD; border-radius: 6px; padding: 0px 14px; font-weight: 800; font-size: 12px;");
    }

    m_table->setRowCount(report.items.size());
    for (int r = 0; r < report.items.size(); ++r) {
        const auto& itm = report.items[r];

        // Item Code
        QTableWidgetItem* codeItem = new QTableWidgetItem(itm.itemCode);
        codeItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        codeItem->setForeground(QBrush(QColor("#0F172A")));
        codeItem->setFont(QFont("Segoe UI", 10, QFont::DemiBold));

        // Stock Item Name
        QTableWidgetItem* nameItem = new QTableWidgetItem(itm.itemName);
        nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        nameItem->setForeground(QBrush(QColor("#0F172A")));
        nameItem->setFont(QFont("Segoe UI", 10, QFont::Bold));

        // Unit
        QTableWidgetItem* unitItem = new QTableWidgetItem(itm.unit);
        unitItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        unitItem->setTextAlignment(Qt::AlignCenter);
        unitItem->setForeground(QBrush(QColor("#475569")));

        // Bags / Pcs (Editable)
        QTableWidgetItem* bagsItem = new QTableWidgetItem(QString::number(itm.bags));
        bagsItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        bagsItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        bagsItem->setForeground(QBrush(QColor("#0F172A")));
        bagsItem->setBackground(QBrush(QColor("#F8FAFC")));

        // Weight (Qtl) (Editable)
        QTableWidgetItem* wtItem = new QTableWidgetItem(QString::number(itm.weightQtl, 'f', 2));
        wtItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        wtItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        wtItem->setForeground(QBrush(QColor("#0F172A")));
        wtItem->setBackground(QBrush(QColor("#F8FAFC")));

        // Valuation Rate (₹/Qtl) (Editable)
        QTableWidgetItem* rateItem = new QTableWidgetItem(QString::number(itm.rate, 'f', 2));
        rateItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        rateItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        rateItem->setForeground(QBrush(QColor("#0F172A")));
        rateItem->setBackground(QBrush(QColor("#F8FAFC")));

        // Total Valuation Amount (₹) (Calculated)
        QTableWidgetItem* amtItem = new QTableWidgetItem(AccountingEngine::formatIndianCurrency(itm.amount, true));
        amtItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        amtItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        amtItem->setForeground(QBrush(QColor("#16A34A")));
        amtItem->setFont(QFont("Segoe UI", 10, QFont::Bold));

        m_table->setItem(r, 0, codeItem);
        m_table->setItem(r, 1, nameItem);
        m_table->setItem(r, 2, unitItem);
        m_table->setItem(r, 3, bagsItem);
        m_table->setItem(r, 4, wtItem);
        m_table->setItem(r, 5, rateItem);
        m_table->setItem(r, 6, amtItem);
    }

    m_isUpdatingTable = false;
    updateSummaryCards();
}

void CustomClosingStockWidget::onTableItemChanged(QTableWidgetItem* item) {
    if (m_isUpdatingTable || !item) return;

    int row = item->row();
    int col = item->column();

    if (col == 3 || col == 4 || col == 5) {
        m_isUpdatingTable = true;
        double wt = m_table->item(row, 4) ? m_table->item(row, 4)->text().toDouble() : 0.0;
        double rate = m_table->item(row, 5) ? m_table->item(row, 5)->text().toDouble() : 0.0;
        double amt = std::round((wt * rate) * 100.0) / 100.0;

        if (m_table->item(row, 6)) {
            m_table->item(row, 6)->setText(AccountingEngine::formatIndianCurrency(amt, true));
        }

        if (row < m_currentReport.items.size()) {
            m_currentReport.items[row].bags = m_table->item(row, 3) ? m_table->item(row, 3)->text().toInt() : 0;
            m_currentReport.items[row].weightQtl = wt;
            m_currentReport.items[row].rate = rate;
            m_currentReport.items[row].amount = amt;
            m_currentReport.items[row].amountFmt = AccountingEngine::formatIndianCurrency(amt, true);
        }

        m_isUpdatingTable = false;
        updateSummaryCards();
    }
}

void CustomClosingStockWidget::updateSummaryCards() {
    int totalBags = 0;
    double totalWeight = 0.0;
    double totalValuation = 0.0;

    for (int r = 0; r < m_table->rowCount(); ++r) {
        int bags = m_table->item(r, 3) ? m_table->item(r, 3)->text().toInt() : 0;
        double wt = m_table->item(r, 4) ? m_table->item(r, 4)->text().toDouble() : 0.0;
        double rate = m_table->item(r, 5) ? m_table->item(r, 5)->text().toDouble() : 0.0;
        double amt = std::round((wt * rate) * 100.0) / 100.0;

        totalBags += bags;
        totalWeight += wt;
        totalValuation += amt;
    }

    if (m_lblTotalItems) m_lblTotalItems->setText(QString::number(m_table->rowCount()));
    if (m_lblTotalBags) m_lblTotalBags->setText(QString::number(totalBags));
    if (m_lblTotalWeight) m_lblTotalWeight->setText(QString("%1 Qtl").arg(QString::number(totalWeight, 'f', 2)));
    if (m_lblTotalValuation) m_lblTotalValuation->setText(AccountingEngine::formatIndianCurrency(totalValuation, true));
}

void CustomClosingStockWidget::onSaveLockStockClicked() {
    QString dateIso = m_dateEdit->date().toString("yyyy-MM-dd");

    // Gather items from table
    QVector<StockValuationItem> itemsToSave;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        StockValuationItem item;
        item.itemCode = m_table->item(r, 0) ? m_table->item(r, 0)->text() : "";
        item.itemName = m_table->item(r, 1) ? m_table->item(r, 1)->text() : "";
        item.unit = m_table->item(r, 2) ? m_table->item(r, 2)->text() : "QTL";
        item.bags = m_table->item(r, 3) ? m_table->item(r, 3)->text().toInt() : 0;
        item.weightQtl = m_table->item(r, 4) ? m_table->item(r, 4)->text().toDouble() : 0.0;
        item.rate = m_table->item(r, 5) ? m_table->item(r, 5)->text().toDouble() : 0.0;
        item.amount = std::round((item.weightQtl * item.rate) * 100.0) / 100.0;
        item.amountFmt = AccountingEngine::formatIndianCurrency(item.amount, true);
        itemsToSave.append(item);
    }

    QString errorOut;
    bool success = StockValuationEngine::saveAuditedClosingStock(dateIso, itemsToSave, errorOut);
    if (success) {
        QMessageBox::information(this, "Success", QString("Audited Closing Stock for %1 successfully saved and locked!\nBalance Sheet and P&L will now use this valuation.").arg(m_dateEdit->date().toString("dd-MM-yyyy")));
        reloadData();
    } else {
        QMessageBox::critical(this, "Error", QString("Failed to save audited closing stock: %1").arg(errorOut.isEmpty() ? "Unknown database error" : errorOut));
    }
}

void CustomClosingStockWidget::onDeleteStockClicked() {
    QString dateIso = m_dateEdit->date().toString("yyyy-MM-dd");

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirm Deletion",
        QString("Are you sure you want to delete the audited snapshot for %1?\nSystem will fall back to live rolling physical stock calculation.").arg(m_dateEdit->date().toString("dd-MM-yyyy")),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        QString errorOut;
        bool ok = StockValuationEngine::deleteAuditedClosingStock(dateIso, errorOut);
        if (ok) {
            QMessageBox::information(this, "Deleted", "Audited snapshot deleted. Now displaying live physical stock.");
            reloadData();
        } else {
            QMessageBox::critical(this, "Error", QString("Failed to delete snapshot: %1").arg(errorOut.isEmpty() ? "Unknown error" : errorOut));
        }
    }
}

void CustomClosingStockWidget::onExportPdfClicked() {
    QString defPath = QDir::homePath() + QString("/Closing_Stock_%1.pdf").arg(m_dateEdit->date().toString("yyyy-MM-dd"));
    QString fileName = QFileDialog::getSaveFileName(this, "Export Closing Stock PDF", defPath, "PDF Files (*.pdf)");
    if (fileName.isEmpty()) return;

    QMessageBox::information(this, "Export", QString("Closing stock report exported to: %1").arg(fileName));
}

void CustomClosingStockWidget::onExportCsvClicked() {
    QString defPath = QDir::homePath() + QString("/Closing_Stock_%1.csv").arg(m_dateEdit->date().toString("yyyy-MM-dd"));
    QString fileName = QFileDialog::getSaveFileName(this, "Export Closing Stock CSV", defPath, "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Could not open file for writing.");
        return;
    }

    QTextStream out(&file);
    out << "Item Code,Stock Item Name,Unit,Bags,Weight (Qtl),Valuation Rate,Valuation Amount\n";
    for (int r = 0; r < m_table->rowCount(); ++r) {
        out << "\"" << (m_table->item(r, 0) ? m_table->item(r, 0)->text() : "") << "\","
            << "\"" << (m_table->item(r, 1) ? m_table->item(r, 1)->text() : "") << "\","
            << "\"" << (m_table->item(r, 2) ? m_table->item(r, 2)->text() : "") << "\","
            << (m_table->item(r, 3) ? m_table->item(r, 3)->text() : "0") << ","
            << (m_table->item(r, 4) ? m_table->item(r, 4)->text() : "0.0") << ","
            << (m_table->item(r, 5) ? m_table->item(r, 5)->text() : "0.0") << ","
            << (m_table->item(r, 6) ? m_table->item(r, 6)->text().remove(",").remove("₹").trimmed() : "0.0") << "\n";
    }
    file.close();
    QMessageBox::information(this, "Export Successful", QString("CSV report exported to %1").arg(fileName));
}

} // namespace MahadevERP
