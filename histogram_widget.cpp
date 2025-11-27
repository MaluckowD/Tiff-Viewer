#include "histogram_widget.h"
#include <QPainter>
#include <QFontMetrics>
#include <algorithm>

HistogramWidget::HistogramWidget(QWidget *parent) : QWidget(parent) {
    setMinimumSize(700, 350);
    channel = 0;
    displayMode = GRAYSCALE;
    rgbChannel = RED_CHANNEL;
    setMouseTracking(true);
}

void HistogramWidget::setHistogramData16bit(const std::vector<int>& hist, int channelIndex) {
    histogram16bit = hist;
    channel = channelIndex;
    is16bit = true;
    
    if (!isZoomed) {
        calculateInformativeRange(hist, zoomMinValue, zoomMaxValue);
        isZoomed = true;
    }
    
    update();
}

void HistogramWidget::setRGBHistogramData(const std::vector<int>& redHist, 
                        const std::vector<int>& greenHist, 
                        const std::vector<int>& blueHist,
                        int redCh, int greenCh, int blueCh) {
    redHistogram = redHist;
    greenHistogram = greenHist;
    blueHistogram = blueHist;
    redChannelIndex = redCh;
    greenChannelIndex = greenCh;
    blueChannelIndex = blueCh;
    displayMode = RGB;
    update();
}

void HistogramWidget::setDisplayMode(DisplayMode mode) {
    displayMode = mode;
    update();
}

void HistogramWidget::setRGBChannel(RGBChannel ch) {
    rgbChannel = ch;
    update();
}

void HistogramWidget::setChannel(int ch) {
    channel = ch;
    update();
}

void HistogramWidget::resetZoom() {
    isZoomed = false;
    zoomMinValue = 0;
    zoomMaxValue = 65535;
    zoomMinCount = 0;
    zoomMaxCount = 0;
    
    if (!histogram16bit.empty()) {
        calculateInformativeRange(histogram16bit, zoomMinValue, zoomMaxValue);
        isZoomed = true;
    }
    
    update();
}

void HistogramWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        const int leftMargin = 80;
        const int rightMargin = 50;
        const int topMargin = 70;
        const int bottomMargin = 90;
        const int plotWidth = width() - leftMargin - rightMargin;
        const int plotHeight = height() - topMargin - bottomMargin;
        
        QRect plotRect(leftMargin, topMargin, plotWidth, plotHeight);
        
        if (plotRect.contains(event->pos())) {
            isSelecting = true;
            selectionStart = event->pos();
            selectionEnd = event->pos();
        }
    }
}

void HistogramWidget::mouseMoveEvent(QMouseEvent *event) {
    if (isSelecting) {
        const int leftMargin = 80;
        const int rightMargin = 50;
        const int topMargin = 70;
        const int bottomMargin = 90;
        const int plotWidth = width() - leftMargin - rightMargin;
        const int plotHeight = height() - topMargin - bottomMargin;
        
        QRect plotRect(leftMargin, topMargin, plotWidth, plotHeight);
        
        int x = std::max(plotRect.left(), std::min(event->pos().x(), plotRect.right()));
        int y = std::max(plotRect.top(), std::min(event->pos().y(), plotRect.bottom()));
        
        selectionEnd = QPoint(x, y);
        update();
    }
}

void HistogramWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && isSelecting) {
        isSelecting = false;
        
        const int leftMargin = 80;
        const int rightMargin = 50;
        const int topMargin = 70;
        const int bottomMargin = 90;
        const int plotWidth = width() - leftMargin - rightMargin;
        const int plotHeight = height() - topMargin - bottomMargin;
        
        int x1 = std::min(selectionStart.x(), selectionEnd.x()) - leftMargin;
        int x2 = std::max(selectionStart.x(), selectionEnd.x()) - leftMargin;
        int y1 = std::min(selectionStart.y(), selectionEnd.y()) - topMargin;
        int y2 = std::max(selectionStart.y(), selectionEnd.y()) - topMargin;
        
        if (std::abs(x2 - x1) > 10 && std::abs(y2 - y1) > 10) {
            x1 = std::max(0, x1);
            x2 = std::min(plotWidth, x2);
            y1 = std::max(0, y1);
            y2 = std::min(plotHeight, y2);
            
            int currentMin = isZoomed ? zoomMinValue : 0;
            int currentMax = isZoomed ? zoomMaxValue : 65535;
            int valueRange = currentMax - currentMin;
            
            int newMinValue = currentMin + (x1 * valueRange) / plotWidth;
            int newMaxValue = currentMin + (x2 * valueRange) / plotWidth;
            
            if (newMaxValue > newMinValue) {
                zoomMinValue = newMinValue;
                zoomMaxValue = newMaxValue;
                
                const std::vector<int>* currentHist = nullptr;
                if (displayMode == GRAYSCALE) {
                    currentHist = &histogram16bit;
                } else {
                    switch (rgbChannel) {
                        case RED_CHANNEL: currentHist = &redHistogram; break;
                        case GREEN_CHANNEL: currentHist = &greenHistogram; break;
                        case BLUE_CHANNEL: currentHist = &blueHistogram; break;
                    }
                }
                
                if (currentHist && !currentHist->empty()) {
                    int maxInRange = 0;
                    for (int i = zoomMinValue; i <= zoomMaxValue && i < static_cast<int>(currentHist->size()); i++) {
                        maxInRange = std::max(maxInRange, (*currentHist)[i]);
                    }
                    
                    if (maxInRange > 0) {
                        int currentMaxCount = isZoomed && zoomMaxCount > 0 ? zoomMaxCount : maxInRange;
                        zoomMaxCount = currentMaxCount - (y1 * currentMaxCount) / plotHeight;
                        zoomMinCount = currentMaxCount - (y2 * currentMaxCount) / plotHeight;
                        
                        isZoomed = true;
                    }
                }
            }
        }
        
        update();
    }
}

void HistogramWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    const int leftMargin = 80;
    const int rightMargin = 50;
    const int topMargin = 70; 
    const int bottomMargin = 90;
    const int plotWidth = width() - leftMargin - rightMargin;
    const int plotHeight = height() - topMargin - bottomMargin;

    if (displayMode == GRAYSCALE) {
        drawGrayscaleHistogram(painter, leftMargin, rightMargin, topMargin, bottomMargin, plotWidth, plotHeight);
    } else {
        drawRGBHistogram(painter, leftMargin, rightMargin, topMargin, bottomMargin, plotWidth, plotHeight);
    }
    
    if (isSelecting) {
        drawSelectionRect(painter);
    }
}

void HistogramWidget::drawSelectionRect(QPainter& painter) {
    painter.setPen(QPen(Qt::blue, 2, Qt::DashLine));
    painter.setBrush(QBrush(QColor(0, 0, 255, 30)));
    
    QRect selectionRect(selectionStart, selectionEnd);
    painter.drawRect(selectionRect.normalized());
}

void HistogramWidget::drawGrayscaleHistogram(QPainter& painter, int leftMargin, int rightMargin, int topMargin, int bottomMargin, int plotWidth, int plotHeight) {
    if (histogram16bit.empty()) {
        painter.drawText(rect(), Qt::AlignCenter, QString::fromUtf8("Нет данных гистограммы"));
        return;
    }

    int minValue = isZoomed ? zoomMinValue : 0;
    int maxValue = isZoomed ? zoomMaxValue : 65535;
    
    if (minValue >= maxValue) {
        minValue = 0;
        maxValue = 65535;
    }
    
    int maxCount = 0;
    for (int i = minValue; i <= maxValue && i < static_cast<int>(histogram16bit.size()); i++) {
        maxCount = std::max(maxCount, histogram16bit[i]);
    }
    
    if (isZoomed && zoomMaxCount > 0) {
        maxCount = std::min(maxCount, zoomMaxCount);
    }
    
    if (maxCount == 0) {
        maxCount = 1;
    }

    drawAxes(painter, leftMargin, rightMargin, topMargin, bottomMargin, plotWidth, plotHeight, maxCount, minValue, maxValue);
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::darkBlue);

    int valueRange = maxValue - minValue;
    int binSize = std::max(1, valueRange / 256);
    int numBins = valueRange / binSize;
    if (numBins == 0) numBins = 1;
    
    for (int bin = 0; bin < numBins; bin++) {
        int binCount = 0;
        for (int i = 0; i < binSize; i++) {
            int index = minValue + bin * binSize + i;
            if (index >= 0 && index < static_cast<int>(histogram16bit.size())) {
                binCount += histogram16bit[index];
            }
        }
        
        if (binCount > 0 && binCount <= maxCount) {
            float heightRatio = static_cast<float>(binCount) / maxCount;
            int barHeight = static_cast<int>(heightRatio * (plotHeight - 5));
            barHeight = std::min(barHeight, plotHeight - 5);
            
            int x = leftMargin + bin * plotWidth / numBins;
            int y = topMargin + plotHeight - barHeight;
            int barWidth = std::max(1, plotWidth / numBins);
            
            painter.drawRect(x, y, barWidth, barHeight);
        }
    }

    QFont titleFont = painter.font();
    titleFont.setPixelSize(14);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(Qt::black);

    QString titleText = QString::fromUtf8("Канал %1 (16-бит данные)").arg(channel + 1);
    if (isZoomed) {
        titleText += QString::fromUtf8(" [Масштаб: %1-%2]").arg(minValue).arg(maxValue);
    }
    QString maxText = QString::fromUtf8("Макс: %1").arg(maxCount);

    QFontMetrics fm(titleFont);
    int titleWidth = fm.horizontalAdvance(titleText);
    int maxWidth = fm.horizontalAdvance(maxText);

    int rightEdge = width() - rightMargin;
    painter.drawText(rightEdge - titleWidth - 10, 30, titleText);
    painter.drawText(rightEdge - maxWidth - 10, 50, maxText);
}

