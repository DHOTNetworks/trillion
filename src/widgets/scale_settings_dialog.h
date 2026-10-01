#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QButtonGroup>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

class ScaleSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit ScaleSettingsDialog(QWidget* parent = nullptr);
    ~ScaleSettingsDialog() override = default;

private slots:
    void onSliderValueChanged(int value);
    void onPresetButtonClicked(int id);
    void onAutoDetectClicked();
    void onResetClicked();
    void onApplyAndSaveClicked();
    void onCancelClicked();

private:
    void setupUi();
    void updateSliderDisplay(int percent);

    int m_initialPercent = 100;
    int m_selectedPercent = 100;

    QLabel* m_currentPercentLabel = nullptr;
    QLabel* m_previewSampleLabel = nullptr;
    QSlider* m_scaleSlider = nullptr;
    QButtonGroup* m_presetGroup = nullptr;
    QPushButton* m_autoDetectBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;
    QPushButton* m_resetBtn = nullptr;
};
