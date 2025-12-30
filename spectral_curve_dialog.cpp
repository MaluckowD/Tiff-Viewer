#include "spectral_curve_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QTextStream>
#include <QClipboard>
#include <QApplication>
#include <QMessageBox>
#include <QPainter>
#include <QFontMetrics>
#include <algorithm>
#include <cmath>

SpectralCurveWidget::SpectralCurveWidget(QWidget* parent) 
    : QWidget(parent), pixelX(0), pixelY(0), showCrosshair(false),
      minWavelength(0), maxWavelength(0), minValue(0), maxValue(0) {
    setMinimumSize(800, 500);
    setMouseTracking(true);
    setStyleSheet("background-color: white;");
}

void SpectralCurveWidget::setSpectralData(const std::vector<SpectralPoint>& data) {
    spectralData = data;
    
    if (!data.empty() || !pinnedPoints.empty()) {
        // Инициализируем диапазоны
        bool initialized = false;
        
        // Учитываем текущую точку
        if (!data.empty()) {
            minWavelength = maxWavelength = data[0].wavelength;
            minValue = maxValue = data[0].value16;
            initialized = true;
            
            for (const auto& point : data) {
                minWavelength = std::min(minWavelength, point.wavelength);
                maxWavelength = std::max(maxWavelength, point.wavelength);
                minValue = std::min(minValue, point.value16);
                maxValue = std::max(maxValue, point.value16);
            }
        }
        
        // Учитываем все закрепленные точки
        for (const auto& pinnedPoint : pinnedPoints) {
            for (const auto& point : pinnedPoint.spectralData) {
                if (!initialized) {
                    minWavelength = maxWavelength = point.wavelength;
                    minValue = maxValue = point.value16;
                    initialized = true;
                } else {
                    minWavelength = std::min(minWavelength, point.wavelength);
                    maxWavelength = std::max(maxWavelength, point.wavelength);
                    minValue = std::min(minValue, point.value16);
                    maxValue = std::max(maxValue, point.value16);
                }
            }
        }
        
        // Добавляем отступы к диапазонам
        if (initialized) {
            double wavelengthRange = maxWavelength - minWavelength;
            if (wavelengthRange > 0) {
                minWavelength -= wavelengthRange * 0.05;
                maxWavelength += wavelengthRange * 0.05;
            }
            
            uint16_t valueRange = maxValue - minValue;
            if (valueRange > 0) {
                minValue = std::max(0, static_cast<int>(minValue - valueRange * 0.05));
                maxValue = std::min(65535, static_cast<int>(maxValue + valueRange * 0.05));
            }
        }
    }
    
    update();
}

void SpectralCurveWidget::setCoordinates(int x, int y) {
    pixelX = x;
    pixelY = y;
}

void SpectralCurveWidget::addPinnedPoint(int x, int y, const std::vector<SpectralPoint>& data, const QColor& color) {
    PinnedPoint point;
    point.x = x;
    point.y = y;
    point.spectralData = data;
    point.color = color;
    point.label = QString("(%1, %2)").arg(x).arg(y);
    pinnedPoints.push_back(point);
    
    // Пересчитываем диапазоны с учетом новой точки
    setSpectralData(spectralData);
}

void SpectralCurveWidget::removePinnedPoint(int index) {
    if (index >= 0 && index < static_cast<int>(pinnedPoints.size())) {
        pinnedPoints.erase(pinnedPoints.begin() + index);
        setSpectralData(spectralData);
    }
}

void SpectralCurveWidget::clearPinnedPoints() {
    pinnedPoints.clear();
    setSpectralData(spectralData);
}

void SpectralCurveWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    const int margin = 80;
    const int topMargin = 60;
    const int bottomMargin = 80;
    QRect plotRect(margin, topMargin, width() - 2 * margin, height() - topMargin - bottomMargin);
    
    drawAxes(painter, plotRect);
    
    if (!pinnedPoints.empty()) {
        drawPinnedCurves(painter, plotRect);
    }
    
    if (!spectralData.empty()) {
        drawCurve(painter, plotRect);
    }
    
    if (showCrosshair) {
        drawCrosshair(painter, plotRect, lastMousePos);
    }
}

