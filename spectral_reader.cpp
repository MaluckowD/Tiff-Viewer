#include "spectral_reader.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>
#include <QFileInfo>
#include <QXmlStreamReader>

bool SpectralReader::readSppFile(const QString& filePath, QVector<SpectralBand>& bands) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open SPP file:" << filePath;
        qDebug() << "Error:" << file.errorString();
        return false;
    }
    
    QTextStream stream(&file);
    QString firstLine = stream.readLine().trimmed();
    file.seek(0);
    
    if (firstLine.startsWith("<?xml") || firstLine.contains("<SPP_ROOT>")) {
        int totalRasterBands = 0;
        return readSppXmlFile(filePath, bands, totalRasterBands);
    }
    
    bands.clear();
    QTextStream in(&file);
    in.setCodec("UTF-8");
    
    QString content = in.readAll();
    qDebug() << "File size:" << file.size() << "bytes";
    qDebug() << "Content preview:" << content.left(500);
    
    if (content.isEmpty()) {
        qDebug() << "File is empty";
        return false;
    }
    
    QStringList allLines = content.split('\n');
    qDebug() << "Total lines in file:" << allLines.size();
    
    in.seek(0);
    
    QString line;
    int bandNumber = 1;
    int lineNumber = 0;
    int skippedLines = 0;
    int processedLines = 0;
    
    while (!in.atEnd()) {
        line = in.readLine().trimmed();
        lineNumber++;
        
        if (line.isEmpty() || line.startsWith("#") || line.startsWith(";") || 
            line.startsWith("//") || line.startsWith("*") || line.startsWith("%")) {
            skippedLines++;
            continue;
        }
        
        if (line.toLower().contains("wavelength") || line.toLower().contains("band") ||
            line.toLower().contains("channel") || line.toLower().contains("nm") ||
            line.toLower().contains("nanometer")) {
            skippedLines++;
            continue;
        }
        
        processedLines++;
        
        QStringList parts = line.split(QRegularExpression("[,;\\s\\t]+"), Qt::SkipEmptyParts);
        if (parts.isEmpty()) {
            skippedLines++;
            continue;
        }
        
        bool lineProcessed = false;
        
        for (const QString& part : parts) {
            bool ok;
            QString cleanPart = part.trimmed().replace(",", ".");
            double wavelength = cleanPart.toDouble(&ok);
            
            if (ok && wavelength > 0 && wavelength < 50000) {
                SpectralBand band;
                band.bandNumber = bandNumber++;
                band.wavelength = wavelength;
                band.waveDelta = 0.0;
                band.oepNum = 1;
                band.description = QString("Band %1").arg(band.bandNumber);
                bands.append(band);
                lineProcessed = true;
                
                if (bands.size() <= 10 || bands.size() % 20 == 0) {
                    qDebug() << "Added band:" << band.bandNumber << "wavelength:" << band.wavelength;
                }
            }
        }
        
        if (!lineProcessed) {
            qDebug() << "Failed to parse line" << lineNumber << ":" << line;
        }
    }
    
    qDebug() << "=== SPP File Reading Summary ===";
    qDebug() << "Total lines in file:" << lineNumber;
    qDebug() << "Lines skipped (comments/empty):" << skippedLines;
    qDebug() << "Lines processed:" << processedLines;
    qDebug() << "Spectral bands extracted:" << bands.size();
    qDebug() << "================================";
    
    return !bands.isEmpty();
}

bool SpectralReader::readSppXmlFile(const QString& filePath, QVector<SpectralBand>& bands, int& totalRasterBands) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open SPP XML file:" << filePath;
        return false;
    }
    
    bands.clear();
    totalRasterBands = 0;
    
    QXmlStreamReader xml(&file);
    
    qDebug() << "=== Reading SPP XML File ===";
    qDebug() << "File:" << filePath;
    
    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        
        if (token == QXmlStreamReader::StartElement) {
            QString elementName = xml.name().toString();
            
            if (elementName == "nRasterBands") {
                QString bandsText = xml.readElementText();
                bool ok;
                totalRasterBands = bandsText.toInt(&ok);
                if (ok) {
                    qDebug() << "Total raster bands:" << totalRasterBands;
                }
            }
            
            else if (elementName == "WaveLength") {
                SpectralBand band;
                band.bandNumber = 0;
                band.wavelength = 0.0;
                band.waveDelta = 0.0;
                band.oepNum = 1;
                
                while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "WaveLength")) {
                    xml.readNext();
                    
                    if (xml.tokenType() == QXmlStreamReader::StartElement) {
                        QString childName = xml.name().toString();
                        QString childText = xml.readElementText();
                        
                        if (childName == "ChannelNumber") {
                            bool ok;
                            band.bandNumber = childText.toInt(&ok);
                        }
                        else if (childName == "WaveLen") {
                            bool ok;
                            band.wavelength = childText.toDouble(&ok);
                        }
                        else if (childName == "WaveDelta") {
                            bool ok;
                            band.waveDelta = childText.toDouble(&ok);
                        }
                        else if (childName == "OepNum") {
                            bool ok;
                            band.oepNum = childText.toInt(&ok);
                        }
                    }
                }
                
                band.description = QString("Channel %1 (λ=%2nm, Δλ=%3nm, OEP=%4)")
                                  .arg(band.bandNumber)
                                  .arg(band.wavelength, 0, 'f', 2)
                                  .arg(band.waveDelta, 0, 'f', 2)
                                  .arg(band.oepNum);
                
                bands.append(band);
                
                if (bands.size() <= 10 || bands.size() % 20 == 0) {
                    qDebug() << "Added XML band:" << band.bandNumber 
                             << "wavelength:" << band.wavelength 
                             << "delta:" << band.waveDelta
                             << "OEP:" << band.oepNum;
                }
            }
        }
    }
    
    if (xml.hasError()) {
        qDebug() << "XML parsing error:" << xml.errorString();
        return false;
    }
    
    std::sort(bands.begin(), bands.end(), [](const SpectralBand& a, const SpectralBand& b) {
        return a.bandNumber < b.bandNumber;
    });
    
    qDebug() << "=== SPP XML Reading Summary ===";
    qDebug() << "Total raster bands in file:" << totalRasterBands;
    qDebug() << "Spectral records found:" << bands.size();
    if (!bands.isEmpty()) {
        qDebug() << "Channel range:" << bands.first().bandNumber << "-" << bands.last().bandNumber;
        qDebug() << "Wavelength range:" << bands.first().wavelength << "-" << bands.last().wavelength << "nm";
    }
    qDebug() << "===============================";
    
    return !bands.isEmpty();
}

