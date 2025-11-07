#include "hyperspectral_image.h"
#include "tiff_reader.h"
#include <QDebug>
#include <algorithm>
#include <cmath>

bool HyperspectralImage::loadFromTiff(const QString& filePath) {
    TiffReader::TiffInfo info;
    if (!TiffReader::readTiffInfo(filePath, info)) {
        qDebug() << "Failed to read TIFF info from" << filePath;
        return false;
    }
    
    width = info.width;
    height = info.height;
    numChannels = info.numChannels;
    tiffFilePath = filePath;
    
    std::vector<std::vector<uint16_t>> tempChannels;
    if (!TiffReader::loadTiffData(filePath, tempChannels, info)) {
        qDebug() << "Failed to load TIFF data from" << filePath;
        return false;
    }
    
    img16bit.clear();
    img8bit.clear();
    histogramCache.clear();
    
    for (int i = 0; i < static_cast<int>(numChannels); i++) {
        if (i < static_cast<int>(tempChannels.size())) {
            img16bit[i] = std::move(tempChannels[i]);
        }
    }
    
    channelContrast.resize(numChannels);
    
    for (int i = 0; i < static_cast<int>(numChannels); i++) {
        CachedHistogram cachedHist;
        cachedHist.histogram.resize(65536, 0);
        
        uint16_t minVal = 65535;
        uint16_t maxVal = 0;
        
        for (uint16_t val : img16bit[i]) {
            cachedHist.histogram[val]++;
            minVal = std::min(minVal, val);
            maxVal = std::max(maxVal, val);
        }
        
        cachedHist.minMax = {minVal, maxVal};
        cachedHist.isValid = true;
        histogramCache[i] = std::move(cachedHist);
        
        // Set default contrast parameters
        channelContrast[i].minVal = minVal;
        channelContrast[i].maxVal = maxVal;
    }
    
    qDebug() << "Successfully loaded TIFF:" << numChannels << "channels," << width << "x" << height;
    
    return true;
}

void HyperspectralImage::normalizeToRange(int channelIndex, uint16_t minVal, uint16_t maxVal) {
    if (channelIndex < 0 || channelIndex >= static_cast<int>(numChannels)) return;
    if (maxVal <= minVal) maxVal = minVal + 1;
    
    channelContrast[channelIndex].minVal = minVal;
    channelContrast[channelIndex].maxVal = maxVal;
    channelContrast[channelIndex].usePercentile = false;
    
    update8bitData(channelIndex);
}

void HyperspectralImage::normalizeByPercentile(int channelIndex, double percentLow, double percentHigh) {
    if (channelIndex < 0 || channelIndex >= static_cast<int>(numChannels)) return;
    
    channelContrast[channelIndex].percentCutLow = percentLow;
    channelContrast[channelIndex].percentCutHigh = percentHigh;
    channelContrast[channelIndex].usePercentile = true;
    
    auto histogram = calculateHistogram16bit(channelIndex);
    auto [minVal, maxVal] = calculatePercentileBounds(histogram, percentLow, percentHigh);
    
    channelContrast[channelIndex].minVal = minVal;
    channelContrast[channelIndex].maxVal = maxVal;
    
    update8bitData(channelIndex);
}

QImage HyperspectralImage::getChannelImage(int channelIndex) {
    if (channelIndex < 0 || channelIndex >= static_cast<int>(numChannels)) {
        return QImage();
    }
    
    if (img16bit.find(channelIndex) == img16bit.end()) {
        return QImage();
    }
    
    if (img8bit.find(channelIndex) == img8bit.end() || 
        img8bit[channelIndex].size() != width * height) {
        update8bitData(channelIndex);
    }
    
    QImage image(width, height, QImage::Format_Grayscale8);
    
    const auto& channel8bit = img8bit[channelIndex];
    for (uint32_t y = 0; y < height; y++) {
        uchar* scanLine = image.scanLine(y);
        for (uint32_t x = 0; x < width; x++) {
            uint32_t index = y * width + x;
            scanLine[x] = channel8bit[index];
        }
    }
    
    return image;
}

