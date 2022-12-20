#pragma once

#include "openxr-include.hpp"

#include <string>

#define CHECK_XRCMD(cmd) \
  CheckResult(cmd, __FILE__, __LINE__)

void CheckResult(XrResult result, const std::string &file, uint32_t line);
std::string GetXrVersionString(XrVersion ver);
void LogLayersAndExtensions();
void LogInstanceInfo(XrInstance instance);
void LogViewConfigurations(XrInstance instance, XrSystemId system_id);
void LogReferenceSpaces(XrSession session);
void LogSystemProperties(XrInstance instance, XrSystemId system_id);
void LogActionSourceName(XrSession session, XrAction action, const std::string &action_name);