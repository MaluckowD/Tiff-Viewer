#ifndef CALIBRATION_DIALOG_H
#define CALIBRATION_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QVector>
#include <QDateTime>

struct CalibrationParams {
    QString searchDirectory;
    QDateTime startDateTime;
    QDateTime endDateTime;
    QString kmlFilePath;
    int coordinateCount;
    QVector<QPair<double, double>> coordinates;
};

class CalibrationDialog : public QDialog {
    Q_OBJECT

public:
    explicit CalibrationDialog(QWidget* parent = nullptr);
    CalibrationParams getParameters() const;

private slots:
    void onBrowseDirectory();
    void onBrowseKML();
    void onCoordinateCountChanged(int count);
    void onStartCalibration();
    void parseKMLFile(const QString& filePath);

private:
    void setupUI();
    void updateCoordinateFields();
    QVector<QString> findMatchingFiles();
    
    QLineEdit* directoryEdit;
    QPushButton* browseDirectoryButton;
    
    QDateTimeEdit* startDateTimeEdit;
    QDateTimeEdit* endDateTimeEdit;
    
    QLineEdit* kmlFileEdit;
    QPushButton* browseKMLButton;
    
    QSpinBox* coordinateCountSpinBox;
    
    QGroupBox* coordinatesGroupBox;
    QVector<QLineEdit*> latitudeEdits;
    QVector<QLineEdit*> longitudeEdits;
    
    QTextEdit* resultsText;
    QPushButton* startButton;
    QPushButton* closeButton;
    
    CalibrationParams params;
};

#endif
