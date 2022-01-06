#include  "logger.hpp"

#include <android/log.h>
#include <map>

void utils::logger::Log(Level severity, const std::string &msg) {
  static std::map<Level, int> android_severity_mapping = {
      {Level::VERBOSE, ANDROID_LOG_VERBOSE},
      {Level::DEBUG, ANDROID_LOG_DEBUG},
      {Level::INFO, ANDROID_LOG_INFO},
      {Level::WARNING, ANDROID_LOG_WARN},
      {Level::FATAL, ANDROID_LOG_FATAL}};

  __android_log_print(android_severity_mapping[severity], "QuestXR", "%s", msg.c_str());
}
