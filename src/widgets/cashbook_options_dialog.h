#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QVector>

namespace MahadevERP {

enum class CashBookOptionResult {
    FlatType,
    OtherType,
    DayWiseBalance,
    FindNegativeBal,
    FindMaximumBal,
    FindMinimumBal,
    CashFlowStatement,
    BankFlowStatement,
    JointFlowStatement,
    Cancelled
};

class CashBookOptionsDialog : public QDialog {
    Q_OBJECT

public:
    explicit CashBookOptionsDialog(QWidget* parent = nullptr);
    ~CashBookOptionsDialog() override = default;

    CashBookOptionResult selectedOption() const { return m_result; }

protected:
    void showEvent(QShowEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void setupUi();
    void updateSelection(int idx);
    void triggerCurrent();

    struct OptionEntry {
        QString title;
        QString hotkeyLetter;
        int hotkeyKey;
        CashBookOptionResult result;
        bool isSeparator = false;
    };

    QVector<OptionEntry> m_options;
    QVector<QFrame*> m_itemFrames;
    int m_selectedIndex = 0;
    CashBookOptionResult m_result = CashBookOptionResult::Cancelled;
};

} // namespace MahadevERP
