#include "main_window.h"
#include "dialogs.h"
#include <QApplication>
#include <QMenuBar>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QFileInfo>
#include <QPushButton>
#include <QFrame>
#include <QStandardPaths>
#include <QDir>
#include <QIcon>
#include <QStyle>
#include <QSplitter>
#include "spectral_reader.h"
#include "spectral_info_dialog.h"
#include "spectral_curve_dialog.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Hyperspectral Image Viewer");
    resize(1600, 900);
    
    colorIndex = 0;
    
    createUI();
    createMenus();
    setupStatusBar();
}

void MainWindow::openFile() {
    QString filePath = QFileDialog::getOpenFileName(this, "Открыть TIFF файл", "", "TIFF Files (*.tif *.tiff)");
    if (filePath.isEmpty()) return;

    if (!hyperspectralImage.loadFromTiff(filePath)) {
        QMessageBox::critical(this, "Оибка", "Не удалось загрузить TIFF файл");
        return;
    }

    channelSelector->clear();
    histogramChannelSelector->clear();

    for (int i = 0; i < hyperspectralImage.getNumChannels(); i++) {
        QString channelName = QString("Канал %1").arg(i + 1);
        channelSelector->addItem(channelName);
    }

    // Заполняем список каналов для гистограммы
    histogramChannelSelector->clear();
    for (int i = 0; i < hyperspectralImage.getNumChannels(); i++) {
        QString channelName = QString("Канал %1").arg(i + 1);
        histogramChannelSelector->addItem(channelName);
    }
    histogramChannelSelector->setCurrentIndex(0);
    histogramChannelSelector->setEnabled(true);
    
    channelSelector->setEnabled(true);
    autoContrastButton->setEnabled(true);

    if (hyperspectralImage.getNumChannels() > 0) {
        // Устанавливаем начальные каналы для RGB
        if (hyperspectralImage.getNumChannels() > 55) currentRedChannel = 54;
        if (hyperspectralImage.getNumChannels() > 29) currentGreenChannel = 28;  
        if (hyperspectralImage.getNumChannels() > 14) currentBlueChannel = 13;
        
        channelSelector->setCurrentIndex(0);
        
        // Автоматически применяем контрастирование при загрузке
        applyAutoContrast();
        
        displayChannel(0);
        updateHistogram();
    }

    statusBar->showMessage(QString("Загружен %1 с %2 каналами, %3x%4 пикселей")
                        .arg(QFileInfo(filePath).fileName())
                        .arg(hyperspectralImage.getNumChannels())
                        .arg(hyperspectralImage.getWidth())
                        .arg(hyperspectralImage.getHeight()));

    // Автоматически загружаем спектральные данные
    loadSpectralData(filePath);
    
    // Очищаем спектральную кривую
    spectralCurveWidget->setSpectralData({});
    spectralCurveLabel->setText("Спектральная кривая: кликните на изображении");
    currentSpectralX = -1;
    currentSpectralY = -1;
}

void MainWindow::loadSpectralData(const QString& tiffFilePath) {
    QFileInfo tiffInfo(tiffFilePath);
    QString baseName = tiffInfo.completeBaseName();
    QString dirPath = tiffInfo.absolutePath();
    
    // Список возможных расширений для файлов спектральных данных
    QStringList spectralExtensions = {"spp", "xml", "hdr"};
    QString foundSpectralFile;
    
    // Ищем файл с тем же именем, но другим расширением
    for (const QString& ext : spectralExtensions) {
        QString candidateFile = dirPath + "/" + baseName + "." + ext;
        if (QFile::exists(candidateFile)) {
            foundSpectralFile = candidateFile;
            break;
        }
    }
    
    // Если не найден файл с точным именем, ищем любые файлы спектральных данных в директории
    if (foundSpectralFile.isEmpty()) {
        QDir dir(dirPath);
        for (const QString& ext : spectralExtensions) {
            QStringList filters;
            filters << "*." + ext;
            QStringList files = dir.entryList(filters, QDir::Files);
            if (!files.isEmpty()) {
                foundSpectralFile = dirPath + "/" + files.first();
                break;
            }
        }
    }
    
    if (!foundSpectralFile.isEmpty()) {
        // Найден файл спектральных данных - загружаем автоматически
        QVector<SpectralBand> loadedBands;
        bool success = false;
        
        
        if (foundSpectralFile.endsWith(".spp", Qt::CaseInsensitive) || 
            foundSpectralFile.endsWith(".xml", Qt::CaseInsensitive)) {
            success = SpectralReader::readSppFile(foundSpectralFile, loadedBands);
        } else if (foundSpectralFile.endsWith(".hdr", Qt::CaseInsensitive)) {
            success = SpectralReader::readHdrFile(foundSpectralFile, loadedBands);
        }
        
        if (success && !loadedBands.isEmpty()) {
            spectralBands = loadedBands;
            hasSpectralData = true;
            statusBar->showMessage(QString("Загружены спектральные данные из %1 (%2 каналов)")
                                 .arg(QFileInfo(foundSpectralFile).fileName())
                                 .arg(loadedBands.size()), 3000);
            return;
        }
    }
    
    // Если автоматически не найден или не удалось загрузить - предлагаем выбрать вручную
    QMessageBox::StandardButton reply = QMessageBox::question(this, 
        "Спектральные данные", 
        "Файл спектральных данных не найден автоматически.\n"
        "Хотите указать его местоположение вручную?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        openSpectralInfo();
    } else {
        hasSpectralData = false;
        spectralBands.clear();
        statusBar->showMessage("Изображение загружено без спектральных данных", 2000);
    }
}

