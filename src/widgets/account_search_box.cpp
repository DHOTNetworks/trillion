#include "account_search_box.h"
#include "../database_manager.h"
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QApplication>
#include <QScreen>
#include <QScrollBar>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>

#include "../engine/ledger_pipeline.h"

AccountSearchBox::AccountSearchBox(QWidget* parent)
    : QLineEdit(parent)
{
    setPlaceholderText("Search Party Name / Ledger... (Alt+S)");
    setFixedHeight(28);
    setStyleSheet(
        "QLineEdit {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 5px;"
        "  padding: 3px 8px;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;"
        "}"
        "QLineEdit:focus {"
        "  border: 1.5px solid #2563EB;"
        "  background-color: #F8FAFC;"
        "}"
    );

    // Default search function against LedgerPipeline
    m_searchFn = [](const QString& query) -> QVariantList {
        return LedgerPipeline::instance().searchLedgers(query, "", 50);
    };

    // Create Popup parented to window
    m_popupFrame = new QFrame(nullptr, Qt::Tool | Qt::FramelessWindowHint);
    m_popupFrame->setAttribute(Qt::WA_ShowWithoutActivating, true);
    m_popupFrame->setAttribute(Qt::WA_DeleteOnClose, false);
    m_popupFrame->setFocusPolicy(Qt::NoFocus);
    m_popupFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #94A3B8;"
        "  border-radius: 6px;"
        "}"
    );

    auto* shadow = new QGraphicsDropShadowEffect(m_popupFrame);
    shadow->setBlurRadius(16);
    shadow->setColor(QColor(0, 0, 0, 45));
    shadow->setOffset(0, 4);
    m_popupFrame->setGraphicsEffect(shadow);

    QVBoxLayout* layout = new QVBoxLayout(m_popupFrame);
    layout->setContentsMargins(3, 3, 3, 3);
    layout->setSpacing(0);

    m_listWidget = new QListWidget(m_popupFrame);
    m_listWidget->setFocusPolicy(Qt::NoFocus);
    m_listWidget->setUniformItemSizes(true);
    m_listWidget->setStyleSheet(
        "QListWidget {"
        "  border: none;"
        "  background-color: #FFFFFF;"
        "  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;"
        "  font-size: 12px;"
        "  outline: none;"
        "}"
        "QListWidget::item {"
        "  height: 28px;"
        "  padding: 4px 8px;"
        "  margin: 1px 2px;"
        "  border-radius: 4px;"
        "  color: #1E293B;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #F1F5F9;"
        "  color: #0F172A;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #2563EB;"
        "  color: #FFFFFF;"
        "  font-weight: 600;"
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
        m_popupFrame->hide();
        m_popupFrame->deleteLater();
        m_popupFrame = nullptr;
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
    m_programmaticChange = false;

    if (partyId > 0) {
        m_selectedPartyId = partyId;
    } else if (!partyName.isEmpty()) {
        QVariant v = DatabaseManager::instance().executeScalar(
            "SELECT id FROM parties WHERE name = ? COLLATE NOCASE LIMIT 1;", {partyName}
        );
        m_selectedPartyId = (v.isValid() && !v.isNull()) ? v.toInt() : 0;
    } else {
        m_selectedPartyId = 0;
    }

    closeSearchPopup();
}

void AccountSearchBox::setSelectedPartyId(int partyId) {
    m_selectedPartyId = partyId;
    if (partyId > 0) {
        QVariant v = DatabaseManager::instance().executeScalar(
            "SELECT name FROM parties WHERE id = ? LIMIT 1;", {partyId}
        );
        if (v.isValid() && !v.isNull()) {
            m_programmaticChange = true;
            setText(v.toString());
            m_programmaticChange = false;
        }
    }
    closeSearchPopup();
}

void AccountSearchBox::clearParty() {
    m_programmaticChange = true;
    clear();
    m_programmaticChange = false;
    m_selectedPartyId = 0;
    m_selectedPartyData.clear();
    closeSearchPopup();
}

QString AccountSearchBox::currentPartyName() const {
    return text().trimmed();
}

int AccountSearchBox::currentPartyId() const {
    if (m_selectedPartyId > 0) return m_selectedPartyId;
    QString txt = text().trimmed();
    if (txt.isEmpty()) return 0;
    QVariant v = DatabaseManager::instance().executeScalar(
        "SELECT id FROM parties WHERE name = ? COLLATE NOCASE LIMIT 1;", {txt}
    );
    return (v.isValid() && !v.isNull()) ? v.toInt() : 0;
}

void AccountSearchBox::openSearchPopup() {
    if (m_isUpdatingPopup) return;
    if (!isVisible() || !hasFocus()) {
        closeSearchPopup();
        return;
    }
    if (window() && (!window()->isVisible() || window()->isMinimized())) {
        closeSearchPopup();
        return;
    }

    m_isUpdatingPopup = true;
    updateResults();

    if (m_listWidget && m_listWidget->count() > 0 && isVisible() && hasFocus()) {
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
    int popupWidth = qMax(width(), 460);
    int itemHeight = 30;
    int popupHeight = qBound(40, m_listWidget->count() * itemHeight + 8, 260);

    // Screen boundary clamping
    QScreen* screen = window() ? window()->screen() : QApplication::primaryScreen();
    if (screen) {
        QRect screenGeo = screen->availableGeometry();
        if (globalPos.y() + popupHeight > screenGeo.bottom()) {
            // Position above if below doesn't fit
            globalPos.setY(mapToGlobal(QPoint(0, 0)).y() - popupHeight - 2);
        }
        if (globalPos.x() + popupWidth > screenGeo.right()) {
            globalPos.setX(screenGeo.right() - popupWidth - 4);
        }
    }

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
    if (hasFocus() && isVisible()) {
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
        } else if (event->key() == Qt::Key_Tab) {
            selectCurrentListItem();
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
    } else if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) {
        if (m_popupFrame && m_popupFrame->isVisible()) {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            QPoint globalPos = mouseEvent->globalPosition().toPoint();
            if (!m_popupFrame->geometry().contains(globalPos) && !this->rect().contains(this->mapFromGlobal(globalPos))) {
                closeSearchPopup();
            }
        }
    } else if (event->type() == QEvent::Move || event->type() == QEvent::Resize) {
        if (m_popupFrame && m_popupFrame->isVisible()) {
            if (!isVisible() || (window() && (!window()->isVisible() || window()->isMinimized()))) {
                closeSearchPopup();
            } else {
                positionPopup();
            }
        }
    }
    return QLineEdit::eventFilter(watched, event);
}
