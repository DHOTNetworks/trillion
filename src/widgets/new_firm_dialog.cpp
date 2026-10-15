#include "new_firm_dialog.h"
#include "../services/gstin_autofill_service.h"
#include "../services/gst_state_codes.h"
#include "../services/master_data_provider.h"
#include "../config/feature_flags.h"
#include "../config/settings_registry.h"
#include "../database_manager.h"
#include "../engine/fiscal_year_helper.h"
#include "../models/account_groups_model.h"
#include "../models/voucher_type_model.h"
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
    QLabel* iconLabel = new QLabel("FIRM", iconFrame);
    iconLabel->setStyleSheet("font-size: 10px; font-weight: 800; color: #1D4ED8; border: none;");
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
    div1->setMinimumHeight(1);
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
        edit->setMinimumHeight(32);
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
    m_firmTypeCombo->setMinimumHeight(32);
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

    grid1->addWidget(makeFieldLabel("Books Beginning Date"), 2, 0);
    QDate cur = QDate::currentDate();
    int fyStartYear = FiscalCalendar::fyStartYearForDate(cur.year(), cur.month());
    QString defaultBooksFrom = FiscalCalendar::fyStartDate(fyStartYear);
    m_booksFromEdit = makeLineEdit("YYYY-MM-DD", defaultBooksFrom);
    grid1->addWidget(m_booksFromEdit, 3, 0);

    formLayout->addLayout(grid1);

    // --- Section 2: Taxation & Licenses ---
    formLayout->addWidget(makeSectionHeader("2. GST, PAN & STATUTORY LICENSES"));

    QGridLayout* grid2 = new QGridLayout();
    grid2->setHorizontalSpacing(14);
    grid2->setVerticalSpacing(6);

    grid2->addWidget(makeFieldLabel("GSTIN Number (15 Digits)"), 0, 0);
    m_gstinEdit = makeLineEdit(MasterDataProvider::instance().gstinPlaceholder());
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

    // --- Section 5: Business Package (S6 wizard) ---
    // Asked once at firm creation; migrators never ask (source data
    // auto-detects and wins — this dialog is only for brand-new firms).
    // Base package radio + valid add-ons → firm.business_type +
    // FeatureFlags::save() → ensureTablesExist() after save (see
    // onCreateClicked). Core (masters, sale/purchase, registers, GST,
    // reports) is always on; only domain flags are chosen here.
    formLayout->addWidget(makeSectionHeader("5. BUSINESS PACKAGE (WHAT THIS FIRM DOES)"));

    auto makeRadio = [&](const QString& text) -> QRadioButton* {
        QRadioButton* r = new QRadioButton(text);
        r->setStyleSheet("QRadioButton { font-size: 12px; color: #0F172A; spacing: 8px; }");
        return r;
    };
    auto makeCheck = [&](const QString& text) -> QCheckBox* {
        QCheckBox* c = new QCheckBox(text);
        c->setStyleSheet("QCheckBox { font-size: 12px; color: #0F172A; spacing: 8px; }");
        return c;
    };

    m_packageGroup = new QButtonGroup(this);
    m_pkgAgent = makeRadio("Commission Agent (Mandi) — mandi + bardana + transport");
    m_pkgMill = makeRadio("Rice Mill / Manufacturer — mill + bardana");
    m_pkgTrader = makeRadio("Trader — core + transport");
    m_pkgTransporter = makeRadio("Transporter — core + transport");
    m_pkgGeneral = makeRadio("General / Other (Kirana etc.) — core only");
    m_packageGroup->addButton(m_pkgAgent, 0);
    m_packageGroup->addButton(m_pkgMill, 1);
    m_packageGroup->addButton(m_pkgTrader, 2);
    m_packageGroup->addButton(m_pkgTransporter, 3);
    m_packageGroup->addButton(m_pkgGeneral, 4);
    formLayout->addWidget(m_pkgAgent);
    formLayout->addWidget(m_pkgMill);
    formLayout->addWidget(m_pkgTrader);
    formLayout->addWidget(m_pkgTransporter);
    formLayout->addWidget(m_pkgGeneral);

    QLabel* addOnLabel = makeFieldLabel("Add-ons (only valid extras stay enabled)");
    formLayout->addWidget(addOnLabel);
    m_addMandi = makeCheck("Mandi forms (J-Form / I-Form / paddy)");
    m_addMill = makeCheck("Milling production");
    m_addBardana = makeCheck("Bardana (gunny bags)");
    m_addTransport = makeCheck("Transport dispatch & gate register");
    formLayout->addWidget(m_addMandi);
    formLayout->addWidget(m_addMill);
    formLayout->addWidget(m_addBardana);
    formLayout->addWidget(m_addTransport);

    connect(m_packageGroup, &QButtonGroup::idClicked, this, [this](int) { updatePackageAddOns(); });
    m_pkgGeneral->setChecked(true);
    updatePackageAddOns();

    scrollArea->setWidget(formContainer);
    rootLayout->addWidget(scrollArea, 1);

    QFrame* div2 = new QFrame(this);
    div2->setMinimumHeight(1);
    div2->setStyleSheet("background-color: #E2E8F0; border: none;");
    rootLayout->addWidget(div2);

    // 3. Footer Buttons Bar
    QHBoxLayout* footerLayout = new QHBoxLayout();
    footerLayout->setSpacing(12);

    m_cancelBtn = new KbdBadgeButton("Cancel", "Esc", this);
    m_cancelBtn->setMinimumHeight(36);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    footerLayout->addWidget(m_cancelBtn);

    footerLayout->addStretch(1);

    m_createBtn = new KbdBadgeButton("Create & Open Company", "Enter", this);
    m_createBtn->setPrimaryColor(ErpTheme::kGreen, ErpTheme::kGreenDark);
    m_createBtn->setTextColor(ErpTheme::kPaper);
    m_createBtn->setMinimumHeight(36);
    connect(m_createBtn, &QPushButton::clicked, this, &NewFirmDialog::onCreateClicked);
    footerLayout->addWidget(m_createBtn);

    rootLayout->addLayout(footerLayout);

    connect(m_gstinEdit, &QLineEdit::textChanged, this, &NewFirmDialog::onGstinChanged);
    connect(m_panEdit, &QLineEdit::textChanged, this, &NewFirmDialog::onPanChanged);
    connect(m_panEdit, &QLineEdit::textEdited, this, [this]() { m_panAutoFilled = false; });

    // Silent background autofill: a finished, valid GSTIN fills name/address.
    MahadevERP::GstinAutofillService::instance().watch(m_gstinEdit, [this](const MahadevERP::GstinProfile& p) {
        if (!p.valid) {
            if (!p.error.isEmpty() && p.error != "Invalid GSTIN checksum.")
                m_errorLabel->setText("GSTIN lookup: " + p.error);
            return;
        }
        const QString bestName = !p.tradeName.isEmpty() ? p.tradeName : p.legalName;
        if (!bestName.isEmpty() && m_compNameEdit->text().trimmed().isEmpty())
            m_compNameEdit->setText(bestName);
        if (!p.address.isEmpty() && m_addressEdit->text().trimmed().isEmpty())
            m_addressEdit->setText(p.address);
        if (!p.city.isEmpty() && m_cityEdit->text().trimmed().isEmpty())
            m_cityEdit->setText(p.city);
        if (!p.pincode.isEmpty() && m_pinEdit->text().trimmed().isEmpty())
            m_pinEdit->setText(p.pincode);
        if (p.gstin.length() == 15 && m_gstinEdit->text().trimmed().toUpper() == p.gstin)
            applyDerivedPan(p.gstin);
        if (p.status.compare("Cancelled", Qt::CaseInsensitive) == 0)
            m_errorLabel->setText("Warning: this GSTIN is Cancelled — invoices under it carry no ITC.");
        else if (m_errorLabel->text().startsWith("GSTIN lookup:"))
            m_errorLabel->setText("");
    });

    m_compNameEdit->setFocus();
}

