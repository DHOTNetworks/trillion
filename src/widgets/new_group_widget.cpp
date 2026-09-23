#include "new_group_widget.h"
#include "custom_dialogs.h"
#include "kbd_badge_button.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QShortcut>

namespace MahadevERP {

NewGroupWidget::NewGroupWidget(AccountGroupsModel* groupsModel, QWidget* parent)
    : QWidget(parent)
    , m_groupsModel(groupsModel)
{
    setupUi();
    resetForm();
}

void NewGroupWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(14);

    // 1. Page Header Bar
    auto* headerLayout = new QHBoxLayout();
    auto* titleCol = new QVBoxLayout();
    auto* titleLabel = new QLabel("Create New Account Group", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #0F172A;");
    auto* subLabel = new QLabel("Define new accounting hierarchy sub-groups under Assets, Liabilities, Income, or Expenses.", this);
    subLabel->setStyleSheet("font-size: 11px; color: #64748B;");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();

    auto* backBtn = new KbdBadgeButton("← Back to Dashboard", "Esc", this);
    backBtn->setPrimaryColor("#F1F5F9", "#E2E8F0");
    backBtn->setTextColor("#475569");
    connect(backBtn, &QPushButton::clicked, this, &NewGroupWidget::backRequested);
    headerLayout->addWidget(backBtn);
    mainLayout->addLayout(headerLayout);

    // Separator
    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #E2E8F0;");
    mainLayout->addWidget(sep);

    // 2. Main Form Card
    auto* formCard = new QFrame(this);
    formCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; }"
        "QLabel.CardHeader { font-size: 12px; font-weight: bold; color: #1E40AF; background-color: #F8FAFC; border-bottom: 1px solid #E2E8F0; border-top-left-radius: 7px; border-top-right-radius: 7px; padding: 8px 12px; }"
        "QLabel { color: #334155; font-size: 12px; font-weight: 600; }"
        "QLineEdit, QComboBox { background-color: #FFFFFF; color: #0F172A; border: 1.5px solid #CBD5E1; border-radius: 4px; padding: 6px 10px; font-size: 13px; }"
        "QLineEdit:focus, QComboBox:focus { border: 2px solid #2563EB; background-color: #F8FAFC; }"
    );

    auto* cardLayout = new QVBoxLayout(formCard);
    cardLayout->setContentsMargins(0, 0, 0, 16);
    cardLayout->setSpacing(12);

    auto* cardHeader = new QLabel("ACCOUNT GROUP DEFINITION & CLASSIFICATION", formCard);
    cardHeader->setProperty("class", "CardHeader");
    cardLayout->addWidget(cardHeader);

    auto* grid = new QGridLayout();
    grid->setContentsMargins(16, 8, 16, 8);
    grid->setSpacing(14);

    m_groupNameEdit = new QLineEdit(formCard);
    m_groupNameEdit->setPlaceholderText("e.g. Direct Expenses - APMC Hamali");
    grid->addWidget(new QLabel("Account Group Name *:"), 0, 0);
    grid->addWidget(m_groupNameEdit, 0, 1);

    m_parentCombo = new QComboBox(formCard);
    m_parentCombo->setEditable(true);
    grid->addWidget(new QLabel("Parent Group *:"), 0, 2);
    grid->addWidget(m_parentCombo, 0, 3);

    m_natureCombo = new QComboBox(formCard);
    m_natureCombo->addItems({"Assets", "Liabilities", "Income", "Expense"});
    grid->addWidget(new QLabel("Primary Group Nature *:"), 1, 0);
    grid->addWidget(m_natureCombo, 1, 1);

    m_descEdit = new QLineEdit(formCard);
    m_descEdit->setPlaceholderText("Sub-group for tracking specific transactions and financial reporting");
    grid->addWidget(new QLabel("Description / Notes:"), 1, 2);
    grid->addWidget(m_descEdit, 1, 3);

    m_bsCheckBox = new QCheckBox("Extract in Financial Statements (Balance Sheet & Profit / Loss Statement)", formCard);
    m_bsCheckBox->setChecked(true);
    m_bsCheckBox->setStyleSheet("QCheckBox { font-size: 12px; color: #1E293B; font-weight: 500; }");
    grid->addWidget(m_bsCheckBox, 2, 0, 1, 4);

    cardLayout->addLayout(grid);
    mainLayout->addWidget(formCard);
    mainLayout->addStretch();

    // 3. Save & Cancel Action Bar
    auto* bottomLayout = new QHBoxLayout();
    m_cancelBtn = new KbdBadgeButton("Cancel", "Esc", this);
    m_cancelBtn->setPrimaryColor("#F1F5F9", "#E2E8F0");
    m_cancelBtn->setTextColor("#475569");
    connect(m_cancelBtn, &QPushButton::clicked, this, &NewGroupWidget::backRequested);
    bottomLayout->addWidget(m_cancelBtn);

    bottomLayout->addStretch();

    m_saveBtn = new KbdBadgeButton("Save Account Group", "Enter", this);
    m_saveBtn->setPrimaryColor("#2563EB", "#1D4ED8");
    m_saveBtn->setTextColor("#FFFFFF");
    m_saveBtn->setMinimumWidth(220);
    connect(m_saveBtn, &QPushButton::clicked, this, &NewGroupWidget::onSaveClicked);
    bottomLayout->addWidget(m_saveBtn);
    mainLayout->addLayout(bottomLayout);

    connect(new QShortcut(QKeySequence(Qt::Key_Escape), this), &QShortcut::activated, this, &NewGroupWidget::backRequested);
    connect(new QShortcut(QKeySequence(Qt::Key_Return), this), &QShortcut::activated, this, &NewGroupWidget::onSaveClicked);
    connect(new QShortcut(QKeySequence(Qt::Key_Enter), this), &QShortcut::activated, this, &NewGroupWidget::onSaveClicked);
}

