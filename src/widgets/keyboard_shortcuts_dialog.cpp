#include "keyboard_shortcuts_dialog.h"
#include <QHeaderView>
#include <QShortcut>
#include <QKeySequence>

namespace MahadevERP {

KeyboardShortcutsDialog::KeyboardShortcutsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QString::fromUtf8("Keyboard Shortcuts Reference • Mahadev ERP"));
    setModal(true);
    resize(720, 560);
    setStyleSheet(
        "QDialog { background-color: #F8FAFC; }"
        "QLineEdit { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 6px 12px; font-size: 13px; color: #0F172A; }"
        "QLineEdit:focus { border-color: #2563EB; }"
        "QTableWidget { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; gridline-color: #F1F5F9; font-size: 13px; color: #0F172A; }"
        "QTableWidget::item { padding: 8px 12px; }"
        "QTableWidget::item:selected { background-color: #EFF6FF; color: #1E3A8A; }"
        "QHeaderView::section { background-color: #0F172A; color: #FFFFFF; font-weight: bold; font-size: 12px; padding: 6px 12px; border: none; }"
        "QPushButton { background-color: #2563EB; color: #FFFFFF; font-weight: bold; font-size: 13px; border-radius: 6px; padding: 8px 20px; border: none; }"
        "QPushButton:hover { background-color: #1D4ED8; }"
    );

    setupUi();
    populateShortcuts();
}

void KeyboardShortcutsDialog::showShortcuts(QWidget* parent) {
    KeyboardShortcutsDialog dlg(parent);
    dlg.exec();
}

void KeyboardShortcutsDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(14);

    // Header Title
    auto* headerLayout = new QHBoxLayout();
    auto* iconLabel = new QLabel(QString::fromUtf8("⌨️"), this);
    iconLabel->setStyleSheet("font-size: 24px;");
    headerLayout->addWidget(iconLabel);

    auto* titleCol = new QVBoxLayout();
    auto* titleLabel = new QLabel("Keyboard Shortcuts Cheat Sheet", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #0F172A;");
    auto* subLabel = new QLabel("Navigate entire application without touching mouse • Press Esc to close", this);
    subLabel->setStyleSheet("font-size: 12px; color: #64748B;");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    // Filter bar
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search shortcut (e.g. Sales, F2, Print, Balance Sheet)...");
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &KeyboardShortcutsDialog::filterShortcuts);
    mainLayout->addWidget(m_searchEdit);

    // Table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({"Category", "Shortcut Key", "Action Description"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    mainLayout->addWidget(m_table);

    // Bottom Bar
    auto* bottomLayout = new QHBoxLayout();
    auto* tipLabel = new QLabel("Tip: Type fast to search hotkeys instantly.", this);
    tipLabel->setStyleSheet("font-size: 11px; color: #94A3B8;");
    bottomLayout->addWidget(tipLabel);
    bottomLayout->addStretch();

    m_closeBtn = new QPushButton("Close (Esc)", this);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottomLayout->addWidget(m_closeBtn);
    mainLayout->addLayout(bottomLayout);

    new QShortcut(QKeySequence(Qt::Key_Escape), this, SLOT(accept()));
}

void KeyboardShortcutsDialog::populateShortcuts() {
    struct ShortcutEntry {
        QString category;
        QString key;
        QString description;
    };

    const QVector<ShortcutEntry> shortcuts = {
        // Navigation
        {"Navigation", "Alt + 1", "Switch to Executive Dashboard"},
        {"Navigation", "Alt + 2", "Switch to Paddy Procurement (Raw Arrivals)"},
        {"Navigation", "Alt + 3", "Switch to Sales & Invoicing"},
        {"Navigation", "Alt + 4", "Switch to Milling & Production Stock"},
        {"Navigation", "Alt + 5", "Open Other Vouchers Submenu"},
        {"Navigation", "Alt + 6", "Switch to Reports & Ledgers"},
        {"Navigation", "Alt + 7", "Open Reports Submenu"},
        {"Navigation", "Alt + F1", "Open Firm / Company Selector"},
        {"Navigation", "Alt + F2", "Change Global Accounting Period / Financial Year"},
        {"Navigation", "Esc", "Navigate Back / Cancel / Close Modal"},

        // Vouchers & Entry
        {"Voucher Entry", "F2", "Context Date Dialog / New Voucher"},
        {"Voucher Entry", "F3", "Open Bank/Cash Payment Voucher"},
        {"Voucher Entry", "F4", "Open Bank/Cash Receipt Voucher"},
        {"Voucher Entry", "F5", "Open Journal Voucher"},
        {"Voucher Entry", "F8", "Open Sales Tax Invoice Entry"},
        {"Voucher Entry", "F9", "Open Purchase Bill Entry"},
        {"Voucher Entry", "F10", "Open J-Form Mandi Procurement Voucher"},
        {"Voucher Entry", "F11", "Open I-Form Mandi Buyer Issue Voucher"},
        {"Voucher Entry", "F12", "Open Milling Batch & Yield Voucher"},
        {"Voucher Entry", "Ctrl + S", "Save Active Voucher / Record"},

        // Reports & Print
        {"Reports & Export", "Ctrl + P", "Print Invoice / Active Ledger / Statement"},
        {"Reports & Export", "Alt + P", "Export Active View as PDF Document"},
        {"Reports & Export", "Ctrl + E", "Export Active View as Excel / CSV Spreadsheet"},
        {"Reports & Export", "F1 / ?", "Open Keyboard Shortcuts Help Reference"},

        // Table & Form Control
        {"Forms & Tables", "Enter / Tab", "Advance focus to next input field"},
        {"Forms & Tables", "Shift + Tab", "Move focus to previous input field"},
        {"Forms & Tables", "↑ / ↓ Arrow", "Navigate table rows / search suggestions"},
        {"Forms & Tables", "Delete", "Delete selected table row / item entry"},
        {"Forms & Tables", "Space", "Toggle checkbox / selection"}
    };

    m_table->setRowCount(shortcuts.size());
    for (int i = 0; i < shortcuts.size(); ++i) {
        auto* catItem = new QTableWidgetItem(shortcuts[i].category);
        catItem->setFont(QFont("Segoe UI", 10, QFont::Bold));
        catItem->setForeground(QBrush(QColor("#475569")));

        auto* keyItem = new QTableWidgetItem(shortcuts[i].key);
        keyItem->setFont(QFont("Segoe UI", 10, QFont::Bold));
        keyItem->setForeground(QBrush(QColor("#1D4ED8")));

        auto* descItem = new QTableWidgetItem(shortcuts[i].description);
        descItem->setFont(QFont("Segoe UI", 10));
        descItem->setForeground(QBrush(QColor("#0F172A")));

        m_table->setItem(i, 0, catItem);
        m_table->setItem(i, 1, keyItem);
        m_table->setItem(i, 2, descItem);
    }
}

void KeyboardShortcutsDialog::filterShortcuts(const QString& query) {
    QString q = query.trimmed().toLower();
    for (int i = 0; i < m_table->rowCount(); ++i) {
        if (q.isEmpty()) {
            m_table->setRowHidden(i, false);
            continue;
        }
        bool match = false;
        for (int c = 0; c < m_table->columnCount(); ++c) {
            auto* item = m_table->item(i, c);
            if (item && item->text().toLower().contains(q)) {
                match = true;
                break;
            }
        }
        m_table->setRowHidden(i, !match);
    }
}

} // namespace MahadevERP
