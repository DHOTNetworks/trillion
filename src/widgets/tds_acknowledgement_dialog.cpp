#include "tds_acknowledgement_dialog.h"
#include "../database_manager.h"
#include "voucher_date_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QDate>
#include <QDoubleValidator>
#include <QIntValidator>
#include <QMessageBox>

namespace MahadevERP {

TdsAcknowledgementDialog::TdsAcknowledgementDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_StyledBackground, true);
    setupUi();
    reloadData();
}

void TdsAcknowledgementDialog::setupUi()
{
    setFixedSize(740, 540);
    setStyleSheet(
        "QDialog {"
        "  background-color: #FFFFFF;"
        "  border: 2px solid #2563EB;"
        "  border-radius: 8px;"
        "}"
        "QLineEdit, QComboBox {"
        "  background-color: #FFFFFF;"
        "  border: 1.5px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  color: #0F172A;"
        "  padding: 5px 8px;"
        "  font-size: 13px;"
        "  min-height: 26px;"
        "}"
        "QLineEdit:focus, QComboBox:focus {"
        "  border: 1.5px solid #2563EB;"
        "}"
        "QLineEdit[readOnly=\"true\"] {"
        "  background-color: #F1F5F9;"
        "  color: #64748B;"
        "  border: 1.5px solid #E2E8F0;"
        "}"
        "QLabel {"
        "  color: #475569;"
        "  font-size: 12px;"
        "  font-weight: 700;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  selection-background-color: #EFF6FF;"
        "  selection-color: #1E3A8A;"
        "}"
    );

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 16);
    mainLayout->setSpacing(10);

    auto *title = new QLabel("TDS / TCS Return Acknowledgement Numbers", this);
    title->setStyleSheet("color: #0F172A; font-size: 15px; font-weight: 800; padding: 12px; background-color: #F8FAFC; border-bottom: 1px solid #E2E8F0; border-top-left-radius: 6px; border-top-right-radius: 6px;");
    title->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(title);

    // Form Grid
    auto *formCard = new QWidget(this);
    formCard->setStyleSheet("background-color: #FFFFFF; border-bottom: 1px solid #E2E8F0;");
    auto *grid = new QGridLayout(formCard);
    grid->setContentsMargins(16, 6, 16, 12);
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(8);

    grid->addWidget(new QLabel("Form Type:", formCard), 0, 0);
    m_formTypeCombo = new QComboBox(formCard);
    m_formTypeCombo->addItems({"26Q (TDS Other than Salary)", "27EQ (TCS)", "24Q (TDS Salary)"});
    grid->addWidget(m_formTypeCombo, 0, 1);

    grid->addWidget(new QLabel("Quarter:", formCard), 0, 2);
    m_quarterCombo = new QComboBox(formCard);
    m_quarterCombo->addItems({"Q1", "Q2", "Q3", "Q4"});
    grid->addWidget(m_quarterCombo, 0, 3);

    grid->addWidget(new QLabel("Ack / Token No *:", formCard), 1, 0);
    m_ackNoEdit = new QLineEdit(formCard);
    grid->addWidget(m_ackNoEdit, 1, 1);

    grid->addWidget(new QLabel("Filing Date:", formCard), 1, 2);
    auto *dtRow = new QHBoxLayout();
    dtRow->setSpacing(4);
    m_filingDateEdit = new QLineEdit(formCard);
    m_filingDateEdit->setReadOnly(true);
    m_filingDateEdit->setText(QDate::currentDate().toString("dd-MM-yyyy"));
    dtRow->addWidget(m_filingDateEdit);
    auto *btnDt = new QPushButton("📅", formCard);
    btnDt->setFixedWidth(32);
    btnDt->setStyleSheet("background-color: #F8FAFC; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px; font-size: 13px;");
    btnDt->setCursor(Qt::PointingHandCursor);
    connect(btnDt, &QPushButton::clicked, this, [this]() {
        QDate cur = QDate::fromString(m_filingDateEdit->text(), "dd-MM-yyyy");
        if (!cur.isValid()) cur = QDate::currentDate();
        QDate chosen = VoucherDateDialog::selectDate(this, cur);
        if (chosen.isValid()) m_filingDateEdit->setText(chosen.toString("dd-MM-yyyy"));
    });
    dtRow->addWidget(btnDt);
    grid->addLayout(dtRow, 1, 3);

    grid->addWidget(new QLabel("Total Deductees:", formCard), 2, 0);
    m_deducteesEdit = new QLineEdit(formCard);
    m_deducteesEdit->setValidator(new QIntValidator(0, 999999, this));
    grid->addWidget(m_deducteesEdit, 2, 1);

    grid->addWidget(new QLabel("Tax Deposited:", formCard), 2, 2);
    m_taxDepositedEdit = new QLineEdit(formCard);
    auto *dVal = new QDoubleValidator(0.0, 999999999.0, 2, this);
    dVal->setNotation(QDoubleValidator::StandardNotation);
    m_taxDepositedEdit->setValidator(dVal);
    grid->addWidget(m_taxDepositedEdit, 2, 3);

    grid->addWidget(new QLabel("Remarks:", formCard), 3, 0);
    m_remarksEdit = new QLineEdit(formCard);
    grid->addWidget(m_remarksEdit, 3, 1, 1, 2);

    auto *btnAdd = new QPushButton("+ Add Entry", formCard);
    btnAdd->setStyleSheet("QPushButton { background-color: #16A34A; color: white; border: none; padding: 7px 16px; border-radius: 6px; font-weight: 700; font-size: 13px; } QPushButton:hover { background-color: #15803D; }");
    btnAdd->setCursor(Qt::PointingHandCursor);
    connect(btnAdd, &QPushButton::clicked, this, &TdsAcknowledgementDialog::onAddClicked);
    grid->addWidget(btnAdd, 3, 3);

    mainLayout->addWidget(formCard);

    m_statusLabel = new QLabel("", this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_statusLabel);

    // Table of saved acknowledgements
    m_table = new QTableWidget(this);
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({
        "ID", "Form", "Quarter", "Ack / Token No", "Filing Date", "Deductees", "Tax Deposited"
    });
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(
        "QTableWidget {"
        "  background-color: #FFFFFF;"
        "  alternate-background-color: #F8FAFC;"
        "  border: 1px solid #CBD5E1;"
        "  gridline-color: #E2E8F0;"
        "  color: #0F172A;"
        "  font-size: 13px;"
        "  margin: 0 16px;"
        "}"
        "QHeaderView::section {"
        "  background-color: #0F172A;"
        "  color: #FFFFFF;"
        "  font-weight: 700;"
        "  font-size: 12px;"
        "  padding: 8px 6px;"
        "  border: 1px solid #1E293B;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: #EFF6FF;"
        "  color: #1E3A8A;"
        "  font-weight: 700;"
        "}"
    );
    mainLayout->addWidget(m_table, 1);

    // Buttons
    auto *btnRow = new QHBoxLayout();
    btnRow->setContentsMargins(16, 0, 16, 0);

    auto *btnDelete = new QPushButton("[Del] Delete Selected", this);
    btnDelete->setStyleSheet("QPushButton { background-color: #DC2626; color: white; border: none; padding: 7px 16px; border-radius: 6px; font-weight: 700; font-size: 13px; } QPushButton:hover { background-color: #B91C1C; }");
    btnDelete->setCursor(Qt::PointingHandCursor);
    connect(btnDelete, &QPushButton::clicked, this, &TdsAcknowledgementDialog::onDeleteClicked);
    btnRow->addWidget(btnDelete);

    btnRow->addStretch(1);

    auto *btnClose = new QPushButton("Close [Esc]", this);
    btnClose->setStyleSheet("QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; color: #475569; padding: 7px 18px; border-radius: 6px; font-weight: 600; font-size: 13px; } QPushButton:hover { background-color: #E2E8F0; }");
    btnClose->setCursor(Qt::PointingHandCursor);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    btnRow->addWidget(btnClose);

    mainLayout->addLayout(btnRow);
}

