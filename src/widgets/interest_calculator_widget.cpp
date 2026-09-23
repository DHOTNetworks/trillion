#include "interest_calculator_widget.h"
#include "kbd_badge_button.h"
#include "custom_dialogs.h"
#include "../engine/fiscal_year_helper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QGroupBox>
#include <QShortcut>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

namespace MahadevERP {

InterestCalculatorWidget::InterestCalculatorWidget(InterestModel* model,
                                                   PrintExportController* printExportCtrl,
                                                   QWidget* parent)
    : QWidget(parent)
    , m_model(model)
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
}

void InterestCalculatorWidget::setupUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(14, 12, 14, 12);
    rootLayout->setSpacing(10);

    // ========================================================================
    // TIER 1: STICKY HEADER CARD
    // ========================================================================
    auto* headerCard = new QFrame(this);
    headerCard->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }");
    auto* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 10, 14, 10);

    auto* titleCol = new QVBoxLayout();
    titleCol->setSpacing(2);
    auto* titleLabel = new QLabel("Daily Product (Aank) & Commercial Interest Calculator", headerCard);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
    auto* subLabel = new QLabel("Calculate interest on ledger running balances using Mandi Daily Product (Aank/Rokka) or standard 365-day simple interest.", headerCard);
    subLabel->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();

    auto* csvBtn = new QPushButton("Export CSV", headerCard);
    csvBtn->setCursor(Qt::PointingHandCursor);
    csvBtn->setStyleSheet("QPushButton { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 6px 12px; font-weight: 800; font-size: 11.5px; color: #334155; } QPushButton:hover { background-color: #F8FAFC; }");
    connect(csvBtn, &QPushButton::clicked, this, &InterestCalculatorWidget::onExportCsv);
    headerLayout->addWidget(csvBtn);

    auto* backBtn = new KbdBadgeButton("← Back to Dashboard", "Esc", headerCard);
    backBtn->setPrimaryColor("#F1F5F9", "#E2E8F0");
    backBtn->setTextColor("#475569");
    connect(backBtn, &QPushButton::clicked, this, &InterestCalculatorWidget::backRequested);
    headerLayout->addWidget(backBtn);
    rootLayout->addWidget(headerCard);

    // ========================================================================
    // TIER 2: FILTER & PARAMETER CONTROLS CARD
    // ========================================================================
    auto* paramCard = new QFrame(this);
    paramCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; }"
        "QLabel { color: #475569; font-weight: 700; font-size: 11.5px; border: none; background: transparent; }"
        "QDateEdit, QLineEdit, QComboBox { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "QDateEdit:focus, QLineEdit:focus, QComboBox:focus { border-color: #2563EB; background-color: #F8FAFC; }"
        "QCheckBox { color: #1E293B; font-weight: 700; font-size: 12px; border: none; background: transparent; }"
    );
    auto* paramGrid = new QGridLayout(paramCard);
    paramGrid->setContentsMargins(12, 10, 12, 10);
    paramGrid->setHorizontalSpacing(10);
    paramGrid->setVerticalSpacing(8);

    paramGrid->addWidget(new QLabel("Party / Ledger Account *:", paramCard), 0, 0);
    m_partySearch = new AccountSearchBox(paramCard);
    connect(m_partySearch, &AccountSearchBox::partySelected, this, &InterestCalculatorWidget::onPartySelected);
    paramGrid->addWidget(m_partySearch, 0, 1, 1, 3);

    FiscalYearInfo initFy = FiscalYearHelper::getActiveFiscalYear();
    QDate initSDate = QDate::fromString(initFy.startDate, "yyyy-MM-dd");
    if (!initSDate.isValid()) initSDate = QDate(QDate::currentDate().month() < 4 ? QDate::currentDate().year() - 1 : QDate::currentDate().year(), 4, 1);
    m_fromDateEdit = new QDateEdit(initSDate, paramCard);
    m_fromDateEdit->setDisplayFormat("dd-MM-yyyy");
    m_fromDateEdit->setCalendarPopup(true);
    paramGrid->addWidget(new QLabel("From Date:", paramCard), 0, 4);
    paramGrid->addWidget(m_fromDateEdit, 0, 5);

    m_toDateEdit = new QDateEdit(QDate::currentDate(), paramCard);
    m_toDateEdit->setDisplayFormat("dd-MM-yyyy");
    m_toDateEdit->setCalendarPopup(true);
    paramGrid->addWidget(new QLabel("To Date:", paramCard), 0, 6);
    paramGrid->addWidget(m_toDateEdit, 0, 7);

    m_drRateEdit = new QLineEdit("12.00", paramCard);
    m_drRateEdit->setPlaceholderText("12.00");
    paramGrid->addWidget(new QLabel("Debit Rate % (Dr):", paramCard), 1, 0);
    paramGrid->addWidget(m_drRateEdit, 1, 1);

    m_crRateEdit = new QLineEdit("12.00", paramCard);
    m_crRateEdit->setPlaceholderText("12.00");
    paramGrid->addWidget(new QLabel("Credit Rate % (Cr):", paramCard), 1, 2);
    paramGrid->addWidget(m_crRateEdit, 1, 3);

    m_divisorCombo = new QComboBox(paramCard);
    m_divisorCombo->addItems({"365 Days (Standard Annual)", "360 Days (Commercial 30-Day Month)", "36000 (Aank / Rokka Method)"});
    paramGrid->addWidget(new QLabel("Calculation Base:", paramCard), 1, 4);
    paramGrid->addWidget(m_divisorCombo, 1, 5);

    m_includeOpBalCheck = new QCheckBox("Include Opening Bal", paramCard);
    m_includeOpBalCheck->setChecked(true);
    paramGrid->addWidget(m_includeOpBalCheck, 1, 6);

    auto* calcBtn = new QPushButton("Calculate ⚡", paramCard);
    calcBtn->setCursor(Qt::PointingHandCursor);
    calcBtn->setStyleSheet("QPushButton { background-color: #2563EB; color: #FFFFFF; font-weight: 800; font-size: 12px; padding: 5px 14px; border-radius: 6px; border: none; } QPushButton:hover { background-color: #1D4ED8; }");
    connect(calcBtn, &QPushButton::clicked, this, &InterestCalculatorWidget::onCalculateClicked);
    paramGrid->addWidget(calcBtn, 1, 7);

    rootLayout->addWidget(paramCard);

    // ========================================================================
    // TIER 3: HIGH-PERFORMANCE DATA SURFACE (PRODUCT TABLE)
    // ========================================================================
    m_table = new QTableWidget(this);
    m_table->setColumnCount(8);
    m_table->setHorizontalHeaderLabels({
        "Date", "Particulars", "Ref / Vch No", "Debit Amount (₹)", "Credit Amount (₹)",
        "Running Balance (₹)", "Days", "Product (Aank)"
    });
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    for (int c = 3; c < 8; ++c) {
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
    rootLayout->addWidget(m_table, 1);

    // ========================================================================
    // TIER 4: SUMMARY METRICS FOOTER & ACTION BAR
    // ========================================================================
    auto* footerRow = new QHBoxLayout();
    footerRow->setSpacing(10);

    auto createMetric = [this](const QString& title, const QString& initVal, const QString& color) {
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
        return qMakePair(card, vLabel);
    };

    auto m1 = createMetric("TOTAL DEBIT PRODUCT (AANK)", "0.00", "#DC2626");
    m_totalDrProductLabel = m1.second;
    footerRow->addWidget(m1.first, 1);

    auto m2 = createMetric("TOTAL CREDIT PRODUCT (AANK)", "0.00", "#16A34A");
    m_totalCrProductLabel = m2.second;
    footerRow->addWidget(m2.first, 1);

    auto m3 = createMetric("NET INTEREST RECEIVABLE / PAYABLE", "₹ 0.00", "#2563EB");
    m_netInterestLabel = m3.second;
    footerRow->addWidget(m3.first, 1);

    m_postBtn = new KbdBadgeButton("Post Interest Voucher to Books", "Ctrl+S", this);
    m_postBtn->setPrimaryColor("#16A34A", "#15803D");
    m_postBtn->setTextColor("#FFFFFF");
    m_postBtn->setEnabled(false);
    connect(m_postBtn, &QPushButton::clicked, this, &InterestCalculatorWidget::onPostVoucherClicked);
    footerRow->addWidget(m_postBtn, 0);

    rootLayout->addLayout(footerRow);

    connect(new QShortcut(QKeySequence(Qt::Key_Escape), this), &QShortcut::activated, this, &InterestCalculatorWidget::backRequested);
}

