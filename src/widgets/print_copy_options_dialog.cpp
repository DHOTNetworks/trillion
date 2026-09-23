#include "print_copy_options_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>

namespace MahadevERP {

PrintCopyOptionsDialog::PrintCopyOptionsDialog(Mode mode, const QString& invoiceNo, const QString& customerName, QWidget* parent)
    : QDialog(parent)
    , m_mode(mode)
    , m_invoiceNo(invoiceNo)
    , m_customerName(customerName)
{
    setWindowTitle(m_mode == Mode::Print ? "Select Invoice Copy to Print" : "Select Invoice Copy to Save as PDF");
    setModal(true);
    setFixedWidth(540);
    setStyleSheet(
        "QDialog { background-color: #FFFFFF; border: 2px solid " + QString(m_mode == Mode::Print ? "#059669" : "#0284C7") + "; border-radius: 12px; }"
        "QPushButton.optionBtn { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; padding: 12px 16px; text-align: left; }"
        "QPushButton.optionBtn:hover { background-color: #F8FAFC; border-color: #0284C7; }"
        "QPushButton.optionBtn:focus { background-color: #EFF6FF; border: 2px solid #2563EB; }"
        "QPushButton.cancelBtn { background-color: #F1F5F9; color: #475569; font-weight: bold; border-radius: 6px; padding: 8px 16px; border: 1px solid #CBD5E1; }"
        "QPushButton.cancelBtn:hover { background-color: #E2E8F0; }"
    );

    setupUi();
}

QString PrintCopyOptionsDialog::selectCopyType(Mode mode, const QString& invoiceNo, const QString& customerName, QWidget* parent) {
    PrintCopyOptionsDialog dlg(mode, invoiceNo, customerName, parent);
    if (dlg.exec() == QDialog::Accepted) {
        return dlg.selectedCopyType();
    }
    return QString();
}

void PrintCopyOptionsDialog::chooseCopy(const QString& copyType) {
    m_selectedCopy = copyType;
    accept();
}

void PrintCopyOptionsDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(14);

    // Header Row
    auto* headerLayout = new QHBoxLayout();
    auto* iconBadge = new QLabel(this);
    iconBadge->setFixedSize(38, 38);
    iconBadge->setAlignment(Qt::AlignCenter);
    iconBadge->setStyleSheet(
        "background-color: " + QString(m_mode == Mode::Print ? "#ECFDF5" : "#F0F9FF") + ";"
        "border: 1px solid " + QString(m_mode == Mode::Print ? "#A7F3D0" : "#BAE6FD") + ";"
        "border-radius: 8px; font-size: 18px;"
    );
    iconBadge->setText(QString::fromUtf8(m_mode == Mode::Print ? "🖨️" : "📄"));
    headerLayout->addWidget(iconBadge);

    auto* titleCol = new QVBoxLayout();
    auto* titleLabel = new QLabel(m_mode == Mode::Print ? "SELECT INVOICE COPY TO PRINT" : "SELECT INVOICE COPY TO SAVE AS PDF", this);
    titleLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #0F172A;");
    titleCol->addWidget(titleLabel);

    QString sub = "";
    if (!m_invoiceNo.isEmpty()) sub += "Invoice No: " + m_invoiceNo;
    if (!m_customerName.isEmpty()) {
        if (!sub.isEmpty()) sub += " | ";
        sub += "Buyer: " + m_customerName;
    }
    if (!sub.isEmpty()) {
        auto* subLabel = new QLabel(sub, this);
        subLabel->setStyleSheet("font-size: 12px; color: #64748B;");
        titleCol->addWidget(subLabel);
    }
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    // Divider
    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #E2E8F0;");
    mainLayout->addWidget(sep);

    auto createOptionButton = [this, mainLayout](const QString& keyNum, const QString& title, const QString& sub, const QString& value, bool defaultFocus = false) {
        auto* btn = new QPushButton(this);
        btn->setProperty("class", "optionBtn");
        auto* btnLayout = new QHBoxLayout(btn);
        btnLayout->setContentsMargins(12, 8, 12, 8);
        btnLayout->setSpacing(12);

        auto* numBadge = new QLabel(keyNum, btn);
        numBadge->setFixedSize(26, 26);
        numBadge->setAlignment(Qt::AlignCenter);
        numBadge->setStyleSheet("background-color: #EFF6FF; border: 1px solid #93C5FD; border-radius: 5px; color: #1D4ED8; font-weight: bold; font-size: 12px;");
        btnLayout->addWidget(numBadge);

        auto* textCol = new QVBoxLayout();
        auto* tLabel = new QLabel(title, btn);
        tLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #0F172A;");
        auto* sLabel = new QLabel(sub, btn);
        sLabel->setStyleSheet("font-size: 11px; color: #64748B;");
        textCol->addWidget(tLabel);
        textCol->addWidget(sLabel);
        btnLayout->addLayout(textCol);
        btnLayout->addStretch();

        connect(btn, &QPushButton::clicked, this, [this, value]() { chooseCopy(value); });
        mainLayout->addWidget(btn);

        if (defaultFocus) {
            btn->setFocus();
        }
        return btn;
    };

    createOptionButton("1", "ORIGINAL FOR RECIPIENT", "Original tax invoice copy for the buyer / customer", "ORIGINAL FOR RECIPIENT");
    createOptionButton("2", "DUPLICATE FOR TRANSPORTER", "Duplicate copy for vehicle driver / goods transport record", "DUPLICATE FOR TRANSPORTER");
    createOptionButton("3", "TRIPLICATE FOR SUPPLIER", "Triplicate copy for supplier accounting & statutory records", "TRIPLICATE FOR SUPPLIER");
    auto* defBtn = createOptionButton("4", "ALL COPIES (TRI-FOLD SET)", "Print all 3 copies consecutively in a single print job", "ALL", true);

    // Cancel Button Row
    auto* bottomLayout = new QHBoxLayout();
    auto* hintLabel = new QLabel("Press 1, 2, 3, 4 (or Esc to cancel)", this);
    hintLabel->setStyleSheet("font-size: 11px; color: #94A3B8;");
    bottomLayout->addWidget(hintLabel);
    bottomLayout->addStretch();

    auto* cancelBtn = new QPushButton("Cancel (Esc)", this);
    cancelBtn->setProperty("class", "cancelBtn");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    bottomLayout->addWidget(cancelBtn);
    mainLayout->addLayout(bottomLayout);
}

void PrintCopyOptionsDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_1 || event->key() == Qt::Key_O) {
        chooseCopy("ORIGINAL FOR RECIPIENT");
    } else if (event->key() == Qt::Key_2 || event->key() == Qt::Key_D) {
        chooseCopy("DUPLICATE FOR TRANSPORTER");
    } else if (event->key() == Qt::Key_3 || event->key() == Qt::Key_T) {
        chooseCopy("TRIPLICATE FOR SUPPLIER");
    } else if (event->key() == Qt::Key_4 || event->key() == Qt::Key_A) {
        chooseCopy("ALL");
    } else {
        QDialog::keyPressEvent(event);
    }
}

} // namespace MahadevERP
