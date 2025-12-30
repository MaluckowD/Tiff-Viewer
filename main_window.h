#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QScrollArea>
#include <QComboBox>
#include <QStatusBar>
#include <QLabel>
#include <QMenuBar>
#include <QAction>
#include <QMenu>
#include <QFrame>
#include <QPushButton>
#include <QSplitter>
#include <QListWidget>
#include <QLibrary>
#include "image_label.h"
#include "histogram_widget.h"
#include "hyperspectral_image.h"
#include "spectral_reader.h"
#include "spectral_curve_dialog.h"

// Объявление типа функции из DLL
typedef void (*ShowCalibrationDialogFunc)(QWidget*);

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void openFile();
    void displayChannel(int channelIndex);
    void updateHistogram();
    void openRGBSettings();
    void displayRGBImage();
    void openContrastDialog();
    void onContrastChanged();
    void closeImage();
    void onMousePosition(int x, int y);
    void onHistogramChannelChanged();
    void openSpectralInfo();
    void autoContrast();
    void showSpectralCurve(int x, int y);
    void updateSpectralCurve();
    void onAddPointClicked();
    void onRemovePointClicked();
    void onClearPointsClicked();
    void onLegendItemDoubleClicked(QListWidgetItem* item);
    void openCalibration();

private:
    void createUI();
    void createMenus();
    void setupStatusBar();
    void loadSpectralData(const QString& tiffFilePath);
    void applyAutoContrast();
    void updateSpectralCurveForMousePosition(int x, int y);
    void updateLegend();
    QColor getNextColor();
    void loadCalibrationModule();  // Загрузка DLL

    // UI элементы
    ImageLabel* imageLabel;
    QScrollArea* scrollArea;
    QComboBox* channelSelector;
    QComboBox* histogramChannelSelector;
    QStatusBar* statusBar;
    HistogramWidget* histogramWidget;
    QPushButton* autoContrastButton;
    
    // Встроенный виджет спектральных кривых
    SpectralCurveWidget* spectralCurveWidget;
    QLabel* spectralCurveLabel;
    QSplitter* rightSplitter;
    
    QListWidget* legendWidget;
    QPushButton* addPointButton;
    QPushButton* removePointButton;
    QPushButton* clearPointsButton;
    int colorIndex;
    
    // Изображение
    HyperspectralImage hyperspectralImage;
    
    // Режимы
    bool isRGBMode = false;
    int currentRedChannel = 0;
    int currentGreenChannel = 0;
    int currentBlueChannel = 0;
    
    // Статусная информация
    QLabel* pixelInfoLabel;
    QLabel* coordinatesLabel;

    // Спектральная информация
    QVector<SpectralBand> spectralBands;
    bool hasSpectralData = false;
    
    // Текущая точка для спектральной кривой
    int currentSpectralX = -1;
    int currentSpectralY = -1;
    
    // Калибровка (DLL)
    QLibrary* calibrationModule;
    ShowCalibrationDialogFunc showCalibrationDialogFunc;
};

#endif // MAIN_WINDOW_H
