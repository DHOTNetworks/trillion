#include "custom_dialogs.h"
#include <QApplication>
#include <QGraphicsDropShadowEffect>

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
    setFixedWidth(420);

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
    headerBar->setFixedHeight(36);
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
    m_textInput->setFixedHeight(32);
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
    btnLayout->addWidget(m_okBtn);

    bodyLayout->addLayout(btnLayout);
    rootLayout->addWidget(bodyWidget);
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
    setFixedWidth(400);

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
    headerBar->setFixedHeight(36);
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
        m_secondaryBtn->setStyleSheet("background-color: #F1F5F9; color: #334155; border: 1px solid #CBD5E1;");
        connect(m_secondaryBtn, &QPushButton::clicked, this, &QDialog::reject);
        btnLayout->addWidget(m_secondaryBtn);
    }

    m_primaryBtn = new QPushButton(primaryBtnText, bodyWidget);
    m_primaryBtn->setStyleSheet(QString("background-color: %1; color: #FFFFFF; border: 1px solid %1;").arg(headerBg));
    m_primaryBtn->setDefault(true);
    connect(m_primaryBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(m_primaryBtn);

    bodyLayout->addLayout(btnLayout);
    rootLayout->addWidget(bodyWidget);
}

void CustomMessageBox::keyPressEvent(QKeyEvent* event) {
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
