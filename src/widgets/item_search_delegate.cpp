#include "item_search_delegate.h"
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QApplication>
#include <QScrollBar>
#include <QScreen>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include "../engine/stock_pipeline.h"

// ============================================================================
// ItemSearchEditor Implementation
// ============================================================================

ItemSearchEditor::ItemSearchEditor(QWidget* parent)
    : QLineEdit(parent)
{
    setStyleSheet(
        "QLineEdit {"
        "  background-color: #EFF6FF;"
        "  color: #0F172A;"
        "  border: 1.5px solid #2563EB;"
        "  border-radius: 4px;"
        "  padding: 2px 6px;"
        "  font-size: 12px;"
        "  font-weight: 700;"
        "  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;"
        "}"
    );

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

    connect(this, &QLineEdit::textChanged, this, &ItemSearchEditor::onTextChanged);
    connect(m_listWidget, &QListWidget::itemClicked, this, &ItemSearchEditor::onListItemClicked);

    if (qApp) {
        qApp->installEventFilter(this);
    }
}

ItemSearchEditor::~ItemSearchEditor() {
    if (qApp) {
        qApp->removeEventFilter(this);
    }
    if (m_popupFrame) {
        m_popupFrame->hide();
        m_popupFrame->deleteLater();
        m_popupFrame = nullptr;
    }
}

void ItemSearchEditor::openSearchPopup() {
    if (m_updating) return;
    if (!isVisible() || !hasFocus()) {
        closeSearchPopup();
        return;
    }
    if (window() && (!window()->isVisible() || window()->isMinimized())) {
        closeSearchPopup();
        return;
    }

    m_updating = true;
    updateResults();

    if (m_listWidget && m_listWidget->count() > 0 && isVisible() && hasFocus()) {
        positionPopup();
        m_popupFrame->show();
        m_popupFrame->raise();
    } else {
        closeSearchPopup();
    }

    m_updating = false;
}

void ItemSearchEditor::closeSearchPopup() {
    if (m_popupFrame && m_popupFrame->isVisible()) {
        m_popupFrame->hide();
    }
}

void ItemSearchEditor::positionPopup() {
    if (!m_popupFrame || !isVisible()) return;
    QPoint globalPos = mapToGlobal(QPoint(0, height() + 2));
    int popupWidth = qMax(width(), 440);
    int itemHeight = 30;
    int popupHeight = qBound(40, m_listWidget->count() * itemHeight + 8, 250);

    QScreen* screen = window() ? window()->screen() : QApplication::primaryScreen();
    if (screen) {
        QRect screenGeo = screen->availableGeometry();
        if (globalPos.y() + popupHeight > screenGeo.bottom()) {
            globalPos.setY(mapToGlobal(QPoint(0, 0)).y() - popupHeight - 2);
        }
        if (globalPos.x() + popupWidth > screenGeo.right()) {
            globalPos.setX(screenGeo.right() - popupWidth - 4);
        }
    }

    m_popupFrame->setGeometry(globalPos.x(), globalPos.y(), popupWidth, popupHeight);
}

void ItemSearchEditor::updateResults() {
    QString q = text();
    QVariantList items = StockPipeline::instance().searchItems(q, 50);

    m_listWidget->clear();
    for (const QVariant& itemVar : items) {
        QVariantMap itemData = itemVar.toMap();
        QString itemName = itemData.value("name").toString();
        double sRate = itemData.value("sale_rate").toDouble();
        double pRate = itemData.value("purchase_rate").toDouble();
        double gst = itemData.value("gst_rate").toDouble();
        double pkg = itemData.value("packing_kg").toDouble();

        QString subText;
        if (pkg > 0) subText += QString("Packing: %1kg").arg(pkg);
        if (sRate > 0) subText += QString(" | Sale: ₹%1").arg(sRate);
        else if (pRate > 0) subText += QString(" | Purc: ₹%1").arg(pRate);
        if (gst > 0) subText += QString(" | GST: %1%").arg(gst);

        QString display = itemName;
        if (!subText.isEmpty()) {
            display += "  (" + subText + ")";
        }

        QListWidgetItem* li = new QListWidgetItem(display, m_listWidget);
        li->setData(Qt::UserRole, itemName);
        li->setData(Qt::UserRole + 1, itemData);
    }

    if (!q.isEmpty() && m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
    } else {
        m_listWidget->setCurrentRow(-1);
    }
}