void MainWindow::applyAutoContrast() {
    if (hyperspectralImage.getNumChannels() == 0) return;
    
    // Применяем автоконтрастирование ко всем каналам
    for (int i = 0; i < hyperspectralImage.getNumChannels(); i++) {
        hyperspectralImage.normalizeByPercentile(i, 2.0, 2.0);
    }
}

void MainWindow::autoContrast() {
    if (hyperspectralImage.getNumChannels() == 0) {
        QMessageBox::warning(this, "Предупреждение", "Сначала откройте TIFF файл");
        return;
    }
    
    applyAutoContrast();
    
    // Обновляем отображение
    if (isRGBMode) {
        displayRGBImage();
    } else {
        displayChannel(channelSelector->currentIndex());
    }
    updateHistogram();
    
    // Обновляем спектральную кривую если есть активная точка
    if (currentSpectralX >= 0 && currentSpectralY >= 0) {
        updateSpectralCurve();
    }
    
    statusBar->showMessage("Применено автоматическое контрастирование (обрезка 2%)", 2000);
}

void MainWindow::showSpectralCurve(int x, int y) {
    if (hyperspectralImage.getNumChannels() == 0) {
        return;
    }
    
    currentSpectralX = x;
    currentSpectralY = y;
    updateSpectralCurve();
}

void MainWindow::updateSpectralCurve() {
    if (currentSpectralX < 0 || currentSpectralY < 0 || hyperspectralImage.getNumChannels() == 0) {
        return;
    }
    
    std::vector<SpectralPoint> spectralPoints;
    
    // Создаем карту спектральных данных для быстрого поиска
    QMap<int, SpectralBand> spectralMap;
    for (const SpectralBand& band : spectralBands) {
        if (band.bandNumber > 0) {
            spectralMap[band.bandNumber] = band;
        }
    }
    
    bool hasSpectralDataLocal = !spectralBands.isEmpty();
    
    for (int i = 0; i < hyperspectralImage.getNumChannels(); i++) {
        SpectralPoint point;
        point.channelIndex = i;
        point.value16 = hyperspectralImage.getPixel16bit(i, currentSpectralX, currentSpectralY);
        point.value8 = hyperspectralImage.getPixel8bit(i, currentSpectralX, currentSpectralY);
        
        // Пытаемся найти спектральные данные для этого канала
        bool foundSpectralData = false;
        
        if (hasSpectralDataLocal) {
            // Вариант 1: прямое соответствие номера канала
            int channelNum = i + 1;
            if (spectralMap.contains(channelNum)) {
                point.wavelength = spectralMap[channelNum].wavelength;
                point.hasWavelength = (point.wavelength > 0);
                foundSpectralData = true;
            }
            // Вариант 2: по порядку в массиве
            else if (i < spectralBands.size() && spectralBands[i].wavelength > 0) {
                point.wavelength = spectralBands[i].wavelength;
                point.hasWavelength = true;
                foundSpectralData = true;
            }
        }
        
        if (!foundSpectralData) {
            // Используем номер канала как "длину волны"
            point.wavelength = i + 1;
            point.hasWavelength = false;
        }
        
        spectralPoints.push_back(point);
    }
    
    // Передаем данные виджету
    spectralCurveWidget->setSpectralData(spectralPoints);
    spectralCurveWidget->setCoordinates(currentSpectralX, currentSpectralY);
    
    // Обновляем заголовок
    spectralCurveLabel->setText(QString("Спектральная кривая точки (%1, %2)")
                               .arg(currentSpectralX).arg(currentSpectralY));
}