void TdsAcknowledgementDialog::reloadData()
{
    populateTable();
}

void TdsAcknowledgementDialog::populateTable()
{
    m_table->setRowCount(0);
    DatabaseManager &db = DatabaseManager::instance();
    QVariantList rows = db.executeQuery(
        "SELECT id, form_type, quarter, ack_number, filing_date, total_deductees, total_tax_deposited "
        "FROM tds_acknowledgements "
        "ORDER BY filing_date DESC, id DESC;"
    );

    m_table->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        QVariantMap m = rows[i].toMap();
        m_table->setItem(i, 0, new QTableWidgetItem(m.value("id").toString()));
        m_table->setItem(i, 1, new QTableWidgetItem(m.value("form_type").toString()));
        m_table->setItem(i, 2, new QTableWidgetItem(m.value("quarter").toString()));
        m_table->setItem(i, 3, new QTableWidgetItem(m.value("ack_number").toString()));

        QDate dt = QDate::fromString(m.value("filing_date").toString(), "yyyy-MM-dd");
        m_table->setItem(i, 4, new QTableWidgetItem(dt.isValid() ? dt.toString("dd-MM-yyyy") : m.value("filing_date").toString()));

        m_table->setItem(i, 5, new QTableWidgetItem(m.value("total_deductees").toString()));

        auto *taxItem = new QTableWidgetItem(QString::number(m.value("total_tax_deposited").toDouble(), 'f', 2));
        taxItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        QFont f = taxItem->font();
        f.setBold(true);
        taxItem->setFont(f);
        taxItem->setForeground(QBrush(QColor("#0F172A")));
        m_table->setItem(i, 6, taxItem);
    }
}

