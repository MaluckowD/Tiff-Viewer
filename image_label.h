#ifndef IMAGE_LABEL_H
#define IMAGE_LABEL_H

#include <QLabel>
#include <QMouseEvent>
#include <QPixmap>
#include <QRect>
#include <QPoint>
#include <QContextMenuEvent>

class ImageLabel : public QLabel {
    Q_OBJECT

public:
    ImageLabel(QWidget* parent = nullptr);

signals:
    void mousePosition(int x, int y);
    void spectralCurveRequested(int x, int y);

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    QPoint imageCoordinatesFromWidget(const QPoint& widgetPos);
    QPoint lastRightClickPos;
};

#endif
