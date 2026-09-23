#include "accounting_period_dialog.h"
#include "../database_manager.h"
#include "../engine/accounting_engine.h"
#include "../engine/fiscal_year_helper.h"
#include "../services/accounting_date_service.h"
#include <QFrame>
#include <QPainter>
#include <QApplication>
#include <QHeaderView>
#include <QGraphicsDropShadowEffect>
#include <QShortcut>

// ------------------------------------------------------------------------------------------------
// DateListWidget implementation
// ------------------------------------------------------------------------------------------------
DateListWidget::DateListWidget(bool isStartColumn, QWidget* parent)
    : QListWidget(parent)
    , m_isStartColumn(isStartColumn)
{
    setStyleSheet(
        "QListWidget {"
        "  background-color: #F8FAFC;"
        "  border: 1px solid #E2E8F0;"
        "  border-radius: 8px;"
        "  outline: none;"
        "  padding: 4px;"
        "}"
        "QListWidget:focus {"
        "  border: 1.5px solid #3B82F6;"
        "  background-color: #F0FDF4;"
        "}"
        "QListWidget::item {"
        "  background-color: transparent;"
        "  border-radius: 6px;"
        "  padding: 0px;"
        "  margin-bottom: 4px;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #F1F5F9;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #ECFDF5;"
        "}"
    );
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSelectionMode(QAbstractItemView::SingleSelection);
}

void DateListWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Right && m_isStartColumn) {
        emit switchColumnRequested();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Left && !m_isStartColumn) {
        emit switchColumnRequested();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        AccountingPeriodDialog* dlg = qobject_cast<AccountingPeriodDialog*>(window());
        if (dlg) {
            dlg->applyPeriod();
            event->accept();
            return;
        }
    }
    if (event->key() == Qt::Key_Escape) {
        AccountingPeriodDialog* dlg = qobject_cast<AccountingPeriodDialog*>(window());
        if (dlg) {
            dlg->reject();
            event->accept();
            return;
        }
    }

    QListWidget::keyPressEvent(event);
}

void DateListWidget::focusInEvent(QFocusEvent* event) {
    QListWidget::focusInEvent(event);
    if (currentRow() < 0 && count() > 0) {
        setCurrentRow(0);
    }
}

// ------------------------------------------------------------------------------------------------
// AccountingPeriodDialog implementation
// ------------------------------------------------------------------------------------------------
AccountingPeriodDialog::AccountingPeriodDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFixedWidth(560);
    setFixedHeight(600);

    new QShortcut(QKeySequence(Qt::Key_Escape), this, SLOT(reject()));
    new QShortcut(QKeySequence(Qt::Key_F2), this, SLOT(applyPeriod()));

    if (qApp) {
        qApp->installEventFilter(this);
    }

    setupUi();
    loadYearsFromDatabase();
}

AccountingPeriodDialog::~AccountingPeriodDialog() {
    if (qApp) {
        qApp->removeEventFilter(this);
    }
}

