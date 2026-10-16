#include "edit_firm_dialog.h"
#include "../database_manager.h"
#include "../config/feature_flags.h"
#include "../config/settings_registry.h"
#include "../services/db_service.h"
#include "../models/voucher_type_model.h"
#include "../models/account_groups_model.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QFrame>
#include <QKeyEvent>
#include <QMessageBox>
#include <QDebug>
#include <functional>

EditFirmDialog::EditFirmDialog(FirmManager* firmMgr, const QString& firmId,
                               const QString& dbPath, QWidget* parent)
    : QDialog(parent)
    , m_firmMgr(firmMgr)
    , m_firmId(firmId)
    , m_dbPath(dbPath)
{
    setWindowTitle("Edit Firm Information");
    setModal(true);
    setFixedSize(840, 620);
    setupUi();
    if (!loadFirm()) {
        m_errorLabel->setText("Could not read this firm's database file.");
    }
}

void EditFirmDialog::setupUi() {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("EditFirmDialog { background-color: #FFFFFF; }");

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 18, 20, 18);
    rootLayout->setSpacing(14);

    QHBoxLayout* headerLayout = new QHBoxLayout();
    QVBoxLayout* titleBox = new QVBoxLayout();
    titleBox->setSpacing(2);
    QLabel* titleLabel = new QLabel("EDIT FIRM INFORMATION", this);
    titleLabel->setStyleSheet("font-size: 15px; font-weight: 800; color: #0F172A; letter-spacing: 0.5px; border: none; background: transparent;");
    titleBox->addWidget(titleLabel);
    QLabel* subtitleLabel = new QLabel("Changes save into this firm's own database file.", this);
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
    auto makeLineEdit = [](const QString& placeholder = "") -> QLineEdit* {
        QLineEdit* edit = new QLineEdit();
        edit->setMinimumHeight(32);
        edit->setPlaceholderText(placeholder);
        edit->setStyleSheet(
            "QLineEdit { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 6px;"
            " padding: 4px 10px; font-size: 12px; color: #0F172A; }"
            "QLineEdit:focus { border: 1.5px solid #2563EB; background-color: #F8FAFC; }");
        return edit;
    };
    auto makeSectionHeader = [](const QString& text) -> QLabel* {
        QLabel* lbl = new QLabel(text);
        lbl->setStyleSheet("font-size: 11px; font-weight: 800; color: #2563EB; letter-spacing: 0.5px; border: none; background: transparent;");
        return lbl;
    };

    formLayout->addWidget(makeSectionHeader("1. FIRM IDENTITY"));
    QGridLayout* grid1 = new QGridLayout();
    grid1->setHorizontalSpacing(14);
    grid1->setVerticalSpacing(6);
    grid1->addWidget(makeFieldLabel("Company / Firm Full Name *"), 0, 0);
    m_compNameEdit = makeLineEdit();
    grid1->addWidget(m_compNameEdit, 1, 0);
    grid1->addWidget(makeFieldLabel("GSTIN Number (15 Digits)"), 0, 1);
    m_gstinEdit = makeLineEdit();
    grid1->addWidget(m_gstinEdit, 1, 1);
    grid1->addWidget(makeFieldLabel("PAN Number (10 Digits)"), 2, 0);
    m_panEdit = makeLineEdit();
    grid1->addWidget(m_panEdit, 3, 0);
    grid1->addWidget(makeFieldLabel("Mandi License No (ML No)"), 2, 1);
    m_mlNoEdit = makeLineEdit();
    grid1->addWidget(m_mlNoEdit, 3, 1);
    grid1->addWidget(makeFieldLabel("FSSAI License No"), 4, 0);
    m_fssaiEdit = makeLineEdit();
    grid1->addWidget(m_fssaiEdit, 5, 0);
    formLayout->addLayout(grid1);

    formLayout->addWidget(makeSectionHeader("2. ADDRESS & COMMUNICATIONS"));
    QGridLayout* grid2 = new QGridLayout();
    grid2->setHorizontalSpacing(14);
    grid2->setVerticalSpacing(6);
    grid2->addWidget(makeFieldLabel("Factory / Office Address"), 0, 0, 1, 2);
    m_addressEdit = makeLineEdit();
    grid2->addWidget(m_addressEdit, 1, 0, 1, 2);
    grid2->addWidget(makeFieldLabel("City / Station"), 2, 0);
    m_cityEdit = makeLineEdit();
    grid2->addWidget(m_cityEdit, 3, 0);
    grid2->addWidget(makeFieldLabel("State"), 2, 1);
    m_stateEdit = makeLineEdit();
    grid2->addWidget(m_stateEdit, 3, 1);
    grid2->addWidget(makeFieldLabel("Pincode"), 4, 0);
    m_pinEdit = makeLineEdit();
    grid2->addWidget(m_pinEdit, 5, 0);
    grid2->addWidget(makeFieldLabel("Phone / Mobile"), 4, 1);
    QHBoxLayout* phoneBox = new QHBoxLayout();
    phoneBox->setSpacing(8);
    m_phoneEdit = makeLineEdit("Phone");
    m_mobileEdit = makeLineEdit("Mobile");
    phoneBox->addWidget(m_phoneEdit);
    phoneBox->addWidget(m_mobileEdit);
    grid2->addLayout(phoneBox, 5, 1);
    formLayout->addLayout(grid2);

    formLayout->addWidget(makeSectionHeader("3. PRIMARY BANKING ACCOUNT"));
    QGridLayout* grid3 = new QGridLayout();
    grid3->setHorizontalSpacing(14);
    grid3->setVerticalSpacing(6);
    grid3->addWidget(makeFieldLabel("Bank Name & Branch"), 0, 0);
    m_bankNameEdit = makeLineEdit();
    grid3->addWidget(m_bankNameEdit, 1, 0);
    grid3->addWidget(makeFieldLabel("Bank Account Number"), 0, 1);
    m_bankAccEdit = makeLineEdit();
    grid3->addWidget(m_bankAccEdit, 1, 1);
    grid3->addWidget(makeFieldLabel("IFSC Code"), 2, 0);
    m_ifscEdit = makeLineEdit();
    grid3->addWidget(m_ifscEdit, 3, 0);
    formLayout->addLayout(grid3);

    // 4. Business package (radios only — additive-only: base flags are
    // OR-merged on save, never cleared, so no add-on boxes here).
    formLayout->addWidget(makeSectionHeader("4. BUSINESS PACKAGE"));
    auto makeRadio = [](const QString& text) -> QRadioButton* {
        QRadioButton* r = new QRadioButton(text);
        r->setStyleSheet("QRadioButton { font-size: 12px; color: #0F172A; spacing: 8px; }");
        return r;
    };
    m_packageGroup = new QButtonGroup(this);
    m_pkgAgent = makeRadio("Commission Agent (Mandi)");
    m_pkgManufacturer = makeRadio("Manufacturer");
    m_pkgTrader = makeRadio("Trader");
    m_pkgTransporter = makeRadio("Transporter");
    m_pkgGeneral = makeRadio("General / Other");
    m_packageGroup->addButton(m_pkgAgent, 0);
    m_packageGroup->addButton(m_pkgManufacturer, 1);
    m_packageGroup->addButton(m_pkgTrader, 2);
    m_packageGroup->addButton(m_pkgTransporter, 3);
    m_packageGroup->addButton(m_pkgGeneral, 4);
    formLayout->addWidget(m_pkgAgent);
    formLayout->addWidget(m_pkgManufacturer);
    formLayout->addWidget(m_pkgTrader);
    formLayout->addWidget(m_pkgTransporter);
    formLayout->addWidget(m_pkgGeneral);

    QLabel* addOnLabel = new QLabel("Add-ons (only valid extras stay enabled)", this);
    addOnLabel->setStyleSheet("font-size: 11px; font-weight: 700; color: #1E293B; border: none; background: transparent;");
    formLayout->addWidget(addOnLabel);
    auto makeCheck = [](const QString& text) -> QCheckBox* {
        QCheckBox* c = new QCheckBox(text);
        c->setStyleSheet("QCheckBox { font-size: 12px; color: #0F172A; spacing: 8px; }");
        return c;
    };
    m_addMandi = makeCheck("Mandi forms (J-Form / I-Form / paddy)");
    m_addManufacturer = makeCheck("Manufacturing production");
    m_addBardana = makeCheck("Bardana (gunny bags)");
    m_addTransport = makeCheck("Transport dispatch & gate register");
    formLayout->addWidget(m_addMandi);
    formLayout->addWidget(m_addManufacturer);
    formLayout->addWidget(m_addBardana);
    formLayout->addWidget(m_addTransport);
    connect(m_packageGroup, &QButtonGroup::idClicked, this, [this](int) { updatePackageAddOns(); });

    scrollArea->setWidget(formContainer);
    rootLayout->addWidget(scrollArea, 1);

    QFrame* div2 = new QFrame(this);
    div2->setMinimumHeight(1);
    div2->setStyleSheet("background-color: #E2E8F0; border: none;");
    rootLayout->addWidget(div2);

    QHBoxLayout* footerLayout = new QHBoxLayout();
    footerLayout->setSpacing(12);
    m_cancelBtn = new KbdBadgeButton("Cancel", "Esc", this);
    m_cancelBtn->setMinimumHeight(36);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    footerLayout->addWidget(m_cancelBtn);
    footerLayout->addStretch(1);
    m_saveBtn = new KbdBadgeButton("Save Changes", "Enter", this);
    m_saveBtn->setPrimaryColor(ErpTheme::kGreen, ErpTheme::kGreenDark);
    m_saveBtn->setTextColor(ErpTheme::kPaper);
    m_saveBtn->setMinimumHeight(36);
    connect(m_saveBtn, &QPushButton::clicked, this, &EditFirmDialog::onSaveClicked);
    footerLayout->addWidget(m_saveBtn);
    rootLayout->addLayout(footerLayout);

    m_compNameEdit->setFocus();
}

