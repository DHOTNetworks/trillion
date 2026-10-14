#include "custom_dialogs.h"
#include "accounting_engine.h"
#include <QApplication>
#include <QComboBox>
#include <QGraphicsDropShadowEffect>
#include <QShortcut>

// ============================================================================
// CustomInputDialog Implementation
// ============================================================================

CustomInputDialog::CustomInputDialog(const QString& title, const QString& prompt,
                                     const QString& defaultText, QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground, true);
    setupUi(title, prompt, defaultText);
}

void CustomInputDialog::setupUi(const QString& title, const QString& prompt, const QString& defaultText) {
    setMinimumWidth(420);

    setStyleSheet(
        "QDialog {"
        "  background-color: #FFFFFF;"
        "  border: 2px solid #0284C7;"
        "  border-radius: 4px;"
        "}"
        "QLabel {"
        "  background: transparent;"
        "  color: #0F172A;"
        "  font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, 'Helvetica Neue', 'Noto Sans', 'Liberation Sans', Arial, sans-serif;"
        "}"
        "QLineEdit {"
        "  background-color: #FFFFFF;"
        "  color: #000000;"
        "  border: 1.5px solid #7F9DB9;"
        "  border-radius: 2px;"
        "  padding: 6px 8px;"
        "  font-size: 13px;"
        "  font-weight: 700;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #0284C7;"
        "  background-color: #FFFFDD;"
        "}"
        "QPushButton {"
        "  font-size: 12px;"
        "  font-weight: 700;"
        "  padding: 5px 16px;"
        "  border-radius: 2px;"
        "  min-width: 80px;"
        "}"
    );

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // Header Bar
    QFrame* headerBar = new QFrame(this);
    headerBar->setMinimumHeight(36);
    headerBar->setStyleSheet("background-color: #0284C7; border-top-left-radius: 2px; border-top-right-radius: 2px; border: none;");
    QHBoxLayout* headerLayout = new QHBoxLayout(headerBar);
    headerLayout->setContentsMargins(12, 0, 12, 0);

    QLabel* titleLabel = new QLabel(title, headerBar);
    titleLabel->setStyleSheet("color: #FFFFFF; font-size: 13px; font-weight: 800; border: none;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch(1);

    rootLayout->addWidget(headerBar);

    // Body Container
    QWidget* bodyWidget = new QWidget(this);
    QVBoxLayout* bodyLayout = new QVBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(16, 16, 16, 16);
    bodyLayout->setSpacing(12);

    QLabel* promptLabel = new QLabel(prompt, bodyWidget);
    promptLabel->setStyleSheet("font-size: 12.5px; font-weight: 700; color: #1E293B;");
    promptLabel->setWordWrap(true);
    bodyLayout->addWidget(promptLabel);

    m_textInput = new QLineEdit(defaultText, bodyWidget);
    m_textInput->setMinimumHeight(32);
    bodyLayout->addWidget(m_textInput);

    // Action Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    btnLayout->addStretch(1);

    m_cancelBtn = new QPushButton("Cancel (Esc)", bodyWidget);
    m_cancelBtn->setStyleSheet("background-color: #F1F5F9; color: #334155; border: 1px solid #CBD5E1;");
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_cancelBtn);

    m_okBtn = new QPushButton("OK (Enter)", bodyWidget);
    m_okBtn->setStyleSheet("background-color: #0284C7; color: #FFFFFF; border: 1px solid #0369A1;");
    m_okBtn->setDefault(true);
    connect(m_okBtn, &QPushButton::clicked, this, &QDialog::accept);
    bodyLayout->addWidget(m_okBtn);

    bodyLayout->addLayout(btnLayout);
    rootLayout->addWidget(bodyWidget);

    // QLineEdit consumes Return internally, so the dialog-level Enter handler
    // never fires while typing — route it explicitly to OK.
    connect(m_textInput, &QLineEdit::returnPressed, this, &QDialog::accept);
}

QString CustomInputDialog::value() const {
    return m_textInput ? m_textInput->text().trimmed() : "";
}

void CustomInputDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        accept();
        event->accept();
    } else if (event->key() == Qt::Key_Escape) {
        reject();
        event->accept();
    } else {
        QDialog::keyPressEvent(event);
    }
}

void CustomInputDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    if (m_textInput) {
        m_textInput->setFocus();
        m_textInput->selectAll();
    }
}

QString CustomInputDialog::getText(QWidget* parent, const QString& title, const QString& prompt,
                                  const QString& defaultText, bool* ok) {
    CustomInputDialog dlg(title, prompt, defaultText, parent);
    int res = dlg.exec();
    if (ok) *ok = (res == QDialog::Accepted);
    return (res == QDialog::Accepted) ? dlg.value() : defaultText;
}

// ============================================================================
// CustomMessageBox Implementation
// ============================================================================

CustomMessageBox::CustomMessageBox(const QString& title, const QString& message,
                                   IconType iconType, const QString& primaryBtnText,
                                   const QString& secondaryBtnText, QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground, true);
    setupUi(title, message, iconType, primaryBtnText, secondaryBtnText);
}

