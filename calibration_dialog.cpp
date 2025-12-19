#include "calibration_dialog.h"
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QXmlStreamReader>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <algorithm>

CalibrationDialog::CalibrationDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUI();
    setWindowTitle("Калибровка - Поиск снимков");
    resize(700, 600);
}

void CalibrationDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* directoryGroup = new QGroupBox("Директория поиска", this);
    QHBoxLayout* directoryLayout = new QHBoxLayout(directoryGroup);
    
    directoryEdit = new QLineEdit(this);
    directoryEdit->setPlaceholderText("Укажите путь к папке со снимками...");
    browseDirectoryButton = new QPushButton("Обзор...", this);
    connect(browseDirectoryButton, &QPushButton::clicked, this, &CalibrationDialog::onBrowseDirectory);
    
    directoryLayout->addWidget(directoryEdit);
    directoryLayout->addWidget(browseDirectoryButton);
    mainLayout->addWidget(directoryGroup);
    
    QGroupBox* timeGroup = new QGroupBox("Временной интервал съемки", this);
    QGridLayout* timeLayout = new QGridLayout(timeGroup);
    
    QLabel* startLabel = new QLabel("Начало:", this);
    startDateTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime().addDays(-30), this);
    startDateTimeEdit->setCalendarPopup(true);
    startDateTimeEdit->setDisplayFormat("dd.MM.yyyy HH:mm");
    
    QLabel* endLabel = new QLabel("Конец:", this);
    endDateTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    endDateTimeEdit->setCalendarPopup(true);
    endDateTimeEdit->setDisplayFormat("dd.MM.yyyy HH:mm");
    
    timeLayout->addWidget(startLabel, 0, 0);
    timeLayout->addWidget(startDateTimeEdit, 0, 1);
    timeLayout->addWidget(endLabel, 1, 0);
    timeLayout->addWidget(endDateTimeEdit, 1, 1);
    mainLayout->addWidget(timeGroup);
    
    QGroupBox* geoGroup = new QGroupBox("Географические координаты", this);
    QVBoxLayout* geoLayout = new QVBoxLayout(geoGroup);
    
    QHBoxLayout* kmlLayout = new QHBoxLayout();
    QLabel* kmlLabel = new QLabel("KML файл:", this);
    kmlFileEdit = new QLineEdit(this);
    kmlFileEdit->setPlaceholderText("Путь к KML файлу (опционально)...");
    browseKMLButton = new QPushButton("Обзор...", this);
    connect(browseKMLButton, &QPushButton::clicked, this, &CalibrationDialog::onBrowseKML);
    
    kmlLayout->addWidget(kmlLabel);
    kmlLayout->addWidget(kmlFileEdit);
    kmlLayout->addWidget(browseKMLButton);
    geoLayout->addLayout(kmlLayout);
    
    QHBoxLayout* countLayout = new QHBoxLayout();
    QLabel* countLabel = new QLabel("Количество координат:", this);
    coordinateCountSpinBox = new QSpinBox(this);
    coordinateCountSpinBox->setMinimum(1);
    coordinateCountSpinBox->setMaximum(2);
    coordinateCountSpinBox->setValue(1);
    coordinateCountSpinBox->setToolTip("1 - точка, 2 - прямоугольник");
    connect(coordinateCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CalibrationDialog::onCoordinateCountChanged);
    
    countLayout->addWidget(countLabel);
    countLayout->addWidget(coordinateCountSpinBox);
    countLayout->addStretch();
    geoLayout->addLayout(countLayout);
    
    coordinatesGroupBox = new QGroupBox("Координаты (широта, долгота)", this);
    QVBoxLayout* coordLayout = new QVBoxLayout(coordinatesGroupBox);
    
    updateCoordinateFields();
    
    geoLayout->addWidget(coordinatesGroupBox);
    mainLayout->addWidget(geoGroup);
    
    QLabel* resultsLabel = new QLabel("Результаты поиска:", this);
    mainLayout->addWidget(resultsLabel);
    
    resultsText = new QTextEdit(this);
    resultsText->setReadOnly(true);
    resultsText->setMinimumHeight(150);
    mainLayout->addWidget(resultsText);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    startButton = new QPushButton("Начать поиск", this);
    startButton->setDefault(true);
    connect(startButton, &QPushButton::clicked, this, &CalibrationDialog::onStartCalibration);
    
    closeButton = new QPushButton("Закрыть", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);
}

