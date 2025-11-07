#include "dialogs.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>

RGBSettingsDialog::RGBSettingsDialog(int numChannels, int currentR, int currentG, int currentB, QWidget* parent) 
    : QDialog(parent) {
    setWindowTitle("Настройки RGB синтеза");
    setFixedSize(300, 200);
    
    QGridLayout* layout = new QGridLayout(this);
    
    redChannelSelector = new QComboBox();
    greenChannelSelector = new QComboBox();
    blueChannelSelector = new QComboBox();
    
    for (int i = 0; i < numChannels; i++) {
        QString channelName = QString("Канал %1").arg(i + 1);
        redChannelSelector->addItem(channelName);
        greenChannelSelector->addItem(channelName);
        blueChannelSelector->addItem(channelName);
    }
    
    redChannelSelector->setCurrentIndex(currentR);
    greenChannelSelector->setCurrentIndex(currentG);
    blueChannelSelector->setCurrentIndex(currentB);
    
    layout->addWidget(new QLabel("Красный:"), 0, 0);
    layout->addWidget(redChannelSelector, 0, 1);
    layout->addWidget(new QLabel("Зеленый:"), 1, 0);
    layout->addWidget(greenChannelSelector, 1, 1);
    layout->addWidget(new QLabel("Синий:"), 2, 0);
    layout->addWidget(blueChannelSelector, 2, 1);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okButton = new QPushButton("OK");
    QPushButton* cancelButton = new QPushButton("Отмена");
    
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    
    layout->addLayout(buttonLayout, 3, 0, 1, 2);
}

// Contrast Dialog
ContrastDialog::ContrastDialog(HyperspectralImage* image, int channelIndex, QWidget* parent) 
    : QDialog(parent), hyperspectralImage(image), currentChannel(channelIndex), contrastMode(GRAYSCALE_MODE) {
    setWindowTitle("Контрастирование канала " + QString::number(channelIndex + 1));
    setFixedSize(500, 400);
    
    setupUI();
    loadCurrentParams();
}

ContrastDialog::ContrastDialog(HyperspectralImage* image, int redCh, int greenCh, int blueCh, QWidget* parent) 
    : QDialog(parent), hyperspectralImage(image), redChannel(redCh), greenChannel(greenCh), blueChannel(blueCh), contrastMode(RGB_MODE) {
    setWindowTitle("RGB Контрастирование");
    setFixedSize(600, 500);
    
    setupUI();
    loadRGBParams();
}

void ContrastDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Режим контрастирования
    QGroupBox* modeGroup = new QGroupBox("Режим контрастирования");
    QHBoxLayout* modeLayout = new QHBoxLayout(modeGroup);
    
    grayscaleModeRadio = new QRadioButton("Одноканальный");
    rgbModeRadio = new QRadioButton("RGB");
    
    contrastModeGroup = new QButtonGroup(this);
    contrastModeGroup->addButton(grayscaleModeRadio, GRAYSCALE_MODE);
    contrastModeGroup->addButton(rgbModeRadio, RGB_MODE);
    
    if (contrastMode == GRAYSCALE_MODE) {
        grayscaleModeRadio->setChecked(true);
    } else {
        rgbModeRadio->setChecked(true);
    }
    
    connect(contrastModeGroup, QOverload<int>::of(&QButtonGroup::idClicked),
            this, &ContrastDialog::onContrastModeChanged);
    
    modeLayout->addWidget(grayscaleModeRadio);
    modeLayout->addWidget(rgbModeRadio);
    
    mainLayout->addWidget(modeGroup);
    
    setupGrayscaleUI();
    setupRGBUI();
    
    // Кнопки
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* applyButton = new QPushButton("Применить");
    QPushButton* resetButton = new QPushButton("Сброс");
    QPushButton* closeButton = new QPushButton("Закрыть");
    
    connect(applyButton, &QPushButton::clicked, this, &ContrastDialog::applyContrast);
    connect(resetButton, &QPushButton::clicked, this, &ContrastDialog::resetToDefault);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    
    buttonLayout->addWidget(applyButton);
    buttonLayout->addWidget(resetButton);
    buttonLayout->addWidget(closeButton);
    
    mainLayout->addLayout(buttonLayout);
    
    onContrastModeChanged();
}

