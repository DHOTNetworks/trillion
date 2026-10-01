#include "scale_settings_dialog.h"
#include "../services/scale_manager.h"
#include <QFrame>
#include <QGridLayout>
#include <QScrollArea>
#include <QGuiApplication>
#include <QScreen>

ScaleSettingsDialog::ScaleSettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    m_initialPercent = ScaleManager::instance().currentScalePercent();
    m_selectedPercent = m_initialPercent;

    setWindowTitle("Display & UI Scaling Settings");
    setMinimumWidth(560);
    setMaximumWidth(680);
    setModal(true);

    setupUi();
}

void ScaleSettingsDialog::setupUi() {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("ScaleSettingsDialog { background-color: #F8FAFC; font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif; }");

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(24, 20, 24, 20);
    rootLayout->setSpacing(16);

    // 1. Header Card
    QFrame* headerCard = new QFrame(this);
    headerCard->setStyleSheet("background-color: #FFFFFF; border: 1.5px solid #E2E8F0; border-radius: 10px; padding: 14px;");
    QVBoxLayout* headerLayout = new QVBoxLayout(headerCard);
    headerLayout->setContentsMargins(12, 10, 12, 10);
    headerLayout->setSpacing(4);

    QLabel* titleLabel = new QLabel("🖥️ Display & UI Scaling Settings", headerCard);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: 800; color: #0F172A; border: none; background: transparent;");
    headerLayout->addWidget(titleLabel);

    QLabel* subLabel = new QLabel(
        "Adjust interface scale, text sharpness, and table heights live. Recommended for 27\" Desktop Monitors (1440p) and 4K displays.",
        headerCard
    );
    subLabel->setWordWrap(true);
    subLabel->setStyleSheet("font-size: 12px; font-weight: 500; color: #64748B; border: none; background: transparent;");
    headerLayout->addWidget(subLabel);
    rootLayout->addWidget(headerCard);

    // 2. Active Scale Display & Live Slider Card
    QFrame* sliderCard = new QFrame(this);
    sliderCard->setStyleSheet("background-color: #FFFFFF; border: 1.5px solid #E2E8F0; border-radius: 10px; padding: 16px;");
    QVBoxLayout* sliderCardLayout = new QVBoxLayout(sliderCard);
    sliderCardLayout->setContentsMargins(14, 12, 14, 12);
    sliderCardLayout->setSpacing(12);

    QHBoxLayout* topSliderRow = new QHBoxLayout();
    QLabel* scaleHeading = new QLabel("Interactive Scale Slider", sliderCard);
    scaleHeading->setStyleSheet("font-size: 14px; font-weight: 700; color: #1E293B; border: none; background: transparent;");
    topSliderRow->addWidget(scaleHeading);

    topSliderRow->addStretch(1);

    m_currentPercentLabel = new QLabel(QString("%1%").arg(m_selectedPercent), sliderCard);
    m_currentPercentLabel->setStyleSheet(
        "font-size: 15px; font-weight: 800; color: #2563EB; background-color: #EFF6FF; "
        "border: 1.5px solid #BFDBFE; border-radius: 6px; padding: 3px 12px;"
    );
    topSliderRow->addWidget(m_currentPercentLabel);
    sliderCardLayout->addLayout(topSliderRow);

    m_scaleSlider = new QSlider(Qt::Horizontal, sliderCard);
    m_scaleSlider->setRange(80, 200);
    m_scaleSlider->setSingleStep(5);
    m_scaleSlider->setPageStep(10);
    m_scaleSlider->setValue(m_selectedPercent);
    m_scaleSlider->setTickPosition(QSlider::TicksBelow);
    m_scaleSlider->setTickInterval(25);
    m_scaleSlider->setStyleSheet(
        "QSlider::groove:horizontal { height: 8px; background: #E2E8F0; border-radius: 4px; }"
        "QSlider::sub-page:horizontal { background: #3B82F6; border-radius: 4px; }"
        "QSlider::handle:horizontal { background: #2563EB; border: 2px solid #FFFFFF; width: 20px; margin-top: -6px; margin-bottom: -6px; border-radius: 10px; }"
        "QSlider::handle:horizontal:hover { background: #1D4ED8; }"
    );
    connect(m_scaleSlider, &QSlider::valueChanged, this, &ScaleSettingsDialog::onSliderValueChanged);
    sliderCardLayout->addWidget(m_scaleSlider);

    // Live Text Preview Box
    QFrame* previewBox = new QFrame(sliderCard);
    previewBox->setStyleSheet("background-color: #F8FAFC; border: 1px dashed #CBD5E1; border-radius: 8px; padding: 10px;");
    QVBoxLayout* previewLayout = new QVBoxLayout(previewBox);
    previewLayout->setContentsMargins(10, 8, 10, 8);
    previewLayout->setSpacing(4);

    QLabel* previewHeader = new QLabel("LIVE PREVIEW SAMPLE", previewBox);
    previewHeader->setStyleSheet("font-size: 10px; font-weight: 800; color: #64748B; letter-spacing: 0.5px; border: none; background: transparent;");
    previewLayout->addWidget(previewHeader);

    m_previewSampleLabel = new QLabel("M/s Balaji Rice Mill • ₹ 1,42,85,690.00 Dr • Basmati 1121 Steam", previewBox);
    m_previewSampleLabel->setStyleSheet("font-weight: 700; color: #0F172A; border: none; background: transparent;");
    previewLayout->addWidget(m_previewSampleLabel);
    sliderCardLayout->addWidget(previewBox);

    rootLayout->addWidget(sliderCard);

    // 3. Preset Quick Selection Cards
    QLabel* presetTitle = new QLabel("Display Presets & Recommendations", this);
    presetTitle->setStyleSheet("font-size: 13px; font-weight: 800; color: #334155; padding-left: 4px;");
    rootLayout->addWidget(presetTitle);

    QFrame* presetsCard = new QFrame(this);
    presetsCard->setStyleSheet("background-color: #FFFFFF; border: 1.5px solid #E2E8F0; border-radius: 10px; padding: 12px;");
    QVBoxLayout* presetsLayout = new QVBoxLayout(presetsCard);
    presetsLayout->setContentsMargins(10, 8, 10, 8);
    presetsLayout->setSpacing(8);

    m_presetGroup = new QButtonGroup(this);
    auto options = ScaleManager::instance().availableScaleOptions();
    for (int i = 0; i < options.size(); ++i) {
        const auto& opt = options.at(i);
        QRadioButton* radio = new QRadioButton(QString("<b>%1</b>  —  <span style='color: #64748B; font-size: 11px;'>%2</span>").arg(opt.label, opt.recommendation), presetsCard);
        radio->setStyleSheet(
            "QRadioButton { font-size: 12.5px; color: #0F172A; padding: 4px 8px; }"
            "QRadioButton::indicator { width: 16px; height: 16px; }"
        );
        if (opt.percent == m_selectedPercent) {
            radio->setChecked(true);
        }
        m_presetGroup->addButton(radio, opt.percent);
        presetsLayout->addWidget(radio);
    }
    connect(m_presetGroup, &QButtonGroup::idClicked, this, &ScaleSettingsDialog::onPresetButtonClicked);
    rootLayout->addWidget(presetsCard);

    // 4. Action Buttons Footer
    QHBoxLayout* footerLayout = new QHBoxLayout();
    footerLayout->setSpacing(10);

    m_autoDetectBtn = new QPushButton("🎯 Auto-Detect Display", this);
    m_autoDetectBtn->setFixedHeight(38);
    m_autoDetectBtn->setCursor(Qt::PointingHandCursor);
    m_autoDetectBtn->setStyleSheet(
        "QPushButton { background-color: #EFF6FF; border: 1.5px solid #3B82F6; border-radius: 6px; padding: 0px 14px; font-weight: 700; color: #1D4ED8; font-size: 12px; }"
        "QPushButton:hover { background-color: #DBEAFE; }"
    );
    connect(m_autoDetectBtn, &QPushButton::clicked, this, &ScaleSettingsDialog::onAutoDetectClicked);
    footerLayout->addWidget(m_autoDetectBtn);

    m_resetBtn = new QPushButton("Reset (100%)", this);
    m_resetBtn->setFixedHeight(38);
    m_resetBtn->setCursor(Qt::PointingHandCursor);
    m_resetBtn->setStyleSheet(
        "QPushButton { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 0px 14px; font-weight: 700; color: #475569; font-size: 12px; }"
        "QPushButton:hover { background-color: #F1F5F9; }"
    );
    connect(m_resetBtn, &QPushButton::clicked, this, &ScaleSettingsDialog::onResetClicked);
    footerLayout->addWidget(m_resetBtn);

    footerLayout->addStretch(1);

    m_cancelBtn = new QPushButton("Cancel", this);
    m_cancelBtn->setFixedHeight(38);
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    m_cancelBtn->setStyleSheet(
        "QPushButton { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 0px 18px; font-weight: 700; color: #64748B; font-size: 12px; }"
        "QPushButton:hover { background-color: #F1F5F9; }"
    );
    connect(m_cancelBtn, &QPushButton::clicked, this, &ScaleSettingsDialog::onCancelClicked);
    footerLayout->addWidget(m_cancelBtn);

    m_saveBtn = new QPushButton("✓ Apply & Save Setting", this);
    m_saveBtn->setFixedHeight(38);
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    m_saveBtn->setStyleSheet(
        "QPushButton { background-color: #16A34A; border: none; border-radius: 6px; padding: 0px 22px; font-weight: 800; color: #FFFFFF; font-size: 12.5px; }"
        "QPushButton:hover { background-color: #15803D; }"
    );
    connect(m_saveBtn, &QPushButton::clicked, this, &ScaleSettingsDialog::onApplyAndSaveClicked);
    footerLayout->addWidget(m_saveBtn);

    rootLayout->addLayout(footerLayout);
}

