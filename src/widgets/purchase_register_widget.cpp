#include "purchase_register_widget.h"
#include "print_copy_options_dialog.h"
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

PurchaseRegisterWidget::PurchaseRegisterWidget(PurchaseRegisterController* controller, PrintExportController* printExportCtrl, QWidget* parent)
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
    m_fromDateEdit->blockSignals(true);
    m_toDateEdit->blockSignals(true);
    m_fromDateEdit->setDate(sDate);
    m_toDateEdit->setDate(eDate);
    m_fromDateEdit->blockSignals(false);
    m_toDateEdit->blockSignals(false);
    loadData(sDate, eDate);
}

void PurchaseRegisterWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 12, 14, 12);
    mainLayout->setSpacing(10);

    // ========================================================================
    // TIER 1: STICKY HEADER CARD
    // ========================================================================
    auto* headerCard = new QFrame(this);
    headerCard->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }");
    auto* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 10, 14, 10);

    auto* titleCol = new QVBoxLayout();
    titleCol->setSpacing(2);
    auto* titleLabel = new QLabel("Purchase Bills Register (Inward Raw & Goods Register)", headerCard);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
    auto* subLabel = new QLabel("Complete audit register of supplier bills, paddy & packaging purchases, and ITC tax ledger.", headerCard);
    subLabel->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();

    auto* newBillBtn = new KbdBadgeButton("+ New Purchase Bill", "F9", headerCard);
    newBillBtn->setPrimaryColor("#16A34A", "#15803D");
    newBillBtn->setTextColor("#FFFFFF");
    connect(newBillBtn, &QPushButton::clicked, this, &PurchaseRegisterWidget::newBillRequested);
    headerLayout->addWidget(newBillBtn);

    auto* printBtn = new KbdBadgeButton("Print Bill", "Ctrl+P", headerCard);
    printBtn->setPrimaryColor("#059669", "#047857");
    printBtn->setTextColor("#FFFFFF");
    connect(printBtn, &QPushButton::clicked, this, &PurchaseRegisterWidget::onPrintBill);
    headerLayout->addWidget(printBtn);

    auto* pdfBtn = new KbdBadgeButton("Save PDF", "Alt+P", headerCard);
    pdfBtn->setPrimaryColor("#0284C7", "#0369A1");
    pdfBtn->setTextColor("#FFFFFF");
    connect(pdfBtn, &QPushButton::clicked, this, &PurchaseRegisterWidget::onExportPdf);
    headerLayout->addWidget(pdfBtn);

    auto* csvBtn = new QPushButton("Export CSV", headerCard);
    csvBtn->setCursor(Qt::PointingHandCursor);
    csvBtn->setStyleSheet("QPushButton { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 6px 12px; font-weight: 800; font-size: 11.5px; color: #334155; } QPushButton:hover { background-color: #F8FAFC; }");
    connect(csvBtn, &QPushButton::clicked, this, &PurchaseRegisterWidget::onExportCsv);
    headerLayout->addWidget(csvBtn);

    auto* backBtn = new KbdBadgeButton("← Back", "Esc", headerCard);
    backBtn->setPrimaryColor("#F1F5F9", "#E2E8F0");
    backBtn->setTextColor("#475569");
    connect(backBtn, &QPushButton::clicked, this, &PurchaseRegisterWidget::backRequested);
    headerLayout->addWidget(backBtn);

    mainLayout->addWidget(headerCard);

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
    connect(m_fromDateEdit, &QDateEdit::dateChanged, this, &PurchaseRegisterWidget::onDateFilterChanged);
    filterLayout->addWidget(m_fromDateEdit);

    filterLayout->addWidget(new QLabel("To Date:", filterCard));
    m_toDateEdit = new QDateEdit(QDate::currentDate(), filterCard);
    m_toDateEdit->setDisplayFormat("dd-MM-yyyy");
    m_toDateEdit->setCalendarPopup(true);
    connect(m_toDateEdit, &QDateEdit::dateChanged, this, &PurchaseRegisterWidget::onDateFilterChanged);
    filterLayout->addWidget(m_toDateEdit);

    filterLayout->addSpacing(16);

    filterLayout->addWidget(new QLabel("Search Query:", filterCard));
    m_searchEdit = new QLineEdit(filterCard);
    m_searchEdit->setPlaceholderText("Search bill no, supplier, item, vehicle...");
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &PurchaseRegisterWidget::onSearchChanged);
    filterLayout->addWidget(m_searchEdit, 1);

    mainLayout->addWidget(filterCard);

    // ========================================================================
    // TIER 3: HIGH-PERFORMANCE DATA SURFACE (TABLE)
    // ========================================================================
    m_table = new QTableWidget(this);
    m_table->setColumnCount(11);
    m_table->setHorizontalHeaderLabels({
        "Bill No", "Date", "Supplier / Farmer", "Item / Commodity",
        "Bags", "Weight (Qtl)", "Rate (₹)", "Taxable (₹)", "GST (₹)", "Total (₹)", "Vehicle No"
    });
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(8, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(9, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(10, QHeaderView::ResizeToContents);
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
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &PurchaseRegisterWidget::onTableDoubleClicked);
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

    m_totalBillsLabel = createMetricCard("TOTAL BILLS", "0", "#2563EB");
    m_totalBagsLabel = createMetricCard("TOTAL BAGS", "0", "#0284C7");
    m_totalWeightLabel = createMetricCard("TOTAL WEIGHT (QTL)", "0.00", "#7C3AED");
    m_totalTaxableLabel = createMetricCard("TAXABLE AMOUNT", "₹ 0.00", "#D97706");
    m_totalGstLabel = createMetricCard("INPUT GST (ITC)", "₹ 0.00", "#059669");
    m_totalGrossLabel = createMetricCard("GROSS PURCHASE", "₹ 0.00", "#16A34A");

    mainLayout->addLayout(metricsLayout);

    // Keyboard Shortcuts
    connect(new QShortcut(QKeySequence(Qt::Key_Escape), this), &QShortcut::activated, this, &PurchaseRegisterWidget::backRequested);
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_P), this, SLOT(onPrintBill()));
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_P), this, SLOT(onExportPdf()));
}

