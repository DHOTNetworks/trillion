#pragma once

#include <QStyledItemDelegate>
#include <QLineEdit>
#include <QListWidget>
#include <QFrame>
#include <QVariantMap>
#include <QVariantList>
#include "../models/stock_items_model.h"

class ItemSearchEditor : public QLineEdit {
    Q_OBJECT

public:
    explicit ItemSearchEditor(QWidget* parent = nullptr);
    ~ItemSearchEditor() override;

    void openSearchPopup();
    void closeSearchPopup();
    bool isPopupVisible() const { return m_popupFrame && m_popupFrame->isVisible(); }

signals:
    void itemChosen(const QVariantMap& itemData);
    void moveNextCell();
    void movePrevCell();
    void moveDownCell();
    void moveUpCell();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void moveEvent(QMoveEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onTextChanged(const QString& text);
    void onListItemClicked(QListWidgetItem* item);

private:
    void positionPopup();
    void updateResults();
    void selectCurrentListItem();

    QFrame* m_popupFrame = nullptr;
    QListWidget* m_listWidget = nullptr;
    StockItemsModel m_stockModel;
    bool m_programmatic = false;
    bool m_updating = false;
};

class ItemSearchDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit ItemSearchDelegate(QObject* parent = nullptr);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

signals:
    void stockItemConfigured(int row, const QVariantMap& itemData);
    void moveNextRequested();
    void movePrevRequested();
    void moveDownRequested();
    void moveUpRequested();

protected:
    bool eventFilter(QObject* object, QEvent* event) override;
};
