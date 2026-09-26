#include "day_book_widget.h"
#include "kbd_badge_button.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"
#include <QHeaderView>
#include <QKeyEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QFrame>
#include <QDate>
#include <cmath>

namespace MahadevERP {

DayBookWidget::DayBookWidget(PrintExportController* printExportCtrl, QWidget* parent)
    : QWidget(parent)
    , m_printExportCtrl(printExportCtrl)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);
    setupUi();
    applyCustomStyles();
}

void DayBookWidget::setupUi() {
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
    QLabel* iconBox = new QLabel("DB", headerCard);
    iconBox->setFixedSize(36, 36);
    iconBox->setAlignment(Qt::AlignCenter);
    iconBox->setStyleSheet("background-color: #EFF6FF; border: 1.5px solid #3B82F6; border-radius: 8px; color: #1D4ED8; font-size: 13px; font-weight: 800;");
    headerLayout->addWidget(iconBox);

    QVBoxLayout* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(1);
    QLabel* titleLabel = new QLabel("Day Book / Daily Transaction Audit Register", headerCard);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif; border: none; background: transparent;");
    QLabel* subtitleLabel = new QLabel("Chronological journal & voucher register across all sales, purchases, bank, cash, and mandi postings.", headerCard);
    subtitleLabel->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);
    headerLayout->addLayout(titleLayout);

    headerLayout->addStretch(1);

    m_refreshBtn = new KbdBadgeButton("Refresh Register", "F5", QColor("#2563EB"), QColor("#1D4ED8"), QColor("#FFFFFF"), QColor("#2563EB"), headerCard);
    connect(m_refreshBtn, &QPushButton::clicked, this, &DayBookWidget::onRefreshClicked);
    headerLayout->addWidget(m_refreshBtn);

    m_pdfBtn = new KbdBadgeButton("Export PDF", "Alt+P", QColor("#059669"), QColor("#047857"), QColor("#FFFFFF"), QColor("#059669"), headerCard);
    connect(m_pdfBtn, &QPushButton::clicked, this, &DayBookWidget::onExportPdfClicked);
    headerLayout->addWidget(m_pdfBtn);

    m_backBtn = new KbdBadgeButton("Back to Dashboard", "Esc", QColor("#EF4444"), QColor("#DC2626"), QColor("#FFFFFF"), QColor("#EF4444"), headerCard);
    connect(m_backBtn, &QPushButton::clicked, this, &DayBookWidget::backRequested);
    headerLayout->addWidget(m_backBtn);

    mainLayout->addWidget(headerCard);

    // ================= 2. FILTER & CONTROL BAR CARD =================
    QFrame* filterCard = new QFrame(this);
    filterCard->setFixedHeight(54);
    filterCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    QHBoxLayout* filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(12, 6, 12, 6);
    filterLayout->setSpacing(10);

    // From Date
    QLabel* fromLbl = new QLabel("From Date:", filterCard);
    fromLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155; border: none; background: transparent;");
    filterLayout->addWidget(fromLbl);

    m_fromDateEdit = new QDateEdit(filterCard);
    m_fromDateEdit->setCalendarPopup(true);
    m_fromDateEdit->setDisplayFormat("dd-MM-yyyy");
    m_fromDateEdit->setFixedHeight(34);
    m_fromDateEdit->setMinimumWidth(125);
    connect(m_fromDateEdit, &QDateEdit::dateChanged, this, &DayBookWidget::onDateChanged);
    filterLayout->addWidget(m_fromDateEdit);

    // To Date
    QLabel* toLbl = new QLabel("To Date:", filterCard);
    toLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155; border: none; background: transparent;");
    filterLayout->addWidget(toLbl);

    m_toDateEdit = new QDateEdit(filterCard);
    m_toDateEdit->setCalendarPopup(true);
    m_toDateEdit->setDisplayFormat("dd-MM-yyyy");
    m_toDateEdit->setFixedHeight(34);
    m_toDateEdit->setMinimumWidth(125);
    connect(m_toDateEdit, &QDateEdit::dateChanged, this, &DayBookWidget::onDateChanged);
    filterLayout->addWidget(m_toDateEdit);

    // Type Filter
    QLabel* typeLbl = new QLabel("Voucher Type:", filterCard);
    typeLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155; border: none; background: transparent;");
    filterLayout->addWidget(typeLbl);

    m_typeFilterCombo = new QComboBox(filterCard);
    m_typeFilterCombo->setFixedHeight(34);
    m_typeFilterCombo->setMinimumWidth(170);
    m_typeFilterCombo->addItems({"ALL Vouchers", "Sales (SL)", "Purchases (PU)", "Journal (JV)", "Bank / Cheque (BK)", "Cash Book (CB)", "J-Form (JFrm)", "I-Form (IFrm)", "Milling Production", "TDS Deductions"});
    connect(m_typeFilterCombo, &QComboBox::currentTextChanged, this, &DayBookWidget::onRefreshClicked);
    filterLayout->addWidget(m_typeFilterCombo);

    // Search Box
    m_searchBox = new QLineEdit(filterCard);
    m_searchBox->setPlaceholderText("Filter by Party, Narration, Opposing A/c...");
    m_searchBox->setFixedHeight(34);
    m_searchBox->setMinimumWidth(260);
    connect(m_searchBox, &QLineEdit::textChanged, this, &DayBookWidget::onRefreshClicked);
    filterLayout->addWidget(m_searchBox);

    filterLayout->addStretch(1);

    // Period Badge
    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    QLabel* fyBadge = new QLabel(activeFy.name + " (Active)", filterCard);
    fyBadge->setAlignment(Qt::AlignCenter);
    fyBadge->setFixedHeight(34);
    fyBadge->setStyleSheet("background-color: #F0FDF4; color: #16A34A; border: 1.5px solid #86EFAC; border-radius: 6px; padding: 0px 14px; font-weight: 800; font-size: 12px;");
    filterLayout->addWidget(fyBadge);

    mainLayout->addWidget(filterCard);

    // ================= 3. DAY BOOK TABLE =================
    m_table = new QTableWidget(this);
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({"Date", "Vch No", "Type", "Particulars / Account Name", "Opposing Account / Narration", "Debit (₹)", "Credit (₹)"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(28);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &DayBookWidget::onRowDoubleClicked);
    mainLayout->addWidget(m_table, 1);

    // ================= 4. SUMMARY FOOTER BAR CARD =================
    QFrame* footerCard = new QFrame(this);
    footerCard->setFixedHeight(44);
    footerCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px;");
    QHBoxLayout* footerLayout = new QHBoxLayout(footerCard);
    footerLayout->setContentsMargins(14, 4, 14, 4);
    footerLayout->setSpacing(14);

    m_rowCountLabel = new QLabel("Total Records: 0", footerCard);
    m_rowCountLabel->setStyleSheet("font-weight: 700; color: #475569; font-size: 12px; border: none; background: transparent;");
    footerLayout->addWidget(m_rowCountLabel);

    footerLayout->addStretch(1);

    m_totalDrLabel = new QLabel("Total Debit: ₹0.00", footerCard);
    m_totalDrLabel->setStyleSheet("font-weight: 800; color: #16A34A; font-size: 13px; border: none; background: transparent;");
    footerLayout->addWidget(m_totalDrLabel);

    footerLayout->addSpacing(20);

    m_totalCrLabel = new QLabel("Total Credit: ₹0.00", footerCard);
    m_totalCrLabel->setStyleSheet("font-weight: 800; color: #DC2626; font-size: 13px; border: none; background: transparent;");
    footerLayout->addWidget(m_totalCrLabel);

    mainLayout->addWidget(footerCard);

    // Initialize with active financial year dates
    QDate sDate = QDate::fromString(activeFy.startDate, "yyyy-MM-dd");
    QDate eDate = QDate::fromString(activeFy.endDate, "yyyy-MM-dd");
    if (!sDate.isValid()) sDate = QDate(QDate::currentDate().month() < 4 ? QDate::currentDate().year() - 1 : QDate::currentDate().year(), 4, 1);
    if (!eDate.isValid()) eDate = QDate::currentDate();
    m_fromDateEdit->setDate(sDate);
    m_toDateEdit->setDate(eDate);
}

