#include "spectral_info_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QClipboard>
#include <QApplication>
#include <QMessageBox>

SpectralInfoDialog::SpectralInfoDialog(const QVector<SpectralBand>& bands, QWidget* parent)
    : QDialog(parent), spectralBands(bands) {
    setWindowTitle("Информация о спектральных каналах");
    setMinimumSize(800, 600);
    resize(1000, 700);
    
    setupUI();
    populateTable();
}

void SpectralInfoDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    infoLabel = new QLabel();
    infoLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 8px; border: 1px solid #ccc; }");
    mainLayout->addWidget(infoLabel);
    
    tableWidget = new QTableWidget();
    tableWidget->setColumnCount(6);
    
    QStringList headers;
    headers << "№ в изображении" << "№ канала (XML)" << "Длина волны (нм)" << "Дельта λ (нм)" << "ОЭП" << "Описание";
    tableWidget->setHorizontalHeaderLabels(headers);
    
    QHeaderView* horizontalHeader = tableWidget->horizontalHeader();
    horizontalHeader->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    horizontalHeader->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    horizontalHeader->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    horizontalHeader->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    horizontalHeader->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    horizontalHeader->setSectionResizeMode(5, QHeaderView::Stretch);
    
    tableWidget->setAlternatingRowColors(true);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setSortingEnabled(true);
    
    mainLayout->addWidget(tableWidget);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    QPushButton* copyButton = new QPushButton("Копировать в буфер");
    QPushButton* closeButton = new QPushButton("Закрыть");

    connect(copyButton, &QPushButton::clicked, this, &SpectralInfoDialog::copyToClipboard);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    buttonLayout->addWidget(copyButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    
    mainLayout->addLayout(buttonLayout);
}

void SpectralInfoDialog::populateTable() {
    tableWidget->setRowCount(spectralBands.size());
    
    for (int i = 0; i < spectralBands.size(); ++i) {
        const SpectralBand& band = spectralBands[i];
        
        QTableWidgetItem* indexItem = new QTableWidgetItem(QString::number(i + 1));
        indexItem->setTextAlignment(Qt::AlignCenter);
        tableWidget->setItem(i, 0, indexItem);
        
        QTableWidgetItem* channelItem = new QTableWidgetItem();
        if (band.bandNumber > 0) {
            channelItem->setText(QString::number(band.bandNumber));
        } else {
            channelItem->setText("—");
        }
        channelItem->setTextAlignment(Qt::AlignCenter);
        tableWidget->setItem(i, 1, channelItem);
        
        QTableWidgetItem* wavelengthItem = new QTableWidgetItem();
        if (band.wavelength > 0) {
            wavelengthItem->setText(QString::number(band.wavelength, 'f', 3));
        } else {
            wavelengthItem->setText("—");
        }
        wavelengthItem->setTextAlignment(Qt::AlignCenter);
        tableWidget->setItem(i, 2, wavelengthItem);
        
        QTableWidgetItem* deltaItem = new QTableWidgetItem();
        if (band.waveDelta > 0) {
            deltaItem->setText(QString::number(band.waveDelta, 'f', 3));
        } else {
            deltaItem->setText("—");
        }
        deltaItem->setTextAlignment(Qt::AlignCenter);
        tableWidget->setItem(i, 3, deltaItem);
        
        // ОЭП
        QTableWidgetItem* oepItem = new QTableWidgetItem();
        if (band.oepNum > 0) {
            oepItem->setText(QString::number(band.oepNum));
        } else {
            oepItem->setText("—");
        }
        oepItem->setTextAlignment(Qt::AlignCenter);
        tableWidget->setItem(i, 4, oepItem);
        
        // Описание
        QTableWidgetItem* descItem = new QTableWidgetItem(band.description);
        tableWidget->setItem(i, 5, descItem);
    }
    
    if (!spectralBands.isEmpty()) {
        int totalChannels = spectralBands.size();
        int channelsWithData = 0;
        double minWavelength = 0;
        double maxWavelength = 0;
        bool firstWavelength = true;
        
        int minChannelNum = 0;
        int maxChannelNum = 0;
        bool firstChannel = true;
        
        for (const SpectralBand& band : spectralBands) {
            if (band.wavelength > 0) {
                channelsWithData++;
                if (firstWavelength) {
                    minWavelength = maxWavelength = band.wavelength;
                    firstWavelength = false;
                } else {
                    minWavelength = qMin(minWavelength, band.wavelength);
                    maxWavelength = qMax(maxWavelength, band.wavelength);
                }
            }
            
            if (band.bandNumber > 0) {
                if (firstChannel) {
                    minChannelNum = maxChannelNum = band.bandNumber;
                    firstChannel = false;
                } else {
                    minChannelNum = qMin(minChannelNum, band.bandNumber);
                    maxChannelNum = qMax(maxChannelNum, band.bandNumber);
                }
            }
        }
        
        QString infoText;
        if (channelsWithData > 0) {
            infoText = QString(
                "Всего записей: %1 | Каналы: %2-%3 | Диапазон длин волн: %4 - %5 нм")
                .arg(totalChannels)
                .arg(minChannelNum)
                .arg(maxChannelNum)
                .arg(minWavelength, 0, 'f', 2)
                .arg(maxWavelength, 0, 'f', 2);
        } else {
            infoText = QString("Всего записей: %1 | Нет спектральных данных").arg(totalChannels);
        }
        
        infoLabel->setText(infoText);
    }
}

void SpectralInfoDialog::copyToClipboard() {
    QString text;
    text += "№ п/п\t№ канала\tДлина волны (нм)\tДельта λ (нм)\tОЭП\tОписание\n";
    
    for (int i = 0; i < spectralBands.size(); ++i) {
        const SpectralBand& band = spectralBands[i];
        text += QString("%1\t%2\t%3\t%4\t%5\t%6\n")
                .arg(i + 1)
                .arg(band.bandNumber > 0 ? QString::number(band.bandNumber) : "")
                .arg(band.wavelength > 0 ? QString::number(band.wavelength, 'f', 3) : "")
                .arg(band.waveDelta > 0 ? QString::number(band.waveDelta, 'f', 3) : "")
                .arg(band.oepNum > 0 ? QString::number(band.oepNum) : "")
                .arg(band.description);
    }
    
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(text);
    
    QMessageBox::information(this, "Копирование завершено", 
        "Спектральная информация скопирована в буфер обмена");
}
