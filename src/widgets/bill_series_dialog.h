#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include "../engine/bill_series.h"

namespace MahadevERP {

// Bill Series config dialog: the user's sale-invoice numbering system.
// Everything is stored as one JSON row ("bill_series/sale") in app_settings
// inside the firm's own .db file, so the format travels with the file.
// (Purchase has no auto-invoice — its bill number is the supplier's own.)
class BillSeriesDialog : public QDialog {
    Q_OBJECT

public:
    explicit BillSeriesDialog(QWidget* parent = nullptr);

private slots:
    void updatePreview();
    void onSaveClicked();

private:
    void setupUi();
    void loadSettings();
    BillSeriesConfig collect() const;
    void showError(const QString& msg);
    void clearError();

    QLineEdit* m_prefixEdit = nullptr;
    QLineEdit* m_sepAEdit = nullptr;
    QComboBox* m_fyCombo = nullptr;
    QLineEdit* m_sepBEdit = nullptr;
    QSpinBox* m_widthSpin = nullptr;
    QSpinBox* m_startSpin = nullptr;
    QCheckBox* m_resetCheck = nullptr;
    QLineEdit* m_suffixEdit = nullptr;
    QLabel* m_previewLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPushButton* m_saveBtn = nullptr;
};

} // namespace MahadevERP
