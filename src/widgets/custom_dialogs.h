#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>

// ============================================================================
// CustomInputDialog: Themed text input modal (Bahi-Khata / ERP styled)
// ============================================================================
class CustomInputDialog : public QDialog {
    Q_OBJECT

public:
    explicit CustomInputDialog(const QString& title, const QString& prompt,
                               const QString& defaultText = "", QWidget* parent = nullptr);
    ~CustomInputDialog() override = default;

    QString value() const;

    static QString getText(QWidget* parent, const QString& title, const QString& prompt,
                           const QString& defaultText = "", bool* ok = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void setupUi(const QString& title, const QString& prompt, const QString& defaultText);

    QLineEdit* m_textInput = nullptr;
    QPushButton* m_okBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;
};

// ============================================================================
// CustomMessageBox: Themed notification & confirmation modal
// ============================================================================
class CustomMessageBox : public QDialog {
    Q_OBJECT

public:
    enum IconType { Info, Warning, Error, Question };

    explicit CustomMessageBox(const QString& title, const QString& message,
                              IconType iconType, const QString& primaryBtnText,
                              const QString& secondaryBtnText = "", QWidget* parent = nullptr);
    ~CustomMessageBox() override = default;

    static void information(QWidget* parent, const QString& title, const QString& message);
    static void warning(QWidget* parent, const QString& title, const QString& message);
    static void critical(QWidget* parent, const QString& title, const QString& message);
    static bool question(QWidget* parent, const QString& title, const QString& message,
                         const QString& yesText = "Yes", const QString& noText = "No");

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setupUi(const QString& title, const QString& message, IconType iconType,
                 const QString& primaryBtnText, const QString& secondaryBtnText);

    QPushButton* m_primaryBtn = nullptr;
    QPushButton* m_secondaryBtn = nullptr;
};
