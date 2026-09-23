#include "modify_group_widget.h"
#include "custom_dialogs.h"
#include "kbd_badge_button.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QShortcut>

namespace MahadevERP {

ModifyGroupWidget::ModifyGroupWidget(AccountGroupsModel* groupsModel, QWidget* parent)
    : QWidget(parent)
    , m_groupsModel(groupsModel)
{
    setupUi();
    resetForm();
}

void ModifyGroupWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(14);

    // 1. Page Header Bar
    auto* headerLayout = new QHBoxLayout();
    auto* titleCol = new QVBoxLayout();
    auto* titleLabel = new QLabel("Modify Existing Account Group", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #0F172A;");
    auto* subLabel = new QLabel("Select any existing accounting group to modify name, parent group, nature, or reporting configuration.", this);
    subLabel->setStyleSheet("font-size: 11px; color: #64748B;");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();

    auto* backBtn = new KbdBadgeButton("← Back to Dashboard", "Esc", this);
    backBtn->setPrimaryColor("#F1F5F9", "#E2E8F0");
    backBtn->setTextColor("#475569");
    connect(backBtn, &QPushButton::clicked, this, &ModifyGroupWidget::backRequested);
    headerLayout->addWidget(backBtn);
    mainLayout->addLayout(headerLayout);

    // Separator
    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #E2E8F0;");
    mainLayout->addWidget(sep);

    // 2. Group Selector Bar
    auto* selectFrame = new QFrame(this);
    selectFrame->setStyleSheet("QFrame { background-color: #EFF6FF; border: 1.5px solid #93C5FD; border-radius: 6px; } QLabel { color: #1E40AF; font-weight: bold; font-size: 11px; }");
    auto* selectLayout = new QHBoxLayout(selectFrame);
    selectLayout->setContentsMargins(10, 6, 10, 6);
    selectLayout->setSpacing(10);
    selectLayout->addWidget(new QLabel("SELECT ACCOUNT GROUP TO MODIFY (Type / Search):", selectFrame));

    m_selectGroupCombo = new QComboBox(selectFrame);
    m_selectGroupCombo->setEditable(true);
    m_selectGroupCombo->setStyleSheet("QComboBox { background-color: #FFFFFF; color: #0F172A; border: 1px solid #60A5FA; border-radius: 4px; padding: 4px 8px; font-size: 12px; font-weight: bold; } QComboBox:focus { border: 2px solid #2563EB; }");
    connect(m_selectGroupCombo, &QComboBox::currentTextChanged, this, &ModifyGroupWidget::onGroupSelected);
    selectLayout->addWidget(m_selectGroupCombo, 1);
    mainLayout->addWidget(selectFrame);

    // 3. Main Form Card
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

    auto* cardHeader = new QLabel("ACCOUNT GROUP EDIT DETAILS", formCard);
    cardHeader->setProperty("class", "CardHeader");
    cardLayout->addWidget(cardHeader);

    auto* grid = new QGridLayout();
    grid->setContentsMargins(16, 8, 16, 8);
    grid->setSpacing(14);

    m_groupNameEdit = new QLineEdit(formCard);
    m_groupNameEdit->setPlaceholderText("Account Group Name");
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
    m_descEdit->setPlaceholderText("Description / Notes");
    grid->addWidget(new QLabel("Description / Notes:"), 1, 2);
    grid->addWidget(m_descEdit, 1, 3);

    m_bsCheckBox = new QCheckBox("Extract in Financial Statements (Balance Sheet & Profit / Loss Statement)", formCard);
    m_bsCheckBox->setChecked(true);
    m_bsCheckBox->setStyleSheet("QCheckBox { font-size: 12px; color: #1E293B; font-weight: 500; }");
    grid->addWidget(m_bsCheckBox, 2, 0, 1, 4);

    cardLayout->addLayout(grid);
    mainLayout->addWidget(formCard);
    mainLayout->addStretch();

    // 4. Update & Cancel Action Bar
    auto* bottomLayout = new QHBoxLayout();
    m_cancelBtn = new KbdBadgeButton("Cancel", "Esc", this);
    m_cancelBtn->setPrimaryColor("#F1F5F9", "#E2E8F0");
    m_cancelBtn->setTextColor("#475569");
    connect(m_cancelBtn, &QPushButton::clicked, this, &ModifyGroupWidget::backRequested);
    bottomLayout->addWidget(m_cancelBtn);

    bottomLayout->addStretch();

    m_updateBtn = new KbdBadgeButton("Update Account Group", "Enter", this);
    m_updateBtn->setPrimaryColor("#2563EB", "#1D4ED8");
    m_updateBtn->setTextColor("#FFFFFF");
    m_updateBtn->setMinimumWidth(220);
    connect(m_updateBtn, &QPushButton::clicked, this, &ModifyGroupWidget::onUpdateClicked);
    bottomLayout->addWidget(m_updateBtn);
    mainLayout->addLayout(bottomLayout);

    connect(new QShortcut(QKeySequence(Qt::Key_Escape), this), &QShortcut::activated, this, &ModifyGroupWidget::backRequested);
}

void ModifyGroupWidget::resetForm() {
    m_currentGroupId = -1;
    m_groupNameEdit->clear();
    m_descEdit->clear();
    m_natureCombo->setCurrentIndex(0);
    m_bsCheckBox->setChecked(true);
    refreshGroupsList();
}

void ModifyGroupWidget::refreshGroupsList() {
    m_selectGroupCombo->blockSignals(true);
    m_selectGroupCombo->clear();
    m_parentCombo->clear();
    m_parentCombo->addItem("Primary");

    if (m_groupsModel) {
        QStringList allGroups = m_groupsModel->get_all_group_names();
        m_selectGroupCombo->addItems(allGroups);

        QStringList parents = m_groupsModel->get_parent_groups();
        for (const QString& p : parents) {
            if (p != "Primary" && !p.isEmpty()) {
                m_parentCombo->addItem(p);
            }
        }
    }
    m_selectGroupCombo->blockSignals(false);

    if (m_selectGroupCombo->count() > 0) {
        onGroupSelected(m_selectGroupCombo->currentText());
    }
}

void ModifyGroupWidget::onGroupSelected(const QString& groupName) {
    if (!m_groupsModel || groupName.trimmed().isEmpty()) return;

    QVariantMap grp = m_groupsModel->get_group_by_name(groupName.trimmed());
    if (grp.isEmpty()) return;

    m_currentGroupId = grp.value("id").toInt();
    m_groupNameEdit->setText(grp.value("name").toString());

    QString parent = grp.value("parentGroup").toString();
    int pIdx = m_parentCombo->findText(parent);
    if (pIdx >= 0) m_parentCombo->setCurrentIndex(pIdx);
    else m_parentCombo->setCurrentText(parent);

    QString nature = grp.value("nature").toString();
    int nIdx = m_natureCombo->findText(nature);
    if (nIdx >= 0) m_natureCombo->setCurrentIndex(nIdx);

    m_descEdit->setText(grp.value("description").toString());
    m_bsCheckBox->setChecked(grp.value("affectsBalanceSheet", true).toBool());
}

void ModifyGroupWidget::focusSearch() {
    m_selectGroupCombo->setFocus();
}

void ModifyGroupWidget::onUpdateClicked() {
    if (m_currentGroupId <= 0) {
        CustomMessageBox::showWarning(this, "Select Group", "Please select a valid Account Group to modify.");
        return;
    }

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

    if (!CustomMessageBox::showConfirmation(this, "Confirm Update", "Are you sure you want to update Account Group '" + name + "'?")) {
        return;
    }

    bool ok = false;
    if (m_groupsModel) {
        ok = m_groupsModel->update_group(m_currentGroupId, name, parentGroup, nature, desc, bs);
    } else {
        ok = true;
    }

    if (ok) {
        CustomMessageBox::showInformation(this, "Success", "Account Group updated successfully.");
        refreshGroupsList();
        emit savedSuccess();
    } else {
        CustomMessageBox::showCritical(this, "Update Failed", "Failed to update Account Group.");
    }
}

} // namespace MahadevERP