void ItemSearchEditor::selectCurrentListItem() {
    int row = m_listWidget ? m_listWidget->currentRow() : -1;
    if (row >= 0 && row < m_listWidget->count()) {
        onListItemClicked(m_listWidget->item(row));
    }
}

void ItemSearchEditor::onListItemClicked(QListWidgetItem* item) {
    if (!item) return;
    QString itemName = item->data(Qt::UserRole).toString();
    QVariantMap itemData = item->data(Qt::UserRole + 1).toMap();

    m_programmatic = true;
    setText(itemName);
    m_programmatic = false;

    closeSearchPopup();
    emit itemChosen(itemData);
}

void ItemSearchEditor::onTextChanged(const QString& /*text*/) {
    if (m_programmatic) return;
    if (hasFocus() && isVisible()) {
        openSearchPopup();
    }
}

void ItemSearchEditor::focusInEvent(QFocusEvent* event) {
    QLineEdit::focusInEvent(event);
    selectAll();
}

void ItemSearchEditor::focusOutEvent(QFocusEvent* event) {
    QLineEdit::focusOutEvent(event);
    closeSearchPopup();
}

void ItemSearchEditor::hideEvent(QHideEvent* event) {
    QLineEdit::hideEvent(event);
    closeSearchPopup();
}

void ItemSearchEditor::keyPressEvent(QKeyEvent* event) {
    if (m_popupFrame && m_popupFrame->isVisible()) {
        if (event->key() == Qt::Key_Down) {
            int row = m_listWidget->currentRow();
            if (row < 0) {
                m_listWidget->setCurrentRow(0);
            } else if (row < m_listWidget->count() - 1) {
                m_listWidget->setCurrentRow(row + 1);
            }
            if (m_listWidget->currentItem()) {
                m_listWidget->scrollToItem(m_listWidget->currentItem(), QAbstractItemView::EnsureVisible);
            }
            event->accept();
            return;
        } else if (event->key() == Qt::Key_Up) {
            int row = m_listWidget->currentRow();
            if (row > 0) {
                m_listWidget->setCurrentRow(row - 1);
                m_listWidget->scrollToItem(m_listWidget->currentItem(), QAbstractItemView::EnsureVisible);
            } else if (row == 0) {
                m_listWidget->setCurrentRow(-1);
            }
            event->accept();
            return;
        } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            if (m_listWidget && m_listWidget->currentRow() >= 0) {
                selectCurrentListItem();
                event->accept();
                return;
            } else if (!text().isEmpty() && m_listWidget && m_listWidget->count() > 0) {
                m_listWidget->setCurrentRow(0);
                selectCurrentListItem();
                event->accept();
                return;
            } else {
                closeSearchPopup();
                emit moveNextCell();
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
        } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Tab) {
            emit moveNextCell();
            event->accept();
            return;
        } else if (event->key() == Qt::Key_Backtab) {
            emit movePrevCell();
            event->accept();
            return;
        }
    }
    QLineEdit::keyPressEvent(event);
}

void ItemSearchEditor::moveEvent(QMoveEvent* event) {
    QLineEdit::moveEvent(event);
    if (m_popupFrame && m_popupFrame->isVisible()) {
        positionPopup();
    }
}

void ItemSearchEditor::resizeEvent(QResizeEvent* event) {
    QLineEdit::resizeEvent(event);
    if (m_popupFrame && m_popupFrame->isVisible()) {
        positionPopup();
    }
}