QImage HyperspectralImage::getRGBImage(int redChannel, int greenChannel, int blueChannel) {
    if (redChannel < 0 || redChannel >= static_cast<int>(numChannels) ||
        greenChannel < 0 || greenChannel >= static_cast<int>(numChannels) ||
        blueChannel < 0 || blueChannel >= static_cast<int>(numChannels)) {
        return QImage();
    }
    
    // Ensure 8-bit data is available for RGB channels
    update8bitData(redChannel);
    update8bitData(greenChannel);
    update8bitData(blueChannel);
    
    QImage image(width, height, QImage::Format_RGB32);
    
    const auto& redData = img8bit[redChannel];
    const auto& greenData = img8bit[greenChannel];
    const auto& blueData = img8bit[blueChannel];
    
    for (uint32_t y = 0; y < height; y++) {
        QRgb* scanLine = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (uint32_t x = 0; x < width; x++) {
            uint32_t index = y * width + x;
            
            uint8_t r = redData[index];
            uint8_t g = greenData[index];
            uint8_t b = blueData[index];
            
            scanLine[x] = qRgb(r, g, b);
        }
    }
    
    return image;
}

std::vector<int> HyperspectralImage::calculateHistogram16bit(int channelIndex) {
    if (channelIndex < 0 || channelIndex >= static_cast<int>(numChannels)) {
        return std::vector<int>(65536, 0);
    }
    
    auto it = histogramCache.find(channelIndex);
    if (it != histogramCache.end() && it->second.isValid) {
        return it->second.histogram;
    }
    
    // Fallback: calculate histogram if not cached
    std::vector<int> histogram(65536, 0);
    if (img16bit.find(channelIndex) != img16bit.end()) {
        for (uint16_t val : img16bit[channelIndex]) {
            histogram[val]++;
        }
    }
    
    return histogram;
}

std::pair<uint16_t, uint16_t> HyperspectralImage::calculatePercentileBounds(const std::vector<int>& histogram, 
                                                       double percentLow, double percentHigh) {
    size_t totalPixels = width * height;
    size_t lowCutoff = static_cast<size_t>(totalPixels * percentLow / 100.0);
    size_t highCutoff = static_cast<size_t>(totalPixels * (100.0 - percentHigh) / 100.0);
    
    uint16_t minVal = 0;
    uint16_t maxVal = 65535;
    
    size_t cumulative = 0;
    for (int i = 0; i < 65536; i++) {
        cumulative += histogram[i];
        if (cumulative >= lowCutoff && minVal == 0) {
            minVal = static_cast<uint16_t>(i);
        }
        if (cumulative >= highCutoff) {
            maxVal = static_cast<uint16_t>(i);
            break;
        }
    }
    
    return {minVal, maxVal};
}

std::pair<uint16_t, uint16_t> HyperspectralImage::getChannelMinMax16bit(int channelIndex) {
    if (channelIndex < 0 || channelIndex >= static_cast<int>(numChannels)) {
        return {0, 65535};
    }
    
    auto it = histogramCache.find(channelIndex);
    if (it != histogramCache.end() && it->second.isValid) {
        return it->second.minMax;
    }
    
    return {0, 65535};
}

HyperspectralImage::ContrastParams HyperspectralImage::getContrastParams(int channelIndex) const {
    if (channelIndex >= 0 && channelIndex < static_cast<int>(channelContrast.size())) {
        return channelContrast[channelIndex];
    }
    return ContrastParams{};
}

uint16_t HyperspectralImage::getPixel16bit(int channelIndex, int x, int y) const {
    if (channelIndex < 0 || channelIndex >= static_cast<int>(numChannels) ||
        x < 0 || x >= static_cast<int>(width) ||
        y < 0 || y >= static_cast<int>(height)) {
        return 0;
    }
    
    auto it = img16bit.find(channelIndex);
    if (it == img16bit.end()) {
        return 0;
    }
    
    uint32_t index = y * width + x;
    return it->second[index];
}