void MainWindow::displayChannel(int channelIndex) {
    if (channelIndex < 0) return;
    
    isRGBMode = false;
    histogramChannelSelector->setEnabled(false);
    
    QImage image = hyperspectralImage.getChannelImage(channelIndex);
    if (image.isNull()) return;

    imageLabel->setPixmap(QPixmap::fromImage(image));
    imageLabel->resize(image.size());
    
    auto [minVal, maxVal] = hyperspectralImage.getChannelMinMax16bit(channelIndex);
    statusBar->showMessage(QString("Канал %1: 16-бит диапазон %2-%3")
                          .arg(channelIndex + 1)
                          .arg(minVal)
                          .arg(maxVal));
    
    // Обновляем список гистограммы для одноканального режима
    histogramChannelSelector->clear();
    for (int i = 0; i < hyperspectralImage.getNumChannels(); i++) {
        QString channelName = QString("Канал %1").arg(i + 1);
        histogramChannelSelector->addItem(channelName);
    }
    histogramChannelSelector->setCurrentIndex(channelIndex);
    histogramChannelSelector->setEnabled(true);
    
    updateHistogram();
}

void MainWindow::updateHistogram() {
    if (hyperspectralImage.getNumChannels() == 0) return;
    
    if (isRGBMode) {
        // RGB режим гистограммы
        auto redHist = hyperspectralImage.calculateHistogram16bit(currentRedChannel);
        auto greenHist = hyperspectralImage.calculateHistogram16bit(currentGreenChannel);
        auto blueHist = hyperspectralImage.calculateHistogram16bit(currentBlueChannel);
        
        histogramWidget->setRGBHistogramData(redHist, greenHist, blueHist, 
                                           currentRedChannel, currentGreenChannel, currentBlueChannel);
    } else {
        // Одноканальный режим
        int selectedHistogramIndex = histogramChannelSelector->currentIndex();
        if (selectedHistogramIndex >= 0 && selectedHistogramIndex < hyperspectralImage.getNumChannels()) {
            auto histogram = hyperspectralImage.calculateHistogram16bit(selectedHistogramIndex);
            histogramWidget->setHistogramData16bit(histogram, selectedHistogramIndex);
            histogramWidget->setDisplayMode(HistogramWidget::GRAYSCALE);
        }
    }
}

void MainWindow::openRGBSettings() {
    if (hyperspectralImage.getNumChannels() == 0) {
        QMessageBox::warning(this, "Предупреждение", "Сначала откройте TIFF файл");
        return;
    }
    
    RGBSettingsDialog dialog(hyperspectralImage.getNumChannels(), 
                           currentRedChannel, currentGreenChannel, currentBlueChannel, this);
    if (dialog.exec() == QDialog::Accepted) {
        currentRedChannel = dialog.getRedChannel();
        currentGreenChannel = dialog.getGreenChannel();
        currentBlueChannel = dialog.getBlueChannel();
        displayRGBImage();
    }
}

void MainWindow::displayRGBImage() {
    QImage rgbImage = hyperspectralImage.getRGBImage(currentRedChannel, currentGreenChannel, currentBlueChannel);
    if (rgbImage.isNull()) return;
    
    isRGBMode = true;
    histogramChannelSelector->setEnabled(true);
    
    imageLabel->setPixmap(QPixmap::fromImage(rgbImage));
    imageLabel->resize(rgbImage.size());
    
    statusBar->showMessage(QString("RGB Синтез: R=Канал %1, G=Канал %2, B=Канал %3")
                          .arg(currentRedChannel + 1)
                          .arg(currentGreenChannel + 1)
                          .arg(currentBlueChannel + 1));
    
    // Обновляем список гистограммы для RGB режима
    histogramChannelSelector->clear();
    histogramChannelSelector->addItem(QString("Канал %1 (Красный)").arg(currentRedChannel + 1));
    histogramChannelSelector->addItem(QString("Канал %1 (Зеленый)").arg(currentGreenChannel + 1));
    histogramChannelSelector->addItem(QString("Канал %1 (Синий)").arg(currentBlueChannel + 1));
    histogramChannelSelector->setCurrentIndex(0);
    histogramChannelSelector->setEnabled(true);
    
    updateHistogram();
}

