#include "navigation_menu_dialog.h"
#include <QCursor>

namespace MahadevERP {

NavigationMenuDialog::NavigationMenuDialog(const MenuNode& node, QWidget* parent)
    : NavigationMenuDialog(node.title, node.borderColor, node.items, node.lastSelectedIndex, parent)
{
}

NavigationMenuDialog::NavigationMenuDialog(const QString& menuTitle, const QString& borderColor,
                                           const QVector<MenuItem>& items, int initialSelectedIndex,
                                           QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint)
    , m_menuTitle(menuTitle)
    , m_borderColor(borderColor)
    , m_items(items)
    , m_selectedIndex(initialSelectedIndex)
    , m_initialMousePos(QCursor::pos())
    , m_hasMouseMoved(false)
{
    setModal(true);
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName("navigationMenuDialog");
    setupUi();
}

void NavigationMenuDialog::setupUi()
{
    setFixedWidth(460);
    setStyleSheet(QString(
        "#navigationMenuDialog { background-color: #FFFFFF; border: 2.5px solid %1; border-radius: 12px; }"
        "QLabel { border: none; background: transparent; }"
    ).arg(m_borderColor));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(8);

    // Header
    auto* header = new QHBoxLayout();
    auto* titleLabel = new QLabel(m_menuTitle, this);
    titleLabel->setStyleSheet(QString(
        "color: %1; font-size: 13px; font-weight: 800; letter-spacing: 1px; border: none; background: transparent;"
    ).arg(m_borderColor));
    header->addWidget(titleLabel);

    header->addStretch(1);
    auto* hint = new QLabel("Press ↑ / ↓ & Enter", this);
    hint->setStyleSheet("color: #64748B; font-size: 11px; font-weight: 700; border: none; background: transparent;");
    header->addWidget(hint);
    root->addLayout(header);

    // Item Rows
    for (int i = 0; i < m_items.size(); ++i) {
        const MenuItem& itm = m_items[i];
        auto* row = new QFrame(this);
        row->setObjectName("menuRow");
        row->setFixedHeight(44);
        row->setCursor(Qt::PointingHandCursor);
        row->installEventFilter(this);

        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(12, 4, 12, 4);

        auto* label = new QLabel(itm.title, row);
        label->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        label->setStyleSheet("font-size: 13px; font-weight: 700; color: #0F172A; border: none; background: transparent;");
        rowLayout->addWidget(label, 1);

        if (!itm.shortcut.isEmpty()) {
            auto* sc = new QLabel(itm.shortcut, row);
            sc->setObjectName("scLabel");
            sc->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            sc->setStyleSheet(
                "#scLabel { background-color: #F1F5F9; border: 1px solid #CBD5E1; border-radius: 4px; padding: 2px 6px; font-size: 10px; font-weight: 700; color: #475569; }"
            );
            rowLayout->addWidget(sc);
        }

        m_itemFrames.append(row);
        root->addWidget(row);
    }

    if (m_selectedIndex < 0 || m_selectedIndex >= m_itemFrames.size()) {
        m_selectedIndex = 0;
    }
    updateSelection(m_selectedIndex);
}

void NavigationMenuDialog::showEvent(QShowEvent* event)
{
    m_initialMousePos = QCursor::pos();
    m_hasMouseMoved = false;
    QDialog::showEvent(event);

    QWidget* topWin = parentWidget() ? parentWidget()->window() : nullptr;
    if (topWin) {
        QPoint center = topWin->mapToGlobal(topWin->rect().center());
        move(center.x() - width() / 2, center.y() - height() / 2);
    } else if (parentWidget()) {
        QPoint center = parentWidget()->mapToGlobal(parentWidget()->rect().center());
        move(center.x() - width() / 2, center.y() - height() / 2);
    }
    updateSelection(m_selectedIndex);
}

bool NavigationMenuDialog::eventFilter(QObject* watched, QEvent* event)
{
    for (int i = 0; i < m_itemFrames.size(); ++i) {
        if (m_itemFrames[i] == watched) {
            if (event->type() == QEvent::MouseButtonPress) {
                m_selectedIndex = i;
                triggerCurrent();
                return true;
            } else if (event->type() == QEvent::MouseMove || event->type() == QEvent::Enter) {
                QPoint currentPos = QCursor::pos();
                if (!m_hasMouseMoved) {
                    if ((currentPos - m_initialMousePos).manhattanLength() > 5) {
                        m_hasMouseMoved = true;
                    }
                }
                if (m_hasMouseMoved) {
                    updateSelection(i);
                }
                return true;
            }
        }
    }
    return QDialog::eventFilter(watched, event);
}

void NavigationMenuDialog::updateSelection(int idx)
{
    if (idx < 0 || idx >= m_itemFrames.size()) return;
    m_selectedIndex = idx;

    for (int i = 0; i < m_itemFrames.size(); ++i) {
        if (i == m_selectedIndex) {
            m_itemFrames[i]->setStyleSheet(QString(
                "#menuRow { background-color: #EFF6FF; border: 1.5px solid %1; border-radius: 8px; }"
                "QLabel { border: none; background: transparent; }"
            ).arg(m_borderColor));
        } else {
            m_itemFrames[i]->setStyleSheet(
                "#menuRow { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }"
                "QLabel { border: none; background: transparent; }"
            );
        }
    }
}

void NavigationMenuDialog::triggerCurrent()
{
    if (m_selectedIndex >= 0 && m_selectedIndex < m_items.size()) {
        accept();
    }
}

const MenuItem* NavigationMenuDialog::selectedItem() const
{
    if (m_selectedIndex >= 0 && m_selectedIndex < m_items.size()) {
        return &m_items[m_selectedIndex];
    }
    return nullptr;
}

void NavigationMenuDialog::keyPressEvent(QKeyEvent* event)
{
    int key = event->key();

    if (key == Qt::Key_Up) {
        int next = (m_selectedIndex > 0) ? m_selectedIndex - 1 : m_itemFrames.size() - 1;
        updateSelection(next);
        event->accept();
        return;
    } else if (key == Qt::Key_Down) {
        int next = (m_selectedIndex < m_itemFrames.size() - 1) ? m_selectedIndex + 1 : 0;
        updateSelection(next);
        event->accept();
        return;
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        triggerCurrent();
        event->accept();
        return;
    } else if (key == Qt::Key_Escape) {
        reject();
        event->accept();
        return;
    } else if (key >= Qt::Key_1 && key <= Qt::Key_9) {
        int num = key - Qt::Key_1;
        if (num < m_itemFrames.size()) {
            m_selectedIndex = num;
            triggerCurrent();
            event->accept();
            return;
        }
    }

    // Check item hotkeys / shortcuts (case-insensitive letter keys)
    QString eventText = event->text().toUpper();
    for (int i = 0; i < m_items.size(); ++i) {
        const MenuItem& itm = m_items[i];
        if (itm.hotkey != 0 && itm.hotkey == key) {
            m_selectedIndex = i;
            triggerCurrent();
            event->accept();
            return;
        }
        if (!eventText.isEmpty() && !itm.shortcut.isEmpty()) {
            if (itm.shortcut.toUpper().trimmed() == eventText) {
                m_selectedIndex = i;
                triggerCurrent();
                event->accept();
                return;
            }
        }
    }

    QDialog::keyPressEvent(event);
}

} // namespace MahadevERP