uint8_t HyperspectralImage::getPixel8bit(int channelIndex, int x, int y) const {
    if (channelIndex < 0 || channelIndex >= static_cast<int>(numChannels) ||
        x < 0 || x >= static_cast<int>(width) ||
        y < 0 || y >= static_cast<int>(height)) {
        return 0;
    }
    
    auto it = img8bit.find(channelIndex);
    if (it == img8bit.end()) {
        return 0;
    }
    
    uint32_t index = y * width + x;
    return it->second[index];
}

std::vector<uint16_t> HyperspectralImage::getPixelSpectrum16bit(int x, int y) const {
    std::vector<uint16_t> spectrum;
    spectrum.reserve(numChannels);
    
    if (x < 0 || x >= static_cast<int>(width) ||
        y < 0 || y >= static_cast<int>(height)) {
        return spectrum;
    }
    
    uint32_t index = y * width + x;
    
    for (int channelIndex = 0; channelIndex < static_cast<int>(numChannels); channelIndex++) {
        auto it = img16bit.find(channelIndex);
        if (it != img16bit.end() && index < it->second.size()) {
            spectrum.push_back(it->second[index]);
        } else {
            spectrum.push_back(0);
        }
    }
    
    return spectrum;
}

const std::vector<uint16_t>& HyperspectralImage::get16bitData(int channelIndex) const {
    static std::vector<uint16_t> empty;
    
    auto it = img16bit.find(channelIndex);
    if (it != img16bit.end()) {
        return it->second;
    }
    return empty;
}

void HyperspectralImage::update8bitData(int channelIndex) {
    if (channelIndex < 0 || channelIndex >= static_cast<int>(numChannels)) return;
    
    auto it16 = img16bit.find(channelIndex);
    if (it16 == img16bit.end()) return;
    
    img8bit[channelIndex].resize(width * height);
    
    const auto& params = channelContrast[channelIndex];
    uint16_t minVal = params.minVal;
    uint16_t maxVal = params.maxVal;
    
    if (maxVal <= minVal) maxVal = minVal + 1;
    
    const auto& channel16bit = it16->second;
    auto& channel8bit = img8bit[channelIndex];
    
    for (size_t i = 0; i < channel16bit.size(); i++) {
        uint16_t val = channel16bit[i];
        
        if (val <= minVal) {
            channel8bit[i] = 0;
        } else if (val >= maxVal) {
            channel8bit[i] = 255;
        } else {
            float normalized = static_cast<float>(val - minVal) / (maxVal - minVal);
            channel8bit[i] = static_cast<uint8_t>(normalized * 255);
        }
    }
}

void HyperspectralImage::updateAll8bitData() {
    for (const auto& pair : img16bit) {
        update8bitData(pair.first);
    }
}

bool HyperspectralImage::loadChannel16bit(int channelIndex) const {
    return img16bit.find(channelIndex) != img16bit.end();
}

void HyperspectralImage::evictOldestChannel() {
}

void HyperspectralImage::markChannelAsUsed(int channelIndex) const {
}

void HyperspectralImage::clearUnusedChannels() {
    img8bit.clear();
}

void HyperspectralImage::preloadChannels(const std::vector<int>& channelIndices) {
}

size_t HyperspectralImage::getMemoryUsage() const {
    size_t total = 0;
    
    // 16-bit data
    for (const auto& pair : img16bit) {
        total += pair.second.size() * sizeof(uint16_t);
    }
    
    // 8-bit data
    for (const auto& pair : img8bit) {
        total += pair.second.size() * sizeof(uint8_t);
    }
    
    // Histograms
    total += histogramCache.size() * 65536 * sizeof(int);
    
    return total;
}