void MainWindow::openContrastDialog() {
    if (hyperspectralImage.getNumChannels() == 0) {
        QMessageBox::warning(this, "Предупреждение", "Сначала откройте TIFF файл");
        return;
    }

    ContrastDialog* dialog = nullptr;

    if (isRGBMode) {
        // RGB режим контрастирования
        dialog = new ContrastDialog(&hyperspectralImage, currentRedChannel, currentGreenChannel, currentBlueChannel, this);
    } else {
        // Одноканальный режим контрастирования
        int currentChannel = channelSelector->currentIndex();
        if (currentChannel >= 0) {
            dialog = new ContrastDialog(&hyperspectralImage, currentChannel, this);
        }
    }

    if (dialog) {
        connect(dialog, &ContrastDialog::contrastChanged, this, &MainWindow::onContrastChanged);
        dialog->exec();
        delete dialog;
    }
}

void MainWindow::onContrastChanged() {
    if (isRGBMode) {
        displayRGBImage();
    } else {
        displayChannel(channelSelector->currentIndex());
    }
    updateHistogram();
    
    // Обновляем спектральную кривую если есть активная точка
    if (currentSpectralX >= 0 && currentSpectralY >= 0) {
        updateSpectralCurve();
    }
}

void MainWindow::closeImage() {
    imageLabel->clear();
    imageLabel->setPixmap(QPixmap());
    imageLabel->setMinimumSize(1, 1);
    imageLabel->resize(1, 1);
    
    channelSelector->clear();
    channelSelector->setEnabled(false);
    histogramChannelSelector->clear();
    histogramChannelSelector->setEnabled(false);
    autoContrastButton->setEnabled(false);
    
    statusBar->clearMessage();
    pixelInfoLabel->clear();
    coordinatesLabel->clear();
    
    isRGBMode = false;
    hasSpectralData = false;
    spectralBands.clear();
    
    hyperspectralImage = HyperspectralImage();
    
    histogramWidget->setHistogramData16bit({}, -1);
    
    // Очищаем спектральную кривую
    spectralCurveWidget->setSpectralData({});
    spectralCurveLabel->setText("Спектральная кривая: нет данных");
    currentSpectralX = -1;
    currentSpectralY = -1;
    
    setWindowTitle("Hyperspectral Image Viewer");
}

void MainWindow::onMousePosition(int x, int y) {
    if (hyperspectralImage.getNumChannels() == 0) return;
    
    coordinatesLabel->setText(QString("X: %1, Y: %2").arg(x).arg(y));
    
    if (isRGBMode) {
        // Показываем значения для всех RGB каналов
        uint16_t r16 = hyperspectralImage.getPixel16bit(currentRedChannel, x, y);
        uint16_t g16 = hyperspectralImage.getPixel16bit(currentGreenChannel, x, y);
        uint16_t b16 = hyperspectralImage.getPixel16bit(currentBlueChannel, x, y);
        
        uint8_t r8 = hyperspectralImage.getPixel8bit(currentRedChannel, x, y);
        uint8_t g8 = hyperspectralImage.getPixel8bit(currentGreenChannel, x, y);
        uint8_t b8 = hyperspectralImage.getPixel8bit(currentBlueChannel, x, y);
        
        pixelInfoLabel->setText(QString("RGB: [%1,%2,%3] (8-бит) [%4,%5,%6] (16-бит)")
                              .arg(r8).arg(g8).arg(b8)
                              .arg(r16).arg(g16).arg(b16));
    } else {
        // Показываем значения для текущего канала
        int currentChannel = channelSelector->currentIndex();
        if (currentChannel >= 0) {
            uint16_t val16 = hyperspectralImage.getPixel16bit(currentChannel, x, y);
            uint8_t val8 = hyperspectralImage.getPixel8bit(currentChannel, x, y);
            
            pixelInfoLabel->setText(QString("Канал %1: %2 (8-бит), %3 (16-бит)")
                                  .arg(currentChannel + 1)
                                  .arg(val8)
                                  .arg(val16));
        }
    }
    
    // Обновляем спектральную кривую при движении мыши
    updateSpectralCurveForMousePosition(x, y);
}