void SpectralCurveWidget::drawAxes(QPainter& painter, const QRect& plotRect) {
    painter.setPen(QPen(Qt::black, 2));
    
    painter.drawLine(plotRect.bottomLeft(), plotRect.bottomRight()); // X axis
    painter.drawLine(plotRect.bottomLeft(), plotRect.topLeft());     // Y axis
    
    // Настройка шрифтов
    QFont axisFont = painter.font();
    axisFont.setPixelSize(11);
    painter.setFont(axisFont);
    
    QFont titleFont = axisFont;
    titleFont.setPixelSize(13);
    titleFont.setBold(true);
    
    painter.setFont(titleFont);
    
    QString xAxisLabel;
    if (!spectralData.empty() && spectralData[0].hasWavelength) {
        xAxisLabel = QString::fromUtf8("Длина волны (нм)");
    } else {
        xAxisLabel = QString::fromUtf8("Номер канала");
    }
    painter.drawText(plotRect.center().x() - 80, height() - 20, xAxisLabel);
    
    painter.save();
    painter.translate(20, plotRect.center().y());
    painter.rotate(-90);
    painter.drawText(-80, 0, QString::fromUtf8("Яркость (16-бит)"));
    painter.restore();
    
    painter.setFont(axisFont);
    
    if (spectralData.empty()) return;
    
    const int xTicks = 8;
    for (int i = 0; i <= xTicks; i++) {
        double value = minWavelength + (maxWavelength - minWavelength) * i / xTicks;
        int x = plotRect.left() + plotRect.width() * i / xTicks;
        
        painter.setPen(QPen(Qt::lightGray, 1));
        painter.drawLine(x, plotRect.top(), x, plotRect.bottom());
        
        painter.setPen(QPen(Qt::black, 1));
        painter.drawLine(x, plotRect.bottom(), x, plotRect.bottom() + 5);
        
        QString label = formatValue(value, spectralData[0].hasWavelength);
        QFontMetrics fm(axisFont);
        int labelWidth = fm.horizontalAdvance(label);
        painter.drawText(x - labelWidth/2, plotRect.bottom() + 20, label);
    }
    
    const int yTicks = 6;
    for (int i = 0; i <= yTicks; i++) {
        uint16_t value = minValue + (maxValue - minValue) * i / yTicks;
        int y = plotRect.bottom() - plotRect.height() * i / yTicks;
        
        painter.setPen(QPen(Qt::lightGray, 1));
        painter.drawLine(plotRect.left(), y, plotRect.right(), y);
        
        painter.setPen(QPen(Qt::black, 1));
        painter.drawLine(plotRect.left() - 5, y, plotRect.left(), y);
        
        QString label = QString::number(value);
        QFontMetrics fm(axisFont);
        int labelWidth = fm.horizontalAdvance(label);
        painter.drawText(plotRect.left() - labelWidth - 10, y + 5, label);
    }
}

void SpectralCurveWidget::drawCurve(QPainter& painter, const QRect& plotRect) {
    if (spectralData.size() < 2) return;
    
    painter.setPen(QPen(Qt::red, 2));
    
    QVector<QPointF> points;
    for (const auto& point : spectralData) {
        double x = plotRect.left() + plotRect.width() * 
                  (point.wavelength - minWavelength) / (maxWavelength - minWavelength);
        double y = plotRect.bottom() - plotRect.height() * 
                  (point.value16 - minValue) / static_cast<double>(maxValue - minValue);
        points.append(QPointF(x, y));
    }
    
    for (int i = 0; i < points.size() - 1; i++) {
        painter.drawLine(points[i], points[i + 1]);
    }
    
    painter.setPen(QPen(Qt::darkRed, 1));
    painter.setBrush(QBrush(Qt::red));
    for (const QPointF& point : points) {
        painter.drawEllipse(point, 3, 3);
    }
}

