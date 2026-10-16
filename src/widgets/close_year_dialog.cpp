#include "close_year_dialog.h"
#include "../models/trial_balance_controller.h"
#include "../models/profit_loss_controller.h"
#include "../models/financial_years_model.h"
#include "../models/parties_model.h"
#include "../engine/fiscal_year_helper.h" // F2 lifecycle (setYearStatus/writeSoftWarning)
#include "../engine/fy_close_engine.h" // F3 close procedure (closeYear/reopenYear)
#include "../database_manager.h"
#include "../config/settings_registry.h"
#include <QInputDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QFrame>
#include <QKeyEvent>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <cmath>

namespace MahadevERP {

namespace {
const double kTrialTolerance = 0.01;
const char* kEnginePending = "No standalone step — runs inside the S2 close (FyCloseEngine::closeYear).";

// App-convention styling (matches accounting_period_dialog / new_firm_dialog):
// white window, tinted card frames, letter-spaced section headers, pill
// statuses, styled buttons with min-height.
QLabel* stepTitle(const QString& text, QWidget* parent) {
    QLabel* l = new QLabel(text, parent);
    l->setStyleSheet("font-size: 11px; font-weight: 800; color: #4F46E5; letter-spacing: 0.5px; border: none; background: transparent;");
    return l;
}
QLabel* stepText(const QString& text, QWidget* parent) {
    QLabel* l = new QLabel(text, parent);
    l->setWordWrap(true);
    l->setStyleSheet("font-size: 11px; color: #475569; border: none; background: transparent;");
    return l;
}
void setStatus(QLabel* l, const QString& text, bool good) {
    l->setText(text);
    l->setStyleSheet(QString("font-size: 11px; font-weight: 700; color: %1; background-color: %2; border: none; border-radius: 4px; padding: 3px 8px;")
                          .arg(good ? "#16A34A" : "#B45309", good ? "#ECFDF5" : "#FFFBEB"));
}

QFrame* cardFrame(QWidget* parent) {
    auto* f = new QFrame(parent);
    f->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }");
    return f;
}

enum class ButtonKind { Primary, Secondary, Danger };
QPushButton* styledButton(const QString& text, ButtonKind kind, QWidget* parent) {
    auto* b = new QPushButton(text, parent);
    b->setMinimumHeight(32);
    b->setCursor(Qt::PointingHandCursor);
    const char* css = nullptr;
    if (kind == ButtonKind::Primary)
        css = "QPushButton { background-color: #4F46E5; color: #FFFFFF; border: none; border-radius: 6px; font-size: 12px; font-weight: 700; padding: 4px 14px; }"
              "QPushButton:hover { background-color: #4338CA; } QPushButton:disabled { background-color: #C7D2FE; color: #FFFFFF; }";
    else if (kind == ButtonKind::Danger)
        css = "QPushButton { background-color: #DC2626; color: #FFFFFF; border: none; border-radius: 6px; font-size: 12px; font-weight: 700; padding: 4px 14px; }"
              "QPushButton:hover { background-color: #B91C1C; } QPushButton:disabled { background-color: #FCA5A5; color: #FFFFFF; }";
    else
        css = "QPushButton { background-color: #FFFFFF; color: #334155; border: 1px solid #CBD5E1; border-radius: 6px; font-size: 12px; font-weight: 700; padding: 4px 14px; }"
              "QPushButton:hover { background-color: #F1F5F9; } QPushButton:disabled { color: #94A3B8; border-color: #E2E8F0; }";
    b->setStyleSheet(QLatin1String(css));
    return b;
}

// File-local helpers (defined before first use — C++ has no hoisting).
static QString closeActor() {
    const QString a = DatabaseManager::instance()
                          .getSetting(SettingsKeys::audit_actor(), "local")
                          .trimmed();
    return a.isEmpty() ? "local" : a;
}

static QVariantMap selectedYearRow(QComboBox* combo) {
    QVariantMap out;
    if (!combo || combo->currentIndex() < 0) return out;
    const QString yn = combo->currentData().toString();
    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT year_name, start_date, end_date, is_locked FROM financial_years WHERE year_name = ? LIMIT 1;", {yn});
    if (!rows.isEmpty()) out = rows.first().toMap();
    return out;
}
} // namespace