void NewGroupWidget::resetForm() {
    m_groupNameEdit->clear();
    m_descEdit->clear();
    m_natureCombo->setCurrentIndex(0);
    m_bsCheckBox->setChecked(true);
    refreshParents();
}

void NewGroupWidget::refreshParents() {
    m_parentCombo->clear();
    m_parentCombo->addItem("Primary");
    if (m_groupsModel) {
        QStringList parents = m_groupsModel->get_parent_groups();
        for (const QString& p : parents) {
            if (p != "Primary" && !p.isEmpty()) {
                m_parentCombo->addItem(p);
            }
        }
    }
}

void NewGroupWidget::focusFirstField() {
    m_groupNameEdit->setFocus();
}

void NewGroupWidget::onSaveClicked() {
    QString name = m_groupNameEdit->text().trimmed();
    if (name.isEmpty()) {
        CustomMessageBox::showWarning(this, "Validation Error", "Please enter an Account Group Name.");
        m_groupNameEdit->setFocus();
        return;
    }

    QString parentGroup = m_parentCombo->currentText().trimmed();
    if (parentGroup.isEmpty()) parentGroup = "Primary";
    QString nature = m_natureCombo->currentText();
    QString desc = m_descEdit->text().trimmed();
    bool bs = m_bsCheckBox->isChecked();

    if (!CustomMessageBox::showConfirmation(this, "Confirm Save", "Are you sure you want to save & create Account Group '" + name + "'?")) {
        return;
    }

    bool ok = false;
    if (m_groupsModel) {
        ok = m_groupsModel->add_group(name, parentGroup, nature, desc, bs);
    } else {
        ok = true;
    }

    if (ok) {
        CustomMessageBox::showInformation(this, "Success", "Account Group '" + name + "' created successfully.");
        resetForm();
        emit savedSuccess();
    } else {
        CustomMessageBox::showCritical(this, "Save Failed", "Failed to create Account Group. It may already exist.");
    }
}

} // namespace MahadevERP