void ContrastDialog::setupGrayscaleUI() {
    QVBoxLayout* mainLayout = static_cast<QVBoxLayout*>(layout());
    
    QGroupBox* grayscaleGroup = new QGroupBox("Одноканальное контрастирование");
    QVBoxLayout* grayscaleLayout = new QVBoxLayout(grayscaleGroup);
    
    QVBoxLayout* methodLayout = new QVBoxLayout();
    minMaxRadio = new QRadioButton("Указать min/max значения");
    percentileRadio = new QRadioButton("Обрезка гистограммы (%)");
    minMaxRadio->setChecked(true);
    
    methodLayout->addWidget(minMaxRadio);
    methodLayout->addWidget(percentileRadio);
    
    connect(minMaxRadio, &QRadioButton::toggled, this, &ContrastDialog::onModeChanged);
    
    QGroupBox* minMaxGroup = new QGroupBox("Min/Max значения");
    QGridLayout* minMaxLayout = new QGridLayout(minMaxGroup);
    
    minMaxLayout->addWidget(new QLabel("Min:"), 0, 0);
    minSpinBox = new QSpinBox();
    minSpinBox->setRange(0, 65535);
    minSpinBox->setMinimumHeight(30);
    minMaxLayout->addWidget(minSpinBox, 0, 1);
    
    minMaxLayout->addWidget(new QLabel("Max:"), 1, 0);
    maxSpinBox = new QSpinBox();
    maxSpinBox->setRange(0, 65535);
    maxSpinBox->setMinimumHeight(30);
    minMaxLayout->addWidget(maxSpinBox, 1, 1);
    
    QGroupBox* percentileGroup = new QGroupBox("Процент обрезки");
    QGridLayout* percentileLayout = new QGridLayout(percentileGroup);
    
    percentileLayout->addWidget(new QLabel("Слева (%):"), 0, 0);
    percentLowSpinBox = new QDoubleSpinBox();
    percentLowSpinBox->setRange(0.0, 50.0);
    percentLowSpinBox->setSingleStep(0.1);
    percentLowSpinBox->setDecimals(1);
    percentLowSpinBox->setMinimumHeight(30);
    percentileLayout->addWidget(percentLowSpinBox, 0, 1);
    
    percentileLayout->addWidget(new QLabel("Справа (%):"), 1, 0);
    percentHighSpinBox = new QDoubleSpinBox();
    percentHighSpinBox->setRange(0.0, 50.0);
    percentHighSpinBox->setSingleStep(0.1);
    percentHighSpinBox->setDecimals(1);
    percentHighSpinBox->setMinimumHeight(30);
    percentileLayout->addWidget(percentHighSpinBox, 1, 1);
    
    grayscaleLayout->addLayout(methodLayout);
    grayscaleLayout->addWidget(minMaxGroup);
    grayscaleLayout->addWidget(percentileGroup);
    
    mainLayout->insertWidget(1, grayscaleGroup);
}

