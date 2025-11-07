#ifndef TIFF_READER_H
#define TIFF_READER_H

#include <QString>
#include <vector>
#include <cstdint>

class TiffReader {
public:
    struct TiffInfo {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t numChannels = 0;
        uint16_t bitsPerSample = 16;
    };

    static bool readTiffInfo(const QString& filePath, TiffInfo& info);
    static bool loadTiffData(const QString& filePath, std::vector<std::vector<uint16_t>>& channels, const TiffInfo& info);

private:
    static bool loadMultiPageTiff(void* tif, std::vector<std::vector<uint16_t>>& channels, const TiffInfo& info);
    static bool loadSinglePageTiff(void* tif, std::vector<std::vector<uint16_t>>& channels, const TiffInfo& info);
    static bool loadSingleChannelTiff(void* tif, std::vector<std::vector<uint16_t>>& channels, const TiffInfo& info);
};

#endif
