#include "tiff_reader.h"
#include <QDebug>
#include <tiffio.h>

bool TiffReader::readTiffInfo(const QString& filePath, TiffInfo& info) {
    TIFF* tif = TIFFOpen(filePath.toLocal8Bit().constData(), "r");
    if (!tif) {
        qDebug() << "Failed to open TIFF file:" << filePath;
        return false;
    }

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &info.width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &info.height);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &info.bitsPerSample);
    
    uint16_t samplesPerPixel = 1;
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
    
    uint16_t dirCount = 0;
    do {
        dirCount++;
    } while (TIFFReadDirectory(tif));
    
    if (dirCount > 1 && samplesPerPixel == 1) {
        info.numChannels = dirCount;
    } else if (dirCount == 1 && samplesPerPixel > 1) {
        info.numChannels = samplesPerPixel;
    } else {
        info.numChannels = 1;
    }

    TIFFClose(tif);
    return true;
}

bool TiffReader::loadTiffData(const QString& filePath, std::vector<std::vector<uint16_t>>& channels, const TiffInfo& info) {
    TIFF* tif = TIFFOpen(filePath.toLocal8Bit().constData(), "r");
    if (!tif) return false;

    channels.clear();
    channels.resize(info.numChannels);
    for (uint32_t i = 0; i < info.numChannels; i++) {
        channels[i].resize(info.width * info.height, 0);
    }

    uint16_t samplesPerPixel = 1;
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
    
    uint16_t dirCount = 0;
    do {
        dirCount++;
    } while (TIFFReadDirectory(tif));
    TIFFSetDirectory(tif, 0);

    bool result = false;
    if (dirCount > 1 && samplesPerPixel == 1) {
        result = loadMultiPageTiff(tif, channels, info);
    } else if (samplesPerPixel > 1) {
        result = loadSinglePageTiff(tif, channels, info);
    } else {
        result = loadSingleChannelTiff(tif, channels, info);
    }
    
    TIFFClose(tif);
    return result;
}

bool TiffReader::loadMultiPageTiff(void* tif_ptr, std::vector<std::vector<uint16_t>>& channels, const TiffInfo& info) {
    TIFF* tif = static_cast<TIFF*>(tif_ptr);
    
    for (uint32_t channelIndex = 0; channelIndex < info.numChannels; channelIndex++) {
        if (TIFFSetDirectory(tif, channelIndex) != 1) continue;
        
        tdata_t buf = _TIFFmalloc(TIFFScanlineSize(tif));
        if (!buf) continue;
        
        for (uint32_t row = 0; row < info.height; row++) {
            if (TIFFReadScanline(tif, buf, row) != 1) continue;
            
            if (info.bitsPerSample == 16) {
                uint16_t* scanline = static_cast<uint16_t*>(buf);
                for (uint32_t col = 0; col < info.width; col++) {
                    channels[channelIndex][row * info.width + col] = scanline[col];
                }
            } else if (info.bitsPerSample == 8) {
                uint8_t* scanline = static_cast<uint8_t*>(buf);
                for (uint32_t col = 0; col < info.width; col++) {
                    channels[channelIndex][row * info.width + col] = scanline[col] * 257;
                }
            }
        }
        _TIFFfree(buf);
    }
    return true;
}

bool TiffReader::loadSinglePageTiff(void* tif_ptr, std::vector<std::vector<uint16_t>>& channels, const TiffInfo& info) {
    TIFF* tif = static_cast<TIFF*>(tif_ptr);
    
    tdata_t buf = _TIFFmalloc(TIFFScanlineSize(tif));
    if (!buf) return false;
    
    for (uint32_t row = 0; row < info.height; row++) {
        if (TIFFReadScanline(tif, buf, row) != 1) continue;
        
        if (info.bitsPerSample == 16) {
            uint16_t* scanline = static_cast<uint16_t*>(buf);
            for (uint32_t col = 0; col < info.width; col++) {
                for (uint32_t band = 0; band < info.numChannels; band++) {
                    channels[band][row * info.width + col] = scanline[col * info.numChannels + band];
                }
            }
        } else if (info.bitsPerSample == 8) {
            uint8_t* scanline = static_cast<uint8_t*>(buf);
            for (uint32_t col = 0; col < info.width; col++) {
                for (uint32_t band = 0; band < info.numChannels; band++) {
                    channels[band][row * info.width + col] = scanline[col * info.numChannels + band] * 257;
                }
            }
        }
    }
    _TIFFfree(buf);
    return true;
}

bool TiffReader::loadSingleChannelTiff(void* tif_ptr, std::vector<std::vector<uint16_t>>& channels, const TiffInfo& info) {
    TIFF* tif = static_cast<TIFF*>(tif_ptr);
    
    tdata_t buf = _TIFFmalloc(TIFFScanlineSize(tif));
    if (!buf) return false;
    
    for (uint32_t row = 0; row < info.height; row++) {
        if (TIFFReadScanline(tif, buf, row) != 1) continue;
        
        if (info.bitsPerSample == 16) {
            uint16_t* scanline = static_cast<uint16_t*>(buf);
            for (uint32_t col = 0; col < info.width; col++) {
                channels[0][row * info.width + col] = scanline[col];
            }
        } else if (info.bitsPerSample == 8) {
            uint8_t* scanline = static_cast<uint8_t*>(buf);
            for (uint32_t col = 0; col < info.width; col++) {
                channels[0][row * info.width + col] = scanline[col] * 257;
            }
        }
    }
    _TIFFfree(buf);
    return true;
}