void CalibrationDialog::updateCoordinateFields() {
    QLayout* layout = coordinatesGroupBox->layout();
    if (layout) {
        QLayoutItem* item;
        while ((item = layout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete layout;
    }
    
    latitudeEdits.clear();
    longitudeEdits.clear();
    
    QGridLayout* coordGrid = new QGridLayout(coordinatesGroupBox);
    
    int count = coordinateCountSpinBox->value();
    QString pointLabel = (count == 1) ? "Точка" : "";
    
    for (int i = 0; i < count; ++i) {
        QString label = (count == 1) ? pointLabel : (i == 0 ? "Левый верхний угол" : "Правый нижний угол");
        
        QLabel* pointLabel = new QLabel(label + ":", this);
        QLabel* latLabel = new QLabel("Широта:", this);
        QLineEdit* latEdit = new QLineEdit(this);
        latEdit->setPlaceholderText("Например: 55.7558");
        
        QLabel* lonLabel = new QLabel("Долгота:", this);
        QLineEdit* lonEdit = new QLineEdit(this);
        lonEdit->setPlaceholderText("Например: 37.6173");
        
        int row = i * 2;
        if (count > 1) {
            coordGrid->addWidget(pointLabel, row, 0, 1, 4);
            row++;
        }
        coordGrid->addWidget(latLabel, row, 0);
        coordGrid->addWidget(latEdit, row, 1);
        coordGrid->addWidget(lonLabel, row, 2);
        coordGrid->addWidget(lonEdit, row, 3);
        
        latitudeEdits.append(latEdit);
        longitudeEdits.append(lonEdit);
    }
}

void CalibrationDialog::onBrowseDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, "Выберите директорию со снимками",
                                                    directoryEdit->text());
    if (!dir.isEmpty()) {
        directoryEdit->setText(dir);
    }
}

void CalibrationDialog::onBrowseKML() {
    QString fileName = QFileDialog::getOpenFileName(this, "Выберите KML файл",
                                                    kmlFileEdit->text(),
                                                    "KML файлы (*.kml);;Все файлы (*)");
    if (!fileName.isEmpty()) {
        kmlFileEdit->setText(fileName);
        parseKMLFile(fileName);
    }
}

void CalibrationDialog::onCoordinateCountChanged(int count) {
    updateCoordinateFields();
}


void CalibrationDialog::parseKMLFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть KML файл");
        return;
    }
    
    QXmlStreamReader xml(&file);
    QVector<QPair<double, double>> coordinates;
    
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == QString("coordinates")) {
            QString coordText = xml.readElementText();
            QStringList coordPairs = coordText.simplified().split(' ', Qt::SkipEmptyParts);
            
            for (const QString& pair : coordPairs) {
                QStringList values = pair.split(',');
                if (values.size() >= 2) {
                    double lon = values[0].toDouble();
                    double lat = values[1].toDouble();
                    coordinates.append(qMakePair(lat, lon));
                }
            }
        }
    }
    
    file.close();
    
    // Заполнить поля координат из KML
    if (!coordinates.isEmpty()) {
        if (coordinates.size() == 1 || coordinateCountSpinBox->value() == 1) {
            coordinateCountSpinBox->setValue(1);
            if (!latitudeEdits.isEmpty()) {
                latitudeEdits[0]->setText(QString::number(coordinates[0].first, 'f', 6));
                longitudeEdits[0]->setText(QString::number(coordinates[0].second, 'f', 6));
            }
        } else if (coordinates.size() >= 2) {
            coordinateCountSpinBox->setValue(2);
            if (latitudeEdits.size() >= 2) {
                latitudeEdits[0]->setText(QString::number(coordinates[0].first, 'f', 6));
                longitudeEdits[0]->setText(QString::number(coordinates[0].second, 'f', 6));
                latitudeEdits[1]->setText(QString::number(coordinates[1].first, 'f', 6));
                longitudeEdits[1]->setText(QString::number(coordinates[1].second, 'f', 6));
            }
        }
        
        resultsText->append("KML файл загружен. Найдено координат: " + QString::number(coordinates.size()));
    }
}