static void withFirmDb(const QString& targetPath, const std::function<void()>& fn) {
    // Switch dance: the singleton can only point at one file. Remember the
    // live path, work on the target, always switch back (even on failure
    // paths — callers must not early-return between the two switches
    // without restoring; this helper owns both ends).
    auto& db = DatabaseManager::instance();
    const QString live = db.dbPath();
    const bool moved = (live != targetPath) && db.switchDatabase(targetPath);
    fn();
    if (moved) {
        db.switchDatabase(live);
        if (!MahadevERP::DbService::instance().restart(live)) {
            qWarning() << "[EditFirmDialog] DbService worker restart failed on" << live
                       << "(data is saved; restart on next firm switch).";
        }
    }
}

bool EditFirmDialog::loadFirm() {
    bool ok = false;
    withFirmDb(m_dbPath, [&]() {
        QVariantList rows = DatabaseManager::instance().executeQuery(
            "SELECT company_name, gstin, pan_no, ml_no, fssai_no, address, city, "
            "state, pincode, phone, mobile, bank_name, bank_account, ifsc_code "
            "FROM company_info LIMIT 1;");
        if (rows.isEmpty()) return;
        const QVariantMap m = rows.first().toMap();
        m_compNameEdit->setText(m.value("company_name").toString());
        m_gstinEdit->setText(m.value("gstin").toString());
        m_panEdit->setText(m.value("pan_no").toString());
        m_mlNoEdit->setText(m.value("ml_no").toString());
        m_fssaiEdit->setText(m.value("fssai_no").toString());
        m_addressEdit->setText(m.value("address").toString());
        m_cityEdit->setText(m.value("city").toString());
        m_stateEdit->setText(m.value("state").toString());
        m_pinEdit->setText(m.value("pincode").toString());
        m_phoneEdit->setText(m.value("phone").toString());
        m_mobileEdit->setText(m.value("mobile").toString());
        m_bankNameEdit->setText(m.value("bank_name").toString());
        m_bankAccEdit->setText(m.value("bank_account").toString());
        m_ifscEdit->setText(m.value("ifsc_code").toString());
        const QString pkg = DatabaseManager::instance()
                                .getSetting(SettingsKeys::firm_business_type(), "general")
                                .trimmed().toLower();
        QRadioButton* btns[] = {m_pkgAgent, m_pkgManufacturer, m_pkgTrader, m_pkgTransporter, m_pkgGeneral};
        const char* KEYS[] = {"commission_agent", "manufacturer", "trader", "transporter", "general"};
        bool matched = false;
        for (int i = 0; i < 5; ++i) {
            if (pkg == KEYS[i]) { btns[i]->setChecked(true); matched = true; break; }
        }
        if (!matched) m_pkgGeneral->setChecked(true);
        m_loadedFlags = FeatureFlags::load();
        ok = true;
    });
    updatePackageAddOns();
    return ok;
}

