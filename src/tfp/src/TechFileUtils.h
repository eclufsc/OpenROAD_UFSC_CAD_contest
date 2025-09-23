#pragma once

#include <string>
#include <vector>

namespace tfp {
namespace utils {

std::string trimString(const std::string& str);
std::vector<std::string> splitString(const std::string& str, char delimiter);
bool isNumeric(const std::string& str);
double stringToDouble(const std::string& str);
int stringToInt(const std::string& str);

} // namespace utils
} // namespace tfp
