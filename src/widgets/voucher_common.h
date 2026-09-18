#pragma once

#include <QString>
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QKeyEvent>
#include <QEvent>
#include <functional>

namespace VoucherCommon {

    // Common Color Constants (Lighter, Authentic Bahi-Khata tones)
    inline const QString BgLightPeach     = "#FDEBD0"; // Softer lighter bahi-khata peach
    inline const QString CardBgWhite      = "#FFFFFF";
    inline const QString CardBgWarm       = "#FFFDF8";
    inline const QString BorderSlate      = "#B88A68";
    inline const QString BorderAmber      = "#D48B5E";
    inline const QString BorderFocusBlue  = "#0000CC";
    inline const QString FocusBgYellow    = "#FFFFDD";
    inline const QString TextPrimary      = "#000000";
    inline const QString TextAccentRed    = "#CC0000";

    // 2D Keyboard Navigation Event Filter: Enter, Left, Right arrows and Escape only
    class GridNavigationFilter : public QObject {
    public:
        GridNavigationFilter(QWidget* current,
                             QWidget* leftTarget,
                             QWidget* rightTarget,
                             std::function<void()> onEnter = nullptr,
                             QObject* parent = nullptr)
            : QObject(parent ? parent : current)
            , m_current(current)
            , m_leftTarget(leftTarget)
            , m_rightTarget(rightTarget)
            , m_onEnter(onEnter)
        {}

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override {
            if (event->type() == QEvent::KeyPress) {
                QKeyEvent* ke = static_cast<QKeyEvent*>(event);
                QLineEdit* le = qobject_cast<QLineEdit*>(m_current);

                if (ke->key() == Qt::Key_Left) {
                    if (le && (le->cursorPosition() == 0 || le->hasSelectedText() || le->text().isEmpty())) {
                        if (m_leftTarget && m_leftTarget->isVisible() && m_leftTarget->isEnabled()) {
                            m_leftTarget->setFocus();
                            if (QLineEdit* targetLe = qobject_cast<QLineEdit*>(m_leftTarget)) {
                                targetLe->selectAll();
                            }
                            return true;
                        }
                    }
                } else if (ke->key() == Qt::Key_Right) {
                    if (le && (le->cursorPosition() >= le->text().length() || le->hasSelectedText() || le->text().isEmpty())) {
                        if (m_rightTarget && m_rightTarget->isVisible() && m_rightTarget->isEnabled()) {
                            m_rightTarget->setFocus();
                            if (QLineEdit* targetLe = qobject_cast<QLineEdit*>(m_rightTarget)) {
                                targetLe->selectAll();
                            }
                            return true;
                        }
                    }
                } else if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
                    if (m_onEnter) {
                        m_onEnter();
                        return true;
                    }
                }
            }
            return QObject::eventFilter(watched, event);
        }

    private:
        QWidget* m_current = nullptr;
        QWidget* m_leftTarget = nullptr;
        QWidget* m_rightTarget = nullptr;
        std::function<void()> m_onEnter;
    };

    inline void installGridNavigation(QWidget* current,
                                     QWidget* leftTarget,
                                     QWidget* rightTarget,
                                     std::function<void()> onEnter = nullptr) {
        if (!current) return;
        current->installEventFilter(new GridNavigationFilter(current, leftTarget, rightTarget, onEnter, current));
    }

} // namespace VoucherCommon
