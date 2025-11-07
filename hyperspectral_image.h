#ifndef HYPERSPECTRAL_IMAGE_H
#define HYPERSPECTRAL_IMAGE_H

#include <QString>
#include <QImage>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <memory>

class HyperspectralImage {
public:
    struct ContrastParams {
        uint16_t minVal = 0;
        uint16_t maxVal = 65535;
        double percentCutLow = 2.0;
        double percentCutHigh = 2.0;
        bool usePercentile = false;
    };

    struct CachedHistogram {
        std::vector<int> histogram;
        std::pair<uint16_t, uint16_t> minMax;
        bool isValid = false;
    };

    bool loadFromTiff(const QString& filePath);
    
    void normalizeToRange(int channelIndex, uint16_t minVal, uint16_t maxVal);
    void normalizeByPercentile(int channelIndex, double percentLow, double percentHigh);
    
    QImage getChannelImage(int channelIndex);
    QImage getRGBImage(int redChannel, int greenChannel, int blueChannel);
    
    std::vector<int> calculateHistogram16bit(int channelIndex);
    std::pair<uint16_t, uint16_t> calculatePercentileBounds(const std::vector<int>& histogram, 
                                                           double percentLow, double percentHigh);
    std::pair<uint16_t, uint16_t> getChannelMinMax16bit(int channelIndex);
    
    ContrastParams getContrastParams(int channelIndex) const;
    
    uint16_t getPixel16bit(int channelIndex, int x, int y) const;
    uint8_t getPixel8bit(int channelIndex, int x, int y) const;
    
    std::vector<uint16_t> getPixelSpectrum16bit(int x, int y) const;
    
    int getNumChannels() const { return numChannels; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
    const std::vector<uint16_t>& get16bitData(int channelIndex) const;

    void setMaxCachedChannels(int maxChannels) { maxCached16bit = maxChannels; }
    void clearUnusedChannels();
    void preloadChannels(const std::vector<int>& channelIndices);
    size_t getMemoryUsage() const;

private:
    void update8bitData(int channelIndex);
    void updateAll8bitData();
    
    bool loadChannel16bit(int channelIndex) const;
    void evictOldestChannel();
    void markChannelAsUsed(int channelIndex) const;

    mutable std::unordered_map<int, std::vector<uint16_t>> img16bit;  // Ленивая загрузка каналов
    mutable std::unordered_map<int, std::vector<uint8_t>> img8bit;    // Кэш 8-битных данных
    mutable std::unordered_map<int, CachedHistogram> histogramCache;  // Кэш гистограмм
    
    mutable std::vector<int> channelAccessOrder;  // Порядок доступа к каналам (LRU)
    mutable std::unordered_set<int> activeChannels;  // Активные каналы в памяти
    
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t numChannels = 0;
    std::vector<ContrastParams> channelContrast;
    
    QString tiffFilePath;  // Путь к TIFF файлу для ленивой загрузки
    int maxCached16bit = 5;  // Максимальное количество каналов в памяти
    int maxCached8bit = 10;  // Максимальное количество 8-битных каналов
};

#endif