CloseYearDialog::CloseYearDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Close Financial Year — Guided Checklist");
    setModal(true);
    setMinimumSize(680, 640);
    setStyleSheet("CloseYearDialog { background-color: #F8FAFC; }");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(18, 16, 18, 16);
    root->setSpacing(12);

    // Header card: icon + title + subtitle (app convention).
    {
        auto* header = cardFrame(this);
        auto* hLay = new QHBoxLayout(header);
        hLay->setContentsMargins(14, 12, 14, 12);
        hLay->setSpacing(12);
        auto* icon = new QLabel("📋", header);
        icon->setStyleSheet("background-color: #EFF6FF; border: 1px solid #BFDBFE; border-radius: 8px; font-size: 18px; padding: 6px 10px;");
        hLay->addWidget(icon);
        auto* vLay = new QVBoxLayout();
        vLay->setSpacing(2);
        auto* title = new QLabel("Close Financial Year", header);
        title->setStyleSheet("font-size: 15px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
        auto* sub = new QLabel("Guided year-end checklist — trial, closing journal, snapshot, sign-offs, lock.", header);
        sub->setStyleSheet("font-size: 11px; color: #64748B; border: none; background: transparent;");
        vLay->addWidget(title);
        vLay->addWidget(sub);
        hLay->addLayout(vLay, 1);
        root->addWidget(header);
    }

    // Year selector card.
    {
        auto* card = cardFrame(this);
        auto* lay = new QVBoxLayout(card);
        lay->setContentsMargins(14, 12, 14, 12);
        lay->setSpacing(8);
        lay->addWidget(stepTitle("FINANCIAL YEAR", this));
        m_yearCombo = new QComboBox(this);
        m_yearCombo->setMinimumHeight(32);
        m_yearCombo->setStyleSheet("QComboBox { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 6px; font-size: 12px; font-weight: 600; color: #0F172A; padding: 4px 8px; }");
        lay->addWidget(m_yearCombo);
        root->addWidget(card);
    }

    m_statusLabel = stepText("", this);
    root->addWidget(m_statusLabel);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background-color: transparent; border: none; }");
    auto* body = new QWidget();
    auto* form = new QVBoxLayout(body);
    form->setSpacing(10);

    // S1 — trial balanced (LIVE read).
    {
        auto* card = cardFrame(this);
        auto* lay = new QVBoxLayout(card);
        lay->setContentsMargins(14, 12, 14, 12);
        lay->setSpacing(6);
        lay->addWidget(stepTitle("S1 · TRIAL BALANCED", this));
        m_s1Detail = stepText("", this);
        lay->addWidget(m_s1Detail);
        m_s1Status = stepText("", this);
        lay->addWidget(m_s1Status);
        m_s1RunBtn = styledButton("Run trial check", ButtonKind::Secondary, this);
        connect(m_s1RunBtn, &QPushButton::clicked, this, &CloseYearDialog::onRunTrialCheck);
        lay->addWidget(m_s1RunBtn);
        form->addWidget(card);
    }

    // S2 — closing journal via the F3 engine (trial gate → P&L net →
    // ONE reversible journal → next-FY ensure → snapshot → lock).
    // Capital ledger is firm-specific (no generic seed) — picked here.
    {
        auto* card = cardFrame(this);
        auto* lay = new QVBoxLayout(card);
        lay->setContentsMargins(14, 12, 14, 12);
        lay->setSpacing(6);
        lay->addWidget(stepTitle("S2 · CLOSING JOURNAL (P&L → CAPITAL)", this));
        m_s2Detail = stepText("", this);
        lay->addWidget(m_s2Detail);
        auto* capRow = new QHBoxLayout();
        capRow->addWidget(stepTitle("Capital ledger:", this));
        m_capitalBox = new AccountSearchBox(this);
        m_capitalBox->setPlaceholderText("Type to search capital ledgers…");
        m_capitalBox->setSearchFunction([](const QString& q) {
            return PartiesModel().search_parties(q);
        });
        capRow->addWidget(m_capitalBox, 1);
        lay->addLayout(capRow);
        m_s2Status = stepText("", this);
        setStatus(m_s2Status, "Not run yet — pick a capital ledger, then post.", true);
        lay->addWidget(m_s2Status);
        m_s2PostBtn = styledButton("Post closing journal", ButtonKind::Primary, this);
        m_s2PostBtn->setToolTip("Runs the full F3 close: trial gate → journal → snapshot → lock.");
        connect(m_s2PostBtn, &QPushButton::clicked, this, &CloseYearDialog::onCloseYear);
        lay->addWidget(m_s2PostBtn);
        form->addWidget(card);
    }

    // S3 — openings snapshot (engine-owned mutation).
    {
        auto* card = cardFrame(this);
        auto* lay = new QVBoxLayout(card);
        lay->setContentsMargins(14, 12, 14, 12);
        lay->setSpacing(6);
        lay->addWidget(stepTitle("S3 · OPENINGS SNAPSHOT", this));
        m_s3Detail = stepText("Balance-sheet balances per ledger → fy_opening_balances.", this);
        lay->addWidget(m_s3Detail);
        m_s3Status = stepText("", this);
        lay->addWidget(m_s3Status);
        m_s3SnapBtn = styledButton("Snapshot openings", ButtonKind::Secondary, this);
        m_s3SnapBtn->setEnabled(false);
        m_s3SnapBtn->setToolTip(kEnginePending);
        lay->addWidget(m_s3SnapBtn);
        form->addWidget(card);
    }

    // S4 — firm-custom steps (registry JSON; session-only sign-off until
    // the engine persists/audits completions).
    {
        auto* card = cardFrame(this);
        auto* lay = new QVBoxLayout(card);
        lay->setContentsMargins(14, 12, 14, 12);
        lay->setSpacing(6);
        lay->addWidget(stepTitle("S4 · FIRM SIGN-OFFS", this));
        m_s4Box = new QWidget(this);
        auto* s4Layout = new QVBoxLayout(m_s4Box);
        s4Layout->setContentsMargins(0, 0, 0, 0);
        s4Layout->setSpacing(4);
        lay->addWidget(m_s4Box);
        m_s4Status = stepText("", this);
        lay->addWidget(m_s4Status);
        form->addWidget(card);
    }

    // S5 — lock (engine-owned mutation; uses the existing writeBlockReason
    // message path once the engine flips is_locked).
    {
        auto* card = cardFrame(this);
        auto* lay = new QVBoxLayout(card);
        lay->setContentsMargins(14, 12, 14, 12);
        lay->setSpacing(6);
        lay->addWidget(stepTitle("S5 · LOCK YEAR", this));
        m_s5Status = stepText("", this);
        lay->addWidget(m_s5Status);
        m_s5LockBtn = styledButton("Lock year", ButtonKind::Primary, this);
        m_s5LockBtn->setToolTip("Transitions open/soft_closed → locked via the F2 lifecycle engine.");
        connect(m_s5LockBtn, &QPushButton::clicked, this, &CloseYearDialog::onLockYear);
        lay->addWidget(m_s5LockBtn);
        form->addWidget(card);
    }

    // Reopen via the F3 engine (reverses ONLY the closing voucher, then
    // unlocks — audited inside the engine).
    {
        auto* card = cardFrame(this);
        auto* lay = new QVBoxLayout(card);
        lay->setContentsMargins(14, 12, 14, 12);
        lay->setSpacing(6);
        lay->addWidget(stepTitle("REOPEN (LOCKED YEARS)", this));
        m_reopenBtn = styledButton("Reverse closing voucher & unlock", ButtonKind::Danger, this);
        m_reopenBtn->setToolTip("FyCloseEngine::reopenYear: reversal mirror + unlock + key clear.");
        connect(m_reopenBtn, &QPushButton::clicked, this, &CloseYearDialog::onReopenYear);
        lay->addWidget(m_reopenBtn);
        form->addWidget(card);
    }

    scroll->setWidget(body);
    root->addWidget(scroll, 1);

    auto* closeRow = new QHBoxLayout();
    closeRow->addStretch(1);
    auto* closeBtn = styledButton("Close", ButtonKind::Secondary, this);
    closeBtn->setMinimumHeight(34);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    closeRow->addWidget(closeBtn);
    root->addLayout(closeRow);

    m_trial = new TrialBalanceController(this);
    m_pnl = new ProfitLossController(nullptr, this);
    m_years = new FinancialYearsModel(this);
    m_years->reload_data();

    for (const QVariant& v : m_years->get_all_years()) {
        const QVariantMap m = v.toMap();
        const QString yn = m.value("year_name").toString();
        if (!yn.isEmpty()) m_yearCombo->addItem(yn, yn);
    }
    connect(m_yearCombo, &QComboBox::currentIndexChanged, this, &CloseYearDialog::onYearChanged);
    refreshAll();
    m_yearCombo->setFocus();
}

void CloseYearDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        reject();
        event->accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

void CloseYearDialog::onYearChanged(int) {
    refreshAll();
}

void CloseYearDialog::onRunTrialCheck() {
    refreshTrialStep();
}

void CloseYearDialog::onCloseYear() {
    const QVariantMap y = selectedYearRow(m_yearCombo);
    if (y.isEmpty()) return;
    const QString fyName = y.value("year_name").toString();
    const QString capName = m_capitalBox ? m_capitalBox->currentPartyName().trimmed() : QString();
    if (capName.isEmpty()) {
        setStatus(m_s2Status, "Pick the capital ledger first (P&L transfers into it).", false);
        if (m_capitalBox) m_capitalBox->setFocus();
        return;
    }
    const FyCloseResult r = FyCloseEngine::closeYear(fyName, capName, closeActor());
    if (!r.ok) {
        setStatus(m_s2Status, "Close refused: " + r.error, false);
        return;
    }
    const QString via = r.closingVoucherNo.isEmpty() ? "no journal (net zero)"
                                                     : "voucher " + r.closingVoucherNo;
    setStatus(m_s2Status,
              QString("Closed: P&L ₹%1 → %2, %3 snapshot rows.")
                  .arg(QString::number(r.plNet, 'f', 2), via,
                       QString::number(r.snapshotRows)),
              true);
    reloadYearCombo(fyName);
    refreshAll();
}