void NewFirmDialog::onGstinChanged(const QString& text) {
    QString g = text.trimmed().toUpper();
    applyDerivedPan(g);
    if (g.length() >= 2) {
        const QString state = GstStateCodes::stateForGstin(g);
        if (!state.isEmpty()) m_stateEdit->setText(state);
    }
}

void NewFirmDialog::onPanChanged(const QString& text) {
    autoDetectFromPan(text);
}

void NewFirmDialog::applyDerivedPan(const QString& gstinUpper) {
    if (gstinUpper.length() < 12) return;
    const QString expected = gstinUpper.mid(2, 10);
    const QString cur = m_panEdit->text().trimmed().toUpper();
    if (cur.isEmpty()) {
        m_panEdit->setText(expected);
        m_panAutoFilled = true;
        m_panAutoGstin = gstinUpper;
        return;
    }
    if (cur == expected) {
        m_panAutoFilled = true;
        m_panAutoGstin = gstinUpper;
        return;
    }
    if (m_panAutoFilled && m_panAutoGstin != gstinUpper) {
        m_panEdit->setText(expected);
        m_panAutoGstin = gstinUpper;
    }
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

void NewFirmDialog::updatePackageAddOns() {
    // §23.8 package map. Base domains come checked + locked; only valid
    // add-ons stay enabled. Core needs no flag (always on).
    const int id = m_packageGroup ? m_packageGroup->checkedId() : 4;
    auto lock = [](QCheckBox* c, bool on) {
        if (!c) return;
        c->setChecked(on);
        c->setEnabled(false);
    };
    auto offer = [](QCheckBox* c, bool on) {
        if (!c) return;
        c->setEnabled(true);
        if (!on) c->setChecked(false);
    };
    // Reset all to offered-off first (additive choices never linger across
    // package switches — the user re-confirms extras per package).
    offer(m_addMandi, false);
    offer(m_addMill, false);
    offer(m_addBardana, false);
    offer(m_addTransport, false);
    switch (id) {
    case 0: // Commission Agent (Mandi): mandi + bardana + transport
        lock(m_addMandi, true);
        lock(m_addBardana, true);
        lock(m_addTransport, true);
        if (m_addMill) m_addMill->setEnabled(false);
        break;
    case 1: // Rice Mill: mill + bardana; add-ons: mandi forms, transport
        lock(m_addMill, true);
        lock(m_addBardana, true);
        break; // mandi + transport stay offered
    case 2: // Trader: core + transport; add-on: bardana
        lock(m_addTransport, true);
        if (m_addMandi) m_addMandi->setEnabled(false);
        if (m_addMill) m_addMill->setEnabled(false);
        break; // bardana stays offered
    case 3: // Transporter: core + transport
        lock(m_addTransport, true);
        if (m_addMandi) m_addMandi->setEnabled(false);
        if (m_addMill) m_addMill->setEnabled(false);
        if (m_addBardana) m_addBardana->setEnabled(false);
        break;
    case 4: // General: core only; any package as add-on
    default:
        break; // all four stay offered
    }
}

QString NewFirmDialog::selectedPackageKey() const {
    const int id = m_packageGroup ? m_packageGroup->checkedId() : 4;
    switch (id) {
    case 0: return "commission_agent";
    case 1: return "rice_mill";
    case 2: return "trader";
    case 3: return "transporter";
    case 4:
    default: return "general";
    }
}

QString NewFirmDialog::selectedPackageLabel() const {
    // Display label for company_info.business_type (dashboard shows it).
    const int id = m_packageGroup ? m_packageGroup->checkedId() : 4;
    switch (id) {
    case 0: return "Commission Agent (Mandi)";
    case 1: return "Rice Mill / Manufacturer";
    case 2: return "Trader";
    case 3: return "Transporter";
    case 4:
    default: return "General / Other";
    }
}

FeatureFlags NewFirmDialog::selectedPackageFlags() const {
    // Base package + checked add-ons. Menus/dropdowns/wizards consume the
    // same isViewEnabled choke — no second mechanism.
    FeatureFlags f;
    const int id = m_packageGroup ? m_packageGroup->checkedId() : 4;
    switch (id) {
    case 0: f.mandi = true; f.bardana = true; f.transport = true; break;
    case 1: f.mill = true; f.bardana = true; break;
    case 2: f.transport = true; break;
    case 3: f.transport = true; break;
    case 4:
    default: break;
    }
    if (m_addMandi && m_addMandi->isChecked()) f.mandi = true;
    if (m_addMill && m_addMill->isChecked()) f.mill = true;
    if (m_addBardana && m_addBardana->isChecked()) f.bardana = true;
    if (m_addTransport && m_addTransport->isChecked()) f.transport = true;
    return f;
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
    info["business_type"] = selectedPackageLabel(); // package display; free text retired (W6)
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
    int startYr = FiscalCalendar::fyStartYearForDate(cur.year(), cur.month());
    QString booksFrom = FiscalCalendar::fyStartDate(startYr);
    QString accYearTo = FiscalCalendar::fyEndDate(startYr);
    QString fyName = QString("FY %1-%2").arg(startYr).arg(QString::number((startYr + 1) % 100).rightJustified(2, '0'));

    info["books_from"] = booksFrom;
    info["acc_year_from"] = booksFrom;
    info["acc_year_to"] = accYearTo;
    info["fy_name"] = fyName;

    if (m_firmMgr) {
        bool ok = m_firmMgr->create_new_firm(info);
        if (ok) {
            // S6 flow (new firms only — migrators never pass through here):
            // persist the package, merge flags ADDITIVELY (OR, never clear),
            // then ensure tables so gated domain tables exist for the
            // enabled package. Menus filter via the isViewEnabled choke.
            DatabaseManager::instance().setSetting(
                SettingsKeys::firm_business_type(), selectedPackageKey());
            FeatureFlags sel = selectedPackageFlags();
            FeatureFlags eff = FeatureFlags::load();
            eff.mandi = eff.mandi || sel.mandi;
            eff.mill = eff.mill || sel.mill;
            eff.bardana = eff.bardana || sel.bardana;
            eff.transport = eff.transport || sel.transport;
            FeatureFlags::save(eff);
            DatabaseManager::instance().ensureTablesExist();
            // Package masters (S6 completion): domain groups + voucher types
            // for the enabled package, via the models (caches handled there).
            // Idempotent; migrators cover their own path via
            // DatabaseManager::ensureDomainMasters().
            if (eff.mandi) {
                AccountGroupsModel::ensureDomainGroups("mandi");
                VoucherTypeModel::ensureDomainTypes("mandi");
            }
            if (eff.mill) {
                AccountGroupsModel::ensureDomainGroups("mill");
                VoucherTypeModel::ensureDomainTypes("mill");
            }
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
