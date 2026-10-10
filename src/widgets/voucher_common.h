#pragma once

#include <QString>
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QKeyEvent>
#include <QEvent>
#include <QCompleter>
#include <QAbstractItemView>
#include <functional>

namespace VoucherCommon {

    // Bahi-Khata style word-wise Backspace: deletes the previous whitespace-
    // delimited word before the cursor (selection first, if any). Returns
    // true when text was actually removed; false when the field is already
    // empty at the cursor (caller should step back to the previous field).
    inline bool backspaceWord(QLineEdit* edit) {
        if (!edit || edit->isReadOnly()) return false;
        if (edit->hasSelectedText()) {
            edit->backspace();
            return true;
        }
        const QString text = edit->text();
        int pos = edit->cursorPosition();
        if (pos <= 0 || text.isEmpty()) return false;
        if (pos > text.length()) pos = text.length();
        int start = pos;
        while (start > 0 && text.at(start - 1).isSpace()) --start;
        while (start > 0 && !text.at(start - 1).isSpace()) --start;
        if (start >= pos) return false;
        edit->setText(text.left(start) + text.mid(pos));
        edit->setCursorPosition(start);
        return true;
    }

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

    // 2D Keyboard Navigation Event Filter: Enter, Left, Right arrows, Backspace
    // (Bahi-Khata/Busy/Tally convention) and Escape only.
    // Backspace: word-deletes typed content; on an empty field it steps back
    // to the previous (left) field, or to onStepBack when the left target is
    // absent/hidden (e.g. back into the item grid). Left-at-edge falls back
    // to onStepBack the same way. All callbacks default to null (prior
    // behavior preserved exactly for existing callers).
    class GridNavigationFilter : public QObject {
    public:
        GridNavigationFilter(QWidget* current,
                             QWidget* leftTarget,
                             QWidget* rightTarget,
                             std::function<void()> onEnter = nullptr,
                             std::function<void()> onStepBack = nullptr,
                             QObject* parent = nullptr)
            : QObject(parent ? parent : current)
            , m_current(current)
            , m_leftTarget(leftTarget)
            , m_rightTarget(rightTarget)
            , m_onEnter(onEnter)
            , m_onStepBack(onStepBack)
        {}

    protected:
        // Editable text control behind the event: the watched widget itself,
        // a combo's line edit, or the combo-owning current widget's edit.
        static QLineEdit* resolveEdit(QObject* watched, QWidget* fallbackOwner) {
            if (auto* le = qobject_cast<QLineEdit*>(watched)) return le;
            auto ownerEdit = [](QObject* o) -> QLineEdit* {
                if (auto* cb = qobject_cast<QComboBox*>(o)) return cb->lineEdit();
                return nullptr;
            };
            if (QLineEdit* e = ownerEdit(watched)) return e;
            return ownerEdit(fallbackOwner);
        }

        // True while a completion popup is open: directional keys belong to
        // the popup then, never to field navigation.
        static bool popupOpen(QLineEdit* edit) {
            if (!edit) return false;
            if (QWidget* p = edit->parentWidget()) {
                if (auto* cb = qobject_cast<QComboBox*>(p)) {
                    if (cb->completer() && cb->completer()->popup() &&
                        cb->completer()->popup()->isVisible()) {
                        return true;
                    }
                }
            }
            if (edit->completer() && edit->completer()->popup() &&
                edit->completer()->popup()->isVisible()) {
                return true;
            }
            return false;
        }

        static void focusWithSelect(QWidget* target) {
            if (!target) return;
            target->setFocus();
            if (QLineEdit* t = qobject_cast<QLineEdit*>(target)) {
                t->selectAll();
            } else if (auto* cb = qobject_cast<QComboBox*>(target)) {
                if (cb->lineEdit()) cb->lineEdit()->selectAll();
            }
        }

        // Usable left target first, custom step-back second, else unhandled.
        bool stepBack() {
            if (m_leftTarget && m_leftTarget->isVisible() && m_leftTarget->isEnabled()) {
                focusWithSelect(m_leftTarget);
                return true;
            }
            if (m_onStepBack) {
                m_onStepBack();
                return true;
            }
            return false;
        }

        bool eventFilter(QObject* watched, QEvent* event) override {
            if (event->type() == QEvent::KeyPress) {
                QKeyEvent* ke = static_cast<QKeyEvent*>(event);
                QLineEdit* le = qobject_cast<QLineEdit*>(watched);
                if (!le) le = qobject_cast<QLineEdit*>(m_current);

                if (ke->key() == Qt::Key_Backspace) {
                    QLineEdit* edit = resolveEdit(watched, m_current);
                    if (backspaceWord(edit)) return true;
                    if (stepBack()) return true;
                } else if (ke->key() == Qt::Key_Left) {
                    if (le && !popupOpen(le) &&
                        (le->cursorPosition() == 0 || le->hasSelectedText() || le->text().isEmpty())) {
                        if (stepBack()) return true;
                    }
                } else if (ke->key() == Qt::Key_Right) {
                    if (le && !popupOpen(le) &&
                        (le->cursorPosition() >= le->text().length() || le->hasSelectedText() || le->text().isEmpty())) {
                        if (m_rightTarget && m_rightTarget->isVisible() && m_rightTarget->isEnabled()) {
                            focusWithSelect(m_rightTarget);
                            return true;
                        }
                    }
                } else if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
                    if (le && popupOpen(le)) {
                        return QObject::eventFilter(watched, event);
                    }
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
        std::function<void()> m_onStepBack;
    };

    inline void installGridNavigation(QWidget* current,
                                     QWidget* leftTarget,
                                     QWidget* rightTarget,
                                     std::function<void()> onEnter = nullptr,
                                     std::function<void()> onStepBack = nullptr) {
        if (!current) return;
        auto* filter = new GridNavigationFilter(current, leftTarget, rightTarget, onEnter, onStepBack, current);
        current->installEventFilter(filter);
        // Typing focus lives in a combo's line edit, not the combo itself;
        // cover it too so arrows/Enter/Backspace work identically there.
        if (auto* cb = qobject_cast<QComboBox*>(current)) {
            if (cb->lineEdit()) cb->lineEdit()->installEventFilter(filter);
        }
    }

} // namespace VoucherCommon
