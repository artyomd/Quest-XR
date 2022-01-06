#pragma once

#include <string>
#include <fmt/format.h>

#include "compiler.hpp"

#define LOG_FATAL(args...)                        \
  utils::logger::Log(utils::logger::Level::FATAL, \
      fmt::format("[FATAL] {}:{} {}",             \
                  __FILE__,                       \
                  __LINE__,                       \
                  fmt::format(args)));            \
  std::abort();
#define CHECK(condition, args...)                                       \
  if(UTILS_UNLIKELY(condition == false)){                                               \
    LOG_FATAL("Check failed: `" #condition "` {}", fmt::format(args));  \
  }

namespace utils::logger {
enum class Level { VERBOSE, DEBUG, INFO, WARNING, FATAL };
void Log(Level severity, const std::string &msg);
}
