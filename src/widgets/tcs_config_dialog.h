#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "account_search_box.h"

namespace MahadevERP {

class TcsConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit TcsConfigDialog(QWidget *parent = nullptr);

private slots:
    void onSaveClicked();

private:
    void setupUi();
    void loadSettings();

    QLineEdit *m_thresholdEdit = nullptr;
    QLineEdit *m_rateWithPanEdit = nullptr;
    QLineEdit *m_rateWithoutPanEdit = nullptr;
    AccountSearchBox *m_tcsLedgerSearch = nullptr;
    QLabel *m_statusLabel = nullptr;
};

} // namespace MahadevERP