void CloseYearDialog::reloadYearCombo(const QString& keepYear) {
    m_years->reload_data();
    m_yearCombo->blockSignals(true);
    m_yearCombo->clear();
    int keep = -1;
    for (const QVariant& v : m_years->get_all_years()) {
        const QString yn = v.toMap().value("year_name").toString();
        if (yn.isEmpty()) continue;
        if (yn == keepYear) keep = m_yearCombo->count();
        m_yearCombo->addItem(yn, yn);
    }
    m_yearCombo->blockSignals(false);
    m_yearCombo->setCurrentIndex(keep >= 0 ? keep : 0);
}

void CloseYearDialog::onLockYear() {
    const QVariantMap y = selectedYearRow(m_yearCombo);
    if (y.isEmpty()) return;
    // Checklist semantics: never lock an unbalanced year.
    refreshTrialStep();
    m_trial->reload();
    if (std::fabs(m_trial->totals().difference) >= kTrialTolerance) {
        setStatus(m_s5Status, "Blocked: trial is not balanced — lock refused.", false);
        return;
    }
    bool ok = false;
    const QString reason = QInputDialog::getText(
        this, "Lock " + y.value("year_name").toString(),
        "Reason (audited):", QLineEdit::Normal,
        "Year-end close checklist complete", &ok);
    if (!ok) return;
    QString err;
    if (!FiscalYearHelper::setYearStatus(y.value("year_name").toString(), "locked",
                                         closeActor(),
                                         reason.trimmed().isEmpty() ? "Year-end close" : reason.trimmed(),
                                         &err)) {
        setStatus(m_s5Status, "Lock refused: " + err, false);
        return;
    }
    reloadYearCombo(y.value("year_name").toString());
    refreshAll();
}

void CloseYearDialog::onReopenYear() {
    const QVariantMap y = selectedYearRow(m_yearCombo);
    if (y.isEmpty()) return;
    // F3 engine: reverses ONLY the closing voucher, then unlocks.
    const FyCloseResult r = FyCloseEngine::reopenYear(y.value("year_name").toString(), closeActor());
    if (!r.ok) {
        setStatus(m_s5Status, "Reopen refused: " + r.error, false);
        return;
    }
    reloadYearCombo(y.value("year_name").toString());
    refreshAll();
    setStatus(m_s5Status, "Reopened: closing voucher reversed, year unlocked.", true);
}