void AccountingPeriodDialog::setupUi() {
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    // Main Card Frame
    QFrame* cardFrame = new QFrame(this);
    cardFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1.5px solid #047857;"
        "  border-radius: 12px;"
        "}"
    );

    QVBoxLayout* mainLayout = new QVBoxLayout(cardFrame);
    mainLayout->setContentsMargins(20, 18, 20, 18);
    mainLayout->setSpacing(12);

    // ================= 1. HEADER =================
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(12);

    QLabel* iconLabel = new QLabel(cardFrame);
    iconLabel->setFixedSize(40, 40);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setText("📅");
    iconLabel->setStyleSheet(
        "background-color: #ECFDF5; border: 1px solid #A7F3D0; border-radius: 10px; font-size: 20px;"
    );
    headerLayout->addWidget(iconLabel);

    QVBoxLayout* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(2);
    QLabel* titleLabel = new QLabel("Accounting Period & Fiscal Year", cardFrame);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A; border: none;");
    QLabel* subtitleLabel = new QLabel("Select active operating fiscal year for transactions & ledger books", cardFrame);
    subtitleLabel->setStyleSheet("font-size: 12px; color: #64748B; border: none;");
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);
    headerLayout->addLayout(titleLayout);

    headerLayout->addStretch(1);

    QPushButton* closeBtn = new QPushButton("✕", cardFrame);
    closeBtn->setFixedSize(30, 30);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: transparent; border: none; color: #64748B; font-weight: bold; font-size: 15px; border-radius: 15px;"
        "}"
        "QPushButton:hover { background-color: #F1F5F9; color: #0F172A; }"
    );
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    headerLayout->addWidget(closeBtn);

    mainLayout->addLayout(headerLayout);

    // ================= 2. FY PRESETS BAR =================
    QLabel* presetsHeader = new QLabel("FINANCIAL YEARS IN DATABASE", cardFrame);
    presetsHeader->setStyleSheet("font-size: 11px; font-weight: 800; color: #64748B; letter-spacing: 0.5px; border: none;");
    mainLayout->addWidget(presetsHeader);

    m_presetsLayout = new QHBoxLayout();
    m_presetsLayout->setSpacing(8);
    mainLayout->addLayout(m_presetsLayout);

    // ================= 3. 2-COLUMN DATE SELECTION (From & To) =================
    QHBoxLayout* columnsLayout = new QHBoxLayout();
    columnsLayout->setSpacing(12);

    // --- Left Column: From Date ---
    QVBoxLayout* fromColLayout = new QVBoxLayout();
    fromColLayout->setSpacing(6);
    QLabel* fromHeader = new QLabel("From Date (01-04-XXXX)", cardFrame);
    fromHeader->setStyleSheet("font-size: 13px; font-weight: 800; color: #334155; border: none;");
    fromColLayout->addWidget(fromHeader);

    m_fromList = new DateListWidget(true, cardFrame);
    fromColLayout->addWidget(m_fromList);
    columnsLayout->addLayout(fromColLayout);

    // --- Right Column: To Date ---
    QVBoxLayout* toColLayout = new QVBoxLayout();
    toColLayout->setSpacing(6);
    QLabel* toHeader = new QLabel("To Date (31-03-XXXX)", cardFrame);
    toHeader->setStyleSheet("font-size: 13px; font-weight: 800; color: #334155; border: none;");
    toColLayout->addWidget(toHeader);

    m_toList = new DateListWidget(false, cardFrame);
    toColLayout->addWidget(m_toList);
    columnsLayout->addLayout(toColLayout);

    mainLayout->addLayout(columnsLayout);

    // Connect seamless Left/Right column switching (retaining each column's independent row position)
    connect(m_fromList, &DateListWidget::switchColumnRequested, this, [this]() {
        if (m_toList->currentRow() < 0 && m_toList->count() > 0) {
            m_toList->setCurrentRow(0);
        }
        m_toList->setFocus();
    });
    connect(m_toList, &DateListWidget::switchColumnRequested, this, [this]() {
        if (m_fromList->currentRow() < 0 && m_fromList->count() > 0) {
            m_fromList->setCurrentRow(0);
        }
        m_fromList->setFocus();
    });

    connect(m_fromList, &QListWidget::currentRowChanged, this, &AccountingPeriodDialog::onFromRowChanged);
    connect(m_toList, &QListWidget::currentRowChanged, this, &AccountingPeriodDialog::onToRowChanged);

    // ================= 4. ACTIVE PERIOD PREVIEW BANNER =================
    QFrame* previewCard = new QFrame(cardFrame);
    previewCard->setFixedHeight(46);
    previewCard->setStyleSheet("background-color: #0F172A; border-radius: 8px; border: none;");
    QHBoxLayout* previewLayout = new QHBoxLayout(previewCard);
    previewLayout->setContentsMargins(14, 0, 14, 0);
    previewLayout->setSpacing(8);

    QLabel* prevTitle = new QLabel("ACTIVE PERIOD:", previewCard);
    prevTitle->setStyleSheet("color: #94A3B8; font-weight: bold; font-size: 12px; border: none; background: transparent;");
    previewLayout->addWidget(prevTitle);

    m_previewDateRangeLabel = new QLabel("—  ->  —", previewCard);
    m_previewDateRangeLabel->setStyleSheet("color: #34D399; font-weight: 800; font-size: 14px; border: none; background: transparent;");
    previewLayout->addWidget(m_previewDateRangeLabel);

    previewLayout->addStretch(1);

    m_previewFyBadge = new QLabel(FiscalYearHelper::getActiveFiscalYear().name, previewCard);
    m_previewFyBadge->setStyleSheet("color: #10B981; background-color: #064E3B; border-radius: 4px; padding: 4px 10px; font-size: 12px; font-weight: 800; border: none;");
    previewLayout->addWidget(m_previewFyBadge);

    mainLayout->addWidget(previewCard);

    // ================= 5. FOOTER & ACTION BUTTONS =================
    QHBoxLayout* footerLayout = new QHBoxLayout();
    footerLayout->setSpacing(8);

    QLabel* kbdHints = new QLabel("[←][→] Switch Column   [↑][↓] Move & Tick   [Enter] Apply", cardFrame);
    kbdHints->setStyleSheet("font-size: 11px; color: #64748B; font-weight: bold; border: none; background: transparent;");
    footerLayout->addWidget(kbdHints);

    footerLayout->addStretch(1);

    m_cancelBtn = new QPushButton("Cancel (Esc)", cardFrame);
    m_cancelBtn->setFixedHeight(38);
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    m_cancelBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #F1F5F9; color: #475569; border: 1px solid #CBD5E1; border-radius: 6px; padding: 4px 16px; font-weight: bold; font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    footerLayout->addWidget(m_cancelBtn);

    m_applyBtn = new QPushButton("Apply Period (Enter)", cardFrame);
    m_applyBtn->setFixedHeight(38);
    m_applyBtn->setCursor(Qt::PointingHandCursor);
    m_applyBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #2563EB; color: #FFFFFF; border: none; border-radius: 6px; padding: 4px 20px; font-weight: bold; font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #1D4ED8; }"
    );
    connect(m_applyBtn, &QPushButton::clicked, this, &AccountingPeriodDialog::applyPeriod);
    footerLayout->addWidget(m_applyBtn);

    mainLayout->addLayout(footerLayout);
    rootLayout->addWidget(cardFrame);
}

