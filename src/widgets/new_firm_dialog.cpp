#include "new_firm_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QFrame>
#include <QKeyEvent>
#include <QMessageBox>
#include <QDate>

NewFirmDialog::NewFirmDialog(FirmManager* firmMgr, QWidget* parent)
    : QDialog(parent)
    , m_firmMgr(firmMgr)
{
    setWindowTitle("Create New Company / Firm");
    setModal(true);
    setFixedSize(840, 620);
    setupUi();
}

void NewFirmDialog::setupUi() {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("NewFirmDialog { background-color: #FFFFFF; }");

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 18, 20, 18);
    rootLayout->setSpacing(14);

    // 1. Header Bar
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(12);

    QFrame* iconFrame = new QFrame(this);
    iconFrame->setFixedSize(36, 36);
    iconFrame->setStyleSheet("background-color: #EFF6FF; border: 1px solid #BFDBFE; border-radius: 8px;");
    QLabel* iconLabel = new QLabel("🏢", iconFrame);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setGeometry(0, 0, 36, 36);
    headerLayout->addWidget(iconFrame);

    QVBoxLayout* titleBox = new QVBoxLayout();
    titleBox->setSpacing(2);
    QLabel* titleLabel = new QLabel("CREATE NEW COMPANY / FIRM", this);
    titleLabel->setStyleSheet("font-size: 15px; font-weight: 800; color: #0F172A; letter-spacing: 0.5px; border: none; background: transparent;");
    titleBox->addWidget(titleLabel);

    QLabel* subtitleLabel = new QLabel("Initializes a dedicated SQLite database and master ledger accounts.", this);
    subtitleLabel->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
    titleBox->addWidget(subtitleLabel);
    headerLayout->addLayout(titleBox);

    headerLayout->addStretch(1);

    m_errorLabel = new QLabel("", this);
    m_errorLabel->setStyleSheet("color: #DC2626; font-size: 12px; font-weight: 700; border: none; background: transparent;");
    headerLayout->addWidget(m_errorLabel);

    rootLayout->addLayout(headerLayout);

    QFrame* div1 = new QFrame(this);
    div1->setFixedHeight(1);
    div1->setStyleSheet("background-color: #E2E8F0; border: none;");
    rootLayout->addWidget(div1);

    // 2. Scrollable Form Body
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background-color: transparent; border: none; }");

    QWidget* formContainer = new QWidget();
    formContainer->setStyleSheet("background-color: transparent;");
    QVBoxLayout* formLayout = new QVBoxLayout(formContainer);
    formLayout->setContentsMargins(4, 4, 10, 4);
    formLayout->setSpacing(14);

    auto makeFieldLabel = [](const QString& text) -> QLabel* {
        QLabel* lbl = new QLabel(text);
        lbl->setStyleSheet("font-size: 11px; font-weight: 700; color: #1E293B; border: none; background: transparent;");
        return lbl;
    };

    auto makeLineEdit = [](const QString& placeholder = "", const QString& defaultVal = "") -> QLineEdit* {
        QLineEdit* edit = new QLineEdit();
        edit->setFixedHeight(32);
        edit->setPlaceholderText(placeholder);
        if (!defaultVal.isEmpty()) edit->setText(defaultVal);
        edit->setStyleSheet(
            "QLineEdit {"
            "  background-color: #FFFFFF;"
            "  border: 1px solid #CBD5E1;"
            "  border-radius: 6px;"
            "  padding: 4px 10px;"
            "  font-size: 12px;"
            "  color: #0F172A;"
            "}"
            "QLineEdit:focus {"
            "  border: 1.5px solid #2563EB;"
            "  background-color: #F8FAFC;"
            "}"
        );
        return edit;
    };

    auto makeSectionHeader = [](const QString& text) -> QLabel* {
        QLabel* lbl = new QLabel(text);
        lbl->setStyleSheet("font-size: 11px; font-weight: 800; color: #2563EB; letter-spacing: 0.5px; border: none; background: transparent;");
        return lbl;
    };

    // --- Section 1: Firm Identity & Legal Type ---
    formLayout->addWidget(makeSectionHeader("1. FIRM IDENTITY & LEGAL TYPE"));

    QGridLayout* grid1 = new QGridLayout();
    grid1->setHorizontalSpacing(14);
    grid1->setVerticalSpacing(6);

    grid1->addWidget(makeFieldLabel("Company / Firm Full Name *"), 0, 0);
    m_compNameEdit = makeLineEdit("e.g. M/s Shree Ganesh Rice Mills");
    grid1->addWidget(m_compNameEdit, 1, 0);

    grid1->addWidget(makeFieldLabel("Legal Entity Type"), 0, 1);
    m_firmTypeCombo = new QComboBox();
    m_firmTypeCombo->setFixedHeight(32);
    m_firmTypeCombo->addItems({
        "Partnership Firm",
        "Proprietorship Firm",
        "Private Limited Company",
        "Limited Liability Partnership (LLP)",
        "Individual",
        "Hindu Undivided Family (HUF)",
        "Trust",
        "Association of Persons (AOP/BOI)"
    });
    m_firmTypeCombo->setStyleSheet(
        "QComboBox {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  padding: 4px 10px;"
        "  font-size: 12px;"
        "  color: #0F172A;"
        "}"
        "QComboBox:focus {"
        "  border: 1.5px solid #2563EB;"
        "}"
    );
    grid1->addWidget(m_firmTypeCombo, 1, 1);

    grid1->addWidget(makeFieldLabel("Nature of Business / Trade"), 2, 0);
    m_businessEdit = makeLineEdit("e.g. Rice Mill & Grain Processing, Commission Agent");
    grid1->addWidget(m_businessEdit, 3, 0);

    grid1->addWidget(makeFieldLabel("Books Beginning Date"), 2, 1);
    m_booksFromEdit = makeLineEdit("YYYY-MM-DD", "2026-04-01");
    grid1->addWidget(m_booksFromEdit, 3, 1);

    formLayout->addLayout(grid1);

    // --- Section 2: Taxation & Licenses ---
    formLayout->addWidget(makeSectionHeader("2. GST, PAN & STATUTORY LICENSES"));

    QGridLayout* grid2 = new QGridLayout();
    grid2->setHorizontalSpacing(14);
    grid2->setVerticalSpacing(6);

    grid2->addWidget(makeFieldLabel("GSTIN Number (15 Digits)"), 0, 0);
    m_gstinEdit = makeLineEdit("06AAAAA0000A1Z5");
    grid2->addWidget(m_gstinEdit, 1, 0);

    grid2->addWidget(makeFieldLabel("PAN Number (10 Digits)"), 0, 1);
    m_panEdit = makeLineEdit("AAAAA0000A");
    grid2->addWidget(m_panEdit, 1, 1);

    grid2->addWidget(makeFieldLabel("Mandi License No (ML No)"), 2, 0);
    m_mlNoEdit = makeLineEdit("5025/SRS/BOARD");
    grid2->addWidget(m_mlNoEdit, 3, 0);

    grid2->addWidget(makeFieldLabel("FSSAI License No"), 2, 1);
    m_fssaiEdit = makeLineEdit("10822019000152");
    grid2->addWidget(m_fssaiEdit, 3, 1);

    formLayout->addLayout(grid2);

    // --- Section 3: Address & Contact ---
    formLayout->addWidget(makeSectionHeader("3. ADDRESS & COMMUNICATIONS"));

    QGridLayout* grid3 = new QGridLayout();
    grid3->setHorizontalSpacing(14);
    grid3->setVerticalSpacing(6);

    grid3->addWidget(makeFieldLabel("Factory / Office Address"), 0, 0, 1, 2);
    m_addressEdit = makeLineEdit("Plot / Shop No, Industrial Area / Mandi Road");
    grid3->addWidget(m_addressEdit, 1, 0, 1, 2);

    grid3->addWidget(makeFieldLabel("City / Station"), 2, 0);
    m_cityEdit = makeLineEdit("City Name");
    grid3->addWidget(m_cityEdit, 3, 0);

    grid3->addWidget(makeFieldLabel("State"), 2, 1);
    m_stateEdit = makeLineEdit("State Name");
    grid3->addWidget(m_stateEdit, 3, 1);

    grid3->addWidget(makeFieldLabel("Pincode"), 4, 0);
    m_pinEdit = makeLineEdit("Pincode");
    grid3->addWidget(m_pinEdit, 5, 0);

    grid3->addWidget(makeFieldLabel("Phone / Mobile"), 4, 1);
    QHBoxLayout* phoneBox = new QHBoxLayout();
    phoneBox->setSpacing(8);
    m_phoneEdit = makeLineEdit("Phone");
    m_mobileEdit = makeLineEdit("Mobile");
    phoneBox->addWidget(m_phoneEdit);
    phoneBox->addWidget(m_mobileEdit);
    grid3->addLayout(phoneBox, 5, 1);

    formLayout->addLayout(grid3);

    // --- Section 4: Banking Details ---
    formLayout->addWidget(makeSectionHeader("4. PRIMARY BANKING ACCOUNT"));

    QGridLayout* grid4 = new QGridLayout();
    grid4->setHorizontalSpacing(14);
    grid4->setVerticalSpacing(6);

    grid4->addWidget(makeFieldLabel("Bank Name & Branch"), 0, 0);
    m_bankNameEdit = makeLineEdit("e.g. Canara Bank, Grain Market Branch");
    grid4->addWidget(m_bankNameEdit, 1, 0);

    grid4->addWidget(makeFieldLabel("Bank Account Number"), 0, 1);
    m_bankAccEdit = makeLineEdit("Account No");
    grid4->addWidget(m_bankAccEdit, 1, 1);

    grid4->addWidget(makeFieldLabel("IFSC Code"), 2, 0);
    m_ifscEdit = makeLineEdit("CNRB0001234");
    grid4->addWidget(m_ifscEdit, 3, 0);

    formLayout->addLayout(grid4);

    scrollArea->setWidget(formContainer);
    rootLayout->addWidget(scrollArea, 1);

    QFrame* div2 = new QFrame(this);
    div2->setFixedHeight(1);
    div2->setStyleSheet("background-color: #E2E8F0; border: none;");
    rootLayout->addWidget(div2);

    // 3. Footer Buttons Bar
    QHBoxLayout* footerLayout = new QHBoxLayout();
    footerLayout->setSpacing(12);

    m_cancelBtn = new QPushButton("Cancel (Esc)", this);
    m_cancelBtn->setFixedHeight(36);
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    m_cancelBtn->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 6px; padding: 0px 16px; font-weight: 700; color: #475569; font-size: 12px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    footerLayout->addWidget(m_cancelBtn);

    footerLayout->addStretch(1);

    m_createBtn = new QPushButton("Create & Open Company (Enter)", this);
    m_createBtn->setFixedHeight(36);
    m_createBtn->setCursor(Qt::PointingHandCursor);
    m_createBtn->setStyleSheet(
        "QPushButton { background-color: #16A34A; border: none; border-radius: 6px; padding: 0px 20px; font-weight: 700; color: #FFFFFF; font-size: 12px; }"
        "QPushButton:hover { background-color: #15803D; }"
    );
    connect(m_createBtn, &QPushButton::clicked, this, &NewFirmDialog::onCreateClicked);
    footerLayout->addWidget(m_createBtn);

    rootLayout->addLayout(footerLayout);

    connect(m_gstinEdit, &QLineEdit::textChanged, this, &NewFirmDialog::onGstinChanged);
    connect(m_panEdit, &QLineEdit::textChanged, this, &NewFirmDialog::onPanChanged);

    m_compNameEdit->setFocus();
}