void ContrastDialog::setupRGBUI() {
    QVBoxLayout* mainLayout = static_cast<QVBoxLayout*>(layout());
    
    QGroupBox* rgbGroup = new QGroupBox("RGB Контрастирование");
    QVBoxLayout* rgbLayout = new QVBoxLayout(rgbGroup);
    
    // Метод контрастирования для RGB
    QVBoxLayout* rgbMethodLayout = new QVBoxLayout();
    rgbMinMaxRadio = new QRadioButton("Указать min/max значения");
    rgbPercentileRadio = new QRadioButton("Обрезка гистограммы (%)");
    rgbMinMaxRadio->setChecked(true);
    
    rgbMethodLayout->addWidget(rgbMinMaxRadio);
    rgbMethodLayout->addWidget(rgbPercentileRadio);

    connect(rgbMinMaxRadio, &QRadioButton::toggled, this, &ContrastDialog::onRGBModeChanged);

    rgbLayout->addLayout(rgbMethodLayout);
    
    // Red Channel
    QGroupBox* redGroup = new QGroupBox("Красный канал");
    QGridLayout* redLayout = new QGridLayout(redGroup);
    
    redLayout->addWidget(new QLabel("Min:"), 0, 0);
    redMinSpinBox = new QSpinBox();
    redMinSpinBox->setRange(0, 65535);
    redLayout->addWidget(redMinSpinBox, 0, 1);
    
    redLayout->addWidget(new QLabel("Max:"), 0, 2);
    redMaxSpinBox = new QSpinBox();
    redMaxSpinBox->setRange(0, 65535);
    redLayout->addWidget(redMaxSpinBox, 0, 3);
    
    redLayout->addWidget(new QLabel("% слева:"), 1, 0);
    redPercentLowSpinBox = new QDoubleSpinBox();
    redPercentLowSpinBox->setRange(0.0, 50.0);
    redPercentLowSpinBox->setSingleStep(0.1);
    redPercentLowSpinBox->setDecimals(1);
    redLayout->addWidget(redPercentLowSpinBox, 1, 1);
    
    redLayout->addWidget(new QLabel("% справа:"), 1, 2);
    redPercentHighSpinBox = new QDoubleSpinBox();
    redPercentHighSpinBox->setRange(0.0, 50.0);
    redPercentHighSpinBox->setSingleStep(0.1);
    redPercentHighSpinBox->setDecimals(1);
    redLayout->addWidget(redPercentHighSpinBox, 1, 3);
    
    // Green Channel
    QGroupBox* greenGroup = new QGroupBox("Зеленый канал");
    QGridLayout* greenLayout = new QGridLayout(greenGroup);
    
    greenLayout->addWidget(new QLabel("Min:"), 0, 0);
    greenMinSpinBox = new QSpinBox();
    greenMinSpinBox->setRange(0, 65535);
    greenLayout->addWidget(greenMinSpinBox, 0, 1);
    
    greenLayout->addWidget(new QLabel("Max:"), 0, 2);
    greenMaxSpinBox = new QSpinBox();
    greenMaxSpinBox->setRange(0, 65535);
    greenLayout->addWidget(greenMaxSpinBox, 0, 3);
    
    greenLayout->addWidget(new QLabel("% слева:"), 1, 0);
    greenPercentLowSpinBox = new QDoubleSpinBox();
    greenPercentLowSpinBox->setRange(0.0, 50.0);
    greenPercentLowSpinBox->setSingleStep(0.1);
    greenPercentLowSpinBox->setDecimals(1);
    greenLayout->addWidget(greenPercentLowSpinBox, 1, 1);
    
    greenLayout->addWidget(new QLabel("% справа:"), 1, 2);
    greenPercentHighSpinBox = new QDoubleSpinBox();
    greenPercentHighSpinBox->setRange(0.0, 50.0);
    greenPercentHighSpinBox->setSingleStep(0.1);
    greenPercentHighSpinBox->setDecimals(1);
    greenLayout->addWidget(greenPercentHighSpinBox, 1, 3);
    
    // Blue Channel
    QGroupBox* blueGroup = new QGroupBox("Синий канал");
    QGridLayout* blueLayout = new QGridLayout(blueGroup);
    
    blueLayout->addWidget(new QLabel("Min:"), 0, 0);
    blueMinSpinBox = new QSpinBox();
    blueMinSpinBox->setRange(0, 65535);
    blueLayout->addWidget(blueMinSpinBox, 0, 1);
    
    blueLayout->addWidget(new QLabel("Max:"), 0, 2);
    blueMaxSpinBox = new QSpinBox();
    blueMaxSpinBox->setRange(0, 65535);
    blueLayout->addWidget(blueMaxSpinBox, 0, 3);
    
    blueLayout->addWidget(new QLabel("% слева:"), 1, 0);
    bluePercentLowSpinBox = new QDoubleSpinBox();
    bluePercentLowSpinBox->setRange(0.0, 50.0);
    bluePercentLowSpinBox->setSingleStep(0.1);
    bluePercentLowSpinBox->setDecimals(1);
    blueLayout->addWidget(bluePercentLowSpinBox, 1, 1);
    
    blueLayout->addWidget(new QLabel("% справа:"), 1, 2);
    bluePercentHighSpinBox = new QDoubleSpinBox();
    bluePercentHighSpinBox->setRange(0.0, 50.0);
    bluePercentHighSpinBox->setSingleStep(0.1);
    bluePercentHighSpinBox->setDecimals(1);
    blueLayout->addWidget(bluePercentHighSpinBox, 1, 3);
    
    rgbLayout->addWidget(redGroup);
    rgbLayout->addWidget(greenGroup);
    rgbLayout->addWidget(blueGroup);
    
    mainLayout->insertWidget(2, rgbGroup);
}