void CustomMessageBox::setupUi(const QString& title, const QString& message, IconType iconType,
                               const QString& primaryBtnText, const QString& secondaryBtnText) {
    setMinimumWidth(400);

    QString headerBg = "#0284C7"; // Info / default blue
    if (iconType == Warning) headerBg = "#D97706"; // Amber
    else if (iconType == Error) headerBg = "#DC2626"; // Red
    else if (iconType == Question) headerBg = "#2563EB"; // Blue

    setStyleSheet(
        QString(
            "QDialog {"
            "  background-color: #FFFFFF;"
            "  border: 2px solid %1;"
            "  border-radius: 4px;"
            "}"
            "QLabel {"
            "  background: transparent;"
            "  color: #0F172A;"
            "  font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, 'Helvetica Neue', 'Noto Sans', 'Liberation Sans', Arial, sans-serif;"
            "}"
            "QPushButton {"
            "  font-size: 12px;"
            "  font-weight: 700;"
            "  padding: 5px 16px;"
            "  border-radius: 2px;"
            "  min-width: 80px;"
            "}"
        ).arg(headerBg)
    );

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // Header Bar
    QFrame* headerBar = new QFrame(this);
    headerBar->setMinimumHeight(36);
    headerBar->setStyleSheet(QString("background-color: %1; border-top-left-radius: 2px; border-top-right-radius: 2px; border: none;").arg(headerBg));
    QHBoxLayout* headerLayout = new QHBoxLayout(headerBar);
    headerLayout->setContentsMargins(12, 0, 12, 0);

    QLabel* titleLabel = new QLabel(title, headerBar);
    titleLabel->setStyleSheet("color: #FFFFFF; font-size: 13px; font-weight: 800; border: none;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch(1);

    rootLayout->addWidget(headerBar);

    // Body Container
    QWidget* bodyWidget = new QWidget(this);
    QVBoxLayout* bodyLayout = new QVBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(18, 18, 18, 16);
    bodyLayout->setSpacing(14);

    QLabel* msgLabel = new QLabel(message, bodyWidget);
    msgLabel->setStyleSheet("font-size: 12.5px; font-weight: 600; color: #1E293B; line-height: 1.4;");
    msgLabel->setWordWrap(true);
    bodyLayout->addWidget(msgLabel);

    // Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    btnLayout->addStretch(1);

    if (!secondaryBtnText.isEmpty()) {
        m_secondaryBtn = new QPushButton(secondaryBtnText, bodyWidget);
        m_secondaryBtn->setStyleSheet("QPushButton { background-color: #F1F5F9; color: #334155; border: 1px solid #CBD5E1; }"
                                      " QPushButton:focus { border: 2px solid #2563EB; background-color: #DBEAFE; color: #1E40AF; }");
        connect(m_secondaryBtn, &QPushButton::clicked, this, &QDialog::reject);
        btnLayout->addWidget(m_secondaryBtn);
    }

    m_primaryBtn = new QPushButton(primaryBtnText, bodyWidget);
    m_primaryBtn->setStyleSheet(QString("QPushButton { background-color: %1; color: #FFFFFF; border: 1px solid %1; }"
                                        " QPushButton:focus { border: 2px solid #93C5FD; background-color: #1E40AF; }").arg(headerBg));
    m_primaryBtn->setDefault(true);
    connect(m_primaryBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(m_primaryBtn);

    bodyLayout->addLayout(btnLayout);
    rootLayout->addWidget(bodyWidget);

    // Full keyboard control: primary focused + visibly marked, Tab cycles
    // both buttons, arrows move between them.
    m_primaryBtn->setFocus();
    setTabOrder(m_secondaryBtn ? m_secondaryBtn : m_primaryBtn, m_primaryBtn);
    setupKeyboardShortcuts();
}

void CustomMessageBox::setupKeyboardShortcuts() {
    // QShortcuts fire at window level regardless of which child consumes the
    // key, so navigation can never get stuck behind a focused widget. This is
    // the ONLY key mechanism here (no keyPressEvent override) to guarantee a
    // single action per press — never a toggle-then-toggle-back.
    auto addKey = [this](int key, auto&& fn) {
        auto* sc = new QShortcut(QKeySequence(key), this);
        sc->setContext(Qt::WindowShortcut);
        connect(sc, &QShortcut::activated, this, fn);
    };
    addKey(Qt::Key_Return, [this]() { onConfirmKey(); });
    addKey(Qt::Key_Enter, [this]() { onConfirmKey(); });
    addKey(Qt::Key_Escape, [this]() { reject(); });
    if (m_secondaryBtn) {
        // Y = Yes/primary, N = No/secondary (Bahi-Khata style confirm keys).
        addKey(Qt::Key_Y, [this]() { accept(); });
        addKey(Qt::Key_N, [this]() { reject(); });
        addKey(Qt::Key_Left, [this]() { onMoveFocus(-1); });
        addKey(Qt::Key_Right, [this]() { onMoveFocus(1); });
    }
}

void CustomMessageBox::onConfirmKey() {
    // Enter confirms the focused button when one has focus, else primary.
    if (m_secondaryBtn && m_secondaryBtn->hasFocus()) {
        reject();
    } else {
        accept();
    }
}

void CustomMessageBox::onMoveFocus(int direction) {
    if (!m_secondaryBtn || !m_primaryBtn) return;
    // direction -1 = Left (toward secondary), +1 = Right (toward primary).
    bool primaryFocused = m_primaryBtn->hasFocus();
    if (direction < 0) {
        (primaryFocused ? m_secondaryBtn : m_primaryBtn)->setFocus();
    } else {
        ((m_secondaryBtn->hasFocus()) ? m_primaryBtn : m_secondaryBtn)->setFocus();
    }
}

void CustomMessageBox::information(QWidget* parent, const QString& title, const QString& message) {
    CustomMessageBox dlg(title, message, Info, "OK", "", parent);
    dlg.exec();
}

void CustomMessageBox::warning(QWidget* parent, const QString& title, const QString& message) {
    CustomMessageBox dlg(title, message, Warning, "OK", "", parent);
    dlg.exec();
}

void CustomMessageBox::critical(QWidget* parent, const QString& title, const QString& message) {
    CustomMessageBox dlg(title, message, Error, "OK", "", parent);
    dlg.exec();
}

bool CustomMessageBox::question(QWidget* parent, const QString& title, const QString& message,
                               const QString& yesText, const QString& noText) {
    CustomMessageBox dlg(title, message, Question, yesText, noText, parent);
    return (dlg.exec() == QDialog::Accepted);
}

// ============================================================================
// FreightCalculationDialog Implementation
// ============================================================================

FreightCalculationDialog::FreightCalculationDialog(double kandaWeight, int totalBags, QWidget* parent)
    : QDialog(parent)
    , m_kandaWeight(kandaWeight)
    , m_totalBags(totalBags)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground, true);
    setupUi();
    updateCalculations();
}

