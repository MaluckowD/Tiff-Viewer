#ifndef HISTOGRAM_WIDGET_H
#define HISTOGRAM_WIDGET_H

#include <QWidget>
#include <QPaintEvent>
#include <QPainter>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QMouseEvent>
#include <vector>

class HistogramWidget : public QWidget {
    Q_OBJECT

public:
    enum DisplayMode {
        GRAYSCALE,
        RGB
    };

    enum RGBChannel {
        RED_CHANNEL,
        GREEN_CHANNEL,
        BLUE_CHANNEL
    };

    HistogramWidget(QWidget *parent = nullptr);

    void setHistogramData16bit(const std::vector<int>& hist, int channelIndex);
    void setRGBHistogramData(const std::vector<int>& redHist, 
                            const std::vector<int>& greenHist, 
                            const std::vector<int>& blueHist,
                            int redCh, int greenCh, int blueCh);
    void setDisplayMode(DisplayMode mode);
    void setRGBChannel(RGBChannel ch);
    void setChannel(int ch);
    void resetZoom();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void drawGrayscaleHistogram(QPainter& painter, int leftMargin, int rightMargin, 
                               int topMargin, int bottomMargin, int plotWidth, int plotHeight);
    void drawRGBHistogram(QPainter& painter, int leftMargin, int rightMargin, 
                         int topMargin, int bottomMargin, int plotWidth, int plotHeight);
    void drawAxes(QPainter& painter, int leftMargin, int rightMargin, 
                 int topMargin, int bottomMargin, int plotWidth, int plotHeight, int maxCount);
    void drawSelectionRect(QPainter& painter);

    std::vector<int> histogram16bit;
    std::vector<int> redHistogram;
    std::vector<int> greenHistogram;
    std::vector<int> blueHistogram;
    int channel;
    int redChannelIndex = 0;
    int greenChannelIndex = 0;
    int blueChannelIndex = 0;
    bool is16bit = false;
    DisplayMode displayMode;
    RGBChannel rgbChannel;
    
    bool isSelecting = false;
    QPoint selectionStart;
    QPoint selectionEnd;
    bool isZoomed = false;
    int zoomMinValue = 0;
    int zoomMaxValue = 65535;
    int zoomMinCount = 0;
    int zoomMaxCount = 0;
};

#endif