void ContrastDialog::onModeChanged() {
    bool useMinMax = minMaxRadio->isChecked();
    
    minSpinBox->setEnabled(useMinMax);
    maxSpinBox->setEnabled(useMinMax);
    percentLowSpinBox->setEnabled(!useMinMax);
    percentHighSpinBox->setEnabled(!useMinMax);
}

void ContrastDialog::onContrastModeChanged() {
    int selectedMode = contrastModeGroup->checkedId();
    ContrastMode newMode = static_cast<ContrastMode>(selectedMode);
    
    if (newMode != contrastMode) {
        applyContrast();
        contrastMode = newMode;
    }
    
    QVBoxLayout* mainLayout = static_cast<QVBoxLayout*>(layout());
    
    for (int i = 0; i < mainLayout->count(); ++i) {
        QLayoutItem* item = mainLayout->itemAt(i);
        if (item && item->widget()) {
            QGroupBox* group = qobject_cast<QGroupBox*>(item->widget());
            if (group) {
                if (group->title().contains("Одноканальное")) {
                    group->setVisible(contrastMode == GRAYSCALE_MODE);
                } else if (group->title().contains("RGB")) {
                    group->setVisible(contrastMode == RGB_MODE);
                }
            }
        }
    }
    
    if (contrastMode == GRAYSCALE_MODE) {
        onModeChanged();
        loadCurrentParams();
    } else {
        loadRGBParams();
    }
    
    adjustSize();
}

void ContrastDialog::applyContrast() {
    if (contrastMode == GRAYSCALE_MODE) {
        if (minMaxRadio->isChecked()) {
            uint16_t minVal = static_cast<uint16_t>(minSpinBox->value());
            uint16_t maxVal = static_cast<uint16_t>(maxSpinBox->value());
            hyperspectralImage->normalizeToRange(currentChannel, minVal, maxVal);
        } else {
            double percentLow = percentLowSpinBox->value();
            double percentHigh = percentHighSpinBox->value();
            hyperspectralImage->normalizeByPercentile(currentChannel, percentLow, percentHigh);
        }
    } else {
        // RGB режим
        if (rgbMinMaxRadio->isChecked()) {
            // Min/Max для каждого канала
            hyperspectralImage->normalizeToRange(redChannel, 
                static_cast<uint16_t>(redMinSpinBox->value()),
                static_cast<uint16_t>(redMaxSpinBox->value()));
            hyperspectralImage->normalizeToRange(greenChannel, 
                static_cast<uint16_t>(greenMinSpinBox->value()),
                static_cast<uint16_t>(greenMaxSpinBox->value()));
            hyperspectralImage->normalizeToRange(blueChannel, 
                static_cast<uint16_t>(blueMinSpinBox->value()),
                static_cast<uint16_t>(blueMaxSpinBox->value()));
        } else {
            hyperspectralImage->normalizeByPercentile(redChannel, 
                redPercentLowSpinBox->value(), redPercentHighSpinBox->value());
            hyperspectralImage->normalizeByPercentile(greenChannel, 
                greenPercentLowSpinBox->value(), greenPercentHighSpinBox->value());
            hyperspectralImage->normalizeByPercentile(blueChannel, 
                bluePercentLowSpinBox->value(), bluePercentHighSpinBox->value());
        }
    }
    
    emit contrastChanged();
}

