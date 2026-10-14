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

    // Convenience aliases
    static void showInformation(QWidget* parent, const QString& title, const QString& message) {
        information(parent, title, message);
    }
    static void showWarning(QWidget* parent, const QString& title, const QString& message) {
        warning(parent, title, message);
    }
    static void showCritical(QWidget* parent, const QString& title, const QString& message) {
        critical(parent, title, message);
    }
    static bool showConfirmation(QWidget* parent, const QString& title, const QString& message,
                                 const QString& yesText = "Yes", const QString& noText = "No") {
        return question(parent, title, message, yesText, noText);
    }

private slots:
    void onConfirmKey();
    void onMoveFocus(int direction);

private:
    void setupUi(const QString& title, const QString& message, IconType iconType,
                 const QString& primaryBtnText, const QString& secondaryBtnText);
    void setupKeyboardShortcuts();

    QPushButton* m_primaryBtn = nullptr;
    QPushButton* m_secondaryBtn = nullptr;
};

// ============================================================================
// FreightCalculationDialog: Auto-calculates freight from Kanda Weight / Bags
// ============================================================================
class QComboBox;

class FreightCalculationDialog : public QDialog {
    Q_OBJECT

public:
    explicit FreightCalculationDialog(double kandaWeight, int totalBags, QWidget* parent = nullptr);
    ~FreightCalculationDialog() override = default;

    double calculatedFreight() const { return m_calculatedFreight; }
    double freightRate() const { return m_freightRate; }
    QString rateUnit() const { return m_rateUnit; }

    static bool promptFreight(QWidget* parent, double kandaWeight, int totalBags,
                              double* outFreight, double* outRate = nullptr, QString* outUnit = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void setupUi();
    void updateCalculations();

    double m_kandaWeight = 0.0;
    int m_totalBags = 0;
    double m_freightRate = 0.0;
    double m_calculatedFreight = 0.0;
    QString m_rateUnit = "Per Qtl";

    QLineEdit* m_rateInput = nullptr;
    QComboBox* m_unitCombo = nullptr;
    QLabel* m_resultLabel = nullptr;
    QPushButton* m_okBtn = nullptr;
    QPushButton* m_skipBtn = nullptr;
};

// ============================================================================
// Tds194QCalculationDialog: TDS u/s 194Q calculation on purchases > threshold
// ============================================================================
class Tds194QCalculationDialog : public QDialog {
    Q_OBJECT

public:
    explicit Tds194QCalculationDialog(
        const QString& supplierName,
        double priorTurnover,
        double currentBillAmount,
        double defaultThreshold = 5000000.0,
        double defaultRate = 0.100,
        QWidget* parent = nullptr
    );
    ~Tds194QCalculationDialog() override = default;

    double calculatedTds() const { return m_calculatedTds; }
    double tdsRate() const { return m_tdsRate; }
    double thresholdLimit() const { return m_thresholdLimit; }
    double taxableForTds() const { return m_taxableForTds; }

    void accept() override;

    static bool promptTds194Q(
        QWidget* parent,
        const QString& supplierName,
        double priorTurnover,
        double currentBillAmount,
        double* outTdsAmount,
        double* outTdsRate = nullptr,
        double* outTaxableForTds = nullptr,
        double defaultThreshold = 5000000.0,
        double defaultRate = 0.100
    );

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void setupUi();
    void updateCalculations();

    QString m_supplierName;
    double m_priorTurnover = 0.0;
    double m_currentBillAmount = 0.0;
    double m_thresholdLimit = 5000000.0;
    double m_tdsRate = 0.100;
    double m_taxableForTds = 0.0;
    double m_calculatedTds = 0.0;

    QLineEdit* m_thresholdInput = nullptr;
    QLineEdit* m_rateInput = nullptr;
    QLineEdit* m_customTdsInput = nullptr;
    QLabel* m_taxableLabel = nullptr;
    QLabel* m_resultLabel = nullptr;
    QPushButton* m_okBtn = nullptr;
    QPushButton* m_skipBtn = nullptr;
};

// Tcs206CCalculationDialog: TCS u/s 206C(1H) collection on sales.
// Same dialog shape as Tds194QCalculationDialog (party banner, amount over
// total, editable rate, editable TCS override, result box, Apply/Skip).
// Difference: NO turnover/threshold gate — TCS applies on the bill itself,
// so turnover members are kept (future use) but not shown.
// ============================================================================
class Tcs206CCalculationDialog : public QDialog {
    Q_OBJECT

public:
    explicit Tcs206CCalculationDialog(
        const QString& customerName,
        double currentBillAmount,
        double defaultRate = 0.100,
        QWidget* parent = nullptr
    );
    ~Tcs206CCalculationDialog() override = default;

    double calculatedTcs() const { return m_calculatedTcs; }
    double tcsRate() const { return m_tcsRate; }
    double taxableForTcs() const { return m_taxableForTcs; }

    // Turnover tracking kept for future threshold enforcement; currently
    // unused by the calculation (TCS applies on the bill itself).
    void setPriorTurnover(double t) { m_priorTurnover = t; }
    void setThresholdLimit(double t) { m_thresholdLimit = t; }

    void accept() override;

    static bool promptTcs206C(
        QWidget* parent,
        const QString& customerName,
        double currentBillAmount,
        double* outTcsAmount,
        double* outTcsRate = nullptr,
        double* outTaxableForTcs = nullptr,
        double defaultRate = 0.100
    );

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void setupUi();
    void updateCalculations();

    QString m_customerName;
    double m_priorTurnover = 0.0;
    double m_currentBillAmount = 0.0;
    double m_thresholdLimit = 5000000.0;
    double m_tcsRate = 0.100;
    double m_taxableForTcs = 0.0;
    double m_calculatedTcs = 0.0;

    QLineEdit* m_rateInput = nullptr;
    QLineEdit* m_customTcsInput = nullptr;
    QLabel* m_taxableLabel = nullptr;
    QLabel* m_resultLabel = nullptr;
    QPushButton* m_okBtn = nullptr;
    QPushButton* m_skipBtn = nullptr;
};