void EditFirmDialog::updatePackageAddOns() {
    // Same matrix as creation (NewFirmDialog): base domains locked on,
    // only valid add-ons offered. Then already-ON flags lock their boxes
    // checked — additive-only, the UI never offers a removal.
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
    offer(m_addMandi, false);
    offer(m_addManufacturer, false);
    offer(m_addBardana, false);
    offer(m_addTransport, false);
    switch (id) {
    case 0:
        lock(m_addMandi, true);
        lock(m_addBardana, true);
        lock(m_addTransport, true);
        if (m_addManufacturer) m_addManufacturer->setEnabled(false);
        break;
    case 1:
        lock(m_addManufacturer, true);
        lock(m_addBardana, true);
        break;
    case 2:
        lock(m_addTransport, true);
        if (m_addMandi) m_addMandi->setEnabled(false);
        if (m_addManufacturer) m_addManufacturer->setEnabled(false);
        break;
    case 3:
        lock(m_addTransport, true);
        if (m_addMandi) m_addMandi->setEnabled(false);
        if (m_addManufacturer) m_addManufacturer->setEnabled(false);
        if (m_addBardana) m_addBardana->setEnabled(false);
        break;
    case 4:
    default:
        break;
    }
    if (m_loadedFlags.mandi && m_addMandi) { m_addMandi->setChecked(true); m_addMandi->setEnabled(false); }
    if (m_loadedFlags.manufacturer && m_addManufacturer) { m_addManufacturer->setChecked(true); m_addManufacturer->setEnabled(false); }
    if (m_loadedFlags.bardana && m_addBardana) { m_addBardana->setChecked(true); m_addBardana->setEnabled(false); }
    if (m_loadedFlags.transport && m_addTransport) { m_addTransport->setChecked(true); m_addTransport->setEnabled(false); }
}

