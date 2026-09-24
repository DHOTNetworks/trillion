#pragma once

#include <QDialog>
#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QVector>
#include "../models/menu_tree_manager.h"

namespace MahadevERP {

class NavigationMenuDialog : public QDialog {
    Q_OBJECT

public:
    explicit NavigationMenuDialog(const MenuNode& node, QWidget* parent = nullptr);
    NavigationMenuDialog(const QString& menuTitle, const QString& borderColor,
                         const QVector<MenuItem>& items, int initialSelectedIndex = 0,
                         QWidget* parent = nullptr);
    ~NavigationMenuDialog() override = default;

    int selectedIndex() const { return m_selectedIndex; }
    const MenuItem* selectedItem() const;

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void setupUi();
    void updateSelection(int idx);
    void triggerCurrent();

    QString m_menuTitle;
    QString m_borderColor;
    QVector<MenuItem> m_items;
    QVector<QFrame*> m_itemFrames;
    int m_selectedIndex = 0;
    QPoint m_initialMousePos;
    bool m_hasMouseMoved = false;
};

} // namespace MahadevERP