void MainWindow::onHistogramChannelChanged() {
    if (isRGBMode) {
        int selectedIndex = histogramChannelSelector->currentIndex();
        HistogramWidget::RGBChannel rgbChannel;
        
        if (selectedIndex == 0) {
            rgbChannel = HistogramWidget::RED_CHANNEL;
        } else if (selectedIndex == 1) {
            rgbChannel = HistogramWidget::GREEN_CHANNEL;
        } else {
            rgbChannel = HistogramWidget::BLUE_CHANNEL;
        }
        
        histogramWidget->setRGBChannel(rgbChannel);
    } else {
        // В одноканальном режиме обновляем гистограмму выбранного канала
        updateHistogram();
    }
}

void MainWindow::createUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    
    QVBoxLayout* leftLayout = new QVBoxLayout();
    
    // Панель управления
    QHBoxLayout* controlLayout = new QHBoxLayout();
    QLabel* channelLabel = new QLabel("Канал:");
    channelLabel->setMinimumWidth(60);
    
    channelSelector = new QComboBox();
    channelSelector->setEnabled(false);
    channelSelector->setMinimumWidth(120);
    channelSelector->setMinimumHeight(30);
    connect(channelSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::displayChannel);

    QPushButton* contrastButton = new QPushButton("Контрастирование");
    contrastButton->setMinimumHeight(30);
    contrastButton->setMinimumWidth(150);
    connect(contrastButton, &QPushButton::clicked, this, &MainWindow::openContrastDialog);

    autoContrastButton = new QPushButton();
    autoContrastButton->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    autoContrastButton->setToolTip(QString::fromUtf8("Автоматическое контрастирование (обрезка гистограммы 2%)"));
    autoContrastButton->setMinimumSize(35, 30);
    autoContrastButton->setMaximumSize(35, 30);
    autoContrastButton->setEnabled(false);
    connect(autoContrastButton, &QPushButton::clicked, this, &MainWindow::autoContrast);

    controlLayout->addWidget(channelLabel);
    controlLayout->addWidget(channelSelector);
    controlLayout->addWidget(contrastButton);
    controlLayout->addWidget(autoContrastButton);
    controlLayout->addStretch();
    
    leftLayout->addLayout(controlLayout);

    scrollArea = new QScrollArea();
    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidgetResizable(false);
    scrollArea->setAlignment(Qt::AlignCenter);

    imageLabel = new ImageLabel();
    imageLabel->setBackgroundRole(QPalette::Base);
    imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    imageLabel->setMinimumSize(1, 1);
    
    connect(imageLabel, &ImageLabel::mousePosition, this, &MainWindow::onMousePosition);
    connect(imageLabel, &ImageLabel::spectralCurveRequested, this, &MainWindow::showSpectralCurve);

    scrollArea->setWidget(imageLabel);
    leftLayout->addWidget(scrollArea);
    
    // Правая панель с спектральной кривой и гистограммой
    QVBoxLayout* rightLayout = new QVBoxLayout();
    
    rightSplitter = new QSplitter(Qt::Vertical);
    rightSplitter->setMinimumWidth(750);
    
    // Виджет спектральной кривой
    QWidget* spectralWidget = new QWidget();
    spectralWidget->setStyleSheet("QWidget { background-color: white; }");
    QVBoxLayout* spectralLayout = new QVBoxLayout(spectralWidget);
    spectralLayout->setContentsMargins(10, 10, 10, 10);
    
    spectralLayout->addStretch(1);
    
    spectralCurveLabel = new QLabel(QString::fromUtf8("Спектральная кривая: кликните на изображении"));
    spectralCurveLabel->setStyleSheet("QLabel { font-weight: bold; padding: 5px; background-color: white; }");
    spectralLayout->addWidget(spectralCurveLabel);
    
    spectralCurveWidget = new SpectralCurveWidget();
    spectralCurveWidget->setMinimumHeight(300);
    spectralCurveWidget->setMaximumHeight(400);
    spectralLayout->addWidget(spectralCurveWidget);
    
    QHBoxLayout* pointControlLayout = new QHBoxLayout();
    addPointButton = new QPushButton();
    addPointButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    addPointButton->setToolTip(QString::fromUtf8("Закрепить текущую точку на графике"));
    addPointButton->setMinimumSize(32, 32);
    addPointButton->setMaximumSize(32, 32);
    connect(addPointButton, &QPushButton::clicked, this, &MainWindow::onAddPointClicked);
    
    removePointButton = new QPushButton();
    removePointButton->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    removePointButton->setToolTip(QString::fromUtf8("Удалить выбранную точку из графика"));
    removePointButton->setMinimumSize(32, 32);
    removePointButton->setMaximumSize(32, 32);
    connect(removePointButton, &QPushButton::clicked, this, &MainWindow::onRemovePointClicked);
    
    clearPointsButton = new QPushButton();
    clearPointsButton->setIcon(style()->standardIcon(QStyle::SP_DialogDiscardButton));
    clearPointsButton->setToolTip(QString::fromUtf8("Удалить все закрепленные точки"));
    clearPointsButton->setMinimumSize(32, 32);
    clearPointsButton->setMaximumSize(32, 32);
    connect(clearPointsButton, &QPushButton::clicked, this, &MainWindow::onClearPointsClicked);
    
    pointControlLayout->addWidget(addPointButton);
    pointControlLayout->addWidget(removePointButton);
    pointControlLayout->addWidget(clearPointsButton);
    pointControlLayout->addStretch();
    spectralLayout->addLayout(pointControlLayout);
    
    QLabel* legendLabel = new QLabel(QString::fromUtf8("Закрепленные точки:"));
    legendLabel->setStyleSheet("QLabel { font-weight: bold; padding: 5px; background-color: white; }");
    spectralLayout->addWidget(legendLabel);
    
    legendWidget = new QListWidget();
    legendWidget->setMaximumHeight(150);
    legendWidget->setToolTip(QString::fromUtf8("Двойной клик для удаления точки"));
    legendWidget->setStyleSheet("QListWidget { background-color: white; }");
    connect(legendWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::onLegendItemDoubleClicked);
    spectralLayout->addWidget(legendWidget);
    
    rightSplitter->addWidget(spectralWidget);
    
    // Виджет гистограммы
    QWidget* histogramContainer = new QWidget();
    histogramContainer->setStyleSheet("QWidget { background-color: white; }");
    QVBoxLayout* histogramLayout = new QVBoxLayout(histogramContainer);
    histogramLayout->setContentsMargins(10, 10, 10, 10);
    
    QHBoxLayout* histControlLayout = new QHBoxLayout();
    
    QLabel* histChannelLabel = new QLabel(QString::fromUtf8("Канал гистограммы:"));
    histChannelLabel->setStyleSheet("QLabel { background-color: white; }");
    histogramChannelSelector = new QComboBox();
    histogramChannelSelector->setEnabled(false);
    histogramChannelSelector->setMinimumWidth(120);
    connect(histogramChannelSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onHistogramChannelChanged);
    
    QPushButton* resetZoomButton = new QPushButton();
    resetZoomButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    resetZoomButton->setToolTip(QString::fromUtf8("Сбросить масштабирование гистограммы"));
    resetZoomButton->setMinimumSize(32, 32);
    resetZoomButton->setMaximumSize(32, 32);
    connect(resetZoomButton, &QPushButton::clicked, [this]() {
        if (histogramWidget) {
            histogramWidget->resetZoom();
        }
    });
    
    histControlLayout->addWidget(histChannelLabel);
    histControlLayout->addWidget(histogramChannelSelector);
    histControlLayout->addWidget(resetZoomButton);
    histControlLayout->addStretch();
    histogramLayout->addLayout(histControlLayout);

    histogramWidget = new HistogramWidget();
    histogramWidget->setMinimumHeight(350);
    histogramWidget->setMaximumHeight(400);
    histogramLayout->addWidget(histogramWidget);
    
    rightSplitter->addWidget(histogramContainer);
    
    rightLayout->addWidget(rightSplitter);
    
    mainLayout->addLayout(leftLayout, 3);
    mainLayout->addLayout(rightLayout, 1);
}