void FreightCalculationDialog::setupUi() {
    setMinimumWidth(440);

    setStyleSheet(
        "QDialog {"
        "  background-color: #FFFFFF;"
        "  border: 2px solid #0284C7;"
        "  border-radius: 6px;"
        "}"
        "QLabel {"
        "  background: transparent;"
        "  color: #0F172A;"
        "  font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, 'Helvetica Neue', 'Noto Sans', 'Liberation Sans', Arial, sans-serif;"
        "}"
        "QLineEdit {"
        "  background-color: #FFFFFF;"
        "  color: #000000;"
        "  border: 1.5px solid #7F9DB9;"
        "  border-radius: 3px;"
        "  padding: 6px 8px;"
        "  font-size: 13px;"
        "  font-weight: 700;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #0284C7;"
        "  background-color: #FFFFDD;"
        "}"
        "QComboBox {"
        "  background-color: #FFFFFF;"
        "  color: #0F172A;"
        "  border: 1.5px solid #7F9DB9;"
        "  border-radius: 3px;"
        "  padding: 4px 8px;"
        "  font-size: 12px;"
        "  font-weight: 700;"
        "}"
        "QPushButton {"
        "  font-size: 12px;"
        "  font-weight: 700;"
        "  padding: 6px 18px;"
        "  border-radius: 3px;"
        "  min-width: 80px;"
        "}"
    );

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // Header Bar
    QFrame* headerBar = new QFrame(this);
    headerBar->setMinimumHeight(38);
    headerBar->setStyleSheet("background-color: #0284C7; border-top-left-radius: 4px; border-top-right-radius: 4px; border-bottom: none;");
    QHBoxLayout* headerLayout = new QHBoxLayout(headerBar);
    headerLayout->setContentsMargins(14, 0, 14, 0);

    QLabel* titleLabel = new QLabel("Freight Calculation", headerBar);
    titleLabel->setStyleSheet("color: #FFFFFF; font-size: 13px; font-weight: 800;");
    headerLayout->addWidget(titleLabel);
    rootLayout->addWidget(headerBar);

    // Body
    QWidget* bodyWidget = new QWidget(this);
    QVBoxLayout* bodyLayout = new QVBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(18, 16, 18, 16);
    bodyLayout->setSpacing(12);

    // Info Summary Row (Kanda Weight & Total Bags)
    QHBoxLayout* infoRow = new QHBoxLayout();
    infoRow->setSpacing(16);

    QLabel* kwLabel = new QLabel(QString("<b>Kanda Wt:</b> <font color='#0284C7'>%1 Qtl</font>").arg(QString::number(m_kandaWeight, 'f', 2)), bodyWidget);
    kwLabel->setStyleSheet("font-size: 12.5px;");
    infoRow->addWidget(kwLabel);

    QLabel* bagLabel = new QLabel(QString("<b>Total Bags:</b> <font color='#0284C7'>%1</font>").arg(m_totalBags), bodyWidget);
    bagLabel->setStyleSheet("font-size: 12.5px;");
    infoRow->addWidget(bagLabel);
    infoRow->addStretch(1);
    bodyLayout->addLayout(infoRow);

    // Input Grid: Rate & Rate Unit
    QGridLayout* inputGrid = new QGridLayout();
    inputGrid->setHorizontalSpacing(12);
    inputGrid->setVerticalSpacing(8);

    QLabel* rateLbl = new QLabel("Freight Rate:", bodyWidget);
    rateLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155;");
    inputGrid->addWidget(rateLbl, 0, 0);

    m_rateInput = new QLineEdit(bodyWidget);
    m_rateInput->setPlaceholderText("Enter freight rate (e.g. 100)");
    m_rateInput->setText("");
    connect(m_rateInput, &QLineEdit::textChanged, this, &FreightCalculationDialog::updateCalculations);
    // QLineEdit consumes Return internally: route it explicitly to Apply.
    connect(m_rateInput, &QLineEdit::returnPressed, this, [this]() {
        updateCalculations();
        accept();
    });
    inputGrid->addWidget(m_rateInput, 0, 1);

    QLabel* unitLbl = new QLabel("Rate Unit:", bodyWidget);
    unitLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155;");
    inputGrid->addWidget(unitLbl, 1, 0);

    m_unitCombo = new QComboBox(bodyWidget);
    m_unitCombo->addItems({"Per Qtl (Rate x Kanda Wt)", "Per Bag / Qty (Rate x Bags)"});
    connect(m_unitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FreightCalculationDialog::updateCalculations);
    inputGrid->addWidget(m_unitCombo, 1, 1);

    bodyLayout->addLayout(inputGrid);

    // Result Highlight Box
    QFrame* resBox = new QFrame(bodyWidget);
    resBox->setStyleSheet("background-color: #F0FDF4; border: 1.5px solid #86EFAC; border-radius: 4px; padding: 6px;");
    QHBoxLayout* resLayout = new QHBoxLayout(resBox);
    resLayout->setContentsMargins(10, 4, 10, 4);

    QLabel* resTitle = new QLabel("Total Freight Amount:", resBox);
    resTitle->setStyleSheet("font-size: 12px; font-weight: 700; color: #166534;");
    resLayout->addWidget(resTitle);

    m_resultLabel = new QLabel("₹ 0.00", resBox);
    m_resultLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_resultLabel->setStyleSheet("font-size: 15px; font-weight: 900; color: #15803D;");
    resLayout->addWidget(m_resultLabel, 1);

    bodyLayout->addWidget(resBox);

    // Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    btnLayout->addStretch(1);

    m_skipBtn = new QPushButton("Skip (Esc)", bodyWidget);
    m_skipBtn->setStyleSheet("background-color: #F1F5F9; color: #475569; border: 1px solid #CBD5E1;");
    connect(m_skipBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_skipBtn);

    m_okBtn = new QPushButton("Apply Freight (Enter)", bodyWidget);
    m_okBtn->setStyleSheet("background-color: #0284C7; color: #FFFFFF; border: 1px solid #0369A1;");
    m_okBtn->setDefault(true);
    connect(m_okBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(m_okBtn);

    bodyLayout->addLayout(btnLayout);
    rootLayout->addWidget(bodyWidget);
}

void FreightCalculationDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    if (m_rateInput) {
        m_rateInput->setFocus();
        m_rateInput->selectAll();
    }
}

