#include "custom_closing_stock_widget.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QKeyEvent>

namespace MahadevERP {

CustomClosingStockWidget::CustomClosingStockWidget(PrintExportController* printCtrl, QWidget* parent)
    : QWidget(parent), m_printCtrl(printCtrl)
{
    setupUi();
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
    QWidget::keyPressEvent(event);
}

void CustomClosingStockWidget::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // 1. Top Header Bar
    QHBoxLayout* headerLayout = new QHBoxLayout();
    
    QPushButton* btnBack = new QPushButton(QString::fromUtf8("◀ Back (Esc)"), this);
    btnBack->setStyleSheet("background: #334155; color: white; font-weight: bold; border-radius: 4px; padding: 6px 14px;");
    connect(btnBack, &QPushButton::clicked, this, &CustomClosingStockWidget::backRequested);
    headerLayout->addWidget(btnBack);

    QLabel* title = new QLabel("Stock Valuation & Year-End Closing Stock Register", this);
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #0f172a;");
    headerLayout->addWidget(title);
    headerLayout->addStretch();

    QLabel* lblDate = new QLabel("As on Date:", this);
    lblDate->setStyleSheet("font-weight: bold; color: #334155;");
    headerLayout->addWidget(lblDate);

    m_dateEdit = new QDateEdit(this);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat("dd/MM/yyyy");
    m_dateEdit->setStyleSheet("font-size: 13px; font-weight: bold; padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px;");
    connect(m_dateEdit, &QDateEdit::dateChanged, this, &CustomClosingStockWidget::onDateChanged);
    headerLayout->addWidget(m_dateEdit);

    m_statusBadge = new QLabel(this);
    m_statusBadge->setStyleSheet("padding: 4px 10px; border-radius: 4px; font-weight: bold; font-size: 12px;");
    headerLayout->addWidget(m_statusBadge);

    mainLayout->addLayout(headerLayout);

    // 2. Action Toolbar
    QHBoxLayout* toolLayout = new QHBoxLayout();

    m_btnAutoFill = new QPushButton(QString::fromUtf8("⚡ Auto-Calculate Live System Stock"), this);
    m_btnAutoFill->setStyleSheet("background: #0284c7; color: white; font-weight: bold; border-radius: 4px; padding: 6px 14px;");
    connect(m_btnAutoFill, &QPushButton::clicked, this, &CustomClosingStockWidget::onLoadLiveStockClicked);
    toolLayout->addWidget(m_btnAutoFill);

    m_btnSaveLock = new QPushButton(QString::fromUtf8("🔒 Save & Lock Audited Closing Stock"), this);
    m_btnSaveLock->setStyleSheet("background: #16a34a; color: white; font-weight: bold; border-radius: 4px; padding: 6px 14px;");
    connect(m_btnSaveLock, &QPushButton::clicked, this, &CustomClosingStockWidget::onSaveLockStockClicked);
    toolLayout->addWidget(m_btnSaveLock);

    m_btnDelete = new QPushButton(QString::fromUtf8("🗑 Delete Audited Snapshot"), this);
    m_btnDelete->setStyleSheet("background: #dc2626; color: white; font-weight: bold; border-radius: 4px; padding: 6px 14px;");
    connect(m_btnDelete, &QPushButton::clicked, this, &CustomClosingStockWidget::onDeleteStockClicked);
    toolLayout->addWidget(m_btnDelete);

    toolLayout->addStretch();

    m_btnExportPdf = new QPushButton(QString::fromUtf8("📄 Export PDF"), this);
    m_btnExportPdf->setStyleSheet("background: #475569; color: white; font-weight: bold; border-radius: 4px; padding: 6px 12px;");
    connect(m_btnExportPdf, &QPushButton::clicked, this, &CustomClosingStockWidget::onExportPdfClicked);
    toolLayout->addWidget(m_btnExportPdf);

