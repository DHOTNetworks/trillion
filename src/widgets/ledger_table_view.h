#pragma once

#include <QTableView>
#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QVariantMap>
#include "../models/ledger_statement_model.h"

class LedgerTableModelAdapter : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit LedgerTableModelAdapter(LedgerStatementSideModel* sourceModel, const QString& side, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void toggleSelection(int row);
    QVariantMap getEntry(int row) const;

private:
    LedgerStatementSideModel* m_sourceModel = nullptr;
    QString m_side; // "Dr" or "Cr"
};

class LedgerTableDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit LedgerTableDelegate(const QString& side, QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

private:
    QString m_side; // "Dr" or "Cr"
};

class LedgerTableView : public QTableView {
    Q_OBJECT

public:
    explicit LedgerTableView(const QString& side, LedgerStatementSideModel* sourceModel, QWidget* parent = nullptr);

    void setSourceModel(LedgerStatementSideModel* sourceModel);
    void selectRowIndex(int index);
    int selectedRowIndex() const;

signals:
    void voucherActivated(const QVariantMap& entry);
    void switchSideRequested(const QString& targetSide);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    QString m_side;
    LedgerTableModelAdapter* m_adapter = nullptr;
    LedgerTableDelegate* m_delegate = nullptr;
};