void MainWindow::createMenus() {
    QMenu* fileMenu = menuBar()->addMenu("&Файл");
    QAction* openAction = new QAction("&Открыть", this);
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    fileMenu->addAction(openAction);
    
    QAction* closeAction = new QAction("&Закрыть", this);
    connect(closeAction, &QAction::triggered, this, &MainWindow::closeImage);
    fileMenu->addAction(closeAction);
    
    QMenu* viewMenu = menuBar()->addMenu("&Вид");
    QAction* rgbSettingsAction = new QAction("&Настройки RGB", this);
    connect(rgbSettingsAction, &QAction::triggered, this, &MainWindow::openRGBSettings);
    viewMenu->addAction(rgbSettingsAction);
    
    QAction* contrastAction = new QAction("&Контрастирование", this);
    connect(contrastAction, &QAction::triggered, this, &MainWindow::openContrastDialog);
    viewMenu->addAction(contrastAction);

    viewMenu->addSeparator();
    QAction* spectralInfoAction = new QAction("&Спектральная информация", this);
    connect(spectralInfoAction, &QAction::triggered, this, &MainWindow::openSpectralInfo);
    viewMenu->addAction(spectralInfoAction);
}

void MainWindow::setupStatusBar() {
    statusBar = new QStatusBar();
    setStatusBar(statusBar);
    
    coordinatesLabel = new QLabel();
    coordinatesLabel->setMinimumWidth(120);
    coordinatesLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    
    pixelInfoLabel = new QLabel();
    pixelInfoLabel->setMinimumWidth(400);
    pixelInfoLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    
    statusBar->addPermanentWidget(coordinatesLabel);
    statusBar->addPermanentWidget(pixelInfoLabel);
}