void FreightCalculationDialog::updateCalculations() {
    if (!m_rateInput) return;
    bool ok = false;
    m_freightRate = m_rateInput->text().trimmed().toDouble(&ok);
    if (!ok || m_freightRate < 0.0) m_freightRate = 0.0;

    int unitIdx = m_unitCombo ? m_unitCombo->currentIndex() : 0;
    if (unitIdx == 0) {
        m_rateUnit = "Per Qtl";
        m_calculatedFreight = std::round(m_freightRate * m_kandaWeight * 100.0) / 100.0;
    } else {
        m_rateUnit = "Per Bag";
        m_calculatedFreight = std::round(m_freightRate * (double)m_totalBags * 100.0) / 100.0;
    }

    if (m_resultLabel) {
        m_resultLabel->setText(QString("₹ %1").arg(QString::number(m_calculatedFreight, 'f', 2)));
    }
}

void FreightCalculationDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        updateCalculations();
        accept();
        event->accept();
    } else if (event->key() == Qt::Key_Escape) {
        reject();
        event->accept();
    } else {
        QDialog::keyPressEvent(event);
    }
}

bool FreightCalculationDialog::promptFreight(QWidget* parent, double kandaWeight, int totalBags,
                                            double* outFreight, double* outRate, QString* outUnit) {
    FreightCalculationDialog dlg(kandaWeight, totalBags, parent);
    if (dlg.exec() == QDialog::Accepted) {
        if (outFreight) *outFreight = dlg.calculatedFreight();
        if (outRate) *outRate = dlg.freightRate();
        if (outUnit) *outUnit = dlg.rateUnit();
        return true;
    }
    return false;
}

// ============================================================================
// Tds194QCalculationDialog Implementation
// ============================================================================

Tds194QCalculationDialog::Tds194QCalculationDialog(
    const QString& supplierName,
    double priorTurnover,
    double currentBillAmount,
    double defaultThreshold,
    double defaultRate,
    QWidget* parent
)
    : QDialog(parent)
    , m_supplierName(supplierName)
    , m_priorTurnover(priorTurnover)
    , m_currentBillAmount(currentBillAmount)
    , m_thresholdLimit(defaultThreshold)
    , m_tdsRate(defaultRate)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground, true);
    setupUi();
    updateCalculations();
}

