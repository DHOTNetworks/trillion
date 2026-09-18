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
    setFixedHeight(26);
    setStyleSheet(
        "QLineEdit {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 0px;"
        "  padding: 2px 6px;"
        "  font-size: 11px;"
        "  font-weight: 700;"
        "  font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, 'Helvetica Neue', 'Noto Sans', 'Liberation Sans', Arial, sans-serif;"
        "}"
        "QLineEdit:focus {"
        "  border: 1.5px solid #2563EB;"
        "  background-color: #EFF6FF;"
        "}"
    );

    // Create Popup with ToolTip window type to prevent focus oscillation
    m_popupFrame = new QFrame(nullptr, Qt::ToolTip | Qt::FramelessWindowHint);
    m_popupFrame->setAttribute(Qt::WA_ShowWithoutActivating, true);
    m_popupFrame->setFocusPolicy(Qt::NoFocus);
    m_popupFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1.5px solid #2563EB;"
        "  border-radius: 0px;"
        "}"
    );

    QVBoxLayout* layout = new QVBoxLayout(m_popupFrame);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(0);

    m_listWidget = new QListWidget(m_popupFrame);
    m_listWidget->setFocusPolicy(Qt::NoFocus);
    m_listWidget->setStyleSheet(
        "QListWidget {"
        "  border: none;"
        "  background-color: #FFFFFF;"
        "  font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, 'Helvetica Neue', 'Noto Sans', 'Liberation Sans', Arial, sans-serif;"
        "  font-size: 11.5px;"
        "}"
        "QListWidget::item {"
        "  padding: 6px 8px;"
        "  border-radius: 0px;"
        "  color: #1E293B;"
        "  border-bottom: 1px solid #F1F5F9;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #F8FAFC;"
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
    return text();
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
    int popupWidth = qMax(width(), 420);
    int itemHeight = 32;
    int popupHeight = qMin(280, m_listWidget->count() * itemHeight + 10);
    m_popupFrame->setGeometry(globalPos.x(), globalPos.y(), popupWidth, popupHeight);
}

void AccountSearchBox::updateResults() {
    if (!m_searchFn) return;
    QString q = text();
    QVariantList results = m_searchFn(q);

    m_listWidget->clear();
    for (const QVariant& item : results) {
        QVariantMap map = item.toMap();
        QString partyName = map.value("name").toString();
        if (partyName.isEmpty()) partyName = map.value("party_name").toString();
        if (partyName.isEmpty()) partyName = map.value("title").toString();

        QString gstin = map.value("gstin").toString();
        QString city = map.value("city").toString();
        QString balance = map.value("balance").toString();

        QString display = partyName;
        if (!city.isEmpty() || !gstin.isEmpty()) {
            display += "  (";
            if (!city.isEmpty()) display += city;
            if (!city.isEmpty() && !gstin.isEmpty()) display += " | ";
            if (!gstin.isEmpty()) display += "GSTIN: " + gstin;
            display += ")";
        }

        QListWidgetItem* listItem = new QListWidgetItem(display, m_listWidget);
        listItem->setData(Qt::UserRole, partyName);
        listItem->setData(Qt::UserRole + 1, map);
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
    emit returnPressed();
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
                m_listWidget->scrollToItem(m_listWidget->currentItem(), QAbstractItemView::EnsureVisible);
            }
            event->accept();
            return;
        } else if (event->key() == Qt::Key_Up) {
            int row = m_listWidget->currentRow();
            if (row > 0) {
                m_listWidget->setCurrentRow(row - 1);
                m_listWidget->scrollToItem(m_listWidget->currentItem(), QAbstractItemView::EnsureVisible);
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