void MainWindow::openSpectralInfo() {
    QString filePath = QFileDialog::getOpenFileName(this, 
        "Открыть файл спектральной информации", 
        "", 
        "Spectral Files (*.spp *.xml *.hdr);;SPP Files (*.spp *.xml);;HDR Files (*.hdr);;All Files (*.*)");
    
    if (filePath.isEmpty()) return;
    
    QVector<SpectralBand> loadedBands;
    int totalRasterBands = 0;
    bool success = false;
    
    
    if (filePath.endsWith(".spp", Qt::CaseInsensitive) || filePath.endsWith(".xml", Qt::CaseInsensitive)) {
        success = SpectralReader::readSppFile(filePath, loadedBands);
    } else if (filePath.endsWith(".hdr", Qt::CaseInsensitive)) {
        success = SpectralReader::readHdrFile(filePath, loadedBands);
    } else {
        // Пытаемся определить формат автоматически
        success = SpectralReader::readSppFile(filePath, loadedBands);
        if (!success) {
            success = SpectralReader::readHdrFile(filePath, loadedBands);
        }
    }
    
    if (!success || loadedBands.isEmpty()) {
        QString errorMsg = QString(
            "Не удалось загрузить спектральную информацию из файла:\n%1\n\n"
            "Поддерживаемые форматы:\n"
            "• .spp/.xml - XML файл с детальной спектральной информацией\n"
            "• .hdr - ENVI header файл\n"
            "• .spp - простой текстовый файл с длинами волн").arg(filePath);
        
        QMessageBox::warning(this, "Ошибка чтения файла", errorMsg);
        return;
    }
    
    // Сохраняем загруженные данные
    spectralBands = loadedBands;
    hasSpectralData = true;
    
    // Показываем информацию о загруженных данных
    QString infoMsg = QString("Успешно загружено %1 спектральных каналов из файла").arg(loadedBands.size());
    statusBar->showMessage(infoMsg, 3000);
    
    // Обновляем спектральную кривую если есть активная точка
    if (currentSpectralX >= 0 && currentSpectralY >= 0) {
        updateSpectralCurve();
    }
    
    // Показываем диалог с данными из файла
    SpectralInfoDialog dialog(loadedBands, this);
    dialog.exec();
}

void MainWindow::updateSpectralCurveForMousePosition(int x, int y) {
    if (hyperspectralImage.getNumChannels() == 0) return;
    
    currentSpectralX = x;
    currentSpectralY = y;
    
    // Получаем спектральную характеристику для данной точки одним вызовом
    std::vector<uint16_t> spectrum = hyperspectralImage.getPixelSpectrum16bit(x, y);
    if (spectrum.empty()) return;
    
    std::vector<SpectralPoint> spectralPoints;
    spectralPoints.reserve(spectrum.size());
    
    QMap<int, SpectralBand> spectralMap;
    for (const SpectralBand& band : spectralBands) {
        if (band.bandNumber > 0) {
            spectralMap[band.bandNumber] = band;
        }
    }
    
    bool hasSpectralDataLocal = !spectralBands.isEmpty();
    
    for (int i = 0; i < static_cast<int>(spectrum.size()); i++) {
        SpectralPoint point;
        point.channelIndex = i;
        point.value16 = spectrum[i];
        point.value8 = hyperspectralImage.getPixel8bit(i, x, y);
        
        // Пытаемся найти спектральные данные для этого канала
        bool foundSpectralData = false;
        
        if (hasSpectralDataLocal) {
            int channelNum = i + 1;
            if (spectralMap.contains(channelNum)) {
                point.wavelength = spectralMap[channelNum].wavelength;
                point.hasWavelength = (point.wavelength > 0);
                foundSpectralData = true;
            }
            else if (i < spectralBands.size() && spectralBands[i].wavelength > 0) {
                point.wavelength = spectralBands[i].wavelength;
                point.hasWavelength = true;
                foundSpectralData = true;
            }
        }
        
        if (!foundSpectralData) {
            point.wavelength = i + 1;
            point.hasWavelength = false;
        }
        
        spectralPoints.push_back(point);
    }
    
    // Передаем данные виджету
    spectralCurveWidget->setSpectralData(spectralPoints);
    spectralCurveWidget->setCoordinates(x, y);
    
    // Обновляем заголовок
    spectralCurveLabel->setText(QString("Спектральная кривая точки (%1, %2)")
                               .arg(x).arg(y));
}