bool ItemSearchEditor::eventFilter(QObject* watched, QEvent* event) {
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

// ============================================================================
// ItemSearchDelegate Implementation
// ============================================================================

ItemSearchDelegate::ItemSearchDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

QWidget* ItemSearchDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    Q_UNUSED(option);

    bool isItemCol = (index.column() == 0);
    if (const QAbstractItemModel* m = index.model()) {
        QString h = m->headerData(index.column(), Qt::Horizontal, Qt::DisplayRole).toString().toLower();
        if (!h.isEmpty()) {
            isItemCol = (h.contains("item") || h.contains("description") || h.contains("commodity") || h.contains("variety") || h.contains("paddy") || h.contains("output") || h.contains("product") || h.contains("particular"));
        }
    }

    if (isItemCol) {
        ItemSearchEditor* editor = new ItemSearchEditor(parent);
        connect(editor, &ItemSearchEditor::itemChosen, this, [this, editor, index](const QVariantMap& data) {
            emit const_cast<ItemSearchDelegate*>(this)->commitData(editor);
            emit const_cast<ItemSearchDelegate*>(this)->closeEditor(editor, QAbstractItemDelegate::NoHint);
            emit const_cast<ItemSearchDelegate*>(this)->stockItemConfigured(index.row(), data);
        });
        connect(editor, &ItemSearchEditor::moveNextCell, this, [this, editor]() {
            emit const_cast<ItemSearchDelegate*>(this)->commitData(editor);
            emit const_cast<ItemSearchDelegate*>(this)->closeEditor(editor, QAbstractItemDelegate::NoHint);
            emit const_cast<ItemSearchDelegate*>(this)->moveNextRequested();
        });
        connect(editor, &ItemSearchEditor::movePrevCell, this, [this, editor]() {
            emit const_cast<ItemSearchDelegate*>(this)->commitData(editor);
            emit const_cast<ItemSearchDelegate*>(this)->closeEditor(editor, QAbstractItemDelegate::NoHint);
            emit const_cast<ItemSearchDelegate*>(this)->movePrevRequested();
        });
        return editor;
    }

    QLineEdit* editor = new QLineEdit(parent);
    editor->setStyleSheet(
        "QLineEdit {"
        "  background-color: #EFF6FF;"
        "  color: #0F172A;"
        "  border: 1.5px solid #2563EB;"
        "  border-radius: 4px;"
        "  font-size: 12px;"
        "  font-weight: 700;"
        "  padding: 2px 6px;"
        "}"
    );
    if (index.column() >= 1) {
        editor->setAlignment(Qt::AlignRight);
    }
    return editor;
}

void ItemSearchDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    QString value = index.model()->data(index, Qt::EditRole).toString();
    if (value.isEmpty()) {
        value = index.model()->data(index, Qt::DisplayRole).toString();
    }
    if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor)) {
        lineEdit->setText(value);
        lineEdit->selectAll();
    }
}

void ItemSearchDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
    if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor)) {
        model->setData(index, lineEdit->text(), Qt::EditRole);
    }
}

void ItemSearchDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& /*index*/) const {
    editor->setGeometry(option.rect);
}

bool ItemSearchDelegate::eventFilter(QObject* object, QEvent* event) {
    QWidget* editor = qobject_cast<QWidget*>(object);
    if (event->type() == QEvent::KeyPress && editor) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        int key = keyEvent->key();

        ItemSearchEditor* itemEditor = qobject_cast<ItemSearchEditor*>(editor);
        if (itemEditor && itemEditor->isPopupVisible()) {
            if (key == Qt::Key_Up || key == Qt::Key_Down || key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Escape) {
                return false; // let ItemSearchEditor process popup navigation and item selection
            }
        }

        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);

        if (key == Qt::Key_Tab || key == Qt::Key_Return || key == Qt::Key_Enter) {
            emit commitData(editor);
            emit closeEditor(editor, QAbstractItemDelegate::NoHint);
            emit moveNextRequested();
            return true;
        } else if (key == Qt::Key_Backtab) {
            emit commitData(editor);
            emit closeEditor(editor, QAbstractItemDelegate::NoHint);
            emit movePrevRequested();
            return true;
        } else if (key == Qt::Key_Left) {
            if (lineEdit && (lineEdit->cursorPosition() == 0 || lineEdit->hasSelectedText() || lineEdit->text().isEmpty())) {
                emit commitData(editor);
                emit closeEditor(editor, QAbstractItemDelegate::NoHint);
                emit movePrevRequested();
                return true;
            }
        } else if (key == Qt::Key_Right) {
            if (lineEdit && (lineEdit->cursorPosition() >= lineEdit->text().length() || lineEdit->hasSelectedText() || lineEdit->text().isEmpty())) {
                emit commitData(editor);
                emit closeEditor(editor, QAbstractItemDelegate::NoHint);
                emit moveNextRequested();
                return true;
            }
        }
    }
    return QStyledItemDelegate::eventFilter(object, event);
}
