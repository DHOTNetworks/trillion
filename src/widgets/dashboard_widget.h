#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QKeyEvent>
#include <QDialog>
#include <QVector>
#include "../engine/fiscal_year_helper.h"
#include "../services/print_export_controller.h"
#include "../models/dashboard_controller.h"
#include "../models/firm_manager.h"
#include "../engine/bahi_khata_migrator.h"

// Forward declarations
class DashboardMenuCard;
class DashboardSubmenuDialog;

class DashboardWidget : public QWidget {
    Q_OBJECT

public:
    explicit DashboardWidget(DashboardController* dashCtrl = nullptr,
                            FirmManager* firmMgr = nullptr,
                            PrintExportController* printExportCtrl = nullptr,
                            BahiKhataMigrator* migrator = nullptr,
                            QWidget* parent = nullptr);
    ~DashboardWidget() override = default;

    void refreshStats();
    void setControllers(DashboardController* dashCtrl, FirmManager* firmMgr, BahiKhataMigrator* migrator = nullptr);

signals:
    void openViewRequested(int viewIndex);
    void requestAccountingPeriodDialog();
    void switchFirmRequested();

public slots:
    void openLedgerMenu(int initialIndex = 0);
    void openStockMenu(int initialIndex = 0);
    void openAddVoucherMenu(int initialIndex = 0);
    void openOtherVoucherMenu(int initialIndex = 0);
    void openReportsMenu(int initialIndex = 0);
    void onSyncClicked();

protected:
    void showEvent(QShowEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void triggerMenuIndex(int index, int initialSubmenuIndex = 0);
    void updatePeriodBadge();

private:
    void setupUi();
    QWidget* createLeftSection();
    QWidget* createMiddleSection();
    QWidget* createRightSection();
    QWidget* createStatCard(const QString& title, QLabel** outValLabel, const QString& subtext, const QString& accentColor);
    DashboardMenuCard* createNavMenuCard(int index, const QString& numStr, const QString& title, const QString& subtext,
                                        const QString& shortcutKey, const QString& badgeColor, const QString& badgeTextColor,
                                        const QString& activeBg, const QString& activeBorder);

    void updateSelection(int index);

    DashboardController* m_dashCtrl = nullptr;
    FirmManager* m_firmMgr = nullptr;
    PrintExportController* m_printExportCtrl = nullptr;
    BahiKhataMigrator* m_migrator = nullptr;

    int m_selectedMenuIndex = 0;
    int m_lastOpenedMenuIndex = -1;
    int m_lastSubmenuSelectedIndex = 0;
    bool m_reopenSubmenuOnReturn = false;
    QPoint m_initialMousePos;
    bool m_hasMouseMoved = false;

    QVector<DashboardMenuCard*> m_menuCards;

    // Header elements
    QPushButton* m_openFirmBtn = nullptr;
    QPushButton* m_syncBtn = nullptr;
    QPushButton* m_periodBtn = nullptr;
    QPushButton* m_newPaddyBtn = nullptr;
    QPushButton* m_newInvoiceBtn = nullptr;

    // Firm profile labels
    QLabel* m_firmNameLabel = nullptr;
    QLabel* m_fyBadgeLabel = nullptr;
    QLabel* m_gstinLabel = nullptr;
    QLabel* m_panLabel = nullptr;
    QLabel* m_statutoryLabel = nullptr;
    QLabel* m_businessTypeLabel = nullptr;
    QLabel* m_locationLabel = nullptr;

    // Stat card labels
    QLabel* m_paddyStockLabel = nullptr;
    QLabel* m_riceStockLabel = nullptr;
    QLabel* m_totalSalesLabel = nullptr;
};

// Interactive Menu Item Card
class DashboardMenuCard : public QFrame {
    Q_OBJECT
public:
    DashboardMenuCard(int index, const QString& numStr, const QString& title, const QString& subtext,
                      const QString& shortcutKey, const QString& badgeBg, const QString& badgeText,
                      const QString& activeBg, const QString& activeBorder, QWidget* parent = nullptr);

    void setSelected(bool sel);
    int index() const { return m_index; }

signals:
    void cardClicked(int index);
    void cardHovered(int index);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;

private:
    int m_index;
    bool m_isSelected = false;
    QString m_activeBg;
    QString m_activeBorder;
    QString m_normalBg = "#FFFFFF";
    QString m_normalBorder = "#E2E8F0";
};

// Popup Submenu Dialog
class DashboardSubmenuDialog : public QDialog {
    Q_OBJECT
public:
    struct SubmenuItem {
        QString title;
        QString shortcut;
        int targetViewIndex;
    };

    DashboardSubmenuDialog(const QString& menuTitle, const QString& borderColor,
                           const QVector<SubmenuItem>& items, int initialSelectedIndex = 0, QWidget* parent = nullptr);

    int selectedViewIndex() const { return m_selectedViewIndex; }
    int selectedIndex() const { return m_selectedIndex; }

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void updateSelection(int idx);
    void triggerCurrent();

    QVector<SubmenuItem> m_items;
    QVector<QFrame*> m_itemFrames;
    int m_selectedIndex = 0;
    int m_selectedViewIndex = -1;
    QString m_borderColor;
    QPoint m_initialMousePos;
    bool m_hasMouseMoved = false;
};

