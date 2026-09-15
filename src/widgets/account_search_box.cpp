#include "account_search_box.h"
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QApplication>
#include <QScreen>
#include <QScrollBar>

AccountSearchBox::AccountSearchBox(QWidget* parent)
    : QLineEdit(parent)
{
    setPlaceholderText("Search Party Name / Ledger... (Alt+S)");
    setFixedHeight(36);
    setStyleSheet(
        "QLineEdit {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 6px;"
        "  padding: 4px 12px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  font-family: 'Segoe UI', sans-serif;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #2563EB;"
        "  background-color: #FFFFFF;"
        "}"
    );

    // Create Popup with ToolTip window type to prevent focus oscillation
    m_popupFrame = new QFrame(nullptr, Qt::ToolTip | Qt::FramelessWindowHint);
    m_popupFrame->setAttribute(Qt::WA_ShowWithoutActivating, true);
    m_popupFrame->setFocusPolicy(Qt::NoFocus);
    m_popupFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 8px;"
        "}"
    );

    QVBoxLayout* layout = new QVBoxLayout(m_popupFrame);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(0);

    m_listWidget = new QListWidget(m_popupFrame);
    m_listWidget->setFocusPolicy(Qt::NoFocus);
    m_listWidget->setStyleSheet(
        "QListWidget {"
        "  border: none;"
        "  background-color: transparent;"
        "  font-family: 'Segoe UI', sans-serif;"
        "  font-size: 12px;"
        "}"
        "QListWidget::item {"
        "  padding: 8px 10px;"
        "  border-radius: 4px;"
        "  color: #1E293B;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #F1F5F9;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #DBEAFE;"
        "  color: #1D4ED8;"
        "  font-weight: bold;"
        "}"
    );

    layout->addWidget(m_listWidget);

    connect(this, &QLineEdit::textChanged, this, &AccountSearchBox::onTextChanged);
    connect(m_listWidget, &QListWidget::itemClicked, this, &AccountSearchBox::onListItemClicked);

    if (parent) {
        parent->installEventFilter(this);
    }
}

AccountSearchBox::~AccountSearchBox() {
    if (m_popupFrame) {
        m_popupFrame->deleteLater();
    }
}

void AccountSearchBox::setSearchFunction(std::function<QVariantList(const QString&)> fn) {
    m_searchFn = std::move(fn);
}

void AccountSearchBox::setPartyName(const QString& partyName) {
    m_programmaticChange = true;
    setText(partyName);
    m_programmaticChange = false;
    closeSearchPopup();
}

QString AccountSearchBox::currentPartyName() const {
    return text().trimmed();
}

void AccountSearchBox::openSearchPopup() {
    if (m_isUpdatingPopup) return;
    m_isUpdatingPopup = true;

    updateResults();
    if (m_listWidget->count() > 0 && isVisible()) {
        positionPopup();
        m_popupFrame->show();
        m_popupFrame->raise();
    } else {
        closeSearchPopup();
    }

    m_isUpdatingPopup = false;
}

void AccountSearchBox::closeSearchPopup() {
    if (m_popupFrame && m_popupFrame->isVisible()) {
        m_popupFrame->hide();
    }
}

void AccountSearchBox::positionPopup() {
    if (!m_popupFrame || !isVisible()) return;
    QPoint globalPos = mapToGlobal(QPoint(0, height() + 2));
    int popupWidth = qMax(width(), 400);
    int itemHeight = 36;
    int popupHeight = qMin(300, m_listWidget->count() * itemHeight + 10);
    m_popupFrame->setGeometry(globalPos.x(), globalPos.y(), popupWidth, popupHeight);
}

void AccountSearchBox::updateResults() {
    if (!m_searchFn) return;
    QString q = text().trimmed();
    QVariantList results = m_searchFn(q);

    m_listWidget->clear();
    for (const QVariant& item : results) {
        QVariantMap map = item.toMap();
        QString name = map.value("name").toString();
        QString group = map.value("group_name").toString();
        QString display = name;
        if (!group.isEmpty()) {
            display += " (" + group + ")";
        }
        QListWidgetItem* listItem = new QListWidgetItem(display, m_listWidget);
        listItem->setData(Qt::UserRole, name);
    }

    if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
    }
}

void AccountSearchBox::onTextChanged(const QString& /*text*/) {
    if (m_programmaticChange) return;
    if (hasFocus()) {
        openSearchPopup();
    }
}

void AccountSearchBox::onListItemClicked(QListWidgetItem* item) {
    if (!item) return;
    QString partyName = item->data(Qt::UserRole).toString();
    setPartyName(partyName);
    emit partySelected(partyName);
}

void AccountSearchBox::focusInEvent(QFocusEvent* event) {
    QLineEdit::focusInEvent(event);
    selectAll();
}

void AccountSearchBox::focusOutEvent(QFocusEvent* event) {
    QLineEdit::focusOutEvent(event);
    if (m_popupFrame && !m_popupFrame->underMouse()) {
        closeSearchPopup();
    }
}

void AccountSearchBox::keyPressEvent(QKeyEvent* event) {
    if (m_popupFrame && m_popupFrame->isVisible()) {
        if (event->key() == Qt::Key_Down) {
            int row = m_listWidget->currentRow();
            if (row < m_listWidget->count() - 1) {
                m_listWidget->setCurrentRow(row + 1);
            }
            event->accept();
            return;
        } else if (event->key() == Qt::Key_Up) {
            int row = m_listWidget->currentRow();
            if (row > 0) {
                m_listWidget->setCurrentRow(row - 1);
            }
            event->accept();
            return;
        } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            QListWidgetItem* item = m_listWidget->currentItem();
            if (item) {
                onListItemClicked(item);
                event->accept();
                return;
            }
        } else if (event->key() == Qt::Key_Escape) {
            closeSearchPopup();
            event->accept();
            return;
        }
    } else {
        if (event->key() == Qt::Key_Down) {
            openSearchPopup();
            event->accept();
            return;
        }
    }
    QLineEdit::keyPressEvent(event);
}

void AccountSearchBox::moveEvent(QMoveEvent* event) {
    QLineEdit::moveEvent(event);
    if (m_popupFrame && m_popupFrame->isVisible()) {
        positionPopup();
    }
}

void AccountSearchBox::resizeEvent(QResizeEvent* event) {
    QLineEdit::resizeEvent(event);
    if (m_popupFrame && m_popupFrame->isVisible()) {
        positionPopup();
    }
}

bool AccountSearchBox::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::Move || event->type() == QEvent::Resize) {
        if (m_popupFrame && m_popupFrame->isVisible()) {
            positionPopup();
        }
    }
    return QLineEdit::eventFilter(watched, event);
}