void PurchaseRegisterWidget::loadData(const QDate& fromDate, const QDate& toDate) {
    if (fromDate.isValid()) m_fromDateEdit->setDate(fromDate);
    if (toDate.isValid()) m_toDateEdit->setDate(toDate);

    if (m_controller) {
        m_controller->model()->clear();
    }

    populateTable();
}

void PurchaseRegisterWidget::populateTable() {
    m_table->setRowCount(0);

    QString sDate = m_fromDateEdit->date().toString("yyyy-MM-dd");
    QString eDate = m_toDateEdit->date().toString("yyyy-MM-dd");
    QString q = m_searchEdit->text();

    QString sql = "SELECT id, voucher_no, invoice_no, invoice_date, supplier_name, item_name, "
                  "bag_count AS total_bags, weight_qtl AS total_weight_qtl, rate_per_qtl, "
                  "taxable_amount, gst_amount, total_amount AS grand_total, vehicle_no "
                  "FROM purchase_invoices WHERE invoice_date >= '" + sDate + "' AND invoice_date <= '" + eDate + "' "
                  "ORDER BY invoice_date DESC, invoice_no DESC;";
    QVariantList rows = DatabaseManager::instance().executeQuery(sql);

    int row = 0;
    int totBills = 0;
    long long totBags = 0;
    double totWeight = 0.0;
    double totTaxable = 0.0;
    double totGst = 0.0;
    double totGross = 0.0;

    for (const QVariant& itemValRow : rows) {
        QVariantMap rowMap = itemValRow.toMap();
        QString invNo = rowMap.value("invoice_no").toString();
        QString vchNo = rowMap.value("voucher_no").toString();
        QString dateVal = rowMap.value("invoice_date").toString();
        QString suppVal = rowMap.value("supplier_name").toString();
        QString itemVal = rowMap.value("item_name").toString();
        long long bags = rowMap.value("total_bags").toLongLong();
        double weight = rowMap.value("total_weight_qtl").toDouble();
        double rate = rowMap.value("rate_per_qtl").toDouble();
        double taxable = rowMap.value("taxable_amount").toDouble();
        double gst = rowMap.value("gst_amount").toDouble();
        double gross = rowMap.value("grand_total").toDouble();
        QString veh = rowMap.value("vehicle_no").toString();
        int id = rowMap.value("id").toInt();

        if (!q.isEmpty()) {
            QString searchTarget = (invNo + " " + vchNo + " " + suppVal + " " + itemVal + " " + veh).toLower();
            if (!searchTarget.contains(q.toLower())) continue;
        }

        totBills++;
        totBags += bags;
        totWeight += weight;
        totTaxable += taxable;
        totGst += gst;
        totGross += gross;

        m_table->insertRow(row);
        auto* itemInv = new QTableWidgetItem(invNo.isEmpty() ? vchNo : invNo);
        itemInv->setData(Qt::UserRole, id);
        itemInv->setData(Qt::UserRole + 1, vchNo);
        itemInv->setData(Qt::UserRole + 2, dateVal);
        itemInv->setFont(QFont("Segoe UI", 9, QFont::Bold));
        itemInv->setForeground(QBrush(QColor("#1D4ED8")));
        m_table->setItem(row, 0, itemInv);

        m_table->setItem(row, 1, new QTableWidgetItem(FiscalYearHelper::formatDisplayDate(dateVal)));
        m_table->setItem(row, 2, new QTableWidgetItem(suppVal));
        m_table->setItem(row, 3, new QTableWidgetItem(itemVal));

        auto* itemBags = new QTableWidgetItem(QString::number(bags));
        itemBags->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 4, itemBags);

        auto* itemWt = new QTableWidgetItem(QString::number(weight, 'f', 2));
        itemWt->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 5, itemWt);

        auto* itemRate = new QTableWidgetItem(QString::number(rate, 'f', 2));
        itemRate->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 6, itemRate);

        auto* itemTax = new QTableWidgetItem(QString::number(taxable, 'f', 2));
        itemTax->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 7, itemTax);

        auto* itemGst = new QTableWidgetItem(QString::number(gst, 'f', 2));
        itemGst->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 8, itemGst);

        auto* itemTot = new QTableWidgetItem(QString::number(gross, 'f', 2));
        itemTot->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        itemTot->setFont(QFont("Segoe UI", 9, QFont::Bold));
        itemTot->setForeground(QBrush(QColor("#15803D")));
        m_table->setItem(row, 9, itemTot);

        m_table->setItem(row, 10, new QTableWidgetItem(veh));
        row++;
    }

    m_totalBillsLabel->setText(QString::number(totBills));
    m_totalBagsLabel->setText(QString::number(totBags));
    m_totalWeightLabel->setText(QString::number(totWeight, 'f', 2));
    m_totalTaxableLabel->setText("₹ " + QString::number(totTaxable, 'f', 2));
    m_totalGstLabel->setText("₹ " + QString::number(totGst, 'f', 2));
    m_totalGrossLabel->setText("₹ " + QString::number(totGross, 'f', 2));
}

