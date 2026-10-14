#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

namespace MahadevERP {

class Form16AListWidget : public QWidget {
    Q_OBJECT

public:
    explicit Form16AListWidget(QWidget *parent = nullptr);

    void reloadData();
    void markDirty() { m_isDirty = true; }
    bool isDirty() const { return m_isDirty; }
    void focusTable();

signals:
    void backRequested();
    void newCertificateRequested();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onNewClicked();
    void onDeleteClicked();
    void onRefreshClicked();
    void onFilterChanged();

private:
    void setupUi();
    void populateTable();

    QLineEdit *m_searchEdit = nullptr;
    QTableWidget *m_table = nullptr;

    QLabel *m_totalCertsVal = nullptr;
    QLabel *m_totalPaidVal = nullptr;
    QLabel *m_totalTdsVal = nullptr;

    QVariantList m_currentCerts;
    bool m_isDirty = true;
};

} // namespace MahadevERP