void Tds194QCalculationDialog::setupUi() {
    setMinimumWidth(480);

    setStyleSheet(
        "QDialog {"
        "  background-color: #FFFFFF;"
        "  border: 2px solid #0284C7;"
        "  border-radius: 6px;"
        "}"
        "QLabel {"
        "  background: transparent;"
        "  color: #0F172A;"
        "  font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, 'Helvetica Neue', 'Noto Sans', 'Liberation Sans', Arial, sans-serif;"
        "}"
        "QLineEdit {"
        "  background-color: #FFFFFF;"
        "  color: #000000;"
        "  border: 1.5px solid #7F9DB9;"
        "  border-radius: 3px;"
        "  padding: 5px 8px;"
        "  font-size: 13px;"
        "  font-weight: 700;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #0284C7;"
        "  background-color: #FFFFDD;"
        "}"
        "QPushButton {"
        "  font-size: 12px;"
        "  font-weight: 700;"
        "  padding: 6px 18px;"
        "  border-radius: 3px;"
        "  min-width: 80px;"
        "}"
    );

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // Header Bar
    QFrame* headerBar = new QFrame(this);
    headerBar->setMinimumHeight(38);
    headerBar->setStyleSheet("background-color: #0284C7; border-top-left-radius: 4px; border-top-right-radius: 4px; border-bottom: none;");
    QHBoxLayout* headerLayout = new QHBoxLayout(headerBar);
    headerLayout->setContentsMargins(14, 0, 14, 0);

    QLabel* titleLabel = new QLabel("TDS u/s 194Q Calculation (Purchase > ₹50 Lakh)", headerBar);
    titleLabel->setStyleSheet("color: #FFFFFF; font-size: 13px; font-weight: 800;");
    headerLayout->addWidget(titleLabel);
    rootLayout->addWidget(headerBar);

    // Body
    QWidget* bodyWidget = new QWidget(this);
    QVBoxLayout* bodyLayout = new QVBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(18, 14, 18, 14);
    bodyLayout->setSpacing(10);

    // Supplier Name banner
    QLabel* suppLbl = new QLabel(QString("<b>Party:</b> <font color='#0284C7'>%1</font>").arg(m_supplierName.isEmpty() ? "Selected Supplier" : m_supplierName), bodyWidget);
    suppLbl->setStyleSheet("font-size: 13px;");
    suppLbl->setWordWrap(true);
    bodyLayout->addWidget(suppLbl);

    // Turnover Stats Grid
    QFrame* statsBox = new QFrame(bodyWidget);
    statsBox->setStyleSheet("background-color: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 4px; padding: 6px;");
    QGridLayout* statsGrid = new QGridLayout(statsBox);
    statsGrid->setContentsMargins(8, 6, 8, 6);
    statsGrid->setHorizontalSpacing(14);
    statsGrid->setVerticalSpacing(4);

    statsGrid->addWidget(new QLabel("Prior FY Purchase Turnover:", statsBox), 0, 0);
    QLabel* priorValLbl = new QLabel(QString("₹ %1").arg(AccountingEngine::formatIndianCurrency(m_priorTurnover)), statsBox);
    priorValLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    priorValLbl->setStyleSheet("font-weight: 700; color: #334155;");
    statsGrid->addWidget(priorValLbl, 0, 1);

    statsGrid->addWidget(new QLabel("Current Bill Gross Amount:", statsBox), 1, 0);
    QLabel* currValLbl = new QLabel(QString("₹ %1").arg(AccountingEngine::formatIndianCurrency(m_currentBillAmount)), statsBox);
    currValLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    currValLbl->setStyleSheet("font-weight: 700; color: #334155;");
    statsGrid->addWidget(currValLbl, 1, 1);

    double cumTurnover = m_priorTurnover + m_currentBillAmount;
    statsGrid->addWidget(new QLabel("Cumulative FY Purchase:", statsBox), 2, 0);
    QLabel* cumValLbl = new QLabel(QString("₹ %1").arg(AccountingEngine::formatIndianCurrency(cumTurnover)), statsBox);
    cumValLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    cumValLbl->setStyleSheet("font-weight: 800; color: #0369A1;");
    statsGrid->addWidget(cumValLbl, 2, 1);

    bodyLayout->addWidget(statsBox);

    // Editable Inputs Grid: Threshold, Rate, Taxable Amount
    QGridLayout* inputGrid = new QGridLayout();
    inputGrid->setHorizontalSpacing(10);
    inputGrid->setVerticalSpacing(8);

    QLabel* threshLbl = new QLabel("Threshold Limit (₹):", bodyWidget);
    threshLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155;");
    inputGrid->addWidget(threshLbl, 0, 0);

    m_thresholdInput = new QLineEdit(QString::number(m_thresholdLimit, 'f', 2), bodyWidget);
    m_thresholdInput->setAlignment(Qt::AlignRight);
    connect(m_thresholdInput, &QLineEdit::textChanged, this, &Tds194QCalculationDialog::updateCalculations);
    connect(m_thresholdInput, &QLineEdit::returnPressed, this, [this]() {
        updateCalculations();
        if (m_rateInput) {
            m_rateInput->setFocus();
            m_rateInput->selectAll();
        }
    });
    inputGrid->addWidget(m_thresholdInput, 0, 1);

    QLabel* rateLbl = new QLabel("TDS Rate (%):", bodyWidget);
    rateLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155;");
    inputGrid->addWidget(rateLbl, 1, 0);

    m_rateInput = new QLineEdit(QString::number(m_tdsRate, 'f', 3), bodyWidget);
    m_rateInput->setAlignment(Qt::AlignRight);
    connect(m_rateInput, &QLineEdit::textChanged, this, &Tds194QCalculationDialog::updateCalculations);
    connect(m_rateInput, &QLineEdit::returnPressed, this, [this]() {
        updateCalculations();
        if (m_customTdsInput) {
            m_customTdsInput->setFocus();
            m_customTdsInput->selectAll();
        }
    });
    inputGrid->addWidget(m_rateInput, 1, 1);

    QLabel* taxAmtLbl = new QLabel("Subject to TDS u/s 194Q:", bodyWidget);
    taxAmtLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155;");
    inputGrid->addWidget(taxAmtLbl, 2, 0);

    m_taxableLabel = new QLabel("₹ 0.00", bodyWidget);
    m_taxableLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_taxableLabel->setStyleSheet("font-size: 12.5px; font-weight: 800; color: #0284C7;");
    inputGrid->addWidget(m_taxableLabel, 2, 1);

    QLabel* customTdsLbl = new QLabel("TDS Amount to Deduct:", bodyWidget);
    customTdsLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #DC2626;");
    inputGrid->addWidget(customTdsLbl, 3, 0);

    m_customTdsInput = new QLineEdit("0.00", bodyWidget);
    m_customTdsInput->setAlignment(Qt::AlignRight);
    connect(m_customTdsInput, &QLineEdit::textEdited, this, [this]() {
        bool ok = false;
        double val = m_customTdsInput->text().trimmed().toDouble(&ok);
        if (ok && val >= 0.0) {
            m_calculatedTds = val;
            if (m_resultLabel) {
                m_resultLabel->setText(QString("₹ %1").arg(AccountingEngine::formatIndianCurrency(m_calculatedTds)));
            }
        }
    });
    connect(m_customTdsInput, &QLineEdit::returnPressed, this, [this]() {
        bool ok = false;
        double val = m_customTdsInput->text().trimmed().toDouble(&ok);
        if (ok && val >= 0.0) {
            m_calculatedTds = val;
        }
        accept();
    });
    inputGrid->addWidget(m_customTdsInput, 3, 1);

    bodyLayout->addLayout(inputGrid);

    // Result Highlight Box
    QFrame* resBox = new QFrame(bodyWidget);
    resBox->setStyleSheet("background-color: #FEF2F2; border: 1.5px solid #FCA5A5; border-radius: 4px; padding: 6px;");
    QHBoxLayout* resLayout = new QHBoxLayout(resBox);
    resLayout->setContentsMargins(10, 4, 10, 4);

    QLabel* resTitle = new QLabel("Total TDS u/s 194Q to Deduct:", resBox);
    resTitle->setStyleSheet("font-size: 12px; font-weight: 700; color: #991B1B;");
    resLayout->addWidget(resTitle);

    m_resultLabel = new QLabel("₹ 0.00", resBox);
    m_resultLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_resultLabel->setStyleSheet("font-size: 16px; font-weight: 900; color: #DC2626;");
    resLayout->addWidget(m_resultLabel, 1);

    bodyLayout->addWidget(resBox);

    // Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    btnLayout->addStretch(1);

    m_skipBtn = new QPushButton("Skip / Cancel (Esc)", bodyWidget);
    m_skipBtn->setStyleSheet("background-color: #F1F5F9; color: #475569; border: 1px solid #CBD5E1;");
    connect(m_skipBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_skipBtn);

    m_okBtn = new QPushButton("Apply TDS (Enter)", bodyWidget);
    m_okBtn->setStyleSheet("background-color: #0284C7; color: #FFFFFF; border: 1px solid #0369A1;");
    m_okBtn->setDefault(true);
    connect(m_okBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(m_okBtn);

    bodyLayout->addLayout(btnLayout);
    rootLayout->addWidget(bodyWidget);
}

void Tds194QCalculationDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    if (m_customTdsInput) {
        m_customTdsInput->setFocus();
        m_customTdsInput->selectAll();
    }
}