void HistogramWidget::drawRGBHistogram(QPainter& painter, int leftMargin, int rightMargin, int topMargin, int bottomMargin, int plotWidth, int plotHeight) {
    if (redHistogram.empty() || greenHistogram.empty() || blueHistogram.empty()) {
        painter.drawText(rect(), Qt::AlignCenter, QString::fromUtf8("Нет данных RGB гистограммы"));
        return;
    }

    const std::vector<int>* currentHist = nullptr;
    QColor histColor;
    QString channelName;
    int channelIndex = 0;

    switch (rgbChannel) {
        case RED_CHANNEL:
            currentHist = &redHistogram;
            histColor = QColor(200, 50, 50);
            channelName = QString::fromUtf8("Красный");
            channelIndex = redChannelIndex;
            break;
        case GREEN_CHANNEL:
            currentHist = &greenHistogram;
            histColor = QColor(50, 200, 50);
            channelName = QString::fromUtf8("Зеленый");
            channelIndex = greenChannelIndex;
            break;
        case BLUE_CHANNEL:
            currentHist = &blueHistogram;
            histColor = QColor(50, 50, 200);
            channelName = QString::fromUtf8("Синий");
            channelIndex = blueChannelIndex;
            break;
    }

    int minValue = isZoomed ? zoomMinValue : 0;
    int maxValue = isZoomed ? zoomMaxValue : 65535;
    
    if (minValue >= maxValue) {
        minValue = 0;
        maxValue = 65535;
    }
    
    int maxCount = 0;
    for (int i = minValue; i <= maxValue && i < static_cast<int>(currentHist->size()); i++) {
        maxCount = std::max(maxCount, (*currentHist)[i]);
    }
    
    if (isZoomed && zoomMaxCount > 0) {
        maxCount = std::min(maxCount, zoomMaxCount);
    }
    
    if (maxCount == 0) {
        maxCount = 1;
    }

    drawAxes(painter, leftMargin, rightMargin, topMargin, bottomMargin, plotWidth, plotHeight, maxCount, minValue, maxValue);
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(histColor);

    int valueRange = maxValue - minValue;
    int binSize = std::max(1, valueRange / 256);
    int numBins = valueRange / binSize;
    if (numBins == 0) numBins = 1;
    
    for (int bin = 0; bin < numBins; bin++) {
        int binCount = 0;
        for (int i = 0; i < binSize; i++) {
            int index = minValue + bin * binSize + i;
            if (index >= 0 && index < static_cast<int>(currentHist->size())) {
                binCount += (*currentHist)[index];
            }
        }
        
        if (binCount > 0 && binCount <= maxCount) {
            float heightRatio = static_cast<float>(binCount) / maxCount;
            int barHeight = static_cast<int>(heightRatio * (plotHeight - 5));
            barHeight = std::min(barHeight, plotHeight - 5);
            
            int x = leftMargin + bin * plotWidth / numBins;
            int y = topMargin + plotHeight - barHeight;
            int barWidth = std::max(1, plotWidth / numBins);
            
            painter.drawRect(x, y, barWidth, barHeight);
        }
    }

    QFont titleFont = painter.font();
    titleFont.setPixelSize(14);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(Qt::black);

    QString titleText = QString::fromUtf8("RGB Режим - %1 канал (Канал %2)").arg(channelName).arg(channelIndex + 1);
    if (isZoomed) {
        titleText += QString::fromUtf8(" [Масштаб: %1-%2]").arg(minValue).arg(maxValue);
    }
    QString maxText = QString::fromUtf8("Макс: %1").arg(maxCount);

    QFontMetrics fm(titleFont);
    int titleWidth = fm.horizontalAdvance(titleText);
    int maxWidth = fm.horizontalAdvance(maxText);

    int rightEdge = width() - rightMargin;
    painter.drawText(rightEdge - titleWidth - 10, 30, titleText);
    painter.drawText(rightEdge - maxWidth - 10, 50, maxText);
}