void NewFirmDialog::onGstinChanged(const QString& text) {
    QString g = text.trimmed().toUpper();
    if (g.length() >= 12 && m_panEdit->text().trimmed().length() < 10) {
        m_panEdit->setText(g.mid(2, 10));
    }
    if (g.length() >= 2) {
        QString code = g.left(2);
        if (code == "06") m_stateEdit->setText("Haryana");
        else if (code == "03") m_stateEdit->setText("Punjab");
        else if (code == "08") m_stateEdit->setText("Rajasthan");
        else if (code == "07") m_stateEdit->setText("Delhi");
        else if (code == "09") m_stateEdit->setText("Uttar Pradesh");
    }
}

void NewFirmDialog::onPanChanged(const QString& text) {
    autoDetectFromPan(text);
}

void NewFirmDialog::autoDetectFromPan(const QString& panStr) {
    if (panStr.length() < 4) return;
    QString clean = panStr.trimmed().toUpper();
    if (clean.length() == 15) clean = clean.mid(2, 10);
    if (clean.length() >= 4) {
        QChar c = clean.at(3);
        QString targetType;
        if (c == 'P') targetType = "Proprietorship Firm";
        else if (c == 'F') targetType = "Partnership Firm";
        else if (c == 'C') targetType = "Private Limited Company";
        else if (c == 'H') targetType = "Hindu Undivided Family (HUF)";
        else if (c == 'T') targetType = "Trust";
        else if (c == 'A' || c == 'B') targetType = "Association of Persons (AOP/BOI)";

        if (!targetType.isEmpty()) {
            int idx = m_firmTypeCombo->findText(targetType);
            if (idx >= 0) {
                m_firmTypeCombo->setCurrentIndex(idx);
            }
        }
    }
}

