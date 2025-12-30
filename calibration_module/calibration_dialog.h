#ifndef CALIBRATION_DIALOG_H
#define CALIBRATION_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QDateTime>
#include <QVector>
#include <QPair>
#include <QFileDialog>

class CalibrationDialog : public QDialog {
    Q_OBJECT

public:
    explicit CalibrationDialog(QWidget* parent = nullptr);
    ~CalibrationDialog() = default;
    
    struct CalibrationParams {
        QString searchDirectory;
        QDateTime startDateTime;
        QDateTime endDateTime;
        QString kmlFilePath;
        int coordinateCount;
        QVector<QPair<double, double>> coordinates;
    };
    
    CalibrationParams getParameters() const;

private slots:
    void onBrowseDirectory();
    void onBrowseKML();
    void onCoordinateCountChanged(int count);
    void onStartCalibration();

private:
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

    void setupUI();
    void updateCoordinateFields();
    void parseKMLFile(const QString& kmlFilePath);
    QVector<QString> findMatchingFiles();
};

#endif