void CalibrationDialog::onStartCalibration() {
    if (directoryEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Укажите директорию для поиска");
        return;
    }
    
    params.searchDirectory = directoryEdit->text();
    params.startDateTime = startDateTimeEdit->dateTime();
    params.endDateTime = endDateTimeEdit->dateTime();
    params.kmlFilePath = kmlFileEdit->text();
    params.coordinateCount = coordinateCountSpinBox->value();
    params.coordinates.clear();
    
    for (int i = 0; i < params.coordinateCount; ++i) {
        bool latOk, lonOk;
        double lat = latitudeEdits[i]->text().toDouble(&latOk);
        double lon = longitudeEdits[i]->text().toDouble(&lonOk);
        
        if (!latOk || !lonOk) {
            QMessageBox::warning(this, "Ошибка", 
                               QString("Некорректные координаты для точки %1").arg(i + 1));
            return;
        }
        
        params.coordinates.append(qMakePair(lat, lon));
    }
    
    resultsText->clear();
    resultsText->append("=== Параметры поиска ===");
    resultsText->append("Директория: " + params.searchDirectory);
    resultsText->append("Период: " + params.startDateTime.toString("dd.MM.yyyy HH:mm") + 
                       " - " + params.endDateTime.toString("dd.MM.yyyy HH:mm"));
    
    if (params.coordinateCount == 1) {
        resultsText->append(QString("Точка: %1, %2")
                           .arg(params.coordinates[0].first, 0, 'f', 6)
                           .arg(params.coordinates[0].second, 0, 'f', 6));
    } else {
        resultsText->append(QString("Область: [%1, %2] - [%3, %4]")
                           .arg(params.coordinates[0].first, 0, 'f', 6)
                           .arg(params.coordinates[0].second, 0, 'f', 6)
                           .arg(params.coordinates[1].first, 0, 'f', 6)
                           .arg(params.coordinates[1].second, 0, 'f', 6));
    }
    
    resultsText->append("\n=== Поиск файлов ===");
    
    QVector<QString> matchingFiles = findMatchingFiles();
    
    resultsText->append("\nНайдено файлов: " + QString::number(matchingFiles.size()));
    
    if (!matchingFiles.isEmpty()) {
        resultsText->append("\n=== Список найденных файлов ===");
        for (const QString& file : matchingFiles) {
            resultsText->append(file);
        }
    } else {
        resultsText->append("\nФайлы, удовлетворяющие условиям, не найдены.");
    }
}


