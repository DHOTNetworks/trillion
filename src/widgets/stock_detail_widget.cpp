#include "stock_detail_widget.h"
#include "item_movement_dialog.h"
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

namespace MahadevERP {

StockDetailWidget::StockDetailWidget(StockRegisterController* controller, PrintExportController* printExportCtrl, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_printExportCtrl(printExportCtrl)
{
    setupUi();
    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    QDate sDate = QDate::fromString(activeFy.startDate, "yyyy-MM-dd");
    QDate eDate = QDate::fromString(activeFy.endDate, "yyyy-MM-dd");
    if (!sDate.isValid()) sDate = QDate(QDate::currentDate().month() < 4 ? QDate::currentDate().year() - 1 : QDate::currentDate().year(), 4, 1);
    if (!eDate.isValid()) eDate = QDate::currentDate();
    m_fromDateEdit->setDate(sDate);
    m_toDateEdit->setDate(eDate);
    reloadData(sDate.toString("yyyy-MM-dd"), eDate.toString("yyyy-MM-dd"));
}

void StockDetailWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // 1. Header Bar
    auto* headerLayout = new QHBoxLayout();
    auto* titleCol = new QVBoxLayout();
    auto* titleLabel = new QLabel("Comprehensive Stock Detail & Inventory Register", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #0F172A;");
    auto* subLabel = new QLabel("Real-time physical inventory ledger with opening, inward, outward, and closing valuation • Double click row for Item Movement Audit", this);
    subLabel->setStyleSheet("font-size: 11px; color: #64748B;");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();

    auto* printBtn = new KbdBadgeButton("Print Register", "Ctrl+P", this);
    printBtn->setPrimaryColor("#059669", "#047857");
    printBtn->setTextColor("#FFFFFF");
    connect(printBtn, &QPushButton::clicked, this, &StockDetailWidget::onPrintRegister);
    headerLayout->addWidget(printBtn);

    auto* pdfBtn = new KbdBadgeButton("Export PDF", "Alt+P", this);
    pdfBtn->setPrimaryColor("#0284C7", "#0369A1");
    pdfBtn->setTextColor("#FFFFFF");
    connect(pdfBtn, &QPushButton::clicked, this, &StockDetailWidget::onExportPdf);
    headerLayout->addWidget(pdfBtn);

    auto* csvBtn = new QPushButton("Export CSV", this);
    csvBtn->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 6px; padding: 6px 12px; font-weight: bold; color: #334155;");
    connect(csvBtn, &QPushButton::clicked, this, &StockDetailWidget::onExportCsv);
    headerLayout->addWidget(csvBtn);

    auto* backBtn = new KbdBadgeButton("← Back", "Esc", this);
    backBtn->setPrimaryColor("#F1F5F9", "#E2E8F0");
    backBtn->setTextColor("#475569");
    connect(backBtn, &QPushButton::clicked, this, &StockDetailWidget::backRequested);
    headerLayout->addWidget(backBtn);
    mainLayout->addLayout(headerLayout);

    // ========================================================================
    // TIER 2: FILTER & DATE RANGE BAR
    // ========================================================================
    auto* filterCard = new QFrame(this);
    filterCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; }"
        "QLabel { color: #475569; font-weight: 700; font-size: 11.5px; border: none; background: transparent; }"
        "QDateEdit, QLineEdit { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "QDateEdit:focus, QLineEdit:focus { border-color: #2563EB; background-color: #F8FAFC; }"
    );
    auto* filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(12, 8, 12, 8);
    filterLayout->setSpacing(12);

    filterLayout->addWidget(new QLabel("From Date:", filterCard));
    FiscalYearInfo initFy = FiscalYearHelper::getActiveFiscalYear();
    QDate initSDate = QDate::fromString(initFy.startDate, "yyyy-MM-dd");
    if (!initSDate.isValid()) initSDate = QDate(QDate::currentDate().month() < 4 ? QDate::currentDate().year() - 1 : QDate::currentDate().year(), 4, 1);
    m_fromDateEdit = new QDateEdit(initSDate, filterCard);
    m_fromDateEdit->setDisplayFormat("dd-MM-yyyy");
    m_fromDateEdit->setCalendarPopup(true);
    connect(m_fromDateEdit, &QDateEdit::dateChanged, this, &StockDetailWidget::onDateFilterChanged);
    filterLayout->addWidget(m_fromDateEdit);

    filterLayout->addWidget(new QLabel("To Date:", filterCard));
    m_toDateEdit = new QDateEdit(QDate::currentDate(), filterCard);
    m_toDateEdit->setDisplayFormat("dd-MM-yyyy");
    m_toDateEdit->setCalendarPopup(true);
    connect(m_toDateEdit, &QDateEdit::dateChanged, this, &StockDetailWidget::onDateFilterChanged);
    filterLayout->addWidget(m_toDateEdit);

    filterLayout->addSpacing(16);

    filterLayout->addWidget(new QLabel("Search Item:", filterCard));
    m_searchEdit = new QLineEdit(filterCard);
    m_searchEdit->setPlaceholderText("Search commodity, type, code...");
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &StockDetailWidget::onSearchChanged);
    filterLayout->addWidget(m_searchEdit, 1);

