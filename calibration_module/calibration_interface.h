#ifndef CALIBRATION_INTERFACE_H
#define CALIBRATION_INTERFACE_H

#include <QWidget>
#include <QString>

#ifdef _WIN32
    #ifdef CALIBRATION_MODULE_EXPORT
        #define CALIBRATION_API __declspec(dllexport)
    #else
        #define CALIBRATION_API __declspec(dllimport)
    #endif
#else
    #define CALIBRATION_API
#endif

extern "C" {
    CALIBRATION_API void showCalibrationDialog(QWidget* parent);
    
    CALIBRATION_API const char* getModuleVersion();
    
    CALIBRATION_API bool searchFilesInDirectory(
        const char* directory,
        const char* startDate,
        const char* endDate,
        double lat1, double lon1,
        double lat2, double lon2,
        int coordinateCount,
        char* resultBuffer,
        int bufferSize
    );
}

#endif
