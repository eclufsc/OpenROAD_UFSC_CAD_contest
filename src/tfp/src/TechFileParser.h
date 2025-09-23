#pragma once

#include "tfp/tfp.h"
#include <string>

namespace tfp {

class TechFileReader {
public:
    explicit TechFileReader(utl::Logger* logger);
    ~TechFileReader();
    
    TechFileData parseContent(const std::string& content);
    
private:
    utl::Logger* logger_;
    std::string content_;
    size_t pos_;
    
    // Parsing utilities
    void skipWhitespace();
    void skipComments();
    std::string readIdentifier();
    std::string readString();
    double readNumber();
    bool expect(char c);
    char peek();
    std::string readValue();
    
    // Block parsers
    std::string extractBlock(const std::string& blockName);
    std::map<std::string, std::string> parsePropertyBlock(const std::string& block);
    
    // Specific parsers
    TechnologyInfo parseTechnology();
    std::vector<ColorInfo> parseColors();
    std::vector<LayerInfo> parseLayers();
    std::vector<ViaInfo> parseVias();
    
    // Property parsers
    std::string parseStringProperty(const std::map<std::string, std::string>& props, 
                                   const std::string& key, const std::string& defaultValue = "");
    int parseIntProperty(const std::map<std::string, std::string>& props, 
                        const std::string& key, int defaultValue = 0);
    double parseDoubleProperty(const std::map<std::string, std::string>& props, 
                              const std::string& key, double defaultValue = 0.0);
};

} // namespace tfp