    mainLayout->addWidget(filterCard);

    // ========================================================================
    // TIER 3: HIGH-PERFORMANCE DATA SURFACE (TABLE)
    // ========================================================================
    m_table = new QTableWidget(this);
    m_table->setColumnCount(13);
    m_table->setHorizontalHeaderLabels({
        "Stock Item Name", "Code", "Type", "Unit",
        "Op Bags", "Op Qtl", "Inward Qtl", "Outward Qtl", "Close Bags", "Close Qtl",
        "Valuation Rate (₹)", "Closing Value (₹)", "Action"
    });
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    for (int c = 4; c < 13; ++c) {
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    }
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(
        "QTableWidget { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; gridline-color: #F1F5F9; font-size: 12px; color: #0F172A; }"
        "QTableWidget::item { padding: 6px 10px; }"
        "QTableWidget::item:selected { background-color: #EFF6FF; color: #1E3A8A; font-weight: bold; }"
        "QHeaderView::section { background-color: #0F172A; color: #FFFFFF; font-weight: 800; font-size: 11px; padding: 6px 10px; border: none; }"
    );
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &StockDetailWidget::onTableDoubleClicked);
    mainLayout->addWidget(m_table, 1);

    // ========================================================================
    // TIER 4: SUMMARY METRICS FOOTER CARDS
    // ========================================================================
    auto* metricsLayout = new QHBoxLayout();
    metricsLayout->setSpacing(10);

    auto createMetricCard = [this, metricsLayout](const QString& title, const QString& initVal, const QString& color) {
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
        vLabel->setStyleSheet("font-size: 14px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
        cLayout->addWidget(tLabel);
        cLayout->addWidget(vLabel);
        metricsLayout->addWidget(card);
        return vLabel;
    };

    m_totalItemsLabel = createMetricCard("TOTAL COMMODITIES / ITEMS", "0", "#2563EB");
    m_totalClosingQtyLabel = createMetricCard("TOTAL CLOSING STOCK (QTL)", "0.00", "#0284C7");
    m_totalClosingValLabel = createMetricCard("TOTAL CLOSING VALUATION", "₹ 0.00", "#16A34A");

    mainLayout->addLayout(metricsLayout);

    // Keyboard Shortcuts
    connect(new QShortcut(QKeySequence(Qt::Key_Escape), this), &QShortcut::activated, this, &StockDetailWidget::backRequested);
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_P), this, SLOT(onPrintRegister()));
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_P), this, SLOT(onExportPdf()));
}

void StockDetailWidget::reloadData(const QString& fromDate, const QString& toDate) {
    if (!fromDate.isEmpty()) m_fromDateEdit->setDate(QDate::fromString(fromDate, "yyyy-MM-dd"));
    if (!toDate.isEmpty()) m_toDateEdit->setDate(QDate::fromString(toDate, "yyyy-MM-dd"));

    if (m_controller) {
        m_controller->reload(m_fromDateEdit->date().toString("yyyy-MM-dd"), m_toDateEdit->date().toString("yyyy-MM-dd"));
    }

    populateTable();
}