void Tds194QCalculationDialog::updateCalculations() {
    bool okThresh = false, okRate = false;
    if (m_thresholdInput) m_thresholdLimit = m_thresholdInput->text().trimmed().toDouble(&okThresh);
    if (!okThresh || m_thresholdLimit < 0.0) m_thresholdLimit = 5000000.0;

    if (m_rateInput) m_tdsRate = m_rateInput->text().trimmed().toDouble(&okRate);
    if (!okRate || m_tdsRate < 0.0) m_tdsRate = 0.100;

    // Calculation logic u/s 194Q:
    // Only aggregate purchase above threshold in current FY is subject to TDS
    if (m_priorTurnover >= m_thresholdLimit) {
        m_taxableForTds = m_currentBillAmount;
    } else if (m_priorTurnover + m_currentBillAmount > m_thresholdLimit) {
        m_taxableForTds = (m_priorTurnover + m_currentBillAmount) - m_thresholdLimit;
    } else {
        m_taxableForTds = 0.0;
    }

    m_calculatedTds = std::round(m_taxableForTds * (m_tdsRate / 100.0) * 100.0) / 100.0;

    if (m_taxableLabel) {
        m_taxableLabel->setText(QString("₹ %1").arg(AccountingEngine::formatIndianCurrency(m_taxableForTds)));
    }
    if (m_customTdsInput && !m_customTdsInput->hasFocus()) {
        m_customTdsInput->setText(QString::number(m_calculatedTds, 'f', 2));
    }
    if (m_resultLabel) {
        m_resultLabel->setText(QString("₹ %1").arg(AccountingEngine::formatIndianCurrency(m_calculatedTds)));
    }
}

void Tds194QCalculationDialog::accept() {
    if (m_customTdsInput) {
        bool ok = false;
        double val = m_customTdsInput->text().trimmed().toDouble(&ok);
        if (ok && val >= 0.0) {
            m_calculatedTds = val;
        }
    }
    QDialog::accept();
}

void Tds194QCalculationDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        accept();
        event->accept();
    } else if (event->key() == Qt::Key_Escape) {
        reject();
        event->accept();
    } else {
        QDialog::keyPressEvent(event);
    }
}