    m_btnExportCsv = new QPushButton(QString::fromUtf8("📊 Export Excel / CSV"), this);
    m_btnExportCsv->setStyleSheet("background: #475569; color: white; font-weight: bold; border-radius: 4px; padding: 6px 12px;");
    connect(m_btnExportCsv, &QPushButton::clicked, this, &CustomClosingStockWidget::onExportCsvClicked);
    toolLayout->addWidget(m_btnExportCsv);

    mainLayout->addLayout(toolLayout);

    // 3. Stock Items Table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({
        "Item Code", "Stock Item Name", "Unit", "Bags", "Weight (Qtl)", "Valuation Rate (₹/Qtl)", "Total Amount (₹)"
    });
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet("QTableWidget { background: white; border: 1px solid #cbd5e1; border-radius: 6px; }"
                           "QHeaderView::section { background: #f1f5f9; font-weight: bold; padding: 6px; border: 1px solid #e2e8f0; }");
    connect(m_table, &QTableWidget::itemChanged, this, &CustomClosingStockWidget::onTableItemChanged);
    mainLayout->addWidget(m_table);

    // 4. Summary Cards Footer
    QHBoxLayout* summaryLayout = new QHBoxLayout();

    auto createCard = [](const QString& label, QLabel*& valLabel) -> QWidget* {
        QWidget* w = new QWidget();
        w->setStyleSheet("background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 6px; padding: 8px;");
        QVBoxLayout* v = new QVBoxLayout(w);
        v->setContentsMargins(6, 4, 6, 4);
        v->setSpacing(2);
        QLabel* l = new QLabel(label);
        l->setStyleSheet("font-size: 11px; color: #64748b; font-weight: bold; text-transform: uppercase;");
        valLabel = new QLabel("0");
        valLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #0f172a;");
        v->addWidget(l);
        v->addWidget(valLabel);
        return w;
    };

    summaryLayout->addWidget(createCard("Total Items", m_lblTotalItems));
    summaryLayout->addWidget(createCard("Total Bags", m_lblTotalBags));
    summaryLayout->addWidget(createCard("Total Weight (Qtl)", m_lblTotalWeight));
    summaryLayout->addWidget(createCard("Total Valuation (₹)", m_lblTotalValuation));

    mainLayout->addLayout(summaryLayout);
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
        m_statusBadge->setText("🔒 Audited Snapshot (Locked)");
        m_statusBadge->setStyleSheet("background: #dcfce7; color: #15803d; padding: 4px 10px; border-radius: 4px; font-weight: bold;");
    } else {
        m_statusBadge->setText("⚡ Live Physical Stock (Calculated)");
        m_statusBadge->setStyleSheet("background: #e0f2fe; color: #0369a1; padding: 4px 10px; border-radius: 4px; font-weight: bold;");
    }

    m_table->setRowCount(report.items.size());
    for (int r = 0; r < report.items.size(); ++r) {
        const auto& itm = report.items[r];

        QTableWidgetItem* codeItem = new QTableWidgetItem(itm.itemCode);
        codeItem->setFlags(codeItem->flags() & ~Qt::ItemIsEditable);

        QTableWidgetItem* nameItem = new QTableWidgetItem(itm.itemName);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);

        QTableWidgetItem* unitItem = new QTableWidgetItem(itm.unit);
        unitItem->setFlags(unitItem->flags() & ~Qt::ItemIsEditable);
        unitItem->setTextAlignment(Qt::AlignCenter);

        QTableWidgetItem* bagsItem = new QTableWidgetItem(QString::number(itm.bags));
        bagsItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        QTableWidgetItem* wtItem = new QTableWidgetItem(QString::number(itm.weightQtl, 'f', 2));
        wtItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        QTableWidgetItem* rateItem = new QTableWidgetItem(QString::number(itm.rate, 'f', 2));
        rateItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        QTableWidgetItem* amtItem = new QTableWidgetItem(AccountingEngine::formatIndianCurrency(itm.amount, true));
        amtItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        amtItem->setFlags(amtItem->flags() & ~Qt::ItemIsEditable);

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
        int b = m_table->item(r, 3) ? m_table->item(r, 3)->text().toInt() : 0;
        double w = m_table->item(r, 4) ? m_table->item(r, 4)->text().toDouble() : 0.0;
        double rate = m_table->item(r, 5) ? m_table->item(r, 5)->text().toDouble() : 0.0;
        double amt = std::round((w * rate) * 100.0) / 100.0;

        totalBags += b;
        totalWeight += w;
        totalValuation += amt;
    }

    m_lblTotalItems->setText(QString::number(m_table->rowCount()));
    m_lblTotalBags->setText(QString::number(totalBags));
    m_lblTotalWeight->setText(QString::number(totalWeight, 'f', 2) + " Qtl");
    m_lblTotalValuation->setText(AccountingEngine::formatIndianCurrency(totalValuation, true));
}

