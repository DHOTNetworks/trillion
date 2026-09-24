#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

namespace MahadevERP {

class TdsAcknowledgementDialog : public QDialog {
    Q_OBJECT

public:
    explicit TdsAcknowledgementDialog(QWidget *parent = nullptr);

private slots:
    void onAddClicked();
    void onDeleteClicked();
    void reloadData();

private:
    void setupUi();
    void populateTable();

    QComboBox *m_formTypeCombo = nullptr;
    QComboBox *m_quarterCombo = nullptr;
    QLineEdit *m_ackNoEdit = nullptr;
    QLineEdit *m_filingDateEdit = nullptr;
    QLineEdit *m_deducteesEdit = nullptr;
    QLineEdit *m_taxDepositedEdit = nullptr;
    QLineEdit *m_remarksEdit = nullptr;
    QLabel *m_statusLabel = nullptr;

    QTableWidget *m_table = nullptr;
};

} // namespace MahadevERP