bool Tds194QCalculationDialog::promptTds194Q(
    QWidget* parent,
    const QString& supplierName,
    double priorTurnover,
    double currentBillAmount,
    double* outTdsAmount,
    double* outTdsRate,
    double* outTaxableForTds,
    double defaultThreshold,
    double defaultRate
) {
    Tds194QCalculationDialog dlg(supplierName, priorTurnover, currentBillAmount, defaultThreshold, defaultRate, parent);
    if (dlg.exec() == QDialog::Accepted) {
        if (outTdsAmount) *outTdsAmount = dlg.calculatedTds();
        if (outTdsRate) *outTdsRate = dlg.tdsRate();
        if (outTaxableForTds) *outTaxableForTds = dlg.taxableForTds();
        return true;
    }
    return false;
}

// Tcs206CCalculationDialog Implementation
// ============================================================================

Tcs206CCalculationDialog::Tcs206CCalculationDialog(
    const QString& customerName,
    double currentBillAmount,
    double defaultRate,
    QWidget* parent
)
    : QDialog(parent)
    , m_customerName(customerName)
    , m_currentBillAmount(currentBillAmount)
    , m_tcsRate(defaultRate)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground, true);
    setupUi();
    updateCalculations();
}

void Tcs206CCalculationDialog::setupUi() {
    setMinimumWidth(480);

    setStyleSheet(
        "QDialog {"
        "  background-color: #FFFFFF;"
        "  border: 2px solid #16A34A;"
        "  border-radius: 6px;"
        "}"
        "QLabel {"
        "  background: transparent;"
        "  color: #0F172A;"
        "  font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, 'Helvetica Neue', 'Noto Sans', 'Liberation Sans', Arial, sans-serif;"
        "}"
        "QLineEdit {"
        "  background-color: #FFFFFF;"
        "  color: #000000;"
        "  border: 1.5px solid #7F9DB9;"
        "  border-radius: 3px;"
        "  padding: 5px 8px;"
        "  font-size: 13px;"
        "  font-weight: 700;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #16A34A;"
        "  background-color: #F0FDF4;"
        "}"
        "QPushButton {"
        "  font-size: 12px;"
        "  font-weight: 700;"
        "  padding: 6px 18px;"
        "  border-radius: 3px;"
        "  min-width: 80px;"
        "}"
    );

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // Header Bar (green = collection; TDS dialog uses blue for deduction)
    QFrame* headerBar = new QFrame(this);
    headerBar->setMinimumHeight(38);
    headerBar->setStyleSheet("background-color: #16A34A; border-top-left-radius: 4px; border-top-right-radius: 4px; border-bottom: none;");
    QHBoxLayout* headerLayout = new QHBoxLayout(headerBar);
    headerLayout->setContentsMargins(14, 0, 14, 0);

    QLabel* titleLabel = new QLabel("TCS u/s 206C(1H) Calculation (Sale Collection)", headerBar);
    titleLabel->setStyleSheet("color: #FFFFFF; font-size: 13px; font-weight: 800;");
    headerLayout->addWidget(titleLabel);
    rootLayout->addWidget(headerBar);

    // Body
    QWidget* bodyWidget = new QWidget(this);
    QVBoxLayout* bodyLayout = new QVBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(18, 14, 18, 14);
    bodyLayout->setSpacing(10);

    // Customer Name banner
    QLabel* custLbl = new QLabel(QString("<b>Party:</b> <font color='#16A34A'>%1</font>").arg(m_customerName.isEmpty() ? "Selected Customer" : m_customerName), bodyWidget);
    custLbl->setStyleSheet("font-size: 13px;");
    custLbl->setWordWrap(true);
    bodyLayout->addWidget(custLbl);

    // Amount stats grid (no turnover rows by design)
    QFrame* statsBox = new QFrame(bodyWidget);
    statsBox->setStyleSheet("background-color: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 4px; padding: 6px;");
    QGridLayout* statsGrid = new QGridLayout(statsBox);
    statsGrid->setContentsMargins(8, 6, 8, 6);
    statsGrid->setHorizontalSpacing(14);
    statsGrid->setVerticalSpacing(4);

    statsGrid->addWidget(new QLabel("Current Bill Gross Amount:", statsBox), 0, 0);
    QLabel* currValLbl = new QLabel(QString("₹ %1").arg(AccountingEngine::formatIndianCurrency(m_currentBillAmount)), statsBox);
    currValLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    currValLbl->setStyleSheet("font-weight: 700; color: #334155;");
    statsGrid->addWidget(currValLbl, 0, 1);

    statsGrid->addWidget(new QLabel("Amount over Total (subject to TCS):", statsBox), 1, 0);
    m_taxableLabel = new QLabel("₹ 0.00", statsBox);
    m_taxableLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_taxableLabel->setStyleSheet("font-size: 12.5px; font-weight: 800; color: #16A34A;");
    statsGrid->addWidget(m_taxableLabel, 1, 1);

    bodyLayout->addWidget(statsBox);

    // Editable Inputs Grid: Rate, TCS override
    QGridLayout* inputGrid = new QGridLayout();
    inputGrid->setHorizontalSpacing(10);
    inputGrid->setVerticalSpacing(8);

    QLabel* rateLbl = new QLabel("TCS Rate (%):", bodyWidget);
    rateLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155;");
    inputGrid->addWidget(rateLbl, 0, 0);

    m_rateInput = new QLineEdit(QString::number(m_tcsRate, 'f', 3), bodyWidget);
    m_rateInput->setAlignment(Qt::AlignRight);
    connect(m_rateInput, &QLineEdit::textChanged, this, &Tcs206CCalculationDialog::updateCalculations);
    connect(m_rateInput, &QLineEdit::returnPressed, this, [this]() {
        updateCalculations();
        if (m_customTcsInput) {
            m_customTcsInput->setFocus();
            m_customTcsInput->selectAll();
        }
    });
    inputGrid->addWidget(m_rateInput, 0, 1);

    QLabel* customTcsLbl = new QLabel("TCS Amount to Collect:", bodyWidget);
    customTcsLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #15803D;");
    inputGrid->addWidget(customTcsLbl, 1, 0);

    m_customTcsInput = new QLineEdit("0.00", bodyWidget);
    m_customTcsInput->setAlignment(Qt::AlignRight);
    connect(m_customTcsInput, &QLineEdit::textEdited, this, [this]() {
        bool ok = false;
        double val = m_customTcsInput->text().trimmed().toDouble(&ok);
        if (ok && val >= 0.0) {
            m_calculatedTcs = val;
            if (m_resultLabel) {
                m_resultLabel->setText(QString("₹ %1").arg(AccountingEngine::formatIndianCurrency(m_calculatedTcs)));
            }
        }
    });
    connect(m_customTcsInput, &QLineEdit::returnPressed, this, [this]() {
        bool ok = false;
        double val = m_customTcsInput->text().trimmed().toDouble(&ok);
        if (ok && val >= 0.0) {
            m_calculatedTcs = val;
        }
        accept();
    });
    inputGrid->addWidget(m_customTcsInput, 1, 1);

    bodyLayout->addLayout(inputGrid);

    // Result Highlight Box
    QFrame* resBox = new QFrame(bodyWidget);
    resBox->setStyleSheet("background-color: #F0FDF4; border: 1.5px solid #86EFAC; border-radius: 4px; padding: 6px;");
    QHBoxLayout* resLayout = new QHBoxLayout(resBox);
    resLayout->setContentsMargins(10, 4, 10, 4);

    QLabel* resTitle = new QLabel("Total TCS u/s 206C(1H) to Collect:", resBox);
    resTitle->setStyleSheet("font-size: 12px; font-weight: 700; color: #166534;");
    resLayout->addWidget(resTitle);

    m_resultLabel = new QLabel("₹ 0.00", resBox);
    m_resultLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_resultLabel->setStyleSheet("font-size: 16px; font-weight: 900; color: #15803D;");
    resLayout->addWidget(m_resultLabel, 1);

    bodyLayout->addWidget(resBox);

    // Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    btnLayout->addStretch(1);

    m_skipBtn = new QPushButton("Skip / Cancel (Esc)", bodyWidget);
    m_skipBtn->setStyleSheet("background-color: #F1F5F9; color: #475569; border: 1px solid #CBD5E1;");
    connect(m_skipBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_skipBtn);

    m_okBtn = new QPushButton("Apply TCS (Enter)", bodyWidget);
    m_okBtn->setStyleSheet("background-color: #16A34A; color: #FFFFFF; border: 1px solid #15803D;");
    m_okBtn->setDefault(true);
    connect(m_okBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(m_okBtn);

    bodyLayout->addLayout(btnLayout);
    rootLayout->addWidget(bodyWidget);
}

void Tcs206CCalculationDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    if (m_customTcsInput) {
        m_customTcsInput->setFocus();
        m_customTcsInput->selectAll();
    }
}

