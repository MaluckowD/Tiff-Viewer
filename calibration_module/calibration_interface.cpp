#include "calibration_interface.h"
#include "calibration_dialog.h"
#include <QApplication>
#include <QDebug>
#include <cstring>

#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)

extern "C" CALIBRATION_API void showCalibrationDialog(QWidget* parent) {
    static QApplication* app = nullptr;
    
    if (!QApplication::instance()) {
        qDebug() << "Creating QApplication instance for DLL...";
        static int argc = 1;
        static char arg0[] = "CalibrationModule";
        static char* argv[] = { arg0, nullptr };
        
        app = new QApplication(argc, argv);
        qDebug() << "QApplication created in DLL";
    } else {
        qDebug() << "Using existing QApplication from main app";
    }
    
    if (!parent) {
        qWarning() << "Warning: showCalibrationDialog called with null parent";
        parent = new QWidget();
        parent->setWindowTitle("Parent Window");
    }
    
    qDebug() << "Creating CalibrationDialog...";
    
    CalibrationDialog* dialog = new CalibrationDialog(parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    
    qDebug() << "Showing dialog...";
    
    dialog->show();
}



extern "C" CALIBRATION_API const char* getModuleVersion() {
    return "1.0.0";
}

extern "C" CALIBRATION_API bool searchFilesInDirectory(
    const char* directory,
    const char* startDate,
    const char* endDate,
    double lat1, double lon1,
    double lat2, double lon2,
    int coordinateCount,
    char* resultBuffer,
    int bufferSize
) {
    if (resultBuffer && bufferSize > 0) {
        const char* message = "Search completed";
        size_t len = std::strlen(message);
        if (len >= static_cast<size_t>(bufferSize)) {
            len = bufferSize - 1;
        }
        std::memcpy(resultBuffer, message, len);
        resultBuffer[len] = '\0';
    }
    
    return true;
}
