#include "audit_register_widget.h"
#include "erp_theme.h"
#include "numbering_repair_dialog.h"
#include "../database_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QShortcut>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QDate>

namespace MahadevERP {

AuditRegisterWidget::AuditRegisterWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void AuditRegisterWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(ErpTheme::kCardMargin, 10, ErpTheme::kCardMargin, 10);
    mainLayout->setSpacing(ErpTheme::kGridSpacing);

    // TIER 1: header card
    auto* headerCard = new QFrame(this);
    headerCard->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }");
    auto* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 10, 14, 10);

    auto* titleCol = new QVBoxLayout();
    titleCol->setSpacing(2);
    auto* title = new QLabel("Audit Trail (Edit Log)", headerCard);
    title->setStyleSheet(ErpTheme::headerTitleStyle(16));
    auto* sub = new QLabel("Every voucher posted, edited, deleted — and every refused write. Newest first.", headerCard);
    sub->setStyleSheet(ErpTheme::mutedLabelStyle(11));
    titleCol->addWidget(title);
    titleCol->addWidget(sub);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch(1);

    auto* refreshBtn = new KbdBadgeButton("Refresh", "F5", this);
    refreshBtn->bindShortcut(QKeySequence(Qt::Key_F5));
    connect(refreshBtn, &QPushButton::clicked, this, &AuditRegisterWidget::onRefreshClicked);
    headerLayout->addWidget(refreshBtn);

    // Numbering repair lives here because its evidence (double-posts) and its
    // trail (renumber audit rows) both live in this register.
    auto* repairBtn = new KbdBadgeButton("Repair…", "", this);
    repairBtn->setToolTip("Fix duplicate voucher numbers (renumbers extras, audits everything)");
    connect(repairBtn, &QPushButton::clicked, this, [this]() {
        NumberingRepairDialog::showRepair(this);
        reloadData();
    });
    headerLayout->addWidget(repairBtn);

    auto* backBtn = new KbdBadgeButton("Back", "Esc", this);
    backBtn->bindShortcut(QKeySequence(Qt::Key_Escape));
    connect(backBtn, &QPushButton::clicked, this, &AuditRegisterWidget::backRequested);
    headerLayout->addWidget(backBtn);
    mainLayout->addWidget(headerCard);

    // TIER 2: filter bar
    auto* filterCard = new QFrame(this);
    filterCard->setMinimumHeight(ErpTheme::kFilterBarH);
    filterCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 8px; }"
        "QLabel { color: #475569; font-weight: 700; font-size: 11.5px; border: none; background: transparent; }"
        "QLineEdit, QComboBox { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; font-size: 12px; color: #0F172A; font-weight: 600; }"
        "QLineEdit:focus, QComboBox:focus { border: 2px solid #2563EB; background-color: #FFFFF0; }");
    auto* filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(12, 8, 12, 8);
    filterLayout->setSpacing(12);

    filterLayout->addWidget(new QLabel("Search:", filterCard));
    m_searchEdit = new QLineEdit(filterCard);
    m_searchEdit->setPlaceholderText("Voucher no, party, actor, table...");
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &AuditRegisterWidget::onFilterChanged);
    filterLayout->addWidget(m_searchEdit, 1);

    filterLayout->addWidget(new QLabel("Action:", filterCard));
    m_actionCombo = new QComboBox(filterCard);
    m_actionCombo->addItems({"ALL", "INSERT", "UPDATE", "DELETE", "BLOCKED"});
    connect(m_actionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AuditRegisterWidget::onFilterChanged);
    filterLayout->addWidget(m_actionCombo);
    mainLayout->addWidget(filterCard);

    // TIER 3: table + detail sidecar
    auto* bodyLayout = new QHBoxLayout();
    bodyLayout->setSpacing(ErpTheme::kGridSpacing);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels(
        {"Time", "Action", "Table", "Voucher No", "FY", "Actor", "Summary"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(ErpTheme::kTableRowH);
    m_table->setShowGrid(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(
        "QTableWidget { background-color: #FFFFFF; alternate-background-color: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 8px; gridline-color: #F1F5F9; font-size: 12.5px; color: #0F172A; }"
        "QTableWidget::item { padding: 4px 8px; }"
        "QTableWidget::item:selected { background-color: #2563EB; color: #FFFFFF; font-weight: bold; }"
        "QHeaderView::section { background-color: #0F172A; color: #FFFFFF; font-weight: 800; font-size: 12px; padding: 8px 10px; border: none;  border-right: 1px solid #334155; }");
    connect(m_table, &QTableWidget::currentCellChanged,
            this, [this](int row, int, int, int) { onCurrentRowChanged(row); });
    // Enter jumps to the before/after sidecar (= this register's detail view).
    auto focusDetail = [this]() { if (m_detailView) m_detailView->setFocus(Qt::OtherFocusReason); };
    auto* retSc = new QShortcut(QKeySequence(Qt::Key_Return), m_table);
    retSc->setContext(Qt::WidgetWithChildrenShortcut);
    connect(retSc, &QShortcut::activated, this, focusDetail);
    auto* numSc = new QShortcut(QKeySequence(Qt::Key_Enter), m_table);
    numSc->setContext(Qt::WidgetWithChildrenShortcut);
    connect(numSc, &QShortcut::activated, this, focusDetail);
    bodyLayout->addWidget(m_table, 3);

    m_detailView = new QTextEdit(this);
    m_detailView->setReadOnly(true);
    m_detailView->setMinimumWidth(280);
    m_detailView->setStyleSheet(
        "QTextEdit { background-color: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 8px; font-size: 12px; color: #0F172A; padding: 8px; }");
    bodyLayout->addWidget(m_detailView, 1);
    mainLayout->addLayout(bodyLayout, 1);

    // TIER 4: summary strip
    auto* metricsLayout = new QHBoxLayout();
    metricsLayout->setSpacing(10);
    auto makeCard = [this, metricsLayout](const QString& title, const QString& color) {
        auto* card = new QFrame(this);
        card->setStyleSheet(
            "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; border-left: 4px solid " + color + "; }"
            "QLabel, QWidget { border: none; background: transparent; }");
        auto* cLayout = new QVBoxLayout(card);
        cLayout->setContentsMargins(12, 6, 12, 6);
        cLayout->setSpacing(2);
        auto* t = new QLabel(title, card);
        t->setStyleSheet("font-size: 9.5px; font-weight: 800; color: #64748B; letter-spacing: 0.5px;");
        auto* v = new QLabel("0", card);
        v->setStyleSheet("font-size: 14px; font-weight: 800; color: #0F172A;");
        cLayout->addWidget(t);
        cLayout->addWidget(v);
        metricsLayout->addWidget(card);
        return v;
    };
    m_totalVal = makeCard("AUDIT ROWS (IN FILTER)", ErpTheme::kBlue.name());
    m_blockedVal = makeCard("REFUSED WRITES", ErpTheme::kRed.name());
    m_tablesVal = makeCard("TABLES TOUCHED", ErpTheme::kGreen.name());
    mainLayout->addLayout(metricsLayout);

    // Esc is owned by the Back badge's bindShortcut (single path, no double
    // navigation): no keyPressEvent override on purpose.
    auto* searchSc = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Q), this);
    connect(searchSc, &QShortcut::activated, this, [this]() {
        m_searchEdit->setFocus(Qt::ShortcutFocusReason);
        m_searchEdit->selectAll();
    });
}