void NewFirmDialog::onCreateClicked() {
    QString name = m_compNameEdit->text().trimmed();
    if (name.isEmpty()) {
        m_errorLabel->setText("Company / Firm Name is required.");
        m_compNameEdit->setFocus();
        return;
    }
    m_errorLabel->setText("");

    QVariantMap info;
    info["company_name"] = name;
    info["firm_type"] = m_firmTypeCombo->currentText();
    info["business_type"] = m_businessEdit->text().trimmed();
    info["gstin"] = m_gstinEdit->text().trimmed();
    info["pan_no"] = m_panEdit->text().trimmed();
    info["ml_no"] = m_mlNoEdit->text().trimmed();
    info["fssai_no"] = m_fssaiEdit->text().trimmed();
    info["address"] = m_addressEdit->text().trimmed();
    info["city"] = m_cityEdit->text().trimmed();
    info["state"] = m_stateEdit->text().trimmed();
    QString gstinVal = m_gstinEdit->text().trimmed();
    info["state_code"] = (gstinVal.length() >= 2 && gstinVal.left(2).toInt() > 0) ? gstinVal.left(2) : "";
    info["pincode"] = m_pinEdit->text().trimmed();
    info["phone"] = m_phoneEdit->text().trimmed();
    info["mobile"] = m_mobileEdit->text().trimmed();
    info["bank_name"] = m_bankNameEdit->text().trimmed();
    info["bank_account"] = m_bankAccEdit->text().trimmed();
    info["ifsc_code"] = m_ifscEdit->text().trimmed();

    QDate cur = QDate::currentDate();
    int startYr = (cur.month() >= 4) ? cur.year() : (cur.year() - 1);
    QString booksFrom = QString("%1-04-01").arg(startYr);
    QString accYearTo = QString("%1-03-31").arg(startYr + 1);
    QString fyName = QString("FY %1-%2").arg(startYr).arg(QString::number((startYr + 1) % 100).rightJustified(2, '0'));

    info["books_from"] = booksFrom;
    info["acc_year_from"] = booksFrom;
    info["acc_year_to"] = accYearTo;
    info["fy_name"] = fyName;

    if (m_firmMgr) {
        bool ok = m_firmMgr->create_new_firm(info);
        if (ok) {
            emit firmCreated(m_firmMgr->currentFirmId(), name);
            accept();
        } else {
            m_errorLabel->setText("Failed to create firm. Check if database already exists.");
        }
    }
}

void NewFirmDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        reject();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (!m_createBtn->hasFocus() && !m_cancelBtn->hasFocus()) {
            onCreateClicked();
            event->accept();
            return;
        }
    }
    QDialog::keyPressEvent(event);
}
