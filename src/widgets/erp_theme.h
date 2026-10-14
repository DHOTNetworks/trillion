#pragma once

// Single source of truth for the ERP visual system.
// See docs/QTWIDGETS_DESIGN_SYSTEM.md. No widget may hardcode these values —
// include this header and use the constants / builders instead.
//
// Backwards-compatible: the original constants and the three style helpers
// (filterInputStyle/headerTitleStyle/mutedLabelStyle) keep their signatures.
// Everything else below is additive so existing callers keep compiling.

#include <QAbstractItemView>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPalette>
#include <QSize>
#include <QString>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

namespace ErpTheme {

// Palette
inline const QColor kNavy      = QColor("#0F172A"); // headers, primary text
inline const QColor kMuted     = QColor("#64748B"); // secondary text
inline const QColor kFaint     = QColor("#94A3B8"); // hints, disabled
inline const QColor kBlue      = QColor("#2563EB"); // focus, links
inline const QColor kBlueDark  = QColor("#1D4ED8"); // hover, Dr blue
inline const QColor kGreen     = QColor("#16A34A"); // positive / Cr
inline const QColor kGreenDark = QColor("#15803D");
inline const QColor kRed       = QColor("#DC2626"); // negative
inline const QColor kPaper     = QColor("#FFFFFF");
inline const QColor kCanvas    = QColor("#F8FAFC"); // app background
inline const QColor kLine      = QColor("#CBD5E1"); // input borders
inline const QColor kHeaderBg  = QColor("#F1F5F9"); // legacy card fill (do NOT use for table headers)
inline const QColor kFocusBg   = QColor("#FFFFF0"); // focused input fill
inline const QColor kSelected  = QColor("#EFF6FF"); // selected row fill

// Additions (design-system law, were previously hardcoded per-widget)
inline const QColor kCardBorder  = QColor("#E2E8F0"); // T1/T4 card border
inline const QColor kGridLine    = QColor("#E2E8F0"); // table gridlines
inline const QColor kTableAlt    = QColor("#F8FAFC"); // alternating row fill
inline const QColor kLabelSlate  = QColor("#475569"); // filter-bar label text
inline const QColor kAmber       = QColor("#D97706"); // warning / taxable accent
inline const QColor kEmerald     = QColor("#059669"); // master-form primary (ledger update)
inline const QColor kEmeraldDark = QColor("#047857");
inline const QColor kViolet      = QColor("#7C3AED"); // qty / weight accent
inline const QColor kSky         = QColor("#0284C7"); // info / debit accent
inline const QColor kSkyBg       = QColor("#F0F9FF"); // info badge fill
inline const QColor kSkyBorder   = QColor("#BAE6FD");
inline const QColor kGreenBg     = QColor("#F0FDF4"); // ok badge fill
inline const QColor kGreenBorder = QColor("#86EFAC");
inline const QColor kRedBg       = QColor("#FEF2F2"); // danger badge fill
inline const QColor kRedBorder   = QColor("#FECACA");
inline const QColor kEditableBg  = QColor("#F8FAFC"); // editable cell fill

// Geometry (single-slate, DPI-safe via layouts — never setFixed* on fields)
inline constexpr int kFilterBarH   = 52;  // T2 filter bar height
inline constexpr int kHeaderCardH  = 56;  // T1 header card height
inline constexpr int kTableRowH    = 29;  // T3 row height (spec 28-30)
inline constexpr int kButtonH      = 34;  // KbdBadgeButton height
inline constexpr int kRadius       = 6;   // input / card radius
inline constexpr int kCardRadius   = 8;   // card radius
inline constexpr int kGridSpacing  = 8;   // QGridLayout spacing
inline constexpr int kCardMargin   = 12;  // card contents margin

// Standard stylesheets (compose, don't inline literals)
inline QString filterInputStyle() {
    return QString("QLineEdit, QComboBox { background-color: %1; border: 1.5px solid %2;"
                   " border-radius: %3px; padding: 6px 12px; font-size: 13px; color: %4; }"
                   "QLineEdit:focus, QComboBox:focus { border: 2px solid %5; background-color: %6; }")
        .arg(kPaper.name(), kLine.name()).arg(kRadius).arg(kNavy.name(), kBlue.name(), kFocusBg.name());
}

inline QString headerTitleStyle(int px = 16, bool bold = true) {
    return QString("font-size: %1px; font-weight: %2; color: %3; border: none; background: transparent;")
        .arg(px).arg(bold ? 800 : 400).arg(kNavy.name());
}

inline QString mutedLabelStyle(int px = 12) {
    return QString("font-size: %1px; color: %2; border: none; background: transparent;")
        .arg(px).arg(kMuted.name());
}

// ---- Tier builders (all 4 tiers, spec-locked) -------------------------------

/// T1 header card frame style.
inline QString headerCardStyle() {
    return QString("QFrame { background-color: %1; border: 1px solid %2; border-radius: %3px; }"
                   "QLabel, QWidget { border: none; background: transparent; }")
        .arg(kPaper.name(), kCardBorder.name()).arg(kCardRadius);
}

/// T2 filter-bar frame style (includes label + input rules).
inline QString filterBarStyle() {
    return QString("QFrame { background-color: %1; border: 1px solid %2; border-radius: %3px; }"
                   "QLabel { border: none; background: transparent; font-size: 11.5px; font-weight: 700; color: %4; }"
                   "QDateEdit, QComboBox, QLineEdit { background-color: %1; color: %5; border: 1.5px solid %2;"
                   " border-radius: %6px; padding: 4px 8px; font-weight: 700; font-size: 11.5px; }"
                   "QDateEdit:focus, QComboBox:focus, QLineEdit:focus { border: 2px solid %7; background-color: %8; }")
        .arg(kPaper.name(), kLine.name()).arg(kCardRadius).arg(kLabelSlate.name(), kNavy.name())
        .arg(kRadius).arg(kBlue.name(), kFocusBg.name());
}

/// T3 data-surface style. NOTE: navy header per spec (#0F172A + white text).
/// Legacy widgets used gray #F1F5F9 — migrate them to this builder.
inline QString tableStyle() {
    return QString("QTableWidget { background-color: %1; alternate-background-color: %2;"
                   " border: 1px solid %3; border-radius: %4px; gridline-color: %5;"
                   " font-size: 12px; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif;"
                   " color: %6; }"
                   "QTableWidget::item:selected { background-color: %7; color: #FFFFFF; font-weight: 700; }"
                   "QHeaderView::section { background-color: %6; color: #FFFFFF; font-weight: 700;"
                   " font-size: 12px; padding: 8px 8px; border: none; border-right: 1px solid #334155; }")
        .arg(kPaper.name(), kTableAlt.name(), kLine.name()).arg(kCardRadius)
        .arg(kGridLine.name(), kNavy.name(), kBlue.name());
}

/// T3 data-surface style for QTreeWidget surfaces (Balance Sheet / P&L).
/// Same navy-header contract as tableStyle().
inline QString treeStyle() {
    return QString("QTreeWidget { background-color: %1; alternate-background-color: %2;"
                   " border: 1px solid %3; border-radius: %4px;"
                   " font-size: 12px; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif;"
                   " color: %5; outline: none; }"
                   "QTreeWidget::item { padding: 5px 8px; }"
                   "QTreeWidget::item:hover { background-color: %6; }"
                   "QTreeWidget::item:selected { background-color: %7; color: #FFFFFF; font-weight: 700; }"
                   "QHeaderView::section { background-color: %5; color: #FFFFFF; font-weight: 700;"
                   " font-size: 12px; padding: 8px 10px; border: none; border-right: 1px solid #334155; }")
        .arg(kPaper.name(), kTableAlt.name(), kLine.name()).arg(kCardRadius)
        .arg(kNavy.name(), kSelected.name(), kBlue.name());
}

/// T4 summary card frame style with 4px accent stripe.
inline QString metricCardStyle(const QColor& accent) {
    return QString("QFrame { background-color: %1; border: 1px solid %2; border-radius: %3px;"
                   " border-left: 4px solid %4; }"
                   "QLabel, QWidget { border: none; background: transparent; }")
        .arg(kPaper.name(), kCardBorder.name()).arg(kCardRadius).arg(accent.name());
}

inline QString metricTitleStyle() {
    return QString("font-size: 10px; font-weight: 800; color: %1; letter-spacing: 0.5px;"
                   " border: none; background: transparent;")
        .arg(kMuted.name());
}

inline QString metricValueStyle(const QColor& accent = kNavy) {
    return QString("font-size: 16px; font-weight: 800; color: %1; border: none; background: transparent;")
        .arg(accent.name());
}

/// Secondary / ghost button (Period (F2), Export CSV…). KbdBadgeButton stays
/// the default for primary actions; use this only for plain QPushButtons.
inline QString secondaryButtonStyle() {
    return QString("QPushButton { background-color: %1; color: %2; border: 1px solid %3;"
                   " font-weight: 700; font-size: 11px; border-radius: 6px; padding: 4px 10px; }"
                   "QPushButton:hover { background-color: #DBEAFE; color: #1E40AF; }")
        .arg(kSkyBg.name(), kBlueDark.name(), kSkyBorder.name());
}

/// Status badge styles: "info" | "ok" | "danger".
inline QString badgeStyle(const QString& kind) {
    if (kind == "ok")
        return QString("QLabel { background-color: %1; color: %2; border: 1.5px solid %3;"
                       " border-radius: 6px; padding: 4px 10px; font-weight: 800; font-size: 11.5px; }")
            .arg(kGreenBg.name(), kGreen.name(), kGreenBorder.name());
    if (kind == "danger")
        return QString("QLabel { background-color: %1; color: %2; border: 1.5px solid %3;"
                       " border-radius: 6px; padding: 4px 10px; font-weight: 800; font-size: 11.5px; }")
            .arg(kRedBg.name(), kRed.name(), kRedBorder.name());
    return QString("QLabel { background-color: %1; color: %2; border: 1.5px solid %3;"
                   " border-radius: 6px; padding: 4px 10px; font-weight: 800; font-size: 11.5px; }")
        .arg(kSkyBg.name(), kSky.name(), kSkyBorder.name());
}

// ---- Widget factories ---------------------------------------------------------

/// Standard T1 header card: title + subtitle column, stretch, caller adds buttons.
/// Returns the card; title/subtitle labels via out-params for later updates.
inline QFrame* makeHeaderCard(QWidget* parent, const QString& title, const QString& subtitle,
                              QLabel** titleOut = nullptr, QLabel** subtitleOut = nullptr) {
    auto* card = new QFrame(parent);
    card->setStyleSheet(headerCardStyle());
    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(14, 10, 14, 10);
    auto* col = new QVBoxLayout();
    col->setSpacing(2);
    auto* t = new QLabel(title, card);
    t->setStyleSheet(headerTitleStyle(16));
    auto* s = new QLabel(subtitle, card);
    s->setStyleSheet(QString("font-size: 11px; color: %1; font-weight: 600; border: none; background: transparent;")
                         .arg(kMuted.name()));
    col->addWidget(t);
    col->addWidget(s);
    layout->addLayout(col, 1);
    if (titleOut) *titleOut = t;
    if (subtitleOut) *subtitleOut = s;
    return card;
}

/// Standard T2 filter bar card, fixed to spec height. Caller adds controls.
inline QFrame* makeFilterBar(QWidget* parent, QHBoxLayout** layoutOut = nullptr) {
    auto* card = new QFrame(parent);
    card->setFixedHeight(kFilterBarH);
    card->setStyleSheet(filterBarStyle());
    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(12, 6, 12, 6);
    layout->setSpacing(10);
    if (layoutOut) *layoutOut = layout;
    return card;
}

/// Applies the full T3 data-surface contract to a table (replaces ~15 lines
/// of per-widget setup + the #1 dark-mode bug source: missing foregrounds).
inline void styleTable(QTableWidget* table) {
    table->setAlternatingRowColors(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setVisible(false);
    table->verticalHeader()->setDefaultSectionSize(kTableRowH);
    table->setShowGrid(true);
    table->setStyleSheet(tableStyle());
}

/// Standard T4 metric card. Value label returned for live updates; the card
/// frame pointer allows accent re-stripe (balanced vs discrepancy).
inline QLabel* makeMetricCard(QWidget* parent, QHBoxLayout* row, const QString& title,
                              const QString& initVal, const QColor& accent,
                              QFrame** cardOut = nullptr, QLabel** titleOut = nullptr) {
    auto* card = new QFrame(parent);
    card->setStyleSheet(metricCardStyle(accent));
    auto* cLayout = new QVBoxLayout(card);
    cLayout->setContentsMargins(10, 6, 10, 6);
    cLayout->setSpacing(2);
    auto* t = new QLabel(title, card);
    t->setStyleSheet(metricTitleStyle());
    auto* v = new QLabel(initVal, card);
    v->setStyleSheet(metricValueStyle());
    cLayout->addWidget(t);
    cLayout->addWidget(v);
    row->addWidget(card);
    if (cardOut) *cardOut = card;
    if (titleOut) *titleOut = t;
    return v;
}

/// Re-stripes a metric card accent (e.g. balanced green -> discrepancy red).
inline void restripeCard(QFrame* card, const QColor& accent) {
    card->setStyleSheet(metricCardStyle(accent));
}

// ---- Palette / foreground hardening (dark-mode bug class) ---------------------

/// Pins a light slate palette so no surface falls back to a dark OS theme
/// color (light-on-light invisible text). Call once at top of setupUi().
inline void applySlate(QWidget* w) {
    w->setAutoFillBackground(true);
    QPalette p = w->palette();
    p.setColor(QPalette::Window, kCanvas);
    p.setColor(QPalette::Base, kPaper);
    p.setColor(QPalette::Button, kPaper);
    p.setColor(QPalette::AlternateBase, kCanvas);
    p.setColor(QPalette::WindowText, kNavy);
    p.setColor(QPalette::Text, kNavy);
    w->setPalette(p);
}

/// Every QTableWidgetItem must carry an explicit foreground (spec §2).
/// Use these instead of raw `new QTableWidgetItem` + ad-hoc brushes.
inline QTableWidgetItem* textItem(const QString& text,
                                  Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter,
                                  const QColor& fg = kNavy) {
    auto* it = new QTableWidgetItem(text);
    it->setTextAlignment(align);
    it->setForeground(QBrush(fg));
    return it;
}

inline QTableWidgetItem* moneyItem(const QString& text, const QColor& fg = kNavy) {
    return textItem(text, Qt::AlignRight | Qt::AlignVCenter, fg);
}

inline QTableWidgetItem* debitItem(const QString& text) { return moneyItem(text, kSky); }
inline QTableWidgetItem* creditItem(const QString& text) { return moneyItem(text, kGreen); }

// ---- Standard page scaffold -----------------------------------------------------

/// Standard page margins/spacing for every register/report (T1-T4 stack).
inline QVBoxLayout* makePageLayout(QWidget* page) {
    applySlate(page);
    auto* l = new QVBoxLayout(page);
    l->setContentsMargins(kCardMargin, 10, kCardMargin, 10);
    l->setSpacing(kGridSpacing);
    return l;
}

} // namespace ErpTheme
