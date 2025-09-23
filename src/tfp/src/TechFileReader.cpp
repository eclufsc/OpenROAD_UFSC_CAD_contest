#include "TechFileReader.h"
#include "TechFileUtils.h"
#include <cctype>
#include <stdexcept>

namespace tfp {

TechFileReader::TechFileReader(utl::Logger* logger) : logger_(logger), pos_(0) {}

TechFileReader::~TechFileReader() = default;

TechFileData TechFileReader::parseContent(const std::string& content) {
    content_ = content;
    pos_ = 0;
    
    TechFileData data;
    
    try {
        data.technology = parseTechnology();
        data.colors = parseColors();
        data.layers = parseLayers();
        data.vias = parseVias();
        
    } catch (const std::exception& e) {
        logger_->error(utl::ODB, 200, "Parse error at {}: {}", pos_, e.what());
        throw;
    }
    
    return data;
}

void TechFileReader::skipWhitespace() {
    while (pos_ < content_.size() && std::isspace(content_[pos_])) {
        pos_++;
    }
}

void TechFileReader::skipComments() {
    while (pos_ < content_.size()) {
        skipWhitespace();
        
        if (pos_ + 1 < content_.size() && content_[pos_] == '/' && content_[pos_+1] == '*') {
            pos_ += 2;
            while (pos_ + 1 < content_.size()) {
                if (content_[pos_] == '*' && content_[pos_+1] == '/') {
                    pos_ += 2;
                    break;
                }
                pos_++;
            }
        } else if (pos_ + 1 < content_.size() && content_[pos_] == '/' && content_[pos_+1] == '/') {
            pos_ += 2;
            while (pos_ < content_.size() && content_[pos_] != '\n') {
                pos_++;
            }
        } else if (content_[pos_] == '#') {
            while (pos_ < content_.size() && content_[pos_] != '\n') {
                pos_++;
            }
        } else {
            break;
        }
    }
}

std::string TechFileReader::readIdentifier() {
    skipComments();
    std::string result;
    while (pos_ < content_.size() && 
           (std::isalnum(content_[pos_]) || content_[pos_] == '_' || content_[pos_] == '-')) {
        result += content_[pos_++];
    }
    return result;
}

std::string TechFileReader::readString() {
    skipComments();
    if (pos_ < content_.size() && content_[pos_] == '"') {
        pos_++;
        std::string result;
        while (pos_ < content_.size() && content_[pos_] != '"') {
            if (content_[pos_] == '\\' && pos_ + 1 < content_.size()) {
                pos_++;
                result += content_[pos_++];
            } else {
                result += content_[pos_++];
            }
        }
        if (pos_ < content_.size()) pos_++;
        return result;
    } else {
        return readIdentifier();
    }
}

double TechFileReader::readNumber() {
    skipComments();
    std::string numStr;
    
    if (pos_ < content_.size() && content_[pos_] == '-') {
        numStr += content_[pos_++];
    }
    
    while (pos_ < content_.size()) {
        char c = content_[pos_];
        if (std::isdigit(c) || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-') {
            numStr += c;
            pos_++;
        } else {
            break;
        }
    }
    
    return numStr.empty() ? 0.0 : utils::stringToDouble(numStr);
}

bool TechFileReader::expect(char c) {
    skipComments();
    if (pos_ < content_.size() && content_[pos_] == c) {
        pos_++;
        return true;
    }
    return false;
}

char TechFileReader::peek() {
    skipComments();
    return (pos_ < content_.size()) ? content_[pos_] : '\0';
}

std::string TechFileReader::readValue() {
    skipComments();
    if (peek() == '"') {
        return readString();
    } else if (std::isdigit(peek()) || peek() == '-') {
        double num = readNumber();
        return std::to_string(num);
    } else {
        return readIdentifier();
    }
}

std::string TechFileReader::extractBlock(const std::string& blockName) {
    size_t savedPos = pos_;
    pos_ = 0;
    
    size_t blockPos = content_.find(blockName + " {");
    if (blockPos == std::string::npos) {
        pos_ = savedPos;
        return "";
    }
    
    size_t start = blockPos + blockName.length() + 2;
    size_t braceCount = 1;
    size_t current = start;
    
    while (current < content_.size() && braceCount > 0) {
        if (content_[current] == '{') {
            braceCount++;
        } else if (content_[current] == '}') {
            braceCount--;
        }
        current++;
    }
    
    pos_ = savedPos;
    
    if (braceCount == 0) {
        return content_.substr(start, current - start - 1);
    }
    
    return "";
}

std::map<std::string, std::string> TechFileReader::parsePropertyBlock(const std::string& block) {
    std::map<std::string, std::string> properties;
    
    size_t savedPos = pos_;
    size_t savedContent = content_.size();
    
    content_ = block;
    pos_ = 0;
    
    while (peek() != '\0') {
        std::string property = readIdentifier();
        if (property.empty()) {
            if (pos_ < content_.size()) pos_++;
            continue;
        }
        
        if (!expect('=')) {
            continue;
        }
        
        std::string value = readValue();
        properties[property] = value;
    }
    
    pos_ = savedPos;
    content_.resize(savedContent);
    
    return properties;
}

TechnologyInfo TechFileReader::parseTechnology() {
    std::string techBlock = extractBlock("Technology");
    if (techBlock.empty()) {
        throw std::runtime_error("Technology block not found");
    }
    
    auto props = parsePropertyBlock(techBlock);
    
    TechnologyInfo tech;
    tech.name = parseStringProperty(props, "name", "unknown");
    tech.date = parseStringProperty(props, "date", "");
    tech.unitTimeName = parseStringProperty(props, "unitTimeName", "ns");
    tech.timePrecision = parseIntProperty(props, "timePrecision", 1000);
    tech.unitLengthName = parseStringProperty(props, "unitLengthName", "micron");
    tech.unitVoltageName = parseStringProperty(props, "unitVoltageName", "V");
    tech.voltagePrecision = parseIntProperty(props, "voltagePrecision", 1000000);
    tech.lengthPrecision = parseIntProperty(props, "lengthPrecision", 1000);
    tech.gridResolution = parseIntProperty(props, "gridResolution", 1);
    tech.dielectric = parseDoubleProperty(props, "dielectric", 3.45e-05);
    tech.minBaselineTemperature = parseIntProperty(props, "minBaselineTemperature", 25);
    tech.nomBaselineTemperature = parseIntProperty(props, "nomBaselineTemperature", 25);
    tech.maxBaselineTemperature = parseIntProperty(props, "maxBaselineTemperature", 25);
    
    return tech;
}

std::vector<ColorInfo> TechFileReader::parseColors() {
    std::vector<ColorInfo> colors;
    
    size_t pos = 0;
    while ((pos = content_.find("Color ", pos)) != std::string::npos) {
        size_t nameStart = pos + 6;
        size_t nameEnd = content_.find(" {", nameStart);
        
        if (nameEnd != std::string::npos) {
            ColorInfo color;
            color.name = content_.substr(nameStart, nameEnd - nameStart);
            
            size_t blockStart = nameEnd + 2;
            size_t blockEnd = content_.find("}", blockStart);
            if (blockEnd != std::string::npos) {
                std::string colorBlock = content_.substr(blockStart, blockEnd - blockStart);
                auto props = parsePropertyBlock(colorBlock);
                
                color.rgbDefined = parseIntProperty(props, "rgbDefined", 0);
                color.redIntensity = parseIntProperty(props, "redIntensity", 0);
                color.greenIntensity = parseIntProperty(props, "greenIntensity", 0);
                color.blueIntensity = parseIntProperty(props, "blueIntensity", 0);
                
                colors.push_back(color);
            }
        }
        
        pos = nameEnd != std::string::npos ? nameEnd : pos + 1;
    }
    
    return colors;
}

std::vector<LayerInfo> TechFileReader::parseLayers() {
    std::vector<LayerInfo> layers;
    return layers;
}

std::vector<ViaInfo> TechFileReader::parseVias() {
    std::vector<ViaInfo> vias;
    return vias;
}

std::string TechFileReader::parseStringProperty(const std::map<std::string, std::string>& props, 
                                              const std::string& key, const std::string& defaultValue) {
    auto it = props.find(key);
    if (it != props.end()) {
        return utils::trimString(it->second);
    }
    return defaultValue;
}

int TechFileReader::parseIntProperty(const std::map<std::string, std::string>& props, 
                                   const std::string& key, int defaultValue) {
    auto it = props.find(key);
    if (it != props.end()) {
        return utils::stringToInt(it->second);
    }
    return defaultValue;
}

double TechFileReader::parseDoubleProperty(const std::map<std::string, std::string>& props, 
                                         const std::string& key, double defaultValue) {
    auto it = props.find(key);
    if (it != props.end()) {
        return utils::stringToDouble(it->second);
    }
    return defaultValue;
}

} // namespace tfp
