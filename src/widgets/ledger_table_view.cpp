#include "ledger_table_view.h"
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QHeaderView>
#include <QApplication>

// ==================== LedgerTableModelAdapter ====================

LedgerTableModelAdapter::LedgerTableModelAdapter(LedgerStatementSideModel* sourceModel, const QString& side, QObject* parent)
    : QAbstractTableModel(parent)
    , m_sourceModel(sourceModel)
    , m_side(side)
{
    if (m_sourceModel) {
        connect(m_sourceModel, &QAbstractListModel::rowsInserted, this, &LedgerTableModelAdapter::onSourceDataChanged);
        connect(m_sourceModel, &QAbstractListModel::rowsRemoved, this, &LedgerTableModelAdapter::onSourceDataChanged);
        connect(m_sourceModel, &QAbstractListModel::modelReset, this, &LedgerTableModelAdapter::onSourceDataChanged);
        connect(m_sourceModel, &QAbstractListModel::dataChanged, this, &LedgerTableModelAdapter::onSourceDataChanged);
    }
}

int LedgerTableModelAdapter::rowCount(const QModelIndex& parent) const {
    if (parent.isValid() || !m_sourceModel) return 0;
    return m_sourceModel->rowCount();
}

int LedgerTableModelAdapter::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return 5; // 0: Sel, 1: Date, 2: Ref No, 3: Particulars, 4: Amount
}

QVariant LedgerTableModelAdapter::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || !m_sourceModel) return QVariant();
    int row = index.row();
    int col = index.column();

    QModelIndex srcIdx = m_sourceModel->index(row, 0);

    if (role == Qt::CheckStateRole && col == 0) {
        bool selected = m_sourceModel->data(srcIdx, LedgerStatementSideModel::IsSelectedRole).toBool();
        return selected ? Qt::Checked : Qt::Unchecked;
    }

    if (role == Qt::DisplayRole) {
        switch (col) {
        case 0: return QVariant();
        case 1: return m_sourceModel->data(srcIdx, LedgerStatementSideModel::VDateRole);
        case 2: return m_sourceModel->data(srcIdx, LedgerStatementSideModel::RefNoRole);
        case 3: return m_sourceModel->data(srcIdx, LedgerStatementSideModel::ParticularsRole);
        case 4: return m_sourceModel->data(srcIdx, LedgerStatementSideModel::AmountFmtRole);
        default: return QVariant();
        }
    }

    if (role == Qt::TextAlignmentRole) {
        if (col == 0) return static_cast<int>(Qt::AlignCenter);
        if (col == 4) return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
    }

    if (role == Qt::UserRole + 1) {
        // Return full entry map
        return m_sourceModel->get(row);
    }

    return QVariant();
}

QVariant LedgerTableModelAdapter::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0: return "Sel";
        case 1: return "Date";
        case 2: return "Ref No";
        case 3: return "Particulars";
        case 4: return "Amount (₹)";
        default: return QVariant();
        }
    }
    return QVariant();
}

Qt::ItemFlags LedgerTableModelAdapter::flags(const QModelIndex& index) const {
    if (!index.isValid()) return Qt::NoItemFlags;
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (index.column() == 0) {
        f |= Qt::ItemIsUserCheckable;
    }
    return f;
}

void LedgerTableModelAdapter::toggleSelection(int row) {
    if (m_sourceModel && row >= 0 && row < m_sourceModel->rowCount()) {
        m_sourceModel->toggleSelection(row);
        emit dataChanged(index(row, 0), index(row, 4));
    }
}

QVariantMap LedgerTableModelAdapter::getEntry(int row) const {
    if (m_sourceModel && row >= 0 && row < m_sourceModel->rowCount()) {
        return m_sourceModel->get(row);
    }
    return QVariantMap();
}

void LedgerTableModelAdapter::onSourceDataChanged() {
    beginResetModel();
    endResetModel();
}

// ==================== LedgerTableDelegate ====================

LedgerTableDelegate::LedgerTableDelegate(const QString& side, QObject* parent)
    : QStyledItemDelegate(parent)
    , m_side(side)
{
}

void LedgerTableDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    bool isSelected = option.state & QStyle::State_Selected;
    bool hasFocus = option.state & QStyle::State_HasFocus || (option.state & QStyle::State_Active);
    int row = index.row();
    int col = index.column();

    // Background color
    QColor bg;
    if (isSelected) {
        bg = (m_side == "Cr") ? QColor("#DCFCE7") : QColor("#DBEAFE");
    } else {
        bg = (row % 2 == 0) ? QColor("#FFFFFF") : QColor("#F8FAFC");
    }
    painter->fillRect(option.rect, bg);

    // Border for active selection
    if (isSelected) {
        painter->setPen(QPen((m_side == "Cr") ? QColor("#16A34A") : QColor("#2563EB"), 1));
        if (col == 0) {
            painter->drawLine(option.rect.topLeft(), option.rect.bottomLeft());
        }
        if (col == 4) {
            painter->drawLine(option.rect.topRight(), option.rect.bottomRight());
        }
        painter->drawLine(option.rect.topLeft(), option.rect.topRight());
        painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
    } else {
        painter->setPen(QColor("#E2E8F0"));
        painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
    }

    // Column-specific rendering
    if (col == 0) {
        // Checkbox
        bool checked = (index.data(Qt::CheckStateRole) == Qt::Checked);
        int boxSize = 18;
        int bx = option.rect.x() + (option.rect.width() - boxSize) / 2;
        int by = option.rect.y() + (option.rect.height() - boxSize) / 2;
        QRect boxRect(bx, by, boxSize, boxSize);

        painter->setBrush(checked ? ((m_side == "Cr") ? QColor("#16A34A") : QColor("#2563EB")) : Qt::white);
        painter->setPen(QPen(checked ? ((m_side == "Cr") ? QColor("#16A34A") : QColor("#2563EB")) : QColor("#CBD5E1"), 1.5));
        painter->drawRoundedRect(boxRect, 3, 3);

        if (checked) {
            painter->setPen(QPen(Qt::white, 2));
            painter->drawLine(bx + 4, by + 9, bx + 7, by + 13);
            painter->drawLine(bx + 7, by + 13, bx + 14, by + 5);
        }
    } else {
        QString text = index.data(Qt::DisplayRole).toString();
        QFont font = painter->font();
        font.setFamily("Segoe UI");
        font.setPointSize(10);

        if (col == 2) { // Ref No
            font.setBold(true);
            painter->setPen((m_side == "Cr") ? QColor("#16A34A") : QColor("#2563EB"));
        } else if (col == 4) { // Amount
            font.setBold(true);
            painter->setPen((m_side == "Cr") ? QColor("#15803D") : QColor("#1D4ED8"));
        } else {
            font.setBold(false);
            painter->setPen(QColor("#0F172A"));
        }

        painter->setFont(font);
        QRect textRect = option.rect.adjusted(6, 0, -6, 0);
        int align = index.data(Qt::TextAlignmentRole).toInt();
        if (align == 0) align = Qt::AlignLeft | Qt::AlignVCenter;
        painter->drawText(textRect, align, text);
    }

    painter->restore();
}

QSize LedgerTableDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    return QSize(QStyledItemDelegate::sizeHint(option, index).width(), 36);
}

bool LedgerTableDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& /*option*/, const QModelIndex& index) {
    if (event->type() == QEvent::MouseButtonRelease && index.column() == 0) {
        auto* adapter = qobject_cast<LedgerTableModelAdapter*>(model);
        if (adapter) {
            adapter->toggleSelection(index.row());
            return true;
        }
    }
    return false;
}

// ==================== LedgerTableView ====================

LedgerTableView::LedgerTableView(const QString& side, LedgerStatementSideModel* sourceModel, QWidget* parent)
    : QTableView(parent)
    , m_side(side)
{
    m_adapter = new LedgerTableModelAdapter(sourceModel, side, this);
    m_delegate = new LedgerTableDelegate(side, this);

    setModel(m_adapter);
    setItemDelegate(m_delegate);

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setShowGrid(false);
    setAlternatingRowColors(true);
    verticalHeader()->setVisible(false);
    verticalHeader()->setDefaultSectionSize(36);
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet(
        "QTableView {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #E2E8F0;"
        "  border-radius: 6px;"
        "  outline: none;"
        "}"
        "QHeaderView::section {"
        "  background-color: #F8FAFC;"
        "  color: #475569;"
        "  font-weight: bold;"
        "  font-size: 11px;"
        "  padding: 6px 8px;"
        "  border: none;"
        "  border-bottom: 2px solid #CBD5E1;"
        "}"
    );

    // Set Column Widths
    horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    setColumnWidth(0, 36); // Sel
    horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    setColumnWidth(1, 90); // Date
    horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    setColumnWidth(2, 85); // Ref No
    horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch); // Particulars
    horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    setColumnWidth(4, 110); // Amount
}

void LedgerTableView::setSourceModel(LedgerStatementSideModel* sourceModel) {
    if (m_adapter) {
        m_adapter->deleteLater();
    }
    m_adapter = new LedgerTableModelAdapter(sourceModel, m_side, this);
    setModel(m_adapter);
}

void LedgerTableView::selectRowIndex(int index) {
    if (m_adapter && index >= 0 && index < m_adapter->rowCount()) {
        selectRow(index);
        scrollTo(m_adapter->index(index, 0), QAbstractItemView::EnsureVisible);
    }
}

int LedgerTableView::selectedRowIndex() const {
    QModelIndexList sel = selectionModel() ? selectionModel()->selectedRows() : QModelIndexList();
    if (!sel.isEmpty()) {
        return sel.first().row();
    }
    return -1;
}

void LedgerTableView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Space) {
        int row = selectedRowIndex();
        if (row >= 0 && m_adapter) {
            m_adapter->toggleSelection(row);
            event->accept();
            return;
        }
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        int row = selectedRowIndex();
        if (row >= 0 && m_adapter) {
            emit voucherActivated(m_adapter->getEntry(row));
            event->accept();
            return;
        }
    } else if (event->key() == Qt::Key_Left) {
        if (m_side == "Dr") {
            emit switchSideRequested("Cr");
            event->accept();
            return;
        }
    } else if (event->key() == Qt::Key_Right) {
        if (m_side == "Cr") {
            emit switchSideRequested("Dr");
            event->accept();
            return;
        }
    }
    QTableView::keyPressEvent(event);
}

void LedgerTableView::mouseDoubleClickEvent(QMouseEvent* event) {
    QModelIndex idx = indexAt(event->pos());
    if (idx.isValid() && m_adapter) {
        emit voucherActivated(m_adapter->getEntry(idx.row()));
    }
    QTableView::mouseDoubleClickEvent(event);
}
