#pragma once

#include <QDialog>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>

namespace MahadevERP {

class JointReportsDialog : public QDialog {
    Q_OBJECT

public:
    explicit JointReportsDialog(QWidget* parent = nullptr);
    ~JointReportsDialog() override = default;

    bool isTradingSelected() const { return m_chkTrading->isChecked(); }
    bool isProfitLossSelected() const { return m_chkPnL->isChecked(); }
    bool isCapitalSelected() const { return m_chkCapital->isChecked(); }
    bool isBalanceSheetSelected() const { return m_chkBalanceSheet->isChecked(); }

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setupUi();

    QCheckBox* m_chkTrading = nullptr;
    QCheckBox* m_chkPnL = nullptr;
    QCheckBox* m_chkCapital = nullptr;
    QCheckBox* m_chkBalanceSheet = nullptr;
};

} // namespace MahadevERP
