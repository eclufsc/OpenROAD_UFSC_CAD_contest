#include "../include/tfp/tfp.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>

namespace tfp {

TechFileParser::TechFileParser(utl::Logger* logger) : logger_(logger), pos_(0) {}

TechFileParser::~TechFileParser() {}

void TechFileParser::readTechFile(odb::dbDatabase* db, const std::string& filename) {
    TechFileData data = parseTechFile(filename);
}

TechFileData TechFileParser::parseTechFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    content_ = std::string((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
    file.close();
    pos_ = 0;
    
    TechFileData data;
    
    try {
        data.technology = parseTechnology();
        data.colors = parseColors();
        data.stipples = parseStipples();
        data.tiles = parseTiles();
        data.layers = parseLayers();
        data.vias = parseVias();
        data.designRules = parseDesignRules();
        data.densityRules = parseDensityRules();
        data.prRules = parsePRRules();
        data.layerDataTypes = parseLayerDataTypes();
        
    } catch (const std::exception& e) {
        if (logger_) {
            logger_->error(utl::ODB, 103, "Parse error at {}: {}", pos_, e.what());
        }
        throw;
    }
    
    return data;
}

void TechFileParser::skipWhitespace() {
    while (pos_ < content_.size() && std::isspace(content_[pos_])) {
        pos_++;
    }
}

void TechFileParser::skipComments() {
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
        } else {
            break;
        }
    }
}

std::string TechFileParser::readIdentifier() {
    skipComments();
    std::string result;
    
    if (pos_ < content_.size() && (std::isalpha(content_[pos_]) || content_[pos_] == '_')) {
        while (pos_ < content_.size() && 
               (std::isalnum(content_[pos_]) || content_[pos_] == '_' || content_[pos_] == '-')) {
            result += content_[pos_++];
        }
    }
    return result;
}

std::string TechFileParser::readString() {
    skipComments();
    if (pos_ < content_.size() && content_[pos_] == '"') {
        pos_++;
        std::string result;
        while (pos_ < content_.size() && content_[pos_] != '"') {
            if (content_[pos_] == '\\' && pos_ + 1 < content_.size()) {
                pos_++;
                switch (content_[pos_]) {
                    case 'n': result += '\n'; break;
                    case 't': result += '\t'; break;
                    case 'r': result += '\r'; break;
                    case '\\': result += '\\'; break;
                    case '"': result += '"'; break;
                    default: result += content_[pos_]; break;
                }
            } else {
                result += content_[pos_];
            }
            pos_++;
        }
        if (pos_ < content_.size()) pos_++;
        return result;
    } else {
        return readIdentifier();
    }
}

double TechFileParser::readNumber() {
    skipComments();
    std::string numStr;
    
    if (pos_ < content_.size() && content_[pos_] == '-') {
        numStr += content_[pos_++];
    }
    
    while (pos_ < content_.size()) {
        char c = content_[pos_];
        if (std::isdigit(c) || c == '.') {
            numStr += c;
            pos_++;
        } else if ((c == 'e' || c == 'E') && !numStr.empty()) {
            numStr += c;
            pos_++;
            if (pos_ < content_.size() && (content_[pos_] == '+' || content_[pos_] == '-')) {
                numStr += content_[pos_++];
            }
        } else {
            break;
        }
    }
    
    if (numStr.empty() || numStr == "-") return 0.0;
    
    try {
        return std::stod(numStr);
    } catch (const std::exception&) {
        if (logger_) {
            logger_->warn(utl::ODB, 104, "Invalid number: {}", numStr);
        }
        return 0.0;
    }
}

bool TechFileParser::expect(char c) {
    skipComments();
    if (pos_ < content_.size() && content_[pos_] == c) {
        pos_++;
        return true;
    }
    return false;
}

char TechFileParser::peek() {
    skipComments();
    return (pos_ < content_.size()) ? content_[pos_] : '\0';
}

std::string TechFileParser::readValue() {
    skipComments();
    if (peek() == '"') {
        return readString();
    } else if (std::isdigit(peek()) || peek() == '-' || peek() == '.') {
        double num = readNumber();
        return std::to_string(num);
    } else if (peek() == '(') {
        std::string result;
        while (pos_ < content_.size() && content_[pos_] != ')') {
            result += content_[pos_++];
        }
        if (pos_ < content_.size()) result += content_[pos_++];
        return result;
    } else {
        return readIdentifier();
    }
}

