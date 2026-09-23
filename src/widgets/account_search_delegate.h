#pragma once

#include <QStyledItemDelegate>
#include <QVariantMap>
#include "account_search_box.h"

namespace MahadevERP {

class AccountSearchDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit AccountSearchDelegate(QObject* parent = nullptr);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

signals:
    void partyChosen(int row, int column, const QString& partyName);

protected:
    bool eventFilter(QObject* object, QEvent* event) override;
};

} // namespace MahadevERP
