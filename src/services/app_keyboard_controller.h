#pragma once

#include <QObject>
#include <QEvent>
#include <QKeyEvent>
#include <QApplication>

class MainWindow;

class AppKeyboardController : public QObject {
    Q_OBJECT
    Q_PROPERTY(int currentViewIndex READ currentViewIndex NOTIFY currentViewIndexChanged)

public:
    explicit AppKeyboardController(MainWindow* mainWindow, QObject* parent = nullptr);
    ~AppKeyboardController() override;

    static AppKeyboardController* instance();

    int currentViewIndex() const;

    // Unified Invokable Handlers (Usable in C++ and QML)
    Q_INVOKABLE void handleEscape();
    Q_INVOKABLE void handleEnter();
    Q_INVOKABLE void handleArrowKey(const QString& direction); // "Up", "Down", "Left", "Right"
    Q_INVOKABLE void openPeriodModal();
    Q_INVOKABLE void expandAll();
    Q_INVOKABLE void collapseAll();
    Q_INVOKABLE bool triggerPrint();
    Q_INVOKABLE bool triggerPdfExport();
    Q_INVOKABLE bool triggerCsvExport();
    Q_INVOKABLE bool focusSearch();
    Q_INVOKABLE void navigateTo(int viewIndex);
    Q_INVOKABLE void restoreFocus();

signals:
    void currentViewIndexChanged(int newIndex);
    void escapeHandled();
    void shortcutTriggered(const QString& action);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    bool handleKeyPress(QKeyEvent* keyEvent);
    bool handleKeyRelease(QKeyEvent* keyEvent);
    bool openDateForView(int vIdx);
    bool saveActiveVoucher(int vIdx);
    bool deleteActiveVoucher(int vIdx);

    MainWindow* m_mainWindow = nullptr;
    static AppKeyboardController* s_instance;
};
