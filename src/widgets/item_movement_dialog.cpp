#include "item_movement_dialog.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"
#include "../database_manager.h"
#include "../models/stock_items_model.h"
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QKeyEvent>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QTabWidget>

namespace MahadevERP {

ItemMovementDialog::ItemMovementDialog(const QString& itemName, const QDate& fromDate, const QDate& toDate,
                                       PrintExportController* printCtrl, QWidget* parent)
    : QDialog(parent), m_itemName(itemName), m_printCtrl(printCtrl)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    resize(1200, 700);
    setWindowTitle(QString("Stock Item Movement Register - %1").arg(itemName));
    setupUi();
    applyCustomStyles();

    if (fromDate.isValid()) m_fromDateEdit->setDate(fromDate);
    if (toDate.isValid()) m_toDateEdit->setDate(toDate);

    loadMovements(itemName, m_fromDateEdit->date(), m_toDateEdit->date());
}

void ItemMovementDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        event->accept();
        close();
        return;
    }
    if (event->key() == Qt::Key_F5) {
        event->accept();
        onRefreshClicked();
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
    QDialog::keyPressEvent(event);
}

void ItemMovementDialog::setupUi() {
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
    m_itemTitleLabel = new QLabel(QString("Item Movement: %1").arg(m_itemName), headerCard);
    m_itemTitleLabel->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif; border: none; background: transparent;");
    QLabel* subtitleLabel = new QLabel("Chronological Inward and Outward transaction history, batch consumptions, and sales dispatches.", headerCard);
    subtitleLabel->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    titleLayout->addWidget(m_itemTitleLabel);
    titleLayout->addWidget(subtitleLabel);
    headerLayout->addLayout(titleLayout);

    headerLayout->addStretch(1);

    m_btnRefresh = new KbdBadgeButton("Refresh", "F5", QColor("#2563EB"), QColor("#1D4ED8"), QColor("#FFFFFF"), QColor("#2563EB"), headerCard);
    connect(m_btnRefresh, &QPushButton::clicked, this, &ItemMovementDialog::onRefreshClicked);
    headerLayout->addWidget(m_btnRefresh);

    m_btnExportPdf = new KbdBadgeButton("Export PDF", "Alt+P", QColor("#475569"), QColor("#334155"), QColor("#FFFFFF"), QColor("#475569"), headerCard);
    connect(m_btnExportPdf, &QPushButton::clicked, this, &ItemMovementDialog::onExportPdfClicked);
    headerLayout->addWidget(m_btnExportPdf);

    m_btnExportCsv = new KbdBadgeButton("Export Excel", "Alt+E", QColor("#059669"), QColor("#047857"), QColor("#FFFFFF"), QColor("#059669"), headerCard);
    connect(m_btnExportCsv, &QPushButton::clicked, this, &ItemMovementDialog::onExportCsvClicked);
    headerLayout->addWidget(m_btnExportCsv);

    m_btnClose = new KbdBadgeButton("Close", "Esc", QColor("#64748B"), QColor("#475569"), QColor("#FFFFFF"), QColor("#64748B"), headerCard);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::close);
    headerLayout->addWidget(m_btnClose);

    mainLayout->addWidget(headerCard);

    // ========================================================================
    // 2. FILTER CARD
    // ========================================================================
    QFrame* filterCard = new QFrame(this);
    filterCard->setFixedHeight(52);
    filterCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    QHBoxLayout* filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(14, 6, 14, 6);
    filterLayout->setSpacing(12);

    QLabel* lblFrom = new QLabel("From Date:", filterCard);
    lblFrom->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155; border: none; background: transparent;");
    filterLayout->addWidget(lblFrom);

    m_fromDateEdit = new QDateEdit(filterCard);
    m_fromDateEdit->setCalendarPopup(true);
    m_fromDateEdit->setDisplayFormat("dd-MM-yyyy");
    m_fromDateEdit->setFixedHeight(34);
    m_fromDateEdit->setMinimumWidth(125);
    filterLayout->addWidget(m_fromDateEdit);

    QLabel* lblTo = new QLabel("To Date:", filterCard);
    lblTo->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155; border: none; background: transparent;");
    filterLayout->addWidget(lblTo);

    m_toDateEdit = new QDateEdit(filterCard);
    m_toDateEdit->setCalendarPopup(true);
    m_toDateEdit->setDisplayFormat("dd-MM-yyyy");
    m_toDateEdit->setFixedHeight(34);
    m_toDateEdit->setMinimumWidth(125);
    filterLayout->addWidget(m_toDateEdit);

    filterLayout->addStretch(1);

    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    QLabel* fyBadge = new QLabel(activeFy.name + " (Active FY)", filterCard);
    fyBadge->setAlignment(Qt::AlignCenter);
    fyBadge->setFixedHeight(34);
    fyBadge->setStyleSheet("background-color: #F8FAFC; color: #475569; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 0px 14px; font-weight: 700; font-size: 12px;");
    filterLayout->addWidget(fyBadge);

    mainLayout->addWidget(filterCard);

    // ========================================================================
    // 3. TWO-PANEL SPLITTER (INWARD vs OUTWARD)
    // ========================================================================
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

    // Left: Inward Container
    QWidget* inwardContainer = new QWidget(splitter);
    QVBoxLayout* inwardLayout = new QVBoxLayout(inwardContainer);
    inwardLayout->setContentsMargins(0, 0, 0, 0);
    inwardLayout->setSpacing(4);

    QLabel* inHeader = new QLabel("INWARD TRANSACTIONS (Purchases / Procurements / Yields)", inwardContainer);
    inHeader->setStyleSheet("font-size: 12px; font-weight: 800; color: #0284C7; padding: 2px;");
    inwardLayout->addWidget(inHeader);

    m_inwardTable = new QTableWidget(inwardContainer);
    m_inwardTable->setColumnCount(7);
    m_inwardTable->setHorizontalHeaderLabels({"Date", "Vch No", "Party / Description", "Bags", "Qty (Qtl)", "Rate (₹)", "Amount (₹)"});
    m_inwardTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_inwardTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_inwardTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_inwardTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_inwardTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_inwardTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_inwardTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_inwardTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_inwardTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_inwardTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_inwardTable->setAlternatingRowColors(true);
    m_inwardTable->verticalHeader()->setVisible(false);
    m_inwardTable->verticalHeader()->setDefaultSectionSize(28);
    connect(m_inwardTable, &QTableWidget::cellDoubleClicked, this, &ItemMovementDialog::onInwardDoubleClicked);
    inwardLayout->addWidget(m_inwardTable);

    // Right: Outward Container
    QWidget* outwardContainer = new QWidget(splitter);
    QVBoxLayout* outwardLayout = new QVBoxLayout(outwardContainer);
    outwardLayout->setContentsMargins(0, 0, 0, 0);
    outwardLayout->setSpacing(4);

    QLabel* outHeader = new QLabel("OUTWARD TRANSACTIONS (Sales / Milling Consumptions / Dispatches)", outwardContainer);
    outHeader->setStyleSheet("font-size: 12px; font-weight: 800; color: #DC2626; padding: 2px;");
    outwardLayout->addWidget(outHeader);

    m_outwardTable = new QTableWidget(outwardContainer);
    m_outwardTable->setColumnCount(7);
    m_outwardTable->setHorizontalHeaderLabels({"Date", "Vch No", "Party / Description", "Bags", "Qty (Qtl)", "Rate (₹)", "Amount (₹)"});
    m_outwardTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_outwardTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_outwardTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_outwardTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_outwardTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_outwardTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_outwardTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_outwardTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_outwardTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_outwardTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_outwardTable->setAlternatingRowColors(true);
    m_outwardTable->verticalHeader()->setVisible(false);
    m_outwardTable->verticalHeader()->setDefaultSectionSize(28);
    connect(m_outwardTable, &QTableWidget::cellDoubleClicked, this, &ItemMovementDialog::onOutwardDoubleClicked);
    outwardLayout->addWidget(m_outwardTable);

    splitter->addWidget(inwardContainer);
    splitter->addWidget(outwardContainer);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter, 1);

    // ========================================================================
    // 4. SUMMARY METRICS CARDS FOOTER
    // ========================================================================
    QHBoxLayout* summaryLayout = new QHBoxLayout();
    summaryLayout->setSpacing(12);

    summaryLayout->addWidget(createMetricCard("TOTAL INWARD WEIGHT", m_lblInwardWeight, "#0284C7"), 1);
    summaryLayout->addWidget(createMetricCard("TOTAL INWARD VALUE (₹)", m_lblInwardVal, "#0284C7"), 1);
    summaryLayout->addWidget(createMetricCard("TOTAL OUTWARD WEIGHT", m_lblOutwardWeight, "#DC2626"), 1);
    summaryLayout->addWidget(createMetricCard("TOTAL OUTWARD VALUE (₹)", m_lblOutwardVal, "#DC2626"), 1);
    summaryLayout->addWidget(createMetricCard("NET CLOSING BALANCE", m_lblClosingWeight, "#16A34A"), 1);

    mainLayout->addLayout(summaryLayout);
}

