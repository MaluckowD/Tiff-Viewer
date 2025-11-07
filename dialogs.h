#ifndef DIALOGS_H
#define DIALOGS_H

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include "hyperspectral_image.h"

class RGBSettingsDialog : public QDialog {
    Q_OBJECT
    
public:
    RGBSettingsDialog(int numChannels, int currentR, int currentG, int currentB, QWidget* parent = nullptr);
    
    int getRedChannel() const { return redChannelSelector->currentIndex(); }
    int getGreenChannel() const { return greenChannelSelector->currentIndex(); }
    int getBlueChannel() const { return blueChannelSelector->currentIndex(); }
    
private:
    QComboBox* redChannelSelector;
    QComboBox* greenChannelSelector;
    QComboBox* blueChannelSelector;
};

class ContrastDialog : public QDialog {
    Q_OBJECT
    
public:
    enum ContrastMode {
        GRAYSCALE_MODE,
        RGB_MODE
    };
    
    ContrastDialog(HyperspectralImage* image, int channelIndex, QWidget* parent = nullptr);
    ContrastDialog(HyperspectralImage* image, int redCh, int greenCh, int blueCh, QWidget* parent = nullptr);
    
private slots:
    void onModeChanged();
    void onContrastModeChanged();
    void onRGBModeChanged();
    void applyContrast();
    void resetToDefault();

signals:
    void contrastChanged();
    
private:
    void setupUI();
    void setupGrayscaleUI();
    void setupRGBUI();
    void loadCurrentParams();
    void loadRGBParams();
    
    HyperspectralImage* hyperspectralImage;
    int currentChannel;
    int redChannel, greenChannel, blueChannel;
    ContrastMode contrastMode;
    
    // Режим контрастирования
    QRadioButton* grayscaleModeRadio;
    QRadioButton* rgbModeRadio;
    QButtonGroup* contrastModeGroup;
    
    // Grayscale контролы
    QRadioButton* minMaxRadio;
    QRadioButton* percentileRadio;
    QSpinBox* minSpinBox;
    QSpinBox* maxSpinBox;
    QDoubleSpinBox* percentLowSpinBox;
    QDoubleSpinBox* percentHighSpinBox;
    
    // RGB контролы
    QRadioButton* rgbMinMaxRadio;
    QRadioButton* rgbPercentileRadio;
    
    // Red channel
    QSpinBox* redMinSpinBox;
    QSpinBox* redMaxSpinBox;
    QDoubleSpinBox* redPercentLowSpinBox;
    QDoubleSpinBox* redPercentHighSpinBox;
    
    // Green channel
    QSpinBox* greenMinSpinBox;
    QSpinBox* greenMaxSpinBox;
    QDoubleSpinBox* greenPercentLowSpinBox;
    QDoubleSpinBox* greenPercentHighSpinBox;
    
    // Blue channel
    QSpinBox* blueMinSpinBox;
    QSpinBox* blueMaxSpinBox;
    QDoubleSpinBox* bluePercentLowSpinBox;
    QDoubleSpinBox* bluePercentHighSpinBox;
};

#endif