DateRowWidgetInfo AccountingPeriodDialog::createDateRowWidget(const DateColumnItem& item) {
    DateRowWidgetInfo info;
    info.container = new QWidget();
    info.container->setStyleSheet("background-color: transparent;");
    QHBoxLayout* l = new QHBoxLayout(info.container);
    l->setContentsMargins(10, 6, 10, 6);
    l->setSpacing(8);

    // Radio indicator circle
    info.radioLabel = new QLabel("○", info.container);
    info.radioLabel->setFixedSize(18, 18);
    info.radioLabel->setAlignment(Qt::AlignCenter);
    info.radioLabel->setStyleSheet("color: #CBD5E1; font-size: 14px; background: transparent; border: none;");
    l->addWidget(info.radioLabel);

    // Date label
    info.dateLabel = new QLabel(item.displayDate, info.container);
    info.dateLabel->setStyleSheet("font-size: 13.5px; font-weight: 600; color: #1E293B; border: none; background: transparent;");
    l->addWidget(info.dateLabel);

    l->addStretch(1);

    // FY Sub-badge
    info.fyBadgeLabel = new QLabel(item.fyName, info.container);
    info.fyBadgeLabel->setStyleSheet("font-size: 11px; color: #64748B; font-weight: 600; border: none; background: transparent;");
    l->addWidget(info.fyBadgeLabel);

    return info;
}