void AuditRegisterWidget::reloadData() {
    m_rows = DatabaseManager::instance().executeQuery(
        "SELECT id, ts, actor, action, table_name, row_id, voucher_no, fy_name, before_json, after_json "
        "FROM audit_log ORDER BY id DESC LIMIT 2000;");
    populateTable();
    m_isDirty = false;
}

// Audit timestamps are stored ISO ("yyyy-MM-ddTHH:mm:ss.zzz", UTC). Display
// is always dd-MM-yyyy HH:mm:ss; anything unparseable passes through visibly.
static QString formatAuditTs(const QString& ts) {
    QString t = ts.trimmed();
    const int sep = t.indexOf('T');
    QString datePart = (sep > 0) ? t.left(sep) : t.left(10);
    QString timePart = (sep > 0) ? t.mid(sep + 1).left(8) : QString();
    const QDate d = QDate::fromString(datePart.trimmed(), "yyyy-MM-dd");
    if (!d.isValid()) return t.left(19).replace("T", " ");
    QString out = d.toString("dd-MM-yyyy");
    if (!timePart.isEmpty()) out += " " + timePart;
    return out;
}

QString AuditRegisterWidget::summarize(const QVariantMap& row) {
    const QString action = row.value("action").toString();
    const QString payload = action == "DELETE" ? row.value("before_json").toString()
                                               : row.value("after_json").toString();
    QString party, amount;
    if (!payload.isEmpty()) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8(), &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject o = doc.object();
            party = o.value("party").toString();
            const double amt = o.value("amount").toDouble();
            if (amt > 0.0) amount = QString("₹%1").arg(amt, 0, 'f', 2);
        }
    }
    QString s = action + " " + row.value("table_name").toString();
    if (!party.isEmpty()) s += " • " + party;
    if (!amount.isEmpty()) s += " • " + amount;
    if (action == "BLOCKED") s += " (refused — data unchanged)";
    return s;
}