void SpectralCurveWidget::drawPinnedCurves(QPainter& painter, const QRect& plotRect) {
    for (const auto& pinnedPoint : pinnedPoints) {
        if (pinnedPoint.spectralData.size() < 2) continue;
        
        painter.setPen(QPen(pinnedPoint.color, 2));
        
        QVector<QPointF> points;
        for (const auto& point : pinnedPoint.spectralData) {
            double x = plotRect.left() + plotRect.width() * 
                      (point.wavelength - minWavelength) / (maxWavelength - minWavelength);
            double y = plotRect.bottom() - plotRect.height() * 
                      (point.value16 - minValue) / static_cast<double>(maxValue - minValue);
            points.append(QPointF(x, y));
        }
        
        for (int i = 0; i < points.size() - 1; i++) {
            painter.drawLine(points[i], points[i + 1]);
        }
        
        painter.setPen(QPen(pinnedPoint.color.darker(), 1));
        painter.setBrush(QBrush(pinnedPoint.color));
        for (const QPointF& point : points) {
            painter.drawEllipse(point, 3, 3);
        }
    }
}

void SpectralCurveWidget::drawCrosshair(QPainter& painter, const QRect& plotRect, const QPoint& mousePos) {
    if (!plotRect.contains(mousePos)) return;
    
    painter.setPen(QPen(Qt::gray, 1, Qt::DashLine));
    
    painter.drawLine(mousePos.x(), plotRect.top(), mousePos.x(), plotRect.bottom());
    
    painter.drawLine(plotRect.left(), mousePos.y(), plotRect.right(), mousePos.y());
    
    double wavelengthAtCursor = minWavelength + 
        (maxWavelength - minWavelength) * (mousePos.x() - plotRect.left()) / plotRect.width();
    
    uint16_t valueAtCursor = minValue + 
        (maxValue - minValue) * (plotRect.bottom() - mousePos.y()) / plotRect.height();
    
    QString info;
    if (!spectralData.empty() && spectralData[0].hasWavelength) {
        info = QString::fromUtf8("λ: %1 нм, Яркость: %2")
               .arg(wavelengthAtCursor, 0, 'f', 1)
               .arg(valueAtCursor);
    } else {
        info = QString::fromUtf8("Канал: %1, Яркость: %2")
               .arg(wavelengthAtCursor, 0, 'f', 0)
               .arg(valueAtCursor);
    }
    
    QFontMetrics fm(painter.font());
    QRect textRect = fm.boundingRect(info);
    textRect.adjust(-5, -3, 5, 3);
    
    QPoint textPos = mousePos + QPoint(10, -10);
    if (textPos.x() + textRect.width() > plotRect.right()) {
        textPos.setX(mousePos.x() - textRect.width() - 10);
    }
    if (textPos.y() - textRect.height() < plotRect.top()) {
        textPos.setY(mousePos.y() + textRect.height() + 10);
    }
    
    textRect.moveTopLeft(textPos);
    
    painter.setPen(QPen(Qt::black, 1));
    painter.setBrush(QBrush(QColor(255, 255, 200, 200)));
    painter.drawRect(textRect);
    painter.drawText(textRect, Qt::AlignCenter, info);
}

void SpectralCurveWidget::mouseMoveEvent(QMouseEvent* event) {
    lastMousePos = event->pos();
    showCrosshair = true;
    update();
}

void SpectralCurveWidget::leaveEvent(QEvent* event) {
    showCrosshair = false;
    update();
}

QString SpectralCurveWidget::formatValue(double value, bool isWavelength) {
    if (isWavelength) {
        return QString::number(value, 'f', 0);
    } else {
        return QString::number(static_cast<int>(value));
    }
}

SpectralCurveDialog::SpectralCurveDialog(const HyperspectralImage* image, 
                                       const QVector<SpectralBand>& bands,
                                       int x, int y, 
                                       QWidget* parent)
    : QDialog(parent), hyperspectralImage(image), spectralBands(bands), 
      pixelX(x), pixelY(y), colorIndex(0), addPointMode(false) {
    
    setWindowTitle(QString::fromUtf8("Спектральная характеристика точки (%1, %2)").arg(x).arg(y));
    setMinimumSize(1100, 650);
    resize(1300, 750);
    
    setupUI();
    prepareSpectralData();
}