void ContrastDialog::resetToDefault() {
    if (contrastMode == GRAYSCALE_MODE) {
        auto [minVal, maxVal] = hyperspectralImage->getChannelMinMax16bit(currentChannel);
        minSpinBox->setValue(minVal);
        maxSpinBox->setValue(maxVal);
        percentLowSpinBox->setValue(2.0);
        percentHighSpinBox->setValue(2.0);
    } else {
        auto [redMin, redMax] = hyperspectralImage->getChannelMinMax16bit(redChannel);
        auto [greenMin, greenMax] = hyperspectralImage->getChannelMinMax16bit(greenChannel);
        auto [blueMin, blueMax] = hyperspectralImage->getChannelMinMax16bit(blueChannel);
        
        redMinSpinBox->setValue(redMin);
        redMaxSpinBox->setValue(redMax);
        redPercentLowSpinBox->setValue(2.0);
        redPercentHighSpinBox->setValue(2.0);
        
        greenMinSpinBox->setValue(greenMin);
        greenMaxSpinBox->setValue(greenMax);
        greenPercentLowSpinBox->setValue(2.0);
        greenPercentHighSpinBox->setValue(2.0);
        
        blueMinSpinBox->setValue(blueMin);
        blueMaxSpinBox->setValue(blueMax);
        bluePercentLowSpinBox->setValue(2.0);
        bluePercentHighSpinBox->setValue(2.0);
    }
}

void ContrastDialog::loadCurrentParams() {
    auto params = hyperspectralImage->getContrastParams(currentChannel);
    
    minSpinBox->setValue(params.minVal);
    maxSpinBox->setValue(params.maxVal);
    percentLowSpinBox->setValue(params.percentCutLow);
    percentHighSpinBox->setValue(params.percentCutHigh);
    
    if (params.usePercentile) {
        percentileRadio->setChecked(true);
    } else {
        minMaxRadio->setChecked(true);
    }
    
    onModeChanged();
}

void ContrastDialog::loadRGBParams() {
    auto redParams = hyperspectralImage->getContrastParams(redChannel);
    auto greenParams = hyperspectralImage->getContrastParams(greenChannel);
    auto blueParams = hyperspectralImage->getContrastParams(blueChannel);
    
    redMinSpinBox->setValue(redParams.minVal);
    redMaxSpinBox->setValue(redParams.maxVal);
    redPercentLowSpinBox->setValue(redParams.percentCutLow);
    redPercentHighSpinBox->setValue(redParams.percentCutHigh);
    
    greenMinSpinBox->setValue(greenParams.minVal);
    greenMaxSpinBox->setValue(greenParams.maxVal);
    greenPercentLowSpinBox->setValue(greenParams.percentCutLow);
    greenPercentHighSpinBox->setValue(greenParams.percentCutHigh);
    
    blueMinSpinBox->setValue(blueParams.minVal);
    blueMaxSpinBox->setValue(blueParams.maxVal);
    bluePercentLowSpinBox->setValue(blueParams.percentCutLow);
    bluePercentHighSpinBox->setValue(blueParams.percentCutHigh);
    
    if (redParams.usePercentile || greenParams.usePercentile || blueParams.usePercentile) {
        rgbPercentileRadio->setChecked(true);
    } else {
        rgbMinMaxRadio->setChecked(true);
    }
}

void ContrastDialog::onRGBModeChanged() {
    bool useMinMax = rgbMinMaxRadio->isChecked();
    
    redMinSpinBox->setEnabled(useMinMax);
    redMaxSpinBox->setEnabled(useMinMax);
    redPercentLowSpinBox->setEnabled(!useMinMax);
    redPercentHighSpinBox->setEnabled(!useMinMax);
    
    greenMinSpinBox->setEnabled(useMinMax);
    greenMaxSpinBox->setEnabled(useMinMax);
    greenPercentLowSpinBox->setEnabled(!useMinMax);
    greenPercentHighSpinBox->setEnabled(!useMinMax);
    
    blueMinSpinBox->setEnabled(useMinMax);
    blueMaxSpinBox->setEnabled(useMinMax);
    bluePercentLowSpinBox->setEnabled(!useMinMax);
    bluePercentHighSpinBox->setEnabled(!useMinMax);
}
