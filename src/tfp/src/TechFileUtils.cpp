#include "TechFileUtils.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace tfp {
namespace utils {

std::string trimString(const std::string& str) {
    auto start = str.begin();
    auto end = str.end();
    
    while (start != end && std::isspace(*start)) {
        ++start;
    }
    
    while (start != end && std::isspace(*(end - 1))) {
        --end;
    }
    
    return std::string(start, end);
}

std::vector<std::string> splitString(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    
    while (std::getline(ss, item, delimiter)) {
        result.push_back(trimString(item));
    }
    
    return result;
}

bool isNumeric(const std::string& str) {
    if (str.empty()) return false;
    
    size_t start = 0;
    if (str[0] == '-' || str[0] == '+') {
        start = 1;
    }
    
    bool hasDecimal = false;
    bool hasExponent = false;
    
    for (size_t i = start; i < str.size(); ++i) {
        char c = str[i];
        
        if (std::isdigit(c)) {
            continue;
        } else if (c == '.' && !hasDecimal && !hasExponent) {
            hasDecimal = true;
        } else if ((c == 'e' || c == 'E') && !hasExponent && i > start) {
            hasExponent = true;
            if (i + 1 < str.size() && (str[i + 1] == '+' || str[i + 1] == '-')) {
                ++i;
            }
        } else {
            return false;
        }
    }
    
    return true;
}

double stringToDouble(const std::string& str) {
    try {
        return std::stod(trimString(str));
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid double: " + str);
    }
}

int stringToInt(const std::string& str) {
    try {
        return std::stoi(trimString(str));
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid integer: " + str);
    }
}

} // namespace utils
} // namespace tfp
