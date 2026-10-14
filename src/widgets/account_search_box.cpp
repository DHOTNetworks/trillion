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
#include <QRegularExpression>
#include <algorithm>

#include "../engine/ledger_pipeline.h"

AccountSearchBox::AccountSearchBox(QWidget* parent)
    : QLineEdit(parent)
{
    setPlaceholderText("Search Party Name / Ledger... (Alt+S)");
    setMinimumHeight(28);
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

void AccountSearchBox::resolveCurrentText() {
    QString q = text().trimmed();
    if (q.isEmpty()) return;

    QVariantList list;
    if (m_searchFn) {
        list = m_searchFn(q);
    } else {
        list = LedgerPipeline::instance().searchLedgers(q, "", 1);
    }

    if (!list.isEmpty()) {
        QVariantMap map = list.first().toMap();
        int partyId = map.value("id").toInt();
        QString partyName = map.value("name").toString();
        if (partyName.isEmpty()) partyName = map.value("party_name").toString();
        m_selectedPartyData = map;
        setParty(partyName, partyId);
        emit partySelected(partyName);
        emit partyDataSelected(m_selectedPartyData);
        emit partySelectedWithId(partyName, partyId);
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
            if (m_listWidget && m_listWidget->count() > 0) {
                selectCurrentListItem();
                event->accept();
                return;
            } else {
                closeSearchPopup();
                resolveCurrentText();
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
        } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            if (m_selectedPartyId <= 0 && !text().trimmed().isEmpty()) {
                resolveCurrentText();
            }
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

static QVariantList rankNameResults(const QVariantList& rawItems, const QString& query, int limit) {
    QString q = query.trimmed().toLower();
    static const QRegularExpression splitRegex(QStringLiteral("[\\s\\-\\/\\.\\[\\]\\(\\)\\,\\&\\_\\+]+"));

    if (q.isEmpty()) {
        QVariantList sorted = rawItems;
        std::sort(sorted.begin(), sorted.end(), [](const QVariant& a, const QVariant& b) {
            QString na = a.toMap().value("name").toString();
            QString nb = b.toMap().value("name").toString();
            return QString::compare(na, nb, Qt::CaseInsensitive) < 0;
        });
        if (limit > 0 && sorted.size() > limit) {
            return sorted.mid(0, limit);
        }
        return sorted;
    }

    struct RankedItem {
        QVariantMap map;
        int tier = 999;
        int wordIndex = 999;
        int nameLength = 0;
    };

    QVector<RankedItem> matches;
    matches.reserve(rawItems.size());

    for (const auto& itemVar : rawItems) {
        QVariantMap map = itemVar.toMap();
        QString name = map.value("name").toString().trimmed();
        if (name.isEmpty()) continue;

        QString nameLower = name.toLower();
        int bStart = nameLower.indexOf('[');
        int bEnd = nameLower.indexOf(']', bStart);
        QString bracketLoc;
        QString cleanNamePart = nameLower;
        if (bStart != -1 && bEnd > bStart) {
            bracketLoc = nameLower.mid(bStart + 1, bEnd - bStart - 1).trimmed();
            cleanNamePart = nameLower.left(bStart).trimmed();
        }

        QStringList nameWords = cleanNamePart.split(splitRegex, Qt::SkipEmptyParts);
        QString cityLower = map.value("city").toString().trimmed().toLower();
        QString gstinLower = map.value("gstin").toString().trimmed().toLower();

        int tier = 999;
        int wordIdx = 999;

        if (nameLower == q || cleanNamePart == q) {
            tier = 0;
            wordIdx = 0;
        } else if (cleanNamePart.startsWith(q)) {
            tier = 1;
            wordIdx = 0;
        } else {
            for (int w = 0; w < nameWords.size(); ++w) {
                if (nameWords[w].startsWith(q)) {
                    tier = (w == 0) ? 1 : (2 + w);
                    wordIdx = w;
                    break;
                }
            }
        }

        if (tier == 999) {
            if (!bracketLoc.isEmpty() && bracketLoc.startsWith(q)) {
                tier = 10;
            } else if (!cityLower.isEmpty() && cityLower.startsWith(q)) {
                tier = 10;
            } else if (cleanNamePart.contains(q)) {
                tier = 20;
            } else if (!bracketLoc.isEmpty() && bracketLoc.contains(q)) {
                tier = 25;
            } else if (!cityLower.isEmpty() && cityLower.contains(q)) {
                tier = 25;
            } else if (!gstinLower.isEmpty() && gstinLower.contains(q)) {
                tier = 30;
            }
        }

        if (tier < 999) {
            RankedItem ri;
            ri.map = map;
            ri.tier = tier;
            ri.wordIndex = wordIdx;
            ri.nameLength = cleanNamePart.length();
            matches.append(ri);
        }
    }

    std::sort(matches.begin(), matches.end(), [](const RankedItem& a, const RankedItem& b) {
        if (a.tier != b.tier) return a.tier < b.tier;
        if (a.wordIndex != b.wordIndex) return a.wordIndex < b.wordIndex;
        if (a.nameLength != b.nameLength) return a.nameLength < b.nameLength;
        return QString::compare(a.map.value("name").toString(), b.map.value("name").toString(), Qt::CaseInsensitive) < 0;
    });

    QVariantList results;
    int count = (limit > 0) ? qMin(matches.size(), limit) : matches.size();
    results.reserve(count);
    for (int i = 0; i < count; ++i) {
        results.append(matches[i].map);
    }
    return results;
}

QVariantList AccountSearchBox::searchBrokers(const QString& query, int limit) {
    auto& db = DatabaseManager::instance();
    static const QString sql = QStringLiteral(
        "SELECT DISTINCT name, city, gstin, id FROM ("
        "  SELECT p.name AS name, p.city AS city, p.gstin AS gstin, p.id AS id FROM parties p "
        "    WHERE p.group_id IN (SELECT id FROM account_groups WHERE code1st = 46 OR name LIKE '%Broker%' OR name LIKE '%Dalal%') "
        "  UNION "
        "  SELECT v.broker_name AS name, '' AS city, '' AS gstin, 0 AS id FROM vouchers v "
        "    WHERE v.broker_name IS NOT NULL AND TRIM(v.broker_name) != '' "
        "  UNION "
        "  SELECT si.broker_name AS name, '' AS city, '' AS gstin, 0 AS id FROM sales_invoices si "
        "    WHERE si.broker_name IS NOT NULL AND TRIM(si.broker_name) != '' "
        "  UNION "
        "  SELECT pi.broker_name AS name, '' AS city, '' AS gstin, 0 AS id FROM purchase_invoices pi "
        "    WHERE pi.broker_name IS NOT NULL AND TRIM(pi.broker_name) != '' "
        "  UNION "
        "  SELECT sc.broker_name AS name, '' AS city, '' AS gstin, sc.broker_id AS id FROM sauda_contracts sc "
        "    WHERE sc.broker_name IS NOT NULL AND TRIM(sc.broker_name) != '' "
        "  UNION "
        "  SELECT ds.broker_name AS name, '' AS city, '' AS gstin, ds.broker_id AS id FROM dalali_settlements ds "
        "    WHERE ds.broker_name IS NOT NULL AND TRIM(ds.broker_name) != '' "
        ") WHERE name IS NOT NULL AND TRIM(name) != ''"
    );
    QVariantList raw = db.executeQuery(sql);
    return rankNameResults(raw, query, limit);
}

QVariantList AccountSearchBox::searchTransporters(const QString& query, int limit) {
    auto& db = DatabaseManager::instance();
    static const QString sql = QStringLiteral(
        "SELECT DISTINCT name, city, gstin, id FROM ("
        "  SELECT p.name AS name, p.city AS city, p.gstin AS gstin, p.id AS id FROM parties p "
        "    WHERE p.group_id IN (SELECT id FROM account_groups WHERE name LIKE '%Transport%') "
        "       OR p.name LIKE '%Transport%' OR p.name LIKE '%Carrier%' OR p.name LIKE '%Roadways%' "
        "       OR p.name LIKE '%Road Lines%' OR p.name LIKE '%Freight%' OR p.name LIKE '%Transporter%' "
        "  UNION "
        "  SELECT td.transport AS name, '' AS city, td.transporter_gstin AS gstin, 0 AS id FROM transport_dispatches td "
        "    WHERE td.transport IS NOT NULL AND TRIM(td.transport) != '' "
        "  UNION "
        "  SELECT si.transport AS name, '' AS city, '' AS gstin, 0 AS id FROM sales_invoices si "
        "    WHERE si.transport IS NOT NULL AND TRIM(si.transport) != '' "
        "  UNION "
        "  SELECT pi.transport AS name, '' AS city, '' AS gstin, 0 AS id FROM purchase_invoices pi "
        "    WHERE pi.transport IS NOT NULL AND TRIM(pi.transport) != '' "
        "  UNION "
        "  SELECT gr.transport AS name, '' AS city, '' AS gstin, 0 AS id FROM gate_register gr "
        "    WHERE gr.transport IS NOT NULL AND TRIM(gr.transport) != '' "
        ") WHERE name IS NOT NULL AND TRIM(name) != ''"
    );
    QVariantList raw = db.executeQuery(sql);
    return rankNameResults(raw, query, limit);
}

void AccountSearchBox::configureForBrokerSearch() {
    setPlaceholderText("Search Broker / Dalal Name... (Enter to select)");
    setSearchFunction([](const QString& q) {
        return searchBrokers(q, 50);
    });
}

void AccountSearchBox::configureForTransportSearch() {
    setPlaceholderText("Search Transport / Freight Carrier... (Enter to select)");
    setSearchFunction([](const QString& q) {
        return searchTransporters(q, 50);
    });
}

