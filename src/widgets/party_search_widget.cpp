#include "party_search_widget.h"
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QApplication>
#include <QScreen>
#include <QScrollBar>

PartySearchWidget::PartySearchWidget(QWidget* parent)
    : QLineEdit(parent)
{
    setupUi();
}

PartySearchWidget::~PartySearchWidget() {
    if (m_popupFrame) {
        m_popupFrame->deleteLater();
    }
}

void PartySearchWidget::setupUi() {
    setPlaceholderText("Search Party Account... (Type & press ↓ / Enter)");
    setFixedHeight(28);
    setStyleSheet(
        "QLineEdit {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 4px;"
        "  padding: 2px 8px;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;"
        "}"
        "QLineEdit:focus {"
        "  border: 1.5px solid #2563EB;"
        "  background-color: #EFF6FF;"
        "}"
    );

    m_popupFrame = new QFrame(nullptr, Qt::ToolTip | Qt::FramelessWindowHint);
    m_popupFrame->setAttribute(Qt::WA_ShowWithoutActivating, true);
    m_popupFrame->setFocusPolicy(Qt::NoFocus);
    m_popupFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #CBD5E1;"
        "  border-radius: 6px;"
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
        "  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;"
        "  font-size: 12px;"
        "}"
        "QListWidget::item {"
        "  padding: 6px 10px;"
        "  border-radius: 4px;"
        "  color: #1E293B;"
        "  border-bottom: 1px solid #F1F5F9;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #F8FAFC;"
        "  color: #0F172A;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #2563EB;"
        "  color: #FFFFFF;"
        "  font-weight: bold;"
        "}"
    );

    layout->addWidget(m_listWidget);

    connect(this, &QLineEdit::textChanged, this, &PartySearchWidget::onTextChanged);
    connect(m_listWidget, &QListWidget::itemClicked, this, &PartySearchWidget::onListItemClicked);

    if (parentWidget()) {
        parentWidget()->installEventFilter(this);
    }
}

void PartySearchWidget::setPartyName(const QString& name) {
    m_programmaticChange = true;
    setText(name);
    m_programmaticChange = false;

    QVariantMap p = m_partiesModel.get_party_by_name(name.trimmed());
    if (!p.isEmpty()) {
        m_selectedPartyId = p.value("id").toInt();
        m_selectedPartyData = p;
    }
    closeSearchPopup();
}

void PartySearchWidget::clearSelection() {
    m_programmaticChange = true;
    clear();
    m_programmaticChange = false;
    m_selectedPartyId = -1;
    m_selectedPartyData.clear();
    closeSearchPopup();
}

void PartySearchWidget::openSearchPopup() {
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

void PartySearchWidget::closeSearchPopup() {
    if (m_popupFrame && m_popupFrame->isVisible()) {
        m_popupFrame->hide();
    }
}

void PartySearchWidget::positionPopup() {
    if (!m_popupFrame || !isVisible()) return;
    QPoint globalPos = mapToGlobal(QPoint(0, height() + 1));
    int popupWidth = qMax(width(), 480);
    int itemHeight = 26;
    int popupHeight = qBound(50, m_listWidget->count() * itemHeight + 6, 260);
    m_popupFrame->setGeometry(globalPos.x(), globalPos.y(), popupWidth, popupHeight);
}

void PartySearchWidget::updateResults() {
    QString q = text().trimmed();
    QVariantList results = m_partiesModel.search_parties(q);

    m_listWidget->clear();
    for (const QVariant& item : results) {
        QVariantMap map = item.toMap();
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
            if (!gstin.isEmpty()) display += "GST: " + gstin;
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

void PartySearchWidget::selectCurrentListItem() {
    QListWidgetItem* item = m_listWidget ? m_listWidget->currentItem() : nullptr;
    if (item && !text().trimmed().isEmpty()) {
        onListItemClicked(item);
    } else if (m_listWidget && m_listWidget->count() > 0 && !text().trimmed().isEmpty()) {
        onListItemClicked(m_listWidget->item(0));
    } else {
        closeSearchPopup();
        emit returnPressed();
    }
}

void PartySearchWidget::onListItemClicked(QListWidgetItem* item) {
    if (!item) return;
    QString partyName = item->data(Qt::UserRole).toString();
    QVariantMap partyData = item->data(Qt::UserRole + 1).toMap();

    m_programmaticChange = true;
    setText(partyName);
    m_programmaticChange = false;

    m_selectedPartyId = partyData.value("id").toInt();
    m_selectedPartyData = partyData;

    closeSearchPopup();
    emit partySelected(partyData);
    emit returnPressed();
}

void PartySearchWidget::onTextChanged(const QString& /*text*/) {
    if (m_programmaticChange) return;
    if (hasFocus()) {
        openSearchPopup();
    }
}

void PartySearchWidget::focusInEvent(QFocusEvent* event) {
    QLineEdit::focusInEvent(event);
    selectAll();
    openSearchPopup();
}

void PartySearchWidget::focusOutEvent(QFocusEvent* event) {
    QLineEdit::focusOutEvent(event);
    if (m_popupFrame && !m_popupFrame->underMouse()) {
        closeSearchPopup();
    }
}

void PartySearchWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Tab) {
        if (m_popupFrame && m_popupFrame->isVisible() && m_listWidget->currentItem()) {
            selectCurrentListItem();
        } else {
            closeSearchPopup();
            emit returnPressed();
        }
        event->accept();
        return;
    }

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
            selectCurrentListItem();
            event->accept();
            return;
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

void PartySearchWidget::moveEvent(QMoveEvent* event) {
    QLineEdit::moveEvent(event);
    if (m_popupFrame && m_popupFrame->isVisible()) {
        positionPopup();
    }
}

void PartySearchWidget::resizeEvent(QResizeEvent* event) {
    QLineEdit::resizeEvent(event);
    if (m_popupFrame && m_popupFrame->isVisible()) {
        positionPopup();
    }
}

bool PartySearchWidget::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::Move || event->type() == QEvent::Resize) {
        if (m_popupFrame && m_popupFrame->isVisible()) {
            positionPopup();
        }
    }
    return QLineEdit::eventFilter(watched, event);
}