void TdsAcknowledgementDialog::onAddClicked()
{
    QString ackNo = m_ackNoEdit->text().trimmed();
    if (ackNo.isEmpty()) {
        m_statusLabel->setText("Acknowledgement Number cannot be empty.");
        m_statusLabel->setStyleSheet("color: #ef4444; font-weight: 600;");
        return;
    }

    DatabaseManager &db = DatabaseManager::instance();
    int fyId = 1;
    QVariant fyRow = db.executeScalar("SELECT id FROM financial_years WHERE is_active = 1 LIMIT 1;");
    if (fyRow.isValid() && !fyRow.isNull()) fyId = fyRow.toInt();

    QString formType = m_formTypeCombo->currentText().left(4).trimmed();
    QString qtr = m_quarterCombo->currentText();
    QDate dt = QDate::fromString(m_filingDateEdit->text(), "dd-MM-yyyy");
    QString dtIso = dt.isValid() ? dt.toString("yyyy-MM-dd") : QDate::currentDate().toString("yyyy-MM-dd");

    int deductees = m_deducteesEdit->text().toInt();
    double tax = m_taxDepositedEdit->text().toDouble();
    QString remarks = m_remarksEdit->text().trimmed();

    db.executeNonQuery(
        "INSERT INTO tds_acknowledgements ("
        "fy_id, form_type, quarter, ack_number, filing_date, total_deductees, total_tax_deposited, remarks"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?);",
        {fyId, formType, qtr, ackNo, dtIso, deductees, tax, remarks}
    );

    m_statusLabel->setText("Acknowledgement saved successfully.");
    m_statusLabel->setStyleSheet("color: #10b981; font-weight: 600;");
    m_ackNoEdit->clear();
    m_deducteesEdit->clear();
    m_taxDepositedEdit->clear();
    m_remarksEdit->clear();

    populateTable();
}

void TdsAcknowledgementDialog::onDeleteClicked()
{
    int row = m_table->currentRow();
    if (row < 0) return;

    int id = m_table->item(row, 0)->text().toInt();
    QString no = m_table->item(row, 3)->text();

    int res = QMessageBox::question(
        this, "Delete Acknowledgement",
        QString("Are you sure you want to delete Ack No %1?").arg(no),
        QMessageBox::Yes | QMessageBox::No
    );

    if (res == QMessageBox::Yes) {
        DatabaseManager &db = DatabaseManager::instance();
        db.executeNonQuery("DELETE FROM tds_acknowledgements WHERE id = ?;", {id});
        populateTable();
    }
}

} // namespace MahadevERP