void AccountingPeriodDialog::loadYearsFromDatabase() {
    m_updating = true;

    m_availableFys = FiscalYearHelper::getAllFiscalYears();
    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();

    m_selectedFromIso = activeFy.startDate;
    m_selectedToIso = activeFy.endDate;
    m_selectedFyLabel = activeFy.name;

    // Clear and build preset chips
    qDeleteAll(m_presetButtons);
    m_presetButtons.clear();

    for (const auto& fy : m_availableFys) {
        QPushButton* chip = new QPushButton(fy.name, this);
        chip->setFixedHeight(32);
        chip->setCursor(Qt::PointingHandCursor);

        bool isCurrentSelected = (fy.name == m_selectedFyLabel);
        chip->setStyleSheet(QString(
            "QPushButton {"
            "  background-color: %1; color: %2; border: 1px solid %3; border-radius: 6px; font-size: 11px; font-weight: bold; padding: 4px 10px;"
            "}"
            "QPushButton:hover { background-color: %4; }"
        ).arg(isCurrentSelected ? "#047857" : "#F8FAFC",
              isCurrentSelected ? "#FFFFFF" : "#1E293B",
              isCurrentSelected ? "#047857" : "#E2E8F0",
              isCurrentSelected ? "#065F46" : "#F1F5F9"));

        connect(chip, &QPushButton::clicked, this, [this, fy]() {
            setPreset(fy.name);
        });

        m_presetsLayout->addWidget(chip);
        m_presetButtons.append(chip);
    }

    // Build From Date items (01-04-YYYY) and To Date items (31-03-YYYY)
    m_fromItems.clear();
    m_toItems.clear();
    m_fromRowWidgets.clear();
    m_toRowWidgets.clear();
    m_fromList->clear();
    m_toList->clear();

    int activeFromIndex = 0;
    int activeToIndex = 0;

    for (int i = 0; i < m_availableFys.size(); ++i) {
        const auto& fy = m_availableFys.at(i);

        DateColumnItem fItem;
        fItem.isoDate = fy.startDate;
        fItem.displayDate = FiscalYearHelper::formatDisplayDate(fy.startDate);
        fItem.fyName = fy.name;
        fItem.isStart = true;
        m_fromItems.append(fItem);

        DateColumnItem tItem;
        tItem.isoDate = fy.endDate;
        tItem.displayDate = FiscalYearHelper::formatDisplayDate(fy.endDate);
        tItem.fyName = fy.name;
        tItem.isStart = false;
        m_toItems.append(tItem);

        if (fy.startDate == m_selectedFromIso) activeFromIndex = i;
        if (fy.endDate == m_selectedToIso) activeToIndex = i;

        QListWidgetItem* wFromItem = new QListWidgetItem(m_fromList);
        wFromItem->setSizeHint(QSize(200, 36));
        m_fromList->addItem(wFromItem);
        DateRowWidgetInfo fW = createDateRowWidget(fItem);
        m_fromRowWidgets.append(fW);
        m_fromList->setItemWidget(wFromItem, fW.container);

        QListWidgetItem* wToItem = new QListWidgetItem(m_toList);
        wToItem->setSizeHint(QSize(200, 36));
        m_toList->addItem(wToItem);
        DateRowWidgetInfo tW = createDateRowWidget(tItem);
        m_toRowWidgets.append(tW);
        m_toList->setItemWidget(wToItem, tW.container);
    }

    m_updating = false;

    m_fromList->setCurrentRow(activeFromIndex);
    m_toList->setCurrentRow(activeToIndex);
    updateCheckedStates();
    updatePreview();

    m_fromList->setFocus();
}

void AccountingPeriodDialog::updateCheckedStates() {
    for (int i = 0; i < m_fromRowWidgets.size(); ++i) {
        bool checked = (i < m_fromItems.size() && m_fromItems[i].isoDate == m_selectedFromIso);
        auto& w = m_fromRowWidgets[i];
        if (w.radioLabel && w.dateLabel) {
            w.radioLabel->setText(checked ? "●" : "○");
            w.radioLabel->setStyleSheet(checked
                ? "color: #10B981; font-size: 12px; font-weight: bold; background: transparent; border: none;"
                : "color: #CBD5E1; font-size: 12px; background: transparent; border: none;");
            w.dateLabel->setStyleSheet(QString("font-size: 12px; font-weight: %1; color: %2; border: none; background: transparent;")
                .arg(checked ? "bold" : "normal", checked ? "#065F46" : "#1E293B"));
        }
    }

    for (int j = 0; j < m_toRowWidgets.size(); ++j) {
        bool checked = (j < m_toItems.size() && m_toItems[j].isoDate == m_selectedToIso);
        auto& w = m_toRowWidgets[j];
        if (w.radioLabel && w.dateLabel) {
            w.radioLabel->setText(checked ? "●" : "○");
            w.radioLabel->setStyleSheet(checked
                ? "color: #10B981; font-size: 12px; font-weight: bold; background: transparent; border: none;"
                : "color: #CBD5E1; font-size: 12px; background: transparent; border: none;");
            w.dateLabel->setStyleSheet(QString("font-size: 12px; font-weight: %1; color: %2; border: none; background: transparent;")
                .arg(checked ? "bold" : "normal", checked ? "#065F46" : "#1E293B"));
        }
    }

    // Update preset chip styling
    for (int k = 0; k < m_presetButtons.size(); ++k) {
        if (k < m_availableFys.size()) {
            bool isPresetActive = (m_availableFys[k].name == m_selectedFyLabel);
            m_presetButtons[k]->setStyleSheet(QString(
                "QPushButton {"
                "  background-color: %1; color: %2; border: 1px solid %3; border-radius: 6px; font-size: 11px; font-weight: bold; padding: 4px 10px;"
                "}"
                "QPushButton:hover { background-color: %4; }"
            ).arg(isPresetActive ? "#047857" : "#F8FAFC",
                  isPresetActive ? "#FFFFFF" : "#1E293B",
                  isPresetActive ? "#047857" : "#E2E8F0",
                  isPresetActive ? "#065F46" : "#F1F5F9"));
        }
    }
}