void MainWindow::onAddPointClicked() {
    if (currentSpectralX < 0 || currentSpectralY < 0 || hyperspectralImage.getNumChannels() == 0) {
        return;
    }
    
    // Получаем спектральные данные для текущей точки
    std::vector<uint16_t> spectrum = hyperspectralImage.getPixelSpectrum16bit(currentSpectralX, currentSpectralY);
    if (spectrum.empty()) return;
    
    std::vector<SpectralPoint> spectralPoints;
    spectralPoints.reserve(spectrum.size());
    
    QMap<int, SpectralBand> spectralMap;
    for (const SpectralBand& band : spectralBands) {
        if (band.bandNumber > 0) {
            spectralMap[band.bandNumber] = band;
        }
    }
    
    bool hasSpectralDataLocal = !spectralBands.isEmpty();
    
    for (int i = 0; i < static_cast<int>(spectrum.size()); i++) {
        SpectralPoint point;
        point.channelIndex = i;
        point.value16 = spectrum[i];
        point.value8 = hyperspectralImage.getPixel8bit(i, currentSpectralX, currentSpectralY);
        
        bool foundSpectralData = false;
        
        if (hasSpectralDataLocal) {
            int channelNum = i + 1;
            if (spectralMap.contains(channelNum)) {
                point.wavelength = spectralMap[channelNum].wavelength;
                point.hasWavelength = (point.wavelength > 0);
                foundSpectralData = true;
            }
            else if (i < spectralBands.size() && spectralBands[i].wavelength > 0) {
                point.wavelength = spectralBands[i].wavelength;
                point.hasWavelength = true;
                foundSpectralData = true;
            }
        }
        
        if (!foundSpectralData) {
            point.wavelength = i + 1;
            point.hasWavelength = false;
        }
        
        spectralPoints.push_back(point);
    }
    
    // Добавляем точку на график
    QColor color = getNextColor();
    spectralCurveWidget->addPinnedPoint(currentSpectralX, currentSpectralY, spectralPoints, color);
    
    // Обновляем легенду
    updateLegend();
}

void MainWindow::onRemovePointClicked() {
    int currentRow = legendWidget->currentRow();
    if (currentRow >= 0) {
        spectralCurveWidget->removePinnedPoint(currentRow);
        updateLegend();
    }
}

void MainWindow::onClearPointsClicked() {
    spectralCurveWidget->clearPinnedPoints();
    updateLegend();
}

void MainWindow::onLegendItemDoubleClicked(QListWidgetItem* item) {
    int row = legendWidget->row(item);
    if (row >= 0) {
        spectralCurveWidget->removePinnedPoint(row);
        updateLegend();
    }
}

void MainWindow::updateLegend() {
    legendWidget->clear();
    
    const std::vector<PinnedPoint>& pinnedPoints = spectralCurveWidget->getPinnedPoints();
    
    for (const PinnedPoint& point : pinnedPoints) {
        QListWidgetItem* item = new QListWidgetItem(point.label);
        
        // Создаем иконку с цветом точки
        QPixmap pixmap(16, 16);
        pixmap.fill(point.color);
        item->setIcon(QIcon(pixmap));
        
        legendWidget->addItem(item);
    }
}

QColor MainWindow::getNextColor() {
    static const QColor colors[] = {
        QColor(0, 0, 255),      // Синий
        QColor(0, 200, 0),      // Зеленый
        QColor(255, 140, 0),    // Оранжевый
        QColor(148, 0, 211),    // Фиолетовый
        QColor(0, 191, 255),    // Голубой
        QColor(255, 20, 147),   // Розовый
        QColor(50, 205, 50),    // Лаймовый
        QColor(255, 215, 0)     // Золотой
    };
    
    QColor color = colors[colorIndex % 8];
    colorIndex++;
    return color;
}
