#include "item_search_delegate.h"
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QApplication>
#include <QScrollBar>

// ============================================================================
// ItemSearchEditor Implementation
// ============================================================================

ItemSearchEditor::ItemSearchEditor(QWidget* parent)
    : QLineEdit(parent)
{
    setStyleSheet(
        "QLineEdit {"
        "  background-color: #FFFFFF;"
        "  color: #000000;"
        "  border: 1.5px solid #0000CC;"
        "  border-radius: 0px;"
        "  padding: 1px 4px;"
        "  font-size: 11.5px;"
        "  font-weight: 700;"
        "}"
    );

    m_popupFrame = new QFrame(nullptr, Qt::ToolTip | Qt::FramelessWindowHint);
    m_popupFrame->setAttribute(Qt::WA_ShowWithoutActivating, true);
    m_popupFrame->setFocusPolicy(Qt::NoFocus);
    m_popupFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1.5px solid #0000CC;"
        "  border-radius: 0px;"
        "}"
    );

    QVBoxLayout* layout = new QVBoxLayout(m_popupFrame);
    layout->setContentsMargins(1, 1, 1, 1);
    layout->setSpacing(0);

    m_listWidget = new QListWidget(m_popupFrame);
    m_listWidget->setFocusPolicy(Qt::NoFocus);
    m_listWidget->setStyleSheet(
        "QListWidget {"
        "  border: none;"
        "  background-color: #FFFFFF;"
        "  font-family: 'Segoe UI', -apple-system, sans-serif;"
        "  font-size: 11.5px;"
        "}"
        "QListWidget::item {"
        "  padding: 4px 8px;"
        "  border-radius: 0px;"
        "  color: #000000;"
        "  border-bottom: 1px solid #EEEEEE;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #FFFFDD;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #0066CC;"
        "  color: #FFFFFF;"
        "  font-weight: bold;"
        "}"
    );
    layout->addWidget(m_listWidget);

    connect(this, &QLineEdit::textChanged, this, &ItemSearchEditor::onTextChanged);
    connect(m_listWidget, &QListWidget::itemClicked, this, &ItemSearchEditor::onListItemClicked);
}

ItemSearchEditor::~ItemSearchEditor() {
    if (m_popupFrame) {
        m_popupFrame->deleteLater();
    }
}

void ItemSearchEditor::openSearchPopup() {
    if (m_updating) return;
    m_updating = true;

    updateResults();
    if (m_listWidget->count() > 0 && isVisible()) {
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
    QPoint globalPos = mapToGlobal(QPoint(0, height() + 1));
    int popupWidth = qMax(width(), 420);
    int itemHeight = 26;
    int popupHeight = qBound(50, m_listWidget->count() * itemHeight + 6, 240);
    m_popupFrame->setGeometry(globalPos.x(), globalPos.y(), popupWidth, popupHeight);
}

void ItemSearchEditor::updateResults() {
    QString q = text().trimmed();
    QStringList allItems = m_stockModel.get_items_list();

    m_listWidget->clear();
    for (const QString& itemName : allItems) {
        if (!q.isEmpty() && !itemName.contains(q, Qt::CaseInsensitive)) {
            continue;
        }
        QVariantMap itemData = m_stockModel.get_item_by_name(itemName);
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
    QListWidgetItem* item = m_listWidget->currentItem();
    if (item) {
        onListItemClicked(item);
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
    if (hasFocus()) {
        openSearchPopup();
    }
}

void ItemSearchEditor::focusInEvent(QFocusEvent* event) {
    QLineEdit::focusInEvent(event);
    selectAll();
    openSearchPopup();
}

void ItemSearchEditor::focusOutEvent(QFocusEvent* event) {
    QLineEdit::focusOutEvent(event);
    if (m_popupFrame && !m_popupFrame->underMouse()) {
        closeSearchPopup();
    }
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
            if (text().trimmed().isEmpty() || m_listWidget->currentRow() < 0) {
                closeSearchPopup();
                emit moveNextCell();
                event->accept();
                return;
            } else {
                selectCurrentListItem();
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

// ============================================================================
// ItemSearchDelegate Implementation
// ============================================================================

ItemSearchDelegate::ItemSearchDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

QWidget* ItemSearchDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    Q_UNUSED(option);

    if (index.column() == 1) { // Stock Item Name
        ItemSearchEditor* editor = new ItemSearchEditor(parent);
        connect(editor, &ItemSearchEditor::itemChosen, this, [this, index](const QVariantMap& data) {
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
        "  background-color: #FFFFDD;"
        "  color: #000000;"
        "  border: 1.5px solid #0000CC;"
        "  border-radius: 0px;"
        "  font-size: 11.5px;"
        "  font-weight: 700;"
        "  padding: 1px 4px;"
        "}"
    );
    if (index.column() >= 2 && index.column() <= 7) {
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
