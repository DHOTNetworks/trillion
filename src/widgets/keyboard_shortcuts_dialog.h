#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QTableWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "kbd_badge_button.h"

namespace MahadevERP {

class KeyboardShortcutsDialog : public QDialog {
    Q_OBJECT

public:
    explicit KeyboardShortcutsDialog(QWidget* parent = nullptr);

    static void showShortcuts(QWidget* parent = nullptr);

private slots:
    void filterShortcuts(const QString& query);

private:
    void setupUi();
    void populateShortcuts();

    QLineEdit* m_searchEdit = nullptr;
    QTableWidget* m_table = nullptr;
    KbdBadgeButton* m_closeBtn = nullptr;
};

} // namespace MahadevERP