void AuditRegisterWidget::populateTable() {
    const QString actionFilter = m_actionCombo ? m_actionCombo->currentText() : "ALL";
    const QString query = m_searchEdit ? m_searchEdit->text().trimmed().toLower() : "";

    QVariantList filtered;
    int blocked = 0;
    QSet<QString> tables;
    for (const QVariant& v : m_rows) {
        QVariantMap m = v.toMap();
        if (actionFilter != "ALL" && m.value("action").toString() != actionFilter) continue;
        if (!query.isEmpty()) {
            const QString hay = QString("%1 %2 %3 %4 %5").arg(
                m.value("voucher_no").toString(), m.value("actor").toString(),
                m.value("table_name").toString(), m.value("row_id").toString(),
                m.value("fy_name").toString()).toLower();
            if (!hay.contains(query)) {
                // Also match inside the JSON payloads (party names live there).
                const QString js = (m.value("before_json").toString() + " " +
                                    m.value("after_json").toString()).toLower();
                if (!js.contains(query)) continue;
            }
        }
        filtered.append(m);
        if (m.value("action").toString() == "BLOCKED") ++blocked;
        tables.insert(m.value("table_name").toString());
    }

    m_table->setRowCount(0);
    m_table->setRowCount(filtered.size());
    const QColor fg("#0F172A");
    const QColor neg("#DC2626");
    const QColor pos("#15803D");
    for (int i = 0; i < filtered.size(); ++i) {
        QVariantMap m = filtered[i].toMap();
        const QString action = m.value("action").toString();
        auto cell = [&](const QString& text, const QVariant& id = QVariant()) {
            auto* it = new QTableWidgetItem(text);
            it->setForeground(QBrush(fg)); // explicit: readable on any OS theme
            if (id.isValid()) it->setData(Qt::UserRole, id);
            return it;
        };
        m_table->setItem(i, 0, cell(formatAuditTs(m.value("ts").toString()), m.value("id")));
        auto* actionItem = cell(action);
        if (action == "BLOCKED" || action == "DELETE") actionItem->setForeground(QBrush(neg));
        else if (action == "INSERT") actionItem->setForeground(QBrush(pos));
        m_table->setItem(i, 1, actionItem);
        m_table->setItem(i, 2, cell(m.value("table_name").toString()));
        m_table->setItem(i, 3, cell(m.value("voucher_no").toString()));
        m_table->setItem(i, 4, cell(m.value("fy_name").toString()));
        m_table->setItem(i, 5, cell(m.value("actor").toString()));
        m_table->setItem(i, 6, cell(summarize(m)));
    }

    m_totalVal->setText(QString::number(filtered.size()));
    m_blockedVal->setText(QString::number(blocked));
    m_tablesVal->setText(QString::number(tables.size()));

    if (m_table->rowCount() > 0) {
        m_table->setCurrentCell(0, 0);
        onCurrentRowChanged(0);
    } else {
        m_detailView->clear();
    }
}

void AuditRegisterWidget::onRefreshClicked() {
    reloadData();
}

void AuditRegisterWidget::onFilterChanged() {
    populateTable();
}

void AuditRegisterWidget::onCurrentRowChanged(int currentRow) {
    if (currentRow < 0 || !m_table->item(currentRow, 0)) {
        m_detailView->clear();
        return;
    }
    // Backing row resolved by audit id carried in UserRole — exact even when
    // timestamps repeat within the same second.
    const QVariant rowId = m_table->item(currentRow, 0)->data(Qt::UserRole);
    QVariantMap hit;
    for (const QVariant& v : m_rows) {
        QVariantMap m = v.toMap();
        if (m.value("id") == rowId) {
            hit = m;
            break;
        }
    }
    if (hit.isEmpty()) {
        m_detailView->clear();
        return;
    }
    auto pretty = [](const QString& js) {
        if (js.trimmed().isEmpty()) return QString("—");
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(js.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError)
            return js.left(400);
        return QString::fromUtf8(doc.toJson(QJsonDocument::Indented)).left(2000);
    };
    m_detailView->setPlainText(
        QString("id %1 • %2 • table %3 • row %4 • actor %5\n\n— BEFORE —\n%6\n\n— AFTER —\n%7")
            .arg(hit.value("id").toString(), hit.value("ts").toString(),
                 hit.value("table_name").toString(), hit.value("row_id").toString(),
                 hit.value("actor").toString(), pretty(hit.value("before_json").toString()),
                 pretty(hit.value("after_json").toString())));
}

void AuditRegisterWidget::focusTable() {
    if (m_table->rowCount() > 0) {
        if (m_table->currentRow() < 0) m_table->setCurrentCell(0, 0);
        m_table->setFocus(Qt::OtherFocusReason);
    } else {
        m_searchEdit->setFocus(Qt::OtherFocusReason);
    }
}

} // namespace MahadevERP
