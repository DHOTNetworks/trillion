#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QFutureWatcher>
#include "../models/tax_challan_controller.h"

namespace MahadevERP {

class TaxChallanRegisterWidget : public QWidget {
    Q_OBJECT

public:
    explicit TaxChallanRegisterWidget(TaxChallanController *controller, const QString &initialType = "TDS", QWidget *parent = nullptr);

    void setTaxType(const QString &type);
    void reloadData();
    void focusTable();
    void markDirty() { m_isDirty = true; }
    bool isDirty() const { return m_isDirty; }

signals:
    void backRequested();
    void newChallanRequested(const QString &taxType);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onNewClicked();
    void onDeleteClicked();
    void onDeleteFinished();
    void onQueryReady();
    void onRefreshClicked();
    void onFilterChanged();

private:
    void setupUi();
    void populateTable();

    TaxChallanController *m_controller = nullptr;
    QString m_taxType = "TDS";

    QLabel *m_titleLabel = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QComboBox *m_typeFilterCombo = nullptr;
    QTableWidget *m_table = nullptr;

    QFutureWatcher<bool> m_deleteWatcher;
    QFutureWatcher<QVariantList> m_queryWatcher;
    bool m_deleting = false;

    QLabel *m_totalChallansVal = nullptr;
    QLabel *m_basicTaxVal = nullptr;
    QLabel *m_interestPenaltyVal = nullptr;
    QLabel *m_totalDepositedVal = nullptr;

    QVariantList m_currentChallans;
    bool m_isDirty = true;
};

} // namespace MahadevERP