void Tcs206CCalculationDialog::updateCalculations() {
    bool okRate = false;
    if (m_rateInput) m_tcsRate = m_rateInput->text().trimmed().toDouble(&okRate);
    if (!okRate || m_tcsRate < 0.0) m_tcsRate = 0.100;

    // TCS applies on the bill itself (no turnover gate). Prior-turnover and
    // threshold members are retained for future threshold enforcement.
    m_taxableForTcs = m_currentBillAmount;

    m_calculatedTcs = std::round(m_taxableForTcs * (m_tcsRate / 100.0) * 100.0) / 100.0;

    if (m_taxableLabel) {
        m_taxableLabel->setText(QString("₹ %1").arg(AccountingEngine::formatIndianCurrency(m_taxableForTcs)));
    }
    if (m_customTcsInput && !m_customTcsInput->hasFocus()) {
        m_customTcsInput->setText(QString::number(m_calculatedTcs, 'f', 2));
    }
    if (m_resultLabel) {
        m_resultLabel->setText(QString("₹ %1").arg(AccountingEngine::formatIndianCurrency(m_calculatedTcs)));
    }
}

void Tcs206CCalculationDialog::accept() {
    if (m_customTcsInput) {
        bool ok = false;
        double val = m_customTcsInput->text().trimmed().toDouble(&ok);
        if (ok && val >= 0.0) {
            m_calculatedTcs = val;
        }
    }
    QDialog::accept();
}

void Tcs206CCalculationDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        accept();
        event->accept();
    } else if (event->key() == Qt::Key_Escape) {
        reject();
        event->accept();
    } else {
        QDialog::keyPressEvent(event);
    }
}

bool Tcs206CCalculationDialog::promptTcs206C(
    QWidget* parent,
    const QString& customerName,
    double currentBillAmount,
    double* outTcsAmount,
    double* outTcsRate,
    double* outTaxableForTcs,
    double defaultRate
) {
    Tcs206CCalculationDialog dlg(customerName, currentBillAmount, defaultRate, parent);
    if (dlg.exec() == QDialog::Accepted) {
        if (outTcsAmount) *outTcsAmount = dlg.calculatedTcs();
        if (outTcsRate) *outTcsRate = dlg.tcsRate();
        if (outTaxableForTcs) *outTaxableForTcs = dlg.taxableForTcs();
        return true;
    }
    return false;
}