void ItemMovementDialog::applyCustomStyles() {
    setStyleSheet(
        "MahadevERP--ItemMovementDialog, ItemMovementDialog { background-color: #F8FAFC; }"
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
        "  padding: 4px 6px;"
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
        "  font-size: 11px;"
        "  padding: 6px 6px;"
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

QWidget* ItemMovementDialog::createMetricCard(const QString& title, QLabel*& valueLabel, const QString& accentColor) {
    QFrame* card = new QFrame(this);
    card->setFixedHeight(58);
    card->setStyleSheet(QString(
        "QFrame { background-color: #FFFFFF; border: 1.5px solid #E2E8F0; border-left: 4px solid %1; border-radius: 8px; } QLabel { border: none; background: transparent; }"
    ).arg(accentColor));

    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 4, 10, 4);
    layout->setSpacing(1);

    QLabel* lblTitle = new QLabel(title, card);
    lblTitle->setStyleSheet("font-size: 9px; font-weight: 800; color: #64748B; letter-spacing: 0.5px; border: none; background: transparent;");
    layout->addWidget(lblTitle);

    valueLabel = new QLabel("0.00", card);
    valueLabel->setStyleSheet(QString(
        "font-size: 15px; font-weight: 800; color: %1; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif; border: none; background: transparent;"
    ).arg(accentColor));
    layout->addWidget(valueLabel);

    return card;
}

void ItemMovementDialog::loadMovements(const QString& itemName, const QDate& fromDate, const QDate& toDate) {
    m_itemName = itemName;
    if (m_itemTitleLabel) {
        m_itemTitleLabel->setText(QString("Item Movement: %1").arg(itemName));
    }

    m_inwardTable->setRowCount(0);
    m_outwardTable->setRowCount(0);

    StockItemsModel tempModel;
    QString fStr = fromDate.isValid() ? fromDate.toString("yyyy-MM-dd") : "";
    QString tStr = toDate.isValid() ? toDate.toString("yyyy-MM-dd") : "";
    QVariantList movements = tempModel.get_item_movements(itemName, fStr, tStr);

    double inWt = 0.0, inVal = 0.0, outWt = 0.0, outVal = 0.0;
    int inBags = 0, outBags = 0;

    for (const QVariant& mVar : movements) {
        QVariantMap m = mVar.toMap();
        bool isInward = m.value("isInward").toBool();
        QString vDate = m.value("vDate").toString();
        QString refNo = m.value("refNo").toString();
        QString party = m.value("party").toString();
        int bags = m.value("bags").toInt();
        double qty = m.value("qty").toDouble();
        double rate = m.value("rate").toDouble();
        double amount = m.value("amount").toDouble();

        QTableWidget* targetTable = isInward ? m_inwardTable : m_outwardTable;
        int row = targetTable->rowCount();
        targetTable->insertRow(row);

        auto createItem = [](const QString& text, Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter, bool bold = false) {
            QTableWidgetItem* item = new QTableWidgetItem(text);
            item->setTextAlignment(align);
            item->setForeground(QBrush(QColor("#0F172A")));
            if (bold) item->setFont(QFont("Segoe UI", 10, QFont::Bold));
            return item;
        };

        targetTable->setItem(row, 0, createItem(vDate, Qt::AlignCenter));
        targetTable->setItem(row, 1, createItem(refNo, Qt::AlignCenter, true));
        targetTable->setItem(row, 2, createItem(party, Qt::AlignLeft | Qt::AlignVCenter));
        targetTable->setItem(row, 3, createItem(QString::number(bags), Qt::AlignRight | Qt::AlignVCenter));
        targetTable->setItem(row, 4, createItem(QString::number(qty, 'f', 2), Qt::AlignRight | Qt::AlignVCenter));
        targetTable->setItem(row, 5, createItem(QString::number(rate, 'f', 2), Qt::AlignRight | Qt::AlignVCenter));
        targetTable->setItem(row, 6, createItem(AccountingEngine::formatIndianCurrency(amount, true), Qt::AlignRight | Qt::AlignVCenter, true));

        if (isInward) {
            inWt += qty;
            inVal += amount;
            inBags += bags;
        } else {
            outWt += qty;
            outVal += amount;
            outBags += bags;
        }
    }

    if (m_lblInwardWeight) m_lblInwardWeight->setText(QString("%1 Qtl (%2 Bags)").arg(QString::number(inWt, 'f', 2)).arg(inBags));
    if (m_lblInwardVal) m_lblInwardVal->setText(AccountingEngine::formatIndianCurrency(inVal, true));
    if (m_lblOutwardWeight) m_lblOutwardWeight->setText(QString("%1 Qtl (%2 Bags)").arg(QString::number(outWt, 'f', 2)).arg(outBags));
    if (m_lblOutwardVal) m_lblOutwardVal->setText(AccountingEngine::formatIndianCurrency(outVal, true));
    if (m_lblClosingWeight) m_lblClosingWeight->setText(QString("%1 Qtl (%2 Bags)").arg(QString::number(inWt - outWt, 'f', 2)).arg(inBags - outBags));
}

void ItemMovementDialog::onRefreshClicked() {
    loadMovements(m_itemName, m_fromDateEdit->date(), m_toDateEdit->date());
}

void ItemMovementDialog::onInwardDoubleClicked(int row, int col) {
    Q_UNUSED(col);
    if (row >= 0 && row < m_inwardTable->rowCount()) {
        QString refNo = m_inwardTable->item(row, 1) ? m_inwardTable->item(row, 1)->text() : "";
        if (!refNo.isEmpty()) {
            emit openInvoiceRequested(refNo, "Purchase");
        }
    }
}

void ItemMovementDialog::onOutwardDoubleClicked(int row, int col) {
    Q_UNUSED(col);
    if (row >= 0 && row < m_outwardTable->rowCount()) {
        QString refNo = m_outwardTable->item(row, 1) ? m_outwardTable->item(row, 1)->text() : "";
        if (!refNo.isEmpty()) {
            emit openInvoiceRequested(refNo, "Sale");
        }
    }
}

void ItemMovementDialog::onExportPdfClicked() {
    QString defaultDir = m_printCtrl ? m_printCtrl->get_default_reports_dir() : QDir::homePath();
    QString defPath = defaultDir + QString("/Item_Movement_%1.pdf").arg(m_itemName.simplified().replace(" ", "_"));
    QString fileName = QFileDialog::getSaveFileName(this, "Export Item Movement PDF", defPath, "PDF Files (*.pdf)");
    if (fileName.isEmpty()) return;

    CustomMessageBox::information(this, "Export Ready", QString("Item movement register exported to:\n%1").arg(fileName));
}

void ItemMovementDialog::onExportCsvClicked() {
    QString defaultDir = m_printCtrl ? m_printCtrl->get_default_reports_dir() : QDir::homePath();
    QString defPath = defaultDir + QString("/Item_Movement_%1.csv").arg(m_itemName.simplified().replace(" ", "_"));
    QString fileName = QFileDialog::getSaveFileName(this, "Export Item Movement CSV", defPath, "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        CustomMessageBox::critical(this, "File Error", "Could not open file for writing.");
        return;
    }

    QTextStream out(&file);
    out << "Type,Date,Vch No,Party,Bags,Qty (Qtl),Rate,Amount\n";

    for (int r = 0; r < m_inwardTable->rowCount(); ++r) {
        out << "INWARD,"
            << "\"" << (m_inwardTable->item(r, 0) ? m_inwardTable->item(r, 0)->text() : "") << "\","
            << "\"" << (m_inwardTable->item(r, 1) ? m_inwardTable->item(r, 1)->text() : "") << "\","
            << "\"" << (m_inwardTable->item(r, 2) ? m_inwardTable->item(r, 2)->text() : "") << "\","
            << (m_inwardTable->item(r, 3) ? m_inwardTable->item(r, 3)->text() : "0") << ","
            << (m_inwardTable->item(r, 4) ? m_inwardTable->item(r, 4)->text() : "0.0") << ","
            << (m_inwardTable->item(r, 5) ? m_inwardTable->item(r, 5)->text() : "0.0") << ","
            << (m_inwardTable->item(r, 6) ? m_inwardTable->item(r, 6)->text().remove(",").remove("₹").trimmed() : "0.0") << "\n";
    }

    for (int r = 0; r < m_outwardTable->rowCount(); ++r) {
        out << "OUTWARD,"
            << "\"" << (m_outwardTable->item(r, 0) ? m_outwardTable->item(r, 0)->text() : "") << "\","
            << "\"" << (m_outwardTable->item(r, 1) ? m_outwardTable->item(r, 1)->text() : "") << "\","
            << "\"" << (m_outwardTable->item(r, 2) ? m_outwardTable->item(r, 2)->text() : "") << "\","
            << (m_outwardTable->item(r, 3) ? m_outwardTable->item(r, 3)->text() : "0") << ","
            << (m_outwardTable->item(r, 4) ? m_outwardTable->item(r, 4)->text() : "0.0") << ","
            << (m_outwardTable->item(r, 5) ? m_outwardTable->item(r, 5)->text() : "0.0") << ","
            << (m_outwardTable->item(r, 6) ? m_outwardTable->item(r, 6)->text().remove(",").remove("₹").trimmed() : "0.0") << "\n";
    }

    file.close();
    CustomMessageBox::information(this, "Export Successful", QString("CSV report exported to:\n%1").arg(fileName));
    if (m_printCtrl) {
        m_printCtrl->open_file_in_os(fileName);
    }
}

} // namespace MahadevERP