QVector<QString> CalibrationDialog::findMatchingFiles() {
    QVector<QString> result;
    
    QDir dir(params.searchDirectory);
    if (!dir.exists()) {
        resultsText->append("Ошибка: Директория не существует");
        return result;
    }
    
    QStringList xmlFilters;
    xmlFilters << "*.xml" << "*.XML";
    QFileInfoList xmlFiles = dir.entryInfoList(xmlFilters, QDir::Files);
    
    resultsText->append(QString("Найдено XML файлов метаданных: %1").arg(xmlFiles.size()));
    
    int matchedCount = 0;
    
    for (const QFileInfo& xmlFileInfo : xmlFiles) {
        QString xmlPath = xmlFileInfo.absoluteFilePath();
        
        QFile xmlFile(xmlPath);
        if (!xmlFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }
        
        QXmlStreamReader xml(&xmlFile);
        
        QString imagingDate;
        QString imagingTime;
        QString tiffFileName;
        QVector<double> latitudes;
        QVector<double> longitudes;
        bool hasCoordinates = false;
        
        while (!xml.atEnd()) {
            xml.readNext();
            
            if (xml.isStartElement()) {
                QString elementName = xml.name().toString();
                
                if (elementName == "IMAGING_DATE") {
                    imagingDate = xml.readElementText().trimmed();
                } else if (elementName == "IMAGING_TIME") {
                    imagingTime = xml.readElementText().trimmed();
                } else if (elementName == "DATA_FILE_NAME") {
                    tiffFileName = xml.attributes().value("href").toString();
                } else if (elementName == "DetailRegion" || elementName == "RectRegion") {
                    while (!(xml.isEndElement() && (xml.name() == QString("DetailRegion") || xml.name() == QString("RectRegion")))) {
                        xml.readNext();
                        if (xml.isStartElement()) {
                            if (xml.name() == QString("PointLatArr")) {
                                QString latStr = xml.readElementText().trimmed();
                                QStringList latList = latStr.split(',', Qt::SkipEmptyParts);
                                for (const QString& lat : latList) {
                                    latitudes.append(lat.trimmed().toDouble());
                                }
                            } else if (xml.name() == QString("PointLonArr")) {
                                QString lonStr = xml.readElementText().trimmed();
                                QStringList lonList = lonStr.split(',', Qt::SkipEmptyParts);
                                for (const QString& lon : lonList) {
                                    longitudes.append(lon.trimmed().toDouble());
                                }
                            }
                        }
                    }
                    
                    if (!latitudes.isEmpty() && !longitudes.isEmpty()) {
                        hasCoordinates = true;
                    }
                }
            }
        }
        
        xmlFile.close();
        
        if (xml.hasError()) {
            resultsText->append(QString("Ошибка парсинга XML: %1 - %2").arg(xmlFileInfo.fileName()).arg(xml.errorString()));
            continue;
        }
        
        // Проверка временного интервала
        if (imagingDate.isEmpty()) {
            resultsText->append(QString("⚠️ %1: нет даты съемки").arg(xmlFileInfo.fileName()));
            continue;
        }
        
        QDateTime imageDateTime = QDateTime::fromString(
            imagingDate + " " + imagingTime, 
            "yyyy-MM-dd HH:mm:ss"
        );
        
        if (!imageDateTime.isValid()) {
            imageDateTime = QDateTime::fromString(imagingDate, "yyyy-MM-dd");
        }
        
        bool timeMatches = false;
        if (imageDateTime.isValid()) {
            timeMatches = (imageDateTime >= params.startDateTime &&

                          imageDateTime <= params.endDateTime);
        } else {
            resultsText->append(QString("⚠️ %1: некорректная дата - %2 %3").arg(xmlFileInfo.fileName()).arg(imagingDate).arg(imagingTime));
            continue;
        }
        
        if (!timeMatches) {
            resultsText->append(QString("× %1: не попадает во временной интервал (%2)").arg(xmlFileInfo.fileName()).arg(imageDateTime.toString("dd.MM.yyyy HH:mm")));
            continue;
        }
        
        bool coordsMatch = false;
        if (hasCoordinates && latitudes.size() == longitudes.size()) {
            // Вычисляем границы полигона снимка
            double minLat = *std::min_element(latitudes.begin(), latitudes.end());
            double maxLat = *std::max_element(latitudes.begin(), latitudes.end());
            double minLon = *std::min_element(longitudes.begin(), longitudes.end());
            double maxLon = *std::max_element(longitudes.begin(), longitudes.end());
            
            if (params.coordinateCount == 1) {
                // Точка должна попадать в bounding box области изображения
                double lat = params.coordinates[0].first;
                double lon = params.coordinates[0].second;
                coordsMatch = (lat >= minLat && lat <= maxLat && 
                              lon >= minLon && lon <= maxLon);
                
                if (!coordsMatch) {
                    resultsText->append(QString("× %1: точка [%2, %3] не попадает в область [%4-%5, %6-%7]")
                        .arg(xmlFileInfo.fileName())
                        .arg(lat, 0, 'f', 4).arg(lon, 0, 'f', 4)
                        .arg(minLat, 0, 'f', 4).arg(maxLat, 0, 'f', 4)
                        .arg(minLon, 0, 'f', 4).arg(maxLon, 0, 'f', 4));
                }
            } else if (params.coordinateCount == 2) {
                // Прямоугольники должны пересекаться
                double reqMinLat = qMin(params.coordinates[0].first, params.coordinates[1].first);
                double reqMaxLat = qMax(params.coordinates[0].first, params.coordinates[1].first);
                double reqMinLon = qMin(params.coordinates[0].second, params.coordinates[1].second);
                double reqMaxLon = qMax(params.coordinates[0].second, params.coordinates[1].second);
                
                coordsMatch = !(reqMaxLat < minLat || reqMinLat > maxLat ||
                               reqMaxLon < minLon || reqMinLon > maxLon);
                
                if (!coordsMatch) {
                    resultsText->append(QString("× %1: области не пересекаются").arg(xmlFileInfo.fileName()));
                }
            }
            
            // Добавление в результаты
            if (timeMatches && coordsMatch) {
                matchedCount++;
                
                QString resultLine = QString("✓ %1").arg(xmlFileInfo.fileName());
                if (imageDateTime.isValid()) {
                    resultLine += QString(" | Дата: %1").arg(imageDateTime.toString("dd.MM.yyyy HH:mm:ss"));
                }
                resultLine += QString(" | Координаты: [%1-%2, %3-%4]")
                    .arg(minLat, 0, 'f', 4)
                    .arg(maxLat, 0, 'f', 4)
                    .arg(minLon, 0, 'f', 4)
                    .arg(maxLon, 0, 'f', 4);
                
                resultLine += QString(" | Точек: %1").arg(latitudes.size());
                
                if (!tiffFileName.isEmpty()) {
                    resultLine += QString(" | TIFF: %1").arg(tiffFileName);
                    
                    QString tiffPath = dir.absoluteFilePath(tiffFileName);
                    if (QFile::exists(tiffPath)) {
                        result.append(tiffPath);
                    } else {
                        resultLine += " (файл не найден)";
                    }
                }
                
                resultsText->append(resultLine);
            }

        } else {
            resultsText->append(QString("⚠️ %1: нет координат в метаданных").arg(xmlFileInfo.fileName()));
        }
    }
    
    resultsText->append(QString("\n=== Итого файлов, подходящих по всем условиям: %1 ===").arg(matchedCount));
    
    return result;
}

CalibrationParams CalibrationDialog::getParameters() const {
    return params;
}
