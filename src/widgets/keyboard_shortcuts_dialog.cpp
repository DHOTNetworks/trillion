#include "keyboard_shortcuts_dialog.h"
#include "erp_theme.h"
#include "../services/keymap.h"
#include <QHeaderView>
#include <QKeySequence>
#include <QSet>

namespace MahadevERP {

KeyboardShortcutsDialog::KeyboardShortcutsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QString::fromUtf8("Keyboard Shortcuts Reference • Mahadev ERP"));
    setModal(true);
    resize(720, 560);
    setStyleSheet(
        QString("QDialog { background-color: %1; }"
                "QLineEdit { background-color: %2; border: 1.5px solid %3; border-radius: 6px; padding: 6px 12px; font-size: 13px; color: %4; }"
                "QLineEdit:focus { border-color: %5; }"
                "QTableWidget { background-color: %2; alternate-background-color: %1; border: 1px solid %3; border-radius: 8px; gridline-color: %6; font-size: 12.5px; color: %4; }"
                "QTableWidget::item { padding: 6px 12px; }"
                "QTableWidget::item:selected { background-color: %7; color: %8; font-weight: bold; }"
                "QHeaderView::section { background-color: %6; color: #1E293B; font-weight: 800; font-size: 12px; padding: 8px 12px; border: none; border-bottom: 2px solid %3; border-right: 1px solid #E2E8F0; }")
            .arg(ErpTheme::kCanvas.name(), ErpTheme::kPaper.name(), ErpTheme::kLine.name(),
                 ErpTheme::kNavy.name(), ErpTheme::kBlue.name(), ErpTheme::kHeaderBg.name(),
                 ErpTheme::kSelected.name(), ErpTheme::kBlueDark.name()));

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
    auto* titleCol = new QVBoxLayout();
    auto* titleLabel = new QLabel("Keyboard Shortcuts Cheat Sheet", this);
    titleLabel->setStyleSheet(ErpTheme::headerTitleStyle(18));
    auto* subLabel = new QLabel("Navigate entire application without touching mouse • Press Esc to close", this);
    subLabel->setStyleSheet(ErpTheme::mutedLabelStyle(12));
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
    tipLabel->setStyleSheet(ErpTheme::mutedLabelStyle(11));
    bottomLayout->addWidget(tipLabel);
    bottomLayout->addStretch();

    m_closeBtn = new KbdBadgeButton("Close", "Esc", this);
    m_closeBtn->bindShortcut(QKeySequence(Qt::Key_Escape));
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottomLayout->addWidget(m_closeBtn);
    mainLayout->addLayout(bottomLayout);
    // Esc is owned by m_closeBtn->bindShortcut() above — no second QShortcut.
}

void KeyboardShortcutsDialog::populateShortcuts() {
    struct ShortcutEntry {
        QString category;
        QString key;
        QString description;
    };

    // Rendered FROM ErpKeymap::bindings() — the same table the controller
    // dispatches. A shortcut documented here always works, and vice versa.
    auto categoryFor = [](const QString& action) {
        if (action.startsWith("nav.") || action.startsWith("app.period") ||
            action.startsWith("app.firm") || action == "app.back")
            return QString("Navigation");
        if (action.startsWith("voucher.") || action.startsWith("row.") ||
            action == "report.expand" || action == "report.collapse")
            return QString("Voucher Entry");
        if (action.startsWith("app.print") || action.startsWith("app.pdf") ||
            action.startsWith("app.csv") || action == "app.help")
            return QString("Reports & Export");
        return QString("Forms & Tables");
    };
    auto scopeFor = [](const QString& views) {
        if (views.isEmpty()) return QString();
        if (views == "29,30") return QString(" (Balance Sheet / P&L)");
        if (views == "8") return QString(" (Ledger Statement)");
        return QString(" (views %1)").arg(views);
    };

    QVector<ShortcutEntry> shortcuts;
    QSet<QString> seen; // dedupe actions bound to two chords (Ctrl+P / Alt+P)
    for (const auto& b : ErpKeymap::bindings()) {
        const QString action = QString::fromLatin1(b.action);
        const QString keyText = b.key.toString(QKeySequence::NativeText);
        if (seen.contains(action + "|" + QString::fromLatin1(b.label))) {
            // Merge duplicate chord into the existing row's key text.
            for (auto& s : shortcuts) {
                if (s.description == QString::fromLatin1(b.label)) {
                    s.key += " / " + keyText;
                    break;
                }
            }
            continue;
        }
        seen.insert(action + "|" + QString::fromLatin1(b.label));
        shortcuts.append({categoryFor(action), keyText,
                          QString::fromLatin1(b.label) + scopeFor(QString::fromLatin1(b.views))});
    }
    // Static form/table behaviour (Qt-native, not app bindings).
    shortcuts.append({"Forms & Tables", "Enter / Tab", "Advance focus to next input field"});
    shortcuts.append({"Forms & Tables", "Shift + Tab", "Move focus to previous input field"});
    shortcuts.append({"Forms & Tables", "Arrow keys", "Navigate table rows / search suggestions"});
    shortcuts.append({"Forms & Tables", "Space", "Toggle checkbox / selection"});

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