void StockDetailWidget::populateTable() {
    m_table->setRowCount(0);

    if (m_controller) {
        auto* mdl = m_controller->model();
        int count = mdl ? mdl->rowCount() : 0;
        QString q = m_searchEdit->text();

        int row = 0;
        double totCloseQty = 0.0;
        double totCloseVal = 0.0;

        for (int i = 0; i < count; ++i) {
            auto map = mdl->get(i);
            QString name = map.value("name").toString();
            QString code = map.value("code").toString();
            QString type = map.value("type").toString();
            QString unit = map.value("unit", "Qtl").toString();

            if (!q.isEmpty()) {
                QString searchTarget = (name + " " + code + " " + type).toLower();
                if (!searchTarget.contains(q.toLower())) continue;
            }

            long long opBags = map.value("opBags").toLongLong();
            double opQty = map.value("opQty").toDouble();
            double inQty = map.value("inQty").toDouble();
            double outQty = map.value("outQty").toDouble();
            long long closeBags = map.value("closeBags").toLongLong();
            double closeQty = map.value("closeQty").toDouble();
            double rate = map.value("rate").toDouble();
            double closeVal = map.value("closeVal").toDouble();

            totCloseQty += closeQty;
            totCloseVal += closeVal;

            m_table->insertRow(row);

            auto* itemName = new QTableWidgetItem(name);
            itemName->setFont(QFont("Segoe UI", 9, QFont::Bold));
            itemName->setForeground(QBrush(QColor("#0F172A")));
            m_table->setItem(row, 0, itemName);

            m_table->setItem(row, 1, new QTableWidgetItem(code));
            m_table->setItem(row, 2, new QTableWidgetItem(type));
            m_table->setItem(row, 3, new QTableWidgetItem(unit));

            auto* itemOpBags = new QTableWidgetItem(QString::number(opBags));
            itemOpBags->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_table->setItem(row, 4, itemOpBags);

            auto* itemOpQty = new QTableWidgetItem(QString::number(opQty, 'f', 2));
            itemOpQty->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_table->setItem(row, 5, itemOpQty);

            auto* itemInQty = new QTableWidgetItem(QString::number(inQty, 'f', 2));
            itemInQty->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            itemInQty->setForeground(QBrush(QColor("#16A34A")));
            m_table->setItem(row, 6, itemInQty);

            auto* itemOutQty = new QTableWidgetItem(QString::number(outQty, 'f', 2));
            itemOutQty->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            itemOutQty->setForeground(QBrush(QColor("#DC2626")));
            m_table->setItem(row, 7, itemOutQty);

            auto* itemCloseBags = new QTableWidgetItem(QString::number(closeBags));
            itemCloseBags->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            itemCloseBags->setFont(QFont("Segoe UI", 9, QFont::Bold));
            m_table->setItem(row, 8, itemCloseBags);

            auto* itemCloseQty = new QTableWidgetItem(QString::number(closeQty, 'f', 2));
            itemCloseQty->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            itemCloseQty->setFont(QFont("Segoe UI", 9, QFont::Bold));
            itemCloseQty->setForeground(QBrush(QColor("#1D4ED8")));
            m_table->setItem(row, 9, itemCloseQty);

            auto* itemRate = new QTableWidgetItem(QString::number(rate, 'f', 2));
            itemRate->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_table->setItem(row, 10, itemRate);

            auto* itemCloseVal = new QTableWidgetItem(QString::number(closeVal, 'f', 2));
            itemCloseVal->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            itemCloseVal->setFont(QFont("Segoe UI", 9, QFont::Bold));
            itemCloseVal->setForeground(QBrush(QColor("#15803D")));
            m_table->setItem(row, 11, itemCloseVal);

            auto* itemAction = new QTableWidgetItem("Audit Movement 🔍");
            itemAction->setFont(QFont("Segoe UI", 8, QFont::Bold));
            itemAction->setForeground(QBrush(QColor("#2563EB")));
            m_table->setItem(row, 12, itemAction);

            row++;
        }

        m_totalItemsLabel->setText(QString::number(row));
        m_totalClosingQtyLabel->setText(QString::number(totCloseQty, 'f', 2));
        m_totalClosingValLabel->setText("₹ " + QString::number(totCloseVal, 'f', 2));
    }
}

void StockDetailWidget::onSearchChanged(const QString& query) {
    Q_UNUSED(query);
    populateTable();
}

void StockDetailWidget::onDateFilterChanged() {
    reloadData(m_fromDateEdit->date().toString("yyyy-MM-dd"), m_toDateEdit->date().toString("yyyy-MM-dd"));
}

void StockDetailWidget::focusTable() {
    if (m_table->rowCount() > 0) {
        m_table->setFocus();
        m_table->selectRow(0);
    } else {
        m_searchEdit->setFocus();
    }
}

void StockDetailWidget::onTableDoubleClicked(int row, int col) {
    Q_UNUSED(col);
    if (row < 0 || row >= m_table->rowCount()) return;

    auto* item = m_table->item(row, 0);
    if (!item) return;

    QString itemName = item->text();
    ItemMovementDialog dlg(itemName, m_fromDateEdit->date(), m_toDateEdit->date(), m_printExportCtrl, this);
    dlg.exec();
}

void StockDetailWidget::onPrintRegister() {
    if (m_printExportCtrl) {
        m_printExportCtrl->print_stock_register();
    }
}

void StockDetailWidget::onExportPdf() {
    if (m_printExportCtrl) {
        m_printExportCtrl->export_stock_register_pdf();
    }
}

void StockDetailWidget::onExportCsv() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Stock Register CSV", "StockRegister.csv", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        CustomMessageBox::showCritical(this, "File Error", "Could not open file for writing.");
        return;
    }

    QTextStream out(&file);
    out << "Item Name,Code,Type,Unit,Op Bags,Op Qtl,Inward Qtl,Outward Qtl,Close Bags,Close Qtl,Rate,Closing Value\n";

    for (int r = 0; r < m_table->rowCount(); ++r) {
        QStringList rowVals;
        for (int c = 0; c < 12; ++c) {
            auto* it = m_table->item(r, c);
            QString txt = it ? it->text() : "";
            txt.replace("\"", "\"\"");
            rowVals.append("\"" + txt + "\"");
        }
        out << rowVals.join(",") << "\n";
    }

    file.close();
    CustomMessageBox::showInformation(this, "Export Complete", "Stock Register exported to " + fileName);
}

} // namespace MahadevERP