void CloseYearDialog::refreshAll() {
    const QVariantMap y = selectedYearRow(m_yearCombo);
    if (y.isEmpty()) {
        m_statusLabel->setText("No financial year selected.");
        return;
    }
    const bool locked = y.value("is_locked").toInt() != 0;
    m_statusLabel->setText(QString("%1 (%2 to %3)%4")
                                .arg(y.value("year_name").toString(),
                                     y.value("start_date").toString(),
                                     y.value("end_date").toString(),
                                     locked ? " — LOCKED" : ""));
    refreshTrialStep();

    // S2 read: what the closing journal would transfer (engine posts it).
    // Status follows the recorded closing voucher (S3 key), not a stale
    // pending note — refreshAll must not clobber a just-posted result.
    m_pnl->reload(y.value("start_date").toString(), y.value("end_date").toString());
    const double net = m_pnl->netProfit();
    m_s2Detail->setText(QString("Would transfer %1 ₹%2 to capital (one reversible voucher).")
                            .arg(net >= 0 ? "profit" : "loss")
                            .arg(QString::number(std::fabs(net), 'f', 2)));
    m_s2PostBtn->setEnabled(!locked);
    if (m_capitalBox) m_capitalBox->setEnabled(!locked);

    // S3 state: the engine records the closing voucher under
    // fy_close_voucher/<fy>; snapshot rows land with it inside closeYear.
    const QString closeVch = DatabaseManager::instance()
                                  .getSetting(SettingsKeys::fy_close_voucher(
                                      y.value("year_name").toString()))
                                  .trimmed();
    setStatus(m_s3Status,
              closeVch.isEmpty() ? "No closing voucher recorded — snapshot runs inside the S2 close."
                                 : "Closing voucher " + closeVch + " posted; openings snapshotted with it.",
              !closeVch.isEmpty());
    m_s3SnapBtn->setToolTip("Snapshot runs inside the S2 close (FyCloseEngine::closeYear).");
    refreshCustomSteps();

    // S5 reads the F2 lifecycle: locked shows the guard state, soft-closed
    // surfaces its warning, open is lockable. Lock/reopen call
    // setYearStatus (F2 engine); S1-balance gates the lock click.
    const QString warn = FiscalYearHelper::writeSoftWarning(y.value("start_date").toString());
    if (locked) {
        setStatus(m_s5Status, "Year is locked (writes refused).", true);
    } else if (!warn.isEmpty()) {
        setStatus(m_s5Status, warn, false);
    } else {
        setStatus(m_s5Status, "Open — lock once S1–S4 pass.", true);
    }
    m_s5LockBtn->setEnabled(!locked);
    m_reopenBtn->setEnabled(locked);
}

void CloseYearDialog::refreshTrialStep() {
    const QVariantMap y = selectedYearRow(m_yearCombo);
    if (y.isEmpty()) return;
    m_trial->setDateRange(QDate::fromString(y.value("start_date").toString(), "yyyy-MM-dd"),
                          QDate::fromString(y.value("end_date").toString(), "yyyy-MM-dd"));
    m_trial->reload();
    const auto& t = m_trial->totals();
    const double diff = std::fabs(t.difference);
    m_s1Detail->setText(QString("Close Dr ₹%1 — Close Cr ₹%2 — difference ₹%3 (tolerance ₹%4).")
                            .arg(QString::number(t.totalCloseDebit, 'f', 2),
                                 QString::number(t.totalCloseCredit, 'f', 2),
                                 QString::number(diff, 'f', 2),
                                 QString::number(kTrialTolerance, 'f', 2)));
    setStatus(m_s1Status, diff < kTrialTolerance ? "Balanced — may proceed."
                                                : "NOT balanced — close is blocked until the trial balances.",
              diff < kTrialTolerance);
}

void CloseYearDialog::refreshCustomSteps() {
    // S4 definitions: close_steps/custom (firm-edited) else
    // close_steps/<firm.business_type>; absent both = no extra steps.
    while (!m_s4Checks.isEmpty()) {
        delete m_s4Checks.take(m_s4Checks.firstKey());
    }
    QLayoutItem* li;
    while ((li = m_s4Box->layout()->takeAt(0)) != nullptr) delete li;

    QString raw = DatabaseManager::instance()
                      .getSetting(SettingsKeys::close_steps("custom"))
                      .trimmed();
    const QString pkg = DatabaseManager::instance()
                            .getSetting(SettingsKeys::firm_business_type())
                            .trimmed();
    if (raw.isEmpty() && !pkg.isEmpty())
        raw = DatabaseManager::instance().getSetting(SettingsKeys::close_steps(pkg)).trimmed();
    if (raw.isEmpty())
        raw = SettingsKeys::close_steps_default(pkg); // W9-A built-in fallback
    bool any = false;
    if (!raw.isEmpty()) {
        const QJsonArray arr = QJsonDocument::fromJson(raw.toUtf8()).array();
        for (const QJsonValue& v : arr) {
            const QJsonObject o = v.toObject();
            const QString id = o.value("id").toString().trimmed();
            const QString title = o.value("title").toString().trimmed();
            if (id.isEmpty() || title.isEmpty()) continue; // fail loud on shape, skip bad rows
            const QString note = o.value("note").toString().trimmed();
            auto* cb = new QCheckBox(title + (note.isEmpty() ? "" : " — " + note), m_s4Box);
            cb->setStyleSheet("QCheckBox { font-size: 12px; color: #0F172A; spacing: 8px; }");
            m_s4Box->layout()->addWidget(cb);
            m_s4Checks.insert(id, cb);
            any = true;
        }
    }
    setStatus(m_s4Status,
              any ? "Sign-off is session-only until the engine persists/audits completions."
                  : "No additional steps registered for this firm — nothing to sign off.",
              true);
}

} // namespace MahadevERP
