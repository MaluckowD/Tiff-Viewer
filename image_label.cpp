#include "image_label.h"
#include <algorithm>
#include <QMenu>
#include <QAction>
#include <QStyle>

ImageLabel::ImageLabel(QWidget* parent) : QLabel(parent) {
    setMouseTracking(true);
    setAlignment(Qt::AlignCenter);
    setContextMenuPolicy(Qt::DefaultContextMenu);
}

void ImageLabel::mouseMoveEvent(QMouseEvent* event) {
    QPixmap currentPixmap = pixmap(Qt::ReturnByValue);
    if (!currentPixmap.isNull()) {
        QPoint imagePos = imageCoordinatesFromWidget(event->pos());
        if (imagePos.x() >= 0 && imagePos.y() >= 0) {
            emit mousePosition(imagePos.x(), imagePos.y());
        }
    }
    
    QLabel::mouseMoveEvent(event);
}

void ImageLabel::contextMenuEvent(QContextMenuEvent* event) {
    QPixmap currentPixmap = pixmap(Qt::ReturnByValue);
    if (currentPixmap.isNull()) {
        return;
    }
    
    QPoint imagePos = imageCoordinatesFromWidget(event->pos());
    if (imagePos.x() < 0 || imagePos.y() < 0) {
        return;
    }
    
    lastRightClickPos = imagePos;
    
    QMenu contextMenu(this);
    QAction* spectralCurveAction = contextMenu.addAction("Спектральная характеристика точки");
    spectralCurveAction->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    
    QAction* selectedAction = contextMenu.exec(event->globalPos());
    
    if (selectedAction == spectralCurveAction) {
        emit spectralCurveRequested(lastRightClickPos.x(), lastRightClickPos.y());
    }
}

QPoint ImageLabel::imageCoordinatesFromWidget(const QPoint& widgetPos) {
    QPixmap currentPixmap = pixmap(Qt::ReturnByValue);
    if (currentPixmap.isNull()) {
        return QPoint(-1, -1);
    }
    

    QRect pixmapRect = currentPixmap.rect();
    QRect widgetRect = rect();
    
    double scaleX = static_cast<double>(pixmapRect.width()) / widgetRect.width();
    double scaleY = static_cast<double>(pixmapRect.height()) / widgetRect.height();
    double scale = std::max(scaleX, scaleY);
    
    int scaledWidth = static_cast<int>(pixmapRect.width() / scale);
    int scaledHeight = static_cast<int>(pixmapRect.height() / scale);
    
    int offsetX = (widgetRect.width() - scaledWidth) / 2;
    int offsetY = (widgetRect.height() - scaledHeight) / 2;
    
    QPoint localPos = widgetPos - QPoint(offsetX, offsetY);
    
    if (localPos.x() >= 0 && localPos.x() < scaledWidth &&
        localPos.y() >= 0 && localPos.y() < scaledHeight) {
        
        int imageX = static_cast<int>(localPos.x() * scale);
        int imageY = static_cast<int>(localPos.y() * scale);
        
        imageX = std::max(0, std::min(imageX, pixmapRect.width() - 1));
        imageY = std::max(0, std::min(imageY, pixmapRect.height() - 1));
        
        return QPoint(imageX, imageY);
    }
    
    return QPoint(-1, -1);
}