void ScaleSettingsDialog::updateSliderDisplay(int percent) {
    m_selectedPercent = percent;
    if (m_currentPercentLabel) {
        m_currentPercentLabel->setText(QString("%1%").arg(percent));
    }

    // Apply live scaling dynamically to test appearance immediately
    ScaleManager::instance().setScalePercent(percent, false);

    // Sync radio buttons
    QAbstractButton* btn = m_presetGroup->button(percent);
    if (btn && !btn->isChecked()) {
        btn->setChecked(true);
    }
}

void ScaleSettingsDialog::onSliderValueChanged(int value) {
    updateSliderDisplay(value);
}

void ScaleSettingsDialog::onPresetButtonClicked(int id) {
    if (m_scaleSlider && m_scaleSlider->value() != id) {
        m_scaleSlider->setValue(id);
    } else {
        updateSliderDisplay(id);
    }
}

void ScaleSettingsDialog::onAutoDetectClicked() {
    int rec = ScaleManager::instance().getRecommendedScalePercent();
    if (m_scaleSlider) {
        m_scaleSlider->setValue(rec);
    }
}

void ScaleSettingsDialog::onResetClicked() {
    if (m_scaleSlider) {
        m_scaleSlider->setValue(100);
    }
}

void ScaleSettingsDialog::onApplyAndSaveClicked() {
    // Permanently persist to database and accept dialog
    ScaleManager::instance().setScalePercent(m_selectedPercent, true);
    accept();
}

void ScaleSettingsDialog::onCancelClicked() {
    // Revert to initial scale
    ScaleManager::instance().setScalePercent(m_initialPercent, false);
    reject();
}