void InterestCalculatorWidget::setParty(const QString& partyName) {
    m_partySearch->setText(partyName);
    calculateInterest();
}

void InterestCalculatorWidget::onPartySelected(const QString& partyName) {
    Q_UNUSED(partyName);
    calculateInterest();
}

void InterestCalculatorWidget::onCalculateClicked() {
    calculateInterest();
}

void InterestCalculatorWidget::calculateInterest() {
    QString party = m_partySearch->text().trimmed();
    if (party.isEmpty()) {
        return;
    }

    if (!m_model) return;

    QString fDate = m_fromDateEdit->date().toString("yyyy-MM-dd");
    QString tDate = m_toDateEdit->date().toString("yyyy-MM-dd");
    double crRate = m_crRateEdit->text().toDouble();
    double drRate = m_drRateEdit->text().toDouble();
    bool is360 = (m_divisorCombo->currentIndex() >= 1);
    int div = (m_divisorCombo->currentIndex() == 1) ? 360 : 365;
    bool incOp = m_includeOpBalCheck->isChecked();

    QVariantMap data = m_model->get_interest_data(party, fDate, tDate, crRate, drRate, is360, div, incOp);
    populateTable(data);
}

void InterestCalculatorWidget::populateTable(const QVariantMap& data) {
    m_table->setRowCount(0);

    QVariantList rows = data.value("rows").toList();
    for (int r = 0; r < rows.size(); ++r) {
        auto map = rows[r].toMap();
        m_table->insertRow(r);

        m_table->setItem(r, 0, new QTableWidgetItem(FiscalYearHelper::formatDisplayDate(map.value("date").toString())));
        m_table->setItem(r, 1, new QTableWidgetItem(map.value("particulars").toString()));
        m_table->setItem(r, 2, new QTableWidgetItem(map.value("vchNo").toString()));

        double dr = map.value("debit").toDouble();
        auto* drItem = new QTableWidgetItem(dr > 0.0 ? QString::number(dr, 'f', 2) : "");
        drItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, 3, drItem);

        double cr = map.value("credit").toDouble();
        auto* crItem = new QTableWidgetItem(cr > 0.0 ? QString::number(cr, 'f', 2) : "");
        crItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, 4, crItem);

        double bal = map.value("balance").toDouble();
        QString balSide = map.value("balanceSide", "Dr").toString();
        auto* balItem = new QTableWidgetItem(QString("%1 %2").arg(QString::number(std::abs(bal), 'f', 2), balSide));
        balItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        balItem->setFont(QFont("Segoe UI", 9, QFont::Bold));
        m_table->setItem(r, 5, balItem);

        auto* daysItem = new QTableWidgetItem(QString::number(map.value("days").toInt()));
        daysItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(r, 6, daysItem);

        double prod = map.value("product").toDouble();
        auto* prodItem = new QTableWidgetItem(QString::number(prod, 'f', 2));
        prodItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, 7, prodItem);
    }

    double totDrProd = data.value("totalDrProduct").toDouble();
    double totCrProd = data.value("totalCrProduct").toDouble();
    double netInterest = data.value("netInterest").toDouble();
    m_lastCalculatedInterest = std::abs(netInterest);
    m_isReceivable = (netInterest >= 0.0);

    m_totalDrProductLabel->setText(QString::number(totDrProd, 'f', 2));
    m_totalCrProductLabel->setText(QString::number(totCrProd, 'f', 2));

    QString sign = m_isReceivable ? " (Receivable / Dr)" : " (Payable / Cr)";
    m_netInterestLabel->setText("₹ " + QString::number(m_lastCalculatedInterest, 'f', 2) + sign);

    m_postBtn->setEnabled(m_lastCalculatedInterest > 0.01);
}

