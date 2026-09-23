#include "account_search_delegate.h"
#include <QEvent>
#include <QKeyEvent>
#include <QTimer>

namespace MahadevERP {

AccountSearchDelegate::AccountSearchDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

QWidget* AccountSearchDelegate::createEditor(QWidget* parent,
                                            const QStyleOptionViewItem& /*option*/,
                                            const QModelIndex& index) const {
    auto* editor = new AccountSearchBox(parent);
    editor->setPlaceholderText("Search party / account...");
    editor->setFrame(false);

    int row = index.row();
    int col = index.column();

    connect(editor, &AccountSearchBox::partySelected, this, [this, row, col](const QString& partyName) {
        emit const_cast<AccountSearchDelegate*>(this)->partyChosen(row, col, partyName);
    });

    QTimer::singleShot(50, editor, [editor]() {
        if (editor) {
            editor->openSearchPopup();
        }
    });

    return editor;
}

void AccountSearchDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    auto* searchBox = qobject_cast<AccountSearchBox*>(editor);
    if (searchBox) {
        QString text = index.model()->data(index, Qt::DisplayRole).toString();
        searchBox->setText(text);
        searchBox->selectAll();
    }
}

void AccountSearchDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
    auto* searchBox = qobject_cast<AccountSearchBox*>(editor);
    if (searchBox && model) {
        QString val = searchBox->text().trimmed();
        model->setData(index, val, Qt::EditRole);
        emit const_cast<AccountSearchDelegate*>(this)->partyChosen(index.row(), index.column(), val);
    }
}

void AccountSearchDelegate::updateEditorGeometry(QWidget* editor,
                                                const QStyleOptionViewItem& option,
                                                const QModelIndex& /*index*/) const {
    editor->setGeometry(option.rect);
}

bool AccountSearchDelegate::eventFilter(QObject* object, QEvent* event) {
    auto* editor = qobject_cast<AccountSearchBox*>(object);
    if (editor && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab) {
            editor->closeSearchPopup();
            emit commitData(editor);
            emit closeEditor(editor);
            return false;
        }
    }
    return QStyledItemDelegate::eventFilter(object, event);
}

} // namespace MahadevERP
