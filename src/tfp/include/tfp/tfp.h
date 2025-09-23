#pragma once

#include <string>
#include <vector>
#include <map>
#include "odb/db.h"
#include "utl/Logger.h"

namespace tfp {

struct TechnologyInfo {
    std::string name;
    std::string date;
    std::string unitTimeName;
    int timePrecision;
    std::string unitLengthName;
    std::string unitVoltageName;
    int voltagePrecision;
    std::string unitCurrentName;
    int currentPrecision;
    std::string unitPowerName;
    int powerPrecision;
    std::string unitResistanceName;
    int resistancePrecision;
    std::string unitCapacitanceName;
    int capacitancePrecision;
    std::string unitInductanceName;
    int inductancePrecision;
    int minEdgeMode;
    double dielectric;
    int lengthPrecision;
    int gridResolution;
    int minBaselineTemperature;
    int nomBaselineTemperature;
    int maxBaselineTemperature;
};

struct ColorInfo {
    std::string name;
    int rgbDefined;
    int redIntensity;
    int greenIntensity;
    int blueIntensity;
};

struct LayerInfo {
    std::string name;
    int layerNumber;
    std::string maskName;
    int visible;
    int selectable;
    int blink;
    std::string color;
    std::string lineStyle;
    std::string pattern;
    double pitch;
    double defaultWidth;
    double minWidth;
    double minSpacing;
    double maxWidth;
    double minArea;
    double minEnclosedArea;
    int isDefaultLayer;
    int onWireTrack;
    int numMasks;
    double sameNetMinSpacing;
    int nonPreferredRouteMode;
    int hasRectangleOnly;
    double maxCurrDensity;
    
    int fatTblDimension;
    std::vector<double> fatTblThreshold;
    int fatTblParallelLengthDimension;
    std::vector<double> fatTblParallelLength;
    std::vector<double> fatTblSpacing;
    
    int protrusionTblDim;
    std::vector<double> protrusionFatThresholdTbl;
    std::vector<double> protrusionLengthLimitTbl;
    std::vector<double> protrusionMinWidthTbl;
    
    int cutTblSize;
    std::vector<std::string> cutNameTbl;
    std::vector<double> cutWidthTbl;
    std::vector<double> cutHeightTbl;
    
    int fatTblDimension2;
    std::vector<double> fatTblThreshold2;
    double sameSegAlignedUpperWireMaxSpacingThreshold;
    double sameSegAlignedLowerWireMaxSpacingThreshold;
    double sameSegAlignedCutMinSpacing;
    double minEnclosedWidth;
};

struct ViaInfo {
    std::string name;
    int contactCodeNumber;
    int contactSourceType;
    double cutHeight;
    double cutWidth;
    std::string cutLayer;
    std::string upperLayer;
    std::string lowerLayer;
    double minCutSpacing;
    int minNumCols;
    int minNumRows;
    int maxNumRowsNonTurning;
    int excludedForSignalRoute;
    int isDefaultContact;
    double lowerLayerEncHeight;
    double lowerLayerEncWidth;
    double upperLayerEncHeight;
    double upperLayerEncWidth;
    int nonRotatable;
    double unitMinResistance;
    double unitNomResistance;
    double unitMaxResistance;
};

struct StippleInfo {
    std::string name;
    int height;
    int width;
    std::vector<int> pattern;
};

struct TileInfo {
    std::string name;
    double width;
    double height;
};

struct DesignRule {
    std::string layer1;
    std::string layer2;
    double minSpacing;
    double minEnclosure;
    int stackable;
    
    int cut1TblSize;
    int cut2TblSize;
    std::vector<std::string> cut1NameTbl;
    std::vector<std::string> cut2NameTbl;
    std::vector<double> sameNetXMinSpacingTbl;
    std::vector<double> diffNetXMinSpacingTbl;
    
    int endOfLineEncTblSize;
    double endOfLineEncWidthThreshold;
    std::vector<double> endOfLineEncSideThreshold;
    std::vector<double> endOfLineEncTbl;
};

struct DensityRule {
    std::string layer;
    double windowSize;
    double minDensity;
    double maxDensity;
};

struct PRRule {
    double rowSpacingTopTop;
    double rowSpacingTopBot;
    double rowSpacingBotBot;
    int abuttableTopTop;
    int abuttableTopBot;
    int abuttableBotBot;
};

struct LayerDataType {
    std::string name;
    int layerNumber;
    int dataTypeNumber;
    int nonMask;
    int visible;
    int selectable;
    int blink;
    std::string color;
    std::string lineStyle;
    std::string pattern;
};

struct TechFileData {
    TechnologyInfo technology;
    std::vector<ColorInfo> colors;
    std::vector<StippleInfo> stipples;
    std::vector<TileInfo> tiles;
    std::vector<LayerInfo> layers;
    std::vector<ViaInfo> vias;
    std::vector<DesignRule> designRules;
    std::vector<DensityRule> densityRules;
    std::vector<PRRule> prRules;
    std::vector<LayerDataType> layerDataTypes;
};

class TechFileParser {
public:
    explicit TechFileParser(utl::Logger* logger = nullptr);
    ~TechFileParser();
    
    void readTechFile(odb::dbDatabase* db, const std::string& filename);
    
    TechFileData parseTechFile(const std::string& filename);

private:
    utl::Logger* logger_;
    std::string content_;
    size_t pos_;
    
    void skipWhitespace();
    void skipComments();
    std::string readIdentifier();
    std::string readString();
    double readNumber();
    bool expect(char c);
    char peek();
    std::string readValue();
    
    std::string extractBlock(const std::string& blockName);
    std::map<std::string, std::string> parsePropertyBlock(const std::string& block);
    
    TechnologyInfo parseTechnology();
    std::vector<ColorInfo> parseColors();
    std::vector<StippleInfo> parseStipples();
    std::vector<TileInfo> parseTiles();
    std::vector<LayerInfo> parseLayers();
    std::vector<ViaInfo> parseVias();
    std::vector<DesignRule> parseDesignRules();
    std::vector<DensityRule> parseDensityRules();
    std::vector<PRRule> parsePRRules();
    std::vector<LayerDataType> parseLayerDataTypes();
    
    std::string parseStringProperty(const std::map<std::string, std::string>& props, 
                                   const std::string& key, const std::string& defaultValue);
    int parseIntProperty(const std::map<std::string, std::string>& props, 
                        const std::string& key, int defaultValue);
    double parseDoubleProperty(const std::map<std::string, std::string>& props, 
                              const std::string& key, double defaultValue);
    std::vector<double> parseDoubleArrayProperty(const std::map<std::string, std::string>& props,
                                                const std::string& key);
    std::vector<int> parseIntArrayProperty(const std::map<std::string, std::string>& props,
                                          const std::string& key);
    std::vector<std::string> parseStringArrayProperty(const std::map<std::string, std::string>& props,
                                                     const std::string& key);
};

} // namespace tfp
