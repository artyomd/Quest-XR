#pragma once

#include <string>
namespace utils::logger {
enum class Level { VERBOSE, DEBUG, INFO, WARNING, FATAL };
void Log(Level severity, const std::string &msg);
}
