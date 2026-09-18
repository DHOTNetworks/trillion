#pragma once

#include <QObject>
#include <QEvent>
#include <QKeyEvent>
#include <QApplication>

class GlobalKeyFilter : public QObject {
    Q_OBJECT

public:
    explicit GlobalKeyFilter(QObject* parent = nullptr) : QObject(parent) {}

signals:
    void escapePressed();
    void f1Pressed();
    void f2Pressed();
    void f3Pressed();
    void f4Pressed();
    void f5Pressed();
    void f8Pressed();
    void f9Pressed();
    void f11Pressed();
    void f12Pressed();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override {
        if (event->type() == QEvent::KeyPress) {
            if (QApplication::activeModalWidget()) {
                return QObject::eventFilter(obj, event);
            }
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Escape) {
                QWidget* fw = QApplication::focusWidget();
                if (fw) {
                    // Let the focused native QWidget process its own Escape key
                    return QObject::eventFilter(obj, event);
                }
                emit escapePressed();
                return true;
            }
        }
        return QObject::eventFilter(obj, event);
    }
};
