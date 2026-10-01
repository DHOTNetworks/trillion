#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMap>

class ScaleSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit ScaleSettingsDialog(QWidget* parent = nullptr);
    ~ScaleSettingsDialog() override = default;

private slots:
    void onSliderValueChanged(int value);
    void onPresetButtonClicked(int percent);
    void onAutoDetectClicked();
    void onResetClicked();
    void onStepDownClicked();
    void onStepUpClicked();
    void onApplyAndSaveClicked();
    void onCancelClicked();

private:
    void setupUi();
    void updateScaleDisplay(int percent);
    void updatePresetButtonStyles();

    int m_initialPercent = 100;
    int m_selectedPercent = 100;

    QLabel* m_currentPercentBadge = nullptr;
    QLabel* m_previewSampleLabel = nullptr;
    QSlider* m_scaleSlider = nullptr;
    QPushButton* m_minusBtn = nullptr;
    QPushButton* m_plusBtn = nullptr;
    QMap<int, QPushButton*> m_presetButtons;

    QPushButton* m_autoDetectBtn = nullptr;
    QPushButton* m_resetBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
};