bool SpectralReader::readHdrFile(const QString& filePath, QVector<SpectralBand>& bands) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open HDR file:" << filePath;
        qDebug() << "Error:" << file.errorString();
        return false;
    }
    
    bands.clear();
    QTextStream in(&file);
    in.setCodec("UTF-8");
    QString content = in.readAll();
    
    qDebug() << "HDR file size:" << file.size() << "bytes";
    qDebug() << "HDR content preview:" << content.left(300);
    
    QRegularExpression wavelengthRegex(R"(wavelength\s*=\s*\{([^}]+)\})", QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = wavelengthRegex.match(content);
    
    if (match.hasMatch()) {
        QString wavelengthString = match.captured(1);
        qDebug() << "Found wavelength section:" << wavelengthString.left(100);
        QVector<double> wavelengths = parseWavelengthList(wavelengthString);
        
        for (int i = 0; i < wavelengths.size(); ++i) {
            SpectralBand band;
            band.bandNumber = i + 1;
            band.wavelength = wavelengths[i];
            band.waveDelta = 0.0;
            band.oepNum = 1;
            band.description = QString("Band %1").arg(band.bandNumber);
            bands.append(band);
        }
        
        qDebug() << "Loaded" << bands.size() << "spectral bands from HDR file";
        return !bands.isEmpty();
    }
    
    in.seek(0);
    QString line;
    bool inWavelengthSection = false;
    int bandNumber = 1;
    
    while (!in.atEnd()) {
        line = in.readLine().trimmed();
        
        if (line.toLower().contains("wavelength") && line.contains("=")) {
            inWavelengthSection = true;
            
            int equalPos = line.indexOf("=");
            if (equalPos >= 0) {
                QString valuesPart = line.mid(equalPos + 1).trimmed();
                if (!valuesPart.isEmpty() && !valuesPart.startsWith("{")) {
                    QStringList parts = valuesPart.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);
                    for (const QString& part : parts) {
                        bool ok;
                        double wavelength = cleanValue(part).toDouble(&ok);
                        if (ok && wavelength > 0) {
                            SpectralBand band;
                            band.bandNumber = bandNumber++;
                            band.wavelength = wavelength;
                            band.waveDelta = 0.0;
                            band.oepNum = 1;
                            band.description = QString("Band %1").arg(band.bandNumber);
                            bands.append(band);
                        }
                    }
                }
            }
            continue;
        }
        
        if (inWavelengthSection) {
            if (line.isEmpty() || line.startsWith("}") || line.toLower().contains("band")) {
                if (line.startsWith("}")) {
                    break;
                }
                continue;
            }
            
            QStringList parts = line.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);
            for (const QString& part : parts) {
                bool ok;
                double wavelength = cleanValue(part).toDouble(&ok);
                if (ok && wavelength > 0 && wavelength < 50000) {
                    SpectralBand band;
                    band.bandNumber = bandNumber++;
                    band.wavelength = wavelength;
                    band.waveDelta = 0.0;
                    band.oepNum = 1;
                    band.description = QString("Band %1").arg(band.bandNumber);
                    bands.append(band);
                }
            }
        }
    }
    
    qDebug() << "Loaded" << bands.size() << "spectral bands from HDR file";
    return !bands.isEmpty();
}

QVector<double> SpectralReader::parseWavelengthList(const QString& wavelengthString) {
    QVector<double> wavelengths;
    
    
    QString cleaned = wavelengthString;
    cleaned.remove(QRegularExpression("[{}\\[\\]]"));
    
    QStringList parts = cleaned.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);
    
    
    for (const QString& part : parts) {
        bool ok;
        double wavelength = cleanValue(part).toDouble(&ok);
        if (ok && wavelength > 0 && wavelength < 50000) {
            wavelengths.append(wavelength);
        } else {
        }
    }
    
    qDebug() << "Parsed" << wavelengths.size() << "wavelengths";
    return wavelengths;
}

QString SpectralReader::cleanValue(const QString& value) {
    QString cleaned = value.trimmed();
    cleaned.remove(QRegularExpression("[,;{}\\[\\]]"));
    cleaned.replace(",", ".");
    return cleaned;
}

bool SpectralReader::createTestSppFile(const QString& filePath, int numChannels) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream out(&file);
    out.setCodec("UTF-8");
    
    out << "# Пример файла спектральных длин волн (.spp)\n";
    out << "# Каждая строка содержит длину волны в нанометрах\n";
    out << "# Комментарии начинаются с # или ;\n";
    out << QString("# Создано для %1 каналов\n").arg(numChannels);
    out << "\n";
    
    double startWavelength = 400.0;
    double endWavelength = 1000.0;
    double step = (endWavelength - startWavelength) / (numChannels - 1);
    
    for (int i = 0; i < numChannels; ++i) {
        double wavelength = startWavelength + i * step;
        out << QString::number(wavelength, 'f', 2) << "\n";
    }
    
    return true;
}