std::string TechFileParser::extractBlock(const std::string& blockName) {
    size_t savedPos = pos_;
    
    std::string pattern = blockName + " {";
    size_t blockPos = content_.find(pattern);
    if (blockPos == std::string::npos) {
        pattern = blockName + "{";
        blockPos = content_.find(pattern);
        if (blockPos == std::string::npos) {
            pos_ = savedPos;
            return "";
        }
    }
    
    size_t start = blockPos + pattern.length();
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

std::map<std::string, std::string> TechFileParser::parsePropertyBlock(const std::string& block) {
    std::map<std::string, std::string> properties;
    
    size_t savedPos = pos_;
    std::string savedContent = content_;
    
    content_ = block;
    pos_ = 0;
    
    while (peek() != '\0') {
        std::string property = readIdentifier();
        if (property.empty()) {
            if (pos_ < content_.size()) pos_++;
            continue;
        }
        
        skipComments();
        if (!expect('=')) {
            while (pos_ < content_.size() && content_[pos_] != '=' && !std::isalpha(content_[pos_])) {
                pos_++;
            }
            continue;
        }
        
        std::string value = readValue();
        properties[property] = value;
        
        skipComments();
        if (peek() == ';') {
            pos_++;
        }
    }
    
    content_ = savedContent;
    pos_ = savedPos;
    
    return properties;
}

TechnologyInfo TechFileParser::parseTechnology() {
    std::string techBlock = extractBlock("Technology");
    if (techBlock.empty()) {
        throw std::runtime_error("Technology block not found");
    }
    
    auto props = parsePropertyBlock(techBlock);
    
    TechnologyInfo tech;
    tech.name = parseStringProperty(props, "name", "unknown");
    tech.date = parseStringProperty(props, "date", "");
    tech.lengthPrecision = parseIntProperty(props, "lengthPrecision", 1000);
    tech.gridResolution = parseIntProperty(props, "gridResolution", 1);
    tech.dielectric = parseDoubleProperty(props, "dielectric", 3.45e-05);
    tech.minBaselineTemperature = parseIntProperty(props, "minBaselineTemperature", 25);
    tech.nomBaselineTemperature = parseIntProperty(props, "nomBaselineTemperature", 25);
    tech.maxBaselineTemperature = parseIntProperty(props, "maxBaselineTemperature", 25);
    
    tech.unitTimeName = parseStringProperty(props, "unitTimeName", "ns");
    tech.timePrecision = parseIntProperty(props, "timePrecision", 1000);
    tech.unitLengthName = parseStringProperty(props, "unitLengthName", "micron");
    tech.unitVoltageName = parseStringProperty(props, "unitVoltageName", "V");
    tech.voltagePrecision = parseIntProperty(props, "voltagePrecision", 1000000);
    tech.unitCurrentName = parseStringProperty(props, "unitCurrentName", "ua");
    tech.currentPrecision = parseIntProperty(props, "currentPrecision", 1);
    tech.unitPowerName = parseStringProperty(props, "unitPowerName", "mw");
    tech.powerPrecision = parseIntProperty(props, "powerPrecision", 1000);
    tech.unitResistanceName = parseStringProperty(props, "unitResistanceName", "Mohm");
    tech.resistancePrecision = parseIntProperty(props, "resistancePrecision", 10000000);
    tech.unitCapacitanceName = parseStringProperty(props, "unitCapacitanceName", "pf");
    tech.capacitancePrecision = parseIntProperty(props, "capacitancePrecision", 10000000);
    tech.unitInductanceName = parseStringProperty(props, "unitInductanceName", "nh");
    tech.inductancePrecision = parseIntProperty(props, "inductancePrecision", 100);
    tech.minEdgeMode = parseIntProperty(props, "minEdgeMode", 1);
    
    return tech;
}

std::vector<ColorInfo> TechFileParser::parseColors() {
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

std::vector<LayerInfo> TechFileParser::parseLayers() {
    std::vector<LayerInfo> layers;
    
    size_t pos = 0;
    while ((pos = content_.find("Layer \"", pos)) != std::string::npos) {
        size_t nameStart = pos + 7;
        size_t nameEnd = content_.find("\"", nameStart);
        
        if (nameEnd != std::string::npos) {
            LayerInfo layer;
            layer.name = content_.substr(nameStart, nameEnd - nameStart);
            
            size_t blockStart = content_.find("{", nameEnd);
            if (blockStart != std::string::npos) {
                blockStart++;
                
                size_t braceCount = 1;
                size_t current = blockStart;
                while (current < content_.size() && braceCount > 0) {
                    if (content_[current] == '{') {
                        braceCount++;
                    } else if (content_[current] == '}') {
                        braceCount--;
                    }
                    current++;
                }
                
                if (braceCount == 0) {
                    std::string layerBlock = content_.substr(blockStart, current - blockStart - 1);
                    auto props = parsePropertyBlock(layerBlock);
                    
                    layer.layerNumber = parseIntProperty(props, "layerNumber", 0);
                    layer.maskName = parseStringProperty(props, "maskName", "");
                    layer.visible = parseIntProperty(props, "visible", 1);
                    layer.selectable = parseIntProperty(props, "selectable", 1);
                    layer.blink = parseIntProperty(props, "blink", 0);
                    layer.color = parseStringProperty(props, "color", "white");
                    layer.lineStyle = parseStringProperty(props, "lineStyle", "solid");
                    layer.pattern = parseStringProperty(props, "pattern", "solid");
                    layer.pitch = parseDoubleProperty(props, "pitch", 0.0);
                    layer.defaultWidth = parseDoubleProperty(props, "defaultWidth", 0.0);
                    layer.minWidth = parseDoubleProperty(props, "minWidth", 0.0);
                    layer.minSpacing = parseDoubleProperty(props, "minSpacing", 0.0);
                    layer.maxWidth = parseDoubleProperty(props, "maxWidth", 0.0);
                    layer.minArea = parseDoubleProperty(props, "minArea", 0.0);
                    layer.minEnclosedArea = parseDoubleProperty(props, "minEnclosedArea", 0.0);
                    layer.isDefaultLayer = parseIntProperty(props, "isDefaultLayer", 0);
                    layer.onWireTrack = parseIntProperty(props, "onWireTrack", 0);
                    layer.numMasks = parseIntProperty(props, "numMasks", 1);
                    
                    layer.sameNetMinSpacing = parseDoubleProperty(props, "sameNetMinSpacing", 0.0);
                    layer.nonPreferredRouteMode = parseIntProperty(props, "nonPreferredRouteMode", 0);
                    layer.hasRectangleOnly = parseIntProperty(props, "hasRectangleOnly", 0);
                    layer.maxCurrDensity = parseDoubleProperty(props, "maxCurrDensity", 0.0);
                    layer.minEnclosedWidth = parseDoubleProperty(props, "minEnclosedWidth", 0.0);
                    
                    layer.fatTblDimension = parseIntProperty(props, "fatTblDimension", 0);
                    layer.fatTblThreshold = parseDoubleArrayProperty(props, "fatTblThreshold");
                    layer.fatTblParallelLengthDimension = parseIntProperty(props, "fatTblParallelLengthDimension", 0);
                    layer.fatTblParallelLength = parseDoubleArrayProperty(props, "fatTblParallelLength");
                    layer.fatTblSpacing = parseDoubleArrayProperty(props, "fatTblSpacing");
                    
                    layer.protrusionTblDim = parseIntProperty(props, "protrusionTblDim", 0);
                    layer.protrusionFatThresholdTbl = parseDoubleArrayProperty(props, "protrusionFatThresholdTbl");
                    layer.protrusionLengthLimitTbl = parseDoubleArrayProperty(props, "protrusionLengthLimitTbl");
                    layer.protrusionMinWidthTbl = parseDoubleArrayProperty(props, "protrusionMinWidthTbl");
                    
                    layer.cutTblSize = parseIntProperty(props, "cutTblSize", 0);
                    layer.cutNameTbl = parseStringArrayProperty(props, "cutNameTbl");
                    layer.cutWidthTbl = parseDoubleArrayProperty(props, "cutWidthTbl");
                    layer.cutHeightTbl = parseDoubleArrayProperty(props, "cutHeightTbl");
                    
                    layer.fatTblDimension2 = parseIntProperty(props, "fatTblDimension2", 0);
                    layer.fatTblThreshold2 = parseDoubleArrayProperty(props, "fatTblThreshold2");
                    layer.sameSegAlignedUpperWireMaxSpacingThreshold = parseDoubleProperty(props, "sameSegAlignedUpperWireMaxSpacingThreshold", 0.0);
                    layer.sameSegAlignedLowerWireMaxSpacingThreshold = parseDoubleProperty(props, "sameSegAlignedLowerWireMaxSpacingThreshold", 0.0);
                    layer.sameSegAlignedCutMinSpacing = parseDoubleProperty(props, "sameSegAlignedCutMinSpacing", 0.0);
                    
                    layers.push_back(layer);
                }
            }
        }
        
        pos = nameEnd != std::string::npos ? nameEnd : pos + 1;
    }
    
    return layers;
}

std::vector<ViaInfo> TechFileParser::parseVias() {
    std::vector<ViaInfo> vias;
    
    size_t pos = 0;
    while ((pos = content_.find("ContactCode \"", pos)) != std::string::npos) {
        size_t nameStart = pos + 13;
        size_t nameEnd = content_.find("\"", nameStart);
        
        if (nameEnd != std::string::npos) {
            ViaInfo via;
            via.name = content_.substr(nameStart, nameEnd - nameStart);
            
            size_t blockStart = content_.find("{", nameEnd);
            if (blockStart != std::string::npos) {
                blockStart++;
                
                size_t braceCount = 1;
                size_t current = blockStart;
                while (current < content_.size() && braceCount > 0) {
                    if (content_[current] == '{') {
                        braceCount++;
                    } else if (content_[current] == '}') {
                        braceCount--;
                    }
                    current++;
                }
                
                if (braceCount == 0) {
                    std::string viaBlock = content_.substr(blockStart, current - blockStart - 1);
                    auto props = parsePropertyBlock(viaBlock);
                    
                    via.contactCodeNumber = parseIntProperty(props, "contactCodeNumber", 0);
                    via.contactSourceType = parseIntProperty(props, "contactSourceType", 0);
                    via.cutHeight = parseDoubleProperty(props, "cutHeight", 0.0);
                    via.cutWidth = parseDoubleProperty(props, "cutWidth", 0.0);
                    via.cutLayer = parseStringProperty(props, "cutLayer", "");
                    via.upperLayer = parseStringProperty(props, "upperLayer", "");
                    via.lowerLayer = parseStringProperty(props, "lowerLayer", "");
                    via.minCutSpacing = parseDoubleProperty(props, "minCutSpacing", 0.0);
                    via.minNumCols = parseIntProperty(props, "minNumCols", 1);
                    via.minNumRows = parseIntProperty(props, "minNumRows", 1);
                    via.maxNumRowsNonTurning = parseIntProperty(props, "maxNumRowsNonTurning", 4);
                    via.excludedForSignalRoute = parseIntProperty(props, "excludedForSignalRoute", 0);
                    via.isDefaultContact = parseIntProperty(props, "isDefaultContact", 0);
                    via.lowerLayerEncHeight = parseDoubleProperty(props, "lowerLayerEncHeight", 0.0);
                    via.lowerLayerEncWidth = parseDoubleProperty(props, "lowerLayerEncWidth", 0.0);
                    via.upperLayerEncHeight = parseDoubleProperty(props, "upperLayerEncHeight", 0.0);
                    via.upperLayerEncWidth = parseDoubleProperty(props, "upperLayerEncWidth", 0.0);
                    via.nonRotatable = parseIntProperty(props, "nonRotatable", 0);
                    via.unitMinResistance = parseDoubleProperty(props, "unitMinResistance", 0.0);
                    via.unitNomResistance = parseDoubleProperty(props, "unitNomResistance", 0.0);
                    via.unitMaxResistance = parseDoubleProperty(props, "unitMaxResistance", 0.0);
                    
                    vias.push_back(via);
                }
            }
        }
        
        pos = nameEnd != std::string::npos ? nameEnd : pos + 1;
    }
    
    return vias;
}

std::string TechFileParser::parseStringProperty(const std::map<std::string, std::string>& props, 
                                              const std::string& key, const std::string& defaultValue) {
    auto it = props.find(key);
    if (it != props.end()) {
        std::string value = it->second;
        if (!value.empty() && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.length() - 2);
        }
        return value;
    }
    return defaultValue;
}

int TechFileParser::parseIntProperty(const std::map<std::string, std::string>& props, 
                                   const std::string& key, int defaultValue) {
    auto it = props.find(key);
    if (it != props.end()) {
        try {
            double val = std::stod(it->second);
            return static_cast<int>(val);
        } catch (const std::exception&) {
            if (logger_) {
                logger_->warn(utl::ODB, 105, "Invalid int {}: {}", key, it->second);
            }
            return defaultValue;
        }
    }
    return defaultValue;
}

double TechFileParser::parseDoubleProperty(const std::map<std::string, std::string>& props, 
                                         const std::string& key, double defaultValue) {
    auto it = props.find(key);
    if (it != props.end()) {
        try {
            return std::stod(it->second);
        } catch (const std::exception&) {
            if (logger_) {
                logger_->warn(utl::ODB, 106, "Invalid double {}: {}", key, it->second);
            }
            return defaultValue;
        }
    }
    return defaultValue;
}

std::vector<double> TechFileParser::parseDoubleArrayProperty(const std::map<std::string, std::string>& props,
                                                        const std::string& key) {
    std::vector<double> result;
    auto it = props.find(key);
    if (it != props.end()) {
        std::string value = it->second;
        
        if (!value.empty() && value.front() == '(' && value.back() == ')') {
            value = value.substr(1, value.length() - 2);
        }
        
        std::stringstream ss(value);
        std::string item;
        while (std::getline(ss, item, ',')) {
            item.erase(0, item.find_first_not_of(" \t\n\r"));
            item.erase(item.find_last_not_of(" \t\n\r") + 1);
            
            if (!item.empty()) {
                try {
                    result.push_back(std::stod(item));
                } catch (const std::exception&) {
                    if (logger_) {
                        logger_->warn(utl::ODB, 107, "Invalid double in array {}: {}", key, item);
                    }
                }
            }
        }
    }
    return result;
}

std::vector<int> TechFileParser::parseIntArrayProperty(const std::map<std::string, std::string>& props,
                                                      const std::string& key) {
    std::vector<int> result;
    auto it = props.find(key);
    if (it != props.end()) {
        std::string value = it->second;
        
        if (!value.empty() && value.front() == '(' && value.back() == ')') {
            value = value.substr(1, value.length() - 2);
        }
        
        std::stringstream ss(value);
        std::string item;
        while (std::getline(ss, item, ',')) {
            item.erase(0, item.find_first_not_of(" \t\n\r"));
            item.erase(item.find_last_not_of(" \t\n\r") + 1);
            
            if (!item.empty()) {
                try {
                    result.push_back(std::stoi(item));
                } catch (const std::exception&) {
                    if (logger_) {
                        logger_->warn(utl::ODB, 108, "Invalid int in array {}: {}", key, item);
                    }
                }
            }
        }
    }
    return result;
}

std::vector<std::string> TechFileParser::parseStringArrayProperty(const std::map<std::string, std::string>& props,
                                                                 const std::string& key) {
    std::vector<std::string> result;
    auto it = props.find(key);
    if (it != props.end()) {
        std::string value = it->second;
        
        if (!value.empty() && value.front() == '(' && value.back() == ')') {
            value = value.substr(1, value.length() - 2);
        }
        
        std::stringstream ss(value);
        std::string item;
        while (std::getline(ss, item, ',')) {
            item.erase(0, item.find_first_not_of(" \t\n\r"));
            item.erase(item.find_last_not_of(" \t\n\r") + 1);
            
            if (!item.empty() && item.front() == '"' && item.back() == '"') {
                item = item.substr(1, item.length() - 2);
            }
            
            if (!item.empty()) {
                result.push_back(item);
            }
        }
    }
    return result;
}

std::vector<StippleInfo> TechFileParser::parseStipples() {
   std::vector<StippleInfo> stipples;
   
   size_t pos = 0;
   while ((pos = content_.find("Stipple \"", pos)) != std::string::npos) {
       size_t nameStart = pos + 9;
       size_t nameEnd = content_.find("\"", nameStart);
       
       if (nameEnd != std::string::npos) {
           StippleInfo stipple;
           stipple.name = content_.substr(nameStart, nameEnd - nameStart);
           
           size_t blockStart = content_.find("{", nameEnd);
           if (blockStart != std::string::npos) {
               blockStart++;
               
               size_t braceCount = 1;
               size_t current = blockStart;
               while (current < content_.size() && braceCount > 0) {
                   if (content_[current] == '{') {
                       braceCount++;
                   } else if (content_[current] == '}') {
                       braceCount--;
                   }
                   current++;
               }
               
               if (braceCount == 0) {
                   std::string stippleBlock = content_.substr(blockStart, current - blockStart - 1);
                   auto props = parsePropertyBlock(stippleBlock);
                   
                   stipple.height = parseIntProperty(props, "height", 16);
                   stipple.width = parseIntProperty(props, "width", 16);
                   stipple.pattern = parseIntArrayProperty(props, "pattern");
                   
                   stipples.push_back(stipple);
               }
           }
       }
       
       pos = nameEnd != std::string::npos ? nameEnd : pos + 1;
   }
   
   return stipples;
}

std::vector<TileInfo> TechFileParser::parseTiles() {
   std::vector<TileInfo> tiles;
   
   size_t pos = 0;
   while ((pos = content_.find("Tile \"", pos)) != std::string::npos) {
       size_t nameStart = pos + 6;
       size_t nameEnd = content_.find("\"", nameStart);
       
       if (nameEnd != std::string::npos) {
           TileInfo tile;
           tile.name = content_.substr(nameStart, nameEnd - nameStart);
           
           size_t blockStart = content_.find("{", nameEnd);
           if (blockStart != std::string::npos) {
               blockStart++;
               
               size_t braceCount = 1;
               size_t current = blockStart;
               while (current < content_.size() && braceCount > 0) {
                   if (content_[current] == '{') {
                       braceCount++;
                   } else if (content_[current] == '}') {
                       braceCount--;
                   }
                   current++;
               }
               
               if (braceCount == 0) {
                   std::string tileBlock = content_.substr(blockStart, current - blockStart - 1);
                   auto props = parsePropertyBlock(tileBlock);
                   
                   tile.width = parseDoubleProperty(props, "width", 0.0);
                   tile.height = parseDoubleProperty(props, "height", 0.0);
                   
                   tiles.push_back(tile);
               }
           }
       }
       
       pos = nameEnd != std::string::npos ? nameEnd : pos + 1;
   }
   
   return tiles;
}

std::vector<DesignRule> TechFileParser::parseDesignRules() {
   std::vector<DesignRule> rules;
   
   size_t pos = 0;
   while ((pos = content_.find("DesignRule {", pos)) != std::string::npos) {
       size_t blockStart = pos + 12;
       
       size_t braceCount = 1;
       size_t current = blockStart;
       while (current < content_.size() && braceCount > 0) {
           if (content_[current] == '{') {
               braceCount++;
           } else if (content_[current] == '}') {
               braceCount--;
           }
           current++;
       }
       
       if (braceCount == 0) {
           DesignRule rule;
           std::string ruleBlock = content_.substr(blockStart, current - blockStart - 1);
           auto props = parsePropertyBlock(ruleBlock);
           
           rule.layer1 = parseStringProperty(props, "layer1", "");
           rule.layer2 = parseStringProperty(props, "layer2", "");
           rule.minSpacing = parseDoubleProperty(props, "minSpacing", 0.0);
           rule.minEnclosure = parseDoubleProperty(props, "minEnclosure", 0.0);
           rule.stackable = parseIntProperty(props, "stackable", 0);
           
           rule.cut1TblSize = parseIntProperty(props, "cut1TblSize", 0);
           rule.cut2TblSize = parseIntProperty(props, "cut2TblSize", 0);
           rule.cut1NameTbl = parseStringArrayProperty(props, "cut1NameTbl");
           rule.cut2NameTbl = parseStringArrayProperty(props, "cut2NameTbl");
           rule.sameNetXMinSpacingTbl = parseDoubleArrayProperty(props, "sameNetXMinSpacingTbl");
           rule.diffNetXMinSpacingTbl = parseDoubleArrayProperty(props, "diffNetXMinSpacingTbl");
           
           rule.endOfLineEncTblSize = parseIntProperty(props, "endOfLineEncTblSize", 0);
           rule.endOfLineEncWidthThreshold = parseDoubleProperty(props, "endOfLineEncWidthThreshold", 0.0);
           rule.endOfLineEncSideThreshold = parseDoubleArrayProperty(props, "endOfLineEncSideThreshold");
           rule.endOfLineEncTbl = parseDoubleArrayProperty(props, "endOfLineEncTbl");
           
           rules.push_back(rule);
       }
       
       pos = current;
   }
   
   return rules;
}

std::vector<DensityRule> TechFileParser::parseDensityRules() {
   std::vector<DensityRule> rules;
   
   size_t pos = 0;
   while ((pos = content_.find("DensityRule {", pos)) != std::string::npos) {
       size_t blockStart = pos + 13;
       
       size_t braceCount = 1;
       size_t current = blockStart;
       while (current < content_.size() && braceCount > 0) {
           if (content_[current] == '{') {
               braceCount++;
           } else if (content_[current] == '}') {
               braceCount--;
           }
           current++;
       }
       
       if (braceCount == 0) {
           DensityRule rule;
           std::string ruleBlock = content_.substr(blockStart, current - blockStart - 1);
           auto props = parsePropertyBlock(ruleBlock);
           
           rule.layer = parseStringProperty(props, "layer", "");
           rule.windowSize = parseDoubleProperty(props, "windowSize", 0.0);
           rule.minDensity = parseDoubleProperty(props, "minDensity", 0.0);
           rule.maxDensity = parseDoubleProperty(props, "maxDensity", 100.0);
           
           rules.push_back(rule);
       }
       
       pos = current;
   }
   
   return rules;
}

std::vector<PRRule> TechFileParser::parsePRRules() {
   std::vector<PRRule> rules;
   
   size_t pos = 0;
   while ((pos = content_.find("PRRule {", pos)) != std::string::npos) {
       size_t blockStart = pos + 8;
       
       size_t braceCount = 1;
       size_t current = blockStart;
       while (current < content_.size() && braceCount > 0) {
           if (content_[current] == '{') {
               braceCount++;
           } else if (content_[current] == '}') {
               braceCount--;
           }
           current++;
       }
       
       if (braceCount == 0) {
           PRRule rule;
           std::string ruleBlock = content_.substr(blockStart, current - blockStart - 1);
           auto props = parsePropertyBlock(ruleBlock);
           
           rule.rowSpacingTopTop = parseDoubleProperty(props, "rowSpacingTopTop", 0.0);
           rule.rowSpacingTopBot = parseDoubleProperty(props, "rowSpacingTopBot", 0.0);
           rule.rowSpacingBotBot = parseDoubleProperty(props, "rowSpacingBotBot", 0.0);
           rule.abuttableTopTop = parseIntProperty(props, "abuttableTopTop", 0);
           rule.abuttableTopBot = parseIntProperty(props, "abuttableTopBot", 0);
           rule.abuttableBotBot = parseIntProperty(props, "abuttableBotBot", 0);
           
           rules.push_back(rule);
       }
       
       pos = current;
   }
   
   return rules;
}

std::vector<LayerDataType> TechFileParser::parseLayerDataTypes() {
   std::vector<LayerDataType> types;
   
   size_t pos = 0;
   while ((pos = content_.find("LayerDataType \"", pos)) != std::string::npos) {
       size_t nameStart = pos + 15;
       size_t nameEnd = content_.find("\"", nameStart);
       
       if (nameEnd != std::string::npos) {
           LayerDataType type;
           type.name = content_.substr(nameStart, nameEnd - nameStart);
           
           size_t blockStart = content_.find("{", nameEnd);
           if (blockStart != std::string::npos) {
               blockStart++;
               
               size_t braceCount = 1;
               size_t current = blockStart;
               while (current < content_.size() && braceCount > 0) {
                   if (content_[current] == '{') {
                       braceCount++;
                   } else if (content_[current] == '}') {
                       braceCount--;
                   }
                   current++;
               }
               
               if (braceCount == 0) {
                   std::string typeBlock = content_.substr(blockStart, current - blockStart - 1);
                   auto props = parsePropertyBlock(typeBlock);
                   
                   type.layerNumber = parseIntProperty(props, "layerNumber", 0);
                   type.dataTypeNumber = parseIntProperty(props, "dataTypeNumber", 0);
                   type.nonMask = parseIntProperty(props, "nonMask", 0);
                   type.visible = parseIntProperty(props, "visible", 1);
                   type.selectable = parseIntProperty(props, "selectable", 1);
                   type.blink = parseIntProperty(props, "blink", 0);
                   type.color = parseStringProperty(props, "color", "white");
                     type.lineStyle = parseStringProperty(props, "lineStyle", "solid");
                   type.pattern = parseStringProperty(props, "pattern", "solid");
                   
                   types.push_back(type);
               }
           }
       }
       
       pos = nameEnd != std::string::npos ? nameEnd : pos + 1;
   }
   
   return types;
}

} // namespace tfp