void DayBookWidget::applyCustomStyles() {
    setStyleSheet(
        "DayBookWidget { background-color: #F8FAFC; }"
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

void DayBookWidget::loadDayBookData(const QDate& fromDate, const QDate& toDate, const QString& voucherTypeFilter) {
    if (m_fromDateEdit->date() != fromDate) m_fromDateEdit->setDate(fromDate);
    if (m_toDateEdit->date() != toDate) m_toDateEdit->setDate(toDate);

    QString fromStr = fromDate.toString("yyyy-MM-dd");
    QString toStr = toDate.toString("yyyy-MM-dd");

    // Robust query reading from transactions table containing all double-entry postings
    QString sql = QString(
        "SELECT "
        "  voucher_date AS date, "
        "  voucher_no, "
        "  voucher_type, "
        "  trans_type, "
        "  COALESCE(party_name, '') AS party_name, "
        "  COALESCE(opposing_account, '') AS opposing_account, "
        "  COALESCE(narration, '') AS narration, "
        "  CASE WHEN dr_cr = 'Dr' THEN amount ELSE 0.0 END AS debit, "
        "  CASE WHEN dr_cr = 'Cr' THEN amount ELSE 0.0 END AS credit "
        "FROM transactions "
        "WHERE voucher_date >= '%1' AND voucher_date <= '%2' "
        "ORDER BY voucher_date ASC, id ASC;"
    ).arg(fromStr, toStr);

    m_records = DatabaseManager::instance().executeQuery(sql);

    // Populate Table
    m_table->setRowCount(0);
    double totalDr = 0.0;
    double totalCr = 0.0;

    QString filterQuery = m_searchBox ? m_searchBox->text().trimmed().toLower() : "";
    QString selectedType = m_typeFilterCombo ? m_typeFilterCombo->currentText().toUpper() : "ALL";

    int row = 0;
    for (const auto& var : m_records) {
        QVariantMap r = var.toMap();
        QString vType = r.value("voucher_type").toString();
        QString tType = r.value("trans_type").toString().toUpper();
        QString party = r.value("party_name").toString();
        QString opp = r.value("opposing_account").toString();
        QString narr = r.value("narration").toString();
        double dr = r.value("debit").toDouble();
        double cr = r.value("credit").toDouble();

        // Voucher Type Filter
        if (selectedType.contains("SALES") && !vType.contains("Sales", Qt::CaseInsensitive) && tType != "SL") continue;
        if (selectedType.contains("PURCHASE") && !vType.contains("Purchase", Qt::CaseInsensitive) && tType != "PU") continue;
        if (selectedType.contains("JOURNAL") && !vType.contains("Journal", Qt::CaseInsensitive) && tType != "JRNL" && tType != "JV") continue;
        if (selectedType.contains("BANK") && !vType.contains("Payment", Qt::CaseInsensitive) && !vType.contains("Receipt", Qt::CaseInsensitive) && tType != "CHPT" && tType != "CHRT") continue;
        if (selectedType.contains("CASH") && !vType.contains("Cash", Qt::CaseInsensitive) && tType != "PYMT" && tType != "RCPT") continue;
        if (selectedType.contains("J-FORM") && !vType.contains("J-Form", Qt::CaseInsensitive) && tType != "JFRM") continue;
        if (selectedType.contains("I-FORM") && !vType.contains("I-Form", Qt::CaseInsensitive) && tType != "IFRM") continue;
        if (selectedType.contains("MILLING") && !vType.contains("Milling", Qt::CaseInsensitive) && tType != "MILL") continue;
        if (selectedType.contains("TDS") && !vType.contains("TDS", Qt::CaseInsensitive)) continue;

        // Search Query Filter
        if (!filterQuery.isEmpty()) {
            if (!party.toLower().contains(filterQuery) && !narr.toLower().contains(filterQuery) && !opp.toLower().contains(filterQuery)) {
                continue;
            }
        }

        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(r.value("date").toString()));
        m_table->setItem(row, 1, new QTableWidgetItem(r.value("voucher_no").toString()));
        m_table->setItem(row, 2, new QTableWidgetItem(vType));
        m_table->setItem(row, 3, new QTableWidgetItem(party));

        QString desc = opp.isEmpty() ? narr : (opp + (narr.isEmpty() ? "" : " - " + narr));
        m_table->setItem(row, 4, new QTableWidgetItem(desc));

        auto* drItem = new QTableWidgetItem(dr > 0.0 ? AccountingEngine::formatCurrency(dr) : "");
        drItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (dr > 0.0) drItem->setForeground(QColor("#16A34A"));
        m_table->setItem(row, 5, drItem);

        auto* crItem = new QTableWidgetItem(cr > 0.0 ? AccountingEngine::formatCurrency(cr) : "");
        crItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (cr > 0.0) crItem->setForeground(QColor("#DC2626"));
        m_table->setItem(row, 6, crItem);

        totalDr += dr;
        totalCr += cr;
        row++;
    }

    m_rowCountLabel->setText(QString("Total Records: %1").arg(row));
    m_totalDrLabel->setText(QString("Total Debit: %1").arg(AccountingEngine::formatIndianCurrency(totalDr)));
    m_totalCrLabel->setText(QString("Total Credit: %1").arg(AccountingEngine::formatIndianCurrency(totalCr)));
}

void DayBookWidget::onRefreshClicked() {
    loadDayBookData(m_fromDateEdit->date(), m_toDateEdit->date(), m_typeFilterCombo->currentText());
}

void DayBookWidget::onDateChanged() {
    onRefreshClicked();
}

void DayBookWidget::onRowDoubleClicked(int row, int /*column*/) {
    if (row < 0 || row >= m_records.size()) return;
    const QVariantMap entry = m_records[row].toMap();
    QString vType = entry.value("voucher_type").toString().toUpper();
    QString tType = entry.value("trans_type").toString().toUpper();

    int targetView = 0;
    if (vType.contains("SALE") || tType == "SL") targetView = 14;
    else if (vType.contains("PURCHASE") || tType == "PU") targetView = 15;
    else if (vType.contains("JOURNAL") || tType == "JRNL" || tType == "JV") targetView = 17;
    else if (tType == "PYMT" || tType == "RCPT" || vType == "CASH PAYMENT" || vType == "CASH RECEIPT") targetView = 60;
    else if (vType.contains("PAYMENT") || vType.contains("RECEIPT") || tType == "CHPT" || tType == "CHRT") targetView = 16;
    else if (vType.contains("J-FORM") || tType == "JFRM") targetView = 18;
    else if (vType.contains("I-FORM") || tType == "IFRM") targetView = 19;

    emit alterVoucherRequested(targetView, entry);
}

void DayBookWidget::onExportPdfClicked() {
    if (!m_printExportCtrl) return;
    QString fDate = m_fromDateEdit ? m_fromDateEdit->date().toString("yyyy-MM-dd") : "";
    QString tDate = m_toDateEdit ? m_toDateEdit->date().toString("yyyy-MM-dd") : "";
    m_printExportCtrl->export_day_book_pdf(fDate, tDate);
}

void DayBookWidget::onExportOdfClicked() {
    if (!m_printExportCtrl) return;
    QString fDate = m_fromDateEdit ? m_fromDateEdit->date().toString("yyyy-MM-dd") : "";
    QString tDate = m_toDateEdit ? m_toDateEdit->date().toString("yyyy-MM-dd") : "";
    m_printExportCtrl->export_day_book_odf(fDate, tDate);
}

void DayBookWidget::onExportExcelClicked() {
    if (!m_printExportCtrl) return;
    QString fDate = m_fromDateEdit ? m_fromDateEdit->date().toString("yyyy-MM-dd") : "";
    QString tDate = m_toDateEdit ? m_toDateEdit->date().toString("yyyy-MM-dd") : "";
    m_printExportCtrl->export_day_book_excel(fDate, tDate);
}

void DayBookWidget::onPrintClicked() {
    if (!m_printExportCtrl) return;
    QString fDate = m_fromDateEdit ? m_fromDateEdit->date().toString("yyyy-MM-dd") : "";
    QString tDate = m_toDateEdit ? m_toDateEdit->date().toString("yyyy-MM-dd") : "";
    m_printExportCtrl->print_day_book(fDate, tDate);
}

void DayBookWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        event->accept();
    } else if (event->key() == Qt::Key_F5) {
        onRefreshClicked();
        event->accept();
    } else if (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_P) {
        onPrintClicked();
        event->accept();
    } else if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_P) {
        onExportPdfClicked();
        event->accept();
    } else if (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_O) {
        onExportOdfClicked();
        event->accept();
    } else if (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_E) {
        onExportExcelClicked();
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

} // namespace MahadevERP