void SpectralCurveDialog::setupUI() {
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    
    QVBoxLayout* leftLayout = new QVBoxLayout();
    
    infoLabel = new QLabel();
    infoLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 8px; border: 1px solid #ccc; }");
    leftLayout->addWidget(infoLabel);
    
    curveWidget = new SpectralCurveWidget();
    curveWidget->setCoordinates(pixelX, pixelY);
    leftLayout->addWidget(curveWidget);
    
    // Кнопки
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    QPushButton* closeButton = new QPushButton(QString::fromUtf8("Закрыть"));
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    
    leftLayout->addLayout(buttonLayout);
    
    mainLayout->addLayout(leftLayout, 3);
    
    QVBoxLayout* rightLayout = new QVBoxLayout();
    
    QLabel* legendLabel = new QLabel(QString::fromUtf8("Легенда:"));
    legendLabel->setStyleSheet("font-weight: bold; font-size: 12pt;");
    rightLayout->addWidget(legendLabel);
    
    legendWidget = new QListWidget();
    legendWidget->setMaximumWidth(250);
    legendWidget->setStyleSheet(
        "QListWidget { border: 1px solid #ccc; background-color: white; }"
        "QListWidget::item { padding: 5px; }"
        "QListWidget::item:selected { background-color: #e0e0e0; }"
    );
    connect(legendWidget, &QListWidget::itemDoubleClicked, this, &SpectralCurveDialog::onLegendItemDoubleClicked);
    rightLayout->addWidget(legendWidget);
    
    // Кнопки управления точками
    addPointButton = new QPushButton(QString::fromUtf8("➕ Добавить точку"));
    addPointButton->setStyleSheet("QPushButton { padding: 8px; font-size: 10pt; }");
    connect(addPointButton, &QPushButton::clicked, this, &SpectralCurveDialog::onAddPointClicked);
    rightLayout->addWidget(addPointButton);
    
    removePointButton = new QPushButton(QString::fromUtf8("➖ Удалить выбранную"));
    removePointButton->setStyleSheet("QPushButton { padding: 8px; font-size: 10pt; }");
    removePointButton->setEnabled(false);
    connect(removePointButton, &QPushButton::clicked, this, &SpectralCurveDialog::onRemovePointClicked);
    rightLayout->addWidget(removePointButton);
    
    clearPointsButton = new QPushButton(QString::fromUtf8("🗑 Очистить все"));
    clearPointsButton->setStyleSheet("QPushButton { padding: 8px; font-size: 10pt; }");
    connect(clearPointsButton, &QPushButton::clicked, this, &SpectralCurveDialog::onClearPointsClicked);
    rightLayout->addWidget(clearPointsButton);
    
    rightLayout->addStretch();
    
    mainLayout->addLayout(rightLayout, 1);
    
    // Обновляем легенду при выборе элемента
    connect(legendWidget, &QListWidget::itemSelectionChanged, [this]() {
        removePointButton->setEnabled(legendWidget->currentRow() > 0);
    });
}