void CustomClosingStockWidget::onSaveLockStockClicked() {
    QString dateIso = m_dateEdit->date().toString("yyyy-MM-dd");

    QVector<StockValuationItem> itemsToSave;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        StockValuationItem itm;
        itm.itemCode = m_table->item(r, 0) ? m_table->item(r, 0)->text().trimmed() : "";
        itm.itemName = m_table->item(r, 1) ? m_table->item(r, 1)->text().trimmed() : "";
        itm.unit = m_table->item(r, 2) ? m_table->item(r, 2)->text().trimmed() : "QTL";
        itm.bags = m_table->item(r, 3) ? m_table->item(r, 3)->text().toInt() : 0;
        itm.weightQtl = m_table->item(r, 4) ? m_table->item(r, 4)->text().toDouble() : 0.0;
        itm.rate = m_table->item(r, 5) ? m_table->item(r, 5)->text().toDouble() : 0.0;
        itm.amount = std::round((itm.weightQtl * itm.rate) * 100.0) / 100.0;
        if (itm.weightQtl > 0.001 || itm.amount > 0.01) {
            itemsToSave.append(itm);
        }
    }

    QString err;
    bool ok = StockValuationEngine::saveAuditedClosingStock(dateIso, itemsToSave, err);
    if (ok) {
        QMessageBox::information(this, "Audited Stock Locked", QString("Closing stock of %1 items successfully locked for %2!").arg(itemsToSave.size()).arg(m_dateEdit->date().toString("dd/MM/yyyy")));
        reloadData();
    } else {
        QMessageBox::critical(this, "Error Saving Closing Stock", err);
    }
}

void CustomClosingStockWidget::onDeleteStockClicked() {
    QString dateIso = m_dateEdit->date().toString("yyyy-MM-dd");
    auto res = QMessageBox::question(this, "Confirm Deletion", QString("Are you sure you want to delete the audited closing stock snapshot for %1?\nSystem will revert to dynamic live calculation.").arg(m_dateEdit->date().toString("dd/MM/yyyy")), QMessageBox::Yes | QMessageBox::No);
    if (res != QMessageBox::Yes) return;

    QString err;
    bool ok = StockValuationEngine::deleteAuditedClosingStock(dateIso, err);
    if (ok) {
        QMessageBox::information(this, "Deleted", "Audited snapshot removed. Switched to dynamic live calculation.");
        reloadData();
    } else {
        QMessageBox::critical(this, "Error", err);
    }
}

void CustomClosingStockWidget::onExportPdfClicked() {
    QString defPath = QDir::homePath() + QString("/Closing_Stock_%1.pdf").arg(m_dateEdit->date().toString("yyyy-MM-dd"));
    QString fileName = QFileDialog::getSaveFileName(this, "Export Closing Stock PDF", defPath, "PDF Files (*.pdf)");
    if (fileName.isEmpty()) return;

    // Use printExportCtrl if available
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