void AccountingPeriodDialog::onFromRowChanged(int row) {
    if (m_updating || row < 0 || row >= m_fromItems.size()) return;

    m_selectedFromIso = m_fromItems[row].isoDate;

    // Range guard: if fromDate > toDate, adjust toDate to match
    if (m_selectedFromIso > m_selectedToIso && row < m_toItems.size()) {
        m_selectedToIso = m_toItems[row].isoDate;
        m_updating = true;
        m_toList->setCurrentRow(row);
        m_updating = false;
    }

    updatePreview();
    updateCheckedStates();
}

void AccountingPeriodDialog::onToRowChanged(int row) {
    if (m_updating || row < 0 || row >= m_toItems.size()) return;

    m_selectedToIso = m_toItems[row].isoDate;

    // Range guard: if toDate < fromDate, adjust fromDate to match
    if (m_selectedToIso < m_selectedFromIso && row < m_fromItems.size()) {
        m_selectedFromIso = m_fromItems[row].isoDate;
        m_updating = true;
        m_fromList->setCurrentRow(row);
        m_updating = false;
    }

    updatePreview();
    updateCheckedStates();
}

void AccountingPeriodDialog::setPreset(const QString& fyName) {
    for (int i = 0; i < m_availableFys.size(); ++i) {
        if (m_availableFys[i].name == fyName) {
            m_updating = true;
            m_selectedFromIso = m_availableFys[i].startDate;
            m_selectedToIso = m_availableFys[i].endDate;
            m_selectedFyLabel = m_availableFys[i].name;

            m_fromList->setCurrentRow(i);
            m_toList->setCurrentRow(i);
            m_updating = false;

            updatePreview();
            updateCheckedStates();
            break;
        }
    }
}

void AccountingPeriodDialog::updatePreview() {
    bool matched = false;
    for (const auto& fy : m_availableFys) {
        if (fy.startDate == m_selectedFromIso && fy.endDate == m_selectedToIso) {
            m_selectedFyLabel = fy.name;
            matched = true;
            break;
        }
    }
    if (!matched) {
        m_selectedFyLabel = "Custom Period";
    }

    QString fromDisp = FiscalYearHelper::formatDisplayDate(m_selectedFromIso);
    QString toDisp = FiscalYearHelper::formatDisplayDate(m_selectedToIso);
    m_previewDateRangeLabel->setText(QString("%1  ->  %2").arg(fromDisp, toDisp));
    m_previewFyBadge->setText(m_selectedFyLabel);
}

void AccountingPeriodDialog::applyPeriod() {
    accept();
}

void AccountingPeriodDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        reject();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_F2) {
        applyPeriod();
        event->accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

bool AccountingPeriodDialog::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* kEvent = static_cast<QKeyEvent*>(event);
        if (kEvent->key() == Qt::Key_Escape) {
            reject();
            return true;
        }
        if (kEvent->key() == Qt::Key_F2) {
            applyPeriod();
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

bool AccountingPeriodDialog::selectAndApplyGlobalPeriod(QWidget* parent,
                                                       QString* outFromIso,
                                                       QString* outToIso,
                                                       QString* outFyLabel) {
    AccountingPeriodDialog dlg(parent);
    if (dlg.exec() == QDialog::Accepted) {
        QString fIso = dlg.selectedFromIso();
        QString tIso = dlg.selectedToIso();
        QString fy = dlg.selectedFyLabel();

        if (outFromIso) *outFromIso = fIso;
        if (outToIso) *outToIso = tIso;
        if (outFyLabel) *outFyLabel = fy;

        // 1. Update SQLite financial_years table
        DatabaseManager::instance().executeNonQuery("UPDATE financial_years SET is_active = 0;");
        if (fy != "Custom Period" && !fy.isEmpty()) {
            DatabaseManager::instance().executeNonQuery(
                "UPDATE financial_years SET is_active = 1 WHERE year_name = ?;",
                {fy}
            );
        } else {
            DatabaseManager::instance().executeNonQuery(
                "UPDATE financial_years SET is_active = 1 WHERE id IN ("
                "  SELECT id FROM financial_years WHERE (start_date <= ? AND end_date >= ?) OR (start_date <= ? AND end_date >= ?) LIMIT 1"
                ");",
                {fIso, fIso, tIso, tIso}
            );
        }

        // 2. Set active period in AccountingEngine
        AccountingEngine::setActivePeriod(fIso, tIso, fy);

        return true;
    }
    return false;
}