void HistogramWidget::drawAxes(QPainter& painter, int leftMargin, int rightMargin, int topMargin, int bottomMargin, int plotWidth, int plotHeight, int maxCount, int minVal, int maxVal) {
    QFont axisFont = painter.font();
    axisFont.setPixelSize(11);
    painter.setFont(axisFont);
    painter.setPen(Qt::black);

    painter.drawLine(leftMargin, topMargin + plotHeight, leftMargin + plotWidth, topMargin + plotHeight); // X axis
    painter.drawLine(leftMargin, topMargin, leftMargin, topMargin + plotHeight); // Y axis

    QFont titleFont = painter.font();
    titleFont.setPixelSize(13);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    
    painter.save();
    painter.translate(leftMargin - 70, topMargin + plotHeight/2);
    painter.rotate(-90);
    painter.drawText(QRect(-100, 0, 200, 20), Qt::AlignCenter, QString::fromUtf8("Количество пикселей"));
    painter.restore();
    
    painter.drawText(leftMargin + plotWidth/2 - 100, topMargin + plotHeight + bottomMargin - 25, QString::fromUtf8("16-битные значения (0-65535)"));
    painter.setFont(axisFont);

    int valueRange = maxVal - minVal;
    int numXTicks = 8;
    for (int i = 0; i <= numXTicks; i++) {
        int tickValue = minVal + (valueRange * i) / numXTicks;
        int xPos = leftMargin + (plotWidth * i) / numXTicks;
        
        painter.setPen(Qt::black);
        painter.drawLine(xPos, topMargin + plotHeight, xPos, topMargin + plotHeight + 5);
        
        painter.save();
        painter.translate(xPos, topMargin + plotHeight + 30);
        painter.rotate(-45);
        painter.drawText(QRect(0, 0, 70, 20), Qt::AlignLeft, QString::number(tickValue));
        painter.restore();
    }

    int ySteps = 5;
    for (int i = 0; i <= ySteps; ++i) {
        int yPos = topMargin + plotHeight - (i * plotHeight / ySteps);
        float value = static_cast<float>(maxCount) * (i / static_cast<float>(ySteps));
        
        QString label;
        if (value >= 1000000) {
            label = QString::number(value/1000000, 'f', 1) + "M";
        } else if (value >= 1000) {
            label = QString::number(value/1000, 'f', 1) + "k";
        } else {
            label = QString::number(static_cast<int>(value));
        }

        painter.setPen(QColor(200, 200, 200));
        painter.drawLine(leftMargin, yPos, leftMargin + plotWidth, yPos);
        
        painter.setPen(Qt::black);
        painter.drawText(QRect(10, yPos - 10, leftMargin - 20, 20), 
                        Qt::AlignRight | Qt::AlignVCenter, label);
        
        painter.drawLine(leftMargin - 5, yPos, leftMargin, yPos);
    }
}

void HistogramWidget::calculateInformativeRange(const std::vector<int>& hist, int& outMin, int& outMax) {
    if (hist.empty()) {
        outMin = 0;
        outMax = 65535;
        return;
    }
    
    int firstNonZero = 0;
    int lastNonZero = hist.size() - 1;
    
    for (size_t i = 0; i < hist.size(); i++) {
        if (hist[i] > 0) {
            firstNonZero = i;
            break;
        }
    }
    
    for (int i = hist.size() - 1; i >= 0; i--) {
        if (hist[i] > 0) {
            lastNonZero = i;
            break;
        }
    }
    
    int range = lastNonZero - firstNonZero;
    int padding = std::max(100, range / 20);
    
    outMin = std::max(0, firstNonZero - padding);
    outMax = std::min(65535, lastNonZero + padding);
}
