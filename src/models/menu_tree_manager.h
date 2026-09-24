#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <functional>
#include <QWidget>

namespace MahadevERP {

enum class MenuActionType {
    OpenView,       // Navigates to targetViewIndex and dismisses all menus
    OpenSubmenu,    // Pushes targetSubmenuId onto stack and opens it
    ExecuteCustom,  // Runs callback, then stays in current menu
    Back            // Pops current menu from stack (returns to parent)
};

struct MenuItem {
    QString id;
    QString title;
    QString shortcut;
    int hotkey = 0; // Qt::Key enum value (e.g. Qt::Key_D, Qt::Key_1)
    MenuActionType actionType = MenuActionType::OpenView;
    int targetViewIndex = -1;
    QString targetSubmenuId;
    std::function<void()> callback;

    MenuItem() = default;
    MenuItem(const QString& t, const QString& sc, int hk, MenuActionType act,
             int viewIdx = -1, const QString& subId = QString(),
             std::function<void()> cb = nullptr)
        : title(t), shortcut(sc), hotkey(hk), actionType(act),
          targetViewIndex(viewIdx), targetSubmenuId(subId), callback(cb) {}
};

struct MenuNode {
    QString id;
    QString title;
    QString borderColor = "#2563EB";
    QString parentId;
    QVector<MenuItem> items;
    int lastSelectedIndex = 0;

    MenuNode() = default;
    MenuNode(const QString& i, const QString& t, const QString& color,
             const QString& parent = QString(), const QVector<MenuItem>& itms = {})
        : id(i), title(t), borderColor(color), parentId(parent), items(itms) {}
};

class MenuTreeManager : public QObject {
    Q_OBJECT

public:
    static MenuTreeManager& instance();

    // Menu Tree Registry
    void registerMenu(const MenuNode& node);
    MenuNode* getMenu(const QString& id);
    const MenuNode* getMenu(const QString& id) const;
    bool hasMenu(const QString& id) const;

    // Item customization helpers
    void setItemCallback(const QString& menuId, int itemIndex, std::function<void()> callback);
    void setItemTargetView(const QString& menuId, int itemIndex, int viewIndex);

    // Last selected index tracking
    int lastSelectedIndex(const QString& menuId) const;
    void setLastSelectedIndex(const QString& menuId, int idx);

    // Active Navigation Stack
    const QVector<QString>& navigationStack() const { return m_activeStack; }
    void clearNavigationStack() { m_activeStack.clear(); }

    // Execute menu loop (handles full stack navigation and Esc-unwinding)
    void executeMenu(const QString& startMenuId, QWidget* parent = nullptr, int initialIndex = -1);

    // Returns the menu ID that triggered the most recent view transition
    QString lastTriggeredMenuId() const { return m_lastTriggeredMenuId; }
    int lastTriggeredSubmenuIndex() const { return m_lastTriggeredSubmenuIndex; }

signals:
    void openViewRequested(int viewIndex);

private:
    explicit MenuTreeManager(QObject* parent = nullptr);
    ~MenuTreeManager() override = default;

    void setupDefaultMenus();

    QMap<QString, MenuNode> m_menus;
    QVector<QString> m_activeStack;
    QString m_lastTriggeredMenuId;
    int m_lastTriggeredSubmenuIndex = 0;
};

} // namespace MahadevERP
