#ifndef SPECTRAL_CURVE_DIALOG_H
#define SPECTRAL_CURVE_DIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QWidget>
#include <QPaintEvent>
#include <QPainter>
#include <QMouseEvent>
#include <QListWidget>
#include <vector>
#include "spectral_reader.h"
#include "hyperspectral_image.h"

struct SpectralPoint {
    double wavelength;
    uint16_t value16;
    uint8_t value8;
    int channelIndex;
    bool hasWavelength;
};

struct PinnedPoint {
    int x, y;
    QColor color;
    std::vector<SpectralPoint> spectralData;
    QString label;
};

class SpectralCurveWidget : public QWidget {
    Q_OBJECT
    
public:
    SpectralCurveWidget(QWidget* parent = nullptr);
    void setSpectralData(const std::vector<SpectralPoint>& data);
    void setCoordinates(int x, int y);
    void addPinnedPoint(int x, int y, const std::vector<SpectralPoint>& data, const QColor& color);
    void removePinnedPoint(int index);
    void clearPinnedPoints();
    const std::vector<PinnedPoint>& getPinnedPoints() const { return pinnedPoints; }
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    
private:
    void drawAxes(QPainter& painter, const QRect& plotRect);
    void drawCurve(QPainter& painter, const QRect& plotRect);
    void drawPinnedCurves(QPainter& painter, const QRect& plotRect);
    void drawCrosshair(QPainter& painter, const QRect& plotRect, const QPoint& mousePos);
    QString formatValue(double value, bool isWavelength);
    
    std::vector<SpectralPoint> spectralData;
    std::vector<PinnedPoint> pinnedPoints;
    int pixelX, pixelY;
    QPoint lastMousePos;
    bool showCrosshair;
    double minWavelength, maxWavelength;
    uint16_t minValue, maxValue;
};

class SpectralCurveDialog : public QDialog {
    Q_OBJECT
    
public:
    SpectralCurveDialog(const HyperspectralImage* image, 
                       const QVector<SpectralBand>& bands,
                       int x, int y, 
                       QWidget* parent = nullptr);
    
    void updateCurrentPoint(int x, int y);
    
private slots:
    void onAddPointClicked();
    void onRemovePointClicked();
    void onClearPointsClicked();
    void onLegendItemDoubleClicked(QListWidgetItem* item);
    
private:
    void setupUI();
    void prepareSpectralData();
    void updateLegend();
    QColor getNextColor();
    
    const HyperspectralImage* hyperspectralImage;
    QVector<SpectralBand> spectralBands;
    int pixelX, pixelY;
    
    SpectralCurveWidget* curveWidget;
    QLabel* infoLabel;
    QListWidget* legendWidget;
    QPushButton* addPointButton;
    QPushButton* removePointButton;
    QPushButton* clearPointsButton;
    
    std::vector<SpectralPoint> spectralPoints;
    int colorIndex;
    bool addPointMode;
};

#endif