void PurchaseRegisterWidget::onSearchChanged(const QString& query) {
    Q_UNUSED(query);
    populateTable();
}

void PurchaseRegisterWidget::onDateFilterChanged() {
    populateTable();
}

void PurchaseRegisterWidget::focusTable() {
    if (m_table->rowCount() > 0) {
        m_table->setFocus();
        m_table->selectRow(0);
    } else {
        m_searchEdit->setFocus();
    }
}

void PurchaseRegisterWidget::onTableDoubleClicked(int row, int col) {
    Q_UNUSED(col);
    if (row < 0 || row >= m_table->rowCount()) return;

    auto* item = m_table->item(row, 0);
    if (!item) return;

    QString invNo = item->text();
    int id = item->data(Qt::UserRole).toInt();
    QString vchNo = item->data(Qt::UserRole + 1).toString();
    QString dateVal = item->data(Qt::UserRole + 2).toString();

    QVariantMap map;
    map["invoiceNo"] = invNo;
    map["voucherNo"] = vchNo;
    map["id"] = id;
    map["date"] = dateVal;

    emit alterBillRequested(15, map);
}

void PurchaseRegisterWidget::onPrintBill() {
    int row = m_table->currentRow();
    if (row < 0 || row >= m_table->rowCount()) {
        CustomMessageBox::showWarning(this, "Select Bill", "Please select a purchase bill from the table to print.");
        return;
    }

    QString invNo = m_table->item(row, 0)->text();
    QString supp = m_table->item(row, 2)->text();

    QString copyType = PrintCopyOptionsDialog::selectCopyType(PrintCopyOptionsDialog::Mode::Print, invNo, supp, this);
    if (!copyType.isEmpty() && m_printExportCtrl) {
        m_printExportCtrl->print_purchase_invoice(invNo);
    }
}

void PurchaseRegisterWidget::onExportPdf() {
    int row = m_table->currentRow();
    if (row < 0 || row >= m_table->rowCount()) {
        CustomMessageBox::showWarning(this, "Select Bill", "Please select a purchase bill from the table to export as PDF.");
        return;
    }

    QString invNo = m_table->item(row, 0)->text();
    QString supp = m_table->item(row, 2)->text();

    QString copyType = PrintCopyOptionsDialog::selectCopyType(PrintCopyOptionsDialog::Mode::Pdf, invNo, supp, this);
    if (!copyType.isEmpty() && m_printExportCtrl) {
        m_printExportCtrl->export_purchase_invoice_pdf(invNo, "");
    }
}

void PurchaseRegisterWidget::onExportCsv() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Purchase Register CSV", "PurchaseRegister.csv", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        CustomMessageBox::showCritical(this, "File Error", "Could not open file for writing.");
        return;
    }

    QTextStream out(&file);
    out << "Bill No,Date,Supplier,Commodity,Bags,Weight Qtl,Rate,Taxable,GST,Total,Vehicle No\n";

    for (int r = 0; r < m_table->rowCount(); ++r) {
        QStringList rowVals;
        for (int c = 0; c < m_table->columnCount(); ++c) {
            auto* it = m_table->item(r, c);
            QString txt = it ? it->text() : "";
            txt.replace("\"", "\"\"");
            rowVals.append("\"" + txt + "\"");
        }
        out << rowVals.join(",") << "\n";
    }

    file.close();
    CustomMessageBox::showInformation(this, "Export Complete", "Purchase Register exported to " + fileName);
}

} // namespace MahadevERP
