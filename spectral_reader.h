#ifndef SPECTRAL_READER_H
#define SPECTRAL_READER_H

#include <QString>
#include <QVector>
#include <QStringList>

struct SpectralBand {
    int bandNumber;
    double wavelength;
    double waveDelta;
    int oepNum;
    QString description;
};

class SpectralReader {
public:
    static bool readSppFile(const QString& filePath, QVector<SpectralBand>& bands);
    static bool readSppXmlFile(const QString& filePath, QVector<SpectralBand>& bands, int& totalRasterBands);
    static bool readHdrFile(const QString& filePath, QVector<SpectralBand>& bands);
    static bool createTestSppFile(const QString& filePath, int numChannels = 100);
    
private:
    static QVector<double> parseWavelengthList(const QString& wavelengthString);
    static QString cleanValue(const QString& value);
};

#endif