void InterestCalculatorWidget::onPostVoucherClicked() {
    QString party = m_partySearch->text().trimmed();
    if (party.isEmpty() || m_lastCalculatedInterest <= 0.01) return;

    QString msg = QString("Are you sure you want to post Interest Journal Voucher of ₹ %1 (%2) to account '%3'?")
        .arg(QString::number(m_lastCalculatedInterest, 'f', 2), m_isReceivable ? "Receivable" : "Payable", party);

    if (!CustomMessageBox::showConfirmation(this, "Confirm Voucher Post", msg)) {
        return;
    }

    bool ok = false;
    if (m_model) {
        ok = m_model->post_interest_voucher(
            party,
            m_toDateEdit->date().toString("yyyy-MM-dd"),
            m_lastCalculatedInterest,
            m_isReceivable,
            "Interest calculated for period " + m_fromDateEdit->text() + " to " + m_toDateEdit->text()
        );
    }

    if (ok) {
        CustomMessageBox::showInformation(this, "Success", "Interest Voucher posted successfully.");
        emit voucherPosted();
        calculateInterest();
    } else {
        CustomMessageBox::showCritical(this, "Post Failed", "Failed to post Interest Voucher.");
    }
}

void InterestCalculatorWidget::onExportCsv() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Interest Statement CSV", "InterestCalculation.csv", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        CustomMessageBox::showCritical(this, "File Error", "Could not open file for writing.");
        return;
    }

    QTextStream out(&file);
    out << "Date,Particulars,Ref No,Debit,Credit,Balance,Days,Product\n";

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
    CustomMessageBox::showInformation(this, "Export Complete", "Interest calculation exported to " + fileName);
}

} // namespace MahadevERP
