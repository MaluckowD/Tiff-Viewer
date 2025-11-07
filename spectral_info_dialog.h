#ifndef SPECTRAL_INFO_DIALOG_H
#define SPECTRAL_INFO_DIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include "spectral_reader.h"

class SpectralInfoDialog : public QDialog {
    Q_OBJECT
    
public:
    SpectralInfoDialog(const QVector<SpectralBand>& bands, QWidget* parent = nullptr);
    
private slots:
    void copyToClipboard();
    
private:
    void setupUI();
    void populateTable();
    
    QVector<SpectralBand> spectralBands;
    QTableWidget* tableWidget;
    QLabel* infoLabel;
};

#endif