void SpectralCurveDialog::prepareSpectralData() {
    spectralPoints.clear();
    
    if (!hyperspectralImage) return;
    
    int numChannels = hyperspectralImage->getNumChannels();
    
    QMap<int, SpectralBand> spectralMap;
    for (const SpectralBand& band : spectralBands) {
        if (band.bandNumber > 0) {
            spectralMap[band.bandNumber] = band;
        }
    }
    
    bool hasSpectralData = !spectralBands.isEmpty();
    
    for (int i = 0; i < numChannels; i++) {
        SpectralPoint point;
        point.channelIndex = i;
        point.value16 = hyperspectralImage->getPixel16bit(i, pixelX, pixelY);
        point.value8 = hyperspectralImage->getPixel8bit(i, pixelX, pixelY);
        
        bool foundSpectralData = false;
        
        if (hasSpectralData) {
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
    
    curveWidget->setSpectralData(spectralPoints);
    
    QString infoText;
    if (hasSpectralData) {
        int channelsWithWavelength = 0;
        for (const auto& point : spectralPoints) {
            if (point.hasWavelength) channelsWithWavelength++;
        }
        
        infoText = QString::fromUtf8(
            "Координаты: (%1, %2) | Всего каналов: %3 | Каналов с длинами волн: %4")
            .arg(pixelX).arg(pixelY).arg(numChannels).arg(channelsWithWavelength);
    } else {
        infoText = QString::fromUtf8(
            "Координаты: (%1, %2) | Всего каналов: %3 | Спектральные данные отсутствуют")
            .arg(pixelX).arg(pixelY).arg(numChannels);
    }
    
    infoLabel->setText(infoText);
    updateLegend();
}

void SpectralCurveDialog::updateCurrentPoint(int x, int y) {
    pixelX = x;
    pixelY = y;
    setWindowTitle(QString::fromUtf8("Спектральная характеристика точки (%1, %2)").arg(x).arg(y));
    prepareSpectralData();
}

void SpectralCurveDialog::updateLegend() {
    legendWidget->clear();
    
    // Добавляем текущую точку (красная)
    QListWidgetItem* currentItem = new QListWidgetItem();
    currentItem->setText(QString::fromUtf8("🔴 Текущая: (%1, %2)").arg(pixelX).arg(pixelY));
    currentItem->setForeground(QColor(Qt::red));
    currentItem->setFlags(currentItem->flags() & ~Qt::ItemIsSelectable);
    legendWidget->addItem(currentItem);
    
    // Добавляем закрепленные точки
    const auto& pinnedPoints = curveWidget->getPinnedPoints();
    for (size_t i = 0; i < pinnedPoints.size(); i++) {
        const auto& point = pinnedPoints[i];
        QListWidgetItem* item = new QListWidgetItem();
        
        QString colorCircle;
        if (point.color == Qt::blue) colorCircle = "🔵";
        else if (point.color == Qt::green) colorCircle = "🟢";
        else if (point.color == QColor(255, 165, 0)) colorCircle = "🟠";
        else if (point.color == QColor(128, 0, 128)) colorCircle = "🟣";
        else if (point.color == QColor(0, 128, 128)) colorCircle = "🔷";
        else colorCircle = "⚫";
        
        item->setText(QString("%1 Точка: (%2, %3)").arg(colorCircle).arg(point.x).arg(point.y));
        item->setForeground(point.color);
        legendWidget->addItem(item);
    }
}

QColor SpectralCurveDialog::getNextColor() {
    QColor colors[] = {
        Qt::blue,
        Qt::green,
        QColor(255, 165, 0),  // Orange
        QColor(128, 0, 128),  // Purple
        QColor(0, 128, 128),  // Teal
        QColor(139, 69, 19),  // Brown
    };
    
    QColor color = colors[colorIndex % 6];
    colorIndex++;
    return color;
}

void SpectralCurveDialog::onAddPointClicked() {
    addPointMode = !addPointMode;
    
    if (addPointMode) {
        addPointButton->setText(QString::fromUtf8("✓ Кликните на изображение"));
        addPointButton->setStyleSheet("QPushButton { padding: 8px; font-size: 10pt; background-color: #90EE90; }");
        
        QColor color = getNextColor();
        curveWidget->addPinnedPoint(pixelX, pixelY, spectralPoints, color);
        updateLegend();
        
        addPointMode = false;
        addPointButton->setText(QString::fromUtf8("➕ Добавить точку"));
        addPointButton->setStyleSheet("QPushButton { padding: 8px; font-size: 10pt; }");
    }
}

void SpectralCurveDialog::onRemovePointClicked() {
    int currentRow = legendWidget->currentRow();
    if (currentRow > 0) {
        curveWidget->removePinnedPoint(currentRow - 1);
        updateLegend();
    }
}

void SpectralCurveDialog::onClearPointsClicked() {
    curveWidget->clearPinnedPoints();
    colorIndex = 0;
    updateLegend();
}

void SpectralCurveDialog::onLegendItemDoubleClicked(QListWidgetItem* item) {
    int row = legendWidget->row(item);
    if (row > 0) {
        curveWidget->removePinnedPoint(row - 1);
        updateLegend();
    }
}

#include "spectral_curve_dialog.moc"