QString EditFirmDialog::selectedPackageKey() const {
    const int id = m_packageGroup ? m_packageGroup->checkedId() : 4;
    switch (id) {
    case 0: return "commission_agent";
    case 1: return "manufacturer";
    case 2: return "trader";
    case 3: return "transporter";
    case 4:
    default: return "general";
    }
}

QString EditFirmDialog::selectedPackageLabel() const {
    const int id = m_packageGroup ? m_packageGroup->checkedId() : 4;
    switch (id) {
    case 0: return "Commission Agent (Mandi)";
    case 1: return "Manufacturer";
    case 2: return "Trader";
    case 3: return "Transporter";
    case 4:
    default: return "General / Other";
    }
}

void EditFirmDialog::onSaveClicked() {
    const QString name = m_compNameEdit->text().trimmed();
    if (name.isEmpty()) {
        m_errorLabel->setText("Company / Firm Name is required.");
        m_compNameEdit->setFocus();
        return;
    }
    m_errorLabel->setText("");

    const QString pkgKey = selectedPackageKey();
    const QString pkgLabel = selectedPackageLabel();
    const QString gstin = m_gstinEdit->text().trimmed();
    const QString pan = m_panEdit->text().trimmed();
    const QString ml = m_mlNoEdit->text().trimmed();
    const QString fssai = m_fssaiEdit->text().trimmed();
    const QString addr = m_addressEdit->text().trimmed();
    const QString city = m_cityEdit->text().trimmed();
    const QString state = m_stateEdit->text().trimmed();
    const QString pin = m_pinEdit->text().trimmed();
    const QString phone = m_phoneEdit->text().trimmed();
    const QString mobile = m_mobileEdit->text().trimmed();
    const QString bank = m_bankNameEdit->text().trimmed();
    const QString bankAcc = m_bankAccEdit->text().trimmed();
    const QString ifsc = m_ifscEdit->text().trimmed();

    bool ok = false;
    withFirmDb(m_dbPath, [&]() {
        auto& db = DatabaseManager::instance();
        ok = db.executeNonQuery(
            "UPDATE company_info SET company_name = ?, gstin = ?, pan_no = ?, "
            "ml_no = ?, fssai_no = ?, address = ?, city = ?, state = ?, "
            "pincode = ?, phone = ?, mobile = ?, bank_name = ?, "
            "bank_account = ?, ifsc_code = ?, business_type = ?;",
            {name, gstin, pan, ml, fssai, addr, city, state,
             pin, phone, mobile, bank, bankAcc, ifsc, pkgLabel});
        if (!ok) return;
        // Package: additive-only (base + checked add-ons OR-merged,
        // never cleared) + tables + top-ups.
        FeatureFlags sel;
        if (pkgKey == "commission_agent") { sel.mandi = sel.bardana = sel.transport = true; }
        else if (pkgKey == "manufacturer") { sel.manufacturer = sel.bardana = true; }
        else if (pkgKey == "trader" || pkgKey == "transporter") { sel.transport = true; }
        if (m_addMandi && m_addMandi->isChecked()) sel.mandi = true;
        if (m_addManufacturer && m_addManufacturer->isChecked()) sel.manufacturer = true;
        if (m_addBardana && m_addBardana->isChecked()) sel.bardana = true;
        if (m_addTransport && m_addTransport->isChecked()) sel.transport = true;
        db.setSetting(SettingsKeys::firm_business_type(), pkgKey);
        FeatureFlags eff = FeatureFlags::load();
        eff.mandi = eff.mandi || sel.mandi;
        eff.manufacturer = eff.manufacturer || sel.manufacturer;
        eff.bardana = eff.bardana || sel.bardana;
        eff.transport = eff.transport || sel.transport;
        FeatureFlags::save(eff);
        db.ensureTablesExist();
        if (eff.mandi) {
            AccountGroupsModel::ensureDomainGroups("mandi");
            VoucherTypeModel::ensureDomainTypes("mandi");
        }
        if (eff.manufacturer) {
            AccountGroupsModel::ensureDomainGroups("manufacturer");
            VoucherTypeModel::ensureDomainTypes("manufacturer");
        }
    });
    if (!ok) {
        m_errorLabel->setText("Save failed — is the firm's database file writable?");
        return;
    }
    if (m_firmMgr && !m_firmMgr->rename_firm(m_firmId, name)) {
        m_errorLabel->setText("Saved, but the selector list name did not update.");
        return;
    }
    emit firmUpdated(m_firmId, name);
    accept();
}

void EditFirmDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        reject();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (!m_saveBtn->hasFocus() && !m_cancelBtn->hasFocus()) {
            onSaveClicked();
            event->accept();
            return;
        }
    }
    QDialog::keyPressEvent(event);
}
