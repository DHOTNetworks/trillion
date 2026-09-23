#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QTimeEdit>
#include <QPushButton>
#include <QLabel>
#include "account_search_box.h"
#include "../models/transport_dispatch_controller.h"

namespace MahadevERP {

class WeighbridgeKandaDialog : public QDialog {
    Q_OBJECT

public:
    explicit WeighbridgeKandaDialog(TransportDispatchController* controller = nullptr, int editDispatchId = 0, QWidget* parent = nullptr);

    int editDispatchId() const { return m_editDispatchId; }
    void loadDispatch(int id);

    static bool openKandaSlip(TransportDispatchController* controller = nullptr, int editDispatchId = 0, QWidget* parent = nullptr);

signals:
    void slipSaved();

private slots:
    void onRecalculate();
    void onSaveClicked();
    void onResetForm();

private:
    void setupUi();

    TransportDispatchController* m_controller = nullptr;
    int m_editDispatchId = 0;

    // Fields
    QLineEdit* m_slipNoEdit = nullptr;
    QDateEdit* m_dateEdit = nullptr;
    QTimeEdit* m_timeEdit = nullptr;
    QLineEdit* m_invoiceNoEdit = nullptr;
    AccountSearchBox* m_partyNameEdit = nullptr;
    QLineEdit* m_itemNameEdit = nullptr;
    QLineEdit* m_gradeEdit = nullptr;
    QLineEdit* m_vehicleNoEdit = nullptr;
    QLineEdit* m_driverNameEdit = nullptr;
    QLineEdit* m_driverPhoneEdit = nullptr;
    AccountSearchBox* m_transporterNameEdit = nullptr;
    QLineEdit* m_transporterGstinEdit = nullptr;
    QLineEdit* m_grNoEdit = nullptr;
    QDateEdit* m_grDateEdit = nullptr;
    QLineEdit* m_destinationEdit = nullptr;
    QLineEdit* m_distanceEdit = nullptr;

    QLineEdit* m_bagCountEdit = nullptr;
    QLineEdit* m_packingKgEdit = nullptr;
    QLineEdit* m_grossWeightEdit = nullptr;
    QLineEdit* m_tareWeightEdit = nullptr;
    QLineEdit* m_bagTareEdit = nullptr;
    QLabel* m_netWeightLabel = nullptr;

    QComboBox* m_freightTypeCombo = nullptr;
    QLineEdit* m_freightRateEdit = nullptr;
    QLabel* m_totalFreightLabel = nullptr;
    QLineEdit* m_advanceFreightEdit = nullptr;
    QLabel* m_balanceFreightLabel = nullptr;

    QLineEdit* m_ewayBillEdit = nullptr;
    QLineEdit* m_irnEdit = nullptr;
    QLineEdit* m_notesEdit = nullptr;
};

} // namespace MahadevERP
