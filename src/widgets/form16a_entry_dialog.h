#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include "account_search_box.h"

namespace MahadevERP {

class Form16AEntryDialog : public QDialog {
    Q_OBJECT

public:
    explicit Form16AEntryDialog(QWidget *parent = nullptr);

signals:
    void certificateSaved();

private slots:
    void onSaveClicked();

private:
    void setupUi();

    QLineEdit *m_certNoEdit = nullptr;
    QLineEdit *m_dateEdit = nullptr;
    AccountSearchBox *m_partySearch = nullptr;
    QLineEdit *m_panEdit = nullptr;
    QComboBox *m_quarterCombo = nullptr;
    QLineEdit *m_grossAmtEdit = nullptr;
    QLineEdit *m_tdsAmtEdit = nullptr;
    QComboBox *m_matchedCombo = nullptr;
    QLineEdit *m_remarksEdit = nullptr;
    QLabel *m_statusLabel = nullptr;
};

} // namespace MahadevERP
