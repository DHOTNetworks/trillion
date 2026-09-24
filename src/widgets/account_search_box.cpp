#include "account_search_box.h"
#include "../database_manager.h"
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
        "  border-radius: 4px;"
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

    // Default search function against DatabaseManager parties table
    m_searchFn = [](const QString& query) -> QVariantList {
        if (query.isEmpty()) {
            return DatabaseManager::instance().executeQuery(
                "SELECT id, name, gstin, city, opening_balance as balance FROM parties ORDER BY name COLLATE NOCASE ASC LIMIT 50;"
            );
        } else {
            return DatabaseManager::instance().executeQuery(
                "SELECT id, name, gstin, city, opening_balance as balance FROM parties WHERE name LIKE ? OR city LIKE ? ORDER BY name COLLATE NOCASE ASC LIMIT 50;",
                {"%" + query + "%", "%" + query + "%"}
            );
        }
    };

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

    if (qApp) {
        qApp->installEventFilter(this);
    }
}

AccountSearchBox::~AccountSearchBox() {
    if (qApp) {
        qApp->removeEventFilter(this);
    }
    if (m_popupFrame) {
        m_popupFrame->deleteLater();
    }
}

void AccountSearchBox::setSearchFunction(std::function<QVariantList(const QString&)> fn) {
    m_searchFn = std::move(fn);
}

void AccountSearchBox::setPartyName(const QString& partyName) {
    setParty(partyName, 0);
}

void AccountSearchBox::setParty(const QString& partyName, int partyId) {
    m_programmaticChange = true;
    setText(partyName);
    m_selectedPartyId = partyId;
    if (m_selectedPartyId <= 0 && !partyName.isEmpty()) {
        QVariant v = DatabaseManager::instance().executeScalar(
            "SELECT id FROM parties WHERE name = ? COLLATE NOCASE LIMIT 1;", {partyName}
        );
        if (v.isValid() && !v.isNull()) {
            m_selectedPartyId = v.toInt();
        }
    }
    m_programmaticChange = false;
    closeSearchPopup();
}

void AccountSearchBox::setSelectedPartyId(int partyId) {
    if (partyId <= 0) {
        clearParty();
        return;
    }
    QVariant v = DatabaseManager::instance().executeScalar(
        "SELECT name FROM parties WHERE id = ? LIMIT 1;", {partyId}
    );
    if (v.isValid() && !v.isNull()) {
        setParty(v.toString(), partyId);
    }
}

void AccountSearchBox::clearParty() {
    m_programmaticChange = true;
    clear();
    m_selectedPartyId = 0;
    m_selectedPartyData.clear();
    m_programmaticChange = false;
    closeSearchPopup();
}

QString AccountSearchBox::currentPartyName() const {
    return text();
}

int AccountSearchBox::currentPartyId() const {
    if (m_selectedPartyId > 0) return m_selectedPartyId;
    QString txt = text();
    if (txt.isEmpty()) return 0;
    QVariant v = DatabaseManager::instance().executeScalar(
        "SELECT id FROM parties WHERE name = ? COLLATE NOCASE LIMIT 1;", {txt}
    );
    return (v.isValid() && !v.isNull()) ? v.toInt() : 0;
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

void AccountSearchBox::selectCurrentListItem() {
    if (m_listWidget && m_listWidget->count() > 0) {
        QListWidgetItem* item = m_listWidget->currentItem();
        if (!item) {
            item = m_listWidget->item(0);
        }
        if (item) {
            onListItemClicked(item);
        }
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
        int partyId = map.value("id").toInt();
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
        listItem->setData(Qt::UserRole + 2, partyId);
    }

    if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
    }
}

void AccountSearchBox::onTextChanged(const QString& /*text*/) {
    if (m_programmaticChange) return;
    m_selectedPartyId = 0;
    if (hasFocus()) {
        openSearchPopup();
    }
}

void AccountSearchBox::onListItemClicked(QListWidgetItem* item) {
    if (!item) return;
    QString partyName = item->data(Qt::UserRole).toString();
    m_selectedPartyData = item->data(Qt::UserRole + 1).toMap();
    int partyId = item->data(Qt::UserRole + 2).toInt();
    setParty(partyName, partyId);
    emit partySelected(partyName);
    emit partyDataSelected(m_selectedPartyData);
    emit partySelectedWithId(partyName, partyId);
    emit returnPressed();
}

void AccountSearchBox::focusInEvent(QFocusEvent* event) {
    QLineEdit::focusInEvent(event);
    selectAll();
}

void AccountSearchBox::focusOutEvent(QFocusEvent* event) {
    QLineEdit::focusOutEvent(event);
    closeSearchPopup();
}

void AccountSearchBox::hideEvent(QHideEvent* event) {
    QLineEdit::hideEvent(event);
    closeSearchPopup();
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
    if (event->type() == QEvent::Hide || event->type() == QEvent::Close ||
        event->type() == QEvent::WindowDeactivate || event->type() == QEvent::ApplicationDeactivate ||
        event->type() == QEvent::ActivationChange) {
        closeSearchPopup();
    } else if (event->type() == QEvent::MouseButtonPress) {
        if (m_popupFrame && m_popupFrame->isVisible()) {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            QPoint globalPos = mouseEvent->globalPosition().toPoint();
            if (!m_popupFrame->geometry().contains(globalPos) && !geometry().contains(mapFromGlobal(globalPos))) {
                closeSearchPopup();
            }
        }
    } else if (event->type() == QEvent::Move || event->type() == QEvent::Resize) {
        if (m_popupFrame && m_popupFrame->isVisible()) {
            positionPopup();
        }
    }
    return QLineEdit::eventFilter(watched, event);
}
